/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the ground part of vessel 1, end to end on a PC: fall to the Grasslands, land, the complaint, the
 * first thought, Alex far away, walking into sight of her (thought + "Ask Alex" task), talking to her.
 * it #includes world.c and story.c so it can reach their statics (no test hooks in the shipped game).
 * writes build/ground_*.bmp, exits non-zero if a check fails:
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/groundtest tools/groundtest.c $(ls game/*.c | grep -v -e game/game.c -e game/world.c -e game/story.c) $(sdl2-config --libs) -lm
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

    run(1.5f);                                                  /* fade-in, then the two sky windows */
    for (int i = 0; i < 2; i++) { run(1.5f); tap(TW / 2, TH / 2); }
    run(1.0f);
    CHECK(alex_id < 0, "Alex exists up in the sky");

    CHECK(brainwin_count() == 0, "the head is not empty at the start (%d)", brainwin_count());
    run(3.0f);                                                  /* the player can walk: the cloud thought arrives */
    CHECK(brainwin_count() == 1, "expected exactly the cloud thought in the sky, got %d", brainwin_count());
    on_enter_hole();                                            /* skip Dea's talk: straight into the hole (drops the cloud thought) */
    CHECK(brainwin_count() == 0, "the cloud thought did not go silently when leaving the sky (%d)", brainwin_count());
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
    CHECK(brainwin_count() == 3, "the coin thought did not arrive after the second task (%d)", brainwin_count());

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
    CHECK(brainwin_count() == 2, "the answered Alex thought was not dropped (%d)", brainwin_count());
    shot("build/ground_alex_talk.bmp");

    /* the scripted deal: tap through the pages until the highlighted words have been typed out */
    run(8.0f);                                                  /* the grass thought (16 s after landing) has arrived by now */
    int before = brainwin_count();
    for (int i = 0; i < 14 && mission_pollute < 0; i++) { run(1.5f); tap(TW / 2, TH * 4 / 5); }
    run(1.0f);
    CHECK(mission_pollute >= 0, "'Pollute Dia's shrine' was not added by Alex's line");
    CHECK(brainwin_count() == before - 1, "the old orb thought was not dropped when the deal came (%d -> %d)", before, brainwin_count());
    shot("build/ground_alex_deal.bmp");

    printf(fails ? "%d check(s) FAILED\n" : "all checks passed\n", fails);
    return fails ? 1 : 0;
}
