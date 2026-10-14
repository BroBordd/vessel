/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * tests the language engine on a normal computer (no SDL):
 *   cc -O1 -Igame -o build/langtest tools/langtest.c game/lang*.c game/lines_*.c && build/langtest
 *   build/langtest "hello who are you"          one sentence, as Dea and as Alex
 *   build/langtest --check                      lints every line of the database */
#include "lang.h"
#include "lang_data.h"
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
_Static_assert(sizeof(((Heard *)0)->m) * 8 >= 256 && HEARD_ORDER >= 24, "Heard must hold 256 intents and 24 in order");

static int check_limits(const Line *const *T, const int *N) {
    int bad = 0;
    /* the 256 bit set, and the old `mask` name still reads word 0 */
    Heard h; memset(&h, 0, sizeof h);
    static const int ids[] = { 0, 1, 45, 63, 64, 65, 127, 128, 200, 255 };
    for (unsigned i = 0; i < sizeof ids / sizeof *ids; i++) heard_set(&h, ids[i]);
    for (int i = 0; i < HEARD_MAX; i++) {
        int want = 0; for (unsigned k = 0; k < sizeof ids / sizeof *ids; k++) if (ids[k] == i) want = 1;
        if (heard_has(&h, i) != want) { printf("HEARD BITSET wrong at intent %d\n", i); bad++; break; }
    }
    if (h.mask != h.m[0] || !(h.mask & 1u)) { printf("HEARD mask alias is not m[0]\n"); bad++; }
    static const int all_ids[] = { 0, 1, 45, 63, 64, 65, 127, 128, 200, 255, -1 };
    static const int some_ids[] = { 0, 1, 45, 63, 64, 65, 127, 128, 200, -1 };
    if (!heard_only(&h, all_ids) || heard_only(&h, some_ids)) { printf("heard_only wrong\n"); bad++; }
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

static int check(void) {
    const Line *T[4] = { LINES_A, LINES_B, LINES_C, LINES_D };
    const int   N[4] = { LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N };
    int bad = 0, total = 0, maxlen = 0;
    int per_tone[4] = {0}, per_style[3] = {0};
    for (int t = 0; t < 4; t++)
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
