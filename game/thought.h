/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef THOUGHT_H
#define THOUGHT_H
#include <SDL2/SDL.h>

/* THE THINKING BAR: what the vessel is *thinking*, the game's way of talking to the person playing.
 * a bar with the player's portrait (whoever the ID card shows, see hud_person()) and a line of text.
 * it slides in from the top by itself, stays, and slides away again.
 *
 *  - NOT interactive: it has no touch handler at all, every touch goes straight to the game.
 *  - it lives in the top row, in the gap between the music / pause buttons and the ID card, and
 *    never covers them. the text wraps to fit. if the music card is open and there is no room, the
 *    thought waits (its timer does not run) until the card has folded away again.
 *  - several thoughts queue up and play one after the other.
 *
 * keep thoughts short: the bar shows at most 5 lines (about 60 letters on a phone), the rest is cut.
 *
 * the story can fire one at any time: thought_say("I need to find that orb.", 0); */

/* shows `text` for `seconds` (not counting the slide in / out). seconds <= 0 picks a time that
 * fits the length of the text. the text is copied, so it may be a temporary string. */
void thought_say(const char *text, float seconds);

int  thought_active(void);                  /* a thought is on screen (or about to be) */

/* engine hooks, called by the world */
void thought_init(int w, int h);            /* also clears anything pending */
void thought_update(float dt);
/* x0..x1 is the free span of the top row (screen px), y its top. the bar centres itself in it */
void thought_draw(SDL_Renderer *r, int x0, int x1, int y);

#endif
