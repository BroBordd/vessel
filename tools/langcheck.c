/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the measuring tool of the language database (docs/convo_plan.md, chunk 2). no SDL:
 *   cc -O1 -Igame -o build/langcheck tools/langcheck.c game/lang*.c game/lines_*.c && build/langcheck --all
 *   --check            the lint: charset, braces, empty masks (the same rules as langtest --check)
 *   --stats            rows per key group, per tone, per style; patterns per intent
 *   --coverage [N]     key x tone x style matrix (lines per cell) and every r.* cell under N lines (default 4)
 *   --dupes            exact duplicate lines inside one key
 *   --length           length histogram, lines over 110 (warn) and 150 (fail later, chunk 156)
 *   --tables           (chunk 6) the registry and the indexes against a brute force walk of the tables: every key, every pattern, the word hashes
 *   --bench            (chunk 6) timings: init, 1,000 parses of 200 character lines, 1,000 replies, 10,000 line picks, and the budgets of chunk 159
 *   --moods            (chunk 7) the fallback chain of each of the 20 moods and, per key, which mood's lines answer for each mood (= own lines, a letter = reached through the chain, . = nothing). fails if a chain has no old mood in it or a key with lines is silent for a mood
 *   --all              all of the above. exit code 1 only if --check, --tables or --moods found a problem */
#include "lang.h"
#include "lang_data.h"
#include "lang_tables.h"
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NT 4
#define NS 3
static const Line *const TAB[4] = { LINES_A, LINES_B, LINES_C, LINES_D };
static const int *const CNT[4] = { &LINES_A_N, &LINES_B_N, &LINES_C_N, &LINES_D_N };
static const char *const TONE_N[NT] = { "warm", "neut", "cold", "host" };
static const char *const STYLE_N[NS] = { "plain", "divine", "street" };

/* same order as INTENT_NAME in lang.c (the array has no size, so the assert catches a drift in length) */
static const char *const INTENT_N[] = {
    "greet", "bye", "yes", "no", "ok", "thanks", "sorry", "howru", "whoru", "where",
    "whoami", "mission", "why", "how", "what", "help", "insult", "praise", "love", "worship",
    "joke", "scared", "confused", "angry", "sad", "intro", "repeat", "sky", "music", "hole",
    "secret", "life", "wait", "please", "agree", "disagree", "dontcare", "demand", "callout", "age",
    "real", "address", "selfharm", "alex", "pos", "neg"
};
_Static_assert(sizeof INTENT_N / sizeof *INTENT_N == I_COUNT, "INTENT_N in langcheck.c is out of date: see lang.c");

#define EACH(l) for (int t_ = 0; t_ < 4; t_++) for (int i_ = 0; i_ < *CNT[t_]; i_++) for (const Line *l = &TAB[t_][i_]; l; l = NULL)

