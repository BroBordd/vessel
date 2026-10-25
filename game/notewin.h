/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE NOTE WINDOW: Aonia's note, as a pixel-art page (torn paper, creases, stains) written in blood the player can read, with a CLOSE button under it. modal: it
 * takes every touch while it is open, the world waits (world.c checks notewin_active), the Android back button closes it. engine + the words; story.c decides when.
 *   notewin_open(on_close);   on_close may be NULL; it runs after the window has closed. */
#ifndef NOTEWIN_H
#define NOTEWIN_H
#include <SDL2/SDL.h>

void notewin_init(int w, int h);
void notewin_resize(int w, int h);                                   /* the screen changed shape: the window keeps its state */
void notewin_open(void (*on_close)(void));
int  notewin_active(void);
void notewin_close(void);                                            /* closes it (the back button); runs on_close */
int  notewin_touch(int action, int x, int y);                        /* Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel). 1 = the window took this touch */
void notewin_update(float dt);
void notewin_draw(SDL_Renderer *r);                                  /* over everything but the confirm window */
/* (tests) the centre of the CLOSE button, -1 when closed */
int  notewin_close_x(void);
int  notewin_close_y(void);

#endif
