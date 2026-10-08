/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE TALK OVERLAY: an in-game keyboard so the player can type a line to an npc.
 * (the game process only gets touches, so the keyboard is drawn and handled right here.)
 *
 * while the player types, every phrase the npcs understand (lang_pats.c) is highlighted in
 * green and underlined, case-insensitive and wildcard. tappable suggestion chips sit above
 * the box ("No" / "Yes" ...), but the box is always free for custom text.
 */
#ifndef TALK_H
#define TALK_H
#include <SDL2/SDL.h>

#define TALK_MAX 80

/* npc_line is shown for context. required = 1 hides nothing here, the dialog decides whether
 * a "skip" exists; the overlay just words its back button differently. cb(text) runs when
 * SEND is tapped, with text = what was typed. BACK just closes the overlay (cb is not called). */
void talk_open(const char *npc_name, const char *npc_line, const char *const *suggest, int nsuggest,
               int required, void (*cb)(const char *text));
int  talk_active(void);

/* engine hooks (called by the dialog, which owns the overlay) */
void talk_init(int w, int h);
void talk_touch(int a, int x, int y);
void talk_update(float dt);
void talk_draw(SDL_Renderer *r);

/* test hooks: drive the overlay without a touchscreen */
void talk_debug_type(const char *s);
const char *talk_debug_text(void);
void talk_debug_send(void);                 /* same as tapping SEND */

#endif
