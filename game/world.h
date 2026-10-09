/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef WORLD_H
#define WORLD_H
#include <SDL2/SDL.h>

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

/* opens a hole made of clouds in the floor at a tile (it grows open over ~1.5 s). once it is open,
 * standing close shows an arrow on the interact button; pressing it runs on_enter. */
void world_open_hole(int tile_x, int tile_y, void (*on_enter)(void));

/* the big drop: the player sinks into the hole (if there is one), falls through the sky, lands
 * face-first on the green map, lies there, gets up. THEN the controls come back and on_up runs
 * (that is where the next mission and npcs should be added; the old npcs are gone). */
void world_fall_to_green(void (*on_up)(void));

#endif
