/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * chunk 26-27: Dea's orders and the chat lock. a short headless run of the second life's scolding: the naming comes first, then her orders (take the orb,
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
static int locked_ended;
static void locked_end(void) { locked_ended++; }
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
    for (int i = 0; i < DEA_SCOLD_COUNT + DEA_ORDERS_COUNT && dialog_active(); i++) {   /* the scolding and the orders; her locked line comes after */
        run(2.0f);
        if (named_at < 0 && hud_person() == &VESSEL2) named_at = i;
        if (secret_at < 0 && secret_on) secret_at = i;
        tap(TW / 2, TH * 4 / 5); pages++;
    }
    CHECK(named_at >= 0, "the naming never happened");
    CHECK(secret_on, "Dea's orders did not set the secret flag");
    CHECK(secret_at > named_at, "the orders did not come after the naming (named %d, secret %d)", named_at, secret_at);
    CHECK(pages >= DEA_SCOLD_COUNT + DEA_ORDERS_COUNT, "the orders did not run their %d pages after the scolding (%d taps)", DEA_ORDERS_COUNT, pages);
    /* chunk 27: the order closes the typed chat. Dea's "questions" are now just her line "You heard me. Go." (a popup, no chat window),
     * and then the controls and the hole come back */
    CHECK(convo_locked(), "the chat is not locked after the orders");
    CHECK(dialog_active() && !convo_active(), "Dea's questions opened a typed chat although the order stands");
    CHECK(!controls_visible, "the controls came back before her last line");
    for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
    CHECK(!dialog_active() && !convo_active(), "her last line did not end");
    CHECK(controls_visible && hole_on, "the controls / hole did not come back after her last line");

    /* chunk 28: the second landing: the task is not there at once (the landing thought comes first), then it coins in once */
    CHECK(missions_count() == 0 && mission_get_orb < 0, "the orb task exists before the second landing");
    landed_again();
    CHECK(vessel2_ready_calls == 1, "story_vessel2_ready did not run once (%d)", vessel2_ready_calls);
    CHECK(mission_get_orb < 0 && missions_count() == 0, "the task came in with the landing thought, not after it");
    run(VESSEL2_TASK_DELAY + 0.5f);
    CHECK(mission_get_orb >= 0 && missions_count() == 1, "\"Get the orb from Alex\" did not coin in after the landing (%d tasks)", missions_count());
    run(VESSEL2_TASK_DELAY + 0.5f);
    CHECK(missions_count() == 1, "the task came in twice");

    /* talking to anyone while locked: no chat window, the vessel thinks; Dea answers with her own line; on_end still runs */
    convo_open(&ALEX, &ALEX_MIND, 1, locked_end);
    CHECK(!convo_active() && !dialog_active(), "Alex's chat opened while locked");
    CHECK(thought_active(), "no thought about the order when talking to Alex");
    CHECK(locked_ended == 1, "on_end of a locked chat did not run (%d)", locked_ended);
    convo_ask_questions(&ALEX, &ALEX_MIND, locked_end);
    CHECK(!convo_active() && locked_ended == 2, "convo_ask_questions opened a chat while locked");
    convo_open(&DEA, &DEA_MIND, 1, locked_end);
    CHECK(!convo_active() && dialog_active() && locked_ended == 2, "Dea did not answer with her own line (popup, no chat)");
    for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
    CHECK(!dialog_active() && locked_ended == 3, "Dea's locked line did not run on_end (%d)", locked_ended);
    /* scripted dialogs are not affected */
    {   static const DialogLine one[] = { { &ALEX, "Hello." } };
        dialog_play(one, 1, NULL); CHECK(dialog_active(), "a scripted dialog was blocked by the lock");
        for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); } }
    /* unlocked: the typed chat opens again */
    convo_unlock();
    CHECK(!convo_locked(), "convo_unlock did not unlock");
    convo_open(&ALEX, &ALEX_MIND, 1, NULL);
    CHECK(convo_active(), "the chat does not open after convo_unlock");

    printf(fails ? "orderstest: %d FAILED\n" : "orderstest: ok\n", fails);
    return fails ? 1 : 0;
}
