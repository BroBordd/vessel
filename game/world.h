/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef WORLD_H
#define WORLD_H
#include <SDL2/SDL.h>
#include "char.h"

void world_init(int w, int h);
/* a = Android MotionEvent action (0 down, 1 up, 2 move, 3 cancel) */
void world_touch(int a, int x, int y);
void world_update(float dt);
void world_draw(SDL_Renderer *r);

/* the maps the story can be on. world_init starts on the cloud map. */
enum { MAP_GREEN, MAP_CLOUD };

/* player-facing names of the maps. always use these in text, never the enum names. */
#define SKYLAND_NAME   "the Sky"
#define GRASSLANDS     "the Grasslands"       /* the ground map (MAP_GREEN) */
const char *world_map_name(int which);

/* scripting helpers (used from story.c) */
float world_debug_zoom(void);              /* current zoom (1 = none), for tests */
void world_set_controls_visible(int on);   /* analog stick + interact button */
int  world_player_tile_x(void);
int  world_player_tile_y(void);

/* finds a tile on the current map that the player can really walk to from where they stand (never
 * behind water or trees), with open ground all around it (so a character placed there is not
 * wedged in), roughly halfway between min_tiles and max_tiles away. same map = same answer every
 * time. returns 1 and fills tile_x / tile_y, or 0 if the map has no such place. if nothing fits the
 * range, it falls back to the farthest reachable open spot. */
int  world_find_far_spot(int min_tiles, int max_tiles, int *tile_x, int *tile_y);

/* the same, but the spot must also be at least avoid_dist tiles from (avoid_tx, avoid_ty): so two things placed
 * with the same range do not end up on top of each other. avoid_dist 0 = no restriction. */
int  world_find_spot_away(int min_tiles, int max_tiles, int avoid_tx, int avoid_ty, int avoid_dist, int *tile_x, int *tile_y);

/* Dia's shrine (shrine.h): world_place_shrine puts the prop on the current map (it blocks the player, but nothing
 * happens yet); world_enable_shrine switches it on: the interact button shows a sludge drop next to it, HOLDING it
 * pollutes it, and on_done runs once it is done. the shrine goes away when a new map loads. */
void world_place_shrine(int tile_x, int tile_y);
void world_enable_shrine(void (*on_done)(void));
void world_restore_shrine(int tile_x, int tile_y, int polluted);   /* the shrine as an earlier life left it (chunk 20): inert, and fouled for good if polluted */

/* GRAVES (grave.h, vessel 2 chunk 21): a stone with the dead vessel's framed, greyed portrait on it. world_place_grave puts one on the
 * current map (it blocks the player and shows as a grey block on the minimap) and returns its index (-1 = none free). like the shrine,
 * the graves go away when a new map loads. */
int  world_place_grave(int tile_x, int tile_y, const Person *dead);
/* the nearest tile to (want_tx, want_ty) where a grave fits (chunk 22): open ground all around it, clear of the shrine, the npcs, the other
 * graves and the player. looks 30 tiles around; returns 0 when there is no such tile. call it after the other props are placed. */
int  world_find_prop_spot_near(int want_tx, int want_ty, int *out_tx, int *out_ty);
int  world_grave_count(void);
float world_dist_to_grave(void);                          /* tiles from the player to the nearest grave on this map (1e9: none) */
void world_grave_tile(int i, float *tx, float *ty);       /* feet position in tiles */
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
void world_death_text(const char *text);       /* chunk 15: the words over the cutscene ("Aonia has died."), fade in and stay until the cutscene is cancelled */

/* THE SUMMONING (vessel 2, chunk 8, summon.h): a column of golden light drops onto the spot where the player stands, a glowing
 * ring spreads on the floor, golden sparks rise toward the light, the vessel forms in it (a gold silhouette from the feet up,
 * then its real colours), the light thins and ends. the controls are hidden meanwhile; on_done runs on the frame it ends (the
 * player is standing there in full colour; the controls stay hidden: on_done decides). the chime (sfx_summon) starts with it.
 * chunk 10 uses it at the start of the game. */
void world_summon(void (*on_done)(void));
int  world_summoning(void);                    /* it is running */

/* how far (in tiles) the player's feet are from an npc. a huge number if there is no such npc. */
float world_dist_to_npc(int npc_id);

/* opens a hole made of clouds in the floor at a tile (it grows open over ~1.5 s). once it is open,
 * standing close shows an arrow on the interact button; pressing it runs on_enter. */
void world_open_hole(int tile_x, int tile_y, void (*on_enter)(void));

/* the big drop: the player sinks into the hole (if there is one), falls through the sky, lands
 * face-first on the green map, lies there, gets up. THEN the controls come back and on_up runs
 * (that is where the next mission and npcs should be added; the old npcs are gone). */
void world_fall_to_green(void (*on_up)(void));

#endif
