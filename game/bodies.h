/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE DEAD (story chunk 6): every vessel that dies stays. a death adds a BODY where it fell; the body keeps its own inventory (what the vessel carried when
 * it died), it is solid (we cannot stand on it), it lies in a little pool of blood and it can be inspected (the hand button turns into a magnifying glass
 * next to one: world.c, then story.c says what it carries). the list lives in story terms (a vessel number, a place in tiles), so the look is always made
 * from the number and nothing is stored but a few bytes.
 * vessel 1 (Aonia) is in the list too, but his place is the ritual's (ritual.c draws him and moves him when he is dragged): `vessel == 1` means "ask ritual.c".
 * chunk 7 will let any body be dragged, later chunks cut them up. */
#ifndef BODIES_H
#define BODIES_H
#include <SDL2/SDL.h>
#include "item.h"

#define BODIES_MAX 64
typedef struct {
    unsigned vessel;                    /* whose body: 1 = Aonia (at the circle, ritual.c), 2 = Doia ... */
    float tx, ty;                       /* where the feet are, in tiles (not used for vessel 1) */
    unsigned char inv[ITEM_COUNT];      /* 1 = it carries that item */
} Body;

void bodies_reset(void);                                        /* a new game: nobody lies anywhere */
int  bodies_add(unsigned vessel, float tile_x, float tile_y, const unsigned char *inv);   /* inv may be NULL (empty hands). returns the index, -1 when full */
int  bodies_count(void);
const Body *bodies_get(int i);
const Body *bodies_find(unsigned vessel);                       /* NULL when that vessel has no body */
/* "has a knife and a rock" style text for the toast: lower case, `out` of n bytes */
void bodies_inv_text(const Body *b, char *out, int n);

/* world.c helpers (every body but vessel 1, who is ritual.c's). px = the sprite pixel, tile = 8 px. a lying body is 12 cells long and 9 across: the box is
 * the feet's x +- 6 cells, y from 9 cells above the feet to the feet (the same box ritual.c gives vessel 1) */
int  bodies_hit(float feet_x, float feet_y, int px, int tile);   /* would our feet box (world px) stand on one? */
/* the nearest body within `reach` cells of our feet (world px), vessel 1 left out. returns its index or -1, and the gap in cells */
int  bodies_nearest(float feet_x, float feet_y, int px, int tile, float reach, float *gap);
void bodies_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken);   /* the pools and the bodies, under the characters */

#endif
