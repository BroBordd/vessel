/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * GPU drawing, game side: see gfx.h. In GPU mode the SDL render calls become records in a shared
 * file that the Java app replays with OpenGL ES. In CPU mode they forward to real SDL untouched. */
#include "gfx.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* this file talks to the real SDL: undo the redirect from gfx.h */
#undef SDL_SetRenderDrawColor
#undef SDL_SetRenderDrawBlendMode
#undef SDL_RenderFillRect
#undef SDL_RenderFillRects
#undef SDL_RenderClear
#undef SDL_RenderSetClipRect
#undef SDL_SetRenderTarget
#undef SDL_CreateTexture
#undef SDL_DestroyTexture
#undef SDL_RenderCopy
#undef SDL_RenderPresent

static int gpu;
static int W, H;

static uint8_t  *map;
static uint32_t *hdr;
static uint32_t *slothdr, *rec;
static uint32_t  nrec, cur, frame_no;

/* the SDL renderer state the game relies on, tracked here so every frame can start fully defined */
static Uint8 cr, cg, cb, ca = 255;
static int blend;
static int clip_on; static SDL_Rect clip;
static int target;                                   /* 0 scene, 1 zoom texture */
static int saved_clip_on; static SDL_Rect saved_clip; /* SDL drops the clip when a texture becomes the target and restores it after */

static unsigned rects_frame, rects_max, warned;
static char dummy_renderer, dummy_texture;

static inline uint32_t pack_xy(int x, int y) {
    if (x < -32768) x = -32768; if (x > 32767) x = 32767;
    if (y < -32768) y = -32768; if (y > 32767) y = 32767;
    return (uint32_t)(x + 32768) | ((uint32_t)(y + 32768) << 16);
}
static inline uint32_t pack_wh(int w, int h) {
    if (w < 0) w = 0; if (w > 65535) w = 65535;
    if (h < 0) h = 0; if (h > 65535) h = 65535;
    return (uint32_t)w | ((uint32_t)h << 16);
}
static inline uint32_t color_word(void) {
    return (uint32_t)cr | ((uint32_t)cg << 8) | ((uint32_t)cb << 16) | ((uint32_t)ca << 24);
}

static inline void emit(uint32_t a, uint32_t b, uint32_t c, uint32_t type) {
    if (nrec >= GFX_CAP) return;
    uint32_t *p = rec + (size_t)nrec * 4;
    p[0] = a; p[1] = b; p[2] = c; p[3] = type;
    nrec++;
}

static void emit_clip(void) {
    if (clip_on) emit(pack_xy(clip.x, clip.y), pack_wh(clip.w, clip.h), 1, GFX_CLIP);
    else         emit(0, 0, 0, GFX_CLIP);
}

static void begin_frame(void) {
    slothdr = (uint32_t *)(map + GFX_HDR_BYTES + (size_t)cur * GFX_SLOT_BYTES);
    rec = slothdr + GFX_SLOT_HDR / 4;
    __atomic_store_n(&slothdr[0], 0u, __ATOMIC_RELEASE);     /* "being written" */
    nrec = 0; rects_frame = 0;
    emit(blend, 0, 0, GFX_BLEND);
    emit_clip();
    emit(target, 0, 0, GFX_TARGET);
}

