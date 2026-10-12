/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * MAP ICONS (chunk 60): the little pixel-art pictures the minimap and the map window draw for the things the player has found: Alex's house, the shrine
 * (whole or broken), the hammer, a grave, the goddess's temple. one drawing function per kind, used by both maps, 12 x 12 cells scaled in whole cells.
 * no text: the picture is the whole marker. the orb never has one (it is never on the ground). engine only. */
#ifndef MAPICON_H
#define MAPICON_H
#include <SDL2/SDL.h>

enum { MAPICON_HOUSE, MAPICON_SHRINE, MAPICON_SHRINE_BROKEN, MAPICON_HAMMER, MAPICON_GRAVE, MAPICON_TEMPLE, MAPICON_COUNT };
#define MAPICON_CELLS 12

/* the picture centred on (cx, cy), each of its 12 x 12 cells `cell` px */
void mapicon_draw(SDL_Renderer *r, int kind, int cx, int cy, int cell);

#endif
