/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * checks the ruins generator (gen.c) over many seeds and can print one as text:
 *   cc -O1 -Igame -o build/gentest tools/gentest.c game/gen.c && build/gentest          (checks seeds 1..500)
 *   build/gentest 7                                                                      (prints seed 7)
 * the rules: the start room is at least 9 x 7 tiles, the circle room's spawn reaches the near ledge of the pit on foot, the far ledge does NOT
 * (the pit is the only way across), the gate sits in the far wall, every pit column is at least 3 deep. */
#include "gen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define M 80
static uint8_t g[M][M];
static int seen[M][M];
static int reach(int sx, int sy, int w, int h) {
    static int qx[M * M], qy[M * M]; int hd = 0, tl = 0, n = 0;
    memset(seen, 0, sizeof seen); qx[tl] = sx; qy[tl++] = sy; seen[sy][sx] = 1;
    while (hd < tl) {
        int x = qx[hd], y = qy[hd++]; n++;
        static const int d[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int k = 0; k < 4; k++) { int nx = x + d[k][0], ny = y + d[k][1];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h || seen[ny][nx] || g[ny][nx] != GEN_FLOOR) continue;
            seen[ny][nx] = 1; qx[tl] = nx; qy[tl++] = ny; }
    }
    return n;
}
int main(int argc, char **argv) {
    static const char CH[] = " .#oxG~";
    int bad = 0, lo = 1, hi = 500;
    if (argc > 1) lo = hi = atoi(argv[1]);
    for (int seed = lo; seed <= hi; seed++) {
        GenInfo gi; memset(g, 0, sizeof g);
        if (!gen_temple(&g[0][0], M, M, (uint32_t)seed, &gi)) { printf("seed %d: failed\n", seed); bad++; continue; }
        reach(gi.spawn_x, gi.spawn_y, gi.w, gi.h);
        int near_ok = seen[gi.edge_y][gi.edge_x], far_bad = seen[gi.far_y][gi.far_x];
        int gate_ok = g[gi.gate_y][gi.gate_x] == GEN_GATE, floor_ok = g[gi.spawn_y][gi.spawn_x] == GEN_FLOOR;
        int room_ok = 1;                                                      /* 9 x 7 of floor round the spawn */
        for (int j = -3; j <= 3; j++) for (int i = -4; i <= 4; i++) if (g[gi.spawn_y + j][gi.spawn_x + i] != GEN_FLOOR) room_ok = 0;
        int deep_ok = 1;
        for (int x = gi.pit_x0; x <= gi.pit_x1; x++) { int n = 0; for (int y = gi.pit_y0; y <= gi.pit_y1; y++) n += g[y][x] == GEN_PIT; if (n < 3) deep_ok = 0; }
        if (!near_ok || far_bad || !gate_ok || !floor_ok || !room_ok || !deep_ok) { printf("seed %d: near %d far-reached %d gate %d floor %d room %d deep %d\n", seed, near_ok, far_bad, gate_ok, floor_ok, room_ok, deep_ok); bad++; }
        if (lo == hi) {
            for (int y = 0; y < gi.h; y++) { for (int x = 0; x < gi.w; x++) putchar(x == gi.spawn_x && y == gi.spawn_y ? '@' : CH[g[y][x]]); putchar('\n'); }
        }
    }
    printf(bad ? "%d seeds bad\n" : "all seeds ok\n", bad);
    return bad != 0;
}
