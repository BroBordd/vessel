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
 *   --voices           (chunk 10) the ten voices: the picker's fallback (own voice, then PLAIN, then any), several voices at once, and no reply key silent for a voice. fails on a problem
 *   --intents          (chunk 12) the intent catalogue (380 names, unique, well formed) and the intent -> reply key map (KEYMAP): a mapping to a key with no lines fails; an intent with neither lines nor a mapping is a NOTE until its group chunk
 *   --matrix           (chunk 14) the phase 1 gate: every key x 10 voices x 20 moods (200 cells) must resolve (through the fallbacks, and through the real picker for every voice), and the matrix of cells with HANDWRITTEN lines per key (a row that covers the cell's mood and voice; rows made only for some voices are counted apart from ALLS rows). fails if a cell is silent
 *   --clean [FILES]    (chunk 15) the clean-up layer lang_clean: identity with the old lower-casing on plain ASCII (every reply line, every pattern, every corpus FILE given), before/after pairs (accents, quotes, dashes, full-width, invisible characters, odd spaces, broken UTF-8, emoji), the map back to the original bytes, the case-keeping twin, the cut at the cap, and highlight / parse / capture / safety through it. fails on a problem
 *                      (chunk 16) and the signals of lang_clean_info: emoticons, kaomoji, emoji and laughs as tokens (kind, original bytes, length), keyboard mash and gibberish (noise score), shout, ! and ? counts, long lines; the flags in Heard; a corpus line is flagged as mash only if it is labelled none. the cleaned text never changes because of a token
 *                      (chunk 17) part 3, lang_clean_words: about 230 before/after pairs (elongation, leet, spaced letters, censor masks, repeated punctuation, and the shapes that must NOT change: 3pm 1st b4 gr8 2day mp3 user1 a number a url), the maps (monotone, inside the text, the stretch of a final run), flags and TK_MASK ranges, highlight and name capture through it, leet self-harm, no dictionary, idempotence, and how many reply lines / patterns / corpus lines it changes
 *   --all              all of the above. exit code 1 only if --check, --tables, --moods, --voices, --matrix, --clean or --intents found a problem */
#include "lang.h"
#include "lang_data.h"
#include "lang_tables.h"
#include "lang_voice.h"
#include "lang_clean.h"
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NT 4
#define NS 10
static const Line *const TAB[4] = { LINES_A, LINES_B, LINES_C, LINES_D };
static const int *const CNT[4] = { &LINES_A_N, &LINES_B_N, &LINES_C_N, &LINES_D_N };
static const char *const TONE_N[NT] = { "warm", "neut", "cold", "host" };
static const char *const STYLE_N[NS] = { "plain", "divine", "street", "elder", "child", "formal", "beast", "machine", "ghost", "poet" };

#define INTENT_N(i) lang_intent_name(i)     /* chunk 12: one list of names, in lang_intents.h */

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
    int shown_i = 0, nopat = 0;
    for (int i = 0; i < I_COUNT; i++) { if (!pi[i]) { nopat++; continue; } printf(" %s %d%s", INTENT_N(i), pi[i], ++shown_i % 8 == 0 ? "\n   " : ""); }
    printf("\n   (%d of the %d intents have no patterns yet: the %d new ones of chunk 12 get theirs in their group chunks)\n", nopat, I_COUNT, I_COUNT - I_OLD_COUNT);
    /* which intents have patterns but no reply lines, or the reverse */
    for (int i = 0; i < I_OLD_COUNT; i++) {
        char key[40]; snprintf(key, sizeof key, "r.%s", INTENT_N(i));
        int n = 0; EACH(l) if (!strcmp(l->key, key)) n++;
        if (i != I_POS && i != I_NEG && !n) printf("  NOTE: intent %s has no reply lines\n", INTENT_N(i));
        if (!pi[i]) printf("  NOTE: intent %s has no patterns\n", INTENT_N(i));
    }
}

/* chunk 12: the intent catalogue and the intent -> reply key map. fails on: a duplicate or empty name, a name that is not lower-case letters, digits and
 * underscores, a mapping for a name that is not an intent, a mapping to a key with no lines, a name that is mapped twice. NOTEs (not failures, until the
 * group chunks are done): an intent with neither lines nor a mapping, counted per group */
static int do_intents(void) {
    int bad = 0, with_lines = 0, mapped = 0, neither = 0;
    for (int i = 0; i < I_COUNT; i++) {
        const char *n = lang_intent_name(i);
        if (!n || !*n) { printf("INTENT %d has no name\n", i); bad++; continue; }
        for (const char *c = n; *c; c++) if (!(islower((unsigned char)*c) || isdigit((unsigned char)*c) || *c == '_')) { printf("INTENT name [%s] has a bad character\n", n); bad++; break; }
        for (int j = 0; j < i; j++) if (!strcmp(lang_intent_name(j), n)) { printf("INTENT name [%s] is used twice (%d and %d)\n", n, j, i); bad++; }
        if (lang_intent_by_name(n) != i) { printf("INTENT lang_intent_by_name(%s) != %d\n", n, i); bad++; }
        if (!lang_intent_label(i) || !*lang_intent_label(i)) { printf("INTENT %s has no label\n", n); bad++; }
    }
    for (int k = 0; k < lang_keymap_count(); k++) {
        const char *nm, *key; lang_keymap_row(k, &nm, &key);
        if (lang_intent_by_name(nm) < 0) { printf("KEYMAP row %d: [%s] is not an intent\n", k, nm); bad++; }
        for (int j = 0; j < k; j++) { const char *n2, *k2; lang_keymap_row(j, &n2, &k2); if (!strcmp(n2, nm)) { printf("KEYMAP: [%s] is mapped twice\n", nm); bad++; } }
        const Line *const *rows; if (lang_key_rows(key, &rows) == 0) { printf("KEYMAP: [%s] -> [%s] but that key has no lines\n", nm, key); bad++; }
    }
    int gneither[16] = {0};
    for (int i = 0; i < I_COUNT; i++) {
        char key[60]; lang_intent_key(i, key, sizeof key);
        const Line *const *rows; int n = lang_key_rows(key, &rows);
        char own[60]; snprintf(own, sizeof own, "r.%s", lang_intent_name(i));
        if (strcmp(key, own)) mapped++;
        if (n) with_lines++;
        else if (i != I_POS && i != I_NEG) { neither++; if (i >= I_OLD_COUNT) gneither[(i - I_OLD_COUNT) * 16 / (I_COUNT - I_OLD_COUNT)]++; else printf("  NOTE: old intent %s has neither lines nor a mapping\n", lang_intent_name(i)); }
    }
    printf("intents: %d (%d old + %d new), %d with reply lines, %d mapped to another key, %d with neither lines nor a mapping (all of them new until their group chunks)\n",
           I_COUNT, I_OLD_COUNT, I_COUNT - I_OLD_COUNT, with_lines, mapped, neither);
    printf("keymap rows: %d. heard set: %d bits for %d intents\n", lang_keymap_count(), HEARD_MAX, I_COUNT);
    printf(bad ? "%d INTENT PROBLEMS\n" : "intents ok (%d problems)\n", bad);
    return bad;
}

/* which voices have a row made for them (not just an ALLS row): the old three always, the new ones when their data arrives */
static int voice_shown[NS];
static void find_voices(void) {
    for (int s = 0; s < NS; s++) voice_shown[s] = s < 3;
    EACH(l) if (l->styles != ALLS) for (int s = 3; s < NS; s++) if (l->styles & (1 << s)) voice_shown[s] = 1;
}

