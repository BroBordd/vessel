/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef WORLD_H
#define WORLD_H
#include <SDL2/SDL.h>
#include "char.h"

void world_init(int w, int h);
void world_resize(int w, int h);               /* the screen changed shape: new layout, the world / player / windows / zoom / story all stay (call at a frame boundary) */
/* a = Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel) */
void world_touch(int a, int x, int y);
void world_touch_p(int pid, int a, int x, int y);       /* one finger of several: 0 = the primary, else the app's pointer id + 1 (a thumb on the stick, one on the button) */
void world_update(float dt);
void world_draw(SDL_Renderer *r);

/* the maps the story can be on. world_init starts on the cloud map. */
enum { MAP_GREEN, MAP_CLOUD, MAP_TEMPLE };

/* player-facing names of the maps. always use these in text, never the enum names. */
#define SKYLAND_NAME   "the Sky"
#define TEMPLE_NAME    "the Ruins"            /* the map the game starts in (MAP_TEMPLE, made by gen.c) */
#define GRASSLANDS     "the Grasslands"       /* the ground map (MAP_GREEN) */
const char *world_map_name(int which);

/* scripting helpers (used from story.c) */
float world_debug_zoom(void);              /* current zoom (1 = none), for tests */
void world_set_controls_visible(int on);   /* analog stick + interact button */
int  world_player_tile_x(void);
int  world_player_tile_y(void);
void world_player_tilef(float *x, float *y);   /* where the feet are, in tiles, with the fraction */
int  world_controls_on(void);                  /* the player is free: the controls are showing and no cutscene runs (events.c waits for this) */
int  world_dragging(void);                     /* we hold a body */
int  world_dragging_rock(void);                /* we hold the big rock (story chunk 7) */
int  world_drag_moving(void);                  /* ... and this frame the pull really moved it */

/* finds a tile on the current map that the player can really walk to from where they stand (never
 * behind water or trees), with open ground all around it (so a character placed there is not
 * wedged in), roughly halfway between min_tiles and max_tiles away. same map = same answer every
 * time. returns 1 and fills tile_x / tile_y, or 0 if the map has no such place. if nothing fits the
 * range, it falls back to the farthest reachable open spot. */
int  world_find_far_spot(int min_tiles, int max_tiles, int *tile_x, int *tile_y);

/* the same, but the spot must also be at least avoid_dist tiles from (avoid_tx, avoid_ty): so two things placed
 * with the same range do not end up on top of each other. avoid_dist 0 = no restriction. */
int  world_find_spot_away(int min_tiles, int max_tiles, int avoid_tx, int avoid_ty, int avoid_dist, int *tile_x, int *tile_y);

/* ALEX'S HOUSE (house.h has the art and the layout). a solid log hut on the current map, with a door that is an interact button: it
 * swaps the whole map for the room inside (and back). world_find_house_spot is world_find_far_spot for the hut: the spot is the DOORSTEP
 * tile, and the hut (HUT_W x HUT_H tiles above it) with a margin must fit in the map. world_place_house puts the hut there (water and trees
 * round it are cleared) with `label` on a sign above it. the hut goes away when a new map loads (story.c puts it back).
 * the npcs of the Grasslands live INSIDE the house: from outside nobody is seen (world_dist_to_npc then measures to the door); the story
 * adds Alex at world_house_alex_tile() (a tile of the room's map). world_indoors: we are in the room. */
int  world_find_house_spot(int min_tiles, int max_tiles, int *tile_x, int *tile_y);
int  world_place_house(int doorstep_tx, int doorstep_ty, const char *label);
int  world_house_exists(void);
void world_house_tile(int *doorstep_tx, int *doorstep_ty);
void world_house_alex_tile(int *tile_x, int *tile_y);
int  world_indoors(void);

/* Dia's shrine (shrine.h): world_place_shrine puts the prop on the current map (it blocks the player, but nothing
 * happens yet); world_enable_shrine switches it on: the interact button shows a sludge drop next to it, HOLDING it
 * pollutes it, and on_done runs once it is done. the shrine goes away when a new map loads. */
