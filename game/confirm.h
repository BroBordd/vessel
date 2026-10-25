/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE CONFIRM WINDOW: a small centred window over the dimmed game with a title, a few lines of text and two buttons (YES, NO). it is modal: it takes every
 * touch while it is open, the world waits (world.c checks confirm_active), the Android back button answers NO. engine only: the caller brings the words
 * and what happens on YES.
 *   confirm_ask("STAB SELF", "This will kill you. Sure?", "YES", "NO", on_yes); */
#ifndef CONFIRM_H
#define CONFIRM_H
#include <SDL2/SDL.h>

void confirm_init(int w, int h);
void confirm_resize(int w, int h);                                   /* the screen changed shape: the window keeps its state */
/* opens the window (a second ask while one is open replaces it). on_yes may be NULL. yes / no may be NULL for "YES" / "NO". the callback runs after the window has closed. */
void confirm_ask(const char *title, const char *msg, const char *yes, const char *no, void (*on_yes)(void));
int  confirm_active(void);                                           /* the window is open */
void confirm_cancel(void);                                           /* closes it as NO (the back button, a new life) */
int  confirm_touch(int action, int x, int y);                        /* Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel). 1 = the window took this touch */
void confirm_draw(SDL_Renderer *r);                                  /* over everything */
/* a plain button in the look of the NO button (gold frame, dark body, gold letters); alpha 0..255 fades it. for other screens (the self-stab scene's CONTINUE) */
void confirm_button_draw(SDL_Renderer *r, int x, int y, int w, int h, const char *label, int cell, int pressed, int alpha);
/* (tests) the centre of the YES / NO button, -1 when closed */
int  confirm_yes_x(void);
int  confirm_yes_y(void);
int  confirm_no_x(void);
int  confirm_no_y(void);

#endif
