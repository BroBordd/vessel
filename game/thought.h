/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE BRAIN BUTTON: third of the top-left buttons (music, pause, brain), right of the pause button.
 * it shows what the vessel is *thinking*, the game's way of talking to the person playing. it behaves exactly like the music button: a square button with a
 * pixel brain on it that, when a thought arrives, opens up into a card (the player's face + the
 * words), stays a few seconds, then eases back down into the button. it never goes away.
 *
 *  - tapping the button (or the card) opens the BRAIN WINDOW (brainwin.h): the monitor, and every
 *    thought the vessel has had. the card itself is only a notification: thought_say() does not add
 *    to that window, brainwin_add() does.
 *  - it pushes things out of its way: the mission list and the "New task added" toast under it are
 *    pushed down while the card is taller than a button (see thought_offset), so a thought always sits clean.
 *  - it grows to the right, up to the ID card. if the music card is open and leaves too little room for the WHOLE thought, the thought waits (its timer
 *    holds) until that card folds away: a thought is never cut because of the music card. it is
 *    never taller than the ID card, so the toast hanging under the ID card stays clear.
 *  - several thoughts queue up and play one after the other.
 *
 * keep thoughts short: about 19 letters per line on a phone, 2 or 3 lines depending on the screen
 * (the rest is cut). the story can fire one at any time:
 *     thought_say("I need to find that orb.", 0);
 */
#ifndef THOUGHT_H
#define THOUGHT_H
#include <SDL2/SDL.h>

/* shows `text` for `seconds` (counting from when the card starts to open). seconds <= 0 picks a
 * time that fits the length of the text. the text is copied, so it may be a temporary string. */
void thought_say(const char *text, float seconds);

/* DIA'S WRATH (vessel 1, chunk 11): her voice hijacks the bar. the card turns red, shudders, shows a red
 * diamond eye instead of the player's face, and its words are red. it replaces whatever is showing, forgets
 * whatever was queued, and cannot be tapped open into the brain window. thoughts that arrive while it is up
 * wait behind it. seconds <= 0 = 3 s.   thought_wrath("HOW DARE YOU", 3.0f); */
void thought_wrath(const char *text, float seconds);
int  thought_is_wrath(void);                /* her card is up right now */

int  thought_active(void);                  /* a thought is on screen (or waiting for room) */

/* where the brain button / card is right now (screen px) */
void thought_rect(int *x, int *y, int *w, int *h);

/* how far down (screen px) the mission list should sit so it stays under the music button / card
 * AND the brain button / card. world.c takes the larger of this and nowplaying_offset(). */
int  thought_offset(void);

/* engine hooks, called by game.c every frame, after the music button's own update / draw */
void thought_init(int w, int h);            /* needs nowplaying_init and hud_init first. also clears anything pending */
void thought_set_enabled(int on);           /* the game world is on screen (the button only shows then, like the pause button) */
void thought_update(float dt);              /* dt = 0 while paused: the card freezes and the thought keeps its time */
/* tapping opens the brain window. returns 1 when the touch belonged to the button, so the game
 * underneath should not also see it. a = Android MotionEvent action. */
int  thought_touch(int a, int x, int y);
void thought_draw(SDL_Renderer *r);         /* draw it after the music and pause buttons */

#endif