int gfx_init_gpu(const char *path, int w, int h) {
    int fd = open(path, O_RDWR);
    if (fd < 0) { perror("gfx: open"); return -1; }
    struct stat st;
    if (fstat(fd, &st) != 0 || (size_t)st.st_size < (size_t)GFX_FILE_BYTES) {
        printf("gfx: %s is too small\n", path); fflush(stdout); close(fd); return -1;
    }
    void *m = mmap(NULL, GFX_FILE_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (m == MAP_FAILED) { perror("gfx: mmap"); return -1; }
    map = (uint8_t *)m; hdr = (uint32_t *)m;
    W = w; H = h;
    hdr[1] = (uint32_t)w; hdr[2] = (uint32_t)h; hdr[3] = GFX_NSLOTS; hdr[4] = GFX_CAP;
    hdr[5] = 0; hdr[6] = 0; hdr[7] = 0xFFFFFFFFu;
    cur = 0; frame_no = 0; blend = 0; clip_on = 0; target = 0;
    begin_frame();
    __atomic_store_n(&hdr[0], GFX_MAGIC, __ATOMIC_RELEASE);  /* the app starts reading once this is set */
    gpu = 1;
    printf("gfx: gpu mode, %dx%d, %d slots x %d records\n", w, h, GFX_NSLOTS, GFX_CAP);
    fflush(stdout);
    return 0;
}

int gfx_is_gpu(void) { return gpu; }
SDL_Renderer *gfx_dummy_renderer(void) { return (SDL_Renderer *)&dummy_renderer; }

unsigned gfx_rect_stat(void) { unsigned m = rects_max; rects_max = 0; return m; }

static int f_grey, f_red, f_gold;                    /* the colour filter, see gfx.h */

void gfx_set_gold(int gold) { f_gold = gold < 0 ? 0 : gold > 256 ? 256 : gold; }

void gfx_set_filter(int grey, int red) {
    f_grey = grey < 0 ? 0 : grey > 256 ? 256 : grey;
    f_red  = red  < 0 ? 0 : red  > 256 ? 256 : red;
}

static void filter_colour(Uint8 *R, Uint8 *G, Uint8 *B) {
    int r = *R, g = *G, b = *B;
    if (f_grey) {
        int lum = (299 * r + 587 * g + 114 * b) / 1000;
        r += (lum - r) * f_grey / 256; g += (lum - g) * f_grey / 256; b += (lum - b) * f_grey / 256;
        int dim = 256 - 56 * f_grey / 256;           /* up to a fifth darker */
        r = r * dim / 256; g = g * dim / 256; b = b * dim / 256;
    }
    if (f_red) {
        r += (255 - r) * 77 * f_red / (256 * 256);   /* a little brighter in the reds ... */
        g = g * (256 * 256 - 154 * f_red) / (256 * 256);   /* ... and the greens and blues drain */
        b = b * (256 * 256 - 166 * f_red) / (256 * 256);
    }
    if (f_gold) {                                    /* toward gold: dark parts a deep amber, light parts a pale gold */
        int lum = (299 * r + 587 * g + 114 * b) / 1000;
        int tr = 214 + 41 * lum / 255, tg = 150 + 86 * lum / 255, tb = 40 + 110 * lum / 255;
        r += (tr - r) * f_gold / 256; g += (tg - g) * f_gold / 256; b += (tb - b) * f_gold / 256;
    }
    *R = (Uint8)r; *G = (Uint8)g; *B = (Uint8)b;
}

int gfx_SetRenderDrawColor(SDL_Renderer *r, Uint8 R, Uint8 G, Uint8 B, Uint8 A) {
    if (f_grey || f_red || f_gold) filter_colour(&R, &G, &B);
    cr = R; cg = G; cb = B; ca = A;
    return gpu ? 0 : SDL_SetRenderDrawColor(r, R, G, B, A);
}

int gfx_SetRenderDrawBlendMode(SDL_Renderer *r, SDL_BlendMode m) {
    if (!gpu) return SDL_SetRenderDrawBlendMode(r, m);
    int b = (m == SDL_BLENDMODE_BLEND);
    if (b != blend) { blend = b; emit(b, 0, 0, GFX_BLEND); }
    return 0;
}

static void rect(int x, int y, int w, int h) {
    rects_frame++;
    if (w <= 0 || h <= 0 || x >= W || y >= H || x + w <= 0 || y + h <= 0) return;   /* nothing visible */
    if (nrec >= GFX_CAP - 8) {                                                        /* keep room for control records */
        if (!warned) { warned = 1; printf("gfx: record buffer full, dropping rects\n"); fflush(stdout); }
        return;
    }
    emit(pack_xy(x, y), pack_wh(w, h), color_word(), GFX_RECT);
}

int gfx_RenderFillRect(SDL_Renderer *r, const SDL_Rect *rc) {
    if (!gpu) { rects_frame++; return SDL_RenderFillRect(r, rc); }
    if (rc) rect(rc->x, rc->y, rc->w, rc->h); else rect(0, 0, W, H);
    return 0;
}

int gfx_RenderFillRects(SDL_Renderer *r, const SDL_Rect *rc, int n) {
    if (!gpu) { rects_frame += (unsigned)n; return SDL_RenderFillRects(r, rc, n); }
    for (int i = 0; i < n; i++) rect(rc[i].x, rc[i].y, rc[i].w, rc[i].h);
    return 0;
}

int gfx_RenderClear(SDL_Renderer *r) {
    if (!gpu) return SDL_RenderClear(r);
    emit(0, 0, color_word(), GFX_CLEAR);
    return 0;
}

int gfx_RenderSetClipRect(SDL_Renderer *r, const SDL_Rect *rc) {
    if (!gpu) return SDL_RenderSetClipRect(r, rc);
    if (rc) { clip_on = 1; clip = *rc; } else clip_on = 0;
    emit_clip();
    return 0;
}

int gfx_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t) {
    if (!gpu) return SDL_SetRenderTarget(r, t);
    int nt = t ? 1 : 0;
    if (nt == target) return 0;
    if (nt) { saved_clip_on = clip_on; saved_clip = clip; clip_on = 0; }
    else    { clip_on = saved_clip_on; clip = saved_clip; }
    target = nt;
    emit(nt, 0, 0, GFX_TARGET);
    emit_clip();
    return 0;
}

SDL_Texture *gfx_CreateTexture(SDL_Renderer *r, Uint32 fmt, int access, int w, int h) {
    if (!gpu) return SDL_CreateTexture(r, fmt, access, w, h);
    return (SDL_Texture *)&dummy_texture;     /* the only texture is the zoom target, the app owns it */
}

void gfx_DestroyTexture(SDL_Texture *t) {
    if (!gpu) SDL_DestroyTexture(t);
}

int gfx_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *src, const SDL_Rect *dst) {
    if (!gpu) return SDL_RenderCopy(r, t, src, dst);
    if (dst && !warned) { warned = 1; printf("gfx: RenderCopy with a destination rect is not supported\n"); fflush(stdout); }
    if (src) emit(pack_xy(src->x, src->y), pack_wh(src->w, src->h), 0, GFX_COPY);
    else     emit(pack_xy(0, 0), pack_wh(W, H), 0, GFX_COPY);
    return 0;
}

void gfx_RenderPresent(SDL_Renderer *r) {
    if (rects_frame > rects_max) rects_max = rects_frame;
    if (!gpu) { rects_frame = 0; SDL_RenderPresent(r); return; }

    /* publish: the records and their count first, then the generation, then the header, newest write last */
    slothdr[1] = nrec;
    if (++frame_no == 0) frame_no = 1;
    __atomic_store_n(&slothdr[0], frame_no, __ATOMIC_RELEASE);
    hdr[5] = cur;
    __atomic_store_n(&hdr[6], frame_no, __ATOMIC_RELEASE);

    /* next slot: round robin, so the one just superseded is the last to be overwritten, never the one the app is reading */
    uint32_t n = (cur + 1) % GFX_NSLOTS;
    if (n == __atomic_load_n(&hdr[7], __ATOMIC_ACQUIRE)) n = (n + 1) % GFX_NSLOTS;
    cur = n;
    begin_frame();
}
