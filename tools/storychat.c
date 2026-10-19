/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * chunk 47: Dea and Alex on the new engine, as story.c defines them. it #includes world.c and story.c to reach their statics (DEA_MIND, ALEX_MIND,
 * the orb watch). checks: their hooks / quirk / lean, the scripted orb refusal (ALEX_NO) fires on every orb phrase incl. typos and glued words and not on
 * absorb / orbit / ballad, Dea in a bad mood is cold or hostile in every reply (never warm), Alex stays friendly (never hostile), both hooks have lines.
 *   cc -O1 -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -o build/storychat tools/storychat.c $(ls game/*.c | grep -v -e game/game.c -e game/world.c -e game/story.c) $(sdl2-config --libs) -lm
 *   build/storychat */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include "world.c"
#include "story.c"
#include "lang.h"

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static int orb_hit(const char *typed, char *reply, int cap) {
    char und[400]; lang_understood(typed, und, (int)sizeof und);
    return alex_orb_ask(typed, und, reply, cap);
}

int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_Init(SDL_INIT_VIDEO);
    lang_seed(47);

    /* the scripted orb refusal */
    static const char *const HIT[] = { "give me the orb", "Give me the ORB!", "orb", "orb pls", "give me the obr", "can i have the sphere", "gimme the orbs",
        "giveme the orb", "give me the 0rb", "i want the glowing ball", "where is the ball", "give me the glowing thing", "give me the thing you found",
        "pleeease give me the orb", "hand over the glow" };
    static const char *const MISS[] = { "absorb", "i absorb the light", "the orbit of the moon", "ballad of a ghost", "hello", "who are you", "tell me about the grass",
        "what is that glowstick", "cobweb", "you are glowering at me", "" };
    for (int i = 0; i < (int)(sizeof HIT / sizeof HIT[0]); i++) {
        char r[200]; alex_no_n = 0; alex_refused = 0;
        CHECK(orb_hit(HIT[i], r, (int)sizeof r), "'%s' was not taken for an orb ask", HIT[i]);
        CHECK(!strcmp(r, ALEX_NO[0]) && alex_no_n == 1 && alex_refused, "'%s': first no is '%s' (n=%d)", HIT[i], r, alex_no_n);
    }
    for (int i = 0; i < (int)(sizeof MISS / sizeof MISS[0]); i++) {
        char r[200] = ""; alex_no_n = 0; alex_refused = 0;
        CHECK(!orb_hit(MISS[i], r, (int)sizeof r), "'%s' was taken for an orb ask", MISS[i]);
        CHECK(alex_no_n == 0 && !alex_refused && !r[0], "'%s' changed the refusal state", MISS[i]);
    }
    { char r[200]; alex_no_n = 0; alex_refused = 0;                         /* the three noes in turn, then the last one for good */
      for (int i = 0; i < 5; i++) { orb_hit("give me the orb", r, (int)sizeof r); CHECK(!strcmp(r, ALEX_NO[i < ALEX_NO_COUNT ? i : ALEX_NO_COUNT - 1]), "no number %d is '%s'", i, r); } }
    alex_no_n = 0; alex_refused = 0;

    /* who they are */
    CHECK(!strcmp(DEA_MIND.hooks[0], "mission") && !strcmp(DEA_MIND.hooks[1], "obey") && !DEA_MIND.hooks[2], "Dea's hooks");
    CHECK(ALEX_MIND.hooks[0] && !strcmp(ALEX_MIND.hooks[0], "grass") && !ALEX_MIND.hooks[1], "Alex's hooks: grass only, never the orb (it would contradict her no)");
    CHECK(ALEX_MIND.quirk & Q_ASKS_BACK, "Alex asks back");
    CHECK(!(DEA_MIND.quirk & Q_ASKS_BACK), "Dea does not ask back");
    CHECK((DEA_MIND.lean & LEAN(TONE_COLD)) && !(DEA_MIND.lean & LEAN(TONE_WARM)), "Dea's lean is cold, never warm");
    CHECK((ALEX_MIND.lean & LEAN(TONE_WARM)) && !(ALEX_MIND.lean & LEAN(TONE_HOSTILE)), "Alex's lean is warm, never hostile");
    { char b[300]; CHECK(lang_line(&DEA_MIND, "ch.mission", b, sizeof b) && lang_line(&DEA_MIND, "ch.obey", b, sizeof b), "Dea's hooks have lines");
      CHECK(lang_line(&ALEX_MIND, "ch.grass", b, sizeof b), "Alex's hook has lines"); }

    /* moods: Dea in a bad mood is never warm in any reply; Alex in a good mood is never hostile or cold. 300 replies each, several kinds of line */
    static const char *const SAY[] = { "hello", "how are you", "who are you", "thank you", "tell me a joke", "what is the hole", "i like you", "where am i", "help me", "i am scared" };
    int dea_warm = 0, dea_cold = 0, alex_bad = 0, alex_n = 0, dea_n = 0;
    for (int i = 0; i < 300; i++) {
        Heard h; char r[480];
        DEA_MIND.mood = -70; DEA_MIND.hold = 0; DEA_MIND.held = 0;
        lang_parse(SAY[i % 10], &h); lang_reply(&DEA_MIND, &h, r, sizeof r); int t = lang_reply_tone(); dea_n++;
        if (t == TONE_WARM || t == TONE_TENDER || t == TONE_PLAYFUL || t == TONE_EXCITED) dea_warm++;
        if (t == TONE_COLD || t == TONE_HOSTILE || t == TONE_SARCASTIC || t == TONE_SMUG || t == TONE_BORED) dea_cold++;
        ALEX_MIND.mood = 60; ALEX_MIND.hold = 0; ALEX_MIND.held = 0;
        lang_parse(SAY[i % 10], &h); lang_reply(&ALEX_MIND, &h, r, sizeof r); t = lang_reply_tone(); alex_n++;
        if (t == TONE_HOSTILE || t == TONE_COLD || t == TONE_SARCASTIC) alex_bad++;
    }
    printf("  Dea at mood -70: %d/%d cold-ish, %d warm; Alex at +60: %d/%d hostile or cold\n", dea_cold, dea_n, dea_warm, alex_bad, alex_n);
    CHECK(dea_warm == 0, "Dea was warm %d times in a bad mood", dea_warm);
    CHECK(dea_cold * 10 >= dea_n * 8, "Dea cold-ish only %d of %d times in a bad mood", dea_cold, dea_n);
    CHECK(alex_bad == 0, "Alex was hostile or cold %d times in a good mood", alex_bad);

    /* Alex asks back now and then (Q_ASKS_BACK: from the third turn, then every 3-5 replies); Dea (no quirk, a cold mood) almost never. the chat context is
     * fed the way convo.c does (lang_ctx_note after each turn) */
    {   int asks[2] = { 0, 0 }, maxgap = 0;
        for (int who = 0; who < 2; who++) {
            Persona *m = who ? &DEA_MIND : &ALEX_MIND; int gap = 0;
            m->mood = who ? -30 : 10; lang_ctx_reset();
            for (int i = 0; i < 60; i++) {
                Heard h; char r[480];
                lang_parse(SAY[i % 10], &h); lang_reply(m, &h, r, sizeof r);
                lang_ctx_note(&h, SAY[i % 10], lang_reply_key(), m->mood);
                if (*lang_reply_askback()) { asks[who]++; gap = 0; } else if (!who && ++gap > maxgap) maxgap = gap;
            }
        }
        printf("  60 replies: Alex asked back %d times (longest gap %d), Dea %d\n", asks[0], maxgap, asks[1]);
        CHECK(asks[0] >= 8, "Alex asked back only %d times in 60 replies", asks[0]);
        CHECK(maxgap <= 8, "Alex went %d replies without asking back", maxgap);
        CHECK(asks[1] < asks[0], "Dea asked back as much as Alex (%d vs %d)", asks[1], asks[0]); }
    ALEX_MIND.mood = 10; DEA_MIND.mood = 0;

    printf("storychat: %d checks, %d problems\n", checks, fails);
    return fails ? 1 : 0;
}
