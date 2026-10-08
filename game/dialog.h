/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef DIALOG_H
#define DIALOG_H
#include <SDL2/SDL.h>
#include "char.h"

/* one page of dialog. who == NULL -> centred window, no face (narration / system text).
 * who != NULL -> bottom popup with that person's face and name.
 * text is auto-wrapped; use "\n" to force a line break. */
typedef struct { const Person *who; const char *text; } DialogLine;

/* plays the lines in order. each page types itself out, then a blinking > appears
 * and a tap moves on. on_done (may be NULL) runs after the last page is dismissed.
 * the lines array must stay alive while playing (make it static). */
void dialog_play(const DialogLine *lines, int count, void (*on_done)(void));
int  dialog_active(void);

/* optional: fn(page) runs every time a page of the NEXT / current dialog begins (page 0 is the first).
 * it is dropped when that dialog ends, so set it right before dialog_play. */
void dialog_on_page(void (*fn)(int page));

/* engine hooks, called by the world */
void dialog_init(int w, int h);
void dialog_touch(int a, int x, int y);
void dialog_update(float dt);
void dialog_draw(SDL_Renderer *r);

#endif
