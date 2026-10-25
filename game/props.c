/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * props of the ruins, see props.h. */
#include "props.h"
#include <math.h>

static int   placed, broken;            /* broken: the club smashed it (chunk 12): it never lies there again */
static float rtx, rty;                  /* the rock's feet, in tiles */

#define ROCK_HW 5.0f                    /* half width, cells */
#define ROCK_H  7.0f                    /* height, cells */

void props_reset(void) { placed = broken = 0; rtx = rty = 0; }
int  props_rock_placed(void) { return placed || broken; }
int  props_rock_break(float *tx, float *ty) { if (!placed) return 0; *tx = rtx; *ty = rty; placed = 0; broken = 1; return 1; }
void props_rock_place(float tx, float ty) { placed = 1; rtx = tx; rty = ty; }

int props_rock_box(int px, int tile, float *x0, float *y0, float *x1, float *y1) {
    if (!placed) return 0;
    float fx = rtx * (float)tile, fy = rty * (float)tile;
    *x0 = fx - ROCK_HW * (float)px; *x1 = fx + ROCK_HW * (float)px; *y0 = fy - ROCK_H * (float)px; *y1 = fy;
    return 1;
}
void props_rock_shift(float dx, float dy, int tile) { if (placed && tile > 0) { rtx += dx / (float)tile; rty += dy / (float)tile; } }

int props_rock_hit(float fx, float fy, int px, int tile) {
    float x0, y0, x1, y1;
    if (!props_rock_box(px, tile, &x0, &y0, &x1, &y1)) return 0;
    return fx + 3.0f * px > x0 && fx - 3.0f * px < x1 && fy > y0 && fy - 3.0f * px < y1;
}
float props_rock_gap(float fx, float fy, int px, int tile) {
    float x0, y0, x1, y1;
    if (!props_rock_box(px, tile, &x0, &y0, &x1, &y1)) return 1e9f;
    float dx = fmaxf(fmaxf(x0 - (fx + 3.0f * px), (fx - 3.0f * px) - x1), 0.0f), dy = fmaxf(fmaxf(y0 - fy, (fy - 3.0f * px) - y1), 0.0f);
    return sqrtf(dx * dx + dy * dy) / (float)px;
}

static void cells(SDL_Renderer *r, int x, int y, int w, int h, int px) { SDL_Rect q = { x, y, w * px, h * px }; SDL_RenderFillRect(r, &q); }

void props_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken) {
    if (!placed) return;
    float k = 1.0f - darken; if (k < 0) k = 0;
#define C(R, G, B) SDL_SetRenderDrawColor(r, (Uint8)((R) * k), (Uint8)((G) * k), (Uint8)((B) * k), 255)
    int fx = (int)lroundf(rtx * (float)tile) - cam_x, fy = (int)lroundf(rty * (float)tile) - cam_y;
    static const int HW[7] = { 3, 4, 5, 5, 5, 5, 4 };          /* half widths of the seven rows, top to bottom: a lump, wider in the middle */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 90);                    /* the shadow it sits in */
    cells(r, fx - 6 * px, fy - 2 * px, 12, 3, px);
    for (int j = 0; j < 7; j++) {
        int y = fy - (7 - j) * px, w = HW[j];
        if (j >= 5) C(70, 66, 84); else C(104, 100, 116);       /* the base, darker low down */
        cells(r, fx - w * px, y, 2 * w, 1, px);
        if (j <= 3) { C(150, 146, 164); cells(r, fx - w * px, y, j == 0 ? 2 : 3, 1, px); }          /* light from the upper left */
        if (j >= 3) { C(66, 62, 80); cells(r, fx + (w - 2) * px, y, 2, 1, px); }                    /* the right side in shade */
    }
    C(52, 48, 64);                                              /* two cracks */
    cells(r, fx + px, fy - 5 * px, 1, 2, px); cells(r, fx + 2 * px, fy - 3 * px, 1, 2, px); cells(r, fx - 2 * px, fy - 3 * px, 1, 1, px);
#undef C
}
