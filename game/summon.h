/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE SUMMONING (vessel 2, chunks 8-9): a soul is given a body. a tall column of golden-white light drops from the top of the
 * screen onto the spot where the vessel will stand, a glowing pixel ring spreads on the floor around it, the light holds, and
 * then it ends. (chunk 9 adds the rising sparks and the vessel forming out of the light.)
 *
 *  - engine only: no story text, it does not know which vessel it is. world.c owns the one instance (world_summon) and draws
 *    it between the far and the near half of the ring, so the beam stands inside the ring.
 *  - pixel art only: every cell is one sprite pixel (`cell` screen px) on the grid of the feet position, steps not glides,
 *    alpha blending only (no ADD/MOD blend: the GPU path only knows none/alpha, see GFX_REDIRECT). */
#ifndef SUMMON_H
#define SUMMON_H
#include <SDL2/SDL.h>

#define SUMMON_DROP_T   0.9f        /* the light falls from the top of the screen to the floor */
#define SUMMON_RING_T   0.7f        /* the ring spreads (it starts when the light lands) */
#define SUMMON_HOLD_T   1.6f        /* the light stands (counted from the landing) */
#define SUMMON_END_T    0.8f        /* it thins out and is gone */
#define SUMMON_TOTAL_T  (SUMMON_DROP_T + SUMMON_HOLD_T + SUMMON_END_T)

void  summon_begin(void);                   /* start (or restart) the effect */
void  summon_cancel(void);                  /* stop at once, nothing is left on screen */
int   summon_update(float dt);              /* every frame. returns 1 on the frame the effect has just finished */
int   summon_active(void);                  /* running now (the vessel is not drawn yet: world.c hides the player meanwhile) */
float summon_time(void);                    /* seconds since it began (tests) */
int   summon_beam_rows(void);               /* how many cell rows of light were drawn last frame (tests) */
int   summon_ring_radius(void);             /* the ring's horizontal radius in cells right now (tests) */

/* the far half of the ring (behind the light) and the beam, then the near half (in front). (foot_x, foot_y) = the spot on the
 * floor on screen; cell = one sprite pixel in screen px; w, h = the screen. call summon_draw_back, then draw the player if one
 * is to be seen, then summon_draw_front. */
void  summon_draw_back(SDL_Renderer *r, int foot_x, int foot_y, int cell, int w, int h);
void  summon_draw_front(SDL_Renderer *r, int foot_x, int foot_y, int cell);

#endif
