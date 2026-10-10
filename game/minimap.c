/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "minimap.h"
#include <math.h>

#define VIEW 26                     /* tiles across the window */

static int W, H, cell, side, bx, by, bt;

/* the box is `width` px wide (frame included), so the inner square (side) is not always a multiple of
 * VIEW: tile i starts at TX(i) = i * side / VIEW (integer maths), so tiles are 1px wider/narrower here
 * and there instead of the box having to be a multiple of the tile size. */
#define TX(i) ((int)((long)(i) * side / VIEW))

void minimap_init(int w, int h, int right_edge, int top, int width) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    side = width - 2 * bt;
    cell = side / VIEW; if (cell < 2) cell = 2;                 /* nominal tile size, for the dots */
    bx = right_edge - width;
    by = top;
}
int minimap_bottom(void) { return by + side + 2 * bt; }
int minimap_hit(int x, int y) { return x >= bx && y >= by && x < bx + side + 2 * bt && y < by + side + 2 * bt; }

static void fill(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y, w, h }; SDL_RenderFillRect(r, &q); }

void minimap_draw(SDL_Renderer *r, const uint8_t *tiles, int stride, int mw, int mh,
                  const Rgb *pal, int npal, float ptx, float pty, int facing,
                  const MiniMark *marks, int nmarks, float t) {
    int ox = bx + bt, oy = by + bt;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 235); fill(r, bx, by, side + 2 * bt, side + 2 * bt);       /* frame */
    SDL_SetRenderDrawColor(r, 6, 8, 20, 245);      fill(r, ox, oy, side, side);

    /* which tiles are visible: a VIEW x VIEW window around the player, kept inside the map */
    int x0 = (int)ptx - VIEW / 2, y0 = (int)pty - VIEW / 2;
    if (mw >= VIEW) { if (x0 < 0) x0 = 0; if (x0 > mw - VIEW) x0 = mw - VIEW; } else x0 = -(VIEW - mw) / 2;
    if (mh >= VIEW) { if (y0 < 0) y0 = 0; if (y0 > mh - VIEW) y0 = mh - VIEW; } else y0 = -(VIEW - mh) / 2;

    for (int j = 0; j < VIEW; j++) {
        int ty = y0 + j;
        if (ty < 0 || ty >= mh) continue;
        int i = 0;
        while (i < VIEW) {                                  /* merge runs of the same tile into one rectangle */
            int tx = x0 + i;
            if (tx < 0 || tx >= mw) { i++; continue; }
            int v = tiles[ty * stride + tx], run = 1;
            while (i + run < VIEW && x0 + i + run >= 0 && x0 + i + run < mw && tiles[ty * stride + x0 + i + run] == v) run++;
            Rgb c = v < npal ? pal[v] : (Rgb){ 0, 0, 0 };
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
            fill(r, ox + TX(i), oy + TX(j), TX(i + run) - TX(i), TX(j + 1) - TX(j));
            i += run;
        }
    }

    for (int k = 0; k < nmarks; k++) {
        int mx = ox + (int)((marks[k].tx - x0) * side / VIEW), my = oy + (int)((marks[k].ty - y0) * side / VIEW);
        if (mx < ox || my < oy || mx >= ox + side || my >= oy + side) continue;
        if (marks[k].kind == 1) {                           /* the hole: a dark ring with a pale rim */
            SDL_SetRenderDrawColor(r, 235, 242, 255, 255); fill(r, mx - cell, my - cell / 2, 2 * cell + 1, cell + 1);
            SDL_SetRenderDrawColor(r, 20, 40, 120, 255);   fill(r, mx - cell + 1, my - cell / 2 + 1, 2 * cell - 1, cell - 1);
        } else if (marks[k].kind == 2) {                    /* Dia's shrine: a violet diamond-ish block with a pale core */
            SDL_SetRenderDrawColor(r, 30, 10, 60, 255);    fill(r, mx - cell / 2 - 1, my - cell / 2 - 1, cell + 3, cell + 3);
            SDL_SetRenderDrawColor(r, 176, 120, 255, 255); fill(r, mx - cell / 2, my - cell / 2, cell + 1, cell + 1);
            SDL_SetRenderDrawColor(r, 240, 226, 255, 255); fill(r, mx - 1, my - 1, 3, 3);
        } else if (marks[k].kind == 4) {                    /* an item lying around (the hammer): a small warm block with a pale core */
            SDL_SetRenderDrawColor(r, 40, 24, 8, 255);     fill(r, mx - cell / 2 - 1, my - cell / 2 - 1, cell + 3, cell + 3);
            SDL_SetRenderDrawColor(r, 232, 164, 72, 255);  fill(r, mx - cell / 2, my - cell / 2, cell + 1, cell + 1);
            SDL_SetRenderDrawColor(r, 255, 244, 214, 255); fill(r, mx - 1, my - 1, 3, 3);
        } else if (marks[k].kind == 3) {                    /* a grave: a grey block with a pale top, like a little headstone */
            SDL_SetRenderDrawColor(r, 20, 22, 28, 255);    fill(r, mx - cell / 2 - 1, my - cell / 2 - 1, cell + 3, cell + 3);
            SDL_SetRenderDrawColor(r, 150, 154, 164, 255); fill(r, mx - cell / 2, my - cell / 2, cell + 1, cell + 1);
            SDL_SetRenderDrawColor(r, 206, 210, 220, 255); fill(r, mx - cell / 2, my - cell / 2, cell + 1, 2);
        } else {                                            /* an npc: gold dot with a dark edge */
            SDL_SetRenderDrawColor(r, 20, 14, 0, 255);     fill(r, mx - cell / 2 - 1, my - cell / 2 - 1, cell + 3, cell + 3);
            SDL_SetRenderDrawColor(r, 255, 214, 110, 255); fill(r, mx - cell / 2, my - cell / 2, cell + 1, cell + 1);
        }
    }

    /* the player: blinking white dot with a red core and a little nose showing where they face */
    int px = ox + (int)((ptx - x0) * side / VIEW), py = oy + (int)((pty - y0) * side / VIEW);
    int d = cell + 1;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);   fill(r, px - d / 2 - 1, py - d / 2 - 1, d + 2, d + 2);
    int blink = ((int)(t * 3.0f) & 1);
    SDL_SetRenderDrawColor(r, 255, blink ? 255 : 120, blink ? 255 : 120, 255); fill(r, px - d / 2, py - d / 2, d, d);
    int nx = facing == 2 ? -1 : facing == 3 ? 1 : 0, ny = facing == 0 ? 1 : facing == 1 ? -1 : 0;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    fill(r, px + nx * d - cell / 4, py + ny * d - cell / 4, cell / 2 + 1, cell / 2 + 1);
}
