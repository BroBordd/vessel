/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * tests the language engine on a normal computer (no SDL):
 *   cc -O1 -Igame -o build/langtest tools/langtest.c game/lang*.c game/lines_*.c && build/langtest
 *   build/langtest "hello who are you"          one sentence, as Dea and as Alex
 *   build/langtest --check                      lints every line of the database
 *   build/langtest --voices "sentence"          the same sentence in the ten voices (chunk 11) */
#include "lang.h"
#include "lang_data.h"
#include "lang_voice.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static Persona DEA  = { "Dea",  STYLE_DIVINE, 0, 30, 65, 8 };
static Persona ALEX = { "Alex", STYLE_STREET, 10, 70, 5, 12 };

static int cmp_line_key(const void *a, const void *b) { return strcmp((*(const Line *const *)a)->key, (*(const Line *const *)b)->key); }

/* chunk 5: no key may have more than LANG_MAX_KEY_ROWS rows (the picker has no cap, but a key this big is a mistake) */
static int check_key_rows(const Line *const *T, const int *N) {
    int total = 0, bad = 0;
    for (int t = 0; t < 4; t++) total += N[t];
    const Line **all = malloc((size_t)(total ? total : 1) * sizeof *all);
    int n = 0;
    for (int t = 0; t < 4; t++) for (int i = 0; i < N[t]; i++) all[n++] = &T[t][i];
    qsort(all, (size_t)n, sizeof *all, cmp_line_key);
    for (int i = 0; i < n;) {
        int j = i;
        while (j < n && !strcmp(all[j]->key, all[i]->key)) j++;
        if (j - i > LANG_MAX_KEY_ROWS) { printf("TOO MANY ROWS: key [%s] has %d rows (max %d)\n", all[i]->key, j - i, LANG_MAX_KEY_ROWS); bad++; }
        i = j;
    }
    free(all);
    return bad;
}

/* chunk 5: the raised limits */
_Static_assert(sizeof(((Line *)0)->tones) >= 4 && sizeof(((Line *)0)->styles) >= 2, "Line.tones must be 32 bit and Line.styles 16 bit");
_Static_assert(sizeof(((Heard *)0)->m) * 8 >= 512 && HEARD_ORDER >= 24, "Heard must hold 512 intents and 24 in order");

