/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef NOWPLAYING_H
#define NOWPLAYING_H
#include <SDL2/SDL.h>

/* "NOW PLAYING" card, top-left. slides in from the left whenever music_play starts a track,
 * eases to a stop, stays a few seconds, then slides out. the little pixel bars are driven by
 * music_spectrum(), i.e. the real audio being played. nothing to call when a song starts:
 * it watches music_serial() by itself. */
void nowplaying_init(int w, int h);
void nowplaying_update(float dt);
void nowplaying_draw(SDL_Renderer *r);

/* how far down (screen px) the mission list should sit so the two never overlap. smoothly
 * follows the card as it slides in and out, 0 when the card is gone. */
int  nowplaying_offset(void);

#endif
