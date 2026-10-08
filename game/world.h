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

/* scripting helpers (used from story.c) */
void world_set_controls_visible(int on);   /* analog stick + interact button */
int  world_player_tile_x(void);
int  world_player_tile_y(void);

#endif
