/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the save file (story.c): a headless round trip. a state from the middle of the game is saved by the real autosave, the story is wiped (a new
 * game), LOAD puts it back and the summoning of the saved life runs. also: no file, a broken file, a file from another version. exits non-zero if a
 * check fails. chunk 48: also the block after the v1 struct (mood vector + facts, version byte) and the fixture tools/fixtures/save_v1.dat, the 300 byte
 * file the build BEFORE chunk 48 wrote (do not regenerate it: it is the proof that old saves load). run from build/ so the fixture is found:
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
    DEA_MIND.trust = -30; DEA_MIND.fear = 55; DEA_MIND.energy = -10; DEA_MIND.amuse = 7; DEA_MIND.grief = 90;       /* chunk 48: the vector and the facts */
    ALEX_MIND.trust = 40; ALEX_MIND.fear = -5; ALEX_MIND.energy = 60; ALEX_MIND.amuse = -70; ALEX_MIND.grief = 1;
    { CtxFacts cf; memset(&cf, 0, sizeof cf); snprintf(cf.name, sizeof cf.name, "Zed"); cf.insulted = 1; cf.thanked = 1; lang_ctx_facts_load(&cf); }
    int charisma = lang_charisma();
    CHECK(autosave(), "autosave failed");
    { FILE *g = fopen(SAVE_FILE, "rb"); fseek(g, 0, SEEK_END); long sz = ftell(g); fclose(g);
      CHECK(sz == (long)(sizeof(SaveFile) + sizeof(SaveExt)), "a new save is the v1 struct plus the block (%ld bytes)", sz); }
    CHECK(story_save_exists(), "the saved file is not usable");

    story_start();                                      /* a new game wipes everything ... */
    CHECK(lives == 0 && grave_n == 0 && !alex_dead && !secret_on, "story_start did not wipe the state");
    CHECK(!DEA_MIND.trust && !DEA_MIND.fear && !DEA_MIND.energy && !DEA_MIND.amuse && !DEA_MIND.grief && !ALEX_MIND.trust && !ALEX_MIND.fear && !ALEX_MIND.energy
          && !ALEX_MIND.amuse && !ALEX_MIND.grief, "a new game did not clear the mood vector");
    CHECK(!lang_ctx_facts()->name[0] && !lang_ctx_facts()->insulted && !lang_ctx_facts()->thanked, "a new game did not clear the facts");
    DEA_MIND.mood = 0; ALEX_MIND.mood = 10; lang_add_charisma(50 - lang_charisma());
    CHECK(story_load_begin(), "LOAD failed on a good save");     /* ... and LOAD brings it back */
    CHECK(lives == 2 && aonia_buried && hammer_taken && aonia_searched && hammer_tx == 12 && hammer_ty == 34, "lives / hammer state");
    CHECK(alex_dealt && alex_seen && alex_refused && alex_flow && alex_struck && alex_dead && alex_no_n == 3, "alex state");
    CHECK(shrine_fouled && shrine_repaired && self_done && orb_taken && !orb_given && secret_on && hammer_hinted && hammer_said, "shrine / orb / secret state");
    CHECK(DEA_MIND.mood == -44 && ALEX_MIND.mood == 3 && lang_charisma() == charisma, "moods / charisma (%d %d %d)", DEA_MIND.mood, ALEX_MIND.mood, lang_charisma());
    CHECK(DEA_MIND.trust == -30 && DEA_MIND.fear == 55 && DEA_MIND.energy == -10 && DEA_MIND.amuse == 7 && DEA_MIND.grief == 90, "Dea's mood vector");
    CHECK(ALEX_MIND.trust == 40 && ALEX_MIND.fear == -5 && ALEX_MIND.energy == 60 && ALEX_MIND.amuse == -70 && ALEX_MIND.grief == 1, "Alex's mood vector");
    CHECK(!strcmp(lang_ctx_facts()->name, "Zed") && lang_ctx_facts()->insulted == 1 && lang_ctx_facts()->thanked == 1, "the persisted facts (%s)", lang_ctx_facts()->name);
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
    long full = (long)(sizeof(SaveFile) + sizeof(SaveExt));
    FILE *f = fopen(SAVE_FILE, "r+b"); fseek(f, 20, SEEK_SET); fputc(0x55, f); fputc(0xAA, f); fclose(f);
    CHECK(!story_save_exists(), "a corrupt save was accepted");
    story_start(); CHECK(!story_load_begin() && lives == 0, "a corrupt save was loaded");
    SaveFile sf; story_start(); lives = 1; save_fill(&sf); sf.version = SAVE_VERSION + 1; sf.check = save_check(&sf);
    f = fopen(SAVE_FILE, "wb"); fwrite(&sf, 1, sizeof sf, f); fclose(f);
    CHECK(!story_save_exists(), "a save from another version was accepted");
    f = fopen(SAVE_FILE, "wb"); fwrite(&sf, 1, sizeof sf - 4, f); fclose(f);
    CHECK(!story_save_exists(), "a short file was accepted");
    remove(SAVE_FILE);

    /* chunk 48: a save made BEFORE the block (the fixture is the file the build before this chunk wrote: the v1 struct, 300 bytes) still loads, with the
     * vector at zero and no facts; the moods and the rest come back as saved */
    {   const char *fx[] = { "../tools/fixtures/save_v1.dat", "tools/fixtures/save_v1.dat" }; FILE *in = NULL;
        for (int i = 0; i < 2 && !in; i++) in = fopen(fx[i], "rb");
        CHECK(in != NULL, "the v1 fixture is missing (tools/fixtures/save_v1.dat)");
        if (in) {
            unsigned char fb[1024]; size_t fn = fread(fb, 1, sizeof fb, in); fclose(in);
            CHECK(fn == sizeof(SaveFile), "the v1 fixture is %zu bytes, the v1 struct is %zu", fn, sizeof(SaveFile));
            f = fopen(SAVE_FILE, "wb"); fwrite(fb, 1, fn, f); fclose(f);
            CHECK(story_save_exists(), "the old (v1) save was not accepted");
            DEA_MIND.trust = 99; ALEX_MIND.grief = 99; { CtxFacts cf; memset(&cf, 0, sizeof cf); snprintf(cf.name, sizeof cf.name, "Old"); lang_ctx_facts_load(&cf); }
            story_start(); DEA_MIND.trust = 99; ALEX_MIND.grief = 99;
            { CtxFacts cf; memset(&cf, 0, sizeof cf); snprintf(cf.name, sizeof cf.name, "Old"); cf.insulted = 1; lang_ctx_facts_load(&cf); }
            CHECK(story_load_begin(), "LOAD failed on the old (v1) save");
            CHECK(lives == 2 && aonia_buried && hammer_taken && alex_dead && alex_no_n == 3 && secret_on && orb_taken, "the old save's story state");
            CHECK(DEA_MIND.mood == -44 && ALEX_MIND.mood == 3, "the old save's moods (%d %d)", DEA_MIND.mood, ALEX_MIND.mood);
            CHECK(!DEA_MIND.trust && !DEA_MIND.fear && !DEA_MIND.energy && !DEA_MIND.amuse && !DEA_MIND.grief && !ALEX_MIND.trust && !ALEX_MIND.fear
                  && !ALEX_MIND.energy && !ALEX_MIND.amuse && !ALEX_MIND.grief, "an old save left a mood vector (%d %d)", DEA_MIND.trust, ALEX_MIND.grief);
            CHECK(!lang_ctx_facts()->name[0] && !lang_ctx_facts()->insulted && !lang_ctx_facts()->thanked, "an old save kept facts ('%s')", lang_ctx_facts()->name);
            CHECK(convo_locked(), "the old save's chat lock did not come back");
        }
    }
    remove(SAVE_FILE);

    /* the block: what is accepted and what is not */
    {   SaveExt ex; unsigned char all[sizeof(SaveFile) + sizeof(SaveExt) + 64]; story_start(); lives = 2; save_fill(&sf); save_fill_ext(&ex);
        #define WRITE_FILE(n) do { f = fopen(SAVE_FILE, "wb"); fwrite(all, 1, (n), f); fclose(f); } while (0)
        memcpy(all, &sf, sizeof sf); memcpy(all + sizeof sf, &ex, sizeof ex); WRITE_FILE(sizeof sf + sizeof ex);
        CHECK(story_save_exists(), "a whole new save was not accepted");
        { SaveFile s2; SaveExt e2; int has; CHECK(save_read2(&s2, &e2, &has) && has && e2.version == SAVE_EXT_VERSION && e2.len == sizeof e2, "the block is read"); }
        WRITE_FILE(sizeof sf + sizeof ex - 1); CHECK(!story_save_exists(), "a save cut inside the block was accepted");
        WRITE_FILE(sizeof sf + 4); CHECK(!story_save_exists(), "a save with a 4 byte tail was accepted");
        WRITE_FILE(sizeof sf + 11); CHECK(!story_save_exists(), "a save with a tail shorter than a block header was accepted");
        memset(all + sizeof sf + sizeof ex, 0x7A, 64); WRITE_FILE(sizeof sf + sizeof ex + 1); CHECK(!story_save_exists(), "a save with one byte too many was accepted");
        memcpy(all + sizeof sf, &ex, sizeof ex); memset(all + sizeof sf, 0x7A, 12); WRITE_FILE(sizeof sf + sizeof ex); CHECK(!story_save_exists(), "a tail that is not a block was accepted");
        /* a block with a wrong check: the v1 part is good, so it loads with the defaults */
        { SaveExt bad = ex; bad.dea_vec[0] = 77; memcpy(all + sizeof sf, &bad, sizeof bad); WRITE_FILE(sizeof sf + sizeof bad);
          SaveFile s2; SaveExt e2; int has = 1; CHECK(save_read2(&s2, &e2, &has) && !has, "a block with a wrong check is not used (has %d)", has); }
        /* a block of a later version (same size): skipped, defaults */
        { SaveExt nx = ex; nx.version = SAVE_EXT_VERSION + 1; nx.check = save_ext_check(&nx); memcpy(all + sizeof sf, &nx, sizeof nx); WRITE_FILE(sizeof sf + sizeof nx);
          SaveFile s2; SaveExt e2; int has = 1; CHECK(save_read2(&s2, &e2, &has) && !has, "a block of a later version is skipped (has %d)", has); }
        /* a longer block of a later version: skipped, defaults (len says how long it is) */
        { SaveExt nx = ex; nx.version = SAVE_EXT_VERSION + 1; nx.len = sizeof ex + 16; memcpy(all + sizeof sf, &nx, sizeof nx); memset(all + sizeof sf + sizeof nx, 0, 16);
          WRITE_FILE(sizeof sf + sizeof ex + 16); /* longer than this build's block: not a block this build can hold */
          CHECK(!story_save_exists(), "a longer block was accepted (the buffer of this build ends at its own size)"); }
        /* out of range values in a good block are clamped, a name without its end is cut */
        { SaveExt wild = ex; wild.dea_vec[0] = -128; wild.alex_vec[1] = 127; memset(wild.facts.name, 'x', sizeof wild.facts.name); wild.check = save_ext_check(&wild);
          memcpy(all + sizeof sf, &wild, sizeof wild); WRITE_FILE(sizeof sf + sizeof wild);
          story_start(); CHECK(story_load_begin(), "a wild block stopped LOAD");
          CHECK(DEA_MIND.trust == -100 && ALEX_MIND.fear == 100, "the vector was not clamped (%d %d)", DEA_MIND.trust, ALEX_MIND.fear);
          CHECK(strlen(lang_ctx_facts()->name) < sizeof wild.facts.name, "the name was not cut"); }
        #undef WRITE_FILE
        remove(SAVE_FILE); (void)full; }

    /* the real trigger: a death's hold-over writes the save (life 1 -> summoning of life 2) */
    story_start(); lives = 1; grave_n = 0;
    death_limbo_hold_over();
    CHECK(story_save_exists(), "the summoning after a death did not save the game");
    remove(SAVE_FILE);

    printf(fails ? "savetest: %d FAILED\n" : "savetest: ok\n", fails);
    return fails != 0;
}
