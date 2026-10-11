/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * chunk 26: Dea's orders. a short headless run of the second life's scolding: the naming comes first, then her orders (take the orb,
 * tell no one, do not complain), then her questions; the secret flag is off before and on after the orders. it starts at the respawn
 * (story_reset_for_respawn), so it does not depend on the long groundtest chain.
 * it #includes world.c and story.c to reach their statics. exits non-zero if a check fails:
 *   cc -O1 -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -o build/orderstest tools/orderstest.c $(ls game/*.c | grep -v -e game/game.c -e game/world.c -e game/story.c) $(sdl2-config --libs) -lm
 *   build/orderstest */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include "world.c"
#include "story.c"
#include "lang.h"
#include "pausebtn.h"

#define TW 540
#define TH 1170

static SDL_Renderer *rr; static SDL_Surface *surf;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void frame(float dt) {
    thought_set_enabled(1);
    world_update(dt); thought_update(dt);
    world_draw(rr); thought_draw(rr);
}
static void run(float sec) { for (float s = 0; s < sec; s += 0.016f) frame(0.016f); }
static void tap(int x, int y) { world_touch(0, x, y); frame(0.016f); world_touch(1, x, y); frame(0.016f); }

int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, TW, TH, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    lang_seed(7);
    brainwin_init(TW, TH);
    world_init(TW, TH);

    story_start();                                   /* a new game: the flag is off */
    CHECK(!secret_on, "the secret order is in force on a new game");
    story_reset_for_respawn();                       /* the next life: Dea waits in her temple, we are plain VAS */
    CHECK(!secret_on, "the secret order is in force before Dea spoke");
    run(0.5f);
    CHECK(DEA_ORDERS_COUNT >= 2, "Dea's orders are missing (%d lines)", DEA_ORDERS_COUNT);

    dea_scold();                                     /* what talking to / seeing Dea does on the second life */
    CHECK(dialog_active() && !secret_on, "the scolding did not start, or the order came before it");
    int named_at = -1, secret_at = -1, pages = 0;
    for (int i = 0; i < 12 && dialog_active() && !convo_active(); i++) {
        run(2.0f);
        if (named_at < 0 && hud_person() == &VESSEL2) named_at = i;
        if (secret_at < 0 && secret_on) secret_at = i;
        tap(TW / 2, TH * 4 / 5); pages++;
    }
    CHECK(named_at >= 0, "the naming never happened");
    CHECK(secret_on, "Dea's orders did not set the secret flag");
    CHECK(secret_at > named_at, "the orders did not come after the naming (named %d, secret %d)", named_at, secret_at);
    CHECK(pages >= DEA_SCOLD_COUNT + DEA_ORDERS_COUNT, "the orders did not run their %d pages after the scolding (%d taps)", DEA_ORDERS_COUNT, pages);
    CHECK(convo_active() && dialog_active(), "Dea did not go on to her questions after the orders");
    CHECK(!controls_visible, "the controls came back before the questions were over");

    printf(fails ? "orderstest: %d FAILED\n" : "orderstest: ok\n", fails);
    return fails ? 1 : 0;
}
