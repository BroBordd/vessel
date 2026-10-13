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
#include "mapicon.h"
#include "talk.h"

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
static void inv_run(float sec) { for (float s_ = 0; s_ < sec; s_ += 0.016f) item_update(0.016f); }          /* chunk 70: real frame steps (the ease needs them) */
static void tap(int x, int y) { world_touch(0, x, y); frame(0.016f); world_touch(1, x, y); frame(0.016f); }
static int say(const char *text) {                  /* chunk 63: open the TALK keyboard like the player does and send a line */
    run(2.5f);
    for (int y = TH - 30; y > TH - 120 && !talk_active(); y -= 4) for (int x = TW - 30; x > TW - 120 && !talk_active(); x -= 4) tap(x, y);
    if (!talk_active()) return 0;
    talk_debug_type(text); run(0.3f); talk_debug_send();
    return 1;
}
static unsigned region_hash(int x, int y, int w, int h) {        /* chunk 66: a checksum of a part of the picture */
    unsigned v = 2166136261u; const Uint32 *px = (const Uint32 *)surf->pixels; int pitch = surf->pitch / 4;
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) if (i >= 0 && j >= 0 && i < TW && j < TH) v = (v ^ px[j * pitch + i]) * 16777619u;
    return v;
}
/* chunk 69: the box of everything drawn in a part of the picture that is not black (w, h in px; 0 when empty) */
static void lit_box(int x0, int y0, int x1, int y1, int *w, int *h) {
    int minx = 1 << 30, miny = 1 << 30, maxx = -1, maxy = -1; const Uint32 *px_ = (const Uint32 *)surf->pixels; int pitch = surf->pitch / 4;
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        Uint32 v = px_[y * pitch + x];
        if ((int)(v & 255) + (int)((v >> 8) & 255) + (int)((v >> 16) & 255) > 40) { if (x < minx) minx = x; if (x > maxx) maxx = x; if (y < miny) miny = y; if (y > maxy) maxy = y; }
    }
    *w = maxx < 0 ? 0 : maxx - minx + 1; *h = maxy < 0 ? 0 : maxy - miny + 1;
}
static int hint_ref = -1;
/* chunk 65: freeze the hint at its peak and count how far the fill rises on the left of the button (the icon is not there): every tool must show the same */
static void hint_check(const char *tool) {
    hint_t = HINT_ANIM_T * 0.5f; frame(0.0f); SDL_RenderPresent(rr);
    int x = bcx - (int)(0.75f * br), half = (int)(0.66f * br) - 2, n = 0;
    const Uint32 *px = (const Uint32 *)surf->pixels; int pitch = surf->pitch / 4;
    for (int y = bcy - half; y <= bcy + half; y++) {
        Uint32 v = px[y * pitch + x]; int r_ = v & 255, g_ = (v >> 8) & 255, b_ = (v >> 16) & 255;
        if (abs(r_ - 24) + abs(g_ - 28) + abs(b_ - 48) > 60) n++;
    }
    if (hint_ref < 0) hint_ref = n;
    CHECK(n >= 2 && abs(n - hint_ref) <= 1, "the hint's fill on the %s button is %d px high, the first tool's was %d", tool, n, hint_ref);
    hint_t = 0;
}
static void hold_swing(int x, int y) { world_touch(0, x, y); run(HAMMER_HOLD_T + 0.15f); world_touch(1, x, y); frame(0.016f); }   /* chunk 55: the hammer is a hold */

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

    /* chunk 49: the very first visit. when the summoning ends the first task is "Look around"; sighting Dea ticks it off and "Talk to Goddess"
     * coins in right after; neither comes in twice */
    CHECK(mission_look < 0 && mission_talk_dea < 0 && missions_count() == 0, "a task exists before the first summoning ends");
    intro_done();
    CHECK(mission_look >= 0 && mission_talk_dea < 0 && missions_count() == 1, "\"Look around\" did not coin in when the summoning ended (%d tasks)", missions_count());
    intro_done();
    CHECK(missions_count() == 1, "\"Look around\" came in twice");
    run(1.0f);
    CHECK(mission_talk_dea < 0, "\"Talk to Goddess\" came in before Dea was sighted");
    dea_spotted();
    CHECK(mission_talk_dea >= 0 && mission_talk_dea != mission_look && missions_count() == 2, "\"Talk to Goddess\" did not coin in on the sighting (%d tasks)", missions_count());
    dea_spotted();
    CHECK(missions_count() == 2, "\"Talk to Goddess\" came in twice");
    run(4.0f);                                       /* the ticked line fades away: only \"Talk to Goddess\" is left */
    CHECK(missions_count() == 1, "\"Look around\" was not completed on the sighting (%d tasks left)", missions_count());
    story_reset_for_respawn();                       /* the next life: Dea waits in her temple, we are plain VAS */
    CHECK(mission_look < 0 && missions_count() == 0, "\"Look around\" survived into the next life (%d tasks)", missions_count());
    CHECK(!secret_on, "the secret order is in force before Dea spoke");
    run(0.5f);
    /* chunk 50: later lives. the task is not there while the summoning runs; when it ends "Talk to Goddess" coins in at once (no "Look around"),
     * the sighting does not coin it in again, and Dea's scolding ticks it off */
    CHECK(mission_talk_dea < 0 && mission_look < 0 && missions_count() == 0, "a task exists before the second summoning ends");
    respawn_summoned();
    CHECK(mission_talk_dea >= 0 && mission_look < 0 && missions_count() == 1, "\"Talk to Goddess\" did not coin in at once after the second summoning (%d tasks)", missions_count());
    dea_watch(); dea_spotted();
    CHECK(missions_count() == 1, "the sighting coined \"Talk to Goddess\" in a second time (%d tasks)", missions_count());
    scold_pending = 0; world_set_controls_visible(0);       /* (respawn_summoned gave them back; the scolding below starts without them) */
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
        /* chunk 63: vessel 2 talks to Alex like vessel 1: the order locks Dea, not her. her typed chat opens, she greets us, normal words get
         * normal replies, and asking for the orb gets her no (once the first no sets alex_refused; the head and the grave start when the chat ends) */
        CHECK(convo_locked() && !alex_refused && !alex_flow, "the lock / refusal state is wrong before the talk");
        on_talk_alex(0);
        CHECK(convo_active() && dialog_active(), "Alex's typed chat did not open although the order stands (chunk 63)");
        CHECK(strstr(thought_debug_text(), "can't say") == NULL, "a \"can't say that\" thought appeared");
        CHECK(say("hello"), "could not type to Alex");
        run(2.5f);
        CHECK(convo_active() && !alex_refused, "an ordinary line made her refuse (or ended the chat)");
        CHECK(brainwin_count() == bc, "an ordinary line started the flow");
        CHECK(say("give me the orb"), "could not ask Alex for the orb");
        run(2.5f);
        CHECK(alex_refused && alex_no_n == 1 && !alex_flow, "the first refusal did not set alex_refused once (refused %d, no %d, flow %d)", alex_refused, alex_no_n, alex_flow);
        CHECK(convo_active(), "the chat ended right at her no (the player may still talk)");
        CHECK(!grave_pull_enabled() && brainwin_count() == bc, "the flow started before the chat ended");
        CHECK(say("bye"), "could not say bye");
        for (int i = 0; i < 6 && (dialog_active() || convo_active()); i++) { run(2.5f); tap(TW / 2, TH * 4 / 5); }
        CHECK(!dialog_active() && !convo_active(), "the chat did not end after bye");
        CHECK(alex_flow && grave_pull_enabled(), "leaving the chat after her no did not start the grave flow");
        CHECK(brainwin_count() == bc + 1, "the thought did not come right after the chat (%d -> %d)", bc, brainwin_count());
        run(5.5f);
        CHECK(brainwin_count() == bc + 1, "the refusal left %d thoughts, want exactly one", brainwin_count() - bc);
        /* the words that count as asking for the orb */
        CHECK(asks_for_orb("Give me the orb") && asks_for_orb("ORB?") && asks_for_orb("can I have the orbs"), "asks_for_orb missed an orb");
        CHECK(!asks_for_orb("absorb") && !asks_for_orb("hello") && !asks_for_orb("who are you?"), "asks_for_orb fired on other words");
        /* chunk 47: walking up to a stone thinks nothing (the grave thought is gone) */
        {   float sx = pxp, sy = pyp;
            pxp = (agx + 0.5f) * tile; pyp = (agy + 0.9f + 2.5f) * tile; run(1.0f);
            CHECK(world_dist_to_grave() < 3.5f, "the player is not next to the stone for the grave check");
            run(6.0f);
            CHECK(brainwin_count() == bc + 1, "walking up to a stone added a thought (%d)", brainwin_count() - bc);
            pxp = sx; pyp = sy; run(0.3f);
        }
        /* a second talk: her normal chat again, asking again gets a no, but the flow does not run twice */
        on_talk_alex(0);
        CHECK(convo_active(), "the second talk with Alex did not open a chat");
        CHECK(say("please give me the orb"), "could not ask again");
        run(6.0f);
        CHECK(alex_no_n == 2 && alex_flow, "the second ask was not answered with a no (no %d)", alex_no_n);
        CHECK(say("bye"), "could not say bye the second time");
        for (int i = 0; i < 6 && (dialog_active() || convo_active()); i++) { run(6.0f); tap(TW / 2, TH * 4 / 5); }
        run(6.0f);
        CHECK(!convo_active() && brainwin_count() == bc + 1, "talking to her again added thoughts to the brain (%d)", brainwin_count() - bc);
        /* Dea stays locked */
        CHECK(convo_locked(), "the lock went away with Alex's chat");
        convo_open(&DEA, &DEA_MIND, 1, NULL);
        CHECK(!convo_active() && dialog_active(), "Dea's chat is no longer locked");
        for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
    }
    lives = 0;
    CHECK(grave_pull_enabled() && grave_pull_index() == aonia_grave, "Alex's no did not switch the grave pull on");

    /* chunk 31: hold at the stone: the plot opens, he slides out feet first (1.5 s), then lies beside it; the task \"Bury Aonia\" coins in */
    {   int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        int tasks0 = missions_count(), bc = brainwin_count();
        pxp = (agx + 0.5f) * tile; pyp = (agy + 0.9f + 1.2f) * tile; run(0.3f);
        CHECK(!grave_is_open(aonia_grave) && !grave_body_out(aonia_grave), "the plot was open before the pull");
        hint_check("grave pull");
        /* chunk 66: the photo on a stone never blinks. the blink clock is the real clock (a blink every ~4 s), so look for 9 s: the stone's portrait must be the
         * same pixels every time, and a normal portrait must change at least once (a blink) */
        {   int qx, qy, qw, qh; grave_portrait_rect(aonia_grave, (int)cam_x, (int)cam_y, &qx, &qy, &qw, &qh);
            unsigned first = 0; int same = 1, differs = 0;
            CHECK(qw > 0 && qx >= 0 && qy >= 0 && qx + qw <= TW && qy + qh <= TH, "the photo is not on the screen for the blink check");
            for (int k = 0; k < 225; k++) {
                frame(0.0f); SDL_RenderPresent(rr);
                unsigned h = region_hash(qx, qy, qw, qh);
                if (k == 0) first = h; else if (h != first) same = 0;
                SDL_Delay(40);
            }
            CHECK(same, "the photo on the stone changed over 9 s (it blinked)");
            unsigned f2 = 0;
            for (int k = 0; k < 225; k++) {
                SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderClear(rr);
                char_draw_portrait(rr, &ALEX, 20, 20, 8, 0); SDL_RenderPresent(rr);
                unsigned h = region_hash(20, 20, 80, 80);
                if (k == 0) f2 = h; else if (h != f2) differs = 1;
                SDL_Delay(40);
            }
            CHECK(differs, "a normal portrait did not blink in 9 s");
        }
        world_touch(0, bx, by); run(GRAVE_PULL_HOLD_T + 0.3f);
        CHECK(aonia_out && grave_is_open(aonia_grave) && grave_body_moving(aonia_grave), "the pull did not open the plot and start the slide");
        CHECK(mission_search >= 0 && mission_bury < 0 && missions_count() == tasks0 + 1, "the pull did not coin in \"Search Aonia\" alone (search %d, bury %d, tasks %d -> %d)", mission_search, mission_bury, tasks0, missions_count());   /* chunk 67: the burial comes after the search */
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
        hint_check("grave search");
        /* chunk 64: the search is a hold. a tap does nothing; a hold released early drains back; a full hold searches (1.5 s) */
        int hn_s = sfx_debug_hints();
        world_touch(0, bx, by); world_touch(1, bx, by); run(0.4f);              /* one tap */
        CHECK(sfx_debug_hints() == hn_s + 1, "a tap on the search button gave no hint, or more than one (%d)", sfx_debug_hints() - hn_s);   /* chunk 65 */
        CHECK(!grave_search_active() && grave_search_progress() == 0 && !aonia_searched, "a tap searched (active %d, prog %.2f)", grave_search_active(), grave_search_progress());
        world_touch(0, bx, by); run(0.6f);                                       /* a hold, let go at 0.6 s */
        CHECK(grave_search_active() && grave_search_progress() > 0.25f && grave_search_progress() < 0.6f, "0.6 s of a 1.5 s hold is at %.2f", grave_search_progress());
        world_touch(1, bx, by); run(0.3f);
        { float pd = grave_search_progress();
          CHECK(!grave_search_active() && pd > 0 && pd < 0.35f, "an early release did not drain the bar (%.2f)", pd); }
        run(1.2f);
        CHECK(grave_search_progress() == 0 && !aonia_searched && !item_exists(), "the drained bar did not reach nothing / the search finished by itself");
        world_touch(0, bx, by); run(0.5f);                                       /* hold again */
        CHECK(grave_search_active(), "a hold did not start the search");
        CHECK(grave_search_progress() > 0.2f && grave_search_progress() < 0.5f, "0.5 s of a 1.5 s search is at %.2f", grave_search_progress());
        CHECK(!item_exists() && !aonia_searched, "the hammer came before the search was over");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/search_button.bmp");   /* the glass sweeping over the button (look at it) */
        int sbc = brainwin_count(), mcs0 = missions_count();
        run(1.2f); world_touch(1, bx, by);
        CHECK(mission_bury >= 0 && missions_count() == mcs0 + 1, "the search did not coin in \"Bury Aonia\" exactly once (tasks %d -> %d)", mcs0, missions_count());   /* chunk 67 (the ticked search is still on the list while it fades) */
        CHECK(aonia_searched && item_exists() && !grave_search_enabled(), "the search did not end with the hammer on the ground");
        CHECK(item_dropping(), "the hammer does not fall (it was placed at once)");
        float hx_, hy_; item_tile(&hx_, &hy_);
        CHECK(fabsf(hx_ - sbx) < 1.5f && hy_ > sby && hy_ < sby + 2.0f, "the hammer is not on the grass next to him (%.2f,%.2f vs %.2f,%.2f)", hx_, hy_, sbx, sby);
        CHECK(!item_has(ITEM_HAMMER), "the hammer was in the pockets while it fell");
        CHECK(brainwin_count() == sbc, "the search still thinks something (%d -> %d)", sbc, brainwin_count());
        pxp = (sbx + 3.0f) * tile; run(0.8f);                                    /* (the fall is over; step off so nothing picks it up) */
        CHECK(!item_dropping() && item_exists(), "the hammer did not land");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/search_hammer.bmp");   /* the hammer on the grass beside him (look at it) */
        CHECK(!near_search, "the magnifying glass shows after the search was done");
        world_touch(0, bx, by); world_touch(1, bx, by); run(1.5f);
        CHECK(brainwin_count() == sbc, "searching again added a thought");

        /* chunk 33 + 68: respect the dead. he is unburied, so the hammer is locked. walking onto it does nothing at all (the hand button shows); pressing the hand
         * does not take it: the task flashes red and shakes, the error buzz sounds, the vessel thinks nothing (chunk 62); every press says so */
        CHECK(item_locked(ITEM_HAMMER), "the hammer is not locked while he lies unburied");
        int e0 = sfx_debug_errors(), b0 = item_blocked_count(), tc0 = brainwin_count(), ta0 = thought_active();
        pxp = hx_ * tile; pyp = hy_ * tile; run(0.3f);
        CHECK(near_item, "the hand button does not show with the hammer in reach");
        CHECK(!item_has(ITEM_HAMMER) && item_exists(), "walking onto the locked hammer took it");
        CHECK(sfx_debug_errors() == e0 && item_blocked_count() == b0 && !mission_flashing(mission_bury), "walking onto the locked hammer buzzed or flashed (%d, %d)", sfx_debug_errors() - e0, item_blocked_count() - b0);
        run(3.5f);
        CHECK(sfx_debug_errors() == e0 && !mission_flashing(mission_bury), "the lock buzzed / flashed by itself while we stood there");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/hand_button.bmp");             /* the hand button next to the interact button (look at it) */
        tap(hbx, hby);
        CHECK(!item_has(ITEM_HAMMER) && item_exists(), "the hand took the locked hammer");
        CHECK(sfx_debug_errors() == e0 + 1 && item_blocked_count() == b0 + 1, "the error buzz / the turned-away press were not counted (%d, %d)", sfx_debug_errors() - e0, item_blocked_count() - b0);
        CHECK(mission_flashing(mission_bury), "the task Bury Aonia does not flash");
        CHECK(brainwin_count() == tc0 && (ta0 || !thought_active()), "pressing the hand at the locked hammer thinks something (%d -> %d thoughts, card %d)", tc0, brainwin_count(), thought_active());
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/respect_flash.bmp");           /* the red, shaking line (look at it) */
        run(0.8f);
        CHECK(!mission_flashing(mission_bury), "the flash did not end after %.1f s", MISSION_FLASH_T);
        tap(hbx, hby);                                                              /* every press answers, there is no waiting time */
        CHECK(sfx_debug_errors() == e0 + 2 && mission_flashing(mission_bury), "no second buzz / flash on the second press (%d)", sfx_debug_errors() - e0);
        CHECK(brainwin_count() == tc0 && !item_has(ITEM_HAMMER), "a thought came after the second press, or the hammer was taken");
        pxp = (sbx + 3.0f) * tile; run(0.3f);
        CHECK(!near_item, "the hand button shows with nothing in reach");

        /* chunk 34: bury him. after the search the button next to the body is a hand lowering him; HOLDING it drags him back into the plot, the task
         * completes and the hammer is free. a short hold only fills the bar a little and it creeps back */
        CHECK(grave_bury_enabled() && grave_bury_index() == aonia_grave, "the search did not switch the burial on");
        int mc0 = missions_count();
        pxp = sbx * tile; pyp = (sby - 1.0f) * tile; run(0.3f);
        CHECK(near_bury && !near_search && !near_grave, "the burial button does not show next to the body after the search");
        hint_check("burying");
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
        pxp = hx_ * tile; pyp = hy_ * tile; run(0.5f);
        CHECK(!item_has(ITEM_HAMMER) && near_item, "walking onto the unlocked hammer took it, or the hand does not show");   /* chunk 68: only the hand takes it */
        tap(hbx, hby);
        CHECK(item_has(ITEM_HAMMER) && !item_exists(), "the hand did not take the unlocked hammer after the burial");
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
        world_touch(0, bx, by); run(0.4f);
        pxp = (sbx + 6.0f) * tile; run(1.5f); world_touch(1, bx, by);            /* walks away mid-search */
        CHECK(searched_n == 0 && !grave_search_active() && grave_search_enabled(), "walking away did not cancel the search (%d)", searched_n);
        pxp = sbx * tile; run(0.3f);
        CHECK(near_search && grave_search_progress() == 0, "the search did not start over from nothing");
        world_touch(0, bx, by); run(GRAVE_SEARCH_T + 0.3f); world_touch(1, bx, by);
        CHECK(searched_n == 1, "a whole search did not call on_done exactly once (%d)", searched_n);
        world_touch(0, bx, by); run(GRAVE_SEARCH_T + 0.5f); world_touch(1, bx, by);
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
        int h0 = sfx_debug_hits(), hn0 = sfx_debug_hints();
        /* chunk 55: a tap is a hint, not a swing: the soft sound once, the little fill, no hit, no flash, nothing else */
        tap(bx, by);
        CHECK(sfx_debug_hints() == hn0 + 1, "a tap on the swing button gave no hint (%d)", sfx_debug_hints() - hn0);
        CHECK(sfx_debug_hits() == h0 && world_fight_left() == 3 && !npc_flashing(nid) && talked_n == 1 && !dialog_active(), "a tap swung or talked (%d hits, %d left)", sfx_debug_hits() - h0, world_fight_left());
        CHECK(hint_t > 0 && hint_curve() >= 0, "the hint's fill is not playing");
        hint_check("hammer swing");
        run(0.05f); frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/swing_hint.bmp");     /* the little fill on the button (look at it) */
        run(HINT_ANIM_T + 0.1f);
        CHECK(hint_t == 0 && fight_prog < 0.05f, "the hint's fill did not drain back (%.2f)", fight_prog);
        /* a long hold let go early: it drains back, no hit, no hint (it was not a tap) */
        world_touch(0, bx, by); run(0.6f);
        CHECK(fight_prog > 0.35f && fight_prog < 0.65f && sfx_debug_hits() == h0, "half a hold: wrong fill (%.2f)", fight_prog);
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/swing_hold.bmp");                  /* the button half full (look at it) */
        {   Uint8 r1, g1, b1, a1, r2, g2, b2, a2; int lx = bcx - br * 55 / 100;                          /* the button fills from the bottom: gold low, dark high */
            SDL_GetRGBA(((Uint32 *)surf->pixels)[(bcy + br / 2) * (surf->pitch / 4) + lx], surf->format, &r1, &g1, &b1, &a1);
            SDL_GetRGBA(((Uint32 *)surf->pixels)[(bcy - br / 2) * (surf->pitch / 4) + lx], surf->format, &r2, &g2, &b2, &a2);
            CHECK(r1 > 180 && b1 < 120, "the button's lower half is not filled while held (%d,%d,%d)", r1, g1, b1);
            CHECK(r2 < 120 && b2 > r2, "the button's upper half is filled at half a hold (%d,%d,%d)", r2, g2, b2); }
        world_touch(1, bx, by); run(HAMMER_DECAY_T + 0.2f);
        CHECK(fight_prog == 0 && sfx_debug_hits() == h0 && world_fight_left() == 3 && sfx_debug_hints() == hn0 + 1, "an early release hit, kept its fill or gave a hint (fill %.2f, hints %d)", fight_prog, sfx_debug_hints() - hn0);
        hold_swing(bx, by);
        CHECK(sfx_debug_hits() == h0 + 1 && world_fight_left() == 2, "the first hold did not hit (%d hits, %d left)", sfx_debug_hits() - h0, world_fight_left());
        CHECK(talked_n == 1 && !dialog_active(), "a swing also talked");
        CHECK(npc_flashing(nid), "the npc did not flash white");
        frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/swing_flash.bmp");       /* her, flashing white and shoved back (look at it) */
        world_touch(0, bx, by); run(0.25f); world_touch(1, bx, by);       /* straight away: the hammer is still coming up, so holding does not even fill */
        CHECK(sfx_debug_hits() == h0 + 1 && world_fight_left() == 2 && fight_prog == 0, "a hold inside the cooldown hit again or filled (%d left, fill %.2f)", world_fight_left(), fight_prog);
        run(0.3f); CHECK(!npc_flashing(nid), "the flash did not end");
        run(FIGHT_COOLDOWN);
        hold_swing(bx, by);
        CHECK(sfx_debug_hits() == h0 + 2 && world_fight_left() == 1 && down_n == 0, "the second hit went wrong (%d left, down %d)", world_fight_left(), down_n);
        run(FIGHT_COOLDOWN + 0.1f);
        hold_swing(bx, by);
        CHECK(world_fight_left() == 0 && down_n == 1, "the third hit did not call on_down once (%d)", down_n);
        run(FIGHT_COOLDOWN + 0.1f);
        hold_swing(bx, by); run(FIGHT_COOLDOWN + 0.1f); tap(bx, by);
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
        tap(bx, by);                                                       /* chunk 55: a tap is only the hint: no hit, no alex_struck, she is still there */
        CHECK(!alex_struck && sfx_debug_hits() == h0 && world_fight_left() == 1 && npc_hostile(alex_id) && !npc_headless(alex_id), "a tap struck Alex (struck %d, hits %d)", alex_struck, sfx_debug_hits() - h0);
        hold_swing(bx, by);
        CHECK(alex_struck && sfx_debug_hits() == h0 + 1 && world_fight_left() == 0, "one full hold did not end it (struck %d, hits %d)", alex_struck, sfx_debug_hits() - h0);
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
            run(DEATH_QUICK_TEXT_AT + 0.3f);                              /* hotfix: no monitor here (the beeps are the heart scene's): the words come at once */
            CHECK(sfx_debug_beeps() == b0 && sfx_debug_flatlines() == f0, "Alex's death played the monitor (%d peeps, %d flatlines)", sfx_debug_beeps() - b0, sfx_debug_flatlines() - f0);
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
            hint_check("body search");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/orb_button.bmp");                   /* the glass on the button (look at it) */
            int tc2 = brainwin_count(), pf = sfx_debug_pfft(); (void)pf;
            tap(bx, by); run(0.5f);                                               /* chunk 64: a tap does nothing, the search is a hold */
            CHECK(!bsearch_active() && bsearch_progress() == 0 && !world_has_item(ITEM_ORB), "a tap searched her (active %d, prog %.2f)", bsearch_active(), bsearch_progress());
            world_touch(0, bx, by); run(0.5f);
            CHECK(bsearch_active() && !world_has_item(ITEM_ORB), "the search did not start with a hold, or finished at once");
            run(1.2f); world_touch(1, bx, by);
            CHECK(!world_has_item(ITEM_ORB) && !orb_taken && item_exists() && orb_dropped, "the search put the orb in the pockets (it must drop on the floor)");
            CHECK(mission_repair < 0, "\"Repair the shrine\" came in before the orb was picked up");
            run(ITEM_DROP_T + 0.4f);                                                 /* it lands */
            {   float ix, iy; item_tile(&ix, &iy); pxp = ix * tile; pyp = (iy + 0.4f) * tile; indoors = 1; run(0.3f);
                CHECK(near_item, "the hand button does not show next to the dropped orb (indoors)");
                tap(hbx, hby); run(0.3f); }
            CHECK(world_has_item(ITEM_ORB) && orb_taken && !item_exists(), "no orb in the pockets after the hand");
            CHECK(mission_repair >= 0, "\"Repair the shrine\" did not come in");
            CHECK(!dialog_active() && brainwin_count() == tc2, "the search opened a dialog / added a thought (it must be silent)");
            CHECK(!bsearch_enabled() && !near_bsearch, "she can be searched twice");
            int n_orb = mission_repair; tap(bx, by); run(0.5f);
            CHECK(mission_repair == n_orb && world_has_item(ITEM_ORB), "a second tap did something");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/orb_taken.bmp");                    /* the slot with the orb (look at it) */
            item_ui_touch(0, TW - 40, 120); item_ui_touch(1, TW - 40, 120); run(0.6f);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/orb_panel.bmp");                    /* the open panel: hammer and orb (look at it) */
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
            {   int hn_ = sfx_debug_hints();                                                  /* chunk 55: a tap on the shrine's button is the hint too */
                tap(bx, by);
                CHECK(sfx_debug_hints() == hn_ + 1 && shrine_polluted() && !shrine_repaired, "a tap on the repair did not give the hint, or mended it (%d)", sfx_debug_hints() - hn_);
                hint_check("shrine repair");
                run(1.0f); CHECK(shrine_progress() < 0.02f, "the tap's progress did not creep back (%.3f)", shrine_progress()); }
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
            /* chunk 41: "Die". the task is there and the hammer button is back at the mended shrine: a full HOLD (chunk 56) swings it at our own head. the same end as
             * Alex's: head off, blood, the body down, then the death scene on us, the stone, limbo, the next life with the orb and nothing else */
            CHECK(mission_die >= 0, "\"Die\" did not come in when the shrine was mended");
            CHECK(shrine_self() && near_shrine, "the hammer button is not back at the mended shrine (%d, %d)", shrine_self(), near_shrine);
            run(1.0f);
            CHECK(!world_player_beheaded(), "it swung by itself");
            int h1 = sfx_debug_hits(), l0 = lives, g0 = grave_n, bp0 = sfx_debug_beeps(), fl0 = sfx_debug_flatlines();
            /* chunk 56: the hold is the aim. a tap = the hint only. while the finger holds: the player stops, faces the camera and the pose is drawn (counted),
             * the bar fills; letting go early puts the pose away, the bar drains and the player walks again. only a full bar swings */
            {   int hn_ = sfx_debug_hints(); float x0 = pxp, y0 = pyp;
                tap(bx, by);
                CHECK(sfx_debug_hints() == hn_ + 1 && !world_player_beheaded() && !self_done && !world_aiming(), "a tap at the mended shrine did not give the hint alone (hints %d)", sfx_debug_hints() - hn_);
                hint_check("Die");
                run(HINT_ANIM_T + 0.1f);
                int a0 = char_debug_aims(); frame(0.016f);
                CHECK(char_debug_aims() == a0, "the aiming pose was drawn without a hold");
                world_touch(0, bx, by); run(1.0f);
                CHECK(world_aiming() && world_self_progress() > 0.4f && world_self_progress() < 0.6f, "half the hold: not aiming or the wrong fill (%d, %.2f)", world_aiming(), world_self_progress());
                a0 = char_debug_aims(); frame(0.016f);
                CHECK(char_debug_aims() == a0 + 1 && facing == FACE_DOWN, "the pose is not drawn once per frame while holding (%d), or the player does not face us", char_debug_aims() - a0);
                CHECK(fabsf(pxp - x0) < 0.01f && fabsf(pyp - y0) < 0.01f && !moving, "the player moved while aiming");
                frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/aim_hold.bmp");                    /* the pose in the world, the button half full (look at it) */
                world_touch(1, bx, by); run(0.1f);
                a0 = char_debug_aims(); frame(0.016f);
                CHECK(!world_aiming() && char_debug_aims() == a0 && !world_player_beheaded() && !self_done, "after an early release the pose is still on, or it swung (%d)", world_aiming());
                run(HAMMER_DECAY_T + 0.2f);
                CHECK(world_self_progress() == 0.0f, "the bar did not drain after the early release (%.2f)", world_self_progress());
                world_touch(0, sx, sy + sr * 3 / 4); run(0.6f); world_touch(1, sx, sy + sr * 3 / 4);          /* the stick works again */
                CHECK(fabsf(pxp - x0) + fabsf(pyp - y0) > 1.0f, "the player cannot walk after an early release");
                pxp = x0; pyp = y0; facing = FACE_DOWN; run(0.4f);
                CHECK(shrine_self() && near_shrine && !world_player_beheaded(), "not back at the shrine for the full hold (%d, %d)", shrine_self(), near_shrine);
            }
            /* chunk 57: the full bar throws the hammer straight up. the controls hide and the player cannot move; the hammer leaves the hand and goes off the top, a
             * beat of sky with the shadow growing, then it comes down on the head (the thump, sfx_hit) and the WHOLE body explodes: no head, no body on the floor, a
             * bigger burst than the beheading's. only then does the story run (the death scene is the same as before) */
            #define COUNT_RGB(rgb, outn) do { frame(0.0f); SDL_RenderPresent(rr); int n_ = 0; Uint32 *p_ = (Uint32 *)surf->pixels; \
                for (int yy_ = surf->h / 5; yy_ < surf->h; yy_++) for (int xx_ = 0; xx_ < surf->w; xx_++) { Uint8 r_, g_, b_, a_; SDL_GetRGBA(p_[yy_ * (surf->pitch / 4) + xx_], surf->format, &r_, &g_, &b_, &a_); \
                    if (r_ == (rgb).r && g_ == (rgb).g && b_ == (rgb).b) n_++; } (outn) = n_; } while (0)
            int hd0 = world_hammer_draws(), shirt0;
            world_touch(0, bx, by); run(SELF_HOLD_T + 0.05f);
            CHECK(world_throwing() && !world_aiming() && sfx_debug_hits() == h1 && !world_player_gone() && !self_done, "the full bar did not start the throw alone (throwing %d, hits %d, gone %d, done %d)", world_throwing(), sfx_debug_hits() - h1, world_player_gone(), self_done);
            CHECK(!controls_visible, "the controls are still shown while the hammer is thrown");
            world_touch(1, bx, by);
            COUNT_RGB(VESSEL2.shirt, shirt0);
            CHECK(shirt0 > 20, "no body to see before the impact (%d shirt pixels)", shirt0);
            {   float x1 = pxp, y1 = pyp;
                world_touch(0, sx, sy + sr * 3 / 4); run(0.2f); world_touch(1, sx, sy + sr * 3 / 4);
                CHECK(fabsf(pxp - x1) + fabsf(pyp - y1) < 0.01f, "the player walked while the hammer was thrown"); }
            hd0 = world_hammer_draws(); frame(0.016f);
            CHECK(world_hammer_draws() == hd0 + 1 && world_throw_time() < THROW_UP_T, "the hammer is not drawn rising (%d, t %.2f)", world_hammer_draws() - hd0, world_throw_time());
            SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/throw_up.bmp");                                      /* the hammer leaving the hand (look at it) */
            while (world_throw_time() < THROW_UP_T + 0.5f) frame(0.016f);
            hd0 = world_hammer_draws(); frame(0.016f);
            CHECK(world_hammer_draws() == hd0 && world_throwing() && sfx_debug_hits() == h1, "the sky beat: the hammer is still in view or it landed early (%d)", world_hammer_draws() - hd0);
            SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/throw_sky.bmp");                                     /* the shadow growing (look at it) */
            while (world_throw_time() < THROW_TOTAL - 0.06f) frame(0.016f);
            hd0 = world_hammer_draws(); frame(0.016f);
            CHECK(world_hammer_draws() == hd0 + 1 && sfx_debug_hits() == h1, "the hammer is not drawn coming down (%d)", world_hammer_draws() - hd0);
            SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/throw_fall.bmp");                                    /* just before the thump (look at it) */
            run(0.15f);
            CHECK(sfx_debug_hits() == h1 + 1 && !world_throwing(), "the thump did not come once at the impact (%d hits)", sfx_debug_hits() - h1);
            CHECK(world_player_beheaded() && world_player_gone() && !world_aiming(), "the body did not explode (beheaded %d, gone %d)", world_player_beheaded(), world_player_gone());
            CHECK(!gore_head_exists() && gore_drops_flying() > 60, "a head, or too few red squares in the air after the burst (head %d, %d squares)", gore_head_exists(), gore_drops_flying());
            CHECK(self_done && dying && !shrine_self(), "the swing's state is wrong (%d, %d, %d)", self_done, dying, shrine_self());
            run(0.3f); frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/self_blood.bmp");        /* the burst (look at it) */
            /* hotfix: the flesh now flies and stays in the body's own colours, so counting those colours no longer tells whether the standing body is still drawn:
             * the body is gone (world_player_gone, checked above) and the chunks are what is left */
            CHECK(gore_flesh_count() + gore_drops_flying() > 20, "no flesh in the air or on the floor after the explosion");
            CHECK(!world_death_active(), "the death scene began before the blood had landed");
            run(1.9f);
            CHECK(gore_flesh_count() >= 20, "the body's flesh did not stay scattered on the floor (%d chunks)", gore_flesh_count());
            CHECK(!gore_busy() && gore_stain_count() >= 70, "the blood did not come to rest (%d stains, want 70+ : more than the beheading's 57)", gore_stain_count());
            CHECK(world_death_active() && world_death_npc() < 0, "the death scene is not on us");
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/self_lying.bmp");                  /* the body, the stains, the head (look at it) */
            run(DEATH_SETTLE_T + 0.4f);
            CHECK(zoom > 2.0f, "the camera did not push in on us (%.2f)", zoom);
            frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/self_death.bmp");                  /* grey room, red body (look at it) */
            run(DEATH_QUICK_TEXT_AT + 0.3f);                              /* hotfix: no monitor here either: the words come at once */
            CHECK(sfx_debug_beeps() == bp0 && sfx_debug_flatlines() == fl0, "the sacrifice played the monitor (%d peeps, %d flatlines)", sfx_debug_beeps() - bp0, sfx_debug_flatlines() - fl0);
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
            CHECK(mission_talk_dea >= 0 && mission_look < 0 && missions_count() == 1, "\"Talk to Goddess\" did not coin in at once after the third summoning (%d tasks)", missions_count());
            dea_watch();
            CHECK(missions_count() == 1, "the sighting coined \"Talk to Goddess\" in a second time on the third life (%d tasks)", missions_count());
            on_talk_dea(dea_id);
            CHECK(!scold_pending && dialog_active() && !convo_active(), "talking to Dea did not start her line");
            CHECK(hud_person() == &VESSEL3 && world_player() == &VESSEL3, "the naming did not flash the ID card / change the look");
            CHECK(strstr(DEA_NAMING3[0].text, "number three") && strstr(DEA_NAMING3[0].text, VESSEL3_NAME), "Dea's line does not name number three");
            CHECK(naming3_done_n == 0, "the naming ended before the line was read");
            for (int i = 0; i < 4 && dialog_active(); i++) { run(2.0f); tap(TW / 2, TH * 4 / 5); }
            CHECK(!dialog_active() && naming3_done_n == 1, "the naming did not end after one line (%d)", naming3_done_n);
            run(4.0f);
            CHECK(missions_count() == 1 && mission_give >= 0, "\"Talk to Goddess\" was not ticked off by the naming, or the next task is wrong (%d tasks left)", missions_count());
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
              convo_lock(""); convo_lock_reply(&DEA, "You heard me. Go."); convo_lock_allow(&ALEX);
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
              convo_lock(""); convo_lock_reply(&DEA, "You heard me. Go."); convo_lock_allow(&ALEX); secret_on = 1; }   /* (the rest of this test goes on as the second life, locked) */
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
    convo_open(&VESSEL3, &ALEX_MIND, 1, locked_end);
    CHECK(!convo_active() && !dialog_active(), "a stranger's chat opened while locked");
    CHECK(strstr(thought_debug_text(), "can't say") == NULL, "the \"can't say that\" thought still exists (chunk 63)");
    CHECK(locked_ended == 1, "on_end of a locked chat did not run (%d)", locked_ended);
    convo_ask_questions(&VESSEL3, &ALEX_MIND, locked_end);
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

    /* chunk 52: the worried face. the same person, normal vs worried: the upper half of each eye is a lid of skin (the glint is gone from
     * the top row), the soul wears it for good, and char_set_worried(0) puts it away. eyes can be blinking, so a normal draw is retried
     * until its eyes are open */
    {   static const Person WP = { "Wp", { 90, 60, 40 }, { 240, 200, 170 }, { 200, 60, 60 }, { 50, 60, 120 }, { 30, 30, 30 }, 0, 0 };
        const int s = 8, ox = 40, oy = 40;                                  /* portrait top-left; the left eye is columns 2-3, rows 4-6 */
        SDL_Rect clr = { 0, 0, 200, 200 };
        #define EYEPX(dx, dy) ({ Uint32 *px_ = (Uint32 *)surf->pixels; px_[(oy + (dy) * s + 1) * (surf->pitch / 4) + ox + (dx) * s + 1]; })
        Uint32 skin = SDL_MapRGBA(surf->format, WP.skin.r, WP.skin.g, WP.skin.b, 255);
        int open_seen = 0;
        for (int k = 0; k < 40 && !open_seen; k++) {
            SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderFillRect(rr, &clr);
            char_set_worried(0); char_draw_portrait(rr, &WP, ox, oy, s, 0);
            if (EYEPX(2, 4) != skin) open_seen = 1; else SDL_Delay(300);
        }
        CHECK(open_seen, "normal eyes never showed open (the test cannot compare)");
        if (open_seen) {
            CHECK(EYEPX(2, 4) != skin && EYEPX(2, 6) != skin, "a normal eye has a skin lid (top %08x)", EYEPX(2, 4));
            SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderFillRect(rr, &clr);
            char_set_worried(1); char_draw_portrait(rr, &WP, ox, oy, s, 0); char_set_worried(0);
            CHECK(EYEPX(2, 4) == skin && EYEPX(6, 4) == skin, "worried: the upper row of the eyes is not covered by the lid");
            CHECK(EYEPX(2, 6) != skin && EYEPX(6, 6) != skin, "worried: the lower row of the eyes is covered, the pupil should show");
            SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderFillRect(rr, &clr);
            char_draw_portrait(rr, &WP, ox, oy, s, 0);
            CHECK(EYEPX(2, 6) != skin, "char_set_worried(0) did not put the worried eyes away");
            if (EYEPX(2, 4) == skin) {}                                      /* (a blink can land here; only the row 6 check is firm) */
        }
        CHECK((SOUL.acc & ACC_WORRIED) && (SOUL.acc & ACC_HALO), "the soul is not always worried (or lost its halo)");
        /* the soul in a face popup: the lid shows without char_set_worried */
        { Uint32 sk = SDL_MapRGBA(surf->format, SOUL.skin.r, SOUL.skin.g, SOUL.skin.b, 255); int lid = 0;
          for (int k = 0; k < 3 && !lid; k++) { SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderFillRect(rr, &clr);
              char_draw_portrait(rr, &SOUL, ox, oy, s, 0); lid = EYEPX(2, 4) == sk; }
          CHECK(lid, "the soul's portrait has no worried lid"); }
        SDL_SetRenderDrawColor(rr, 20, 20, 30, 255); SDL_RenderFillRect(rr, &clr);              /* the screenshot: normal | worried | soul */
        char_draw_portrait(rr, &WP, 10, 20, 8, 0); char_set_worried(1); char_draw_portrait(rr, &WP, 100, 20, 8, 0); char_set_worried(0);
        char_draw_portrait(rr, &SOUL, 190, 20, 8, 0);
        SDL_SaveBMP(surf, "build/char_worried.bmp");
        #undef EYEPX
    }

    /* chunk 56: the aiming pose on its own, large, next to the normal front sprite and a later frame (build/char_aim.bmp) */
    {   SDL_Surface *shot = SDL_CreateRGBSurfaceWithFormat(0, 520, 300, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_Renderer *r2 = SDL_CreateSoftwareRenderer(shot);
        SDL_SetRenderDrawBlendMode(r2, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r2, 126, 188, 246, 255); SDL_RenderClear(r2);
        int a0 = char_debug_aims();
        char_draw(r2, &VESSEL2, 90, 280, FACE_DOWN, 0, 0.0f, 16);
        char_draw_aim(r2, &VESSEL2, 260, 280, 16, 0.0f);
        char_draw_aim(r2, &VESSEL2, 430, 280, 16, 0.2f);                                           /* a moment later: the hammer has swayed */
        CHECK(char_debug_aims() == a0 + 2, "char_debug_aims does not count the draws");
        SDL_SaveBMP(shot, "build/char_aim.bmp");
        SDL_DestroyRenderer(r2); SDL_FreeSurface(shot);
    }

    /* chunk 53: vessel 1 is worried from the moment it is doomed (shrine_done: the wrath, the heart failing, the death scene) until its life ends.
     * Doia (lives == 1) is not part of it, and the next body is calm again */
    {   worried_set(0); lives = 1; shrine_done();
        CHECK(!world_player_worried() && !hud_is_worried(), "a later vessel (lives 1) went worried in the shrine's wrath");
        story_reset_for_respawn();
        lives = 0;
        CHECK(!world_player_worried() && !hud_is_worried(), "worried before vessel 1 is dying");
        shrine_done();
        CHECK(world_player_worried() && hud_is_worried(), "vessel 1 is not worried (body %d, card %d) while dying", world_player_worried(), hud_is_worried());
        run(0.3f);                                                                  /* the body and the card draw worried without trouble */
        CHECK(world_player_worried() && hud_is_worried(), "the worry went away while dying");
        lives = 1; story_reset_for_respawn();
        CHECK(!world_player_worried() && !hud_is_worried(), "the next body is still worried after the respawn");
    }

    /* chunk 54: the stick stays readable on anything. over white, black and grass green: the light ring and the dark outline ring are both
     * there (the light one bright, the dark one dark), and on every background at least one of the two (and one of the knob and its rim)
     * is 3:1 or better away from the background. a screenshot of the three, build/stick_bgs.bmp */
    {   static const Rgb BG[3] = { { 255, 255, 255 }, { 0, 0, 0 }, { 70, 132, 70 } };
        static const char *BGN[3] = { "white", "black", "grass" };
        int span = 2 * (sr + 3 * ucell) + ucell, shot_w = span * 3, shot_h = span;
        SDL_Surface *shot = SDL_CreateRGBSurfaceWithFormat(0, shot_w, shot_h, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_Renderer *r2 = SDL_CreateSoftwareRenderer(shot);
        SDL_SetRenderDrawBlendMode(r2, SDL_BLENDMODE_BLEND);
        #define LUM(c_) ({ float v_[3] = { (c_).r / 255.0f, (c_).g / 255.0f, (c_).b / 255.0f }; for (int i_ = 0; i_ < 3; i_++) v_[i_] = v_[i_] <= 0.03928f ? v_[i_] / 12.92f : powf((v_[i_] + 0.055f) / 1.055f, 2.4f); 0.2126f * v_[0] + 0.7152f * v_[1] + 0.0722f * v_[2]; })
        #define CR(a_, b_) ({ float la_ = LUM(a_), lb_ = LUM(b_); (la_ > lb_ ? la_ + 0.05f : lb_ + 0.05f) / (la_ > lb_ ? lb_ + 0.05f : la_ + 0.05f); })
        for (int b = 0; b < 3; b++) {
            int cx = b * span + span / 2, cy = span / 2;
            SDL_SetRenderDrawColor(r2, BG[b].r, BG[b].g, BG[b].b, 255);
            SDL_Rect all = { b * span, 0, span, span }; SDL_RenderFillRect(r2, &all);
            draw_stick(r2, cx, cy, sr, kr, cx + sr / 2, cy, 1);
            #define PX(x_, y_) ({ Rgb c_ = { 0, 0, 0 }; Uint8 R_, G_, B_, A_; SDL_GetRGBA(((Uint32 *)shot->pixels)[(y_) * (shot->pitch / 4) + (x_)], shot->format, &R_, &G_, &B_, &A_); c_.r = R_; c_.g = G_; c_.b = B_; c_; })
            int ry = cy - ucell / 2;                                                       /* the row just above the middle */
            int mo_d = half_cells((float)(sr + ucell) / ucell, -1), mo_l = half_cells((float)sr / ucell, -1);
            Rgb dark = PX(cx + mo_d * ucell - ucell / 2, ry), light = PX(cx + mo_l * ucell - ucell / 2, ry);
            int kx0 = cx + sr / 2, mk = half_cells((float)(kr + ucell) / ucell, -1), mk0 = half_cells((float)kr / ucell, -1);
            Rgb knob = PX(kx0, ry), rim = PX(kx0 + mk * ucell - ucell / 2, ry);
            CHECK(LUM(light) > 0.5f, "%s: the light ring is not light (lum %.2f)", BGN[b], LUM(light));
            CHECK(LUM(dark) < 0.1f, "%s: the dark outline is not dark (lum %.2f)", BGN[b], LUM(dark));
            CHECK(CR(light, BG[b]) >= 3.0f || CR(dark, BG[b]) >= 3.0f, "%s: neither stick ring is 3:1 away from the background (light %.1f, dark %.1f)", BGN[b], CR(light, BG[b]), CR(dark, BG[b]));
            CHECK(mk > mk0 && (CR(knob, BG[b]) >= 3.0f || CR(rim, BG[b]) >= 3.0f), "%s: neither the knob nor its rim is 3:1 away (knob %.1f, rim %.1f)", BGN[b], CR(knob, BG[b]), CR(rim, BG[b]));
            printf("stick on %s: light ring %.1f:1, dark outline %.1f:1, knob %.1f:1, rim %.1f:1\n", BGN[b], CR(light, BG[b]), CR(dark, BG[b]), CR(knob, BG[b]), CR(rim, BG[b]));
            #undef PX
        }
        SDL_SaveBMP(shot, "build/stick_bgs.bmp");
        #undef LUM
        #undef CR
        SDL_DestroyRenderer(r2); SDL_FreeSurface(shot);
    }

    /* chunk 58: finding things. five things on the grass (Alex's house, the shrine, a hammer, two graves) and the temple in the clouds are not found at the start;
     * getting within sight finds each one: its flag, one discovery chime (sfx_found, never the coin) and one toast; not again when we leave and come back; the
     * flags survive a new map (a landing after a death) and are cleared only by a new game (world_found_reset) */
    {   load_map(MAP_GREEN); phase = PH_PLAY; ph_t = 0; death_on = 0; player_beheaded = 0; throw_t = -1; indoors = 0; door_t = -1;
        dialog_init(TW, TH);                                             /* (a talk from the chain above is still open: start clean) */
        CHECK(!dialog_active(), "the found test could not clear the open dialog");
        world_found_reset(); toast_init(TW, TH); frame(0.016f);
        int cx = mw / 2, cy = mh / 2, hx = 0, hy = 0, sx_ = 0, sy_ = 0, kx_ = 0, ky_ = 0, g1x = 0, g1y = 0, g2x = 0, g2y = 0;
        int okh = world_find_house_spot(10, 60, &hx, &hy) && world_place_house(hx, hy, "Alex");
        int far_ = 14;                                                  /* every thing is at least this far from the others */
        #define SPOT(ox, oy, ax, ay) (world_find_prop_spot_near(cx + (ox), cy + (oy), &ax, &ay))
        int oks = SPOT(-24, 0, sx_, sy_), okk = SPOT(24, 0, kx_, ky_), okg1 = SPOT(0, 24, g1x, g1y), okg2 = SPOT(0, -24, g2x, g2y);
        #undef SPOT
        CHECK(okh && oks && okk && okg1 && okg2, "the found test could not place its things (%d %d %d %d %d)", okh, oks, okk, okg1, okg2);
        world_restore_shrine(sx_, sy_, 0); shrine_enable(NULL); world_place_item(ITEM_HAMMER, kx_, ky_);
        CHECK(shrine_enabled(), "the found test could not switch the shrine on (the map marks only a usable shrine)");
        int gi1 = world_place_grave(g1x, g1y, &VESSEL), gi2 = world_place_grave(g2x, g2y, &VESSEL2);
        CHECK(gi1 >= 0 && gi2 >= 0 && house_on && shrine_exists() && item_exists(), "the found test's things are not on the map");
        float hxf = house_sx + 0.5f, hyf = house_sy - HUT_H / 2.0f;
        float tx_[5] = { hxf, sx_ + 0.5f, kx_ + 0.5f, g1x + 0.5f, g2x + 0.5f }, ty_[5] = { hyf, sy_ + 0.5f, ky_ + 0.5f, g1y + 0.5f, g2y + 0.5f };
        for (int a = 0; a < 5; a++) for (int b = a + 1; b < 5; b++)
            CHECK(hypotf(tx_[a] - tx_[b], ty_[a] - ty_[b]) > far_ - 3.0f, "found test setup: things %d and %d are only %.1f tiles apart", a, b, hypotf(tx_[a] - tx_[b], ty_[a] - ty_[b]));
        #define GO(x_, y_) do { pxp = ((x_) + 0.5f) * tile; pyp = ((y_) + 0.5f) * tile; } while (0)
        int f0 = sfx_debug_found(), c0 = sfx_debug_coins();
        GO(cx, cy); run(0.4f);
        CHECK(world_found_count() == 0 && sfx_debug_found() == f0, "something was found before we got close (%d)", world_found_count());
        CHECK(!world_found_house() && !world_found_shrine() && !world_found_hammer() && !world_found_grave(&VESSEL) && !world_found_grave(&VESSEL2), "a found flag is set at the start");
        /* chunk 59: the map (minimap and map window both read gather_marks) shows a house, shrine, hammer or grave only once it is found */
        #define MKIND(k_) ({ MiniMark mm_[MAX_MARKS]; int n_ = gather_marks(mm_), c_ = 0; for (int q_ = 0; q_ < n_; q_++) if (mm_[q_].kind == (k_)) c_++; c_; })
        CHECK(MKIND(5) == 0 && MKIND(2) == 0 && MKIND(4) == 0 && MKIND(3) == 0, "the map shows something nobody has found (house %d, shrine %d, hammer %d, graves %d)", MKIND(5), MKIND(2), MKIND(4), MKIND(3));
        struct { const char *what; float x, y; } th[5] = { { "the house", tx_[0], ty_[0] }, { "the shrine", tx_[1], ty_[1] }, { "the hammer", tx_[2], ty_[2] }, { "Aonia's grave", tx_[3], ty_[3] }, { "Doia's grave", tx_[4], ty_[4] } };
        for (int i = 0; i < 5; i++) {
            pxp = (th[i].x + 3.0f) * tile; pyp = th[i].y * tile; run(0.3f);
            CHECK(world_found_count() == i + 1 && sfx_debug_found() == f0 + i + 1, "%s: found count %d (want %d), chimes %d (want %d)", th[i].what, world_found_count(), i + 1, sfx_debug_found() - f0, i + 1);
            {   int want_h = i >= 0, want_s = i >= 1, want_k = i >= 2, want_g = i >= 3 ? i - 2 : 0;       /* what the map should show by now */
                CHECK(MKIND(5) == want_h && MKIND(2) == want_s && MKIND(4) == want_k && MKIND(3) == want_g, "%s: map markers house %d/%d shrine %d/%d hammer %d/%d graves %d/%d", th[i].what, MKIND(5), want_h, MKIND(2), want_s, MKIND(4), want_k, MKIND(3), want_g); }
            if (i == 0) { run(0.6f); frame(0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/found_toast.bmp"); }     /* the toast (look at it) */
            run(5.0f);                                                  /* the toast goes away before the next one */
        }
        CHECK(world_found_house() && world_found_shrine() && world_found_hammer() && world_found_grave(&VESSEL) && world_found_grave(&VESSEL2), "a found flag is missing");
        CHECK(sfx_debug_coins() == c0, "the coin ding played for a discovery (%d)", sfx_debug_coins() - c0);
        for (int i = 0; i < 5; i++) { GO(cx, cy); run(0.2f); pxp = (th[i].x + 3.0f) * tile; pyp = th[i].y * tile; run(0.3f); }     /* away and back */
        CHECK(world_found_count() == 5 && sfx_debug_found() == f0 + 5, "something was found a second time (%d, %d chimes)", world_found_count(), sfx_debug_found() - f0 - 5);
        /* chunk 60: pictures instead of dots. all five things are found and on the map: each is a marker of its own kind (a picture, no text: MiniMark has no label any
         * more), the shrine's marker says whole or broken, and screenshots of the minimap (by the shrine) and of the map window (everything) are saved */
        GO(cx, cy); run(0.2f);
        CHECK(MKIND(5) == 1 && MKIND(2) == 1 && MKIND(4) == 1 && MKIND(3) == 2 && MKIND(7) == 0, "all found: markers house %d shrine %d hammer %d graves %d temple %d", MKIND(5), MKIND(2), MKIND(4), MKIND(3), MKIND(7));
        {   MiniMark mm[MAX_MARKS]; int n = gather_marks(mm), whole = -1;
            for (int q = 0; q < n; q++) if (mm[q].kind == 2) whole = mm[q].state;
            CHECK(whole == 0, "a whole shrine is marked broken (state %d)", whole); }
        int cv_ = controls_visible; world_set_controls_visible(1);       /* the minimap shows and the map window opens only with the controls on */
        GO(sx_ + 3, sy_); run(0.3f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/map_icons_mini.bmp");
        mapwin_open(); CHECK(mapwin_active(), "the map window did not open"); mapwin_update(0.016f); frame(0.0f); mapwin_draw(rr); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/map_icons_window.bmp"); mapwin_close();
        CHECK(mapwin_active() == 0, "the map window did not close again");
        world_restore_shrine(sx_, sy_, 1); run(0.1f);                   /* the shrine as an earlier life left it: broken */
        {   MiniMark mm[MAX_MARKS]; int n = gather_marks(mm), broken = -1;
            for (int q = 0; q < n; q++) if (mm[q].kind == 2) broken = mm[q].state;
            CHECK(broken == 1, "a broken shrine is not marked broken (state %d)", broken); }
        world_restore_shrine(sx_, sy_, 0);
        world_set_controls_visible(cv_);
        load_map(MAP_GREEN); phase = PH_PLAY; run(0.2f);                 /* a new map: a landing after a death. the things are not there, but what was found stays found */
        CHECK(world_found_count() == 5 && world_found_house() && world_found_shrine() && world_found_hammer() && world_found_grave(&VESSEL2), "the found flags did not survive a new map");
        world_restore_shrine(sx_, sy_, 0); GO(sx_, sy_); run(0.4f);
        CHECK(world_found_count() == 5 && sfx_debug_found() == f0 + 5, "the shrine was found again after the map was made again");
        /* the temple in the clouds */
        load_map(MAP_CLOUD); phase = PH_PLAY; ph_t = 0; indoors = 0; GO(CLOUD_CX, 50); run(0.3f);
        CHECK(!world_found_temple() && world_found_count() == 5, "the temple was found from afar");
        int ttx, tty; world_temple_spot(&ttx, &tty); pxp = (ttx + 0.5f + 3.0f) * tile; pyp = (tty + 0.5f) * tile; run(0.3f);
        CHECK(world_found_temple() && world_found_count() == 6 && sfx_debug_found() == f0 + 6, "the temple was not found when we came close (%d, %d chimes)", world_found_count(), sfx_debug_found() - f0 - 5);
        CHECK(MKIND(7) == 1 && MKIND(5) == 0 && MKIND(2) == 0 && MKIND(3) == 0, "the temple marker: temple %d (want 1), others %d %d %d", MKIND(7), MKIND(5), MKIND(2), MKIND(3));
        {   int cv2_ = controls_visible; world_set_controls_visible(1);
            mapwin_open(); CHECK(mapwin_active(), "the map window did not open"); mapwin_update(0.016f); frame(0.0f); mapwin_draw(rr); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/map_icons_temple.bmp"); mapwin_close(); world_set_controls_visible(cv2_); }
        run(5.0f); GO(CLOUD_CX, 50); run(0.2f); pxp = (ttx + 0.5f + 3.0f) * tile; pyp = (tty + 0.5f) * tile; run(0.3f);
        CHECK(world_found_count() == 6 && sfx_debug_found() == f0 + 6, "the temple was found twice");
        world_found_reset();                                            /* a new game */
        CHECK(world_found_count() == 0 && !world_found_temple() && !world_found_house() && !world_found_grave(&VESSEL), "a new game did not clear the found flags");
        #undef GO
    }

    /* chunk 62: the orb thought after the talk, word for word: the window has "Alex has the orb. I need to break the shrine." and the card says only
     * "I need to break the shrine.". the search and the locked hammer think nothing any more (checked
     * above, where they used to) */
    {   int bc = brainwin_count();
        alex_chat_end();
        CHECK(brainwin_count() == bc + 1, "the orb thought did not join the window (%d -> %d)", bc, brainwin_count());
        CHECK(strcmp(brainwin_debug_text(bc), "Alex has the orb. I need to break the shrine.") == 0, "the orb thought says \"%s\"", brainwin_debug_text(bc));
        CHECK(strcmp(thought_debug_text(), "I need to break the shrine.") == 0, "the orb card says \"%s\"", thought_debug_text());
    }

    /* chunk 60: every kind of map icon is its own picture (not empty, no two alike), and the broken shrine is grey where the whole one has its violet gem */
    {   enum { IC = 3, SZ = MAPICON_CELLS * IC };
        SDL_Surface *is = SDL_CreateRGBSurfaceWithFormat(0, SZ, SZ, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_Renderer *ir = SDL_CreateSoftwareRenderer(is);
        SDL_SetRenderDrawBlendMode(ir, SDL_BLENDMODE_BLEND);
        static Uint32 px[MAPICON_COUNT][SZ * SZ]; int lit[MAPICON_COUNT], violet[MAPICON_COUNT];
        for (int k = 0; k < MAPICON_COUNT; k++) {
            SDL_SetRenderDrawColor(ir, 0, 0, 0, 0); SDL_RenderClear(ir);
            mapicon_draw(ir, k, SZ / 2, SZ / 2, IC);
            memcpy(px[k], is->pixels, sizeof px[k]);
            lit[k] = violet[k] = 0;
            for (int i = 0; i < SZ * SZ; i++) {
                Uint8 R, G, B, A; SDL_GetRGBA(px[k][i], is->format, &R, &G, &B, &A);
                if (A) lit[k]++;
                if (A && B > 200 && R > 150 && G < 140) violet[k]++;
            }
            CHECK(lit[k] > 40, "map icon %d is nearly empty (%d pixels)", k, lit[k]);
        }
        for (int a = 0; a < MAPICON_COUNT; a++) for (int b = a + 1; b < MAPICON_COUNT; b++)
            CHECK(memcmp(px[a], px[b], sizeof px[a]) != 0, "map icons %d and %d are the same picture", a, b);
        CHECK(violet[MAPICON_SHRINE] > 0 && violet[MAPICON_SHRINE_BROKEN] == 0, "the whole shrine has %d violet pixels, the broken one %d (want >0 and 0)", violet[MAPICON_SHRINE], violet[MAPICON_SHRINE_BROKEN]);
        SDL_Surface *sheet = SDL_CreateRGBSurfaceWithFormat(0, SZ * MAPICON_COUNT + 8 * (MAPICON_COUNT + 1), SZ + 16, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_Renderer *sr_ = SDL_CreateSoftwareRenderer(sheet);
        SDL_SetRenderDrawColor(sr_, 76, 162, 78, 255); SDL_RenderClear(sr_);
        for (int k = 0; k < MAPICON_COUNT; k++) mapicon_draw(sr_, k, 8 + k * (SZ + 8) + SZ / 2, 8 + SZ / 2, IC);
        SDL_RenderPresent(sr_); SDL_SaveBMP(sheet, "build/map_icons.bmp");                     /* every picture on grass (look at it) */
        SDL_DestroyRenderer(sr_); SDL_FreeSurface(sheet); SDL_DestroyRenderer(ir); SDL_FreeSurface(is);
    }

    /* chunk 67: a new landing puts back the task that fits the state: he is out and not searched -> "Search Aonia"; searched -> "Bury Aonia"; buried -> neither;
     * one each, none doubled (the task count is compared with a landing where nothing was pulled) */
    {   int base = 0;
        for (int st = 0; st < 4; st++) {                     /* 0: nothing pulled, 1: out, 2: searched, 3: buried */
            lives = 1; story_reset_for_respawn(); lives = 1;
            load_map(MAP_GREEN); phase = PH_PLAY;
            grave_n = 1; graves[0].tx = 20; graves[0].ty = 20; graves[0].who = &VESSEL;
            aonia_out = st == 1 || st == 2; aonia_searched = st == 2; aonia_buried = st == 3; hammer_taken = 0;
            landed_again();
            int n = missions_count();
            if (st == 0) { base = n; CHECK(mission_search < 0 && mission_bury < 0, "a landing before the pull has a search / bury task"); }
            if (st == 1) CHECK(mission_search >= 0 && mission_bury < 0 && n == base + 1, "out, not searched: want exactly \"Search Aonia\" (search %d, bury %d, %d tasks vs %d)", mission_search, mission_bury, n, base);
            if (st == 2) CHECK(mission_search < 0 && mission_bury >= 0 && n == base + 1, "searched: want exactly \"Bury Aonia\" (search %d, bury %d, %d tasks vs %d)", mission_search, mission_bury, n, base);
            if (st == 3) CHECK(mission_search < 0 && mission_bury < 0 && n == base, "buried: want neither task (search %d, bury %d, %d tasks vs %d)", mission_search, mission_bury, n, base);
        }
        aonia_out = aonia_searched = aonia_buried = 0; grave_n = 0; lives = 0;
    }

    /* chunk 69: an item lying on the ground is half the size. the old one lay on its side at 12 x 12 cells: its box (w, h) was the big icon's (h, w) turned; now
     * both sides are half of that (+- one cell: the picture is cut into 2 x 2 blocks). the shadow is half too (black on black here, it is in the screenshot) */
    {   int ow, oh, gw, gh;
        item_reset(px, tile); item_place(ITEM_HAMMER, 5, 5);
        SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderClear(rr);
        item_draw_icon(rr, ITEM_HAMMER, 200, 200, px); lit_box(100, 100, 340, 340, &ow, &oh);        /* the big picture, upright */
        SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderClear(rr);
        item_draw_ground(rr, (int)(5.5f * tile) - 200, (int)(5.5f * tile) - 200, 0.0f); lit_box(100, 100, 340, 340, &gw, &gh);
        CHECK(ow > 0 && gw > 0, "could not measure the hammers (%d, %d)", ow, gw);
        CHECK(abs(gw - oh / 2) <= px && abs(gh - ow / 2) <= px, "the lying hammer is %d x %d px, half of the old %d x %d (turned) is %d x %d", gw, gh, oh, ow, oh / 2, ow / 2);
        CHECK(gw * 2 <= oh + 2 * px && gh * 2 <= ow + 2 * px && gw * 2 + 2 * px >= oh && gh * 2 + 2 * px >= ow, "the lying hammer is not half the size");
        SDL_SetRenderDrawColor(rr, 76, 162, 78, 255); SDL_RenderClear(rr);
        item_draw_ground(rr, (int)(5.5f * tile) - 200, (int)(5.5f * tile) - 200, 0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/item_ground.bmp");   /* on grass (look at it) */
        CHECK(item_in_reach((5.5f + 1.0f) * tile, 5.5f * tile) && !item_in_reach((5.5f + 1.5f) * tile, 5.5f * tile), "the reach changed");
        item_reset(px, tile);
    }

    /* chunk 70: a pretty inventory. the pockets are driven straight through item.c (no world), drawn on grass. closed by default and invisible while empty; the
     * bag shows once something is owned; a tap opens the panel with the ease (0.25 s), a tap on the bag, a tap outside or 4 s without a touch close it; a touch
     * on the panel keeps it open and a slot names itself; a pick-up flies to the bag, the badge counts it on arrival and the new slot flashes */
    {   item_clear(); item_reset(px, tile);
        int bx = item_ui_bag_x(), by = item_ui_bag_y();
        #define GRASS() do { SDL_SetRenderDrawColor(rr, 76, 162, 78, 255); SDL_RenderClear(rr); } while (0)
        GRASS(); unsigned h0 = region_hash(bx - 60, by - 40, 120, 400); item_ui_draw(rr, 0.0f);
        CHECK(region_hash(bx - 60, by - 40, 120, 400) == h0 && !item_ui_hit(bx, by) && !item_ui_open(), "the inventory shows (or hits) while the pockets are empty");
        CHECK(item_ui_touch(0, bx, by) == 0 && item_ui_touch(1, bx, by) == 0, "an empty inventory ate a touch");
        item_give(ITEM_HAMMER);
        CHECK(item_ui_hit(bx, by) && !item_ui_open() && item_ui_progress() == 0.0f && item_ui_shown() == 1, "closed bag: hit %d open %d p %.2f badge %d", item_ui_hit(bx, by), item_ui_open(), item_ui_progress(), item_ui_shown());
        GRASS(); item_ui_draw(rr, 0.0f);
        CHECK(region_hash(bx - 60, by - 40, 120, 80) != h0, "the bag is not drawn once the hammer is in the pockets");
        SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/inv_closed.bmp");                         /* closed, one item: the bag and the badge (look at it) */
        /* the ease */
        CHECK(item_ui_touch(0, bx, by) == 1 && item_ui_touch(1, bx, by) == 1 && item_ui_open() && item_ui_progress() == 0.0f, "a tap on the bag did not open it (open %d)", item_ui_open());
        item_update(0.10f);
        CHECK(item_ui_progress() > 0.1f && item_ui_progress() < 0.9f, "the panel does not ease out (p %.2f after 0.10 s)", item_ui_progress());
        item_update(0.20f);
        CHECK(item_ui_progress() == 1.0f && item_ui_hit(item_ui_slot_x(ITEM_HAMMER), item_ui_slot_y(ITEM_HAMMER)), "the panel is not fully out after 0.30 s (p %.2f)", item_ui_progress());
        item_give(ITEM_ORB); item_update(0.0f);
        int sxh = item_ui_slot_x(ITEM_HAMMER), syh = item_ui_slot_y(ITEM_HAMMER), sxo = item_ui_slot_x(ITEM_ORB), syo = item_ui_slot_y(ITEM_ORB);
        CHECK(sxh > 0 && sxo > sxh && syh == syo, "the two slots are not in one row (%d,%d / %d,%d)", sxh, syh, sxo, syo);
        GRASS(); item_ui_draw(rr, 0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/inv_open.bmp");     /* open, hammer and orb (look at it) */
        CHECK(item_ui_selected() == ITEM_HAMMER || item_ui_selected() == ITEM_ORB, "no name shows when it opens");
        /* a touch on a slot: it stays open, the touch is the inventory's, and that slot's name shows */
        CHECK(item_ui_touch(0, sxh, syh) == 1 && item_ui_touch(1, sxh, syh) == 1 && item_ui_open() && item_ui_selected() == ITEM_HAMMER, "a tap on the hammer's slot did not name it (selected %d)", item_ui_selected());
        item_ui_touch(0, sxo, syo); item_ui_touch(1, sxo, syo);
        CHECK(item_ui_open() && item_ui_selected() == ITEM_ORB, "a tap on the orb's slot did not name it");
        /* it folds by itself, and a touch on the panel keeps it open */
        inv_run(3.0f); item_ui_touch(0, sxh, syh); item_ui_touch(1, sxh, syh); inv_run(3.0f);
        CHECK(item_ui_open(), "it closed although it was touched 3 s ago");
        inv_run(1.1f);
        CHECK(!item_ui_open(), "it did not close by itself after 4 s");
        CHECK(item_ui_progress() > 0.0f && item_ui_progress() < 1.0f, "it does not ease back in (p %.2f)", item_ui_progress());
        item_update(0.3f); CHECK(item_ui_progress() == 0.0f && !item_ui_hit(sxh, syh), "the panel is still there after it closed");
        /* a tap outside closes it and goes on to the world; a tap on the bag closes it too */
        item_ui_touch(0, bx, by); item_ui_touch(1, bx, by); item_update(0.4f);
        CHECK(item_ui_open() && item_ui_touch(0, 5, TH - 5) == 0 && !item_ui_open(), "a tap outside did not close it (or the inventory ate it)");
        item_update(0.4f); item_ui_touch(1, 5, TH - 5);
        item_ui_touch(0, bx, by); item_ui_touch(1, bx, by); item_update(0.4f);
        item_ui_touch(0, bx, by); item_ui_touch(1, bx, by);
        CHECK(!item_ui_open(), "a second tap on the bag did not close it");
        item_update(0.4f);
        /* a pick-up: the icon flies from the ground to the bag, THEN the badge counts it and the slot flashes */
        item_clear(); item_reset(px, tile); item_give(ITEM_ORB); item_update(0.0f);
        item_place(ITEM_HAMMER, 5, 5); GRASS(); item_draw_ground(rr, (int)(5.5f * tile) - 200, (int)(5.5f * tile) - 200, 0.0f);
        CHECK(item_grab(5.5f * tile, 5.5f * tile) == ITEM_HAMMER && item_ui_flying() && item_ui_shown() == 1, "the pick-up does not fly (flying %d, badge %d)", item_ui_flying(), item_ui_shown());
        GRASS(); item_ui_draw(rr, 0.0f); item_update(0.20f);
        GRASS(); unsigned hb = region_hash(bx - 60, by - 40, 120, 80); item_ui_draw(rr, 0.0f); SDL_RenderPresent(rr); SDL_SaveBMP(surf, "build/inv_fly.bmp");   /* the icon on its way (look at it) */
        CHECK(item_ui_flying() && item_ui_shown() == 1, "it landed too early");
        item_update(0.25f);
        CHECK(!item_ui_flying() && item_ui_shown() == 2, "after the flight the badge does not say 2 (flying %d, badge %d)", item_ui_flying(), item_ui_shown());
        (void)hb;
        /* the new slot flashes: the same slot looks different a moment apart (blink), and not any more after 1.5 s */
        item_ui_touch(0, bx, by); item_ui_touch(1, bx, by); item_update(0.4f); item_give(ITEM_HAMMER);        /* (the hammer is new again) */
        int fx = item_ui_slot_x(ITEM_HAMMER), fy = item_ui_slot_y(ITEM_HAMMER);
        GRASS(); item_ui_draw(rr, 0.0f); unsigned f0 = region_hash(fx - 30, fy - 30, 60, 60);
        item_update(0.16f); GRASS(); item_ui_draw(rr, 0.0f); unsigned f1 = region_hash(fx - 30, fy - 30, 60, 60);
        CHECK(f0 != f1, "the new slot does not flash");
        item_update(1.6f); GRASS(); item_ui_draw(rr, 0.0f); unsigned f2 = region_hash(fx - 30, fy - 30, 60, 60);
        item_update(0.16f); GRASS(); item_ui_draw(rr, 0.0f); unsigned f3 = region_hash(fx - 30, fy - 30, 60, 60);
        CHECK(f2 == f3, "the slot still flashes after 1.5 s");
        item_clear(); item_reset(px, tile);
        CHECK(!item_ui_open() && item_ui_progress() == 0.0f && !item_ui_hit(bx, by), "a new body's pockets leave the inventory open or visible");
        #undef GRASS
    }

    printf(fails ? "orderstest: %d FAILED\n" : "orderstest: ok\n", fails);
    return fails ? 1 : 0;
}
