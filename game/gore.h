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
void gore_head_launch(const Person *who, float x, float y, float height, float dir_x, float dir_y);
void gore_update(float dt);
void gore_draw_floor(SDL_Renderer *r, int cam_x, int cam_y);   /* the stains (under the characters) */
void gore_draw_air(SDL_Renderer *r, int cam_x, int cam_y);     /* the drops in the air and the head (over the characters) */

int  gore_stain_count(void);
int  gore_drops_flying(void);
int  gore_head_exists(void);
int  gore_head_settled(void);                                /* it has stopped for good */
void gore_head_pos(float *x, float *y);                      /* where it is on the ground, world px */
int  gore_busy(void);                                        /* anything still in the air or rolling */

#endif
