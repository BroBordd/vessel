/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the mood state of the language engine (docs/convo_plan.md, chunk 8). no SDL:
 *   cc -O1 -Igame -o build/moodtest tools/moodtest.c game/lang*.c game/lines_*.c && build/moodtest
 *   1. a character with no vector and no lean (Dea, Alex) keeps the old behaviour: only the 4 old moods, in the exact proportions of
 *      the old formula (checked against the formula itself, fixed seed), and an old-intent chat never makes a mood stick
 *   2. scripted nudges: the vector, the lean and the hard drop give the expected mood
 *   3. a mood sticks for 2-4 turns (all of 2, 3 and 4 happen) and does not flicker while the push lasts
 *   4. every mood answers every reply key (through the fallback chain) */
#include "lang.h"
#include "lang_data.h"
#include "lang_tables.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int bad;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); bad++; } } while (0)

static void turn(Persona *p) { Heard h; memset(&h, 0, sizeof h); lang_apply(p, &h); }                 /* a turn where nothing was understood */
static void say(Persona *p, const char *t) { Heard h; lang_parse(t, &h); lang_apply(p, &h); }

/* the old formula, written out here on purpose: the chance of each of the 4 moods for a mood value */
static void old_odds(const Persona *ps, double out[4]) {
    int base = ps->mood + (int)((lang_charisma() - 50) * 0.6f) + (int)((ps->warmth - 50) * 0.5f) - (int)(ps->toxic * 0.5f);
    out[0] = out[1] = out[2] = out[3] = 0;
    for (int n = -7; n <= 7; n++) {
        int s = base + n, t = s >= 20 ? 0 : s >= -25 ? 1 : s >= -62 ? 2 : 3;
        out[t] += 1.0 / 15;
    }
}

static void test_old_behaviour(void) {
    Persona who[2] = { { "Dea", STYLE_DIVINE, 0, 30, 65, 8 }, { "Alex", STYLE_STREET, 10, 70, 5, 12 } };
    const int cha[2] = { 50, 20 };
    lang_seed(12345);
    for (int c = 0; c < 2; c++) {
        lang_add_charisma(cha[c] - lang_charisma());
        for (int w = 0; w < 2; w++)
            for (int mood = -100; mood <= 100; mood += 10) {
                Persona p = who[w]; p.mood = mood;
                double odds[4]; old_odds(&p, odds);
                int got[4] = {0}, N = 4000;
                for (int i = 0; i < N; i++) { int t = (int)lang_mood_now(&p); if (t < 0 || t > 3) { got[0] = -1000000; break; } got[t]++; }
                for (int t = 0; t < 4; t++) {
                    double e = odds[t], o = (double)got[t] / N, tol = 4.5 * sqrt(e * (1 - e) / N) + 0.004;
                    CHECK(fabs(o - e) <= tol, "%s cha %d mood %d: mood %d came %.3f of the time, the old formula says %.3f", p.name, cha[c], mood, t, o, e);
                }
            }
    }
    lang_add_charisma(50 - lang_charisma());
    /* an old-intent chat: nothing sticks, only old moods, whatever is said */
    static const char *const SAY[] = { "hello", "you are stupid", "who are you", "i am sorry", "thank you", "i love you", "i am scared", "tell me a joke", "i do not care",
                                       "where am i", "you are a goddess", "help me please", "shut up", "what is the sky", "bye" };
    for (int w = 0; w < 2; w++) {
        Persona p = who[w];
        for (int i = 0; i < 400; i++) {
            Heard h; char out[600]; lang_parse(SAY[(i * 7 + i / 15) % 15], &h);
            lang_reply(&p, &h, out, sizeof out);
            CHECK(p.hold == 0 && (int)lang_mood_now(&p) < 4, "%s: an old chat set a mood that sticks (hold %d, held %d) at turn %d", p.name, p.hold, p.held, i);
            if (p.hold) break;
        }
    }
}

static Tone settle(Persona *p) { turn(p); return lang_mood_now(p); }

