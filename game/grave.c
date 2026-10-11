/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "grave.h"
#include "gfx.h"
#include "audio.h"
#include <math.h>
#include <string.h>

#define GREY_PHOTO 232              /* gfx_set_filter grey, 0..256: nearly colourless, like an old photo */

static int s, tile, count;
static struct { int tx, ty; const Person *who; int open; float t; } g[GRAVE_MAX];       /* open: the target (1 out, 0 in); t: 0 body in .. 1 body out */

/* the pull (chunk 30) */
#define PULL_RANGE  2.0f            /* tiles from the stone's feet: this close and the button shows the grip */
#define PULL_LEAVE  2.5f
#define PULL_DECAY_T 3.0f           /* seconds for a full bar to creep back after letting go */
static struct { int i, on, in_range; float prog, tick; void (*cb)(void); } pull = { -1 };

void grave_reset(int pixel_scale, int tile_size) { s = pixel_scale; tile = tile_size; count = 0; memset(&pull, 0, sizeof pull); pull.i = -1; }

int grave_place(int tile_x, int tile_y, const Person *dead) {
    if (count >= GRAVE_MAX || !dead) return -1;
    g[count].tx = tile_x; g[count].ty = tile_y; g[count].who = dead; g[count].open = 0; g[count].t = 0;
    return count++;
}

int   grave_count(void) { return count; }

void  grave_open(int i)  { if (i >= 0 && i < count) g[i].open = 1; }
void  grave_close(int i) { if (i >= 0 && i < count) g[i].open = 0; }
void  grave_set_open(int i, int open, int instant) {
    if (i < 0 || i >= count) return;
    g[i].open = open ? 1 : 0;
    if (instant) g[i].t = open ? 1.0f : 0.0f;
}
void  grave_update(float dt) {
    for (int i = 0; i < count; i++) {
        if (g[i].open && g[i].t < 1.0f) { g[i].t += dt / GRAVE_SLIDE_T; if (g[i].t > 1.0f) g[i].t = 1.0f; }
        else if (!g[i].open && g[i].t > 0.0f) { g[i].t -= dt / GRAVE_SLIDE_T; if (g[i].t < 0.0f) g[i].t = 0.0f; }
    }
}
int   grave_is_open(int i) { return i >= 0 && i < count && (g[i].open || g[i].t > 0.0f); }
int   grave_body_out(int i) { return i >= 0 && i < count && g[i].open && g[i].t >= 1.0f; }
int   grave_body_moving(int i) { return i >= 0 && i < count && ((g[i].open && g[i].t < 1.0f) || (!g[i].open && g[i].t > 0.0f)); }
void  grave_body_tile(int i, float *x, float *y) { *x = g[i].tx + 0.5f + (float)GRAVE_BODY_DX * s / tile; *y = g[i].ty + 0.9f; }

void  grave_pull_enable(int i, void (*on_done)(void)) {
    if (i < 0 || i >= count) return;
    pull.i = i; pull.on = 1; pull.in_range = 0; pull.prog = pull.tick = 0; pull.cb = on_done;
}
int   grave_pull_enabled(void) { return pull.on; }
int   grave_pull_index(void) { return pull.i; }
float grave_pull_progress(void) { return pull.prog; }
int   grave_pull_near(float pxw, float pyw, int allow) {
    if (!pull.on || !allow || pull.i < 0 || pull.i >= count) { pull.in_range = 0; return 0; }
    float dx = pxw - (g[pull.i].tx + 0.5f) * tile, dy = pyw - (g[pull.i].ty + 0.9f) * tile;
    float d = sqrtf(dx * dx + dy * dy) / tile;
    if (!pull.in_range && d < PULL_RANGE) pull.in_range = 1;
    else if (pull.in_range && d > PULL_LEAVE) pull.in_range = 0;
    return pull.in_range;
}
int   grave_pull_update(float dt, int holding) {
    if (!pull.on) return 0;
    if (holding) {
        pull.prog += dt / GRAVE_PULL_HOLD_T;
        pull.tick -= dt;
        if (pull.tick <= 0) { pull.tick = 0.3f; sfx_blip(0.4f + 0.4f * pull.prog); }    /* a slow, low tick: heavy work */
        if (pull.prog >= 1.0f) {
            pull.prog = 1.0f; pull.on = 0; pull.in_range = 0;
            sfx_blip(0.35f);
            if (pull.cb) { void (*cb)(void) = pull.cb; pull.cb = NULL; cb(); }
            pull.prog = 0;                                  /* finished: nothing left filled */
            return 1;
        }
    } else {
        pull.tick = 0;
        if (pull.prog > 0) { pull.prog -= dt / PULL_DECAY_T; if (pull.prog < 0) pull.prog = 0; }
    }
    return 0;
}
void  grave_tile(int i, float *x, float *y) { *x = g[i].tx + 0.5f; *y = g[i].ty + 0.9f; }
const Person *grave_person(int i) { return g[i].who; }
float grave_foot_y(int i) { return (g[i].ty + 0.9f) * tile; }