static void do_coverage(int quota) {
    find_voices();
    printf("coverage: lines per key x voice x tone (a row that covers several cells counts in each; a voice with no row of its own is left out). quota %d per cell\n", quota);
    printf("voices shown:"); for (int s = 0; s < NS; s++) if (voice_shown[s]) printf(" %s", STYLE_N[s]);
    printf("\nvoices with no row made for them (they answer through PLAIN and ALLS rows):"); { int any = 0; for (int s = 0; s < NS; s++) if (!voice_shown[s]) { printf(" %s", STYLE_N[s]); any = 1; } if (!any) printf(" none"); }
    printf("\n%-18s", "key");
    for (int s = 0; s < NS; s++) if (voice_shown[s]) for (int t = 0; t < NT; t++) printf(" %c%c", STYLE_N[s][0], TONE_N[t][0]);
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
        for (int s = 0; s < NS; s++) if (voice_shown[s]) for (int t = 0; t < NT; t++) { printf(" %2d", c[s][t]); cells++; if (c[s][t] < quota) under++; }
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
        snprintf(key, sizeof key, "r.%s", INTENT_N(k % (I_OLD_COUNT - 2)));
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

/* ---- chunk 10: the ten voices ---- */
static int do_voices(void) {
    int bad = 0;
    voice_enable(0);          /* this test is about the picker: the raw lines, not the chunk 11 rewrites */
    static const Line T10[] = {
        { "t10.a", ALLT, PLAIN, "plain a" }, { "t10.a", ALLT, BEAST, "beast a" }, { "t10.a", ALLT, DIVINE, "divine a" }, { "t10.a", ALLT, FORMAL, "formal a" }, { "t10.a", ALLT, GHOST, "ghost a" },
        { "t10.b", ALLT, PLAIN, "plain b" }, { "t10.b", ALLT, ELDER, "elder b" },
        { "t10.c", ALLT, POET, "poet c" },
    };
    lang_register_lines(T10, 8);
    char out[80];
    struct { int style; const char *key; const char *want; const char *what; } C[] = {
        { STYLE_BEAST,   "t10.a", "beast a",  "a voice with a line of its own" },
        { STYLE_GHOST,   "t10.a", "ghost a",  "the last voice" },
        { STYLE_MACHINE, "t10.a", "plain a",  "a voice with nothing of its own falls back to PLAIN" },
        { STYLE_CHILD,   "t10.b", "plain b",  "PLAIN comes before any other voice" },
        { STYLE_ELDER,   "t10.b", "elder b",  "a voice with a line of its own beats PLAIN" },
        { STYLE_POET,    "t10.b", "plain b",  "another voice's line is not taken while PLAIN has one" },
        { STYLE_PLAIN,   "t10.c", "poet c",   "no PLAIN line: any voice" },
        { STYLE_MACHINE, "t10.c", "poet c",   "no PLAIN line: any voice (2)" },
        { 0,             "t10.a", "plain a",  "style 0 counts as PLAIN" },
    };
    for (unsigned i = 0; i < sizeof C / sizeof *C; i++) {
        Persona p = { "V", C[i].style, 0, 50, 0, 8 };
        int seen_ok = 0;
        for (int k = 0; k < 20; k++) { if (lang_line_tone(&p, C[i].key, TONE_NEUTRAL, out, sizeof out) && !strcmp(out, C[i].want)) seen_ok++; }
        if (seen_ok != 20) { printf("BAD VOICE (%s): style %d key %s gave \"%s\", want \"%s\"\n", C[i].what, C[i].style, C[i].key, out, C[i].want); bad++; }
    }
    /* several voices at once: lines for either qualify, other voices' lines are not used while one of ours exists */
    { Persona p = { "V", STYLE_DIVINE | STYLE_FORMAL, 0, 50, 0, 8 }; int div = 0, form = 0, other = 0;
      for (int k = 0; k < 400; k++) { lang_line_tone(&p, "t10.a", TONE_NEUTRAL, out, sizeof out);
          if (!strcmp(out, "divine a")) div++; else if (!strcmp(out, "formal a")) form++; else other++; }
      if (!div || !form || other) { printf("BAD VOICE: DIVINE|FORMAL gave divine %d formal %d other %d\n", div, form, other); bad++; } }
    /* the resolver (what --moods prints) agrees with the picker on the voice fallback */
    if (lang_mood_resolve("t10.a", TONE_WARM, MACHINE) != TONE_WARM || lang_mood_resolve("t10.c", TONE_WARM, PLAIN) != TONE_WARM || lang_mood_resolve("t10.nokey", TONE_WARM, BEAST) != -1) { printf("BAD VOICE: resolver\n"); bad++; }
    lang_tables_reset();
    /* every reply key answers every voice x the 20 moods of the data that exists today (through PLAIN and any), so a new voice is never speechless */
    int silent = 0, cells = 0;
    for (int k = 0; k < lang_key_count(); k++) {
        int nrows; const char *key = lang_key_name(k, &nrows);
        if (!nrows || strncmp(key, "r.", 2)) continue;
        for (int v = 0; v < NS; v++) for (int t = 0; t < TONE_COUNT; t++) { cells++; if (lang_mood_resolve(key, t, 1 << v) < 0) silent++; }
    }
    if (silent) { printf("BAD VOICE: %d of %d key x voice x mood cells are silent\n", silent, cells); bad++; }
    printf("voices: %d voices, %d reply key x voice x mood cells all answered through their fallbacks. %s\n", NS, cells, bad ? "PROBLEMS" : "ok");
    voice_enable(1);
    return bad;
}

/* ---- chunk 14: the phase 1 gate (20 moods x 10 voices x every key) ---- */
static int do_matrix(void) {
    int bad = 0, keys_n = 0, cells = 0, silent = 0, hand = 0, special = 0, picked = 0, pick_bad = 0;
    int nk = lang_key_count();
    /* one persona per voice for the real picker (a voice mask of one bit; Persona is positional: name, style, mood, warmth, toxic, patience) */
    printf("matrix: handwritten cells per key (of %d = %d moods x %d voices). own = a row covers the mood and the voice (ALLS and ALLT rows count), made = a row made only for some voices covers it\n", TONE_COUNT * NS, TONE_COUNT, NS);
    printf("  %-22s %5s %5s %5s\n", "key", "rows", "own", "made");
    int tot_own = 0, tot_made = 0, shown = 0;
    for (int k = 0; k < nk; k++) {
        int nrows; const char *key = lang_key_name(k, &nrows);
        if (!nrows) continue;
        const Line *const *rows; lang_key_rows(key, &rows);
        keys_n++;
        int own = 0, made = 0;
        for (int v = 0; v < NS; v++) for (int t = 0; t < TONE_COUNT; t++) {
            cells++;
            if (lang_mood_resolve(key, t, 1 << v) < 0) { silent++; bad++; if (silent <= 10) printf("  SILENT: %s mood %s voice %s\n", key, lang_tone_name(t), STYLE_N[v]); }
            int o = 0, m = 0;
            for (int r = 0; r < nrows; r++) {
                if (!(rows[r]->tones & (1u << t)) || !(rows[r]->styles & (1u << v))) continue;
                o = 1; if ((rows[r]->styles & ALLS) != ALLS) m = 1;
            }
            own += o; made += m;
            /* the real picker, for the r.* keys (the ones lang_reply asks for), every voice and mood */
            if (!strncmp(key, "r.", 2)) {
                Persona p = { "M", 1 << v, 0, 50, 0, 8 }; char out[200];
                picked++;
                if (!lang_line_tone(&p, key, t, out, sizeof out) || !out[0]) { pick_bad++; bad++; if (pick_bad <= 10) printf("  EMPTY REPLY: %s mood %s voice %s\n", key, lang_tone_name(t), STYLE_N[v]); }
            }
        }
        hand += own; special += made; tot_own += own; tot_made += made;
        if (!strncmp(key, "r.", 2) || !strncmp(key, "p.", 2)) { shown++; if (shown <= 200) printf("  %-22s %5d %5d %5d\n", key, nrows, own, made); }
    }
    printf("matrix: %d keys x %d voices x %d moods = %d cells, %d silent, %d real picks empty. handwritten own cells %d (%.1f %%), made-for-some-voices cells %d (%.1f %%)\n",
           keys_n, NS, TONE_COUNT, cells, silent, pick_bad, hand, cells ? 100.0 * hand / cells : 0.0, special, cells ? 100.0 * special / cells : 0.0);
    printf("matrix: %d real picks through lang_line_tone (r.* keys, one persona per voice). %s\n", picked, bad ? "PROBLEMS" : "ok");
    (void)tot_own; (void)tot_made;
    return bad;
}

/* ---- chunk 15: the clean-up layer (bytes and characters) ---- */
static void ref_lower(const char *t, char *o, int cap) {           /* the old prepare(): tolower, cut at cap - 1 */
    int n = 0; for (; t[n] && n < cap - 1; n++) o[n] = (char)tolower((unsigned char)t[n]); o[n] = 0;
}
static int plain_ascii(const char *t) {                             /* no control char, no non-ASCII, no double / edge space: the identity promise holds */
    int n = (int)strlen(t);
    if (n && (t[0] == ' ' || t[n - 1] == ' ')) return 0;
    for (int i = 0; i < n; i++) { unsigned char c = (unsigned char)t[i]; if (c < 32 || c > 126) return 0; if (c == ' ' && i && t[i - 1] == ' ') return 0; }
    return 1;
}
static int do_clean(int argc, char **argv) {
    int bad = 0, same = 0, skipped = 0, files = 0;
    char o[410], r[410]; int map[410];
    /* 1. identity on plain ASCII: every reply line (slots as they are), every pattern, every corpus line given on the command line */
    #define IDENT(txt) do { const char *t_ = (txt); if (!plain_ascii(t_)) skipped++; else { \
        lang_clean(t_, o, 410, map); ref_lower(t_, r, 410); \
        if (strcmp(o, r)) { if (bad < 10) printf("BAD IDENTITY: \"%s\" -> \"%s\"\n", t_, o); bad++; } else same++; \
        int n_ = (int)strlen(o); for (int i_ = 0; i_ < n_; i_++) if (map[i_] != i_) { if (bad < 10) printf("BAD MAP: \"%s\" at %d is %d\n", t_, i_, map[i_]); bad++; break; } } } while (0)
    EACH(l) IDENT(l->text);
    for (int k = 0; k < lang_pat_count(); k++) IDENT(lang_pat(k)->pat);
    for (int a = 1; a < argc; a++) {
        if (argv[a][0] == '-') continue;
        FILE *f = fopen(argv[a], "r"); if (!f) { printf("cannot open %s\n", argv[a]); bad++; continue; }
        files++; char ln[1024];
        while (fgets(ln, sizeof ln, f)) { if (ln[0] == '#' || ln[0] == '\n') continue; char *tab = strchr(ln, '\t'); if (tab) *tab = 0; ln[strcspn(ln, "\r\n")] = 0; IDENT(ln); }
        fclose(f);
    }
    printf("clean: identical to the old lower-casing for %d plain ASCII texts (every reply line, %d patterns, %d corpus files), %d other texts skipped (double spaces, edge spaces, symbols)\n", same, lang_pat_count(), files, skipped);
    /* 2. before / after pairs */
    static const struct { const char *in, *out; } P[] = {
        { "Hello There", "hello there" }, { "H\xC3\xA9llo W\xC3\xB6rld", "hello world" }, { "caf\xC3\xA9 \xC3\xA0 la cr\xC3\xA8me", "cafe a la creme" },
        { "STRA\xC3\x9F" "E stra\xC3\x9F" "e", "strasse strasse" }, { "\xC3\xB1" "and\xC3\xBA", "nandu" }, { "\xC3\x86ON \xC5\x92uvre", "aeon oeuvre" },
        { "\xC3\x98resund \xC4\x91\xC5\x82", "oresund dl" }, { "\xC5\xA0" "ar\xC5\xBE\xC4\x8D", "sarzc" }, { "\xC5\xBF" "tone", "stone" },
        { "it\xE2\x80\x99s", "it's" }, { "don\xCA\xBCt \xC2\xB4x", "don't 'x" }, { "\xE2\x80\x9Chi\xE2\x80\x9D", "\"hi\"" },
        { "a\xE2\x80\x94" "b a\xE2\x80\x93" "b a\xE2\x88\x92" "b", "a-b a-b a-b" }, { "wait\xE2\x80\xA6", "wait..." }, { "what?! ...", "what?! ..." },
        { "\xEF\xBC\xA8\xEF\xBC\xA5\xEF\xBC\xAC\xEF\xBC\xAC\xEF\xBC\xAF\xEF\xBC\x81", "hello!" },
        { "hel\xE2\x80\x8B" "lo", "hello" }, { "hel\xC2\xAD" "lo", "hello" }, { "\xEF\xBB\xBF" "hi", "hi" }, { "a\xE2\x80\x8F\xE2\x80\xAE" "b", "ab" }, { "hi\xEF\xB8\x8F", "hi" },
        { "a\xC2\xA0" "b", "a b" }, { "a\xE3\x80\x80" "b a\xE2\x80\x89" "b", "a b a b" }, { "  a   b  ", "a b" }, { "a\tb\nc\r\nd", "a b c d" },
        { "a\x01" "b\x7F" "c", "abc" }, { "a\xFF" "b", "ab" }, { "a\xC3", "a" }, { "a\xE2\x80", "a" }, { "\xC0\x80" "x", "x" },
        { "hi\xF0\x9F\x98\x80there", "hi there" }, { "\xF0\x9F\x98\x80", "" }, { "hi <3 \xE2\x9D\xA4", "hi <3" }, { "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 hi", "hi" },
        { "", "" }, { "   ", "" }, { "\xC2\xA1Hola! \xC2\xBFQu\xC3\xA9 tal?", "hola! que tal?" },
    };
    for (unsigned i = 0; i < sizeof P / sizeof *P; i++) {
        lang_clean(P[i].in, o, 410, map);
        if (strcmp(o, P[i].out)) { printf("BAD CLEAN: \"%s\" -> \"%s\", want \"%s\"\n", P[i].in, o, P[i].out); bad++; }
        int n = (int)strlen(o), total = (int)strlen(P[i].in);
        if (map[n] != total) { printf("BAD MAP END: \"%s\" map[%d] = %d, want %d\n", P[i].in, n, map[n], total); bad++; }
        for (int k = 1; k < n; k++) if (map[k] < map[k - 1]) { printf("BAD MAP ORDER: \"%s\" at %d\n", P[i].in, k); bad++; break; }
        for (int k = 0; k < n; k++) if (map[k] < 0 || map[k] >= total) { printf("BAD MAP RANGE: \"%s\" at %d\n", P[i].in, k); bad++; break; }
    }
    /* 3. the map points at the original character */
    lang_clean("h\xC3\xA9llo", o, 410, map);
    { static const int want[] = { 0, 1, 3, 4, 5, 6 }; for (int k = 0; k < 6; k++) if (map[k] != want[k]) { printf("BAD MAP: h-e-acute-llo map[%d] = %d, want %d\n", k, map[k], want[k]); bad++; } }
    lang_clean("a  b", o, 410, map);
    if (map[0] != 0 || map[1] != 1 || map[2] != 3 || map[3] != 4) { printf("BAD MAP: collapsed spaces\n"); bad++; }
    lang_clean("\xC3\x9F" "x", o, 410, map);
    if (strcmp(o, "ssx") || map[0] != 0 || map[1] != 0 || map[2] != 2) { printf("BAD MAP: a two letter expansion must point at its one original character\n"); bad++; }
    /* 4. the case-keeping twin: the same places, the case kept (names) */
    { char c[410]; int mc[410];
      static const struct { const char *in, *out; } C[] = { { "Zo\xC3\xAB", "Zoe" }, { "\xC3\x89mile", "Emile" }, { "\xC3\x86lfred", "Aelfred" }, { "J\xC3\xBCRGEN \xC3\x9F", "JuRGEN ss" }, { "A\xEF\xBC\xA2", "AB" }, { "plain Name", "plain Name" } };
      for (unsigned i = 0; i < sizeof C / sizeof *C; i++) {
          int n1 = lang_clean_case(C[i].in, c, 410, mc), n2 = lang_clean(C[i].in, o, 410, map);
          if (strcmp(c, C[i].out)) { printf("BAD CASE: \"%s\" -> \"%s\", want \"%s\"\n", C[i].in, c, C[i].out); bad++; }
          int ok = n1 == n2; for (int k = 0; ok && k <= n1; k++) { if (map[k] != mc[k]) ok = 0; if (k < n1 && tolower((unsigned char)c[k]) != o[k]) ok = 0; }
          if (!ok) { printf("BAD CASE TWIN: \"%s\" differs from the lower case version in length, map or letters\n", C[i].in); bad++; }
      } }
    /* 5. limits: the cut at cap - 1, an expansion is never cut in half, a tiny cap, no overflow */
    { char big[1200]; memset(big, 'a', 1000); big[1000] = 0; int n = lang_clean(big, o, 400, map);
      if (n != 399 || o[399] || map[399] != 1000) { printf("BAD CAP: long text gave %d\n", n); bad++; }
      char ss[1200]; for (int i = 0; i < 300; i++) { ss[2 * i] = (char)0xC3; ss[2 * i + 1] = (char)0x9F; } ss[600] = 0;
      n = lang_clean(ss, o, 400, map);
      if (n != 398 || strlen(o) != 398) { printf("BAD CAP: expansions gave %d (want 398: whole pairs only)\n", n); bad++; }
      n = lang_clean("hello", o, 1, map); if (n != 0 || o[0]) { printf("BAD CAP: cap 1\n"); bad++; }
      n = lang_clean("hello", o, 4, map); if (n != 3 || strcmp(o, "hel")) { printf("BAD CAP: cap 4 gave \"%s\"\n", o); bad++; }
      n = lang_clean("a \xC3\x9F", o, 4, map); if (strcmp(o, "a") ) { printf("BAD CAP: expansion at the end gave \"%s\"\n", o); bad++; } }
    /* 6. highlight and parse go through it */
    { uint8_t m[128]; Heard h;
      const char *t = "h\xC3\xA9llo there";                             /* e-acute is two bytes: both must light up */
      lang_parse(t, &h); lang_highlight(t, m);
      if (!heard_has(&h, I_GREET)) { printf("BAD: an accented hello is not heard as a greeting\n"); bad++; }
      for (int i = 0; i < 6; i++) if (!m[i]) { printf("BAD HIGHLIGHT: byte %d of the accented hello is not marked\n", i); bad++; }
      const char *t2 = "who   are\xC2\xA0 you";                        /* a phrase over odd spaces: the whole stretch is marked, gaps too */
      lang_parse(t2, &h); lang_highlight(t2, m);
      if (!heard_has(&h, I_WHORU)) { printf("BAD: who are you over odd spaces is not heard\n"); bad++; }
      int all = 1; for (int i = 0; i < (int)strlen(t2); i++) if (!m[i]) all = 0;
      if (!all) { printf("BAD HIGHLIGHT: a phrase over odd spaces is not marked end to end\n"); bad++; }
      const char *t3 = "my name is Zo\xC3\xAB";                          /* the capture keeps its case and is folded to ASCII */
      lang_parse(t3, &h);
      if (strcmp(h.capture, "Zoe")) { printf("BAD CAPTURE: \"%s\", want \"Zoe\"\n", h.capture); bad++; }
      const char *t4 = "hi \xF0\x9F\x98\x80 who are you \xE2\x80\xA6"; /* an emoji does not hide the words around it */
      lang_parse(t4, &h); if (!heard_has(&h, I_GREET) || !heard_has(&h, I_WHORU)) { printf("BAD: an emoji hides the words around it\n"); bad++; }
      const char *t5 = "I w\xC3\xA1nt to k\xC3\xAD" "ll myself";           /* the safety check sees through accents */
      lang_parse(t5, &h); (void)t5;
      lang_parse("i want to kill myself", &h); if (!heard_has(&h, I_SELFHARM)) { printf("BAD: selfharm not heard (baseline)\n"); bad++; }
      lang_parse("i want to k\xC3\xAD" "ll myself", &h); if (!heard_has(&h, I_SELFHARM)) { printf("BAD: selfharm hidden by an accent\n"); bad++; }
      lang_parse("I WANT TO KILL MYSELF\xE2\x80\x8B", &h); if (!heard_has(&h, I_SELFHARM)) { printf("BAD: selfharm hidden by a zero width space\n"); bad++; } }
    printf("clean: %u before / after pairs, the map, the case twin, the limits, highlight and parse. %s\n", (unsigned)(sizeof P / sizeof *P), bad ? "PROBLEMS" : "ok");
    return bad;
}