static void test_nudges(void) {
    struct { const char *what; Persona p; Tone want; } T[] = {
        { "fear 50",           { "X", STYLE_PLAIN, 0, 50, 0, 8, .fear = 50 },          TONE_WORRIED },
        { "fear 80",           { "X", STYLE_PLAIN, 0, 50, 0, 8, .fear = 80 },          TONE_AFRAID },
        { "amuse 60",          { "X", STYLE_PLAIN, 0, 50, 0, 8, .amuse = 60 },         TONE_PLAYFUL },
        { "energy -60, calm",  { "X", STYLE_PLAIN, 0, 50, 0, 8, .energy = -60 },       TONE_BORED },
        { "energy -60, cross", { "X", STYLE_PLAIN, -50, 50, 0, 8, .energy = -60 },     TONE_WEARY },
        { "energy 50",         { "X", STYLE_PLAIN, 0, 50, 0, 8, .energy = 50 },        TONE_EXCITED },
        { "energy 80 + fear",  { "X", STYLE_PLAIN, 0, 50, 0, 8, .energy = 80, .fear = 30 }, TONE_MANIC },
        { "grief 50",          { "X", STYLE_PLAIN, 0, 50, 0, 8, .grief = 50 },         TONE_SOLEMN },
        { "grief 80",          { "X", STYLE_PLAIN, 0, 50, 0, 8, .grief = 80 },         TONE_HOLLOW },
        { "trust -50",         { "X", STYLE_PLAIN, 0, 50, 0, 8, .trust = -50 },        TONE_SUSPICIOUS },
        { "trust 50, warm",    { "X", STYLE_PLAIN, 60, 50, 0, 8, .trust = 50 },        TONE_TENDER },
    };
    for (unsigned i = 0; i < sizeof T / sizeof *T; i++) {
        Tone got = settle(&T[i].p);
        CHECK(got == T[i].want, "%s: got %s, want %s", T[i].what, lang_tone_name(got), lang_tone_name(T[i].want));
    }
    /* where the base mood blocks a push */
    { Persona p = { "X", STYLE_PLAIN, -100, 50, 0, 8, .trust = -60 }; Tone t = settle(&p); CHECK(t == TONE_HOSTILE || (int)t < 4, "a hostile character turned suspicious (%s)", lang_tone_name(t)); }
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .trust = 60 };    Tone t = settle(&p); CHECK((int)t < 4, "trust made a neutral character tender (%s)", lang_tone_name(t)); }
    /* too weak a push does nothing, a lean makes it count */
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .fear = 33 };                      CHECK((int)settle(&p) < 4, "fear 33 moved a character with no lean"); }
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .fear = 33, .lean = LEAN(TONE_WORRIED) }; Tone t = settle(&p); CHECK(t == TONE_WORRIED, "fear 33 with a lean for WORRIED gave %s", lang_tone_name(t)); }
    /* the strongest push wins */
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .fear = 45, .grief = 90 };           Tone t = settle(&p); CHECK(t == TONE_HOLLOW, "grief 90 against fear 45 gave %s", lang_tone_name(t)); }
    /* the hard drop: an insult makes a sulker sulk, and only a sulker */
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .lean = LEAN(TONE_SULKY) }; say(&p, "you are stupid"); CHECK(lang_mood_now(&p) == TONE_SULKY, "an insult did not make a character with a lean for SULKY sulk (%s)", lang_tone_name(lang_mood_now(&p))); }
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8 };                          say(&p, "you are stupid"); CHECK((int)lang_mood_now(&p) < 4, "an insult made a character with no lean sulk"); }
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .lean = LEAN(TONE_SULKY) }; say(&p, "hello"); CHECK((int)lang_mood_now(&p) < 4, "a greeting made a sulker sulk"); }
}

