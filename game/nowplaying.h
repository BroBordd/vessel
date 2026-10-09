/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef NOWPLAYING_H
#define NOWPLAYING_H
#include <SDL2/SDL.h>

/* the MUSIC BUTTON, first of the top-left buttons. when a track starts it opens up into the
 * "NOW PLAYING" card (title + live equalizer from music_spectrum(), the real audio), stays a few
 * seconds, then lerps back down into a square button with a pixel music note. it never goes away.
 * tapping it (card or button) opens the music window (musicwin.h). nothing to call when a song
 * starts: it watches music_serial() by itself. */
void nowplaying_init(int w, int h);
void nowplaying_update(float dt);
void nowplaying_draw(SDL_Renderer *r);

/* how far down (screen px) the mission list should sit so it stays under the button / card. it
 * follows the card's size every frame, so it glides while the card opens and closes. */
int  nowplaying_offset(void);

/* where the button / card is right now (screen px), and the side of a button square, so the
 * other top buttons can line up next to it */
void nowplaying_rect(int *x, int *y, int *w, int *h);
int  nowplaying_button_size(void);
int  nowplaying_gap(void);                 /* space between top buttons */

/* the shared look of the top buttons: dark square, light frame, a brighter fill while pressed.
 * a = overall opacity 0..255 */
void ui_button_frame(SDL_Renderer *r, int x, int y, int w, int h, int pressed, int a);

/* tapping opens the music window. returns 1 when the touch belonged to the button, so the game
 * underneath should not also see it. a = Android MotionEvent action. */
int  nowplaying_touch(int a, int x, int y);

#endif