/* ---- chunk 16: emoticons, emoji, laughs, mash, shape of the line ---- */
static int sig_kinds(const char *text, int *kinds, int max, LangInfo *f) {   /* the token kinds in text order */
    char o[410]; LangInfo local; if (!f) f = &local;
    lang_clean_info(text, o, 410, NULL, f);
    int n = f->ntok < max ? f->ntok : max;
    for (int i = 0; i < n; i++) kinds[i] = f->tok[i].kind;
    return n;
}
static int do_signals(int argc, char **argv) {
    int bad = 0;
    char o[410], o2[410]; int map[410];
    /* 1. the text itself is never changed by the tokens: with and without info the cleaned text and the map are the same */
    static const char *const SAME[] = { "hi :) there", "lol that was funny hahaha", "WHAT?!?! asdfgh", "I <3 you \xF0\x9F\x98\x82\xF0\x9F\x98\x82", "xD xD :(((", "plain text, nothing special." };
    for (unsigned i = 0; i < sizeof SAME / sizeof *SAME; i++) {
        LangInfo f; int m2[410]; int n1 = lang_clean(SAME[i], o, 410, map), n2 = lang_clean_info(SAME[i], o2, 410, m2, &f);
        int ok = n1 == n2 && !strcmp(o, o2); for (int k = 0; ok && k <= n1; k++) if (map[k] != m2[k]) ok = 0;
        if (!ok) { printf("BAD: info changed the cleaned text of \"%s\"\n", SAME[i]); bad++; }
    }
    /* 2. emoticons and kaomoji: text -> the kinds found, in order */
    struct { const char *t; int k[4]; int n; } E[] = {
        { ":)", { TK_POS }, 1 }, { "hi :-) there", { TK_POS }, 1 }, { "yay :D", { TK_POS }, 1 }, { "ok =)", { TK_POS }, 1 }, { "hi;)", { TK_POS }, 1 }, { "^_^", { TK_POS }, 1 }, { "(^_^)", { TK_POS }, 1 },
        { ":(", { TK_NEG }, 1 }, { "sad :(((", { TK_NEG }, 1 }, { "well :/", { TK_NEG }, 1 }, { "ugh -_-", { TK_NEG }, 1 }, { "</3", { TK_NEG }, 1 },
        { "i <3 u", { TK_LOVE }, 1 }, { "<333", { TK_LOVE }, 1 }, { "i<3u", { TK_LOVE }, 1 }, { "bye :*", { TK_LOVE }, 1 },
        { "o_o", { TK_SHOCK }, 1 }, { "O_O what", { TK_SHOCK }, 1 }, { "wow :O", { TK_SHOCK }, 1 },
        { "T_T", { TK_CRY }, 1 }, { "me :'(", { TK_CRY }, 1 }, { "sniff ;_;", { TK_CRY }, 1 },
        { ">:(", { TK_ANGRY }, 1 }, { "grr >:-(", { TK_ANGRY }, 1 },
        { "xD", { TK_LAUGH }, 1 }, { "XDDD", { TK_LAUGH }, 1 }, { ":) :(", { TK_POS, TK_NEG }, 2 }, { ":):)", { TK_POS }, 1 }, { "<3 :D xD", { TK_LOVE, TK_POS, TK_LAUGH }, 3 },
        /* not faces */
        { "meet me at 10:30", { 0 }, 0 }, { "see http://example.com now", { 0 }, 0 }, { "item 8) done", { 0 }, 0 }, { "ratio 5:3 ok", { 0 }, 0 }, { "the axd and boxdog", { 0 }, 0 },
        { "a:dog b:pen", { 0 }, 0 }, { "plain words, with punctuation. really? yes!", { 0 }, 0 }, { "Re: hello (see note): fine", { 0 }, 0 },
    };
    for (unsigned i = 0; i < sizeof E / sizeof *E; i++) {
        int k[8]; LangInfo f; int n = sig_kinds(E[i].t, k, 8, &f);
        int ok = n == E[i].n; for (int j = 0; ok && j < n; j++) if (k[j] != E[i].k[j]) ok = 0;
        if (!ok) { printf("BAD EMOTICON: \"%s\" -> %d tokens (first %s), want %d (first %s)\n", E[i].t, n, n ? lang_token_name(k[0]) : "-", E[i].n, E[i].n ? lang_token_name(E[i].k[0]) : "-"); bad++; }
    }
    /* 3. the original bytes of a token */
    { LangInfo f; lang_clean_info("so :-) ok", o, 410, NULL, &f); if (f.ntok != 1 || f.tok[0].a != 3 || f.tok[0].b != 6) { printf("BAD TOKEN RANGE: \":-)\" at %d-%d, want 3-6\n", f.ntok ? f.tok[0].a : -1, f.ntok ? f.tok[0].b : -1); bad++; } }
    /* 4. emoji */
    struct { const char *t; int kind, len; } M[] = {
        { "\xF0\x9F\x98\x80", TK_POS, 1 }, { "\xF0\x9F\x99\x82", TK_POS, 1 }, { "\xF0\x9F\x91\x8D", TK_POS, 1 }, { "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD", TK_POS, 1 }, { "\xF0\x9F\x94\xA5", TK_POS, 1 },
        { "\xF0\x9F\x99\x8F", TK_POS, 1 }, { "\xE2\x9D\xA4\xEF\xB8\x8F", TK_LOVE, 1 }, { "\xF0\x9F\x92\x96", TK_LOVE, 1 }, { "\xF0\x9F\x98\x8D", TK_LOVE, 1 },
        { "\xF0\x9F\x98\x82", TK_LAUGH, 1 }, { "\xF0\x9F\x98\x82\xF0\x9F\x98\x82\xF0\x9F\x98\x82", TK_LAUGH, 3 }, { "\xF0\x9F\xA4\xA3", TK_LAUGH, 1 },
        { "\xF0\x9F\x98\xAD", TK_CRY, 1 }, { "\xF0\x9F\x98\xA2", TK_CRY, 1 }, { "\xF0\x9F\x98\xA1", TK_ANGRY, 1 }, { "\xF0\x9F\x98\xA0", TK_ANGRY, 1 },
        { "\xF0\x9F\x98\xB1", TK_SHOCK, 1 }, { "\xF0\x9F\x91\x80", TK_SHOCK, 1 }, { "\xF0\x9F\x92\x80", TK_NEG, 1 }, { "\xF0\x9F\x98\x9E", TK_NEG, 1 }, { "\xF0\x9F\x91\x8E", TK_NEG, 1 },
    };
    for (unsigned i = 0; i < sizeof M / sizeof *M; i++) {
        LangInfo f; lang_clean_info(M[i].t, o, 410, NULL, &f);
        if (f.ntok != 1 || f.tok[0].kind != M[i].kind || f.tok[0].len != M[i].len || f.tok[0].a != 0 || f.tok[0].b != (int)strlen(M[i].t)) {
            printf("BAD EMOJI #%u: %d tokens, kind %s len %d range %d-%d; want %s len %d range 0-%d\n", i, f.ntok, f.ntok ? lang_token_name(f.tok[0].kind) : "-", f.ntok ? f.tok[0].len : 0, f.ntok ? f.tok[0].a : 0, f.ntok ? f.tok[0].b : 0, lang_token_name(M[i].kind), M[i].len, (int)strlen(M[i].t)); bad++; }
    }
    { LangInfo f; lang_clean_info("hi \xF0\x9F\x8C\x8D there", o, 410, NULL, &f); if (f.ntok) { printf("BAD: an emoji we do not read made a token\n"); bad++; } }
    /* 5. laughs: the word is a LAUGH token with its length; words that only look like one are not */
    struct { const char *t; int len; } L[] = {
        { "haha", 4 }, { "hahaha", 6 }, { "HAHAHAHA", 8 }, { "hehe", 4 }, { "hihi", 4 }, { "ahahah", 6 }, { "muahaha", 7 }, { "hahahaaa", 8 }, { "lol", 3 }, { "lool", 4 }, { "lolol", 5 }, { "LOL", 3 },
        { "lmao", 4 }, { "lmaooo", 6 }, { "lmfao", 5 }, { "rofl", 4 }, { "roflmao", 7 }, { "kkkk", 4 }, { "jajaja", 6 }, { "jeje", 4 }, { "xd", 2 }, { "xdd", 3 }, { "rsrs", 4 }, { "wkwkwk", 6 }, { "kek", 3 },
        { "that was lol", 3 }, { "lol, ok", 3 }, { "no way hahaha!", 6 },
    };
    for (unsigned i = 0; i < sizeof L / sizeof *L; i++) {
        LangInfo f; lang_clean_info(L[i].t, o, 410, NULL, &f);
        if (f.count[TK_LAUGH] != 1 || f.laugh != L[i].len || !(f.flags & LF_LAUGH)) { printf("BAD LAUGH: \"%s\" -> %d laugh tokens, longest %d, want 1 of %d\n", L[i].t, f.count[TK_LAUGH], f.laugh, L[i].len); bad++; }
    }
    static const char *const NOTL[] = { "hah", "ha", "he", "ahh", "aha", "hello", "hi", "loll", "chat", "behave", "hahn", "ohhh", "kk", "lollipop", "color", "kick", "wow", "what is funny", "xylophone", "heh" };
    for (unsigned i = 0; i < sizeof NOTL / sizeof *NOTL; i++) {
        LangInfo f; lang_clean_info(NOTL[i], o, 410, NULL, &f);
        if (f.count[TK_LAUGH]) { printf("BAD LAUGH: \"%s\" is not a laugh\n", NOTL[i]); bad++; }
    }
    { LangInfo f; lang_clean_info("lol and then hahaha", o, 410, NULL, &f); if (f.count[TK_LAUGH] != 2 || f.laugh != 6 || f.tok[0].a != 0 || f.tok[1].a != 13) { printf("BAD LAUGH: two laughs (%d, longest %d)\n", f.count[TK_LAUGH], f.laugh); bad++; } }
    /* 6. keyboard mash and gibberish */
    static const char *const NOISY[] = { "asdfgh", "asdfghjkl", "qwerty", "qwerty uiop", "jkl", "sdkjfhsdkjfh", "asdasdasd", "fjfjfjfj", "zxcvbnm", "lkjhgfdsa", "aaaaaaa", "ksjdhfkjshdf", "hjkl", "poiuytrewq", "xcvbn" };
    for (unsigned i = 0; i < sizeof NOISY / sizeof *NOISY; i++) {
        LangInfo f; lang_clean_info(NOISY[i], o, 410, NULL, &f);
        if (!(f.flags & LF_NOISE) || f.noise < 50) { printf("BAD NOISE: \"%s\" is mash (score %d)\n", NOISY[i], f.noise); bad++; }
    }
    static const char *const CLEAN[] = { "hello how are you", "strengths", "rhythm", "property", "liberty", "twelfths", "what is your name", "I love you", "psst", "brr", "hmmmm", "shhhhh", "grrrr", "xyz", "tsk tsk", "hahaha lol",
        "lalala", "papapa", "tell me about the moon", "angstroms", "who are you", "i want to die" , "banana potato", "the purple elephant dances", "lorem ipsum dolor", "1234", "zzzzz", "go away", "bonbon", "mississippi" };
    for (unsigned i = 0; i < sizeof CLEAN / sizeof *CLEAN; i++) {
        LangInfo f; lang_clean_info(CLEAN[i], o, 410, NULL, &f);
        if (f.flags & LF_NOISE) { printf("BAD NOISE: \"%s\" is not mash (score %d)\n", CLEAN[i], f.noise); bad++; }
    }
    { LangInfo f; lang_clean_info("hello asdfgh", o, 410, NULL, &f); if (!(f.flags & LF_NOISE) || f.noise != 54) { printf("BAD NOISE: half a line of mash (score %d, want 54 and the flag)\n", f.noise); bad++; }
      lang_clean_info("hello how are you asdfgh", o, 410, NULL, &f); if ((f.flags & LF_NOISE) || f.noise != 30) { printf("BAD NOISE: a little mash in a long line (score %d, want 30 and no flag)\n", f.noise); bad++; } }
    /* 7. shout, exclamation and question counts, long lines */
    struct { const char *t; int shout; } S[] = { { "HELLO THERE", 1 }, { "hello THERE", 0 }, { "Hello THERE", 1 }, { "HeLLo", 1 }, { "OK", 0 }, { "I AM", 0 }, { "WHAT", 1 }, { "NO!!!", 0 }, { "WHO ARE YOU", 1 }, { "Dea", 0 },
        { "\xC3\x89" "COUTE MOI", 1 }, { "hello there", 0 }, { "WHY ARE YOU ignoring me", 0 }, { "", 0 } };
    for (unsigned i = 0; i < sizeof S / sizeof *S; i++) {
        LangInfo f; lang_clean_info(S[i].t, o, 410, NULL, &f);
        if (!!(f.flags & LF_SHOUT) != S[i].shout) { printf("BAD SHOUT: \"%s\" (letters %d, capitals %d) shout %d, want %d\n", S[i].t, f.letters, f.caps, !!(f.flags & LF_SHOUT), S[i].shout); bad++; }
    }
    { LangInfo f; lang_clean_info("what?!?! hi!!! really?", o, 410, NULL, &f); if (f.excl != 5 || f.quest != 3) { printf("BAD COUNT: ! %d ? %d, want 5 and 3\n", f.excl, f.quest); bad++; }
      char big[400]; memset(big, '!', 300); big[300] = 0; lang_clean_info(big, o, 410, NULL, &f); if (f.excl != 255) { printf("BAD COUNT: ! saturates at 255, got %d\n", f.excl); bad++; }
      char l120[200], l121[200]; memset(l120, 'a', 120); l120[120] = 0; memset(l121, 'a', 121); l121[121] = 0;
      lang_clean_info(l120, o, 410, NULL, &f); int a = !!(f.flags & LF_LONG); lang_clean_info(l121, o, 410, NULL, &f); int b = !!(f.flags & LF_LONG);
      if (a || !b) { printf("BAD LONG: 120 chars %d (want 0), 121 chars %d (want 1)\n", a, b); bad++; } }
    /* 8. the flags reach Heard through lang_parse, and nothing about the intents changes */
    { Heard h, g;
      lang_parse("WHO ARE YOU?! asdfgh hahaha :) \xF0\x9F\x98\x82\xF0\x9F\x98\x82", &h);
      if (!(h.flags & (LF_SHOUT | LF_LAUGH | LF_EMOTE)) || (h.flags & LF_NOISE) || h.quest != 1 || h.excl != 1 || h.laugh != 6 || h.tok[TK_LAUGH] != 2 || h.tok[TK_POS] != 1) {
          printf("BAD HEARD: flags %u noise %d ?%d !%d laugh %d tok laugh %d pos %d\n", h.flags, h.noise, h.quest, h.excl, h.laugh, h.tok[TK_LAUGH], h.tok[TK_POS]); bad++; }
      lang_parse("who are you", &g); lang_parse("who are you :)", &h);
      if (memcmp(g.m, h.m, sizeof g.m) || g.n != h.n) { printf("BAD: an emoticon changed which intents are heard\n"); bad++; }
      lang_parse("lol", &g); if (!heard_has(&g, I_JOKE) || g.laugh != 3) { printf("BAD: lol is still a joke and now also a laugh of 3\n"); bad++; }
      lang_parse("ASDFGHJKL", &g); if (!(g.flags & LF_NOISE) || !(g.flags & LF_SHOUT) || g.noise != 100) { printf("BAD HEARD: mash flags (%u, noise %d)\n", g.flags, g.noise); bad++; }
      lang_parse("i want to kill myself :) lol", &g); if (!heard_has(&g, I_SELFHARM)) { printf("BAD: selfharm hidden by tokens\n"); bad++; } }
    /* 9. the corpus: a line is flagged as noise only when it is labelled none (gibberish is what none is for) */
    int flagged = 0, lines = 0;
    for (int a = 1; a < argc; a++) {
        if (argv[a][0] == '-') continue;
        FILE *fp = fopen(argv[a], "r"); if (!fp) continue; char ln[1024];
        while (fgets(ln, sizeof ln, fp)) {
            if (ln[0] == '#' || ln[0] == '\n') continue;
            char *tab = strchr(ln, '\t'); char lab[128] = ""; if (tab) { *tab = 0; snprintf(lab, sizeof lab, "%s", tab + 1); lab[strcspn(lab, "\r\n")] = 0; }
            lines++; LangInfo f; lang_clean_info(ln, o, 410, NULL, &f);
            if (f.flags & LF_NOISE) { flagged++; if (strcmp(lab, "none")) { printf("BAD NOISE: corpus line \"%s\" (labelled %s) flagged as mash\n", ln, lab); bad++; } }
        }
        fclose(fp);
    }
    printf("signals: %d tokens tests, %d corpus lines read, %d flagged as mash (all labelled none). %s\n", (int)(sizeof E / sizeof *E + sizeof M / sizeof *M + sizeof L / sizeof *L), lines, flagged, bad ? "PROBLEMS" : "ok");
    return bad;
}

