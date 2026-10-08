/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "loading.h"
#include "font.h"
#include <string.h>

static int   W, H;
static float t;

void loading_init(int w, int h) { W = w; H = h; t = 0; }

int loading_update(float dt) {
    t += dt;
    return t >= LOADING_SECONDS;
}

void loading_draw(SDL_Renderer *r) {
    float u = (W < H ? W : H) / 360.0f;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    /* "LOADING" + 0..3 dots, left-aligned on the full-width string so it doesn't jitter */
    int cell = (int)(4 * u); if (cell < 2) cell = 2;
    const char *full = "LOADING...";
    int dots = (int)(t * 3.0f) % 4;
    char buf[16]; memcpy(buf, full, 7 + (size_t)dots); buf[7 + dots] = 0;
    int tx = (W - font_width(full, cell)) / 2, ty = (int)(H * 0.46f);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    font_draw(r, buf, tx, ty, cell);

    /* progress bar: outlined box, chunky blocks fill left to right */
    int bw = (int)(200 * u), bh = (int)(14 * u), bt = (int)(2 * u); if (bt < 1) bt = 1;
    int bx = (W - bw) / 2, by = ty + font_height(cell) + (int)(24 * u);
    SDL_Rect outer = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &outer);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_Rect inner = { bx + bt, by + bt, bw - 2 * bt, bh - 2 * bt };
    SDL_RenderFillRect(r, &inner);

    float p = t / LOADING_SECONDS; if (p > 1) p = 1;
    int blocks = 20, gapw = bt, avail = bw - 4 * bt;
    int blk = (avail - (blocks - 1) * gapw) / blocks;
    int filled = (int)(p * blocks + 0.5f);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    for (int i = 0; i < filled; i++) {
        SDL_Rect q = { bx + 2 * bt + i * (blk + gapw), by + 2 * bt, blk, bh - 4 * bt };
        SDL_RenderFillRect(r, &q);
    }
}
