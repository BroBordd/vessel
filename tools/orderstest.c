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
static int pulled_n;
static void pull_done(void) { pulled_n++; }
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

    /* chunk 29: Alex says no, once. no question, no chat; then the two thoughts, in order, the second a little later */
    lives = 1;
    int agx = (int)(pxp / tile) - 5, agy = (int)(pyp / tile);          /* our first body's stone, as landed_again would have put it (chunk 31) */
    aonia_grave = world_place_grave(agx, agy, &VESSEL);
    CHECK(aonia_grave >= 0, "could not place Aonia's grave");
    CHECK(!grave_pull_enabled(), "the grave pull was on before Alex said no");
    {   int bc = brainwin_count();
        on_talk_alex(0);
        CHECK(dialog_active() && !convo_active(), "Alex's refusal did not start as a plain dialog");
        int pages = 0;
        for (int i = 0; i < ALEX_REFUSE_COUNT + 2 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); pages++; }
        CHECK(!dialog_active() && !convo_active(), "the refusal left a dialog or a chat open (no question, no chat)");
        CHECK(pages >= ALEX_REFUSE_COUNT, "the refusal ran %d pages, want %d", pages, ALEX_REFUSE_COUNT);
        CHECK(brainwin_count() == bc + 1, "the first thought did not come right after the refusal (%d -> %d)", bc, brainwin_count());
        run(5.5f);
        CHECK(brainwin_count() == bc + 2, "the second thought did not come after the first (%d -> %d)", bc, brainwin_count());
        on_talk_alex(0);
        CHECK(!dialog_active() && !convo_active(), "the refusal played twice (or a chat opened)");
        run(6.0f);
        CHECK(brainwin_count() == bc + 2, "talking to her again added thoughts to the brain (%d)", brainwin_count() - bc);
    }
    lives = 0;
    CHECK(grave_pull_enabled() && grave_pull_index() == aonia_grave, "Alex's no did not switch the grave pull on");

    /* chunk 31: hold at the stone: the plot opens, he slides out feet first (1.5 s), then lies beside it; the task \"Bury Aonia\" coins in */
    {   int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        int tasks0 = missions_count(), bc = brainwin_count();
        pxp = (agx + 0.5f) * tile; pyp = (agy + 0.9f + 1.2f) * tile; run(0.3f);
        CHECK(!grave_is_open(aonia_grave) && !grave_body_out(aonia_grave), "the plot was open before the pull");
        world_touch(0, bx, by); run(GRAVE_PULL_HOLD_T + 0.3f);
        CHECK(aonia_out && grave_is_open(aonia_grave) && grave_body_moving(aonia_grave), "the pull did not open the plot and start the slide");
        CHECK(mission_bury >= 0 && missions_count() == tasks0 + 1, "the task \"Bury Aonia\" did not coin in");
        CHECK(brainwin_count() >= bc + 1, "the pull thought did not come (%d -> %d)", bc, brainwin_count());   /* (standing at his stone also brings the grave thought of chunk 23) */
        world_touch(1, bx, by);
        {   int gcx = (int)((agx + 0.5f) * tile) - (int)cam_x, gcy = (int)((agy + 0.9f) * tile) - (int)cam_y; (void)gcx; (void)gcy; }
        run(0.7f);
        CHECK(grave_body_moving(aonia_grave) && !grave_body_out(aonia_grave), "the slide ended too early");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/grave_slide.bmp");
        run(1.2f);
        CHECK(grave_body_out(aonia_grave) && !grave_body_moving(aonia_grave), "the body is not out after the slide");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/grave_out.bmp");
        float bxt, byt, gtx, gty; grave_body_tile(aonia_grave, &bxt, &byt); grave_tile(aonia_grave, &gtx, &gty);
        CHECK(bxt > gtx + 1.0f && fabsf(byt - gty) < 0.01f, "the body does not lie beside the plot (%.2f,%.2f vs %.2f,%.2f)", bxt, byt, gtx, gty);
        CHECK(!grave_pull_enabled(), "the pull is still on after the body was pulled out");
        /* backwards: the plot closes again */
        world_grave_open(aonia_grave, 0); run(0.7f);
        CHECK(grave_is_open(aonia_grave) && grave_body_moving(aonia_grave), "the close did not start sliding back");
        run(1.2f);
        CHECK(!grave_is_open(aonia_grave) && !grave_body_out(aonia_grave), "the plot did not close after the body slid back in");
        world_grave_open(aonia_grave, 1); run(2.0f);                     /* leave him out for the rest of the story */
        CHECK(grave_body_out(aonia_grave), "the body did not come out again");
    }

    /* chunk 30: the grave pull. a grave is placed near the player; nothing happens until the story switches it on; then the interact button
     * shows next to it, a short hold fills the bar and it creeps back, a full hold calls on_done once and the pull is over */
    {   int gx = (int)(pxp / tile) + 4, gy = (int)(pyp / tile);
        int gi = world_place_grave(gx, gy, &VESSEL);
        CHECK(gi >= 0, "could not place a grave for the pull test");
        pxp = (gx + 0.5f) * tile; pyp = (gy + 0.9f + 1.2f) * tile;       /* just in front of the stone */
        int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        run(0.3f);
        CHECK(!near_grave && !grave_pull_enabled(), "the grave pull is on before the story switched it on");
        world_touch(0, bx, by); run(GRAVE_PULL_HOLD_T + 0.5f); world_touch(1, bx, by); run(0.1f);
        CHECK(grave_pull_progress() == 0 && pulled_n == 0, "the button worked on a grave that was not switched on");

        world_enable_grave_pull(gi, pull_done);
        CHECK(grave_pull_enabled() && grave_pull_index() == gi, "the pull did not switch on");
        run(0.3f);
        CHECK(near_grave, "the button does not show next to the switched-on grave");
        world_touch(0, bx, by); run(0.1f);                                /* a tap is not enough */
        world_touch(1, bx, by); run(0.1f);
        CHECK(pulled_n == 0 && !dialog_active(), "a tap pulled the body (or opened a dialog)");
        world_touch(0, bx, by); run(1.0f);                                /* hold, then let go early */
        float p1 = grave_pull_progress();
        SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/pull_button.bmp");      /* the grip on the button, mid-hold (look at it) */
        CHECK(p1 > 0.25f && p1 < 0.6f, "one second of 2.5 should fill about 40%% (%.2f)", p1);
        world_touch(1, bx, by); run(1.0f);
        CHECK(grave_pull_progress() < p1 && grave_pull_progress() > 0, "the bar did not creep back (%.2f -> %.2f)", p1, grave_pull_progress());
        CHECK(pulled_n == 0, "letting go early still pulled the body");
        world_touch(0, bx, by); run(GRAVE_PULL_HOLD_T + 0.5f);            /* hold all the way */
        CHECK(pulled_n == 1, "a full hold did not call on_done exactly once (%d)", pulled_n);
        CHECK(!grave_pull_enabled() && !near_grave, "the pull is still on after it was done");
        world_touch(1, bx, by); run(GRAVE_PULL_HOLD_T + 0.5f);
        world_touch(0, bx, by); run(GRAVE_PULL_HOLD_T + 0.5f); world_touch(1, bx, by);
        CHECK(pulled_n == 1, "the pull ran again (%d)", pulled_n);
    }

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
