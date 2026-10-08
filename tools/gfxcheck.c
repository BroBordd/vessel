/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * proves that the GPU recorder (game/gfx.c) describes exactly the picture the CPU renderer draws.
 * The same scripted scenes (menu, loading, world, zoom, dialogs) run twice:
 *   cpu: drawn with SDL's software renderer, as before
 *   gpu: every draw call recorded into the shared file, then the records are replayed the way the app's
 *        GfxRenderer.java does it (here with SDL's software renderer instead of OpenGL ES)
 * and the pixels of every checkpoint are compared. Build and run (needs the usual SDL2 dev files):
 *   cc -O1 $(sdl2-config --cflags) -Igame -DGFX_REDIRECT -include game/gfx.h -DSDL_GetTicks=fake_ticks \
 *      -o build/gfxcheck tools/gfxcheck.c $(ls game/*.c | grep -v game/game.c) $(sdl2-config --libs) -lm
 *   mkdir -p build/cpu build/gpu
 *   build/gfxcheck cpu build/cpu && build/gfxcheck gpu build/gpu && build/gfxcheck cmp build/cpu build/gpu
 * (-DSDL_GetTicks=fake_ticks makes the little bobbing animation deterministic. each run writes ~60 MB of raw frames.) */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "world.h"
#include "dialog.h"
#include "talk.h"
#include "convo.h"
#include "story.h"
#include "lang.h"
#include "menu.h"
#include "loading.h"
#include "nowplaying.h"
#include "gfx.h"

#define W 540
#define H 1170
#define DT 0.016f

static Uint32 fake_ms;
Uint32 fake_ticks(void) { return fake_ms; }

static int gpu_mode;
static const char *outdir;
static FILE *index_file;
static int cp_n;
static SDL_Surface *surf;           /* cpu: the game's target. gpu: the replay target */
static SDL_Renderer *rr;            /* software renderer on surf */
static SDL_Texture *zt;             /* replay only: the zoom texture */
static uint8_t *gmap;               /* gpu: the shared record file */
static uint32_t max_records;        /* gpu: the biggest frame seen, in records (capacity is GFX_CAP) */

/* ---- replay: what GfxRenderer.java does, with real SDL calls (so un-redirect them) ---- */
#pragma push_macro("SDL_SetRenderDrawColor")
#pragma push_macro("SDL_SetRenderDrawBlendMode")
#pragma push_macro("SDL_RenderFillRect")
#pragma push_macro("SDL_RenderClear")
#pragma push_macro("SDL_RenderSetClipRect")
#pragma push_macro("SDL_SetRenderTarget")
#pragma push_macro("SDL_RenderCopy")
#undef SDL_SetRenderDrawColor
#undef SDL_SetRenderDrawBlendMode
#undef SDL_RenderFillRect
#undef SDL_RenderClear
#undef SDL_RenderSetClipRect
#undef SDL_SetRenderTarget
#undef SDL_RenderCopy
static void replay(const uint32_t *rec, uint32_t n) {
    for (uint32_t i = 0; i < n; i++, rec += 4) {
        uint32_t a = rec[0], b = rec[1], c = rec[2];
        int x = (int)(a & 0xFFFF) - 32768, y = (int)(a >> 16) - 32768, w = (int)(b & 0xFFFF), h = (int)(b >> 16);
        SDL_Rect q = { x, y, w, h };
        switch (rec[3]) {
        case GFX_RECT:
            SDL_SetRenderDrawColor(rr, c & 255, (c >> 8) & 255, (c >> 16) & 255, c >> 24);
            SDL_RenderFillRect(rr, &q);
            break;
        case GFX_BLEND:  SDL_SetRenderDrawBlendMode(rr, a ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE); break;
        case GFX_CLIP:   SDL_RenderSetClipRect(rr, c ? &q : NULL); break;
        case GFX_TARGET: SDL_SetRenderTarget(rr, a ? zt : NULL); break;
        case GFX_COPY:   SDL_RenderCopy(rr, zt, &q, NULL); break;
        case GFX_CLEAR:
            SDL_SetRenderDrawColor(rr, c & 255, (c >> 8) & 255, (c >> 16) & 255, c >> 24);
            SDL_RenderClear(rr);
            break;
        }
    }
}
#pragma pop_macro("SDL_SetRenderDrawColor")
#pragma pop_macro("SDL_SetRenderDrawBlendMode")
#pragma pop_macro("SDL_RenderFillRect")
#pragma pop_macro("SDL_RenderClear")
#pragma pop_macro("SDL_RenderSetClipRect")
#pragma pop_macro("SDL_SetRenderTarget")
#pragma pop_macro("SDL_RenderCopy")

static void present(SDL_Renderer *r) {
    fake_ms += 16;
    SDL_RenderPresent(r);                      /* cpu: real present. gpu: publishes the recorded frame */
    if (gpu_mode) {
        const uint32_t *hd = (const uint32_t *)gmap;
        const uint32_t *slot = (const uint32_t *)(gmap + GFX_HDR_BYTES + (size_t)hd[5] * GFX_SLOT_BYTES);
        if (slot[1] > max_records) max_records = slot[1];
        replay(slot + GFX_SLOT_HDR / 4, slot[1]);
    }
}

static void checkpoint(const char *name) {
    char path[512], file[128];
    snprintf(file, sizeof file, "cp_%02d_%s.raw", cp_n++, name);
    snprintf(path, sizeof path, "%s/%s", outdir, file);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(2); }
    fwrite(surf->pixels, 1, (size_t)W * H * 4, f);
    fclose(f);
    fprintf(index_file, "%s\n", file);
    printf("checkpoint %s\n", file);
}

/* ---- the scenes ---- */
static SDL_Renderer *R;                    /* what the game draws with: cpu = rr, gpu = the recorder's stand-in */
static void np(void) { nowplaying_update(DT); nowplaying_draw(R); }
static void world_frame(void) { world_update(DT); world_draw(R); np(); present(R); }
static void world_run(float sec) { for (float t = 0; t < sec; t += DT) world_frame(); }
static void world_tap(int x, int y) { world_touch(0, x, y); world_frame(); world_touch(1, x, y); world_frame(); }
static void dialog_frame(void) { SDL_SetRenderDrawColor(R, 126, 188, 246, 255); SDL_RenderClear(R); dialog_draw(R); np(); present(R); }

static void scenes(void) {
    lang_seed(7);
    nowplaying_init(W, H);

    menu_init(W, H);
    for (int i = 0; i < 40; i++) { menu_update(DT); menu_draw(R); np(); present(R); if (i == 0 || i == 39) checkpoint("menu"); }

    loading_init(W, H);
    for (int i = 0; i < 600; i++) {
        if (loading_update(DT)) break;
        loading_draw(R); np(); present(R);
        if (i == 10 || i == 100) checkpoint("loading");
    }

    world_init(W, H);
    world_frame(); checkpoint("world_first");
    world_run(1.5f); checkpoint("world_intro");
    for (int i = 0; i < 2; i++) { world_run(1.5f); world_tap(W / 2, H / 2); }
    world_run(1.0f); checkpoint("world");
    {   /* walk up to Dea, press the interact button, and watch the zoom lerp in */
        int u1 = 26 * W / 360, sr = 58 * W / 360, sx = u1 + sr, sy = H - u1 - sr;
        world_touch(0, sx, sy); world_touch(2, sx, sy - 60);
        world_run(1.9f); checkpoint("world_walk");
        world_touch(1, sx, sy - 60); world_run(0.3f);
        int br = 44 * W / 360, bx = W - u1 - br, by = H - u1 - br;
        world_tap(bx, by);
        for (int i = 0; i < 12; i++) { world_run(0.1f); if (i % 3 == 0) checkpoint("zoom_lerp"); }
        world_run(3.0f); checkpoint("zoom_full");
    }

    /* the conversation screens, driven directly */
    extern const Person DEA, VESSEL;
    static Persona mind = { "Dea", STYLE_DIVINE, 0, 30, 65, 8 };
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    dialog_init(W, H);
    convo_set_player(&VESSEL);
    convo_ask_questions(&DEA, &mind, NULL);
    for (float t = 0; t < 3.0f; t += DT) dialog_update(DT);
    dialog_frame(); checkpoint("question");
    for (int y = H - 30; y > H - 120 && !talk_active(); y -= 4) for (int x = W - 30; x > W - 120 && !talk_active(); x -= 4) { dialog_touch(0, x, y); dialog_touch(1, x, y); }
    for (float t = 0; t < 0.5f; t += DT) dialog_update(DT);
    talk_debug_type("hello! who are you? thanks");
    for (float t = 0; t < 0.3f; t += DT) dialog_update(DT);
    dialog_frame(); checkpoint("talk");
}

/* ---- compare two runs ---- */
static uint8_t *slurp(const char *path, size_t n) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t *b = malloc(n);
    size_t got = fread(b, 1, n, f);
    fclose(f);
    if (got != n) { free(b); return NULL; }
    return b;
}

