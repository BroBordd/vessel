/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * ITEMS AND THE INVENTORY: a thing that lies on the map until the player walks over it, and the pockets it goes into.
 *
 *  - story.c puts an item on the map (world_place_item). chunk 68: nothing is picked up by walking any more: within ITEM_REACH_R tiles the HAND
 *    button shows (world.c) and a TAP on it takes the item (item_grab): it leaves the map and goes into the inventory. one item lies on a map at a
 *    time (the hammer is the only one so far). not solid.
 *  - the inventory is a little slot row right under the minimap, one slot per item owned (nothing shows while it is empty).
 *    tap it and it unfolds into a panel with every item drawn big and its name; tap again and it folds back.
 *  - item_draw_icon draws the 12x12 picture of an item anywhere: the slot, the panel, the interact button.
 *  - everything here is engine: no story text. the inventory survives map changes; story.c empties it with a new life. */
#ifndef ITEM_H
#define ITEM_H
#include <SDL2/SDL.h>

enum { ITEM_HAMMER, ITEM_ORB, ITEM_COUNT };      /* chunk 39: the orb (it is never on the ground: it goes straight into the pockets) */
#define ITEM_ICON_CELLS 12                  /* an icon is 12 x 12 cells */
#define ITEM_REACH_R    1.3f                /* tiles: this close and the hand button shows (chunk 68; it was the walk-over pick-up radius) */

const char *item_name(int kind);            /* "HAMMER", "ORB" (capitals: the pixel font) */

/* world hooks */
void  item_reset(int pixel_scale, int tile_size);       /* a new map: whatever lay on the old one is gone (the inventory stays) */
void  item_place(int kind, int tile_x, int tile_y);     /* it lies there */
/* chunk 32: it FALLS there (out of someone's hand): drops from a little above, lands with a thunk and a bounce, a glint. it cannot be
 * picked up until it has landed. item_dropping() is 1 until then (tests). */
#define ITEM_DROP_T 0.45f
void  item_drop(int kind, int tile_x, int tile_y);
int   item_dropping(void);
/* chunk 33: a LOCKED kind cannot be taken. chunk 68: pressing the hand next to it does not take it: the callback set with item_set_blocked_cb runs, once per
 * press ("not like this"; walking close does nothing). item_reset (a new map) drops the locks and the callback. */
void  item_set_locked(int kind, int on);
int   item_locked(int kind);
void  item_set_blocked_cb(void (*cb)(void));
int   item_blocked_count(void);                         /* tests: how many presses were turned away */
int   item_exists(void);                                /* something lies on this map */
void  item_tile(float *tx, float *ty);                  /* where, in tiles (minimap) */
int   item_has(int kind);
void  item_give(int kind);                              /* straight into the pockets */
void  item_take(int kind);                              /* out of the pockets (chunk 43: the orb goes to Dea) */
void  item_clear(void);                                 /* empty pockets (a new body) */
/* every frame while playing: the drop animation and the pocket pulse run (nothing is picked up here any more) */
void  item_update(float dt);
int   item_in_reach(float player_x, float player_y);    /* chunk 68: an item lies within ITEM_REACH_R tiles of the feet (world px) and has landed: the hand button shows */
int   item_grab(float player_x, float player_y);        /* chunk 68: the tap on the hand. takes it and returns the kind; -1 when nothing is in reach; a locked kind is not taken (the blocked callback runs) */
void  item_draw_ground(SDL_Renderer *r, int cam_x, int cam_y, float t);

/* an icon, top-left at (x, y), each cell `cell` px. */
void  item_draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell);
/* chunk 32: the magnifying glass (12 x 12, same style) on the interact button next to a body that can be searched (chunks 32, 39). not an item. */
void  item_draw_search_icon(SDL_Renderer *r, int x, int y, int cell);
/* chunk 68: an open hand (12 x 12) on the hand button. not an item. */
void  item_draw_hand_icon(SDL_Renderer *r, int x, int y, int cell);

/* the inventory widget. the box is `width` px wide (the minimap's width) and right-aligned to right_edge, starts at top */
void  item_ui_init(int w, int h, int right_edge, int top, int width);
int   item_ui_touch(int action, int x, int y);          /* Android MotionEvent action. 1 = this touch is the inventory's (do not pass it on) */
void  item_ui_draw(SDL_Renderer *r, float t);
int   item_ui_open(void);                               /* (tests) the panel is unfolded */
int   item_ui_hit(int x, int y);                        /* (tests) is (x, y) on the slots / the panel */

#endif
