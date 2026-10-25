/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE DEAD (story chunk 6): every vessel that dies stays. a death adds a BODY where it fell; the body keeps its own inventory (what the vessel carried when
 * it died), it is solid (we cannot stand on it), it lies in a little pool of blood and it can be inspected (the hand button turns into a magnifying glass
 * next to one: world.c, then story.c says what it carries). the list lives in story terms (a vessel number, a place in tiles), so the look is always made
 * from the number and nothing is stored but a few bytes.
 * vessel 1 (Aonia) is in the list too, but his place is the ritual's (ritual.c draws him and moves him when he is dragged): `vessel == 1` means "ask ritual.c".
 * chunk 7: any body can be dragged and leaves a fading blood trail, later chunks cut them up. */
#ifndef BODIES_H
#define BODIES_H
#include <SDL2/SDL.h>
#include "item.h"

#define BODIES_MAX 64
typedef struct {
    unsigned vessel;                    /* whose body: 1 = Aonia (at the circle, ritual.c), 2 = Doia ... */
    float tx, ty;                       /* where the feet are, in tiles (not used for vessel 1) */
    float pool_x, pool_y;               /* where the pool of blood lies, in tiles: where it died. the pool is the floor's, not the body's: dragging the body leaves it behind (only the smears follow) */
    unsigned char inv[ITEM_COUNT];      /* how many of that item it carries */
    unsigned char cuts;                 /* story chunk 9: how far it has been cut up (0..BODIES_CUTS, section 6 of the plan) */
} Body;

void bodies_reset(void);                                        /* a new game: nobody lies anywhere */
int  bodies_add(unsigned vessel, float tile_x, float tile_y, const unsigned char *inv);   /* inv may be NULL (empty hands). returns the index, -1 when full */
int  bodies_count(void);
const Body *bodies_get(int i);
const Body *bodies_find(unsigned vessel);                       /* NULL when that vessel has no body */
/* story chunk 8: the pockets of a body go into ours (item_give). returns how many things were taken (0: nothing, or no such body) */
int  bodies_take_all(unsigned vessel);
int  bodies_take_kind(unsigned vessel, int kind);                /* story chunk 17: the whole stack of one kind into our pockets (a medium thing: one, and only with the hands free). returns how many */
#define BODIES_CUTS 7
int  bodies_give(unsigned vessel, int kind);                    /* story chunk 20: the whole stack of one kind out of our pockets into the body's. returns how many (0: the orb, the hammer, nothing) */
int  bodies_remove(unsigned vessel);                            /* story chunk 11: it fell into the pit. 0 when there is no such body (vessel 1 is ritual.c's and stays) */
int  bodies_cut(unsigned vessel);                               /* story chunk 9: the next stab: returns the step it reached (1..7), 0 when there is nothing left to cut or no such body */
void bodies_cut_done(unsigned vessel);                          /* ... all of it at once (the rock) */
/* "has a knife and a rock" style text for the toast: lower case, `out` of n bytes */
void bodies_inv_text(const Body *b, char *out, int n);

/* world.c helpers (every body but vessel 1, who is ritual.c's). px = the sprite pixel, tile = 8 px. a lying body is 12 cells long and 9 across: the box is
 * the feet's x +- 6 cells, y from 9 cells above the feet to the feet (the same box ritual.c gives vessel 1) */
int  bodies_hit(float feet_x, float feet_y, int px, int tile);   /* would our feet box (world px) stand on one? */
/* the nearest body within `reach` cells of our feet (world px), vessel 1 left out. returns its index or -1, and the gap in cells */
int  bodies_nearest(float feet_x, float feet_y, int px, int tile, float reach, float *gap);
void bodies_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken);   /* the pools and the bodies, under the characters */

/* chunk 7: any body can be dragged (world.c holds the rope). the box of a body in world px, like the one bodies_hit uses; 0 when that vessel has no body
 * here (vessel 1 is ritual.c's, so 0 for him too). bodies_shift moves the body by a drag step (world px) */
int  bodies_box(unsigned vessel, int px, int tile, float *x0, float *y0, float *x1, float *y1);
void bodies_shift(unsigned vessel, float dx_px, float dy_px, int tile);

/* chunk 7: THE BLOOD TRAIL. a body that is dragged leaves smears of blood on the floor, and they fade after a while (BODIES_TRAIL_LIFE seconds). world.c adds a
 * smear every few cells of dragging, updates the ages, and draws them before everything else on the floor (so the circle, the pools and the bodies lie over them) */
#define BODIES_TRAIL_LIFE 40.0f
void bodies_trail_add(float world_x, float world_y, int tile);          /* a smear at this world px */
void bodies_trail_update(float dt);
void bodies_trail_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken);

#endif
