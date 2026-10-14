/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef MENU_H
#define MENU_H
#include <SDL2/SDL.h>

typedef enum { MENU_NONE = 0, MENU_PLAY, MENU_LOAD, MENU_EXIT } MenuAction;

void       menu_init(int w, int h);
/* the LOAD button is dim and takes no touches until there is a save to load (game.c asks story_save_exists) */
void       menu_set_load_enabled(int on);
/* a = Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel). returns an action on tap-release */
MenuAction menu_touch(int a, int x, int y);
void       menu_update(float dt);
void       menu_draw(SDL_Renderer *r);
/* PLAY was pressed: the title and the buttons fade out (MENU_FADE_T) over the same moving stars, and take no more touches.
 * menu_faded() is 1 once they are gone: the game goes on from that very frame (no loading screen). */
#define MENU_FADE_T 0.8f
void       menu_fade_out(void);
int        menu_faded(void);

#endif
