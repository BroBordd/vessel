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

/* THE OPEN PLOT AND THE BODY (chunk 31): grave_open(i) opens the earth plot and the dead vessel's body (the grave's own Person, greyed) is drawn
 * SLIDING OUT of it, feet first, over GRAVE_SLIDE_T seconds, then lying on the grass beside it (head towards the stone). grave_close(i) plays
 * it all backwards (the body slides back in and the plot closes). the stone and the photo stay. grave_set_open(i, open, instant) sets the state
 * at once (a map that is made again). grave_update(dt) runs the slide (world.c calls it every frame). */
#define GRAVE_SLIDE_T 1.5f
#define GRAVE_BODY_DX 22                                    /* units (one unit = one world pixel cell) from the plot's middle to the lying body's middle */
void  grave_open(int i);
void  grave_close(int i);
void  grave_set_open(int i, int open, int instant);
void  grave_update(float dt);
int   grave_is_open(int i);                                 /* the plot is open (or opening): 1 until the close has finished */
int   grave_body_out(int i);                                /* the body lies on the grass, the slide is over (searchable, chunks 32-34) */
int   grave_body_moving(int i);                             /* a slide is under way, either way */
void  grave_body_tile(int i, float *tx, float *ty);         /* where the body lies (feet position, tiles) */

/* THE SEARCH (chunk 32): the story switches it on for a grave (grave_search_enable). once that grave's body lies on the grass (grave_body_out) and the
 * player stands next to it, the interact button shows a magnifying glass. the search is a HOLD (chunk 64): the bar fills while the finger is on the button, GRAVE_SEARCH_T seconds
 * in all (letting go drains it back, walking out of reach loses it) and then on_done runs, once: the search is over and does not switch itself on again. the story decides what he was carrying. */
#define GRAVE_SEARCH_T 1.5f
#define SEARCH_DECAY_T 1.0f         /* seconds for a full search bar to drain back after letting go */
void  grave_search_enable(int i, void (*on_done)(void));
int   grave_search_enabled(void);                           /* switched on and not finished */
int   grave_search_index(void);                             /* which grave (-1: none) */
int   grave_search_active(void);                            /* the finger is searching right now (held, in reach) */
float grave_search_progress(void);                          /* 0..1 while searching */
int   grave_search_near(float player_x, float player_y, int allow);   /* in reach of the body (hysteresis). allow = 0 forces "no" and cancels a search */
int   grave_search_update(float dt, int holding);           /* every frame while playing; holding = the finger is on the button in reach. returns 1 on the frame it is done (on_done has run) */

/* THE BURIAL (chunk 34): the pull played backwards. the story switches it on for a grave (grave_bury_enable); once that grave's body lies on the
 * grass (grave_body_out) and the player stands next to it, the interact button shows a hand lowering him into the plot, and HOLDING it fills a bar
 * (letting go early makes it creep back). when it is full the body is dragged back into the plot (grave_close, 1.5 s) and on_done runs, once: the
 * burial does not switch itself on again. */
#define GRAVE_BURY_HOLD_T 2.5f
void  grave_bury_enable(int i, void (*on_done)(void));
int   grave_bury_enabled(void);
int   grave_bury_index(void);
float grave_bury_progress(void);                            /* 0..1 */
int   grave_bury_near(float player_x, float player_y, int allow);   /* in reach of the body (hysteresis). allow = 0 forces "no" */
int   grave_bury_update(float dt, int holding);             /* every frame while playing. returns 1 on the frame it is done (on_done has run) */

/* where the portrait is drawn (screen px) for a camera at (cam_x, cam_y): the square the old photo fills, inside its frame */
void  grave_portrait_rect(int i, int cam_x, int cam_y, int *x, int *y, int *w, int *h);

#endif