int grave_collides(float fx, float fy, float half_w, float h) {
    for (int i = 0; i < count; i++)
        if (fabsf(fx - (g[i].tx + 0.5f) * tile) < half_w + 7.0f * s && fabsf(fy - grave_foot_y(i)) < h + 1.0f * s) return 1;
    return 0;
}

/* the old photo: the portrait is a 10 x 10 sprite, drawn at half the world's pixel size so it is a small picture on the stone */
static int photo_scale(void) { int ps = s / 2; return ps < 1 ? 1 : ps; }

void grave_portrait_rect(int i, int cam_x, int cam_y, int *x, int *y, int *w, int *h) {
    int fx = (int)((g[i].tx + 0.5f) * tile) - cam_x, fy = (int)((g[i].ty + 0.9f) * tile) - cam_y;
    int p = 10 * photo_scale();
    *w = p; *h = p;
    *x = fx - p / 2;
    *y = fy - 17 * s + s;                                   /* the frame is one unit thick and starts 17 units above the feet */
}

/* ---------- drawing: pixel art, 18 units wide, 19 tall. (x, y) = feet centre. ---------- */
static void box(SDL_Renderer *r, int x, int y, int ux, int uy, int uw, int uh) {      /* units, relative to the feet centre */
    SDL_Rect q = { x + ux * s, y + uy * s, uw * s, uh * s };
    SDL_RenderFillRect(r, &q);
}
static void col(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }

