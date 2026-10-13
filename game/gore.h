/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef GORE_H
#define GORE_H
#include <SDL2/SDL.h>
#include "char.h"

/* chunk 38: Alex's death, stylised. a burst of red pixel squares that flies out, falls and lands as stains that stay on the floor, and her head,
 * which flies off, bounces and lies still. everything is made of square pixels (the pixel scale given to gore_reset); nothing smooth.
 * positions are world px, the point on the ground (feet); heights (z) are px above it. */
void gore_reset(int pixel_scale, int tile_size);             /* clears the stains, the drops and the head (a new map) */
void gore_set_floor(int (*floor_ok)(int tile_x, int tile_y)); /* where a drop may land and the head may roll (NULL: anywhere) */
void gore_burst(float x, float y, float height, float dir_x, float dir_y);   /* the splash: from a point `height` above (x, y), biased along (dir_x, dir_y) */
/* chunk 57: the whole body goes. a much bigger burst of red pixel squares (some of them big) from a point `height` above (x, y), out in every direction;
 * they land as stains like the beheading's (the oldest stains still go first when the list is full). no head. chunk 71: and the flesh: chunks in the colours of `who` (skin, shirt, trousers, hair, boots) are thrown out with it and, unlike the blood, STAY where they land (a body does not vanish). */
void gore_explode(float x, float y, float height, const Person *who);
void gore_head_launch(const Person *who, float x, float y, float height, float dir_x, float dir_y);
/* chunk 38.1: the idle of the lying body. from the neck (x, y on the ground, `height` px above it) short bursts of small red squares shoot out in a
 * straight line along (dir_x, 0): a squeeze (the body twitches), a pfft (sfx_pfft), a pause, again, until the map is swapped (gore_reset). the first
 * burst comes `delay` seconds after the start. the squares leave no stains (the 320 stains of the death must not be pushed out by an endless idle).
 * gore_sparks_enable(0) pauses it (no sound, no new squares) while nobody is in the room. */
void gore_sparks_start(float x, float y, float height, float dir_x, float delay);
void gore_sparks_enable(int on);
int  gore_sparks_running(void);
int  gore_spark_bursts(void);                                /* tests: how many bursts so far */
int  gore_sparks_flying(void);
void gore_spark_shake(int *off_x, int *off_y);               /* how far the body is shaken right now, px (whole pixel units) */
void gore_update(float dt);
void gore_draw_floor(SDL_Renderer *r, int cam_x, int cam_y);   /* the stains (under the characters) */
void gore_draw_air(SDL_Renderer *r, int cam_x, int cam_y);     /* the drops in the air and the head (over the characters) */

int  gore_stain_count(void);
int  gore_flesh_count(void);                                 /* chunk 71: chunks of a body that have landed (they stay) */
int  gore_drops_flying(void);
int  gore_head_exists(void);
int  gore_head_settled(void);                                /* it has stopped for good */
void gore_head_pos(float *x, float *y);                      /* where it is on the ground, world px */
int  gore_busy(void);                                        /* anything still in the air or rolling */

#endif