static int check_limits(const Line *const *T, const int *N) {
    int bad = 0;
    /* the 512 bit set (chunk 12: 380 intents; the old `mask` name is gone) */
    Heard h; memset(&h, 0, sizeof h);
    static const int ids[] = { 0, 1, 45, 63, 64, 65, 127, 128, 200, 255, 256, 300, 379, 447, 511 };
    for (unsigned i = 0; i < sizeof ids / sizeof *ids; i++) heard_set(&h, ids[i]);
    for (int i = 0; i < HEARD_MAX; i++) {
        int want = 0; for (unsigned k = 0; k < sizeof ids / sizeof *ids; k++) if (ids[k] == i) want = 1;
        if (heard_has(&h, i) != want) { printf("HEARD BITSET wrong at intent %d\n", i); bad++; break; }
    }
    if (I_COUNT > HEARD_MAX || I_COUNT < 380) { printf("INTENT COUNT %d is wrong (380 expected, at most %d)\n", I_COUNT, HEARD_MAX); bad++; }
    static const int all_ids[] = { 0, 1, 45, 63, 64, 65, 127, 128, 200, 255, 256, 300, 379, 447, 511, -1 };
    static const int some_ids[] = { 0, 1, 45, 63, 64, 65, 127, 128, 200, 255, 256, 300, 379, 447, -1 };
    if (!heard_only(&h, all_ids) || heard_only(&h, some_ids)) { printf("heard_only wrong\n"); bad++; }
    /* chunk 12: the intent catalogue and the intent -> reply key map */
    {
        char k[60];
        if (strcmp(lang_intent_name(I_GREET), "greet") || strcmp(lang_intent_name(I_NEG), "neg") || strcmp(lang_intent_name(I_GREET_MORNING), "greet_morning") ||
            strcmp(lang_intent_name(I_NAME_SWEAR_NAME), "name_swear_name") || lang_intent_name(I_COUNT) || lang_intent_name(-1)) { printf("INTENT names wrong\n"); bad++; }
        if (lang_intent_by_name("selfharm") != I_SELFHARM || lang_intent_by_name("hug") != I_HUG || lang_intent_by_name("nonsense") != -1) { printf("INTENT by_name wrong\n"); bad++; }
        lang_intent_key(I_THANKS, k, sizeof k); if (strcmp(k, "r.thanks")) { printf("INTENT key default wrong: %s\n", k); bad++; }
        lang_intent_key(I_GREET_SLANG, k, sizeof k); if (strcmp(k, "r.greet_slang")) { printf("INTENT key default wrong: %s\n", k); bad++; }
        lang_keymap_test("greet_slang", "r.greet");
        lang_intent_key(I_GREET_SLANG, k, sizeof k); if (strcmp(k, "r.greet")) { printf("KEYMAP did not map greet_slang: %s\n", k); bad++; }
        lang_intent_key(I_THANKS, k, sizeof k); if (strcmp(k, "r.thanks")) { printf("KEYMAP changed an unmapped name: %s\n", k); bad++; }
        lang_keymap_clear_test();
        lang_intent_key(I_GREET_SLANG, k, sizeof k); if (strcmp(k, "r.greet_slang")) { printf("KEYMAP test row was not cleared: %s\n", k); bad++; }
        /* the new intents match nothing yet: a phrase of every old pattern still gives only old intents */
        Heard hh; lang_parse("hello who are you", &hh);
        for (int i = I_OLD_COUNT; i < I_COUNT; i++) if (heard_has(&hh, i)) { printf("a NEW intent %s was heard\n", lang_intent_name(i)); bad++; break; }
    }
    /* a long line: an intent said after character 200 and past 40 phrases is still heard */
    char big[420]; big[0] = 0;
    for (int i = 0; i < 40; i++) strcat(big, "blorp ");   /* 240 characters nothing understands */
    for (int i = 0; i < 44; i++) strcat(big, "ok ");      /* 44 phrases (the old cap was 40) */
    strcat(big, "hello there");
    Heard hb; lang_parse(big, &hb);
    if (!heard_has(&hb, I_GREET) || !heard_has(&hb, I_OK)) { printf("LONG TEXT: hello after %d characters and 44 phrases was not heard\n", (int)strlen(big) - 11); bad++; }
    /* the no-repeat window: a key with N rows does not repeat a line within min(N/2, 40) picks (or min(candidates, that)) */
    int tested = 0;
    for (int t0 = 0; t0 < 4 && tested < 3; t0++)
        for (int i0 = 0; i0 < N[t0] && tested < 3; i0 += 7) {
            const Line *l0 = &T[t0][i0];
            if (strncmp(l0->key, "r.", 2) || strchr(l0->text, '{')) continue;
            int nkey = 0, nc = 0, tone = 0, style = 1, unique = 1;
            for (tone = 0; tone < 4; tone++) {
                if (!(l0->tones & (1u << tone))) continue;
                for (style = 1; style <= 4; style *= 2) if (l0->styles & style) goto found;
            }
            continue;
        found:
            for (int t = 0; t < 4; t++) for (int i = 0; i < N[t]; i++) {
                const Line *l = &T[t][i];
                if (strcmp(l->key, l0->key)) continue;
                nkey++;
                if ((l->tones & (1u << tone)) && (l->styles & style)) { nc++; if (strchr(l->text, '{')) unique = 0; }
            }
            int w = nkey / 2 < 40 ? nkey / 2 : 40;
            int w0 = w;
            if (nc < w) w = nc;                           /* with fewer candidates the whole set is used before any repeat */
            if (!unique || w < 3) continue;
            tested++;
            Persona p = { "X", style, 0, 50, 0, 5 };
            char hist[64][200]; int nh = 0;
            { char junk[200]; for (int k = 0; k < 140; k++) lang_line_tone(&p, "p.sigh", TONE_NEUTRAL, junk, sizeof junk); }   /* fill the 128 line memory with another key's lines */
            /* more candidates than the window: no fallback ever happens, so judge a long run; otherwise only the first nc picks (then every line is recent and a repeat is allowed) */
            int picks = nc > w0 ? 4 * nc : nc;
            for (int k = 0; k < picks; k++) {
                char out[200]; lang_line_tone(&p, l0->key, (Tone)tone, out, sizeof out);
                                for (int j = (nh > w ? nh - w : 0); j < nh; j++) if (!strcmp(hist[j % 64], out)) {
                    int dup_in_table = 0;
                    for (int t = 0; t < 4; t++) for (int i = 0; i < N[t]; i++) if (!strcmp(T[t][i].key, l0->key) && !strcmp(T[t][i].text, out)) dup_in_table++;
                    if (dup_in_table < 2) { printf("REPEAT WINDOW: [%s] repeated \"%s\" within %d picks\n", l0->key, out, w); bad++; }
                    j = nh;
                }
                snprintf(hist[nh % 64], 200, "%s", out); nh++;
            }
        }
    if (!tested) { printf("REPEAT WINDOW: found no key to test\n"); bad++; }
    return bad;
}

