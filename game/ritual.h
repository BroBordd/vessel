/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE BLOOD CIRCLE. where the story starts: a magic circle carved into the temple floor, its grooves filled with blood that runs in from
 * the body of vessel 1, who lies dead beside it, to its left (the knife next to him, a pool under him). when the grooves are full the circle charges,
 * flashes, and the soul's new body is drawn up out of the blood. engine only: world.c owns the one instance and draws it.
 *  - pixel art: every cell is one sprite pixel (`cell` screen px), alpha blending only (no ADD/MOD blend: see GFX_REDIRECT).
 *  - the scene (circle, blood, body) stays after the arrival and keeps flowing, until ritual_cancel (a new map). */
#ifndef RITUAL_H
#define RITUAL_H
#include <SDL2/SDL.h>
#include "char.h"

#define RITUAL_RADIUS 15            /* the outer ring: 15 cells to each side, 7.5 up and down (a tile is 8 cells): flat, like the floor seen from above at an angle */
#define RITUAL_FLOW_AT  0.8f        /* the blood starts to run (seconds after the start) */
#define RITUAL_FLOW_V   34.0f       /* how fast it runs along the grooves, cells per second */
#define RITUAL_CHARGE_T 1.8f        /* the circle charges (a bright head runs round it, faster and faster, embers swirl in) */
#define RITUAL_FORM_T   0.9f        /* the body is drawn up out of the blood, from the feet up */
#define RITUAL_DISS_T   0.6f        /* ... and the blood pixels on it fade away */
#define RITUAL_HOLD_T   0.5f        /* then the controls come back */

/* tile_x, tile_y: where the player will stand, in tiles (the circle is centred there). dead: who lies beside it. */
void  ritual_begin(float tile_x, float tile_y, const Person *dead);
void  ritual_cancel(void);                  /* scene gone */
int   ritual_scene(void);                   /* the circle is on the floor */
int   ritual_active(void);                  /* the arrival is running (the player is not whole yet) */
int   ritual_update(float dt);              /* every frame. 1 on the frame the arrival has just finished */
int   ritual_take_chime(void);              /* 1 once, when the charge begins (the sound) */
float ritual_time(void);
int   ritual_player_rows(void);             /* rows of the new body shown, from the feet up: 0 .. 12 */
void  ritual_centre(float *tile_x, float *tile_y);

/* PULLING THE BODY (world.c): he can be held and dragged over the floor. his box in cells from the circle's centre (x0 < x1, y0 < y1); 0 when
 * there is nothing to pull (no scene, no body, or the arrival is still running). ritual_body_shift moves him (the pool, the knife and the circle stay) */
int   ritual_body_box(float *x0, float *y0, float *x1, float *y1);
void  ritual_body_shift(float dx_cells, float dy_cells);

/* sx, sy: the circle's centre on screen (where the player's feet are) */
void  ritual_draw_floor(SDL_Renderer *r, int sx, int sy, int cell, float darken);                 /* grooves, blood, pool, body, knife: under the characters */
void  ritual_draw_over(SDL_Renderer *r, int sx, int sy, int cell, const Person *who);             /* shock ring, blood pixels on the new body, embers: over it */

#endif