static void test_sticking(void) {
    int seen[8] = {0};
    for (unsigned seed = 1; seed <= 200; seed++) {
        lang_seed(seed);
        Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .fear = 80 };
        turn(&p);
        CHECK(lang_mood_now(&p) == TONE_AFRAID, "no AFRAID at the first turn");
        p.fear = 0;                                       /* the push is over: how many turns does it last? */
        int turns = 1;
        for (; turns < 10; turns++) { turn(&p); if (lang_mood_now(&p) != TONE_AFRAID) break; }
        CHECK(turns >= 2 && turns <= 4, "seed %u: AFRAID lasted %d turns, want 2-4", seed, turns);
        if (turns < 8) seen[turns]++;
    }
    CHECK(seen[2] && seen[3] && seen[4], "the sticking time is not random over 2, 3 and 4 (%d %d %d)", seen[2], seen[3], seen[4]);
    lang_seed(7);
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .grief = 80 }; for (int i = 0; i < 40; i++) { turn(&p); CHECK(lang_mood_now(&p) == TONE_HOLLOW, "flicker: turn %d gave %s while the grief lasts", i, lang_tone_name(lang_mood_now(&p))); } }
    /* held means held: the plain mood is not asked while it sticks, so the random draw does not flicker it either */
    { Persona p = { "X", STYLE_PLAIN, 0, 50, 0, 8, .amuse = 90 }; turn(&p); for (int i = 0; i < 50; i++) CHECK(lang_mood_now(&p) == TONE_PLAYFUL, "held mood changed between calls"); }
    /* a lean drifts now and then, never to a rude mood from a warm base, never to a nice one from a hostile base */
    lang_seed(99);
    { Persona p = { "X", STYLE_PLAIN, 60, 50, 0, 8, .lean = LEAN(TONE_PLAYFUL) | LEAN(TONE_SMUG) | LEAN(TONE_SARCASTIC) }; int playful = 0, rude = 0;
      for (int i = 0; i < 600; i++) { turn(&p); Tone t = lang_mood_now(&p); if (t == TONE_PLAYFUL) playful++; if ((RUDE >> t) & 1u && (int)t >= 4) rude++; }
      CHECK(playful > 40 && playful < 450, "a lean for PLAYFUL gave %d of 600 turns", playful);
      CHECK(rude == 0, "a warm character drifted to a rude mood %d times", rude); }
    { Persona p = { "X", STYLE_PLAIN, -100, 50, 0, 8, .lean = LEAN(TONE_PLAYFUL) | LEAN(TONE_SMUG) }; int nice = 0, smug = 0;
      for (int i = 0; i < 600; i++) { turn(&p); Tone t = lang_mood_now(&p); if (((NICE >> t) & 1u) && (int)t >= 4) nice++; if (t == TONE_SMUG) smug++; }
      CHECK(nice == 0, "a hostile character drifted to a nice mood %d times", nice);
      CHECK(smug > 40, "a hostile character with a lean for SMUG did not drift to it (%d)", smug); }
}

static void test_answers(void) {
    static const char *const KEYS[] = { "r.greet", "r.bye", "r.insult", "r.sad", "r.unknown", "p.pre", "p.post", "p.sigh" };
    Persona p = { "X", STYLE_DIVINE, 0, 50, 0, 8 };
    char out[200];
    for (int t = 0; t < TONE_COUNT; t++) for (unsigned k = 0; k < sizeof KEYS / sizeof *KEYS; k++)
        CHECK(lang_line_tone(&p, KEYS[k], (Tone)t, out, sizeof out) && out[0], "%s has no %s line", lang_tone_name(t), KEYS[k]);
    /* a full reply with a held mood works and says something */
    for (int t = 0; t < TONE_COUNT; t++) {
        Persona q = { "X", STYLE_STREET, 0, 50, 0, 8 }; q.hold = 3; q.held = (int8_t)t;
        Heard h; lang_parse("hello, who are you?", &h);
        CHECK(lang_reply(&q, &h, out, sizeof out) >= 1 && out[0], "%s gave no reply", lang_tone_name(t));
    }
}

int main(void) {
    lang_seed(2026);
    test_old_behaviour();
    test_nudges();
    test_sticking();
    test_answers();
    printf(bad ? "moodtest: %d PROBLEMS\n" : "moodtest: ok\n", bad);
    return bad ? 1 : 0;
}
