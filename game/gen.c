/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * GENERATION (see gen.h). THE RUINS: one way through, no maze. the circle room (where we wake), a short passage north, the fallen hall (two rows of
 * broken columns, rubble, a niche in each side wall), a doorway, and the pit chamber: a crack runs right across it, wall to wall, with a ledge on the
 * far side (the second circle's spot and the sealed gate). the far ledge is NOT reachable on foot: that is the point of the pit.
 * the seed only moves things about (where the middle is, how long the hall is, which columns have fallen, where the niches are). */
#include "gen.h"
#include <string.h>

/* ---------- seeded random numbers (xorshift32) ---------- */
static uint32_t rs = 1;
void gen_seed(uint32_t seed) { rs = seed ? seed : 0x9E3779B9u; for (int i = 0; i < 4; i++) gen_rand(); }
uint32_t gen_rand(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
int gen_range(int lo, int hi) { return hi <= lo ? lo : lo + (int)(gen_rand() % (uint32_t)(hi - lo + 1)); }
float gen_float(void) { return (float)(gen_rand() >> 8) * (1.0f / 16777216.0f); }

/* ---------- the ruins ---------- */
#define TW 46                       /* the ruins' size in tiles */
#define TH 60

static uint8_t *G;
static int S;
#define AT(x, y) G[(y) * S + (x)]

static void carve(int x, int y, int w, int h) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) if (i > 1 && j > 1 && i < TW - 2 && j < TH - 2) AT(i, j) = GEN_FLOOR;
}

static int16_t dist[TH][TW];
static int flood(int sx, int sy) {                      /* distance over floor from (sx, sy), 4 neighbours; returns how many tiles it reached */
    static int16_t qx[TW * TH], qy[TW * TH];
    for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) dist[y][x] = -1;
    int head = 0, tail = 0, n = 0;
    qx[tail] = (int16_t)sx; qy[tail++] = (int16_t)sy; dist[sy][sx] = 0;
    while (head < tail) {
        int x = qx[head], y = qy[head++]; n++;
        static const int d4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int k = 0; k < 4; k++) {
            int nx = x + d4[k][0], ny = y + d4[k][1];
            if (nx < 0 || ny < 0 || nx >= TW || ny >= TH || dist[ny][nx] >= 0 || AT(nx, ny) != GEN_FLOOR) continue;
            dist[ny][nx] = (int16_t)(dist[y][x] + 1);
            qx[tail] = (int16_t)nx; qy[tail++] = (int16_t)ny;
        }
    }
    return n;
}

