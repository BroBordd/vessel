/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * tests the language engine on a normal computer (no SDL):
 *   cc -O1 -Igame -o build/langtest tools/langtest.c game/lang.c game/lang_pats.c game/lines_*.c && build/langtest
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
        for (int st = 1; st <= 4; st *= 2) for (int tone = 0; tone < 4; tone++) {
            Persona p = { "X", st, 0, 50, 0, 5 };
            if (!lang_line_tone(&p, key, (Tone)tone, out, sizeof out)) { printf("NO LINE for %s style %d tone %d\n", key, st, tone); bad++; }
        }
    }
    static const char *FLOW[] = { "open","open.again","ask.questions","ask.go","end.noquestions","end.silence","end.bored","end.angry",
        "demand.sorry","demand.again","accept.sorry","r.unknown","r.unknown_q","r.unknown_pos","r.unknown_neg","r.unknown_short", NULL };
    for (int k = 0; FLOW[k]; k++)
        for (int st = 1; st <= 4; st *= 2) for (int tone = 0; tone < 4; tone++) {
            Persona p = { "X", st, 0, 50, 0, 5 };
            if (!lang_line_tone(&p, FLOW[k], (Tone)tone, out, sizeof out)) { printf("NO LINE for %s style %d tone %d\n", FLOW[k], st, tone); bad++; }
        }
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
