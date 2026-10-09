/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE MUSIC WINDOW: opens when the player taps the "NOW PLAYING" card.
 *   - the same equalizer as the card, but big
 *   - what is in the song right now: drum hits (kick / snare / hi-hat) and how loud the bass,
 *     middle and lead registers are (analyze.c listens to the real audio and guesses these)
 *   - a piano whose keys light up with the notes that are sounding, coloured by register. tap or
 *     slide a finger over the keys to play them yourself. */
#ifndef MUSICWIN_H
#define MUSICWIN_H
#include <SDL2/SDL.h>

void musicwin_init(int w, int h);
void musicwin_open(void);
int  musicwin_active(void);                /* open (or still sliding away) */
/* a = Android MotionEvent action. returns 1 when the touch was used (always, while the window is open) */
int  musicwin_touch(int a, int x, int y);
void musicwin_update(float dt);
void musicwin_draw(SDL_Renderer *r);

/* test hooks */
int  musicwin_debug_key_at(int x, int y);  /* midi note under a screen point, -1 if none */

#endif
