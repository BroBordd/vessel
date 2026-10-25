/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef WORLD_H
#define WORLD_H
#include <SDL2/SDL.h>
#include "char.h"

void world_init(int w, int h);
void world_resize(int w, int h);               /* the screen changed shape: new layout, the world / player / windows / story all stay (call at a frame boundary) */
/* a = Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel) */
void world_touch(int a, int x, int y);
void world_touch_p(int pid, int a, int x, int y);       /* one finger of several: 0 = the primary, else the app's pointer id + 1 (a thumb on the stick, one on the button) */
void world_update(float dt);
void world_draw(SDL_Renderer *r);

/* player-facing name of the map: the world has one map, the ruins (made by gen.c) */
#define TEMPLE_NAME    "the Ruins"

/* scripting helpers (used from story.c and events.c) */
void world_set_controls_visible(int on);   /* analog stick + interact button */
void world_player_tilef(float *x, float *y);   /* where the feet are, in tiles, with the fraction */
int  world_controls_on(void);                  /* the player is free: the controls are showing (events.c waits for this) */
int  world_dragging(void);                     /* we hold a body */
int  world_dragging_rock(void);                /* we hold the big rock (story chunk 7) */
int  world_drag_moving(void);                  /* ... and this frame the pull really moved it */

/* ITEMS (item.h): world_place_item lays one on the map (the hand button takes it, walking over it does nothing; the map forgets it when a new one loads),
 * world_drop_item makes it fall there (it cannot be picked up until it has landed), world_has_item asks the pockets, world_give_item fills them straight
 * away, world_take_item takes one out, world_clear_items empties them (a new body). */
void world_place_item(int kind, int tile_x, int tile_y);
void world_drop_item(int kind, int tile_x, int tile_y);
int  world_has_item(int kind);
void world_give_item(int kind);
void world_take_item(int kind);
void world_clear_items(void);

/* story chunk 11: THE PIT. world_pit_add: a body went in (the bridge column gets one more floor tile; returns the crack tiles still open), world_pit_pile: how many
 * went in, world_pit_reset: a new game. the pile outlives the map. world_set_pit_cb: a dragged body was pushed over the edge (it is already out of the list).
 * world_pit_push_time: seconds we have been walking into the crack (about 1 s is a fall; the story kills us). world_swing_across: fly to the far ledge, then done runs */
void world_pit_reset(void);
int  world_pit_pile(void);
int  world_pit_left(void);
int  world_pit_add(void);
void world_set_pit_cb(void (*cb)(unsigned vessel));
float world_pit_push_time(void);
void world_pit_push_clear(void);
int  world_swing_across(void (*done)(void));
/* the ruins' pit (gen.c): tiles from the player to the nearest pit tile (1e9: none on this map), and the spots on either side of it */
float world_dist_to_pit(void);
int   world_pit_spots(int *far_x, int *far_y, int *edge_x, int *edge_y);

/* bodies, the big rock, throwing */
int  world_body_tile(unsigned vessel, float *tx, float *ty);   /* story chunk 9: where a body lies (feet, tiles); 0 = no such body here */
int  world_free_tile_near(float tx, float ty, int *tile_x, int *tile_y);   /* story chunk 15: the nearest tile that is not solid (0 = none) */
void world_drop_item_near(int kind, float tx, float ty);       /* it falls on free floor around that spot */
void world_set_smash_cb(void (*cb)(unsigned vessel));          /* the big rock dragged into a body: the callback gets the vessel */
int  world_rock_in_reach(void);                           /* story chunk 12: the big rock is within the club's reach */
int  world_rock_break(float *tile_x, float *tile_y);       /* ... and it breaks (gone for good); where it lay, 0 = nothing */
unsigned world_body_in_reach(void);                       /* story chunk 8: the nearest body in reach (1 = Aonia), 0 = none: where a stab goes */
int  world_throw_item(int kind);                          /* story chunk 8: it flies the way we face; 1 = it fell into the pit (gone), 0 = it lies on the floor */
void world_set_inspect_cb(void (*cb)(unsigned vessel));   /* story chunk 6: next to a body the hand button is a magnifying glass; a tap calls this with the body's vessel number (1 = Aonia) */

/* the blood circle (ritual.c): world_ritual carves it where the player stands with `dead` lying beside it, the new body forms out of the blood and on_done
 * (may be NULL) runs then, the controls come back. world_ritual_c2: the same at the second circle (story chunk 13): put the player on it first (world_player_to_tile) */
void world_ritual(const Person *dead, void (*on_done)(void));
void world_ritual_c2(void (*on_done)(void));
int  world_ritual_running(void);
void world_player_to_tile(float tile_x, float tile_y);     /* the feet go there (tiles) */

/* world_fade_to_black(seconds, on_black): everything on screen (HUD too) fades to black; on_black runs on the frame it is
 * fully dark and the screen then stays black until world_reload_ruins (the story hands over to limbo there). */
void world_fade_to_black(float seconds, void (*on_black)(void));

/* a new life: the ruins again (the same seed gives the same map), the player at the spawn, the fades undone, the controls hidden
 * (the blood circle gives them back). nothing else of the story is touched (see story.c). */
void world_reload_ruins(void);

/* the player's look: the Person drawn for the player everywhere in the world. the default is VESSEL (Aonia); NULL puts it back. */
void world_set_player(const Person *who);
const Person *world_player(void);

#endif
