/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * GRAVES: where a dead vessel is remembered (vessel 2, chunk 21). a stone slab with a small plot of earth, a flower, and a framed
 * portrait of whoever died on the stone, greyed so that it reads as an old photo.
 *
 *  - story.c puts one on the map per dead vessel (world_place_grave): it does not know which vessel it is, it is given the Person.
 *  - it blocks the player like the shrine does and shows as a grey block on the minimap.
 *  - up to GRAVE_MAX graves on a map, so later vessels (Tria, Ceathia ...) are one more call.
 *  - everything here is engine: no story text. world.c owns the instances and clears them when a new map loads (like the shrine).
 *  - pixel art only: square cells, alpha blending only, the grey is gfx_set_filter (works in CPU and GPU mode). */
#ifndef GRAVE_H
#define GRAVE_H
#include <SDL2/SDL.h>
#include "char.h"

#define GRAVE_MAX 8

/* world hooks */
void  grave_reset(int pixel_scale, int tile_size);          /* a new map: the graves are gone */
int   grave_place(int tile_x, int tile_y, const Person *dead);   /* a grave with that person's portrait. returns its index, -1 when full */
int   grave_count(void);
void  grave_tile(int i, float *tx, float *ty);              /* feet position in tiles (minimap, nearness) */
const Person *grave_person(int i);                          /* whose grave it is */

float grave_foot_y(int i);                                  /* world px, for draw ordering */
void  grave_draw(SDL_Renderer *r, int i, int cam_x, int cam_y);
int   grave_collides(float feet_x, float feet_y, float half_w, float h);   /* blocks the player */

/* where the portrait is drawn (screen px) for a camera at (cam_x, cam_y): the square the old photo fills, inside its frame */
void  grave_portrait_rect(int i, int cam_x, int cam_y, int *x, int *y, int *w, int *h);

#endif
