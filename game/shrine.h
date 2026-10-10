/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * DIA'S SHRINE: the one prop of vessel 1 you can interact with. a stone altar with a glowing gem.
 *
 *  - story.c puts it on the map (world_place_shrine) and, once Alex has asked for it, switches it on
 *    (world_enable_shrine). from then on the interact button shows a sludge drop when you stand
 *    next to it, and the sabotage is a HOLD: keep the button pressed for about 2.5 s and the shrine
 *    fills with ooze. let go early and the ooze creeps back (it does not snap, so it reads as a
 *    struggle). when the bar is full the shrine stays polluted for good and the callback runs.
 *  - it blocks the player like a tree does, and shows as a violet dot on the minimap once enabled.
 *  - everything here is engine: no story text. world.c owns the one instance (like the hole). */
#ifndef SHRINE_H
#define SHRINE_H
#include <SDL2/SDL.h>

#define SHRINE_HOLD_T  2.5f         /* seconds of holding the button to pollute it */

/* world hooks */
void  shrine_reset(int pixel_scale, int tile_size);         /* a new map: the shrine is gone */
void  shrine_place(int tile_x, int tile_y);                 /* it stands there, but cannot be used yet */
void  shrine_enable(void (*on_done)(void));                 /* it can be used now. on_done runs once, when it turns polluted */
int   shrine_exists(void);
int   shrine_enabled(void);                                 /* usable (placed, switched on, not polluted yet) */
int   shrine_polluted(void);
float shrine_progress(void);                                /* 0..1, how much ooze it has right now */
void  shrine_tile(float *tx, float *ty);                    /* feet position in tiles (minimap) */

/* is the player (feet, world px) in range of a usable shrine? has hysteresis, so the button does not flicker.
 * allow = 0 forces "no" (dialog open, cutscene) */
int   shrine_near(float player_x, float player_y, int allow);

/* every frame while playing. holding = the interact button is pressed on the shrine. returns 1 on the
 * frame the shrine turns polluted (on_done has just run) */
int   shrine_update(float dt, int holding);

float shrine_foot_y(void);                                  /* world px, for draw ordering */
void  shrine_draw(SDL_Renderer *r, int cam_x, int cam_y, float t);
int   shrine_collides(float feet_x, float feet_y, float half_w, float h);   /* blocks the player */

#endif