void grave_draw(SDL_Renderer *r, int i, int cam_x, int cam_y) {
    if (i < 0 || i >= count) return;
    int x = (int)((g[i].tx + 0.5f) * tile) - cam_x, y = (int)((g[i].ty + 0.9f) * tile) - cam_y;

    col(r, 0, 0, 0, 70);        box(r, x, y, -10, -1, 20, 2);                          /* ground shadow */

    /* the stone: a slab with a rounded top and a plinth, lit from the left */
    col(r, 98, 102, 112, 255);  box(r, x, y, -7, -5, 14, 2);                           /* plinth */
    col(r, 150, 154, 164, 255); box(r, x, y, -7, -5, 14, 1);
    col(r, 140, 144, 154, 255); box(r, x, y, -4, -19, 8, 1);                           /* slab: top, shoulders, body */
    box(r, x, y, -5, -18, 10, 1); box(r, x, y, -6, -17, 12, 12);
    col(r, 178, 182, 192, 255); box(r, x, y, -4, -19, 8, 1); box(r, x, y, -5, -18, 1, 1); box(r, x, y, -6, -17, 1, 12);   /* light edge */
    col(r, 104, 108, 118, 255); box(r, x, y, 5, -17, 1, 12); box(r, x, y, 4, -18, 1, 1); /* shadow edge */
    col(r, 112, 140, 100, 255); box(r, x, y, -5, -7, 2, 1); box(r, x, y, 3, -9, 1, 2);   /* a little moss */

    /* the framed photo: a dark frame one unit thick, a pale card behind the picture, the picture itself greyed */
    int px_, py_, pw, ph; grave_portrait_rect(i, cam_x, cam_y, &px_, &py_, &pw, &ph);
    SDL_Rect frame = { px_ - s, py_ - s, pw + 2 * s, ph + 2 * s };
    col(r, 46, 36, 30, 255);    SDL_RenderFillRect(r, &frame);
    gfx_set_filter(GREY_PHOTO, 0);
    col(r, 214, 208, 192, 255); { SDL_Rect card = { px_, py_, pw, ph }; SDL_RenderFillRect(r, &card); }
    char_draw_portrait(r, g[i].who, px_, py_, photo_scale(), 0);
    gfx_set_filter(0, 0);                                                              /* (world.c puts its own filter back) */
    col(r, 92, 96, 106, 255);   box(r, x, y, -3, -8, 6, 1); box(r, x, y, -2, -6, 4, 1); /* two engraved lines: a name nobody can read */

    /* the plot: a mound of earth in front of the stone, a few tufts of grass, one flower. OPEN (chunk 31): a dark pit instead of the mound, with the
     * earth piled up beside it, and the body in it / sliding out of it / lying on the grass */
    if (grave_is_open(i)) {
        col(r, 52, 38, 28, 255);   box(r, x, y, -9, -3, 18, 4);                        /* the pit */
        col(r, 34, 24, 18, 255);   box(r, x, y, -8, -2, 16, 2); box(r, x, y, -9, 0, 18, 1);
        col(r, 122, 90, 62, 255);  box(r, x, y, -10, -4, 1, 5);                        /* its lip, and the heap of loose earth at the end */
        col(r, 104, 76, 52, 255);  box(r, x, y, 9, -3, 5, 3); box(r, x, y, 10, -4, 3, 1);
        col(r, 78, 56, 40, 255);   box(r, x, y, 9, -1, 5, 1);
        float e = g[i].t; e = e * e * (3.0f - 2.0f * e);                                /* eased: it starts and stops gently */
        int bx = x + (int)((float)GRAVE_BODY_DX * e * s), by = y + s;
        gfx_set_filter(GREY_PHOTO, 0);
        if (g[i].t >= 1.0f) char_draw_lying(r, g[i].who, bx, by, s);
        else {                                                                          /* half in the ground: drawn through the pit's window and the open grass beside it */
            SDL_Rect pit = { x - 9 * s, y - 3 * s, 18 * s, 4 * s }, grass = { x + 9 * s, y - 14 * s, 40 * s, 22 * s };
            SDL_RenderSetClipRect(r, &pit);    char_draw_lying(r, g[i].who, bx, by, s);
            SDL_RenderSetClipRect(r, &grass);  char_draw_lying(r, g[i].who, bx, by, s);
            SDL_RenderSetClipRect(r, NULL);
        }
        gfx_set_filter(0, 0);
    } else {
    col(r, 92, 66, 46, 255);    box(r, x, y, -9, -3, 18, 3);
    col(r, 122, 90, 62, 255);   box(r, x, y, -9, -3, 18, 1);
    col(r, 70, 50, 36, 255);    box(r, x, y, -9, -1, 18, 1);
    }
    col(r, 76, 150, 66, 255);   box(r, x, y, -8, -4, 1, 1); box(r, x, y, -7, -5, 1, 2); box(r, x, y, -5, -4, 1, 1); box(r, x, y, 2, -4, 1, 1);
    col(r, 70, 140, 60, 255);   box(r, x, y, 7, -7, 1, 4); box(r, x, y, 8, -5, 1, 1);  /* the stem and a leaf */
    col(r, 240, 120, 170, 255); box(r, x, y, 7, -10, 1, 1); box(r, x, y, 6, -9, 1, 1); box(r, x, y, 8, -9, 1, 1); box(r, x, y, 7, -8, 1, 1);   /* the petals */
    col(r, 255, 214, 110, 255); box(r, x, y, 7, -9, 1, 1);                             /* its heart */
}
