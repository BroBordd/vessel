/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef HUD_H
#define HUD_H
#include <SDL2/SDL.h>
#include "char.h"

/* the ID card, top-right: portrait, name and an HP bar, all on one chunky pixel grid.
 * when the name changes (hud_set_person with a different name) the name flashes, a coin
 * "YOU ARE NOW <NAME>" toast (toast.h, the same panel as "New task added", under the mission list) drops in with the coin ding. */
void hud_init(int w, int h, const Person *who);   /* shows `who` straight away, no announcement */
void hud_set_person(const Person *who);           /* different name -> flash + ding + toast */
void hud_set_hp(int hp, int hp_max);
void hud_reset(const Person *who, int hp_full);   /* a new life: shows `who` at once (no flash, ding or toast) and fills the bar to hp_full (0 = the start value) */              /* the bar eases down, leaving a pale trail */

/* who the card shows right now (the thinking bar borrows the face) */
const Person *hud_person(void);

/* where the card is (screen px), so other widgets can sit next to it */
void hud_card_rect(int *x, int *y, int *w, int *h);

/* the little spinning coin (7x7 cells of `cell` px, tt = seconds since it appeared), shared with the task toast */
void hud_draw_coin(SDL_Renderer *r, int x, int y, int cell, float tt);

/* engine hooks, called by the world */
void hud_update(float dt);
void hud_draw(SDL_Renderer *r);

#endif
