/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * chunk 39: SEARCHING A DEAD NPC. like the grave search (grave.h, chunk 32) but for a lying npc (Alex's body in her house): once switched on, next to
 * the body the interact button shows the magnifying glass; the search is a HOLD (chunk 64: GRAVE_SEARCH_T seconds of holding the button, the same soft rustle;
 * letting go drains it back, walking away loses it); a full search calls on_done once and the search is over. engine only: what she carries is up to story.c. */
#ifndef BSEARCH_H
#define BSEARCH_H

void  bsearch_reset(int tile_size);                     /* a new map (the npc ids are gone) */
void  bsearch_enable(int npc_id, void (*on_done)(void));
int   bsearch_enabled(void);
int   bsearch_npc(void);                                /* whose body (-1: none) */
int   bsearch_near(float player_x, float player_y, int allow);   /* player feet, world px: 1 while the button shows the magnifying glass */
int   bsearch_update(float dt, int holding);            /* every frame; holding = the finger is on the button in reach; 1 on the frame it finishes */
int   bsearch_active(void);                             /* the finger is searching right now */
float bsearch_progress(void);

#endif
