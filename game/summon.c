/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "summon.h"
#include <math.h>
#include <stdint.h>

#define RING_RX   11.0f             /* full ring radii in cells (a tile is 8): flat, like the hole's ellipse */
#define RING_RY   5.5f
#define BEAM_HALF 5                 /* the glow is 2*5+1 cells wide, the mid layer 7, the core 3 */

static int   on;
static float st;                    /* seconds since it began */
static int   last_rows, last_rx;

static uint32_t hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 374761393u + b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

void  summon_begin(void) { on = 1; st = 0; last_rows = last_rx = 0; }
void  summon_cancel(void) { on = 0; }
int   summon_active(void) { return on; }
float summon_time(void) { return st; }
int   summon_beam_rows(void) { return last_rows; }
int   summon_ring_radius(void) { return last_rx; }

int summon_update(float dt) {
    if (!on) return 0;
    st += dt;
    if (st >= SUMMON_TOTAL_T) { on = 0; return 1; }
    return 0;
}

/* 0..1 over the drop, in chunky steps */
static float drop_k(void) {
    float k = st / SUMMON_DROP_T; if (k > 1) k = 1;
    k = k * k;                                              /* it accelerates, like something falling */
    return k;
}
/* 0..1: how much of the light is still there (1 until the end begins) */
static float life(void) {
    float e = st - (SUMMON_DROP_T + SUMMON_HOLD_T);
    if (e <= 0) return 1.0f;
    float k = 1.0f - e / SUMMON_END_T;
    return k < 0 ? 0 : k;
}
static float ring_p(void) {                                 /* the ring's growth 0..1, in 12 steps */
    float k = (st - SUMMON_DROP_T) / SUMMON_RING_T;
    if (k < 0) k = 0;
    if (k > 1) k = 1;
    k = 1.0f - (1.0f - k) * (1.0f - k);
    return floorf(k * 12.0f) / 12.0f;
}

/* one half of an elliptical ring made of cells. far = the rows above the centre, near = the rest */
static void ring_half(SDL_Renderer *r, int cx, int cy, int cell, int far_half) {
    float p = ring_p(), lf = life();
    if (p <= 0 || lf <= 0) return;
    float rx = p * RING_RX, ry = p * RING_RY;
    if (rx < 1.0f) return;
    int R = (int)rx + 3, Rv = (int)ry + 3;
    int tw = (int)(st * 6.0f);                              /* the rim shimmers */
    for (int j = -Rv; j < Rv; j++) {
        if ((j < 0) != far_half) continue;
        for (int i = -R; i < R; i++) {
            float fx = i + 0.5f, fy = j + 0.5f;
            float d = fx * fx / (rx * rx) + fy * fy / (ry * ry);
            int a;
            if (d > 1.0f && d <= 1.34f) {                   /* the bright rim, one to two cells thick */
                a = ((i + j + tw) & 1) ? 235 : 190;
                SDL_SetRenderDrawColor(r, 255, 214, 92, (int)(a * lf));      /* gold: it has to read on a white cloud floor too */
            } else if (d <= 1.0f && d > 0.55f) {            /* a soft golden glow inside the rim, dithered into two bands */
                if (((i + j) & 1) && d < 0.8f) continue;
                SDL_SetRenderDrawColor(r, 255, 208, 84, (int)((d > 0.8f ? 150 : 100) * lf));
            } else if (d <= 0.55f) {                        /* the middle of the circle where the light lands */
                if (((i + j) & 1) == 0) continue;
                SDL_SetRenderDrawColor(r, 255, 224, 120, (int)(90 * lf));
            } else continue;
            SDL_Rect q = { cx + i * cell, cy + j * cell, cell, cell };
            SDL_RenderFillRect(r, &q);
        }
    }
    if (!far_half) last_rx = (int)rx;
}

static void bar(SDL_Renderer *r, int x, int y, int cells_w, int cell, int cr, int cg, int cb, int a) {
    if (a <= 0) return;
    SDL_SetRenderDrawColor(r, cr, cg, cb, a);
    SDL_Rect q = { x - (cells_w * cell) / 2, y, cells_w * cell, cell };     /* centred on x (the ring's centre too) */
    SDL_RenderFillRect(r, &q);
}

void summon_draw_back(SDL_Renderer *r, int fx, int fy, int cell, int w, int h) {
    (void)w; (void)h;
    last_rows = 0;
    if (!on || cell < 1) return;
    ring_half(r, fx, fy, cell, 1);

    /* the beam: a column from the top of the screen down to its head, which falls to the floor in chunky steps */
    float dk = drop_k(), lf = life();
    if (lf <= 0) return;
    int rows_total = fy / cell;                             /* cell rows between the top of the screen and the floor */
    int head = (int)floorf(dk * rows_total / 3.0f) * 3;     /* the head moves three rows at a time */
    if (st >= SUMMON_DROP_T) head = rows_total;
    if (head > rows_total) head = rows_total;
    int top_y = fy - rows_total * cell;                     /* a multiple of cells above the floor: on the same grid */
    int tick = (int)(st * 14.0f);
    float pulse = 0.5f + 0.5f * sinf(st * 9.0f);
    for (int row = 0; row < head; row++) {
        int y = top_y + row * cell;
        uint32_t hh = hash((uint32_t)row, (uint32_t)tick);
        int flick = (int)(hh & 1);                          /* the edges flicker one cell */
        float fade_top = row < 6 ? row / 6.0f : 1.0f;       /* the light begins softly at the top edge */
        float headk = 1.0f;
        if (st < SUMMON_DROP_T && row >= head - 4) headk = 1.4f;     /* the head of a falling beam is the brightest part */
        int wl = (int)(lf * (BEAM_HALF * 2 + 1)); if (wl < 1) wl = 1;
        int wm = (int)(lf * 7);                  if (wm < 1) wm = 1;
        int wc = (int)(lf * 3);                  if (wc < 1) wc = 1;
        wl |= 1; wm |= 1; wc |= 1;                           /* odd widths: centred on the floor spot */
        int a_glow = (int)((70 + 30 * pulse) * lf * fade_top * headk);
        int a_mid  = (int)((150 + 40 * pulse) * lf * fade_top * headk);
        int a_core = (int)(235 * lf * fade_top);
        bar(r, fx, y, wl + (flick ? 2 : 0), cell, 255, 206, 84, a_glow > 255 ? 255 : a_glow);
        bar(r, fx, y, wm,                    cell, 255, 232, 150, a_mid > 255 ? 255 : a_mid);
        bar(r, fx, y, wc,                    cell, 255, 255, 240, a_core > 255 ? 255 : a_core);
    }
    last_rows = head;
}

void summon_draw_front(SDL_Renderer *r, int fx, int fy, int cell) {
    if (!on || cell < 1) return;
    ring_half(r, fx, fy, cell, 0);
}