void world_place_shrine(int tile_x, int tile_y);
void world_enable_shrine(void (*on_done)(void));
int  world_aiming(void);                          /* chunk 56: the aiming pose is on (the finger holds the "Die" button) (tests) */
float world_self_progress(void);                  /* chunk 56: the "Die" hold, 0..1 (tests) */
void world_enable_shrine_self(void (*on_hit)(void));    /* chunk 41: at the MENDED shrine the button is the hammer again; a full HOLD (chunk 56, aiming at the sky) swings it at ourselves (shrine.h): on_hit runs once */
void world_player_behead(void);                          /* chunk 41: our head comes off (flies, blood, the body topples and lies, no more walking) */
int  world_player_beheaded(void);
void world_found_reset(void);                               /* chunk 58: a new game: nothing has been found (a death does not call this) */
int  world_found_count(void);                               /* (tests) things found so far */
int  world_found_house(void);                               /* (tests) found flags */
int  world_found_shrine(void);
int  world_found_hammer(void);
int  world_found_temple(void);
int  world_found_grave(const Person *who);                  /* (tests) graves are told apart by whose they are */
void world_found_get(int flags[4], int *total, const Person **graves, int *n);                     /* the save file (story.c): house, shrine, hammer, temple, the count, and whose graves were found */
void world_found_set(const int flags[4], int total, const Person *const *graves, int n);
void world_player_explode(void);                            /* chunk 57: the whole body goes (a burst of red squares, nothing is left of it; no walking). story.c calls it when the hammer lands */
int  world_throwing(void);                                  /* chunk 57: (tests) the hammer is thrown and has not landed yet */
float world_throw_time(void);                               /* (tests) seconds since the throw, -1 when none */
int  world_player_gone(void);                               /* (tests) the body exploded: nothing of it is drawn */
int  world_hammer_draws(void);                              /* (tests) how many times the flying hammer was drawn */
void world_enable_shrine_repair(void (*on_start)(void), void (*on_done)(void));   /* chunk 40: the same hold with the hammer MENDS a broken shrine (shrine.h). on_start: the first time the hold starts; on_done: it is whole again */
void world_restore_shrine(int tile_x, int tile_y, int polluted);   /* the shrine as an earlier life left it (chunk 20): inert, and fouled for good if polluted */

/* GRAVES (grave.h, vessel 2 chunk 21): a stone with the dead vessel's framed, greyed portrait on it. world_place_grave puts one on the
 * current map (it blocks the player and shows as a grey block on the minimap) and returns its index (-1 = none free). like the shrine,
 * the graves go away when a new map loads. */
int  world_place_grave(int tile_x, int tile_y, const Person *dead);
/* the nearest tile to (want_tx, want_ty) where a grave fits (chunk 22): open ground all around it, clear of the shrine, the npcs, the other
 * graves and the player. looks 30 tiles around; returns 0 when there is no such tile. call it after the other props are placed. */
int  world_find_prop_spot_near(int want_tx, int want_ty, int *out_tx, int *out_ty);
/* the pull (grave.h, chunk 30): the interact button shows a grip next to grave i and HOLDING it fills a bar; when it is full on_done runs once */
void world_enable_grave_pull(int grave_index, void (*on_done)(void));
/* the search (grave.h, chunk 32): once body i lies on the grass and the player is next to it, the interact button shows a magnifying glass; a HOLD
 * (chunk 64, GRAVE_SEARCH_T, a tap is only a hint) searches and on_done runs once. world_grave_body_tile: where the body lies (feet, tiles). */
void world_enable_grave_search(int grave_index, void (*on_done)(void));
void world_grave_body_tile(int i, float *tx, float *ty);
/* the burial (grave.h, chunk 34): next to a body that lies on the grass the button shows a hand lowering him; HOLDING it drags him back into the plot, then on_done runs once */
void world_enable_grave_bury(int grave_index, void (*on_done)(void));
/* the fight (chunk 36): npc_id has been marked hostile (npc.h, npc_set_hostile) and the player carries the hammer: next to her the interact button is
 * a hammer swing instead of the talk face, and each full HOLD (HAMMER_HOLD_T, chunk 55: a tap is only a hint) hits (she flashes white and is shoved back, a thump, a short cooldown). after `hits` hits on_down
 * runs once and the fight is over. world_fight_left / world_fight_total: for the health bar (chunk 37). */