/* ---- distinct keys ---- */
static const char *keys[400]; static int nkeys;
static int key_id(const char *k) {
    for (int i = 0; i < nkeys; i++) if (!strcmp(keys[i], k)) return i;
    if (nkeys < 400) keys[nkeys++] = k;
    return nkeys - 1;
}
static int cmpkey(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

static int cmp_line_key(const void *a, const void *b) { return strcmp((*(const Line *const *)a)->key, (*(const Line *const *)b)->key); }

static int do_check(void) {
    int bad = 0, total = 0, maxlen = 0;
    EACH(l) {
        total++;
        int len = (int)strlen(l->text); if (len > maxlen) maxlen = len;
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
    /* chunk 5: no key may have more than LANG_MAX_KEY_ROWS rows */
    {
        const Line **all = malloc((size_t)(total ? total : 1) * sizeof *all);
        int n = 0;
        EACH(l) all[n++] = l;
        qsort(all, (size_t)n, sizeof *all, cmp_line_key);
        for (int i = 0; i < n;) {
            int j = i;
            while (j < n && !strcmp(all[j]->key, all[i]->key)) j++;
            if (j - i > LANG_MAX_KEY_ROWS) { printf("TOO MANY ROWS: key [%s] has %d rows (max %d)\n", all[i]->key, j - i, LANG_MAX_KEY_ROWS); bad++; }
            i = j;
        }
        free(all);
    }
    printf("check: %d lines, longest %d chars, %d problems\n", total, maxlen, bad);
    return bad;
}

static void do_stats(void) {
    int total = 0, per_tone[NT] = {0}, per_style[NS] = {0};
    int ngroup = 0; char gname[40][16]; int gcount[40] = {0};
    EACH(l) {
        total++;
        for (int k = 0; k < NT; k++) if (l->tones & (1 << k)) per_tone[k]++;
        for (int k = 0; k < NS; k++) if (l->styles & (1 << k)) per_style[k]++;
        key_id(l->key);
        char g[16]; int n = 0;
        while (l->key[n] && l->key[n] != '.' && n < 15) { g[n] = l->key[n]; n++; }
        g[n] = 0;
        int f = -1; for (int i = 0; i < ngroup; i++) if (!strcmp(gname[i], g)) { f = i; break; }
        if (f < 0 && ngroup < 40) { f = ngroup++; strcpy(gname[f], g); }
        if (f >= 0) gcount[f]++;
    }
    printf("stats: %d lines, %d distinct keys\n", total, nkeys);
    printf("  by tone  :"); for (int k = 0; k < NT; k++) printf(" %s %d", TONE_N[k], per_tone[k]); printf("  (a row can cover several)\n");
    printf("  by style :"); for (int k = 0; k < NS; k++) printf(" %s %d", STYLE_N[k], per_style[k]); printf("\n");
    printf("  by key group:"); for (int i = 0; i < ngroup; i++) printf(" %s %d", gname[i], gcount[i]); printf("\n");
    int pi[I_COUNT] = {0}, bad = 0;
    for (int k = 0; k < PATS_N; k++) { if (PATS[k].intent >= 0 && PATS[k].intent < I_COUNT) pi[PATS[k].intent]++; else bad++; }
    printf("  patterns : %d rows%s\n   ", PATS_N, bad ? " (SOME HAVE A BAD INTENT)" : "");
    for (int i = 0; i < I_COUNT; i++) printf(" %s %d%s", INTENT_N[i], pi[i], i % 8 == 7 ? "\n   " : "");
    printf("\n");
    /* which intents have patterns but no reply lines, or the reverse */
    for (int i = 0; i < I_COUNT; i++) {
        char key[40]; snprintf(key, sizeof key, "r.%s", INTENT_N[i]);
        int n = 0; EACH(l) if (!strcmp(l->key, key)) n++;
        if (i != I_POS && i != I_NEG && !n) printf("  NOTE: intent %s has no reply lines\n", INTENT_N[i]);
        if (!pi[i]) printf("  NOTE: intent %s has no patterns\n", INTENT_N[i]);
    }
}

static void do_coverage(int quota) {
    printf("coverage: lines per key x tone x style (a row that covers several cells counts in each). quota %d per cell\n", quota);
    printf("%-18s", "key");
    for (int s = 0; s < NS; s++) for (int t = 0; t < NT; t++) printf(" %c%c", STYLE_N[s][0], TONE_N[t][0]);
    printf("  under\n");
    qsort(keys, (size_t)nkeys, sizeof *keys, cmpkey);
    int under_total = 0, cells = 0;
    for (int k = 0; k < nkeys; k++) {
        if (strncmp(keys[k], "r.", 2)) continue;
        int c[NS][NT] = {{0}};
        EACH(l) if (!strcmp(l->key, keys[k]))
            for (int s = 0; s < NS; s++) for (int t = 0; t < NT; t++) if ((l->styles & (1 << s)) && (l->tones & (1 << t))) c[s][t]++;
        int under = 0;
        printf("%-18s", keys[k]);
        for (int s = 0; s < NS; s++) for (int t = 0; t < NT; t++) { printf(" %2d", c[s][t]); cells++; if (c[s][t] < quota) under++; }
        printf("  %d\n", under);
        under_total += under;
    }
    printf("cells under %d lines: %d of %d (the plan's core grid wants >= 4 everywhere, chunks 63-126)\n", quota, under_total, cells);
}

static void do_dupes(void) {
    int dup = 0;
    for (int t = 0; t < 4; t++) for (int i = 0; i < *CNT[t]; i++) {
        const Line *a = &TAB[t][i];
        for (int u = t; u < 4; u++) for (int j = (u == t ? i + 1 : 0); j < *CNT[u]; j++) {
            const Line *b = &TAB[u][j];
            if (!strcmp(a->key, b->key) && !strcmp(a->text, b->text)) { printf("DUPE [%s] %s\n", a->key, a->text); dup++; }
        }
    }
    printf("dupes: %d exact duplicates inside a key\n", dup);
}

static void do_length(void) {
    int hist[16] = {0}, over110 = 0, over150 = 0, total = 0;
    EACH(l) {
        int len = (int)strlen(l->text); total++;
        int b = len / 20; if (b > 15) b = 15; hist[b]++;
        if (len > 110) over110++;
        if (len > 150) { over150++; printf("  LONG %d [%s] %s\n", len, l->key, l->text); }
    }
    printf("length: %d lines. histogram (chars):", total);
    for (int b = 0; b < 16; b++) if (hist[b]) printf(" %d-%d:%d", b * 20, b * 20 + 19, hist[b]);
    printf("\n  over 110: %d (warn)   over 150: %d (fails from chunk 156)\n", over110, over150);
}


/* ---- chunk 6: the registry and the indexes ---- */
static int tfail;
#define TCHECK(c, ...) do { if (!(c)) { printf("  TABLES FAIL: " __VA_ARGS__); printf("\n"); tfail++; } } while (0)

static const Line T6_LINES[] = { { "t6.key", ALLT, ALLS, "one" }, { "t6.key", ALLT, ALLS, "two" }, { "r.greet", WARM, PLAIN, "Extra greeting." } };
static const Pat  T6_PATS[]  = { { "zzqx*", I_GREET }, { "zzqx hello", I_BYE } };
static const char *const T6_LEX[]   = { "adore|love|4", "adore|praise|2", "moon|topic.moon|5", "meh|dontcare|3", 0 };
static const char *const T6_VOCAB[] = { "the|1|x", "moon|2210|n", "wraith|9001|n", 0 };

static int do_tables(void) {
    tfail = 0;
    lang_tables_reset();
    /* 1. the key index against the tables: every row reachable under its key, keys strictly sorted, rows of a key in table order */
    const Line *T[4] = { LINES_A, LINES_B, LINES_C, LINES_D };
    int N[4] = { LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N }, total = N[0] + N[1] + N[2] + N[3], sum = 0;
    TCHECK(lang_line_count() == total, "line count %d, tables have %d", lang_line_count(), total);
    int nk = lang_key_count();
    for (int t = 0, base = 0; t < 4; base += N[t], t++)
        for (int i = 0; i < N[t]; i++) {
            const Line *l = &T[t][i]; const Line *const *rows; int n = lang_key_rows(l->key, &rows), found = 0, last = -1, ordered = 1;
            for (int r = 0; r < n; r++) {
                int ord = -1;
                for (int u = 0, b = 0; u < 4 && ord < 0; b += N[u], u++) if (rows[r] >= T[u] && rows[r] < T[u] + N[u]) ord = b + (int)(rows[r] - T[u]);
                if (rows[r] == l) found = 1;
                if (ord <= last) ordered = 0;
                last = ord;
                if (strcmp(rows[r]->key, l->key)) { TCHECK(0, "key [%s] holds a row of [%s]", l->key, rows[r]->key); break; }
            }
            if (!found || !ordered) { TCHECK(0, "row %d of table %d (key [%s]): found %d, in table order %d", i, t, l->key, found, ordered); goto keys_done; }
        }
keys_done:;
    { int cnt = 0, nrows; const char *pk = NULL;
      for (int i = 0; i < nk; i++) {
          const char *k = lang_key_name(i, &nrows);
          TCHECK(k && (!pk || strcmp(pk, k) < 0), "keys are not strictly sorted at %d", i);
          sum += nrows; pk = k; cnt++;
      }
      TCHECK(lang_key_name(nk, NULL) == NULL && lang_key_name(-1, NULL) == NULL, "key name out of range must be NULL");
      TCHECK(sum == total && cnt == nk && nk == nkeys, "key index covers %d of %d rows in %d keys (tool saw %d)", sum, total, nk, nkeys); }
    { const Line *const *rows; TCHECK(lang_key_rows("no.such.key", &rows) == 0 && rows == NULL, "an unknown key must give no rows"); }

    /* 2. the pattern index: every pattern is a candidate for its own surface text, ids come in ascending order without repeats */
    int np = lang_pat_count(); TCHECK(np == PATS_N, "pattern count %d, PATS has %d", np, PATS_N);
    int bad_p = 0;
    for (int k = 0; k < np && bad_p < 5; k++) {
        char txt[160]; int o = 0;
        for (const char *p = lang_pat(k)->pat; *p && o < 150; p++) { if (*p == '*') txt[o++] = 'x'; else if (*p == '?') txt[o++] = 'a'; else if (*p != '$') txt[o++] = *p; }
        txt[o] = 0;
        PatCur c; lang_pat_cursor(&c, txt); int id, got = 0, lastid = -1, asc = 1;
        while ((id = lang_pat_next(&c)) >= 0) { if (id == k) got = 1; if (id <= lastid) asc = 0; lastid = id; }
        if (!got || !asc) { TCHECK(0, "pattern %d [%s]: reachable %d, ascending %d", k, lang_pat(k)->pat, got, asc); bad_p++; }
    }
    { const int *ids; int ns = lang_pat_safety(&ids), want = 0; for (int k = 0; k < PATS_N; k++) if (PATS[k].intent == I_SELFHARM) want++;
      TCHECK(ns == want && ns > 0, "safety list has %d patterns, PATS has %d", ns, want); }
    { PatCur c; lang_pat_cursor(&c, "qqqq"); int n = 0; while (lang_pat_next(&c) >= 0) n++; TCHECK(n <= 3, "a word nothing starts with met %d candidates", n); }

    /* 3. registering later: the indexes rebuild and the old answers stay */
    int lines0 = lang_line_count(), pats0 = lang_pat_count(), greet0; { const Line *const *r; greet0 = lang_key_rows("r.greet", &r); }
    TCHECK(lang_register_lines(T6_LINES, 3) == 0 && lang_register_pats(T6_PATS, 2) == 0, "register failed");
    TCHECK(lang_register_lex(T6_LEX) == 0 && lang_register_vocab(T6_VOCAB) == 0, "register lex/vocab failed");
    { const Line *const *r; int n = lang_key_rows("t6.key", &r), g = lang_key_rows("r.greet", &r);
      TCHECK(n == 2 && r[0] != NULL, "new key t6.key has %d rows (want 2)", n);
      TCHECK(g == greet0 + 1 && !strcmp(r[g - 1]->text, "Extra greeting."), "r.greet has %d rows (want %d), the new one last", g, greet0 + 1); }
    TCHECK(lang_line_count() == lines0 + 3 && lang_pat_count() == pats0 + 2, "counts after register: %d lines (want %d), %d patterns (want %d)", lang_line_count(), lines0 + 3, lang_pat_count(), pats0 + 2);
    { PatCur c; lang_pat_cursor(&c, "zzqx hello"); int id, a = 0, b = 0; while ((id = lang_pat_next(&c)) >= 0) { if (id == pats0) a = 1; if (id == pats0 + 1) b = 1; }
      TCHECK(a && b, "the registered patterns are not candidates (%d %d)", a, b); }
    { const char *rows[4]; int n = lang_lex_find("adore", rows, 4);
      TCHECK(n == 2 && !strcmp(rows[0], "adore|love|4") && !strcmp(rows[1], "adore|praise|2"), "lex find adore: %d rows, registration order", n);
      TCHECK(lang_lex_find("adored", rows, 4) == 0 && lang_lex_find("ador", rows, 4) == 0 && lang_lex_find("", rows, 4) == 0, "lex find matched a non word");
      TCHECK(lang_lex_find("moon", rows, 1) == 1, "lex find max");
      TCHECK(lang_lex_count() == 4 && lang_vocab_count() == 3, "word counts %d %d", lang_lex_count(), lang_vocab_count());
      const char *v = lang_vocab_find("wraith"); TCHECK(v && !strcmp(v, "wraith|9001|n"), "vocab find wraith");
      TCHECK(!lang_vocab_find("wrait") && !lang_vocab_find("adore"), "vocab find matched a word it does not have"); }
    TCHECK(lang_init_ms() >= 0.0 && lang_line_count() == lines0 + 3, "a forced rebuild changed the counts");

    /* 4. reset: back to the built-in tables only, and nothing about the test is left */
    lang_tables_reset();
    { const Line *const *r; TCHECK(lang_line_count() == lines0 && lang_pat_count() == pats0 && lang_key_rows("t6.key", &r) == 0 && lang_lex_count() == 0 && lang_vocab_count() == 0, "reset did not restore the built-in tables"); }
    printf("tables: %d lines in %d keys, %d patterns, registry and indexes %s\n", total, nk, np, tfail ? "FAIL" : "ok");
    return tfail;
}

/* ---- chunk 6: timings ---- */
static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1e6; }

static void do_bench(void) {
    static const char *const PH[] = { "hello who are you", "i dont care about your mission", "thank you so much", "where am i and what is this place",
        "you are stupid and i hate this", "can you help me please", "tell me a joke", "i am scared of the sky", "my name is Bob", "what is the hole",
        "are you real or just a game", "i love the music", "sorry i did not mean it", "why would i listen to you", "how old are you",
        "tell me a secret about life", "wait a second", "i agree with you", "bye for now", "zzzz qqqq xxxx vvvv wwww" };
    enum { NL = 40 };
    static char lines[NL][256];
    unsigned seed = 12345u;
    for (int i = 0; i < NL; i++) {
        lines[i][0] = 0;
        while (strlen(lines[i]) < 200) { seed = seed * 1103515245u + 12345u; strcat(lines[i], PH[(seed >> 16) % 20]); strcat(lines[i], " "); }
        lines[i][200] = 0;
    }
    double init = lang_init_ms();                       /* a full build of every index */
    for (int i = 0; i < NL; i++) { Heard h; lang_parse(lines[i], &h); }   /* warm up */

    double t0 = now_ms();
    for (int k = 0; k < 1000; k++) { Heard h; lang_parse(lines[k % NL], &h); }
    double parse = (now_ms() - t0) / 1000.0;

    Persona p = { "Dea", STYLE_DIVINE, 0, 30, 65, 8 };
    t0 = now_ms();
    for (int k = 0; k < 1000; k++) { Heard h; char out[600]; lang_parse(lines[k % NL], &h); p.mood = 0; lang_reply(&p, &h, out, sizeof out); }
    double reply = (now_ms() - t0) / 1000.0 - parse;    /* the reply alone (the parse inside is taken off) */

    static const int STY[3] = { STYLE_PLAIN, STYLE_DIVINE, STYLE_STREET };
    char key[40], out[400]; t0 = now_ms(); int picked = 0;
    for (int k = 0; k < 10000; k++) {
        Persona q = { "X", STY[k % 3], 0, 50, 0, 8 };
        snprintf(key, sizeof key, "r.%s", INTENT_N[k % (I_COUNT - 2)]);
        picked += lang_line_tone(&q, key, (Tone)(k % 4), out, sizeof out);
    }
    double pick = (now_ms() - t0) / 10000.0;

    printf("bench: %d lines, %d keys, %d patterns\n", lang_line_count(), lang_key_count(), lang_pat_count());
    printf("  init (all indexes)   %8.3f ms\n", init);
    printf("  parse, 200 chars     %8.4f ms each (1,000 parses)\n", parse);
    printf("  reply (after parse)  %8.4f ms each (1,000 replies)\n", reply);
    printf("  line pick            %8.4f ms each (10,000 picks, %d found)\n", pick, picked);
    printf("  budgets (chunk 159, measured on the phone): init <= 80 ms %s, parse <= 1 ms %s, reply <= 3 ms %s\n",
           init <= 80 ? "ok" : "OVER", parse <= 1 ? "ok" : "OVER", reply <= 3 ? "ok" : "OVER");
}

/* ---- chunk 7: the 20 moods and their fallback chains ---- */
static int do_moods(void) {
    int bad = 0;
    printf("moods: %d, fallback chains (own mood first, then what is tried, then the nearest of the old four)\n", TONE_COUNT);
    for (int t = 0; t < TONE_COUNT; t++) {
        int ord[TONE_COUNT], n = lang_mood_chain(t, ord, TONE_COUNT), old = 0;
        printf("  %c %-10s:", lang_tone_code(t), lang_tone_name(t));
        for (int k = 0; k < n; k++) { printf(" %c", lang_tone_code(ord[k])); if (ord[k] < 4) old++; }
        printf("\n");
        if (ord[0] != t) { printf("BAD CHAIN: %s does not start with itself\n", lang_tone_name(t)); bad++; }
        if (old < 4) { printf("BAD CHAIN: %s never reaches all four old moods (%d)\n", lang_tone_name(t), old); bad++; }
    }
    /* the chain itself, on throwaway lines: own mood wins, then the chain, then the nearest old mood, then any style */
    {
        static const Line T7[] = { { "t7.key", COLD, ALLS, "cold line" }, { "t7.key", PLAY, ALLS, "play line" }, { "t7.only", HOST, DIVINE, "host divine line" } };
        lang_register_lines(T7, 3);
        Persona q = { "T", STYLE_PLAIN, 0, 50, 0, 8 }; char out[80];
        struct { const char *key; int mood, want; } cases[] = {
            { "t7.key", TONE_PLAYFUL, TONE_PLAYFUL }, { "t7.key", TONE_SARCASTIC, TONE_COLD }, { "t7.key", TONE_AFRAID, TONE_COLD },
            { "t7.key", TONE_WARM, TONE_COLD }, { "t7.key", TONE_EXCITED, TONE_PLAYFUL }, { "t7.key", TONE_MANIC, TONE_PLAYFUL }, { "t7.nokey", TONE_WARM, -1 } };
        for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++) {
            int r = lang_mood_resolve(cases[i].key, cases[i].mood, 0);
            if (r != cases[i].want) { printf("BAD RESOLVE %s %s: got %d want %d\n", cases[i].key, lang_tone_name(cases[i].mood), r, cases[i].want); bad++; }
        }
        if (!lang_line_tone(&q, "t7.key", TONE_PLAYFUL, out, sizeof out) || strcmp(out, "play line")) { printf("BAD: playful did not get its own line (%s)\n", out); bad++; }
        if (!lang_line_tone(&q, "t7.key", TONE_SARCASTIC, out, sizeof out) || strcmp(out, "cold line")) { printf("BAD: sarcastic did not fall back to cold (%s)\n", out); bad++; }
        if (!lang_line_tone(&q, "t7.only", TONE_SHY, out, sizeof out) || strcmp(out, "host divine line")) { printf("BAD: last resort (any mood, any style) failed (%s)\n", out); bad++; }
        if (lang_mood_resolve("t7.only", TONE_SHY, STYLE_PLAIN) != TONE_HOSTILE || lang_mood_resolve("t7.only", TONE_SHY, 0) != TONE_HOSTILE) { printf("BAD: resolve with a style\n"); bad++; }
        lang_tables_reset();
        if (lang_mood_resolve("t7.key", TONE_WARM, 0) != -1) { printf("BAD: the throwaway lines are still there after reset\n"); bad++; }
    }
    printf("answers per key (column = mood %s; = own lines, letter = answered by that mood through the chain, . = nothing). any style:\n  ", "WNCHPSEOQLMTKUAZXYBV");
    for (int t = 0; t < TONE_COUNT; t++) printf("%c", lang_tone_code(t));
    printf("\n");
    int silent = 0, direct = 0, cells = 0;
    for (int k = 0; k < lang_key_count(); k++) {
        int nrows; const char *key = lang_key_name(k, &nrows);
        if (!nrows) continue;
        char row[TONE_COUNT + 1];
        for (int t = 0; t < TONE_COUNT; t++) {
            int r = lang_mood_resolve(key, t, 0);
            row[t] = r < 0 ? '.' : r == t ? '=' : lang_tone_code(r);
            cells++; if (r < 0) { silent++; bad++; } else if (r == t) direct++;
        }
        row[TONE_COUNT] = 0;
        if (strncmp(key, "r.", 2) == 0 || strncmp(key, "p.", 2) == 0) printf("  %s  %s\n", row, key);
    }
    printf("moods: %d key x mood cells, %d answered by own lines, %d through a chain, %d silent. %s\n", cells, direct, cells - direct - silent, silent, bad ? "PROBLEMS" : "ok");
    return bad;
}