static int compare(const char *da, const char *db) {
    char path[512], name[256];
    snprintf(path, sizeof path, "%s/index.txt", da);
    FILE *ix = fopen(path, "r");
    if (!ix) { perror(path); return 2; }
    int bad = 0, total = 0;
    size_t n = (size_t)W * H * 4;
    while (fgets(name, sizeof name, ix)) {
        name[strcspn(name, "\r\n")] = 0;
        char pa[512], pb[512];
        snprintf(pa, sizeof pa, "%s/%s", da, name);
        snprintf(pb, sizeof pb, "%s/%s", db, name);
        uint8_t *a = slurp(pa, n), *b = slurp(pb, n);
        total++;
        if (!a || !b) { printf("MISSING %s\n", name); bad++; free(a); free(b); continue; }
        long diff = 0, first = -1;
        for (size_t i = 0; i < n; i += 4)
            if (memcmp(a + i, b + i, 4)) { if (first < 0) first = (long)(i / 4); diff++; }
        if (diff) {
            bad++;
            printf("DIFF %s: %ld of %d pixels differ, first at x=%ld y=%ld  cpu=%02x%02x%02x%02x gpu=%02x%02x%02x%02x\n",
                   name, diff, W * H, first % W, first / W,
                   a[first * 4], a[first * 4 + 1], a[first * 4 + 2], a[first * 4 + 3],
                   b[first * 4], b[first * 4 + 1], b[first * 4 + 2], b[first * 4 + 3]);
        } else printf("ok   %s\n", name);
        free(a); free(b);
    }
    fclose(ix);
    printf("%d of %d checkpoints identical\n", total - bad, total);
    return bad ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[1], "cmp")) return compare(argv[2], argv[3]);
    if (argc != 3 || (strcmp(argv[1], "cpu") && strcmp(argv[1], "gpu"))) {
        fprintf(stderr, "usage: gfxcheck cpu|gpu <outdir>   or   gfxcheck cmp <dir> <dir>\n");
        return 2;
    }
    gpu_mode = !strcmp(argv[1], "gpu");
    outdir = argv[2];
    char path[512];
    snprintf(path, sizeof path, "%s/index.txt", outdir);
    index_file = fopen(path, "w");
    if (!index_file) { perror(path); return 2; }

    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    if (!surf || !rr) { fprintf(stderr, "no software renderer\n"); return 2; }

    if (gpu_mode) {
        snprintf(path, sizeof path, "%s/gfx", outdir);
        int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0 || ftruncate(fd, GFX_FILE_BYTES) != 0) { perror(path); return 2; }
        gmap = mmap(NULL, GFX_FILE_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (gmap == MAP_FAILED) { perror("mmap"); return 2; }
        zt = SDL_CreateTexture(rr, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, W, H);   /* gfx mode is still off here, so this is the real SDL call */
        if (gfx_init_gpu(path, W, H) != 0) return 2;
        R = gfx_dummy_renderer();
    } else {
        R = rr;
    }
    scenes();
    fclose(index_file);
    printf("%s run done, %d checkpoints\n", gpu_mode ? "gpu" : "cpu", cp_n);
    if (gpu_mode) printf("biggest frame: %u records of %d\n", max_records, GFX_CAP);
    return 0;
}
