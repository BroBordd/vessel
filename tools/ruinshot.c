/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the ruins on screen, headless: build/ruin_spawn.bmp (the circle room, after the arrival), build/ruin_edge.bmp (on the near ledge, the pit ahead),
 * build/ruin_far.bmp (the far ledge):
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/ruinshot tools/ruinshot.c $(ls game/*.c | grep -v game/game.c) $(sdl2-config --libs) -lm
 *   SDL_AUDIODRIVER=dummy build/ruinshot */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include "world.h"
#define W 540
#define H 960
static SDL_Surface *surf; static SDL_Renderer *r;
static void frame(float dt) { world_update(dt); world_draw(r); }
static void run(float sec) { for (float t = 0; t < sec; t += 0.016f) frame(0.016f); }
static void shot(const char *name) { SDL_RenderPresent(r); SDL_SaveBMP(surf, name); printf("wrote %s\n", name); }
static void scene(const char *at, const char *file) {
    if (at) SDL_setenv("VESSEL_AT", at, 1); else SDL_setenv("VESSEL_AT", "", 1);
    world_init(W, H);
    run(16.0f);                                  /* the arrival: grooves fill, the circle charges, the body forms, the controls come back */
    shot(file);
}
int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    r = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    scene(NULL, "build/ruin_spawn.bmp");
    scene("edge", "build/ruin_edge.bmp");
    scene("far", "build/ruin_far.bmp");
    return 0;
}