int main(int argc, char **argv) {
    int all = argc < 2, rc = 0;
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--all")) all = 1;
    int quota = 4;
    for (int i = 1; i + 1 < argc; i++) if (!strcmp(argv[i], "--coverage") && isdigit((unsigned char)argv[i + 1][0])) quota = atoi(argv[i + 1]);
    EACH(l) key_id(l->key);
    int any = 0;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--check"))    { rc |= do_check() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--stats"))    { do_stats(); any = 1; }
        else if (!strcmp(a, "--coverage")) { do_coverage(quota); any = 1; }
        else if (!strcmp(a, "--dupes"))    { do_dupes(); any = 1; }
        else if (!strcmp(a, "--length"))   { do_length(); any = 1; }
        else if (!strcmp(a, "--tables"))   { rc |= do_tables() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--moods"))    { rc |= do_moods() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--bench"))    { do_bench(); any = 1; }
    }
    if (all && !any) { rc |= do_check() ? 1 : 0; do_stats(); do_coverage(quota); do_dupes(); do_length(); rc |= do_tables() ? 1 : 0; rc |= do_moods() ? 1 : 0; do_bench(); }
    else if (all) { do_stats(); do_coverage(quota); do_dupes(); do_length(); rc |= do_tables() ? 1 : 0; rc |= do_moods() ? 1 : 0; do_bench(); }
    return rc;
}