void world_enable_fight(int npc_id, int hits, void (*on_down)(void));
int  world_fight_left(void);
int  world_fight_total(void);
/* chunk 31: open the plot of grave i (the body slides out feet first, 1.5 s) or close it (it slides back in). grave.h has the rest (body_out, set_open ...) */
void world_grave_open(int grave_index, int open);
void world_grave_set_open(int grave_index, int open, int instant);   /* instant: no slide (a map made again) */
int  world_grave_count(void);
float world_dist_to_grave(void);                          /* tiles from the player to the nearest grave on this map (1e9: none) */
void world_grave_tile(int i, float *tx, float *ty);       /* feet position in tiles */
/* ITEMS (item.h): world_place_item lays one on the current map (the hand button takes it, chunk 68, walking over it does nothing; the map forgets it when a new one loads),
 * world_has_item asks the pockets, world_give_item fills them straight away, world_clear_items empties them (a new body). the shrine's
 * interact button only appears for a player who has the hammer (ITEM_HAMMER). */
void world_place_item(int kind, int tile_x, int tile_y);
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
int  world_body_tile(unsigned vessel, float *tx, float *ty);   /* story chunk 9: where a body lies (feet, tiles); 0 = no such body here */
void world_drop_item_near(int kind, float tx, float ty);       /* it falls on free floor around that spot */
void world_set_smash_cb(void (*cb)(unsigned vessel));          /* the big rock dragged into a body: the callback gets the vessel */
void world_ritual_c2(void (*on_done)(void));              /* story chunk 13: the next body forms at the second circle (ritual_c2_set); put the player there first */
void world_player_to_tile(float tile_x, float tile_y);     /* ... the feet go there (tiles) */
int  world_rock_in_reach(void);                           /* story chunk 12: the big rock is within the club's reach */
int  world_rock_break(float *tile_x, float *tile_y);       /* ... and it breaks (gone for good); where it lay, 0 = nothing */
unsigned world_body_in_reach(void);                       /* story chunk 8: the nearest body in reach (1 = Aonia), 0 = none: where a stab goes */
int  world_throw_item(int kind);                          /* story chunk 8: it flies the way we face; 1 = it fell into the pit (gone), 0 = it lies on the floor */
void world_set_inspect_cb(void (*cb)(unsigned vessel));   /* story chunk 6: next to a body the hand button is a magnifying glass; a tap calls this with the body's vessel number (1 = Aonia) */
void world_lock_item(int kind, int on, void (*on_blocked)(void));   /* chunk 33: a locked item on the ground is not picked up; on_blocked runs when someone walks into it (at most every 3 s) */
void world_enable_body_search(int npc_id, void (*on_done)(void));   /* chunk 39: next to that lying npc (indoors) the button shows the magnifying glass; a HOLD (chunk 64, a tap is only a hint) searches, then on_done once (bsearch.h) */
void world_drop_item_by_npc(int kind, int npc_id);        /* it falls onto the floor next to that npc (a free tile around her feet) */
void world_drop_item(int kind, int tile_x, int tile_y);     /* chunk 32: like world_place_item, but it falls there (cannot be picked up until it has landed) */
int  world_has_item(int kind);
void world_give_item(int kind);
void world_take_item(int kind);                /* chunk 43: out of the pockets */
void world_clear_items(void);
int  world_shrine_exists(void);                /* the shrine on the current map: is there one, is it polluted, and its feet position in tiles */
int  world_shrine_polluted(void);
void world_shrine_tile(float *tx, float *ty);

/* THE DEATH CUTSCENE (vessel 1's ending, chunks 12-15). world_death_begin(): the controls go, the player turns to face
 * us, the world freezes (no more wind, water or clouds), the camera pushes in slowly to 300 % on the player and the
 * colour drains from everything but the player, who goes red (gfx_set_filter). after ~2.6 s everything has arrived
 * and on_ready runs. the HUD (mission list, ID card, brain button) is not filtered.
 * world_death_player: where the player is on the screen right now, zoom included: feet centre, the chest (the
 * shirt rows: where the heart goes) and the screen size of one sprite pixel.
 * world_death_cancel: puts everything back (zoom eases out, colour returns, controls come back). only for stand-ins
 * and tests while the later chunks are not there yet. */