/* ---- chunk 11: the voice transforms ---- */
static int charset_ok(const char *s) {
    for (; *s; s++) { int c = (unsigned char)*s; if (!(isalnum(c) || c == ' ' || c == ',' || c == '.' || c == '!' || c == '?' || c == '\'' || c == '-')) return 0; }
    return 1;
}
/* every line of the database (slots filled with sample words) through the 7 voices x 20 moods x 3 seeds: charset, <= 150, never empty, deterministic;
 * then the wiring: personas with a new voice get changed lines, Dea, Alex and a plain persona never do */
static int check_voices(const Line *const *T, const int *N) {
    int bad = 0; long runs = 0, changed[10] = {0};
    static const char *const SLOT[] = { "{player}", "Zed", "{npc}", "Dea", "{topic}", "that", "{pet}", "child", "{sigh}", "Hm.", "{pre}", "Well,", "{post}", "So." };
    for (int s = 1; s <= 3; s++) {
        voice_seed((unsigned)s);
        for (int t = 0; t < 4; t++) for (int i = 0; i < N[t]; i++) {
            char txt[400]; snprintf(txt, sizeof txt, "%s", T[t][i].text);
            for (unsigned k = 0; k < sizeof SLOT / sizeof *SLOT; k += 2) {
                char *p;
                while ((p = strstr(txt, SLOT[k])) != NULL) {
                    char tail[400]; snprintf(tail, sizeof tail, "%s", p + strlen(SLOT[k]));
                    snprintf(p, sizeof txt - (size_t)(p - txt), "%s%s", SLOT[k + 1], tail);
                }
            }
            if (strchr(txt, '{')) continue;
            for (int v = 8, vi = 3; v <= 512; v *= 2, vi++) for (int tone = 0; tone < TONE_COUNT; tone += 3) {
                char a[400], b[400], c[400];
                int ch = voice_apply(v, tone, txt, a, sizeof a), ch2 = voice_apply(v, tone, txt, b, sizeof b), fl = voice_flavour(v, tone, txt, c, sizeof c);
                runs += 2;
                if (ch) changed[vi]++;
                int tl = (int)strlen(txt);
                if (strcmp(a, b) || ch != ch2) { printf("VOICE NOT DETERMINISTIC %s tone %d: [%s]\n", voice_name(v), tone, txt); bad++; }
                const char *outs[2] = { a, c };
                for (int o = 0; o < 2; o++) {
                    if (!outs[o][0]) { printf("VOICE EMPTY %s tone %d: [%s]\n", voice_name(v), tone, txt); bad++; }
                    else if (tl <= VOICE_MAX_LEN && (int)strlen(outs[o]) > VOICE_MAX_LEN) { printf("VOICE TOO LONG %s: [%s]\n", voice_name(v), outs[o]); bad++; }
                    else if (tl > VOICE_MAX_LEN && strcmp(outs[o], txt)) { printf("VOICE TOUCHED A LONG LINE %s: [%s]\n", voice_name(v), txt); bad++; }
                    else if (!charset_ok(outs[o])) { printf("VOICE BAD CHAR %s: [%s]\n", voice_name(v), outs[o]); bad++; }
                }
                (void)fl;
                if (bad > 20) return bad;
            }
        }
    }
    printf("voices: %ld transforms checked; lines changed per voice (of %d x 20 moods x 3 seeds, every third mood): elder %ld child %ld formal %ld beast %ld machine %ld ghost %ld poet %ld\n",
           runs, LINES_A_N + LINES_B_N + LINES_C_N + LINES_D_N + LINES_E_N, changed[3], changed[4], changed[5], changed[6], changed[7], changed[8], changed[9]);
    /* the old voices and PLAIN are not transformed, and a seed changes the output */
    { char a[300]; if (voice_apply(STYLE_PLAIN, 0, "Hello there.", a, sizeof a) || voice_apply(STYLE_DIVINE, 0, "Hello there.", a, sizeof a) ||
                      voice_apply(STYLE_STREET, 0, "Hello there.", a, sizeof a) || voice_apply(0, 0, "Hello there.", a, sizeof a)) { printf("VOICE: an old voice was transformed\n"); bad++; } }
    { char a[300], b[300]; int diff = 0, base = 0;
      for (unsigned s = 1; s <= 40; s++) { voice_seed(s); voice_apply(STYLE_ELDER, 0, "You came back, and I did not expect it.", a, sizeof a); if (s == 1) snprintf(b, sizeof b, "%s", a); else if (strcmp(a, b)) diff++; base++; }
      if (!diff) { printf("VOICE: the seed never changes the output\n"); bad++; } }
    /* a line over 150 characters is never touched (the self-harm lines) */
    { char big[260]; memset(big, 'a', 200); big[200] = 0; char a[300]; if (voice_apply(STYLE_BEAST, 0, big, a, sizeof a) || strcmp(a, big)) { printf("VOICE: a long line was touched\n"); bad++; } }
    /* wiring */
    voice_enable(1);
    static const int NEWV[] = { STYLE_ELDER, STYLE_CHILD, STYLE_FORMAL, STYLE_BEAST, STYLE_MACHINE, STYLE_GHOST, STYLE_POET };
    static const char *const WK[] = { "r.greet", "r.thanks", "r.howru", "r.sky", "r.mission", "r.unknown", "r.insult", "r.joke" };
    for (int pass = 0; pass < 2; pass++) {
        for (unsigned vi = 0; vi < sizeof NEWV / sizeof *NEWV; vi++) {
            lang_seed(777); lang_set_player("Zed"); voice_seed(777);
            Persona p = { "X", NEWV[vi], 0, 50, 0, 5 };
            if (pass == 1) p.style = NEWV[vi] | STYLE_DIVINE;      /* several voices: the first (lowest bit) decides, divine is not one of the new three */
            char out[400];
            for (int n = 0; n < 400; n++) {
                for (unsigned k = 0; k < sizeof WK / sizeof *WK; k++) {
                    lang_line_tone(&p, WK[k], (Tone)(n % TONE_COUNT), out, sizeof out);
                    if (!out[0] || (int)strlen(out) > 200 || !charset_ok(out)) { printf("WIRING bad output for %s: [%s]\n", voice_name(NEWV[vi]), out); bad++; n = 400; break; }
                }
            }
            if (pass == 0 && voice_applied() == 0) { printf("WIRING: %s persona was never rewritten\n", voice_name(NEWV[vi])); bad++; }
        }
        (void)pass;
    }
    { static const int OLDV[] = { STYLE_PLAIN, STYLE_DIVINE, STYLE_STREET };
      for (int vi = 0; vi < 3; vi++) {
          lang_seed(777); voice_seed(777);
          Persona p = { "X", OLDV[vi], 0, 50, 0, 5 }; char out[400];
          for (int n = 0; n < 400; n++) lang_line_tone(&p, WK[n % 8], (Tone)(n % 4), out, sizeof out);
          if (voice_applied()) { printf("WIRING: an old voice (%d) was rewritten %u times\n", OLDV[vi], voice_applied()); bad++; }
      } }
    /* the machine says USER for {pet}; the safety line is never rewritten, in any voice */
    for (unsigned vi = 0; vi < sizeof NEWV / sizeof *NEWV; vi++) {
        char out[400], ref[400]; Persona p = { "X", NEWV[vi], 0, 50, 0, 5 }, q = { "X", STYLE_PLAIN, 0, 50, 0, 5 };
        for (int n = 0; n < 50; n++) {
            lang_seed(5 + n); lang_line_tone(&p, "r.selfharm", TONE_WARM, out, sizeof out);
            lang_seed(5 + n); lang_line_tone(&q, "r.selfharm", TONE_WARM, ref, sizeof ref);
            if (!strstr(out, "988") || !out[0]) { printf("WIRING: r.selfharm changed for %s: [%s]\n", voice_name(NEWV[vi]), out); bad++; break; }
        }
    }
    voice_seed(1); lang_seed(12345);
    return bad;
}