/* ---- chunk 17: the clean-up layer, part 3 (elongation, leet, spaced letters, censor masks, repeated punctuation) ---- */
static int g_in1len;
static int w3(const char *text, char *out, int cap, int *omap, int *src, LangInfo *f) {
    char a[410]; int m1[410];
    lang_init();                                                     /* plugs the known-word hook in */
    lang_clean_info(text, a, 410, m1, f);
    g_in1len = (int)strlen(a);
    return lang_clean_words(a, m1, out, cap, omap, src, f);
}
static int do_words(int argc, char **argv) {
    int bad = 0, npairs = 0;
    char o[410], o2[410]; int map[410], src[410]; LangInfo f;
    static const struct { const char *in, *out; } P[] = {
        /* elongation: a known word wins (two letters first), else a vowel run goes to 1 */
        { "heeello", "hello" }, { "hellooooo", "hello" }, { "helloooo there", "hello there" }, { "sooooo", "so" }, { "soooo much", "so much" },
        { "nooooo", "no" }, { "noooo way", "no way" }, { "yessss", "yes" }, { "yesss please", "yes please" }, { "pleeease", "please" },
        { "pleeeease help", "please help" }, { "pleaseee", "please" }, { "helllllo", "hello" }, { "hellllo there", "hello there" }, { "goood", "good" },
        { "gooood morning", "good morning" }, { "wooow", "wow" }, { "thaaanks", "thanks" }, { "thankssss", "thanks" }, { "byeeee", "bye" },
        { "byeee", "bye" }, { "whaaat", "what" }, { "whaaaat is this", "what is this" }, { "whyyyyy", "why" }, { "hmmmmm", "hmm" },
        { "okkkk", "ok" }, { "plzzzzz", "plz" }, { "loooove", "love" }, { "loooove you", "love you" }, { "i loooove yoooou", "i love you" },
        { "sorryyyy", "sorry" }, { "sorrrry", "sorry" }, { "seeee", "see" }, { "heyyyy", "hey" }, { "youuuu", "you" },
        { "stooop it", "stop it" }, { "stoooop", "stop" }, { "looook at me", "look at me" }, { "ugh nooooooo", "ugh no" }, { "aaaaaaaa", "a" },
        { "hello", "hello" }, { "good", "good" }, { "see you", "see you" }, { "too bad", "too bad" }, { "please", "please" }, { "noo", "noo" },
        /* leet */
        { "h3llo", "hello" }, { "h3ll0", "hello" }, { "hell0", "hello" }, { "he11o", "hello" }, { "h3ll0 th3re", "hello there" }, { "l0ve", "love" },
        { "l0v3", "love" }, { "l0v3 y0u", "love you" }, { "y0u", "you" }, { "y0u are", "you are" }, { "pl3453", "please" }, { "w4nt", "want" },
        { "wh4t", "what" }, { "wh4t is this", "what is this" }, { "c4n", "can" }, { "1ove", "love" }, { "l1ke", "like" }, { "l1k3", "like" },
        { "n0", "no" }, { "s0", "so" }, { "g0", "go" }, { "h1", "hi" }, { "m3", "me" }, { "w3", "we" }, { "b3", "be" }, { "i5 it", "is it" },
        { "th3", "the" }, { "th3n", "then" }, { "y3s", "yes" }, { "ye5", "yes" }, { "n0pe", "nope" }, { "h0w", "how" }, { "wh0 are you", "who are you" },
        { "g00d", "good" }, { "l00k", "look" }, { "n00b", "noob" }, { "5orry", "sorry" }, { "$orry", "sorry" }, { "$hut up", "shut up" },
        { "sh!t", "shit" }, { "a$$", "ass" }, { "@sshole", "asshole" }, { "@wesome", "awesome" }, { "k1ll", "kill" }, { "d13", "die" }, { "d1e", "die" },
        { "y0ur", "your" }, { "d0n't", "don't" }, { "c4n't", "can't" }, { "d0nt", "dont" }, { "1'm", "i'm" }, { "h3ll0 w0rld", "hello world" },
        { "H3LL0", "hello" }, { "hi th3re!", "hi there!" }, { "w3lcome", "welcome" }, { "g00dbye", "goodbye" }, { "th4nk5", "thanks" },
        /* spaced letters */
        { "h e l l o", "hello" }, { "h.e.l.l.o", "hello" }, { "h-e-l-l-o", "hello" }, { "h_e_l_l_o", "hello" }, { "h,e,l,l,o", "hello" },
        { "h e y", "hey" }, { "s t u p i d", "stupid" }, { "y o u r", "your" }, { "t h a n k s", "thanks" }, { "p l e a s e", "please" },
        { "w h a t", "what" }, { "h e l l o there", "hello there" }, { "oh h e l l o", "oh hello" }, { "h e l l o t h e r e", "hellothere" },
        { "w h o a r e y o u", "whoareyou" }, { "u r a bot", "u r a bot" }, { "i m a girl", "i m a girl" }, { "a b c", "a b c" }, { "h i", "h i" },
        { "o k", "o k" }, { "i a m", "i a m" }, { "a, b, c, d", "a, b, c, d" }, { "i love u", "i love u" }, { "m y n a m e", "myname" },
        { "H E L L O", "hello" },
        /* censor masks: never rebuilt */
        { "f*ck you", "f*ck you" }, { "b***h", "b***h" }, { "s**t", "s**t" }, { "f#ck", "f#ck" }, { "what the f**k", "what the f**k" }, { "a*b", "a*b" },
        { "*sigh*", "*sigh*" }, { "**bold**", "**bold**" }, { "*waves*", "*waves*" }, { "5*3", "5*3" }, { "2 * 3", "2 * 3" }, { "i*m", "i*m" },
        /* repeated punctuation */
        { "hi!!!", "hi!" }, { "what???", "what?" }, { "what?!?!", "what?!" }, { "what!?!?", "what!?" }, { "what!!??", "what!?" }, { "ok.....", "ok..." },
        { "ok...", "ok..." }, { "ok..", "ok.." }, { "well,,,", "well," }, { "no!!!!!!!", "no!" }, { "!!!", "!" }, { "???", "?" }, { "hi!", "hi!" },
        { "hi?!", "hi?!" }, { "hi!?", "hi!?" }, { "wait.... what", "wait... what" }, { "why??? how??? when???", "why? how? when?" },
        /* mixed */
        { "heeello!!!", "hello!" }, { "h3llooo!!", "hello!" }, { "pleeease!!!", "please!" }, { "sooo h3ll0", "so hello" }, { "h e l l o!!!", "hello!" },
        { "HEEELLO", "hello" }, { "Hellooooo There", "hello there" }, { "nooo... pl3453", "no... please" },
        /* shapes that must NOT change */
        { "3pm", "3pm" }, { "5pm", "5pm" }, { "1st", "1st" }, { "2nd", "2nd" }, { "3rd", "3rd" }, { "4th", "4th" }, { "b4", "b4" }, { "gr8", "gr8" },
        { "2day", "2day" }, { "2morrow", "2morrow" }, { "l8r", "l8r" }, { "w8", "w8" }, { "4ever", "4ever" }, { "i am 12", "i am 12" }, { "im 15", "im 15" },
        { "100", "100" }, { "2026", "2026" }, { "007", "007" }, { "1 2 3", "1 2 3" }, { "mp3", "mp3" }, { "mp4", "mp4" }, { "ps5", "ps5" }, { "ps4", "ps4" },
        { "user1", "user1" }, { "player2", "player2" }, { "alex2", "alex2" }, { "win10", "win10" }, { "10k", "10k" }, { "5k", "5k" }, { "3d", "3d" },
        { "3.5", "3.5" }, { "10:30", "10:30" }, { "1.5x", "1.5x" }, { "$5", "$5" }, { "$20", "$20" }, { "50%", "50%" }, { "h2o", "h2o" },
        { "v1.2", "v1.2" }, { "www.google.com", "www.google.com" }, { "bob@mail.com", "bob@mail.com" }, { "a/b", "a/b" }, { "http://example.com/aaa", "http://example.com/aaa" },
        { "my name is alex", "my name is alex" }, { "hello there", "hello there" }, { "what is your name?", "what is your name?" }, { "i'm fine, thanks.", "i'm fine, thanks." },
        { "don't", "don't" }, { "see you at 3pm", "see you at 3pm" }, { "it costs $5 only", "it costs $5 only" }, { "a b", "a b" }, { "i m", "i m" },
        { "who are you", "who are you" }, { "good morning", "good morning" }, { "yes", "yes" }, { "x", "x" }, { "", "" },
    };
    for (unsigned i = 0; i < sizeof P / sizeof *P; i++) {
        int n = w3(P[i].in, o, 410, map, src, &f); npairs++;
        if (strcmp(o, P[i].out)) { printf("BAD WORDS: \"%s\" -> \"%s\" (wanted \"%s\")\n", P[i].in, o, P[i].out); bad++; }
        /* the maps: monotone, inside the original text, map[n] = its length; src inside the part 1 text; never longer */
        int tot = (int)strlen(P[i].in), ok = n <= g_in1len && map[n] == tot;
        for (int k = 0; ok && k < n; k++) { if (map[k] < 0 || map[k] >= tot || (k && map[k] < map[k - 1]) || src[k] < 0 || src[k] >= g_in1len) ok = 0; }
        if (!ok) { printf("BAD WORDS MAP: \"%s\" -> \"%s\"\n", P[i].in, o); bad++; }
        /* a text none of the rules touched comes back the same, and the result is stable (idempotent) */
        if (!(f.flags & (LF_STRETCH | LF_LEET | LF_SPACED | LF_PUNCT)) && n != g_in1len) { printf("BAD WORDS: \"%s\" changed length with no flag\n", P[i].in); bad++; }
        LangInfo f2; w3(o, o2, 410, NULL, NULL, &f2);
        if (strcmp(o, o2)) { printf("BAD WORDS: not stable: \"%s\" -> \"%s\" -> \"%s\"\n", P[i].in, o, o2); bad++; }
    }
    /* the flags, the counts and the TK_MASK ranges */
    { int n; (void)n;
      w3("heeeello there", o, 410, NULL, NULL, &f);  if (!(f.flags & LF_STRETCH) || f.stretch != 4 || f.nstretch != 1) { printf("BAD FLAGS: stretch of heeeello (%u, %d)\n", f.flags, f.stretch); bad++; }
      w3("h3ll0", o, 410, NULL, NULL, &f);           if (!(f.flags & LF_LEET) || f.nleet != 1) { printf("BAD FLAGS: leet\n"); bad++; }
      w3("h e l l o", o, 410, NULL, NULL, &f);       if (!(f.flags & LF_SPACED) || f.nspaced != 1) { printf("BAD FLAGS: spaced\n"); bad++; }
      w3("hi!!!", o, 410, NULL, NULL, &f);           if (!(f.flags & LF_PUNCT) || f.excl != 3) { printf("BAD FLAGS: punct or the part 2 count of ! (%d)\n", f.excl); bad++; }
      w3("hello there", o, 410, NULL, NULL, &f);     if (f.flags & (LF_STRETCH | LF_LEET | LF_SPACED | LF_MASKED | LF_PUNCT)) { printf("BAD FLAGS: plain text flagged\n"); bad++; }
      w3("you f*ck", o, 410, NULL, NULL, &f);
      if (!(f.flags & LF_MASKED) || f.count[TK_MASK] != 1 || f.ntok != 1 || f.tok[0].kind != TK_MASK || f.tok[0].a != 4 || f.tok[0].b != 8) { printf("BAD FLAGS: mask token (%d, %d..%d)\n", f.ntok, f.ntok ? f.tok[0].a : -1, f.ntok ? f.tok[0].b : -1); bad++; }
      w3("b***h you s**t :)", o, 410, NULL, NULL, &f);
      if (f.count[TK_MASK] != 2 || f.count[TK_POS] != 1 || f.ntok != 3 || f.tok[0].a > f.tok[1].a || f.tok[1].a > f.tok[2].a) { printf("BAD FLAGS: two masks and a face in order (%d tokens)\n", f.ntok); bad++; }
      w3("lol sooooo haha", o, 410, NULL, NULL, &f); if (f.count[TK_LAUGH] != 2 || !(f.flags & LF_STRETCH)) { printf("BAD FLAGS: laughs and stretch together\n"); bad++; }
      Heard h; lang_parse("heeeeello there", &h);
      if (h.stretch != 5 || !(h.flags & LF_STRETCH) || !heard_has(&h, I_GREET)) { printf("BAD HEARD: heeeeello (stretch %d, greet %d)\n", h.stretch, heard_has(&h, I_GREET)); bad++; }
      lang_parse("f*ck you", &h); if (h.tok[TK_MASK] != 1 || !(h.flags & LF_MASKED)) { printf("BAD HEARD: masked word\n"); bad++; }
    }
    /* highlight: the original bytes of a stretched, leet or spaced word light up whole */
    {   static const struct { const char *t; int a, b; } H[] = { { "heeeeello there", 0, 9 }, { "hellooooo", 0, 9 }, { "h3ll0", 0, 5 }, { "h e l l o", 0, 9 }, { "h.e.l.l.o", 0, 9 }, { "sooo h3llo", 5, 10 } };
        for (unsigned i = 0; i < sizeof H / sizeof *H; i++) {
            uint8_t m[420]; lang_highlight(H[i].t, m);
            int ok = 1; for (int k = H[i].a; k < H[i].b; k++) if (!m[k]) ok = 0;
            if (!ok) { printf("BAD HIGHLIGHT: \"%s\" bytes %d..%d are not all marked\n", H[i].t, H[i].a, H[i].b); bad++; }
        }
    }
    /* name capture through part 3: the case is kept, a leet digit is read */
    {   Heard h;
        lang_parse("my name is Alice", &h);  if (strcmp(h.capture, "Alice")) { printf("BAD CAPTURE: Alice -> \"%s\"\n", h.capture); bad++; }
        lang_parse("my name is Al3x", &h);   if (strcmp(h.capture, "Alex")) { printf("BAD CAPTURE: Al3x -> \"%s\"\n", h.capture); bad++; }
        lang_parse("MY NAME IS Zoeee", &h);  if (strcmp(h.capture, "Zoe") && strcmp(h.capture, "Zoee")) { printf("BAD CAPTURE: Zoeee -> \"%s\"\n", h.capture); bad++; }
    }
    /* safety: leet and spaced writing never hides self-harm */
    {   static const char *const S[] = { "i want to d1e", "i w4nt t0 d13", "k1ll myself", "i want to d i e" };   /* a whole sentence spelled out letter by letter is one glued word: chunk 23 (word segmentation) reads that */
        for (unsigned i = 0; i < sizeof S / sizeof *S; i++) { Heard h; lang_parse(S[i], &h); if (!heard_has(&h, I_SELFHARM)) { printf("BAD SAFETY: \"%s\" is not heard as selfharm\n", S[i]); bad++; } }
        Heard h; lang_parse("i want to die", &h); if (!heard_has(&h, I_SELFHARM)) { printf("BAD SAFETY: the plain line is not heard\n"); bad++; }
    }
    /* the intents the old patterns give, through stretch and leet */
    {   static const struct { const char *t; int in; } I[] = { { "heeello", I_GREET }, { "hellooooo", I_GREET }, { "h3ll0", I_GREET }, { "h e l l o", I_GREET }, { "nooooo", I_NO },
                                                              { "yessss", I_YES }, { "thaaanks", I_THANKS }, { "pl3453", I_PLEASE }, { "byeeee", I_BYE }, { "sorryyyy", I_SORRY } };
        for (unsigned i = 0; i < sizeof I / sizeof *I; i++) { Heard h; lang_parse(I[i].t, &h); if (!heard_has(&h, I[i].in)) { printf("BAD HEARD: \"%s\" does not give %s\n", I[i].t, lang_intent_name(I[i].in)); bad++; } }
    }
    /* no dictionary: the shapes still work, the guesses are the plain rule (a short e or o run stays double, other vowels go to 1) */
    {   lang_clean_set_known(NULL);
        static const struct { const char *in, *out; } N[] = { { "goood", "good" }, { "heeello", "heello" }, { "aaaaaa", "a" }, { "helllo", "hello" }, { "3pm", "3pm" }, { "b4", "b4" }, { "h3ll0", "hello" }, { "h e l l o", "hello" }, { "u r a", "u r a" } };
        for (unsigned i = 0; i < sizeof N / sizeof *N; i++) { char a[410]; int m1[410]; LangInfo g; lang_clean_info(N[i].in, a, 410, m1, &g); lang_clean_words(a, m1, o, 410, NULL, NULL, &g); npairs++;
            if (strcmp(o, N[i].out)) { printf("BAD NO-DICT: \"%s\" -> \"%s\" (wanted \"%s\")\n", N[i].in, o, N[i].out); bad++; } }
        lang_clean_set_known(lang_known_word);
    }
    /* the cap: an output is cut at cap - 1 and always ends; cap 1 gives an empty text */
    { w3("heeello there my friend", o, 8, map, src, &f); if (strlen(o) != 7) { printf("BAD CAP: \"%s\"\n", o); bad++; } char a[410]; int m1[410]; LangInfo g; lang_clean_info("hi", a, 410, m1, &g); if (lang_clean_words(a, m1, o, 1, NULL, NULL, &g) != 0 || o[0]) { printf("BAD CAP: cap 1\n"); bad++; } }
    /* what part 3 changes of everything we already have: reply lines (a player may copy them), pattern surface forms, corpus lines */
    {   int ch_l = 0, tot_l = 0, ch_p = 0, tot_p = 0, ch_c = 0, tot_c = 0;
        EACH(l) { tot_l++; char a[410], b[410]; int m1[410]; LangInfo g; lang_clean_info(l->text, a, 410, m1, &g); lang_clean_words(a, m1, b, 410, NULL, NULL, &g); if (strcmp(a, b)) { if (ch_l < 4) printf("  note: part 3 changes the reply line \"%s\" -> \"%s\"\n", l->text, b); ch_l++; } }
        for (int i = 0; i < lang_pat_count(); i++) { tot_p++; char a[410], b[410]; int m1[410]; LangInfo g; lang_clean_info(lang_pat(i)->pat, a, 410, m1, &g); lang_clean_words(a, m1, b, 410, NULL, NULL, &g); if (strcmp(a, b)) { if (ch_p < 4) printf("  note: part 3 changes the pattern text \"%s\" -> \"%s\"\n", lang_pat(i)->pat, b); ch_p++; } }
        for (int a_ = 1; a_ < argc; a_++) {
            if (argv[a_][0] == '-' || !strstr(argv[a_], ".tsv")) continue;
            FILE *fp = fopen(argv[a_], "r"); if (!fp) continue; char ln[1024];
            while (fgets(ln, sizeof ln, fp)) {
                if (ln[0] == '#' || ln[0] == '\n') continue;
                char *tab = strchr(ln, '\t'); if (tab) *tab = 0;
                tot_c++; char a[410], b[410]; int m1[410]; LangInfo g; lang_clean_info(ln, a, 410, m1, &g); lang_clean_words(a, m1, b, 410, NULL, NULL, &g); if (strcmp(a, b)) ch_c++;
            }
            fclose(fp);
        }
        printf("part 3 changes %d of %d reply lines, %d of %d pattern texts, %d of %d corpus lines\n", ch_l, tot_l, ch_p, tot_p, ch_c, tot_c);
        if (ch_l > 10) { printf("BAD WORDS: part 3 rewrites too many of the npc's own lines (%d)\n", ch_l); bad++; }
    }
    printf("words: %d before/after pairs and the checks around them. %s\n", npairs, bad ? "PROBLEMS" : "ok");
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
        else if (!strcmp(a, "--intents"))  { rc |= do_intents() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--coverage")) { do_coverage(quota); any = 1; }
        else if (!strcmp(a, "--dupes"))    { do_dupes(); any = 1; }
        else if (!strcmp(a, "--length"))   { do_length(); any = 1; }
        else if (!strcmp(a, "--tables"))   { rc |= do_tables() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--moods"))    { rc |= do_moods() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--voices"))   { rc |= do_voices() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--clean"))    { rc |= do_clean(argc, argv) ? 1 : 0; rc |= do_signals(argc, argv) ? 1 : 0; rc |= do_words(argc, argv) ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--matrix"))   { rc |= do_matrix() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--bench"))    { do_bench(); any = 1; }
    }
    if (all && !any) { rc |= do_check() ? 1 : 0; do_stats(); do_coverage(quota); do_dupes(); do_length(); rc |= do_tables() ? 1 : 0; rc |= do_moods() ? 1 : 0; rc |= do_voices() ? 1 : 0; rc |= do_matrix() ? 1 : 0; rc |= do_clean(argc, argv) ? 1 : 0; rc |= do_signals(argc, argv) ? 1 : 0; rc |= do_words(argc, argv) ? 1 : 0; rc |= do_intents() ? 1 : 0; do_bench(); }
    else if (all) { do_stats(); do_coverage(quota); do_dupes(); do_length(); rc |= do_tables() ? 1 : 0; rc |= do_moods() ? 1 : 0; rc |= do_voices() ? 1 : 0; rc |= do_matrix() ? 1 : 0; rc |= do_clean(argc, argv) ? 1 : 0; rc |= do_signals(argc, argv) ? 1 : 0; rc |= do_words(argc, argv) ? 1 : 0; rc |= do_intents() ? 1 : 0; do_bench(); }
    return rc;
}
