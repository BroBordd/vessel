/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * SPACE: a black field with white pixel "stars" drifting past on a gentle wind. The menu's background, and
 * (vessel 2) LIMBO, the screen the soul waits in at the start of the game and after every death.
 *
 *   space_init(w, h)    once, with the screen size (also restarts the stars and the clock)
 *   space_update(dt)    every frame
 *   space_draw(r)       clears the whole screen to black and draws the stars. draw everything else after it
 */
#ifndef SPACE_H
#define SPACE_H
#include <SDL2/SDL.h>

void space_init(int w, int h);
void space_update(float dt);
void space_draw(SDL_Renderer *r);

#endif
