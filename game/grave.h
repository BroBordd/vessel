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
#define GRAVE_PULL_HOLD_T  2.5f     /* seconds of holding the button to pull a body out of its grave */

/* world hooks */
void  grave_reset(int pixel_scale, int tile_size);          /* a new map: the graves are gone */
int   grave_place(int tile_x, int tile_y, const Person *dead);   /* a grave with that person's portrait. returns its index, -1 when full */
int   grave_count(void);
void  grave_tile(int i, float *tx, float *ty);              /* feet position in tiles (minimap, nearness) */
const Person *grave_person(int i);                          /* whose grave it is */

float grave_foot_y(int i);                                  /* world px, for draw ordering */
void  grave_draw(SDL_Renderer *r, int i, int cam_x, int cam_y);
int   grave_collides(float feet_x, float feet_y, float half_w, float h);   /* blocks the player */

/* THE PULL (vessel 2, chunk 30): the story switches one grave on (grave_pull_enable). from then on the interact button shows a grip when the
 * player stands next to that grave, and HOLDING it fills a bar (like the shrine's); letting go early makes the bar creep back; when it is full
 * on_done runs once and the pull is over (it does not switch itself on again). chunk 31 draws what comes out of the ground. */
void  grave_pull_enable(int i, void (*on_done)(void));
int   grave_pull_enabled(void);                             /* a pull is switched on and not finished */
int   grave_pull_index(void);                               /* which grave (-1: none) */
float grave_pull_progress(void);                            /* 0..1 */
int   grave_pull_near(float player_x, float player_y, int allow);   /* in reach of the switched-on grave (hysteresis). allow = 0 forces "no" */
int   grave_pull_update(float dt, int holding);             /* every frame while playing. returns 1 on the frame it is done (on_done has run) */

/* where the portrait is drawn (screen px) for a camera at (cam_x, cam_y): the square the old photo fills, inside its frame */
void  grave_portrait_rect(int i, int cam_x, int cam_y, int *x, int *y, int *w, int *h);

#endif
