/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "minimap.h"
#include "mapicon.h"
#include <math.h>

#define VIEW 26                     /* tiles across the window */

static int W, H, cell, side, bx, by, bt;

/* the window slides by FRACTIONS of a tile (like the game camera slides by pixels), so the map never hops a whole tile:
 * the tiles are drawn at fractional offsets and clipped to the inner square. a tile edge at map coordinate e sits at
 * ox + round((e - fx0) * side / VIEW); neighbouring runs share the same edge value, so there are no gaps or overlaps. */
#define EDGE(e, f0) ((int)floorf(((e) - (f0)) * side / VIEW + 0.5f))

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

/* a fill clipped to the inner square (the tiles overhang it by up to a tile while the window slides) */
static void fillc(SDL_Renderer *r, int ox, int oy, int x, int y, int w, int h) {
    int x1 = x + w, y1 = y + h;
    if (x < ox) x = ox;
    if (y < oy) y = oy;
    if (x1 > ox + side) x1 = ox + side;
    if (y1 > oy + side) y1 = oy + side;
    if (x1 > x && y1 > y) fill(r, x, y, x1 - x, y1 - y);
}

void minimap_draw(SDL_Renderer *r, const uint8_t *tiles, int stride, int mw, int mh,
                  const Rgb *pal, int npal, float ptx, float pty, int facing,
                  const MiniMark *marks, int nmarks, float t) {
    int ox = bx + bt, oy = by + bt;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 235); fill(r, bx, by, side + 2 * bt, side + 2 * bt);       /* frame */
    SDL_SetRenderDrawColor(r, 6, 8, 20, 245);      fill(r, ox, oy, side, side);

    /* which part of the map is visible: a VIEW x VIEW window centred on the player (fractional, so it slides), kept inside the map */
    float fx0 = ptx - VIEW / 2.0f, fy0 = pty - VIEW / 2.0f;
    if (mw >= VIEW) { if (fx0 < 0) fx0 = 0; if (fx0 > mw - VIEW) fx0 = (float)(mw - VIEW); } else fx0 = -(VIEW - mw) / 2.0f;
    if (mh >= VIEW) { if (fy0 < 0) fy0 = 0; if (fy0 > mh - VIEW) fy0 = (float)(mh - VIEW); } else fy0 = -(VIEW - mh) / 2.0f;
    int ix0 = (int)floorf(fx0), iy0 = (int)floorf(fy0);       /* first (partly) visible tile */

    for (int j = 0; j <= VIEW; j++) {                       /* one extra row / column: the one peeking in at the far edge */
        int ty = iy0 + j;
        if (ty < 0 || ty >= mh) continue;
        int yy0 = oy + EDGE(ty, fy0), yy1 = oy + EDGE(ty + 1, fy0);
        int i = 0;
        while (i <= VIEW) {                                 /* merge runs of the same tile into one rectangle */
            int tx = ix0 + i;
            if (tx < 0 || tx >= mw) { i++; continue; }
            int v = tiles[ty * stride + tx], run = 1;
            while (i + run <= VIEW && ix0 + i + run < mw && tiles[ty * stride + ix0 + i + run] == v) run++;
            Rgb c = v < npal ? pal[v] : (Rgb){ 0, 0, 0 };
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
            int xx0 = ox + EDGE(tx, fx0), xx1 = ox + EDGE(tx + run, fx0);
            fillc(r, ox, oy, xx0, yy0, xx1 - xx0, yy1 - yy0);
            i += run;
        }
    }

    for (int k = 0; k < nmarks; k++) {
        int mx = ox + (int)((marks[k].tx - fx0) * side / VIEW), my = oy + (int)((marks[k].ty - fy0) * side / VIEW);
        if (mx < ox || my < oy || mx >= ox + side || my >= oy + side) continue;
        if (marks[k].kind == 1) {                           /* the hole: a dark ring with a pale rim */
            SDL_SetRenderDrawColor(r, 235, 242, 255, 255); fill(r, mx - cell, my - cell / 2, 2 * cell + 1, cell + 1);
            SDL_SetRenderDrawColor(r, 20, 40, 120, 255);   fill(r, mx - cell + 1, my - cell / 2 + 1, 2 * cell - 1, cell - 1);
        } else if (marks[k].kind == 2 || marks[k].kind == 3 || marks[k].kind == 4 || marks[k].kind == 5 || marks[k].kind == 7) {     /* a found thing: its own little picture (chunk 60), about 3 tiles wide */
            int ic = cell * 3 / MAPICON_CELLS; if (ic < 1) ic = 1;
            int pic = marks[k].kind == 2 ? (marks[k].state ? MAPICON_SHRINE_BROKEN : MAPICON_SHRINE) : marks[k].kind == 3 ? MAPICON_GRAVE : marks[k].kind == 4 ? MAPICON_HAMMER : marks[k].kind == 5 ? MAPICON_HOUSE : MAPICON_TEMPLE;
            mapicon_draw(r, pic, mx, my, ic);
        } else {                                            /* an npc: gold dot with a dark edge */
            SDL_SetRenderDrawColor(r, 20, 14, 0, 255);     fill(r, mx - cell / 2 - 1, my - cell / 2 - 1, cell + 3, cell + 3);
            SDL_SetRenderDrawColor(r, 255, 214, 110, 255); fill(r, mx - cell / 2, my - cell / 2, cell + 1, cell + 1);
        }
    }

    /* the player: blinking white dot with a red core and a little nose showing where they face */
    int px = ox + (int)((ptx - fx0) * side / VIEW), py = oy + (int)((pty - fy0) * side / VIEW);
    int d = cell + 1;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);   fill(r, px - d / 2 - 1, py - d / 2 - 1, d + 2, d + 2);
    int blink = ((int)(t * 3.0f) & 1);
    SDL_SetRenderDrawColor(r, 255, blink ? 255 : 120, blink ? 255 : 120, 255); fill(r, px - d / 2, py - d / 2, d, d);
    int nx = facing == 2 ? -1 : facing == 3 ? 1 : 0, ny = facing == 0 ? 1 : facing == 1 ? -1 : 0;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    fill(r, px + nx * d - cell / 4, py + ny * d - cell / 4, cell / 2 + 1, cell / 2 + 1);
}
