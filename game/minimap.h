/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE MINIMAP: a little window of the map, top-right under the ID card (thumbs never cover it,
 * the stick and the interact button own the bottom corners). it follows the player and shows
 * nearby npcs (gold dots) and the hole (blue ring). */
#ifndef MINIMAP_H
#define MINIMAP_H
#include <SDL2/SDL.h>
#include <stdint.h>
#include "char.h"

typedef struct { float tx, ty; int kind; } MiniMark;      /* kind: 0 npc, 1 hole, 2 shrine, 3 grave */

void minimap_init(int w, int h, int right_edge, int top, int width);   /* the box (frame included) is `width` px wide and square, right-aligned to right_edge, starts at top */
int  minimap_bottom(void);                                  /* y just below the box */
void minimap_draw(SDL_Renderer *r, const uint8_t *tiles, int stride, int mw, int mh,
                  const Rgb *palette, int npal, float player_tx, float player_ty, int facing,
                  const MiniMark *marks, int nmarks, float t);

#endif
