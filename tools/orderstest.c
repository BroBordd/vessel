/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * chunk 26-27: Dea's orders and the chat lock. a short headless run of the second life's scolding: the naming comes first, then her orders (take the orb,
 * tell no one, do not complain), then her questions; the secret flag is off before and on after the orders. it starts at the respawn
 * (story_reset_for_respawn), so it does not depend on the long groundtest chain.
 * it #includes world.c and story.c to reach their statics. exits non-zero if a check fails:
 *   cc -O1 -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -o build/orderstest tools/orderstest.c $(ls game/*.c | grep -v -e game/game.c -e game/world.c -e game/story.c) $(sdl2-config --libs) -lm
 *   build/orderstest */
#define PULL_DECAY_T_TEST 3.2f
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
static int searched_n;
static void search_done(void) { searched_n++; }
static int locked_ended;
static void locked_end(void) { locked_ended++; }
static int talked_n, down_n;
static void fight_talk(int id) { (void)id; talked_n++; }
static void fight_down(void) { down_n++; }
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

    /* chunk 29: Alex says no, once. no question, no chat; then one thought (chunk 47: the \"I know where there is one\" thought is gone) */
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
        /* chunk 48: she talks to someone who is silent: no question for us to answer (one rhetorical remark about the silence at most,
         * never the first or the last line), and she never asks why we want it */
        {   int q = 0;
            for (int i = 0; i < ALEX_REFUSE_COUNT; i++) {
                const char *t = ALEX_REFUSE[i].text;
                for (const char *c = t; *c; c++) if (*c == '?') q++;
                CHECK(ALEX_REFUSE[i].who == &ALEX, "refusal line %d is not Alex's", i);
                CHECK(!strstr(t, "Why do you") && !strstr(t, "why do you"), "refusal line %d asks us why", i);
                if (i == 0 || i == ALEX_REFUSE_COUNT - 1) CHECK(!strchr(t, '?'), "refusal line %d (first or last) holds a question", i);
            }
            CHECK(q <= 1, "the refusal asks %d questions, one rhetorical remark at most", q);
        }
        CHECK(brainwin_count() == bc + 1, "the first thought did not come right after the refusal (%d -> %d)", bc, brainwin_count());
        run(5.5f);
        CHECK(brainwin_count() == bc + 1, "the refusal left %d thoughts, want exactly one (%d -> %d)", brainwin_count() - bc, bc, brainwin_count());
        /* chunk 47: walking up to a stone thinks nothing (the grave thought is gone) */
        {   float sx = pxp, sy = pyp;
            pxp = (agx + 0.5f) * tile; pyp = (agy + 0.9f + 2.5f) * tile; run(1.0f);
            CHECK(world_dist_to_grave() < 3.5f, "the player is not next to the stone for the grave check");
            run(6.0f);
            CHECK(brainwin_count() == bc + 1, "walking up to a stone added a thought (%d)", brainwin_count() - bc);
            pxp = sx; pyp = sy; run(0.3f);
        }
        on_talk_alex(0);
        CHECK(!dialog_active() && !convo_active(), "the refusal played twice (or a chat opened)");
        run(6.0f);
        CHECK(brainwin_count() == bc + 1, "talking to her again added thoughts to the brain (%d)", brainwin_count() - bc);
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
        CHECK(brainwin_count() >= bc + 1, "the pull thought did not come (%d -> %d)", bc, brainwin_count());
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

        /* chunk 32: search him. the search was switched on by the pull; it needs the body out and the player beside it. a tap searches (1 s),
         * the hammer falls out of his hand onto the grass next to him (it cannot be taken while it falls), once only */
        CHECK(grave_search_enabled() && grave_search_index() == aonia_grave, "the pull did not switch the search on");
        CHECK(!item_exists() && !item_has(ITEM_HAMMER), "a hammer lay on the map before the search");
        run(0.3f);
        CHECK(!near_search, "the magnifying glass shows at the stone, 3 tiles from the body");
        float sbx, sby; grave_body_tile(aonia_grave, &sbx, &sby);
        pxp = sbx * tile; pyp = (sby + 1.2f) * tile; run(0.3f);                 /* beside him */
        CHECK(near_search && !near_grave, "the magnifying glass does not show next to the body");
        world_touch(0, bx, by); world_touch(1, bx, by); run(0.4f);              /* one tap */
        CHECK(grave_search_active(), "a tap did not start the search");
        CHECK(grave_search_progress() > 0.2f && grave_search_progress() < 0.7f, "0.4 s of a 1 s search is at %.2f", grave_search_progress());
        CHECK(!item_exists() && !aonia_searched, "the hammer came before the search was over");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/search_button.bmp");   /* the glass sweeping over the button (look at it) */
        int sbc = brainwin_count();
        run(0.8f);
        CHECK(aonia_searched && item_exists() && !grave_search_enabled(), "the search did not end with the hammer on the ground");
        CHECK(item_dropping(), "the hammer does not fall (it was placed at once)");
        float hx_, hy_; item_tile(&hx_, &hy_);
        CHECK(fabsf(hx_ - sbx) < 1.5f && hy_ > sby && hy_ < sby + 2.0f, "the hammer is not on the grass next to him (%.2f,%.2f vs %.2f,%.2f)", hx_, hy_, sbx, sby);
        CHECK(!item_has(ITEM_HAMMER), "the hammer was in the pockets while it fell");
        CHECK(brainwin_count() == sbc + 1, "the search thought did not come (%d -> %d)", sbc, brainwin_count());
        pxp = (sbx + 3.0f) * tile; run(0.8f);                                    /* (the fall is over; step off so nothing picks it up) */
        CHECK(!item_dropping() && item_exists(), "the hammer did not land");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/search_hammer.bmp");   /* the hammer on the grass beside him (look at it) */
        CHECK(!near_search, "the magnifying glass shows after the search was done");
        world_touch(0, bx, by); world_touch(1, bx, by); run(1.5f);
        CHECK(brainwin_count() == sbc + 1, "searching again added a thought");

        /* chunk 33: respect the dead. he is unburied, so the hammer is locked: walking onto it does not take it; the task flashes red, the error
         * buzz sounds and the vessel thinks (once in the window); one try every 3 s at most */
        CHECK(item_locked(ITEM_HAMMER), "the hammer is not locked while he lies unburied");
        int e0 = sfx_debug_errors(), b0 = item_blocked_count(), tc0 = brainwin_count();
        pxp = hx_ * tile; pyp = hy_ * tile; frame(0.016f);
        CHECK(!item_has(ITEM_HAMMER) && item_exists(), "walking onto the locked hammer took it");
        CHECK(sfx_debug_errors() == e0 + 1 && item_blocked_count() == b0 + 1, "the error buzz / the turned-away try were not counted (%d, %d)", sfx_debug_errors() - e0, item_blocked_count() - b0);
        CHECK(mission_flashing(mission_bury), "the task Bury Aonia does not flash");
        CHECK(brainwin_count() == tc0 + 1, "the 'not like this' thought did not come once (%d -> %d)", tc0, brainwin_count());
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/respect_flash.bmp");           /* the red, shaking line (look at it) */
        run(0.8f);
        CHECK(!mission_flashing(mission_bury), "the flash did not end after %.1f s", MISSION_FLASH_T);
        run(1.0f);                                                                  /* still standing on it, 1.8 s in: too soon for another */
        CHECK(sfx_debug_errors() == e0 + 1, "a second buzz came within 3 s (%d)", sfx_debug_errors() - e0);
        run(1.6f);                                                                  /* past 3 s: again, but the window keeps one thought */
        CHECK(sfx_debug_errors() == e0 + 2 && mission_flashing(mission_bury), "no second buzz / flash after 3 s (%d)", sfx_debug_errors() - e0);
        CHECK(brainwin_count() == tc0 + 1 && !item_has(ITEM_HAMMER), "the thought was added twice, or the hammer was taken");
        pxp = (sbx + 3.0f) * tile; run(0.3f);

        /* chunk 34: bury him. after the search the button next to the body is a hand lowering him; HOLDING it drags him back into the plot, the task
         * completes and the hammer is free. a short hold only fills the bar a little and it creeps back */
        CHECK(grave_bury_enabled() && grave_bury_index() == aonia_grave, "the search did not switch the burial on");
        int mc0 = missions_count();
        pxp = sbx * tile; pyp = (sby - 1.0f) * tile; run(0.3f);
        CHECK(near_bury && !near_search && !near_grave, "the burial button does not show next to the body after the search");
        world_touch(0, bx, by); run(0.8f); world_touch(1, bx, by); run(0.1f);
        CHECK(grave_bury_progress() > 0.1f && grave_bury_progress() < 0.5f && grave_bury_enabled() && grave_body_out(aonia_grave), "a short hold did something wrong (%.2f)", grave_bury_progress());
        CHECK(item_locked(ITEM_HAMMER) && !aonia_buried, "a short hold buried him");
        run(PULL_DECAY_T_TEST); CHECK(grave_bury_progress() == 0, "the bar did not creep back (%.2f)", grave_bury_progress());
        world_touch(0, bx, by); run(1.4f);
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/bury_button.bmp");             /* the hand over the pit, earth rising (look at it) */
        run(GRAVE_BURY_HOLD_T);
        world_touch(1, bx, by); run(0.1f);
        CHECK(aonia_buried && !grave_bury_enabled(), "a full hold did not bury him");
        CHECK(grave_body_moving(aonia_grave) || !grave_is_open(aonia_grave), "he was not dragged back into the plot");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/bury_slide.bmp");              /* him sliding back in (look at it) */
        run(2.0f);
        CHECK(!grave_is_open(aonia_grave) && !grave_body_out(aonia_grave), "the plot did not close");
        CHECK(!item_locked(ITEM_HAMMER), "the hammer is still locked after the burial");
        CHECK(missions_count() == mc0 - 1, "the task Bury Aonia did not complete and fade (%d -> %d)", mc0, missions_count());
        CHECK(!near_bury, "the burial button shows after he was buried");
        lives = 1;                                                              /* (the real game is in its second life here) */
        pxp = hx_ * tile; pyp = hy_ * tile; frame(0.016f);
        CHECK(item_has(ITEM_HAMMER), "the unlocked hammer was not picked up after the burial");
        /* chunk 35: the hammer is ours: the flat thought, once, the flag for a new landing, the task stays */
        {   int tc1 = brainwin_count(), mc1 = missions_count();
            run(0.3f);
            CHECK(hammer_taken, "the pickup did not set hammer_taken");
            CHECK(thought_active(), "no thought when the hammer was picked up");
            CHECK(brainwin_count() >= tc1, "the hammer thought was not kept in the brain window (%d -> %d)", tc1, brainwin_count());
            CHECK(mission_get_orb >= 0 && missions_count() == mc1, "the task list changed on the hammer pickup");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/hammer_ours.bmp");   /* the thought card (look at it) */
        }
        lives = 0;
        item_clear(); item_reset(px, tile);                                      /* tidy up for the tests that follow */
    }

    /* chunk 32, engine: walking away cancels a search; a second body can be searched too; once only */
    {   int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        int gx = (int)(pxp / tile) + 3, gy = (int)(pyp / tile) + 8;
        int gi = world_place_grave(gx, gy, &VESSEL2);
        CHECK(gi >= 0, "could not place a grave for the search test");
        world_grave_set_open(gi, 1, 1);
        searched_n = 0;
        world_enable_grave_search(gi, search_done);
        float sbx, sby; grave_body_tile(gi, &sbx, &sby);
        pxp = sbx * tile; pyp = (sby + 1.2f) * tile; run(0.3f);
        CHECK(near_search, "the glass does not show next to the second body");
        world_touch(0, bx, by); world_touch(1, bx, by); run(0.4f);
        pxp = (sbx + 6.0f) * tile; run(1.5f);                                    /* walks away mid-search */
        CHECK(searched_n == 0 && !grave_search_active() && grave_search_enabled(), "walking away did not cancel the search (%d)", searched_n);
        pxp = sbx * tile; run(0.3f);
        CHECK(near_search && grave_search_progress() == 0, "the search did not start over from nothing");
        world_touch(0, bx, by); world_touch(1, bx, by); run(1.3f);
        CHECK(searched_n == 1, "a whole search did not call on_done exactly once (%d)", searched_n);
        world_touch(0, bx, by); world_touch(1, bx, by); run(1.5f);
        CHECK(searched_n == 1 && !near_search, "the search ran twice (%d)", searched_n);
        /* a grave whose body is still in the ground cannot be searched */
        int gj = world_place_grave(gx + 4, gy, &VESSEL2);
        searched_n = 0; world_enable_grave_search(gj, search_done);
        float tbx, tby; grave_tile(gj, &tbx, &tby);
        pxp = (tbx + 3.0f) * tile; pyp = (tby + 1.2f) * tile; run(0.3f);
        CHECK(!near_search, "the glass shows for a body that is still buried");
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

    /* chunk 36: the swing. a hostile npc + the hammer: the button is a swing, each tap hits (flash, shove, thump, cooldown), N hits call on_down
     * once. without the hammer, or without the flag, a tap talks instead */
    {   int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        int ntx = (int)(pxp / tile) + 6, nty = (int)(pyp / tile) - 6;
        talked_n = 0; down_n = 0;
        int nid = npc_add(&ALEX, ntx, nty, fight_talk);
        CHECK(nid >= 0, "could not add the npc for the fight test");
        pxp = (ntx + 0.5f) * tile; pyp = (nty + 0.9f + 1.0f) * tile;       /* one tile in front of her */
        item_clear(); item_reset(px, tile);
        run(0.3f);
        CHECK(near_id == nid, "the test npc is not in reach (%d)", near_id);
        tap(bx, by);                                                       /* no flag, no hammer: a talk */
        CHECK(talked_n == 1, "a tap next to a plain npc did not talk (%d)", talked_n);
        world_give_item(ITEM_HAMMER);
        npc_set_hostile(nid, 1);
        world_enable_fight(nid, 3, fight_down);
        CHECK(world_fight_left() == 3 && world_fight_total() == 3, "the fight did not start with 3 hits (%d/%d)", world_fight_left(), world_fight_total());
        run(0.3f);
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/swing_button.bmp");      /* the hammer on the button (look at it) */
        int h0 = sfx_debug_hits();
        tap(bx, by);
        CHECK(sfx_debug_hits() == h0 + 1 && world_fight_left() == 2, "the first tap did not hit (%d hits, %d left)", sfx_debug_hits() - h0, world_fight_left());
        CHECK(talked_n == 1 && !dialog_active(), "a swing also talked");
        CHECK(npc_flashing(nid), "the npc did not flash white");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/swing_flash.bmp");       /* her, flashing white and shoved back (look at it) */
        tap(bx, by);                                                       /* straight away: the hammer is still coming up */
        CHECK(sfx_debug_hits() == h0 + 1 && world_fight_left() == 2, "a tap inside the cooldown hit again (%d left)", world_fight_left());
        run(0.3f); CHECK(!npc_flashing(nid), "the flash did not end");
        run(FIGHT_COOLDOWN);
        tap(bx, by);
        CHECK(sfx_debug_hits() == h0 + 2 && world_fight_left() == 1 && down_n == 0, "the second hit went wrong (%d left, down %d)", world_fight_left(), down_n);
        run(FIGHT_COOLDOWN + 0.1f);
        tap(bx, by);
        CHECK(world_fight_left() == 0 && down_n == 1, "the third hit did not call on_down once (%d)", down_n);
        run(FIGHT_COOLDOWN + 0.1f);
        tap(bx, by); run(FIGHT_COOLDOWN + 0.1f); tap(bx, by);
        CHECK(down_n == 1 && sfx_debug_hits() == h0 + 3, "the fight went on after it was over (down %d, hits %d)", down_n, sfx_debug_hits() - h0);
        CHECK(talked_n >= 2, "after the fight a tap does not talk again (%d)", talked_n);
        /* a hostile npc without the hammer is just talked to */
        item_clear(); item_reset(px, tile);
        npc_set_hostile(nid, 1); world_enable_fight(nid, 2, fight_down);
        run(0.3f); int t0 = talked_n; tap(bx, by);
        CHECK(world_fight_left() == 2 && talked_n == t0 + 1, "without the hammer the tap hit instead of talking");
        world_enable_fight(-1, 0, NULL);
        item_clear(); item_reset(px, tile);
    }

    /* chunk 37: one swing. in her house, with the hammer taken (chunk 35), Alex is hostile and the hammer button is on; ONE tap is the end of the
     * fight (no health bar, no lines). before that (no hammer taken / not in the house) nothing is switched on */
    {   int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        int ntx = (int)(pxp / tile) - 6, nty = (int)(pyp / tile) - 6;
        for (int ok = 0, oy = -12; oy <= 12 && !ok; oy++) for (int ox = -12; ox <= 12 && !ok; ox++) {      /* an open patch of floor: 7 x 6 tiles with nothing solid (the head and the blood need room) */
            int cx_ = (int)(pxp / tile) + ox, cy_ = (int)(pyp / tile) + oy, free_ = 1;
            for (int j = -4; j <= 1 && free_; j++) for (int i = -3; i <= 3 && free_; i++) if (solid_tile(cx_ + i, cy_ + j)) free_ = 0;
            if (free_ && abs(ox) + abs(oy) >= 2) { ntx = cx_; nty = cy_; ok = 1; }
        }
        int was_buried = aonia_buried;
        lives = 1; alex_fight_on = alex_struck = 0; hammer_taken = 0; aonia_buried = 0;      /* (not buried: hammer_watch does not take the hammer for us) */
        alex_id = npc_add(&ALEX, ntx, nty, fight_talk);
        pxp = (ntx + 0.5f) * tile; pyp = (nty + 0.9f + 1.0f) * tile;
        item_clear(); item_reset(px, tile); world_give_item(ITEM_HAMMER);
        indoors = 1; run(0.3f);
        CHECK(!npc_hostile(alex_id) && world_fight_left() == 0, "Alex is hostile before the hammer was taken the story's way");
        hammer_taken = 1; indoors = 0; run(0.3f);
        CHECK(!npc_hostile(alex_id) && world_fight_left() == 0, "Alex is hostile while we are not in her house");
        indoors = 1; run(0.3f);
        CHECK(npc_hostile(alex_id) && world_fight_left() == 1 && world_fight_total() == 1, "Alex is not hostile with the hammer in her house (%d, %d hits)", npc_hostile(alex_id), world_fight_left());
        int h0 = sfx_debug_hits(), mc = missions_count(), tc = brainwin_count(), pf0 = sfx_debug_pfft();
        tap(bx, by);
        CHECK(alex_struck && sfx_debug_hits() == h0 + 1 && world_fight_left() == 0, "one tap did not end it (struck %d, hits %d)", alex_struck, sfx_debug_hits() - h0);
        CHECK(npc_flashing(alex_id) && !npc_hostile(alex_id), "she did not flash, or is still hostile after the hit");
        CHECK(!dialog_active() && missions_count() == mc && brainwin_count() == tc, "the hit opened a dialog / changed the tasks / added a thought (chunk 38 does that)");
        /* chunk 38: the head comes off, the blood splashes, she falls */
        CHECK(npc_headless(alex_id) && npc_lying(alex_id), "she is not headless and lying after the hit");
        CHECK(gore_head_exists() && gore_drops_flying() > 0, "no head / no blood in the air after the hit (%d, %d)", gore_head_exists(), gore_drops_flying());
        run(0.25f); frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/alex_blood.bmp");        /* the splash in the air (look at it) */
        CHECK(!world_death_active(), "the death cutscene began before the blood had landed");
        run(FIGHT_COOLDOWN + 0.2f); tap(bx, by); run(0.3f);
        CHECK(sfx_debug_hits() == h0 + 1, "a second hit landed after the end");
        run(1.0f);                                                         /* about 2.2 s after the hit */
        CHECK(!gore_busy() && gore_head_settled(), "the head / the blood did not come to rest");
        CHECK(gore_stain_count() >= 25, "the blood left only %d stains", gore_stain_count());
        {   float hx, hy, ax, ay; gore_head_pos(&hx, &hy); npc_tile(alex_id, &ax, &ay);
            CHECK(fabsf(hx - ax * tile) + fabsf(hy - ay * tile) > 1.0f * tile, "the head lies where the body is (it did not fly): head %.1f,%.1f body %.1f,%.1f tile %d", hx, hy, ax * tile, ay * tile, tile); }
        CHECK(world_death_active() && world_death_npc() == alex_id, "the death cutscene is not on Alex (%d, %d)", world_death_active(), world_death_npc());
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/alex_stains.bmp");                    /* the stains on the floor, the head (look at it) */
        {   int b0 = sfx_debug_beeps(), f0 = sfx_debug_flatlines();
            run(DEATH_SETTLE_T + 0.4f);
            CHECK(zoom > 2.0f, "the camera did not push in on her (%.2f)", zoom);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/alex_death.bmp");                 /* grey room, red body (look at it) */
            run(DEATH_BEEPS * DEATH_BEEP_GAP + 0.3f);
            CHECK(sfx_debug_beeps() - b0 == DEATH_BEEPS && sfx_debug_flatlines() - f0 == 1, "the peeps / the flatline are wrong (%d, %d)", sfx_debug_beeps() - b0, sfx_debug_flatlines() - f0);
            run(ALEX_TEXT_AT + 0.6f);
            CHECK(strcmp(death_msg, "Alex has died.") == 0, "the words are missing (\"%s\")", death_msg);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/alex_text.bmp");                  /* the words (look at it) */
            CHECK(!alex_dead, "alex_dead before the cutscene ended");
            CHECK(sfx_debug_pfft() == pf0 && gore_spark_bursts() == 0, "the sparks started during the death scene (%d pfft, %d bursts)", sfx_debug_pfft() - pf0, gore_spark_bursts());
            run(ALEX_END_HOLD + 0.5f);
            CHECK(!world_death_active() && world_death_npc() < 0 && alex_dead, "the world is not free after the words (%d)", world_death_active());
            run(2.0f);
            CHECK(zoom < 1.2f, "the camera did not ease out (%.2f)", zoom);
        }
        /* chunk 38.1: the sparks. quiet through the whole cutscene, then the endless idle: bursts, a pfft each, the body shakes */
        {   int p0 = sfx_debug_pfft(), b0 = gore_spark_bursts(), shook = 0, flew = 0, ox, oy;
            CHECK(gore_sparks_running(), "the sparks are not running after the cutscene");
            for (float sec = 0; sec < 8.0f; sec += 0.016f) {
                frame(0.016f);
                gore_spark_shake(&ox, &oy); if (ox || oy) shook++;
                if (gore_sparks_flying() > 0) flew++;
            }
            int bursts = gore_spark_bursts() - b0, pf = sfx_debug_pfft() - p0;
            CHECK(bursts >= 4 && bursts <= 9, "the burst counter is off after 8 s (%d)", bursts);
            CHECK(pf == bursts, "the pfft is not played once per burst (%d pfft, %d bursts)", pf, bursts);
            CHECK(shook > 0 && flew > 0, "the body never shook / no squares flew (%d, %d)", shook, flew);
            frame(0.0f); frame(0.0f);
            {   int t = 0; for (float sec = 0; sec < 3.0f && !gore_sparks_flying(); sec += 0.016f) frame(0.016f);
                for (int k = 0; k < 3; k++) { frame(0.016f); t++; }
                frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/alex_sparks.bmp"); }          /* a squirt from the neck (look at it) */
            CHECK(gore_stain_count() >= 25 && gore_stain_count() < 320, "the sparks changed the stains (%d)", gore_stain_count());
            CHECK(!gore_busy(), "the endless sparks count as busy");
            /* nobody in the room: nothing squirts and nothing is heard */
            indoors = 0; run(0.1f);
            int b1 = gore_spark_bursts(), p1 = sfx_debug_pfft();
            run(4.0f);
            CHECK(gore_spark_bursts() == b1 && sfx_debug_pfft() == p1, "it squirts while we are outside the room");
            indoors = 1; run(3.0f);
            CHECK(gore_spark_bursts() > b1, "it did not go on when we came back in");
        }
        /* chunk 39: taking the orb. she is down and the cutscene is over: next to her body the button is the magnifying glass; a tap searches her once and the
         * orb goes into the pockets, the task is swapped, and nothing else happens (no thought, no dialog) */
        {   CHECK(bsearch_enabled() && bsearch_npc() == alex_id, "the body search is not switched on after her death");
            CHECK(!orb_taken && !world_has_item(ITEM_ORB) && mission_repair < 0, "the orb / the next task is there before the search");
            CHECK(!item_exists(), "an orb lies on the ground (it must not)");
            pxp = (ntx + 0.5f) * tile; pyp = (nty + 0.9f + 1.0f) * tile; indoors = 1; run(0.3f);
            CHECK(near_bsearch, "no magnifying glass next to her body");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/orb_button.bmp");                   /* the glass on the button (look at it) */
            int tc2 = brainwin_count(), pf = sfx_debug_pfft(); (void)pf;
            tap(bx, by); run(0.5f);
            CHECK(bsearch_active() && !world_has_item(ITEM_ORB), "the search did not start with a tap, or finished at once");
            run(0.7f);
            CHECK(world_has_item(ITEM_ORB) && orb_taken, "no orb in the pockets after the search");
            CHECK(mission_repair >= 0, "\"Repair the shrine\" did not come in");
            CHECK(!dialog_active() && brainwin_count() == tc2, "the search opened a dialog / added a thought (it must be silent)");
            CHECK(!bsearch_enabled() && !near_bsearch, "she can be searched twice");
            int n_orb = mission_repair; tap(bx, by); run(0.5f);
            CHECK(mission_repair == n_orb && world_has_item(ITEM_ORB), "a second tap did something");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/orb_taken.bmp");                    /* the slot with the orb (look at it) */
            item_ui_touch(0, TW - 40, 120); item_ui_touch(1, TW - 40, 120); run(0.6f);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/orb_panel.bmp");                    /* the unfolded pockets: hammer and orb (look at it) */
        }
        /* chunk 40: the repair. the shrine is broken (as vessel 1 left it), we hold the hammer and have the orb: the hold mends it, only the hold: letting go makes
         * it creep back. the first hold thinks once. whole afterwards, the task is done, no second thought */
        {   CHECK(world_has_item(ITEM_HAMMER), "the hammer is not in the pockets for the repair test");
            int sx_ = ntx + 2, sy_ = nty;
            indoors = 0; world_restore_shrine(sx_, sy_, 1);
            pxp = (sx_ + 0.5f) * tile; pyp = (sy_ + 0.9f + 1.0f) * tile; run(0.3f);
            CHECK(shrine_polluted() && !shrine_repairing(), "the shrine is not broken / is armed before the repair was switched on");
            frame(0.0f); world_touch(0, bx, by); world_touch(1, bx, by); run(0.5f);
            CHECK(!near_shrine, "the repair button shows before the repair is switched on");
            shrine_arm_repair();
            run(0.3f);
            CHECK(shrine_repairing() && near_shrine, "the button is not there with the hammer next to the broken shrine (%d, %d)", shrine_repairing(), near_shrine);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/repair_0.bmp");                   /* broken, before the hold (look at it) */
            int tc3 = brainwin_count(), r0 = 0;
            run(1.0f);                                                                        /* a tap / no hold does nothing */
            CHECK(shrine_progress() == 0.0f && shrine_polluted(), "progress moved without a hold");
            world_touch(0, bx, by); run(0.8f);
            CHECK(shrine_progress() > 0.25f && shrine_progress() < 0.45f && shrine_polluted(), "hold 0.8 s: wrong progress (%.2f)", shrine_progress());
            CHECK(brainwin_count() == tc3 + 1, "the first hold did not think once (%d new)", brainwin_count() - tc3); r0 = brainwin_count();
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/repair_1.bmp");                   /* the stump has grown (look at it) */
            run(0.5f); frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/repair_2.bmp");        /* the pillar, no gem yet (look at it) */
            world_touch(1, bx, by);                                                           /* let go early: it creeps back, it does not mend */
            float pr = shrine_progress(); run(1.5f);
            CHECK(shrine_progress() < pr && shrine_progress() > 0 && shrine_polluted() && !shrine_repaired, "letting go: it did not creep back (%.2f -> %.2f)", pr, shrine_progress());
            world_touch(0, bx, by); run(2.0f);
            CHECK(shrine_polluted() && !shrine_repaired, "it mended before a full hold (%.2f)", shrine_progress());
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/repair_3.bmp");                   /* nearly there, the gem is coming (look at it) */
            run(1.5f);
            CHECK(!shrine_polluted() && shrine_repaired && !shrine_repairing(), "the full hold did not mend it");
            CHECK(brainwin_count() == r0, "a second thought came with the later holds (%d)", brainwin_count() - r0);
            world_touch(1, bx, by); run(0.5f);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/repair_4.bmp");                   /* whole again (look at it) */
            /* chunk 41: "Die". the task is there and the hammer button is back at the mended shrine: a TAP, not a hold, swings it at our own head. the same end as
             * Alex's: head off, blood, the body down, then the death scene on us, the stone, limbo, the next life with the orb and nothing else */
            CHECK(mission_die >= 0, "\"Die\" did not come in when the shrine was mended");
            CHECK(shrine_self() && near_shrine, "the hammer button is not back at the mended shrine (%d, %d)", shrine_self(), near_shrine);
            run(1.0f);
            CHECK(!world_player_beheaded(), "it swung by itself");
            int h1 = sfx_debug_hits(), l0 = lives, g0 = grave_n, bp0 = sfx_debug_beeps(), fl0 = sfx_debug_flatlines();
            tap(bx, by);
            CHECK(sfx_debug_hits() == h1 + 1 && world_player_beheaded(), "the tap did not swing (%d hits, beheaded %d)", sfx_debug_hits() - h1, world_player_beheaded());
            CHECK(gore_head_exists() && gore_drops_flying() > 0, "no head / no blood in the air after the swing");
            CHECK(self_done && dying && !shrine_self(), "the swing's state is wrong (%d, %d, %d)", self_done, dying, shrine_self());
            run(0.3f); frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/self_blood.bmp");        /* the splash (look at it) */
            CHECK(!world_death_active(), "the death scene began before the blood had landed");
            run(1.9f);
            CHECK(!gore_busy() && gore_head_settled() && gore_stain_count() >= 25, "the head / the blood did not come to rest (%d stains)", gore_stain_count());
            CHECK(world_death_active() && world_death_npc() < 0, "the death scene is not on us");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/self_lying.bmp");                  /* the body, the stains, the head (look at it) */
            run(DEATH_SETTLE_T + 0.4f);
            CHECK(zoom > 2.0f, "the camera did not push in on us (%.2f)", zoom);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/self_death.bmp");                  /* grey room, red body (look at it) */
            run(DEATH_BEEPS * DEATH_BEEP_GAP + 0.3f);
            CHECK(sfx_debug_beeps() - bp0 == DEATH_BEEPS && sfx_debug_flatlines() - fl0 == 1, "the peeps / the flatline are wrong (%d, %d)", sfx_debug_beeps() - bp0, sfx_debug_flatlines() - fl0);
            run(ALEX_TEXT_AT + 0.6f);
            CHECK(strcmp(death_msg, "Doia has died.") == 0, "the words are wrong (\"%s\")", death_msg);
            run(DEATH_END_HOLD + 0.5f);
            CHECK(vessel_dead && lives == l0 + 1 && grave_n == g0 + 1, "the life did not end (dead %d, lives %d, graves %d)", vessel_dead, lives, grave_n - g0);
            CHECK(graves[grave_n - 1].who == &VESSEL2, "the stone is not Doia's");
            run(DEATH_FADE_T + 0.5f);
            CHECK(story_limbo_take_enter() == 1 && dialog_active(), "no limbo after the black / the soul does not speak");
            for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
            for (float sl = 0; sl < DEATH_LIMBO_HOLD + 0.5f; sl += 0.016f) { story_limbo_update(0.016f); frame(0.016f); }   /* (game.c runs the soul's own clock in limbo) */
            CHECK(death_hold_done >= 1, "the next life was not made ready");
            CHECK(world_has_item(ITEM_ORB) && !world_has_item(ITEM_HAMMER), "the pockets of the next life are wrong (orb %d, hammer %d)", world_has_item(ITEM_ORB), world_has_item(ITEM_HAMMER));
            /* chunk 42: vessel 3. the summoning in the clouds brings us back already looking like Tria, the ID card is still plain VAS until Dea names us; her one
             * line flashes the card (the moment it begins), nothing else is asked, the chat lock stands and the orb is still ours. no hole, nowhere to fall to */
            CHECK(lives == 2 && world_player() == &VESSEL3, "the third body is not Tria's look (lives %d)", lives);
            CHECK(hud_person() != &VESSEL3, "the ID card said Tria before the naming");
            CHECK(dea_id >= 0 && !hole_on, "Dea is not waiting in her temple / a hole is open (%d, %d)", dea_id, hole_on);
            run(10.0f);
            CHECK(scold_pending && !dialog_active(), "the third summoning did not end with Dea waiting to speak (%d)", scold_pending);
            on_talk_dea(dea_id);
            CHECK(!scold_pending && dialog_active() && !convo_active(), "talking to Dea did not start her line");
            CHECK(hud_person() == &VESSEL3 && world_player() == &VESSEL3, "the naming did not flash the ID card / change the look");
            CHECK(strstr(DEA_NAMING3[0].text, "number three") && strstr(DEA_NAMING3[0].text, VESSEL3_NAME), "Dea's line does not name number three");
            CHECK(naming3_done_n == 0, "the naming ended before the line was read");
            for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
            CHECK(!dialog_active() && naming3_done_n == 1, "the naming did not end after one line (%d)", naming3_done_n);
            CHECK(controls_visible && !hole_on, "the controls did not come back / a hole opened after the naming");
            CHECK(convo_locked() && secret_on, "the chat lock / the secret ended with the naming (chunk 43 ends it)");
            CHECK(world_has_item(ITEM_ORB), "the orb left the pockets with the naming");
            /* chunk 43: giving the orb. the task is there after the naming; the orb leaves the pockets when Dea answers, the task is done, the order ends (lock and
             * secret off) and story_vessel3_ready runs once. */
            CHECK(mission_give >= 0 && missions_count() >= 1, "\"Give the orb to Dea\" did not coin in after the naming");
            CHECK(vessel3_ready_calls == 0 && !orb_given, "the hand-over happened before we talked to her");
            { int mc = missions_count();
              on_talk_dea(dea_id);
              CHECK(dialog_active() && !convo_active() && world_has_item(ITEM_ORB), "talking to Dea with the orb did not start the hand-over (orb still ours)");
              run(2.0f); tap(TW / 2, TH * 4 / 5); run(0.5f);
              CHECK(!world_has_item(ITEM_ORB) && orb_given, "the orb did not leave the pockets when Dea answered");
              CHECK(vessel3_ready_calls == 0, "the hook ran before the talk ended");
              for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
              CHECK(!dialog_active() && vessel3_ready_calls == 1, "story_vessel3_ready did not run once (%d)", vessel3_ready_calls);
              CHECK(!convo_locked() && !secret_on, "the order did not end with the hand-over (locked %d, secret %d)", convo_locked(), secret_on);
              run(2.0f); CHECK(missions_count() <= mc, "the task list grew after the hand-over"); }
            { int rc = vessel3_ready_calls;
              world_give_item(ITEM_ORB);                          /* (a second talk, orb back in the pockets: no second hand-over; relock so it is not a typed chat) */
              convo_lock("I can't say that. I was told to keep quiet."); convo_lock_reply(&DEA, "You heard me. Go.");
              on_talk_dea(dea_id);
              CHECK(vessel3_ready_calls == rc && world_has_item(ITEM_ORB), "a second talk handed the orb over again");
              for (int i = 0; i < 4 && (dialog_active() || convo_active()); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
              world_take_item(ITEM_ORB); }
            /* without the orb she nags (a fresh third life state) */
            { int og = orb_given; orb_given = 0; world_take_item(ITEM_ORB);
              on_talk_dea(dea_id);
              CHECK(dialog_active() && !world_has_item(ITEM_ORB), "Dea did not nag without the orb");
              for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
              CHECK(vessel3_ready_calls == 1 && !orb_given, "the nag handed something over");
              orb_given = og;
              convo_lock("I can't say that. I was told to keep quiet."); convo_lock_reply(&DEA, "You heard me. Go."); secret_on = 1; }   /* (the rest of this test goes on as the second life, locked) */
            /* the three looks differ (hair, shirt, skin), on a screenshot too */
            CHECK(memcmp(&VESSEL3.hair, &VESSEL.hair, 3) && memcmp(&VESSEL3.hair, &VESSEL2.hair, 3), "Tria's hair matches another vessel");
            CHECK(memcmp(&VESSEL3.shirt, &VESSEL.shirt, 3) && memcmp(&VESSEL3.shirt, &VESSEL2.shirt, 3), "Tria's shirt matches another vessel");
            CHECK(memcmp(&VESSEL3.skin, &VESSEL.skin, 3) && memcmp(&VESSEL3.skin, &VESSEL2.skin, 3), "Tria's skin matches another vessel");
            CHECK(strcmp(VESSEL3.name, "Tria") == 0, "vessel 3 is not called Tria");
            SDL_SetRenderDrawColor(rr, 70, 90, 120, 255); SDL_RenderClear(rr);
            { const Person *three[3] = { &VESSEL, &VESSEL2, &VESSEL3 };
              for (int k = 0; k < 3; k++) {
                  char_draw(rr, three[k], 90 + k * 180, 330, FACE_DOWN, 0, 0.0f, 8);
                  char_draw_portrait(rr, three[k], 40 + k * 180, 420, 8, 0);
              } }
            SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/three_looks.bmp");                          /* Aonia, Doia, Tria (look at it) */
            lives = 1; vessel_dead = 0;                                                       /* (the rest of this test goes on as the second life) */
            shrine_reset(px, tile); item_clear(); item_reset(px, tile); world_give_item(ITEM_HAMMER);
        }
        alex_id = -1; alex_struck = alex_fight_on = 0; hammer_taken = 0; aonia_buried = was_buried; indoors = 0; lives = 0;
        item_clear(); item_reset(px, tile);
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
