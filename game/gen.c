/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * GENERATION (see gen.h). the deep temple: rooms are dropped into the rock without touching, joined by two-tile halls, then walled,
 * decorated (pillars in the big rooms, rubble) and checked: every floor tile must still be reachable from the start or the decorations go. */
#include "gen.h"
#include <string.h>

/* ---------- seeded random numbers (xorshift32) ---------- */
static uint32_t rs = 1;
void gen_seed(uint32_t seed) { rs = seed ? seed : 0x9E3779B9u; for (int i = 0; i < 4; i++) gen_rand(); }
uint32_t gen_rand(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
int gen_range(int lo, int hi) { return hi <= lo ? lo : lo + (int)(gen_rand() % (uint32_t)(hi - lo + 1)); }
float gen_float(void) { return (float)(gen_rand() >> 8) * (1.0f / 16777216.0f); }

/* ---------- the temple ---------- */
#define TW 56                       /* the temple's size in tiles */
#define TH 48
#define MAX_ROOMS 12
typedef struct { int x, y, w, h; } Room;

static uint8_t *G;
static int S;
#define AT(x, y) G[(y) * S + (x)]

static void carve(int x, int y, int w, int h) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) if (i > 1 && j > 1 && i < TW - 2 && j < TH - 2) AT(i, j) = GEN_FLOOR;
}
static void hall_h(int x0, int x1, int y) { if (x0 > x1) { int t = x0; x0 = x1; x1 = t; } carve(x0, y, x1 - x0 + 2, 2); }
static void hall_v(int y0, int y1, int x) { if (y0 > y1) { int t = y0; y0 = y1; y1 = t; } carve(x, y0, 2, y1 - y0 + 2); }
static void join(const Room *a, const Room *b) {        /* an L-shaped hall between two room centres (which way round is random) */
    int ax = a->x + a->w / 2, ay = a->y + a->h / 2, bx = b->x + b->w / 2, by = b->y + b->h / 2;
    if (gen_rand() & 1) { hall_h(ax, bx, ay); hall_v(ay, by, bx); }
    else                { hall_v(ay, by, ax); hall_h(ax, bx, by); }
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

    /* 1. rooms: dropped at random, kept only if they keep three tiles from every other */
    Room rm[MAX_ROOMS]; int nr = 0;
    for (int tries = 0; tries < 300 && nr < MAX_ROOMS; tries++) {
        Room c; c.w = gen_range(5, 12); c.h = gen_range(5, 10);
        c.x = gen_range(3, TW - c.w - 4); c.y = gen_range(3, TH - c.h - 4);
        int ok = 1;
        for (int i = 0; i < nr && ok; i++)
            if (c.x < rm[i].x + rm[i].w + 3 && c.x + c.w + 3 > rm[i].x && c.y < rm[i].y + rm[i].h + 3 && c.y + c.h + 3 > rm[i].y) ok = 0;
        if (ok) rm[nr++] = c;
    }
    if (nr < 2) return 0;
    for (int i = 0; i < nr; i++) carve(rm[i].x, rm[i].y, rm[i].w, rm[i].h);

    /* 2. halls: each room to the next one made, then two extra loops so it is not one long snake */
    for (int i = 0; i + 1 < nr; i++) join(&rm[i], &rm[i + 1]);
    for (int k = 0; k < 2 && nr > 3; k++) join(&rm[gen_range(0, nr - 1)], &rm[gen_range(0, nr - 1)]);

    /* 3. walls: every rock tile touching a floor tile (corners too) */
    for (int y = 1; y < TH - 1; y++)
        for (int x = 1; x < TW - 1; x++) {
            if (AT(x, y) != GEN_VOID) continue;
            int near = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (AT(x + i, y + j) == GEN_FLOOR) near = 1;
            if (near) AT(x, y) = GEN_WALL;
        }

    /* 4. the start: the middle of the first room */
    int sx = rm[0].x + rm[0].w / 2, sy = rm[0].y + rm[0].h / 2;
    int floors = 0;
    for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) if (AT(x, y) == GEN_FLOOR) floors++;

    /* 5. decoration: columns in the big rooms (every other tile, some missing: a ruin), rubble on open floor. never near the start */
    for (int i = 1; i < nr; i++) {
        if (rm[i].w < 7 || rm[i].h < 7) continue;
        for (int y = rm[i].y + 2; y <= rm[i].y + rm[i].h - 3; y += 2)
            for (int x = rm[i].x + 2; x <= rm[i].x + rm[i].w - 3; x += 2)
                if (gen_float() < 0.75f && AT(x, y) == GEN_FLOOR) AT(x, y) = GEN_PILLAR;
    }
    for (int y = 3; y < TH - 3; y++)
        for (int x = 3; x < TW - 3; x++) {
            if (AT(x, y) != GEN_FLOOR || gen_float() > 0.045f) continue;
            if (x > sx - 4 && x < sx + 4 && y > sy - 4 && y < sy + 4) continue;
            int open = 1;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (AT(x + i, y + j) != GEN_FLOOR) open = 0;
            if (open) AT(x, y) = GEN_RUBBLE;
        }
    int reach = flood(sx, sy), solids = 0;
    for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) if (AT(x, y) == GEN_PILLAR || AT(x, y) == GEN_RUBBLE) solids++;
    if (reach < floors - solids)                                  /* the columns / rubble cut a floor tile off: take them all away, the plain temple is always joined */
        for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) if (AT(x, y) == GEN_PILLAR || AT(x, y) == GEN_RUBBLE) AT(x, y) = GEN_FLOOR;
    flood(sx, sy);

    /* 6. the sealed gate: in the top wall (else the bottom wall) of the room that is farthest to walk to; if that room has no free stretch of
     * wall, the next farthest */
    int gx = -1, gy = -1, tried[MAX_ROOMS] = { 0 };
    for (int round = 1; round < nr && gx < 0; round++) {
        int far_i = -1, far_d = -1;
        for (int i = 1; i < nr; i++) {
            if (tried[i]) continue;
            int cx = rm[i].x + rm[i].w / 2, cy = rm[i].y + rm[i].h / 2, d = -1;
            for (int j = -1; j <= 1 && d < 0; j++) for (int k = -1; k <= 1 && d < 0; k++) d = dist[cy + j][cx + k];
            if (d > far_d) { far_d = d; far_i = i; }
        }
        if (far_i < 0) break;
        tried[far_i] = 1;
        Room *g = &rm[far_i]; int mid = g->x + g->w / 2;
        for (int o = 0; o < g->w && gx < 0; o++) {
            int xs[2] = { mid + o, mid - o };
            for (int k = 0; k < 2 && gx < 0; k++) {
                int x = xs[k];
                if (x <= g->x || x >= g->x + g->w - 1) continue;
                if (AT(x, g->y - 1) == GEN_WALL && AT(x, g->y) == GEN_FLOOR) { gx = x; gy = g->y - 1; }
                else if (AT(x, g->y + g->h) == GEN_WALL && AT(x, g->y + g->h - 1) == GEN_FLOOR) { gx = x; gy = g->y + g->h; }
            }
        }
    }
    if (gx >= 0) AT(gx, gy) = GEN_GATE;

    if (info) { info->w = TW; info->h = TH; info->spawn_x = sx; info->spawn_y = sy; info->gate_x = gx; info->gate_y = gy; info->rooms = nr; }
    return 1;
}
