/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef HUD_H
#define HUD_H
#include <SDL2/SDL.h>
#include "char.h"

/* the ID card, top-right: portrait, name and an HP bar, all on one chunky pixel grid.
 * when the name changes (hud_set_person with a different name) the name flashes, a coin
 * ding plays and a "YOU ARE NOW <NAME>" toast drops in under the card. */
void hud_init(int w, int h, const Person *who);   /* shows `who` straight away, no announcement */
void hud_set_person(const Person *who);           /* different name -> flash + ding + toast */
void hud_set_hp(int hp, int hp_max);              /* the bar eases down, leaving a pale trail */

/* engine hooks, called by the world */
void hud_update(float dt);
void hud_draw(SDL_Renderer *r);

#endif
