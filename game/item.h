/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * ITEMS AND THE INVENTORY: a thing that lies on the map until the player walks over it, and the pockets it goes into.
 *
 *  - story.c puts an item on the map (world_place_item). walking within PICK_R tiles picks it up: it leaves the map and
 *    goes into the inventory. one item lies on a map at a time (the hammer is the only one so far). not solid.
 *  - the inventory is a little slot row right under the minimap, one slot per item owned (nothing shows while it is empty).
 *    tap it and it unfolds into a panel with every item drawn big and its name; tap again and it folds back.
 *  - item_draw_icon draws the 12x12 picture of an item anywhere: the slot, the panel, the interact button.
 *  - everything here is engine: no story text. the inventory survives map changes; story.c empties it with a new life. */
#ifndef ITEM_H
#define ITEM_H
#include <SDL2/SDL.h>

enum { ITEM_HAMMER, ITEM_COUNT };
#define ITEM_ICON_CELLS 12                  /* an icon is 12 x 12 cells */
#define ITEM_PICK_R     1.0f                /* tiles: this close and it is yours */

const char *item_name(int kind);            /* "HAMMER" (capitals: the pixel font) */

/* world hooks */
void  item_reset(int pixel_scale, int tile_size);       /* a new map: whatever lay on the old one is gone (the inventory stays) */
void  item_place(int kind, int tile_x, int tile_y);     /* it lies there */
int   item_exists(void);                                /* something lies on this map */
void  item_tile(float *tx, float *ty);                  /* where, in tiles (minimap) */
int   item_has(int kind);
void  item_give(int kind);                              /* straight into the pockets */
void  item_clear(void);                                 /* empty pockets (a new body) */
/* every frame while playing. player = feet, world px. allow = 0 stops picking up (dialog open). returns the kind picked up on
 * the frame it happens, else -1 */
int   item_update(float dt, float player_x, float player_y, int allow);
void  item_draw_ground(SDL_Renderer *r, int cam_x, int cam_y, float t);

/* an icon, top-left at (x, y), each cell `cell` px. */
void  item_draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell);

/* the inventory widget. the box is `width` px wide (the minimap's width) and right-aligned to right_edge, starts at top */
void  item_ui_init(int w, int h, int right_edge, int top, int width);
int   item_ui_touch(int action, int x, int y);          /* Android MotionEvent action. 1 = this touch is the inventory's (do not pass it on) */
void  item_ui_draw(SDL_Renderer *r, float t);
int   item_ui_open(void);                               /* (tests) the panel is unfolded */
int   item_ui_hit(int x, int y);                        /* (tests) is (x, y) on the slots / the panel */

#endif
