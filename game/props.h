/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * PROPS (story chunk 7): things of the ruins that are not bodies and not items. so far one: THE BIG ROCK. it lies on the south ledge, in front of the
 * pit, and it is BIG in the sense of docs/story/vessel.md section 4: it cannot be picked up, only dragged (world.c does that with the same sandbag
 * rope as a body, only heavier), and it slows the player a lot. it is solid, it keeps its place across deaths (the map is made again from the same
 * seed, the rock is not), and chunk 9 will throw it at a body. position is the feet's middle, in tiles, like a body's. */
#ifndef PROPS_H
#define PROPS_H
#include <SDL2/SDL.h>

void  props_reset(void);                                            /* a new game: no rock lies anywhere yet */
int   props_rock_placed(void);
void  props_rock_place(float tile_x, float tile_y);                 /* it lies there (feet, tiles) */
/* its box in world px: x +- 5 cells, y from 7 cells above the feet to the feet (a cell is one sprite pixel, `px` screen px). 0 when it is not placed */
int   props_rock_box(int px, int tile, float *x0, float *y0, float *x1, float *y1);
void  props_rock_shift(float dx_px, float dy_px, int tile);        /* dragged */
int   props_rock_hit(float feet_x, float feet_y, int px, int tile);       /* would our feet box (world px) stand on it? (the same test as bodies_hit) */
float props_rock_gap(float feet_x, float feet_y, int px, int tile);       /* cells between our feet box and it (1e9: none) */
void  props_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken);   /* under the characters */

#endif