int gen_temple(uint8_t *grid, int stride, int max, uint32_t seed, GenInfo *info) {
    if (max < TW || max < TH) return 0;
    G = grid; S = stride;
    gen_seed(seed);
    for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) AT(x, y) = GEN_VOID;

    int cx = gen_range(20, 25);                                   /* the middle line everything hangs on */

    /* 1. the circle room (rows 47..55): big enough for the blood circle and the dead vessel beside it (ritual.c needs 9 x 7 at least) */
    int sw = gen_range(12, 14), sx0 = cx - sw / 2, sy0 = 47, sh = 9;
    carve(sx0, sy0, sw, sh);
    /* 2. the passage north (rows 42..46), three wide */
    carve(cx - 1, 42, 3, 5);
    /* 3. the fallen hall (rows 24..41) */
    int hw = gen_range(22, 24), hx0 = cx - hw / 2, hx1 = hx0 + hw - 1, hy0 = 24, hy1 = 41;
    carve(hx0, hy0, hw, hy1 - hy0 + 1);
    /* 4. a niche in each side wall of the hall (4 x 4, dead ends) */
    int ny_w = gen_range(27, 36), ny_e = gen_range(27, 36);
    carve(hx0 - 4, ny_w, 4, 4);
    carve(hx1 + 1, ny_e, 4, 4);
    /* 5. the doorway north (rows 20..23), six wide */
    carve(cx - 3, 20, 6, 4);
    /* 6. the pit chamber (rows 6..19): a south ledge (15..19), the crack (10..14, ragged), the far ledge (6..9) */
    int px0 = cx - 10, px1 = cx + 9;
    carve(px0, 6, px1 - px0 + 1, 14);
    int pit_x0 = 99, pit_y0 = 99, pit_x1 = -1, pit_y1 = -1;
    for (int x = px0; x <= px1; x++) {
        int top = 10 + (int)(gen_rand() % 2u), bot = 14 - (int)(gen_rand() % 2u);        /* 3 to 5 rows deep, a ragged line */
        if (x == px0 || x == px1) { top = 10; bot = 14; }
        for (int y = top; y <= bot; y++) {
            AT(x, y) = GEN_PIT;
            if (x < pit_x0) pit_x0 = x;
            if (x > pit_x1) pit_x1 = x;
            if (y < pit_y0) pit_y0 = y;
            if (y > pit_y1) pit_y1 = y;
        }
    }

    /* 7. walls: every rock tile touching floor or pit (corners too) */
    for (int y = 1; y < TH - 1; y++)
        for (int x = 1; x < TW - 1; x++) {
            if (AT(x, y) != GEN_VOID) continue;
            int near = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (AT(x + i, y + j) == GEN_FLOOR || AT(x + i, y + j) == GEN_PIT) near = 1;
            if (near) AT(x, y) = GEN_WALL;
        }

    /* 8. decoration: (REMOVED: the rows of columns and the rubble heaps were solid and blocked walking; the random draws below are kept as they were) was: two rows of columns
     * down the hall (every third row, some fallen into rubble), rubble heaps, never in the way of the middle line, never near the start */
    int sx = cx, sy = sy0 + sh / 2;
    for (int y = hy0 + 2; y <= hy1 - 2; y += 3) {
        int xs[2] = { hx0 + 3, hx1 - 3 };
        for (int k = 0; k < 2; k++) {
            float f = gen_float(); (void)f;
            (void)xs[k];                                                                 /* no columns, no rubble: nothing static stands in the way (the draws are kept so a seed makes the same temple otherwise) */
        }
    }
    for (int y = 3; y < TH - 3; y++)
        for (int x = 3; x < TW - 3; x++) {
            if (AT(x, y) != GEN_FLOOR || gen_float() > 0.05f) continue;
            if (x > cx - 3 && x < cx + 3 && y > 5 && y < 56) continue;                  /* the way through stays clear */
            if (y >= sy0 - 1) continue;                                                  /* nothing in the circle room or its passage */
            int open = 1;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (AT(x + i, y + j) != GEN_FLOOR) open = 0;
            (void)open;                                                                  /* (no rubble heaps either) */
        }
    int south_y = 17;                                                                    /* the south ledge, where we stand to look into the pit */
    flood(sx, sy);
    if (dist[south_y][cx] < 0)                                                           /* a column cut the way: take the decorations away */
        for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) if (AT(x, y) == GEN_PILLAR || AT(x, y) == GEN_RUBBLE) AT(x, y) = GEN_FLOOR;
    flood(sx, sy);

    /* 9. the sealed gate, in the top wall of the far ledge (the way out, past the pit) */
    int gx = cx, gy = 5;
    if (AT(gx, gy) == GEN_WALL) AT(gx, gy) = GEN_GATE;

    if (info) {
        info->w = TW; info->h = TH; info->spawn_x = sx; info->spawn_y = sy; info->gate_x = gx; info->gate_y = gy; info->rooms = 6;
        info->pit_x0 = pit_x0; info->pit_y0 = pit_y0; info->pit_x1 = pit_x1; info->pit_y1 = pit_y1;
        info->far_x = cx; info->far_y = 8;                  /* the far ledge: where the second circle's carvings will be (chunk 13) */
        info->edge_x = cx; info->edge_y = south_y;          /* the south ledge's middle */
    }
    return 1;
}
