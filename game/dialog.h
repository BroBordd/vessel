/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef DIALOG_H
#define DIALOG_H
#include <SDL2/SDL.h>
#include "char.h"

/* one page of dialog. who == NULL -> centred window, no face (narration / system text).
 * who != NULL -> bottom popup with that person's face and name.
 * text is auto-wrapped; use "\n" to force a line break.
 * HIGHLIGHT MARKUP: put { } around words to draw them green + underlined (same look as the matched
 * words in the typed chat), e.g. "I dropped my {orb} in the Grasslands." A span may cover several
 * words and may wrap over lines. Spans are numbered 0, 1, 2 ... in the order they appear on the page
 * (max 8 per page). Use dialog_on_highlight to react when one has been typed out. */
typedef struct { const Person *who; const char *text; int reply; } DialogLine;

/* DialogLine.reply (leave it out for a normal page):
 *   REPLY_NONE      a normal page: tap the blinking arrow to go on
 *   REPLY_OPTIONAL  after the text, TALK and BYE buttons appear. BYE = walk away (text == NULL below).
 *   REPLY_REQUIRED  only a TALK button: the player must say something
 * TALK opens the keyboard overlay (talk.c). what happens next is up to dialog_on_reply. */
enum { REPLY_NONE = 0, REPLY_OPTIONAL = 1, REPLY_REQUIRED = 2 };

/* plays the lines in order. each page types itself out, then a blinking > appears
 * and a tap moves on. on_done (may be NULL) runs after the last page is dismissed.
 * the lines array must stay alive while playing (make it static). */
void dialog_play(const DialogLine *lines, int count, void (*on_done)(void));
int  dialog_active(void);

/* optional: fn(page) runs every time a page of the NEXT / current dialog begins (page 0 is the first).
 * it is dropped when that dialog ends, so set it right before dialog_play. */
void dialog_on_page(void (*fn)(int page));

/* optional: fn(page, span) runs once when the {highlighted} span number `span` (0 = the first one on
 * that page) has finished typing out. page is the same index dialog_on_page gets. dropped when the
 * dialog ends, so set it right before dialog_play. fn may start a new dialog_play. */
void dialog_on_highlight(void (*fn)(int page, int span));

/* optional: fn(page, text) runs when the player answers a REPLY page. text is what they typed, or
 * NULL if they pressed BYE. afterwards the dialog moves to the next page, UNLESS fn started a new
 * dialog_play (that is how conversations keep going). dropped when the dialog ends. set it right
 * before dialog_play, like dialog_on_page. */
void dialog_on_reply(void (*fn)(int page, const char *text));

/* optional: suggestion chips shown above the keyboard ("No", "Yes" ...). the list must stay alive
 * while the dialog plays (make it static). dropped when the dialog ends. */
void dialog_suggest(const char *const *list, int count);

/* engine hooks, called by the world */
void dialog_init(int w, int h);
void dialog_touch(int a, int x, int y);
void dialog_update(float dt);
void dialog_draw(SDL_Renderer *r);

#endif
