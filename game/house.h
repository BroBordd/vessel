/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * ALEX'S HOUSE: a little log hut in the Grasslands, and the room inside it. this file is only ART and LAYOUT (no state, no story):
 * world.c owns where the hut stands, the doors, and the map switch between the outside and the room.
 *
 *  - outside: a solid log hut (shingle roof with a smoking chimney, stone foundation, door with a step, two windows with shutters
 *    and flower boxes, bushes). its footprint is HUT_W x HUT_H tiles; the doorstep is the tile just below the middle of the bottom edge.
 *  - inside: a HOUSE_IN_W x HOUSE_IN_H room (wood floor, back wall with window, bed, nightstand lamp, bookshelf, fireplace, table and
 *    chair, rug, barrel, crates, plant, doormat). the furniture is drawn as "pieces" that sort with the characters by their feet. */
#ifndef HOUSE_H
#define HOUSE_H
#include <SDL2/SDL.h>

#define HUT_W 7                         /* footprint in tiles; the doorstep tile is just below its bottom middle */
#define HUT_H 6

#define HOUSE_IN_W 12                   /* the room, in tiles */
#define HOUSE_IN_H 10
#define HOUSE_IN_DOOR_X 6               /* the doorway: a gap in the bottom wall (row HOUSE_IN_H - 1) */
#define HOUSE_IN_ALEX_X 5               /* where Alex stands */
#define HOUSE_IN_ALEX_Y 4

/* the hut. (ox, oy) = screen px of the footprint's top-left corner, px = screen px per sprite pixel. the roof and the smoke reach a bit
 * above and beside the footprint. sorted by the foot line: the footprint's bottom edge. */
void house_draw_outside(SDL_Renderer *r, int ox, int oy, int px, float t);

/* the room. is a furniture piece standing on this tile (it blocks the player)? */
int  house_inside_solid(int tx, int ty);
/* the pieces: draw each one before or after the player by its foot line (tiles, y). (ox, oy) = screen px of tile (0, 0) of the room. */
int   house_piece_count(void);
float house_piece_foot(int i);
void  house_piece_draw(SDL_Renderer *r, int i, int ox, int oy, int px, float t);
/* the plain tiles of the room (floor planks and walls), same coordinates: x, y = screen px of the tile */
void house_draw_floor_tile(SDL_Renderer *r, int tx, int ty, int x, int y, int px);
void house_draw_wall_tile(SDL_Renderer *r, int tx, int ty, int mw, int x, int y, int px);

#endif
