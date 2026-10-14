/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the save file (story.c): a headless round trip. a state from the middle of the game is saved by the real autosave, the story is wiped (a new
 * game), LOAD puts it back and the summoning of the saved life runs. also: no file, a broken file, a file from another version. exits non-zero if a
 * check fails:
 *   cc -O1 -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -o build/savetest tools/savetest.c $(ls game/*.c | grep -v -e game/game.c -e game/world.c -e game/story.c) $(sdl2-config --libs) -lm
 *   (cd build && ./savetest) */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include "world.c"
#include "story.c"
#include "lang.h"

#define TW 540
#define TH 1170
static SDL_Renderer *rr; static SDL_Surface *surf;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void frame(float dt) { thought_set_enabled(1); story_limbo_update(dt); world_update(dt); thought_update(dt); world_draw(rr); thought_draw(rr); }
static void run(float sec) { for (float s = 0; s < sec; s += 0.016f) frame(0.016f); }

int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, TW, TH, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    lang_seed(7);
    brainwin_init(TW, TH);
    world_init(TW, TH);
    remove(SAVE_FILE);

    CHECK(!story_save_exists(), "a save exists before anything was saved");
    story_start();
    CHECK(!story_load_begin(), "LOAD worked with no save file");
    CHECK(!limbo.on, "a failed LOAD touched limbo");

    /* a state from the middle of the game: Doia (life 2) died at the shrine after Aonia's burial, Alex is dead, the orb is Dea's */
    lives = 2; aonia_buried = 1; hammer_taken = 1; aonia_searched = 1; hammer_tx = 12; hammer_ty = 34;
    alex_dealt = alex_seen = alex_refused = alex_flow = alex_struck = alex_dead = 1; alex_no_n = 3;
    shrine_fouled = shrine_repaired = self_done = 1; orb_taken = 1; secret_on = 1;
    hammer_hinted = hammer_said = 1;
    DEA_MIND.mood = -44; ALEX_MIND.mood = 3; lang_add_charisma(20);
    green.house_tx = 40; green.house_ty = 41; green.alex_there = 1; green.shrine_tx = 20; green.shrine_ty = 21; green.shrine_there = 1; green.shrine_polluted = 0;
    grave_n = 2; graves[0].tx = 5; graves[0].ty = 6; graves[0].who = &VESSEL; graves[1].tx = 7; graves[1].ty = 8; graves[1].who = &VESSEL2;
    { int ff[4] = { 1, 1, 0, 1 }; const Person *fg[2] = { &VESSEL, &VESSEL2 }; world_found_set(ff, 6, fg, 2); }
    int charisma = lang_charisma();
    CHECK(autosave(), "autosave failed");
    CHECK(story_save_exists(), "the saved file is not usable");

    story_start();                                      /* a new game wipes everything ... */
    CHECK(lives == 0 && grave_n == 0 && !alex_dead && !secret_on, "story_start did not wipe the state");
    DEA_MIND.mood = 0; ALEX_MIND.mood = 10; lang_add_charisma(50 - lang_charisma());
    CHECK(story_load_begin(), "LOAD failed on a good save");     /* ... and LOAD brings it back */
    CHECK(lives == 2 && aonia_buried && hammer_taken && aonia_searched && hammer_tx == 12 && hammer_ty == 34, "lives / hammer state");
    CHECK(alex_dealt && alex_seen && alex_refused && alex_flow && alex_struck && alex_dead && alex_no_n == 3, "alex state");
    CHECK(shrine_fouled && shrine_repaired && self_done && orb_taken && !orb_given && secret_on && hammer_hinted && hammer_said, "shrine / orb / secret state");
    CHECK(DEA_MIND.mood == -44 && ALEX_MIND.mood == 3 && lang_charisma() == charisma, "moods / charisma (%d %d %d)", DEA_MIND.mood, ALEX_MIND.mood, lang_charisma());
    CHECK(green.house_tx == 40 && green.house_ty == 41 && green.alex_there && green.shrine_tx == 20 && green.shrine_ty == 21 && green.shrine_there && !green.shrine_polluted, "green");
    CHECK(grave_n == 2 && graves[0].tx == 5 && graves[0].who == &VESSEL && graves[1].tx == 7 && graves[1].ty == 8 && graves[1].who == &VESSEL2, "graves");
    CHECK(world_found_house() && world_found_shrine() && !world_found_hammer() && world_found_temple() && world_found_count() == 6, "found flags");
    CHECK(world_found_grave(&VESSEL) && world_found_grave(&VESSEL2) && !world_found_grave(&VESSEL3), "found graves");
    CHECK(convo_locked(), "Dea's order (the chat lock) did not come back");
    CHECK(limbo.on && limbo.running, "limbo did not start");

    run(0.5f);                                          /* the beat of stars ... */
    CHECK(limbo.on && !story_limbo_take_end(), "limbo ended early");
    run(1.5f);                                          /* ... then the next life is made ready like after a death */
    CHECK(story_limbo_take_end(), "limbo did not end");
    CHECK(world_player() == &VESSEL3, "the saved life's body is not summoned (life 3 = Tria)");
    CHECK(dea_id >= 0 && dea_seen, "Dea is not waiting in her temple");
    run(12.0f);                                         /* the dawn and the summoning */
    CHECK(mission_talk_dea >= 0, "\"Talk to Goddess\" did not coin in after the loaded summoning");
    CHECK(orb_taken && world_has_item(ITEM_ORB), "the orb we carried is not in the pockets");

    /* a broken file and another version are ignored */
    FILE *f = fopen(SAVE_FILE, "r+b"); fseek(f, 20, SEEK_SET); fputc(0x55, f); fputc(0xAA, f); fclose(f);
    CHECK(!story_save_exists(), "a corrupt save was accepted");
    story_start(); CHECK(!story_load_begin() && lives == 0, "a corrupt save was loaded");
    SaveFile sf; story_start(); lives = 1; save_fill(&sf); sf.version = SAVE_VERSION + 1; sf.check = save_check(&sf);
    f = fopen(SAVE_FILE, "wb"); fwrite(&sf, 1, sizeof sf, f); fclose(f);
    CHECK(!story_save_exists(), "a save from another version was accepted");
    f = fopen(SAVE_FILE, "wb"); fwrite(&sf, 1, sizeof sf - 4, f); fclose(f);
    CHECK(!story_save_exists(), "a short file was accepted");
    remove(SAVE_FILE);

    /* the real trigger: a death's hold-over writes the save (life 1 -> summoning of life 2) */
    story_start(); lives = 1; grave_n = 0;
    death_limbo_hold_over();
    CHECK(story_save_exists(), "the summoning after a death did not save the game");
    remove(SAVE_FILE);

    printf(fails ? "savetest: %d FAILED\n" : "savetest: ok\n", fails);
    return fails != 0;
}
