/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE PAUSE BUTTON: second of the top-left buttons (music, pause, brain), right of the music button
 * (it slides along as the now-playing card opens and closes). only there while
 * the game world is running.
 * pausing freezes the world and dims it behind a PAUSED panel; the music keeps playing. */
#ifndef PAUSEBTN_H
#define PAUSEBTN_H
#include <SDL2/SDL.h>

void pausebtn_init(int w, int h);
void pausebtn_resize(int w, int h);          /* the screen changed shape: new layout, state kept */
void pausebtn_set_enabled(int on);          /* the game world is on screen (turning it off also resumes) */
int  pausebtn_paused(void);
void pausebtn_set_paused(int p);            /* only while the world is on screen (the Android back button and the app going to the background) */
/* returns 0 = not ours, 1 = used, 2 = used and the game was just paused (drop held controls) */
int  pausebtn_touch(int a, int x, int y);
void pausebtn_update(float dt);
void pausebtn_draw_overlay(SDL_Renderer *r);   /* the dim + PAUSED panel: draw before the top buttons */
void pausebtn_draw(SDL_Renderer *r);           /* the button itself */

#endif
