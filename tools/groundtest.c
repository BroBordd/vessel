/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the ground part of vessel 1, end to end on a PC: fall to the Grasslands, land, the complaint, the
 * first thought, Alex far away, walking into sight of her (thought + "Ask Alex" task), talking to her,
 * the deal, and then (a second run, skipping the talk) Dia's shrine: inert until the deal, a hold on the
 * interact button pollutes it, letting go early does not.
 * it #includes world.c and story.c so it can reach their statics (no test hooks in the shipped game).
 * writes build/ground_*.bmp, exits non-zero if a check fails:
 *   (built with -DGFX_REDIRECT -include game/gfx.h like the game: the death cutscene's colour filter lives in gfx.c)
 *   cc -O1 -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -o build/groundtest tools/groundtest.c $(ls game/*.c | grep -v -e game/game.c -e game/world.c -e game/story.c) $(sdl2-config --libs) -lm
 *   build/groundtest */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include "world.c"
#include "story.c"
#include "lang.h"
#include "pausebtn.h"

#define TW 540
#define TH 1170

static SDL_Surface *surf; static SDL_Renderer *rr;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

/* the same order game.c uses: world, then the top buttons on top of it (music, brain, pause) */
static void frame(float dt) {
    thought_set_enabled(1);
    world_update(dt); pausebtn_update(dt); nowplaying_update(dt); thought_update(dt);
    world_draw(rr); nowplaying_draw(rr); thought_draw(rr); pausebtn_draw(rr);
}
static void run(float sec) { for (float s = 0; s < sec; s += 0.016f) frame(0.016f); }
static void tap(int x, int y) { world_touch(0, x, y); frame(0.016f); world_touch(1, x, y); frame(0.016f); }
static void shot(const char *name) { SDL_RenderPresent(rr); SDL_SaveBMP(surf, name); printf("wrote %s\n", name); }
static void put_player(float tx, float ty) { pxp = tx * tile; pyp = ty * tile; }   /* teleport (world.c's statics) */

int limbo_done_calls;
static void limbo_test_done(void) { limbo_done_calls++; }

int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, TW, TH, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    lang_seed(7);
    nowplaying_init(TW, TH); pausebtn_init(TW, TH); pausebtn_set_enabled(1);   /* game.c does this before the world starts */
    brainwin_init(TW, TH);
    world_init(TW, TH);

    /* chunks 8-9: the summoning runs first (chunk 10): light drops, ring spreads, sparks rise, the vessel forms
     * (gold silhouette from the feet up, then its colours), the light ends, the controls stay hidden until on_done says so */
    {   int sxp = TW / 2, syp = TH / 2;                          /* the player's feet are in the middle of the screen (not at a map edge) */
        int s0 = sfx_debug_summons();
        CHECK(world_summoning() && summon_active(), "the summoning did not start with the game");
        CHECK(!controls_visible, "the controls are visible during the summoning");
        run(0.25f);
        CHECK(sfx_debug_summons() == s0 + 1, "the chime did not start with the light (%d calls)", sfx_debug_summons() - s0);
        CHECK(summon_beam_rows() > 0 && summon_beam_rows() < syp / px, "the light is not falling (rows %d of %d)", summon_beam_rows(), syp / px);
        CHECK(summon_ring_radius() == 0, "the ring is spreading before the light has landed");
        CHECK(summon_player_rows() == 0 && summon_spark_count() == 0, "the vessel or the sparks are there before the light has landed");
        run(0.9f);                                               /* 1.15 s: landed, the ring is spreading, the sparks have begun */
        CHECK(summon_beam_rows() == syp / px, "the light has not reached the floor (rows %d, want %d)", summon_beam_rows(), syp / px);
        CHECK(summon_ring_radius() > 0, "no ring on the floor");
        CHECK(summon_spark_count() > 3, "no sparks rising (%d)", summon_spark_count());
        CHECK(summon_player_rows() == 0, "the vessel is there before its time (rows %d)", summon_player_rows());
        {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, red = 0;
            for (int y = syp - 10 * px; y < syp; y += 2) for (int x = sxp - 4 * px; x < sxp + 4 * px; x += 2) { Uint8 *c = pix + y * pitch + x * 4; if (c[0] > 180 && c[1] < 90 && c[2] < 90) red++; }
            CHECK(red == 0, "the player is drawn before the vessel forms (%d red shirt samples)", red); }
        run(0.35f);                                              /* 1.5 s: the silhouette is just starting */
        shot("build/summon_hold.bmp");
        {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, gold = 0;
            for (int y = 20; y < syp - 14 * px; y += 4) { Uint8 *c = pix + y * pitch + sxp * 4; if (c[0] > 240 && c[1] > 215 && c[2] > 150) gold++; }
            CHECK(gold > 30, "no column of golden light over the spot (%d bright samples)", gold);
            int ring = 0;                                        /* the ring: gold-ish pixels on the floor left and right of the light */
            for (int dx = -9 * px; dx <= 9 * px; dx += px) { Uint8 *c = pix + (syp + 2) * pitch + (sxp + dx) * 4; if (c[0] > 200 && c[2] < 215 && abs(dx) > 3 * px) ring++; }      /* gold, not the white of the clouds */
            CHECK(ring >= 4, "no glowing ring on the floor (%d samples)", ring); }
        run(0.25f);                                              /* 1.75 s: the vessel is growing up out of the light */
        {   int rows = summon_player_rows();
            CHECK(rows > 0 && rows < SUMMON_ROWS, "the vessel is not forming row by row (rows %d)", rows);
            CHECK(summon_player_gold() == 256, "the silhouette is not flat gold while it grows (%d)", summon_player_gold()); }
        shot("build/summon_form.bmp");
        {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, red = 0, golds = 0;
            for (int y = syp - 12 * px; y < syp; y += 2) for (int x = sxp - 4 * px; x < sxp + 4 * px; x += 2) { Uint8 *c = pix + y * pitch + x * 4; if (c[0] > 180 && c[1] < 90 && c[2] < 90) red++; if (c[0] > 200 && c[1] > 130 && c[1] < 215 && c[2] < 110) golds++; }
            CHECK(red == 0, "the silhouette has real colours already (%d red samples)", red);
            CHECK(golds > 8, "no gold silhouette on the screen (%d samples)", golds); }
        run(0.4f);                                               /* 2.15 s: fully formed, the colours are coming */
        CHECK(summon_player_rows() == SUMMON_ROWS, "the vessel is not whole (rows %d)", summon_player_rows());
        {   int g = summon_player_gold(); CHECK(g > 0 && g <= 256, "the gold does not start to leave (%d)", g); }
        run(0.95f);                                              /* 3.1 s: real colours, the light still stands */
        CHECK(summon_player_gold() == 0 && world_summoning(), "the vessel did not take its real colours (gold %d)", summon_player_gold());
        shot("build/summon_colours.bmp");
        {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, red = 0;
            for (int y = syp - 12 * px; y < syp; y += 2) for (int x = sxp - 4 * px; x < sxp + 4 * px; x += 2) { Uint8 *c = pix + y * pitch + x * 4; if (c[0] > 180 && c[1] < 90 && c[2] < 90) red++; }
            CHECK(red > 8, "the vessel's red shirt is not on the screen at the end (%d samples)", red); }
        CHECK(summon_time() > SUMMON_DROP_T + SUMMON_HOLD_T, "the end of the light has not begun (t %.2f)", summon_time());
        run(SUMMON_TOTAL_T - 3.1f - 0.1f);
        CHECK(world_summoning(), "the summoning ended too early");
        run(0.3f);
        CHECK(!world_summoning() && !summon_active(), "the summoning did not end");
        CHECK(controls_visible, "intro_done did not give the controls back when the light ended");
        CHECK(!dialog_active(), "a welcome window followed the summoning (chunk 10 removed them)");
        CHECK(mission_talk_dea < 0, "the Talk to Goddess task is there before Dea is sighted (chunk 11)");
        CHECK(npc_count() == 1 && dea_ty == world_player_tile_y() - 8 && dea_tx == world_player_tile_x(), "Dea is not placed 8 tiles above the player");
        CHECK(sfx_debug_summons() == s0 + 1, "the chime was asked for more than once (%d)", sfx_debug_summons() - s0);
        shot("build/summon_after.bmp");
    }
    run(1.0f);
    CHECK(alex_id < 0, "Alex exists up in the sky");

    CHECK(brainwin_count() == 0, "the head is not empty at the start (%d)", brainwin_count());
    run(3.0f);                                                  /* the player can walk: the cloud thought arrives, no task yet */
    CHECK(brainwin_count() == 1, "expected exactly the cloud thought in the sky, got %d", brainwin_count());
    CHECK(!dea_seen && mission_talk_dea < 0, "the Talk to Goddess task came before Dea was sighted");
    put_player((float)dea_tx, (float)(dea_ty + 5));             /* walk into sight of her */
    run(0.5f);
    CHECK(dea_seen && mission_talk_dea >= 0, "sighting Dea did not add the Talk to Goddess task");
    CHECK(brainwin_count() == 2, "\"Someone is up there.\" did not join the head (%d)", brainwin_count());
    run(3.0f);
    CHECK(brainwin_count() == 2, "the sighting fired more than once (%d)", brainwin_count());
    on_enter_hole();                                            /* skip Dea's talk: straight into the hole (drops the cloud thoughts) */
    CHECK(brainwin_count() == 0, "the cloud thoughts did not go silently when leaving the sky (%d)", brainwin_count());
    run(9.0f);                                                  /* fall 5.5 + lie 1.5 + get up 1.2 */
    CHECK(cur_map == MAP_GREEN, "not on the Grasslands");
    CHECK(alex_id >= 0 && npc_count() == 1, "Alex was not placed (id %d, npcs %d)", alex_id, npc_count());
    float d0 = world_dist_to_npc(alex_id);
    {   int tx, ty; float fx, fy; npc_tile(alex_id, &fx, &fy); tx = (int)fx; ty = (int)fy;
        printf("player tile %d,%d  Alex tile %d,%d  distance %.1f tiles\n", world_player_tile_x(), world_player_tile_y(), tx, ty, d0);
        CHECK(!solid_tile(tx, ty), "Alex stands on a solid tile"); }
    CHECK(d0 >= 20.0f, "Alex is too close to the landing spot (%.1f tiles)", d0);
    CHECK(dialog_active(), "the landing complaint is not showing");
    CHECK(!thought_active(), "a thought is showing before the complaint is over");

    for (int i = 0; i < 12 && dialog_active(); i++) { run(1.5f); tap(TW / 2, TH * 4 / 5); }   /* two pages: type out, tap, type out, tap */
    CHECK(!dialog_active(), "the complaint never ended");
    run(0.7f);
    CHECK(thought_active(), "no first thought after the complaint");
    CHECK(brainwin_count() == 1, "the orb thought did not join the window (%d)", brainwin_count());
    shot("build/ground_first_thought.bmp");
    CHECK(!alex_seen, "Alex counted as spotted from the landing spot");

    /* walk into sight of her: 5 tiles away, on her left */
    float ax, ay; npc_tile(alex_id, &ax, &ay);
    put_player(ax - 5.0f, ay);
    run(0.2f);
    CHECK(alex_seen, "no sighting at 5 tiles");
    CHECK(mission_ask_alex >= 0, "'Ask Alex' was not added");
    run(0.9f);  shot("build/ground_spotted_toast.bmp");         /* toast + first thought on screen */
    CHECK(thought_offset() > nowplaying_offset(), "the open thought card did not push the mission list down");
    {   int tx, ty, tw, th; thought_rect(&tx, &ty, &tw, &th);
        CHECK(missions_bottom() > ty + th, "the mission list is not below the thought card"); }
    run(4.5f);  shot("build/ground_spotted_thought.bmp");       /* her thought has taken over */
    CHECK(brainwin_count() == 2, "expected orb + Alex thoughts, got %d", brainwin_count());
    run(8.0f);
    CHECK(!thought_active(), "thoughts did not finish");
    CHECK(brainwin_count() == 2, "an extra thought arrived after the sighting (%d), the coin thought is gone", brainwin_count());

    /* it fires once: leave and come back */
    put_player(ax - 30.0f, ay); run(0.3f);
    put_player(ax - 5.0f, ay);  run(0.5f);
    CHECK(!thought_active(), "the sighting fired a second time");

    /* talk to her: stand next to her, press the interact button */
    put_player(ax + 1.0f, ay); run(0.4f);
    CHECK(near_id == alex_id, "the interact button does not see Alex (near %d)", near_id);
    {   int u1 = 26 * TW / 360, br = 44 * TW / 360, bx = TW - u1 - br, by = TH - u1 - br;
        tap(bx, by); }
    run(0.3f);
    CHECK(dialog_active(), "talking to Alex opened nothing");
    CHECK(brainwin_count() == 1, "the answered Alex thought was not dropped (%d)", brainwin_count());
    shot("build/ground_alex_talk.bmp");

    /* the scripted deal: tap through the pages until the highlighted words have been typed out */
    run(8.0f);                                                  /* the grass thought (16 s after landing) has arrived by now */
    int before = brainwin_count();
    for (int i = 0; i < 14 && mission_pollute < 0; i++) { run(1.5f); tap(TW / 2, TH * 4 / 5); }
    run(1.0f);
    CHECK(mission_pollute >= 0, "'Pollute the shrine' was not added by Alex's line");
    CHECK(brainwin_count() == before - 1, "the old orb thought was not dropped when the deal came (%d -> %d)", before, brainwin_count());
    shot("build/ground_alex_deal.bmp");

    /* ---------- chunk 10: Dia's shrine, on a fresh run so no dialog is in the way ---------- */
    printf("--- shrine ---\n");
    world_init(TW, TH);
    run(1.5f);
    for (int i = 0; i < 2; i++) { run(1.5f); tap(TW / 2, TH / 2); }
    run(1.0f);
    on_enter_hole();
    run(9.0f);
    for (int i = 0; i < 12 && dialog_active(); i++) { run(1.5f); tap(TW / 2, TH * 4 / 5); }
    run(0.5f);
    CHECK(cur_map == MAP_GREEN && !dialog_active(), "did not get back on the ground for the shrine test");
    CHECK(shrine_exists(), "Dia's shrine was not placed on the Grasslands");
    CHECK(!shrine_enabled(), "the shrine is usable before Alex has asked for it");
    float shx, shy; shrine_tile(&shx, &shy);
    {   float ex, ey; npc_tile(alex_id, &ex, &ey);
        float dl = sqrtf((shx - 40.5f) * (shx - 40.5f) + (shy - 40.9f) * (shy - 40.9f));
        float da = sqrtf((shx - ex) * (shx - ex) + (shy - ey) * (shy - ey));
        printf("shrine tile %.0f,%.0f  %.1f tiles from the landing, %.1f from Alex\n", shx, shy, dl, da);
        CHECK(!solid_tile((int)shx, (int)shy), "the shrine stands on a solid tile");
        CHECK(da >= SHRINE_FROM_ALEX - 0.5f, "the shrine is too close to Alex (%.1f)", da); }
    {   int bx = TW - 26 * TW / 360 - 44 * TW / 360, by = TH - 26 * TW / 360 - 44 * TW / 360;
        put_player(shx - 1.6f, shy); run(0.4f);
        CHECK(!near_shrine, "the button shows for a shrine that is not switched on");
        CHECK(!blocked((shx - 1.6f) * tile, shy * tile), "the player is blocked 1.6 tiles from the shrine");
        CHECK(blocked(shx * tile, shy * tile), "the shrine does not block the player");

        alex_deal_highlight(ALEX_DEAL_PAGE, 0);                 /* what Alex's last line does when its words are typed out */
        run(0.4f);
        CHECK(shrine_enabled() && mission_pollute >= 0, "Alex's line did not switch the shrine on");
        put_player(shx - 8.0f, shy); run(0.3f);
        CHECK(!near_shrine, "the button shows 8 tiles from the shrine");
        put_player(shx - 1.6f, shy); run(0.4f);
        CHECK(near_shrine, "the button does not see the shrine at 1.6 tiles");
        shot("build/ground_shrine_near.bmp");

        tap(bx, by); run(0.3f);                                 /* a tap is not enough */
        CHECK(!shrine_polluted(), "a single tap polluted the shrine");

        world_touch(0, bx, by); run(1.0f);                      /* hold, then let go early */
        float half = shrine_progress();
        CHECK(half > 0.3f && half < 0.6f, "after 1 s of holding the ooze is at %.2f", half);
        shot("build/ground_shrine_half.bmp");
        world_touch(1, bx, by); run(1.5f);
        CHECK(shrine_progress() < half && !shrine_polluted(), "letting go early did not make the ooze creep back (%.2f)", shrine_progress());

        world_touch(0, bx, by); run(SHRINE_HOLD_T + 0.5f);      /* hold all the way */
        CHECK(shrine_polluted(), "holding the button did not pollute the shrine");
        CHECK(shrine_fouled, "the story was not told the shrine is polluted");
        CHECK(!btn_down && !near_shrine, "the button stays on after the shrine is done");
        world_touch(1, bx, by); run(0.3f);
        shot("build/ground_shrine_done.bmp");
        CHECK(!shrine_enabled(), "a polluted shrine can be polluted again");
        CHECK(blocked(shx * tile, shy * tile), "the polluted shrine stopped blocking"); }

    /* ---------- chunk 11: Dia's wrath ---------- */
    printf("--- wrath ---\n");
    CHECK(dying, "the story does not know the vessel is going to die");
    CHECK(!controls_visible, "the controls are still there once the shrine is polluted");
    CHECK(!thought_is_wrath(), "her voice came with no beat of quiet first");
    run(WRATH_BEAT + 0.8f);                                     /* the beat, then her card opening */
    CHECK(thought_is_wrath(), "Dia did not take over the brain button");
    CHECK(!controls_visible, "the controls came back during her wrath");
    shot("build/ground_wrath_open.bmp");
    run(0.3f);  shot("build/ground_wrath_shake.bmp");           /* a different shake frame */
    {   int tx, ty, tw, th; thought_rect(&tx, &ty, &tw, &th);   /* tapping her card must not open the brain window */
        world_touch(0, tx + tw / 2, ty + th / 2);
        thought_touch(0, tx + tw / 2, ty + th / 2); thought_touch(1, tx + tw / 2, ty + th / 2);
        run(0.3f);
        CHECK(!brainwin_active(), "tapping Dia's card opened the brain window"); }
    int ding_before = brainwin_count();
    think("Nothing to see here.", NULL, BRAIN_SCENE_ORB, THOUGHT_COIN, 1);   /* an own thought arriving now is swallowed */
    CHECK(brainwin_count() == ding_before, "the vessel thinks its own thoughts while Dia is in its head");
    run(0.3f);
    CHECK(thought_is_wrath(), "her card vanished too early");          /* about 2.6 s since it opened, it has 3 */
    for (int i = 0; i < 120 && !world_death_active(); i++) run(0.1f);   /* her card's time, the fold, the pause */
    CHECK(!thought_is_wrath(), "her card never folded away");
    CHECK(world_death_active(), "the death never began");
    CHECK(!controls_visible, "the controls are back while the vessel is dying");

    /* ---------- chunk 12: death cutscene I, camera + filters ---------- */
    printf("--- death camera ---\n");
    float t0 = t; int b0 = sfx_debug_beeps(), f0 = sfx_debug_flatlines();
    CHECK(world_death_active(), "death not active");
    run(0.2f);
    CHECK(sfx_debug_beeps() == b0, "the monitor peeped before everything had arrived");
    CHECK(world_debug_zoom() < 1.3f, "the zoom starts with a jump (%.2f)", world_debug_zoom());
    run(1.0f);
    CHECK(world_debug_zoom() > 1.3f && world_debug_zoom() < 2.9f, "mid push-in zoom is %.2f", world_debug_zoom());
    shot("build/ground_death_mid.bmp");
    run(2.0f);
    CHECK(fabsf(world_debug_zoom() - ZOOM_DEATH) < 0.02f, "the camera did not reach 300 %% (%.2f)", world_debug_zoom());
    CHECK(sfx_debug_beeps() == b0 + 1 && sfx_debug_flatlines() == f0, "at the first moment after arriving: %d peeps, %d flatlines (want 1, 0)", sfx_debug_beeps() - b0, sfx_debug_flatlines() - f0);
    CHECK(heart_state() == HEART_BEATING && heart_particles() == 0, "the heart is not beating over the chest (state %d)", heart_state());
    {   int fx, fy, cy, ps; world_death_player(&fx, &fy, &cy, &ps);                 /* the heart is red and sits on the chest */
        Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, hit = 0, n = 0;
        for (int dy = -ps / 2; dy <= ps / 2; dy += ps / 4) for (int dx = -ps / 2; dx <= ps / 2; dx += ps / 4) {      /* the heart is ~2 sprite pixels wide, one pixel right of the middle */
            Uint8 *c = pix + (cy + dy) * pitch + (fx + ps + dx) * 4; n++;
            if (c[0] > 150 && c[1] < 100 && c[2] < 110) hit++;
        }
        CHECK(hit >= n * 2 / 3, "only %d of %d samples at the chest are heart red", hit, n);
        shot("build/ground_heart.bmp"); }
    CHECK(t == t0, "the world is not frozen: its clock moved by %.2f s", t - t0);
    shot("build/ground_death_settled.bmp");
    {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, bad = 0, n = 0;
        for (int y = 560; y < 960; y += 40)                       /* a column of the map, far from the player and the HUD */
            for (int x = 8; x < 60; x += 12) {
                Uint8 *c = pix + y * pitch + x * 4; n++;
                if (abs(c[0] - c[1]) > 3 || abs(c[1] - c[2]) > 3) bad++;
            }
        CHECK(bad == 0, "%d of %d sampled map pixels are not grey", bad, n);
        int red = 0, m = 0;
        for (int dy = -20; dy <= 20; dy += 20) {                  /* the player: around the middle of the screen */
            Uint8 *c = pix + (TH / 2 + dy) * pitch + (TW / 2) * 4; m++;
            if (c[0] > c[1] + 30 && c[0] > c[2] + 30) red++;
        }
        CHECK(red == m, "only %d of %d sampled player pixels are red", red, m); }
    {   int fx, fy, cy, ps; world_death_player(&fx, &fy, &cy, &ps);
        printf("player on screen: feet %d,%d  chest y %d  sprite pixel %d\n", fx, fy, cy, ps);
        CHECK(abs(fx - TW / 2) < 12, "the player is not in the middle of the screen (feet x %d)", fx);
        CHECK(cy < fy && fy - cy > ps * 2 && fy - cy < ps * 5, "the chest is not just above the feet (%d / %d)", cy, fy); }
    CHECK(!controls_visible, "the controls came back during the cutscene");
    run(2.0f);                                                    /* ~2.6 s after arriving: the flatline and the burst are just in */
    CHECK(heart_state() == HEART_BURST && heart_particles() == 2, "the heart did not break with the flatline (state %d, %d halves)", heart_state(), heart_particles());
    CHECK(sfx_debug_flatlines() == f0 + 1, "the flatline and the burst are not together");
    shot("build/ground_burst.bmp");
    run(0.4f);
    shot("build/ground_burst2.bmp");
    {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, redpx = 0;      /* the broken halves on the screen: some bright red */
        for (int y = 200; y < 1000; y += 6) for (int x = 20; x < 520; x += 6) { Uint8 *c = pix + y * pitch + x * 4; if (c[0] > 150 && c[1] < 70 && c[2] < 80) redpx++; }
        CHECK(redpx > 60, "no red on the screen (%d red samples)", redpx); }
    run(1.5f);
    CHECK(heart_particles() == 0 && heart_state() == HEART_BURST, "the broken heart never faded (%d halves left)", heart_particles());
    run(0.8f);                                                    /* same timeline as before: 2.7 s after the burst */
    /* chunk 15.1: a beat after the burst the vessel topples; the words come after it. 2.7 s after the burst: falling is done */
    CHECK(death_fall_t > DEATH_FALL_T, "the vessel did not fall (t %.2f)", death_fall_t);
    shot("build/ground_death_fallen.bmp");
    run(1.0f);                                                    /* the words have had time to fade in */
    /* chunk 15: the words */
    CHECK(death_msg[0] && strcmp(death_msg, "Aonia has died.") == 0, "the death text is '%s'", death_msg);
    CHECK(!vessel_dead, "story_vessel_died ran before the words had their time");
    {   Uint8 *pix = (Uint8 *)surf->pixels; int pitch = surf->pitch, white = 0;      /* the words: white pixels in the lower third, on the dark band */
        for (int y = TH * 4 / 5 - 60; y < TH * 4 / 5 + 60; y += 2) for (int x = 20; x < TW - 20; x += 2) { Uint8 *c = pix + y * pitch + x * 4; if (c[0] > 235 && c[1] > 235 && c[2] > 235) white++; }
        CHECK(white > 40, "no white words on the screen (%d white samples)", white); }
    shot("build/ground_death_text.bmp");
    for (float w = 0; w < DEATH_END_HOLD + 10.0f && !vessel_dead; w += 0.016f) frame(0.016f);   /* up to the end hook */
    CHECK(vessel_dead, "story_vessel_died never ran");
    run(0.3f);                                                    /* the fade has just begun */
    CHECK(world_death_active() && death_msg[0], "the death screen went before the fade to black began");
    CHECK(sfx_debug_beeps() == b0 + DEATH_BEEPS && sfx_debug_flatlines() == f0 + 1, "the monitor played %d peeps and %d flatlines (want %d, 1)", sfx_debug_beeps() - b0, sfx_debug_flatlines() - f0, DEATH_BEEPS);
    CHECK(!controls_visible, "the controls came back at the end");
    /* chunk 12: the world fades to black over DEATH_FADE_T, the cutscene is put away behind it, limbo is requested once */
    CHECK(lives == 1, "lives is %d after vessel 1 died (want 1)", lives);
    CHECK(death_tx == world_player_tile_x() && death_ty == world_player_tile_y(), "the death tile was not remembered");
    CHECK(blackout_t > 0 && blackout_t < blackout_dur && !story_limbo_active() && !story_limbo_take_enter(), "the fade to black is not under way (t %.2f)", blackout_t);
    shot("build/ground_fade_black.bmp");
    run(DEATH_FADE_T);
    CHECK(blackout_t >= blackout_dur, "the fade to black never finished");
    CHECK(!world_death_active() && heart_state() == HEART_OFF && !death_msg[0], "the cutscene was not put away behind the black");
    CHECK(story_limbo_active() && thought_voice() == &SOUL, "the soul's limbo did not begin after the death");
    CHECK(story_limbo_take_enter() == 1 && story_limbo_take_enter() == 0, "the move into limbo must be reported to game.c exactly once");
    /* chunk 13: the soul says "Uh. I died." at once (no coin, not in the brain window), then a hold of DEATH_LIMBO_HOLD before the story goes on */
    CHECK(limbo.n == 1 && strcmp(limbo.lines[0], "Uh. I died.") == 0 && thought_active(), "the soul did not say \"Uh. I died.\" on arriving in limbo");
    CHECK(DEATH_LINES_COUNT >= 1 && DEATH_LINES[0].lines == DEATH_LINES_1, "the vessel table has no entry for vessel 1");
    {   int bc = brainwin_count(), h0 = death_hold_done;
        for (float t = 0; t < DEATH_LIMBO_HOLD - 0.3f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(death_hold_done == h0, "the hold after the death line ended too early");
        for (float t = 0; t < 0.6f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(death_hold_done == h0 + 1, "the hold after the death line did not end (%d)", death_hold_done - h0);
        for (float t = 0; t < 5.0f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(death_hold_done == h0 + 1 && story_limbo_active(), "the stand-in hold ran twice or limbo ended by itself");
        CHECK(brainwin_count() == bc, "the death line joined the brain window"); }
    limbo_end(); story_limbo_take_end(); blackout_t = -1;           /* the tests below start from the world again */
    run(2.5f);
    CHECK(world_debug_zoom() < 1.05f, "the camera did not zoom back out (%.2f)", world_debug_zoom());

    /* chunk 6: limbo_run. the soul says its lines (first at once, then one per gap), none of them joins the brain window,
     * on_done runs one gap after the last, limbo_end() gives the voice back and tells game.c exactly once */
    {   static const char *const LT[] = { "Where am I?", "Hello?", "Anyone?" };
        int bc = brainwin_count();
        CHECK(!story_limbo_active() && !story_limbo_take_end(), "limbo is on before limbo_run");
        limbo_done_calls = 0;
        limbo_run(LT, 3, 1.0f, limbo_test_done);
        CHECK(story_limbo_active(), "limbo_run did not start limbo");
        CHECK(thought_voice() == &SOUL, "the thoughts do not wear the soul's face in limbo");
        CHECK(thought_active(), "the first limbo thought was not said at once");
        for (float t = 0; t < 0.9f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(limbo.i == 1, "second thought came before its gap (i %d)", limbo.i);
        for (float t = 0; t < 0.2f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(limbo.i == 2, "second thought did not come after one gap (i %d)", limbo.i);
        for (float t = 0; t < 1.0f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(limbo.i == 3 && limbo_done_calls == 0, "third thought / on_done out of order (i %d, done %d)", limbo.i, limbo_done_calls);
        for (float t = 0; t < 0.8f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(limbo_done_calls == 0, "on_done ran before the last thought had its hold");
        for (float t = 0; t < 0.3f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(limbo_done_calls == 1, "on_done did not run after the hold (%d)", limbo_done_calls);
        for (float t = 0; t < 3.0f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(limbo_done_calls == 1, "on_done ran more than once (%d)", limbo_done_calls);
        CHECK(brainwin_count() == bc, "a limbo thought joined the brain window (%d -> %d)", bc, brainwin_count());
        CHECK(story_limbo_active() && thought_voice() == &SOUL, "limbo ended by itself");
        limbo_end();
        CHECK(!story_limbo_active() && thought_voice() != &SOUL, "limbo_end did not give the voice back");
        CHECK(story_limbo_take_end() == 1 && story_limbo_take_end() == 0, "limbo_end must be reported to game.c exactly once");
        limbo_run(NULL, 0, 0.5f, limbo_test_done);                /* no lines at all: just a hold, then on_done */
        for (float t = 0; t < 0.6f; t += 0.01f) story_limbo_update(0.01f);
        CHECK(limbo_done_calls == 2, "an empty limbo_run did not just hold and finish (%d)", limbo_done_calls);
        limbo_end(); story_limbo_take_end();
    }

    /* chunk 7: the beginning. a beat of stars, "Where am I?" in the soul's voice, a hold of about 3.5 s, then limbo ends
     * (stand-in: the world and the old intro follow). nothing joins the brain window; the dev entry stays in limbo */
    {   int bc = brainwin_count();
        story_limbo_begin(0);
        CHECK(story_limbo_active() && thought_voice() == &SOUL, "story_limbo_begin did not start limbo with the soul's voice");
        for (float t = 0; t < 0.9f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(!thought_active(), "a thought came before the beat of silence");
        for (float t = 0; t < 0.3f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }    /* 1.2 s */
        CHECK(thought_active() && limbo.n == 1 && strcmp(limbo.lines[0], "Where am I?") == 0, "no \"Where am I?\" after the beat");
        for (float t = 0; t < 3.0f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }    /* 4.2 s: still holding */
        CHECK(story_limbo_active() && !story_limbo_take_end(), "limbo ended before the hold was over");
        for (float t = 0; t < 0.5f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }    /* 4.7 s */
        CHECK(story_limbo_take_end() == 1, "limbo did not end after \"Where am I?\" and its hold");
        CHECK(!story_limbo_active() && thought_voice() != &SOUL, "the soul's voice stayed after limbo");
        CHECK(brainwin_count() == bc, "\"Where am I?\" joined the brain window");
        story_limbo_begin(1);                                                                          /* dev entry: says it and stays */
        for (float t = 0; t < 12.0f; t += 0.01f) { story_limbo_update(0.01f); thought_update(0.01f); }
        CHECK(story_limbo_active() && !story_limbo_take_end(), "the dev limbo ended by itself");
        limbo_end(); story_limbo_take_end();
    }

    printf(fails ? "%d check(s) FAILED\n" : "all checks passed\n", fails);
    return fails ? 1 : 0;
}