void world_death_begin(void (*on_ready)(void));
int  world_death_active(void);
void world_death_player(int *feet_x, int *feet_y, int *chest_y, int *pixel);
void world_death_cancel(void);
/* chunk 38: the cutscene about an npc (Alex): the camera pushes in on her, everything else goes grey (the player too) and she goes red. no heart: the
 * story plays the peeps and the flatline itself. world_death_cancel ends it as before. */
void world_death_begin_at(int npc_id, void (*on_ready)(void));
int  world_death_npc(void);                    /* the npc the cutscene is about, -1 for the player's own */
/* chunk 38: the one swing's kill: her head comes off and flies away from the player, red pixel squares splash everywhere and stay as stains on the
 * floor, the body falls and lies (npc.h). the stains and the head are the gore module's (gore.h) and only show inside the room. */
void world_npc_behead(int npc_id);
void world_npc_sparks(int npc_id, float delay);   /* chunk 38.1: the endless red squirts from the neck of a lying, headless npc (until the map is swapped) */

/* world_fade_to_black(seconds, on_black): everything on screen (HUD too) fades to black; on_black runs on the frame it is
 * fully dark and the screen then stays black until world_init (the story hands over to limbo there, chunk 12). */
void world_fade_to_black(float seconds, void (*on_black)(void));

/* a new life (chunk 14): back on the cloud map, the player at its spawn, hole closed, every effect undone, controls hidden, the world
 * fades in from black. nothing else of the story is touched (see story_reset_for_respawn) and the Grasslands stay as they are. */
void world_return_to_clouds(void);

/* the player's look (chunk 17): the Person drawn for the player everywhere in the world (walking, falling, lying, the death cutscene,
 * the summoning). the default is VESSEL (Aonia); NULL puts it back. world_player() says who it is right now. */
void world_set_player(const Person *who);
const Person *world_player(void);
void world_death_fall(void);                   /* chunk 15.1: the vessel topples over (1.1 s) and stays lying; world_death_player is for the standing pose only */
int  world_player_worried(void);                /* (tests) */
void world_set_player_worried(int on);          /* chunk 53: the player's body (every pose) has the worried eyes while on; story.c switches it with the hud card */
void world_death_text(const char *text);       /* chunk 15: the words over the cutscene ("Aonia has died."), fade in and stay until the cutscene is cancelled */

/* THE SUMMONING (vessel 2, chunk 8, summon.h): a column of golden light drops onto the spot where the player stands, a glowing
 * ring spreads on the floor, golden sparks rise toward the light, the vessel forms in it (a gold silhouette from the feet up,
 * then its real colours), the light thins and ends. the controls are hidden meanwhile; on_done runs on the frame it ends (the
 * player is standing there in full colour; the controls stay hidden: on_done decides). the chime (sfx_summon) starts with it.
 * chunk 10 uses it at the start of the game. */
void world_summon(void (*on_done)(void));
int  world_summoning(void);                    /* it is running */
void world_ritual(const Person *dead, void (*on_done)(void));   /* the story's start: a blood circle on the floor where the player stands, `dead` lying beside it, the new body formed out of the blood */
int  world_ritual_running(void);
void world_begin_dawn(void);                   /* the opening: the stars brighten into the sky, the cloud floor builds in, THEN the summoning runs */
int  world_dawning(void);                      /* the dawn is running (game.c draws the stars under the world and keeps the buttons away) */

/* the ruins' pit (gen.c): tiles from the player to the nearest pit tile (1e9: none on this map), and the spots on either side of it */
float world_dist_to_pit(void);
int   world_pit_spots(int *far_x, int *far_y, int *edge_x, int *edge_y);

/* how far (in tiles) the player's feet are from an npc. a huge number if there is no such npc. */
float world_dist_to_npc(int npc_id);

/* the cloud map: the tile where the goddess stands, up in her temple (far from where we land) */
void world_temple_spot(int *tile_x, int *tile_y);

/* opens a hole made of clouds in the floor at a tile (it grows open over ~1.5 s). once it is open,
 * standing close shows an arrow on the interact button; pressing it runs on_enter. */
void world_open_hole(int tile_x, int tile_y, void (*on_enter)(void));

/* the big drop: the player sinks into the hole (if there is one), falls through the sky, lands
 * face-first on the green map, lies there, gets up. THEN the controls come back and on_up runs
 * (that is where the next mission and npcs should be added; the old npcs are gone). */
void world_fall_to_green(void (*on_up)(void));

#endif
