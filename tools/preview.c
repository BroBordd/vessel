/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * renders screenshots without a phone (BMP files in build/):
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/preview tools/preview.c $(ls game/*.c | grep -v game/game.c) $(sdl2-config --libs) -lm
 *   build/preview            -> build/shot_world.bmp, build/shot_talk.bmp */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include "world.h"
#include "dialog.h"
#include "talk.h"
#include "convo.h"
#include "story.h"
#include "lang.h"

#define W 540
#define H 1170

static SDL_Surface *surf; static SDL_Renderer *r;
static void shot(const char *name) { SDL_RenderPresent(r); SDL_SaveBMP(surf, name); printf("wrote %s\n", name); }
static void frame(float dt) { world_update(dt); world_draw(r); }
static void tap(int x, int y) { world_touch(0, x, y); frame(0.016f); world_touch(1, x, y); frame(0.016f); }
static void run(float sec) { for (float t = 0; t < sec; t += 0.016f) frame(0.016f); }

int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    r = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    lang_seed(7);
    world_init(W, H);
    run(1.5f);                                   /* fade-in + intro dialog */
    for (int i = 0; i < 2; i++) { run(1.5f); tap(W / 2, H / 2); }   /* dismiss "Hello" and "You have been summoned" */
    run(1.0f);
    shot("build/shot_world.bmp");
    {   /* walk up to Dea, press the interact button, and watch the zoom lerp in */
        int u1 = 26 * W / 360, sr = 58 * W / 360, sx = u1 + sr, sy = H - u1 - sr;
        world_touch(0, sx, sy); world_touch(2, sx, sy - 60);
        run(1.9f);
        world_touch(1, sx, sy - 60); run(0.3f);
        int br = 44 * W / 360, bx = W - u1 - br, by = H - u1 - br;
        tap(bx, by);
        for (int i = 0; i < 12; i++) { run(0.1f); printf("zoom %.3f\n", world_debug_zoom()); }
        run(3.0f);
        shot("build/shot_zoom.bmp");
    }
    /* the conversation screens, driven directly (no need to walk to Dea) */
    extern const Person DEA, VESSEL;
    static Persona mind = { "Dea", STYLE_DIVINE, 0, 30, 65, 8 };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    dialog_init(W, H);
    convo_set_player(&VESSEL);
    convo_ask_questions(&DEA, &mind, NULL);
    for (float t = 0; t < 3.0f; t += 0.016f) dialog_update(0.016f);
    SDL_SetRenderDrawColor(r, 126, 188, 246, 255); SDL_RenderClear(r);
    dialog_draw(r); shot("build/shot_question.bmp");
    /* TALK is bottom-right inside the box: sweep the area until the overlay opens */
    for (int y = H - 30; y > H - 120 && !talk_active(); y -= 4) for (int x = W - 30; x > W - 120 && !talk_active(); x -= 4) { dialog_touch(0, x, y); dialog_touch(1, x, y); }
    printf("talk overlay: %d\n", talk_active());
    for (float t = 0; t < 0.5f; t += 0.016f) dialog_update(0.016f);
    talk_debug_type("hello! who are you? thanks");
    for (float t = 0; t < 0.3f; t += 0.016f) dialog_update(0.016f);
    SDL_SetRenderDrawColor(r, 126, 188, 246, 255); SDL_RenderClear(r);
    dialog_draw(r); shot("build/shot_talk.bmp");
    return 0;
}
