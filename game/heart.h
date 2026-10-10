/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE HEART (death cutscene III): a pixel heart drawn over the player's chest while the heart monitor peeps,
 * that bursts into blood on the flatline. engine only, no story: story.c calls
 *
 *   heart_show();    the heart appears (with a thump)       heart_pulse();  it thumps once more (one per peep)
 *   heart_burst();   it explodes into blood pixels          heart_reset();  everything gone (world.c does it)
 *
 * world.c updates it every frame and draws it with heart_draw() after the scene, at the chest position that
 * world_death_player() gives. the blood is kept in units of heart cells, so it does not care about the screen. */
#ifndef HEART_H
#define HEART_H
#include <SDL2/SDL.h>

enum { HEART_OFF = 0, HEART_BEATING = 1, HEART_BURST = 2 };

void heart_reset(void);
void heart_show(void);
void heart_pulse(void);
void heart_burst(void);
void heart_update(float dt);
/* cx, cy: the middle of the heart on the screen (the chest). pixel: the screen size of one sprite pixel (world_death_player) */
void heart_draw(SDL_Renderer *r, int cx, int cy, int pixel);

int  heart_state(void);
int  heart_particles(void);        /* blood pixels still in the air (tests) */

#endif
