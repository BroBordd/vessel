/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef NPC_H
#define NPC_H
#include <SDL2/SDL.h>
#include "char.h"

/* places a character on a map tile. on_talk (may be NULL) runs when the player presses the
 * interact button while standing near them. returns the npc id, or -1 if full. */
int  npc_add(const Person *who, int tile_x, int tile_y, void (*on_talk)(int npc_id));

/* the way they stand when nobody is close (default: facing down). they turn to look at the
 * player while in talking range, and go back to this once the player walks away. */
void npc_set_facing(int id, int facing);

/* chunk 36: a hostile npc can be hit with the hammer (the world's interact button turns into a swing for a player who carries it; she never
 * attacks). npc_hit makes her flash white and shoves her back a little, away from (from_x, from_y) (world px); npc_tick (every frame) runs the
 * flash and the recoil. the recoil is only drawn: her tile does not move. */
void npc_set_hostile(int id, int on);
int  npc_hostile(int id);
void npc_hit(int id, float from_x, float from_y);
int  npc_flashing(int id);
void npc_tick(float dt);

/* engine hooks, called by the world */
void  npc_reset(int pixel_scale, int tile_size);                /* clears all npcs */
void  npc_update(float player_x, float player_y);                /* player feet position, world px */
int   npc_nearby(void);                                         /* id of the closest npc in talking range, or -1 */
const Person *npc_person(int id);                               /* for drawing their face on the button */
void  npc_interact(int id);                                     /* runs the npc's on_talk */
int   npc_count(void);
void  npc_tile(int id, float *tx, float *ty);                   /* feet position in tiles (for the minimap) */
float npc_foot_y(int id);                                       /* world px, for draw ordering */
void  npc_draw(SDL_Renderer *r, int id, int cam_x, int cam_y);
int   npc_collides(float feet_x, float feet_y, float half_w, float h);   /* blocks the player */

#endif