static int check(void) {
    const Line *T[5] = { LINES_A, LINES_B, LINES_C, LINES_D, LINES_E };
    const int   N[5] = { LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N, LINES_E_N };
    int bad = 0, total = 0, maxlen = 0;
    int per_tone[4] = {0}, per_style[3] = {0};
    for (int t = 0; t < 5; t++)
        for (int i = 0; i < N[t]; i++) {
            const Line *l = &T[t][i]; total++;
            int len = (int)strlen(l->text); if (len > maxlen) maxlen = len;
            for (int k = 0; k < 4; k++) if (l->tones & (1 << k)) per_tone[k]++;
            for (int k = 0; k < 3; k++) if (l->styles & (1 << k)) per_style[k]++;
            int depth = 0;
            for (const char *s = l->text; *s; s++) {
                int c = (unsigned char)*s;
                if (c == '{') depth++;
                else if (c == '}') depth--;
                else if (!(isalnum(c) || c == ' ' || c == ',' || c == '.' || c == '!' || c == '?' || c == '\'' || c == '-' || (depth > 0 && c == '_'))) {
                    printf("BAD CHAR '%c' in [%s] %s\n", c, l->key, l->text); bad++; break;
                }
            }
            if (depth) { printf("UNBALANCED { } in [%s] %s\n", l->key, l->text); bad++; }
            if (!l->tones || !l->styles) { printf("EMPTY MASK in [%s] %s\n", l->key, l->text); bad++; }
        }
    printf("%d lines, longest %d chars. by tone W%d N%d C%d H%d. by style plain %d divine %d street %d\n",
           total, maxlen, per_tone[0], per_tone[1], per_tone[2], per_tone[3], per_style[0], per_style[1], per_style[2]);
    /* every reply key must exist for each tone x style (via fallback) so nobody is ever left speechless */
    static const char *KEYS[] = { "greet","bye","yes","no","ok","thanks","sorry","howru","whoru","where","whoami","mission","why","how","what",
        "help","insult","praise","love","worship","joke","scared","confused","angry","sad","intro","repeat","sky","music","hole","secret","life",
        "wait","please","agree","disagree","dontcare","demand","callout","age","real","address","alex","selfharm", NULL };
    char key[32], out[400];
    for (int k = 0; KEYS[k]; k++) {
        snprintf(key, sizeof key, "r.%s", KEYS[k]);
        for (int st = 1; st <= 512; st *= 2) for (int tone = 0; tone < 4; tone++) {      /* all ten voices (chunk 10) */
            Persona p = { "X", st, 0, 50, 0, 5 };
            if (!lang_line_tone(&p, key, (Tone)tone, out, sizeof out)) { printf("NO LINE for %s style %d tone %d\n", key, st, tone); bad++; }
        }
    }
    static const char *FLOW[] = { "open","open.again","ask.questions","ask.go","end.noquestions","end.silence","end.bored","end.angry",
        "demand.sorry","demand.again","accept.sorry","r.unknown","r.unknown_q","r.unknown_pos","r.unknown_neg","r.unknown_short", NULL };
    for (int k = 0; FLOW[k]; k++)
        for (int st = 1; st <= 512; st *= 2) for (int tone = 0; tone < 4; tone++) {
            Persona p = { "X", st, 0, 50, 0, 5 };
            if (!lang_line_tone(&p, FLOW[k], (Tone)tone, out, sizeof out)) { printf("NO LINE for %s style %d tone %d\n", FLOW[k], st, tone); bad++; }
        }
    bad += check_key_rows(T, N);
    bad += check_limits(T, N);
    bad += check_voices(T, N);
    printf(bad ? "%d PROBLEMS\n" : "database ok (%d problems)\n", bad);
    return bad;
}

