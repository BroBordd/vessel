/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef NPC_H
#define NPC_H
#include <SDL2/SDL.h>
#include "char.h"

/* places a character on a map tile. on_talk (may be NULL) runs when the player presses the
 * interact button while standing near them. returns the npc id, or -1 if full. */
int  npc_add(const Person *who, int tile_x, int tile_y, void (*on_talk)(int npc_id));

/* engine hooks, called by the world */
void  npc_reset(int pixel_scale, int tile_size);                /* clears all npcs */
void  npc_update(float player_x, float player_y);                /* player feet position, world px */
int   npc_nearby(void);                                         /* id of the closest npc in talking range, or -1 */
const Person *npc_person(int id);                               /* for drawing their face on the button */
void  npc_interact(int id);                                     /* runs the npc's on_talk */
int   npc_count(void);
float npc_foot_y(int id);                                       /* world px, for draw ordering */
void  npc_draw(SDL_Renderer *r, int id, int cam_x, int cam_y);
int   npc_collides(float feet_x, float feet_y, float half_w, float h);   /* blocks the player */

#endif
