/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * GENERATION. maps are made from a seed: the same seed gives the same map, a new seed a new one. gen.c holds the seeded random numbers
 * and the generators; a generator fills a grid of GEN_* codes and says where to spawn. the world (world.c) turns codes into its own tiles,
 * so nothing here knows about drawing. */
#ifndef GEN_H
#define GEN_H
#include <stdint.h>

/* the codes a generator writes (world.c maps them to its tile types) */
enum { GEN_VOID,        /* solid rock, never seen */
       GEN_FLOOR,       /* walkable stone */
       GEN_WALL,        /* the edge of the rooms (solid) */
       GEN_PILLAR,      /* a column (solid) */
       GEN_RUBBLE,      /* fallen stone (solid) */
       GEN_GATE };      /* a sealed gate: the way out, shut for now (solid) */

typedef struct {
    int w, h;                   /* the used size of the grid */
    int spawn_x, spawn_y;       /* where the player starts (a floor tile) */
    int gate_x, gate_y;         /* the sealed gate (the farthest room's wall) */
    int rooms;                  /* how many rooms were carved */
} GenInfo;

/* the seeded random numbers (a small xorshift: the same on every device, unlike rand()) */
void     gen_seed(uint32_t seed);
uint32_t gen_rand(void);
int      gen_range(int lo, int hi);     /* lo..hi inclusive */
float    gen_float(void);               /* 0..1 */

/* the deep temple: rooms and halls carved into the rock, pillars in the big rooms, rubble, a sealed gate far from the start.
 * writes into grid[y * stride + x] (x < max, y < max). returns 1 on success */
int gen_temple(uint8_t *grid, int stride, int max, uint32_t seed, GenInfo *info);

#endif