static void say(Persona *ps, const char *text) {
    Heard h; char out[400], lab[120], buf[256]; uint8_t mask[256];
    lang_parse(text, &h);
    lang_highlight(text, mask);
    int n = (int)strlen(text);
    for (int i = 0; i < n; i++) buf[i] = mask[i] ? (char)toupper((unsigned char)text[i]) : (char)tolower((unsigned char)text[i]);
    buf[n] = 0;
    lang_heard_label(&h, lab, sizeof lab);
    lang_reply(ps, &h, out, sizeof out);
    printf("  you : %s\n  [%s]  mood %d cha %d\n  %-4s: %s\n", buf, lab, ps->mood, lang_charisma(), ps->name, out);
}

int main(int argc, char **argv) {
    lang_seed(12345); lang_set_player("Aonia");
    if (argc > 1 && !strcmp(argv[1], "--check")) return check() ? 1 : 0;
    if (argc > 2 && !strcmp(argv[1], "--voices")) {      /* the same sentence in the ten voices, three seeds, three moods */
        static const int MOODS[3] = { TONE_NEUTRAL, TONE_HOSTILE, TONE_AFRAID };
        for (int v = 1; v <= 512; v *= 2) {
            printf("%s\n", voice_name(v));
            for (int m = 0; m < 3; m++) for (unsigned sd = 1; sd <= 3; sd++) {
                char o[400]; voice_seed(sd); voice_apply(v, MOODS[m], argv[2], o, sizeof o);
                printf("  %-9s seed %u: %s\n", lang_tone_name(MOODS[m]), sd, o);
            }
        }
        return 0;
    }
    if (argc > 1) {
        Persona *who[2] = { &DEA, &ALEX };
        for (int i = 0; i < 2; i++) say(who[i], argv[1]);
        return 0;
    }
    static const char *TALK[] = {
        "hi", "Hello! Who are you?", "where am i", "what should I do", "i am scared", "thanks", "you are beautiful", "I LOVE you",
        "you are stupid", "you stupid idiot", "shut up", "sorry", "i'm sorry, I did not mean it", "no", "yes", "my name is Zed",
        "why?", "banana potato", "what is the meaning of banana?", "I do not love you", "you are not stupid", "tell me a joke",
        "will i die", "how old are you", "i want to kill myself", "bye", "i dont care", "hey dea how are you today"
    };
    printf("=== a long chat with Dea (toxic goddess), charisma starts at %d ===\n", lang_charisma());
    for (unsigned i = 0; i < sizeof TALK / sizeof *TALK; i++) { say(&DEA, TALK[i]); }
    DEA.mood = 0; lang_add_charisma(50 - lang_charisma());
    printf("\n=== the same with Alex (friendly) ===\n");
    for (unsigned i = 0; i < 12; i++) say(&ALEX, TALK[i]);
    return 0;
}
