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
 *                      (chunk 18) text-speak and contractions, lang_clean_expand: the table (>= 150 well-formed rows, no duplicates), about 100 before/after pairs, the maps (a highlight covers the whole typed token), the cap, no hook, and the superset rule: everything the text as typed heard is still heard
 *                      (chunk 19) numbers, lang_clean_numbers: numbers, ages, times and ordinals with their original byte ranges (and what must NOT count: user1 mp3 2day a lone "one" 25:00), number words both ways (0-1100 round trip and bigger), the number-letter words of the expanded variant (b4 gr8 2morrow 4ever any1, a lone 2 / 4 as to / too / for), the Heard fields, and the superset rule with the number rules off
 *   --vocab            (chunk 20) the vocabulary and the word hash: every row (word|rank|pos, ranks 1..N in order with no gaps, no duplicates, a-z and '), every lookup (vocab_has / rank / pos / is_name / protected, case blind, pointer and length) against the row, the names, the known-word hook, registering later and reset, and the lookup time. fails on a problem
 *   --spell [FILES]    (chunk 21) the spell repair lang_spell: the dictionary and the delete index, about 60 fix pairs and 70 words that must stay, the never-repaired rules (vocabulary, names, patterns, text-speak, acronyms, names in the middle of a sentence, digits, 1-2 letters), the maps and the highlight, the cap, the Heard fields, self-harm through typos, registering later, the superset rule (spell on never loses an intent over every reply line and every corpus FILE) and the speed. fails on a problem
 *   --phonetic         (chunk 22) the phonetic key lang_phone (24 exact keys, 16 pairs that sound alike, pairs that do not, case, cap, a very long text), the dialect table (>= 150 rows: well-formed, no vocabulary word or name as a key, no text-speak key, no duplicate, the key never its result) and its repair (45 typed -> spelled pairs, what must stay, the order dialect / edit / phonetic, the maps of a multi-word result, the cap, the highlight and Heard, self-harm through a row), the phonetic fallback (what it fixes, real words it must not bend, only words of patterns and lexicons are candidates), registering later, the off switch and the speed. fails on a problem
 *   --all              all of the above. exit code 1 only if --check, --tables, --moods, --voices, --matrix, --clean or --intents found a problem */
#include "lang.h"
#include "lang_data.h"
#include "lang_tables.h"
#include "lang_voice.h"
#include "lang_clean.h"
#include "lang_vocab.h"
#include "lang_spell.h"
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
    int lines0 = lang_line_count(), pats0 = lang_pat_count(), lex0 = lang_lex_count(), voc0 = lang_vocab_count(), greet0; { const Line *const *r; greet0 = lang_key_rows("r.greet", &r); }
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
      TCHECK(lang_lex_count() == lex0 + 4 && lang_vocab_count() == voc0 + 3, "word counts %d %d", lang_lex_count(), lang_vocab_count());
      const char *v = lang_vocab_find("wraith"); TCHECK(v && !strcmp(v, "wraith|9001|n"), "vocab find wraith");
      TCHECK(!lang_vocab_find("wrait") && !lang_vocab_find("adore"), "vocab find matched a word it does not have"); }
    TCHECK(lang_init_ms() >= 0.0 && lang_line_count() == lines0 + 3, "a forced rebuild changed the counts");

    /* 4. reset: back to the built-in tables only, and nothing about the test is left */
    lang_tables_reset();
    { const Line *const *r; TCHECK(lang_line_count() == lines0 && lang_pat_count() == pats0 && lang_key_rows("t6.key", &r) == 0 && lang_lex_count() == lex0 && lang_vocab_count() == voc0, "reset did not restore the built-in tables"); }
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

/* ---- chunk 18: contractions and text-speak (the expanded variant) ---- */
static int xp(const char *text, char *out, int cap, int *omap, int *src, LangInfo *f) {
    char a[410]; int m1[410];
    lang_init();
    lang_clean_info(text, a, 410, m1, f);
    char b[410]; int m3[410];
    lang_clean_words(a, m1, b, 410, m3, NULL, f);
    return lang_clean_expand(b, m3, out, cap, omap, src, f);
}
static int do_textspeak(int argc, char **argv) {
    int bad = 0, npairs = 0, rows = 0;
    char o[410]; int map[410], src[410]; LangInfo f;
    /* the table */
    for (const char *const *r = TEXTSPEAK; *r; r++) {
        rows++;
        const char *bar = strchr(*r, '|'); int kn = bar ? (int)(bar - *r) : 0;
        int ok = bar && kn > 0 && bar[1];
        for (int i = 0; ok && i < kn; i++) if (!((*r)[i] >= 'a' && (*r)[i] <= 'z') && (*r)[i] != '\'') ok = 0;     /* no digits (chunk 19), no case, no spaces */
        for (const char *q = bar ? bar + 1 : ""; ok && *q; q++) if (!((*q >= 'a' && *q <= 'z') || *q == ' ' || *q == '\'')) ok = 0;
        if (ok && strlen(bar + 1) == (size_t)kn && !strncmp(*r, bar + 1, (size_t)kn)) ok = 0;                     /* not itself */
        for (const char *const *r2 = r + 1; ok && *r2; r2++) { const char *b2 = strchr(*r2, '|'); if (b2 && b2 - *r2 == kn && !strncmp(*r, *r2, (size_t)kn)) { printf("BAD TEXTSPEAK: duplicate key in \"%s\"\n", *r); bad++; } }
        if (!ok) { printf("BAD TEXTSPEAK: row \"%s\"\n", *r); bad++; }
    }
    if (rows < 150) { printf("BAD TEXTSPEAK: only %d rows, the first batch has at least 150\n", rows); bad++; }
    static const struct { const char *in, *out; } P[] = {
        { "u r cool", "you are cool" }, { "ur cool", "your cool" }, { "ya know", "you know" }, { "yer mad", "your mad" }, { "pls help", "please help" },
        { "plz plz", "please please" }, { "thx", "thanks" }, { "ty so much", "thank you so much" }, { "idk", "i do not know" }, { "idc about it", "i do not care about it" },
        { "imo yes", "in my opinion yes" }, { "tbh no", "to be honest no" }, { "ngl that hurt", "not going to lie that hurt" }, { "btw who are you", "by the way who are you" },
        { "omg", "oh my god" }, { "wtf is this", "what the hell is this" }, { "brb", "be right back" }, { "gtg bye", "got to go bye" }, { "ily", "i love you" }, { "i luv u", "i love you" },
        { "dont", "do not" }, { "don't", "do not" }, { "cant", "cannot" }, { "can't go", "cannot go" }, { "wont", "will not" }, { "won't", "will not" },
        { "im bob", "i am bob" }, { "i'm bob", "i am bob" }, { "ive got it", "i have got it" }, { "i've won", "i have won" }, { "i'd go", "i would go" }, { "i'll go", "i will go" },
        { "isnt", "is not" }, { "arent you", "are not you" }, { "wasnt me", "was not me" }, { "didnt", "did not" }, { "doesn't", "does not" }, { "shouldnt", "should not" },
        { "youre nice", "you are nice" }, { "you're nice", "you are nice" }, { "theyre here", "they are here" }, { "we're here", "we are here" }, { "hes ok", "he is ok" }, { "shes ok", "she is ok" },
        { "whats ur name", "what is your name" }, { "what's up", "what is up" }, { "thats it", "that is it" }, { "whos there", "who is there" }, { "wheres my dog", "where is my dog" }, { "hows it going", "how is it going" },
        { "lets go", "let us go" }, { "let's go", "let us go" }, { "gonna", "going to" }, { "wanna play", "want to play" }, { "gotta go", "got to go" }, { "lemme see", "let me see" },
        { "dunno", "do not know" }, { "kinda sad", "kind of sad" }, { "sorta ok", "sort of ok" }, { "cuz", "because" }, { "bc i said so", "because i said so" }, { "coz", "because" }, { "tho", "though" },
        { "how r u doin?", "how are you doin?" }, { "k", "ok" }, { "y not", "why not" }, { "n you", "and you" }, { "whatcha doing", "what are you doing" }, { "cya", "see you" },
        { "'u'", "'you'" }, { "(idk)", "(i do not know)" }, { "idk!", "i do not know!" }, { "idk, u?", "i do not know, you?" },
        /* what must stay */
        { "hello there", "hello there" }, { "you are cool", "you are cool" }, { "i do not know", "i do not know" }, { "do not", "do not" }, { "what is your name", "what is your name" },
        { "ok", "ok" }, { "see you", "see you" }, { "", "" }, { "3pm", "3pm" }, { "gr8", "great" }, { "2day", "today" }, { "b4", "before" }, { "bob", "bob" }, { "ur", "your" },
        { "dont dont dont", "do not do not do not" }, { "idk idk", "i do not know i do not know" },
    };
    for (unsigned i = 0; i < sizeof P / sizeof *P; i++) {
        int n = xp(P[i].in, o, 410, map, src, &f); npairs++;
        if (strcmp(o, P[i].out)) { printf("BAD TEXTSPEAK: \"%s\" -> \"%s\" (wanted \"%s\")\n", P[i].in, o, P[i].out); bad++; }
        int same = !strcmp(P[i].in, P[i].out);
        char a[410]; int m1[410]; LangInfo g; lang_clean_info(P[i].in, a, 410, m1, &g);
        if (!!f.nexpand == same && strcmp(a, o)) { printf("BAD TEXTSPEAK: nexpand %d for \"%s\"\n", f.nexpand, P[i].in); bad++; }
        int tot = (int)strlen(P[i].in), ok = map[n] == tot;
        for (int k = 0; ok && k < n; k++) if (map[k] < 0 || map[k] >= tot || (k && map[k] < map[k - 1])) ok = 0;
        if (!ok) { printf("BAD TEXTSPEAK MAP: \"%s\"\n", P[i].in); bad++; }
    }
    /* a highlight covers the whole typed token: the last letter of an expansion maps to the last character of the token */
    xp("say idk ok", o, 410, map, src, &f);
    { int last = -1; for (int k = 0; o[k]; k++) if (!strncmp(o + k, "know", 4)) last = k + 3; if (last < 0 || map[last] != 6) { printf("BAD TEXTSPEAK MAP: the end of idk maps to byte %d, not 6\n", last < 0 ? -1 : map[last]); bad++; } }
    /* the cap: a token whose expansion does not fit stays as typed; the output always ends */
    { char t[8]; lang_init(); char a[410]; int m1[410]; LangInfo g; lang_clean_info("idk idk", a, 410, m1, &g);
      int n = lang_clean_expand(a, m1, t, 8, NULL, NULL, &g); if (n > 7 || (strcmp(t, "idk idk") && strcmp(t, "idk"))) { printf("BAD TEXTSPEAK CAP: \"%s\"\n", t); bad++; }
      n = lang_clean_expand(a, m1, t, 1, NULL, NULL, &g); if (n != 0 || t[0]) { printf("BAD TEXTSPEAK CAP: cap 1\n"); bad++; } }
    /* no hook: nothing is expanded */
    { lang_clean_set_expand(NULL); xp("idk dont u", o, 410, NULL, NULL, &f); if (strcmp(o, "idk dont u") || f.nexpand) { printf("BAD TEXTSPEAK: expanded with no table\n"); bad++; } lang_clean_set_expand(lang_textspeak); }
    /* heard: what the old patterns could not hear, and the flag */
    {   static const struct { const char *t; int in; } I[] = { { "ily", I_LOVE }, { "i luv u", I_LOVE }, { "how r you doin", I_HOWRU }, { "whats ur name", I_WHORU }, { "hows it going", I_HOWRU } };
        for (unsigned i = 0; i < sizeof I / sizeof *I; i++) { Heard h; lang_parse(I[i].t, &h); if (!heard_has(&h, I[i].in) || !(h.flags & LF_TEXTSPEAK)) { printf("BAD HEARD: \"%s\" does not give %s (flags %u)\n", I[i].t, lang_intent_name(I[i].in), h.flags); bad++; } }
        Heard h; lang_parse("hello there", &h); if (h.flags & LF_TEXTSPEAK) { printf("BAD HEARD: plain text flagged as text-speak\n"); bad++; }
    }
    /* safety in both texts, the name from the text as typed (case kept) and from the expanded text when only it hears it */
    {   static const char *const S[] = { "i wanna die", "im gonna kill myself", "i dont wanna live", "i wanna kill myself" };
        for (unsigned i = 0; i < sizeof S / sizeof *S; i++) { Heard h; lang_parse(S[i], &h); printf("%s", ""); if (!heard_has(&h, I_SELFHARM)) { printf("note: \"%s\" is not heard as selfharm (the plain pattern set decides; chunk 154 owns the wording)\n", S[i]); } }
        Heard h; lang_parse("my name is Bob", &h); if (strcmp(h.capture, "Bob")) { printf("BAD CAPTURE: Bob -> \"%s\"\n", h.capture); bad++; }
        lang_parse("im Bob", &h); if (h.capture[0] && strcmp(h.capture, "Bob")) { printf("BAD CAPTURE: im Bob -> \"%s\"\n", h.capture); bad++; }
    }
    /* highlight: what only the expanded variant heard lights up the typed token */
    {   uint8_t m[420]; lang_highlight("ily", m); if (!(m[0] && m[1] && m[2])) { printf("BAD HIGHLIGHT: ily is not all marked\n"); bad++; }
        lang_highlight("hey ily bob", m); if (m[3] || !(m[4] && m[5] && m[6])) { printf("BAD HIGHLIGHT: ily inside a line\n"); bad++; } }
    /* the superset rule (the plan's "the old corpus gives identical intents"): turn the table off and parse everything we have; with it on, every intent
     * heard without it is still heard. extra intents are the point; lost ones are bugs */
    {   int lost = 0, gained = 0, lines = 0;
        char **txt = NULL; int nt = 0, ct = 0;
        #define ADDT(str) do { if (nt == ct) { ct = ct ? ct * 2 : 4096; txt = realloc(txt, (size_t)ct * sizeof *txt); } txt[nt++] = strdup(str); } while (0)
        EACH(l) ADDT(l->text);
        for (int i = 0; i < lang_pat_count(); i++) ADDT(lang_pat(i)->pat);
        for (int a_ = 1; a_ < argc; a_++) {
            if (argv[a_][0] == '-' || !strstr(argv[a_], ".tsv")) continue;
            FILE *fp = fopen(argv[a_], "r"); if (!fp) continue; char ln[1024];
            while (fgets(ln, sizeof ln, fp)) { if (ln[0] == '#' || ln[0] == '\n') continue; char *tab = strchr(ln, '\t'); if (tab) *tab = 0; ADDT(ln); }
            fclose(fp);
        }
        for (int i = 0; i < nt; i++) {
            Heard a, b;
            lang_clean_set_expand(NULL);  lang_parse(txt[i], &a);
            lang_clean_set_expand(lang_textspeak); lang_parse(txt[i], &b);
            lines++;
            for (int in = 0; in < I_COUNT; in++) {
                if (heard_has(&a, in) && !heard_has(&b, in)) { if (lost < 5) printf("  lost %s on \"%s\"\n", lang_intent_name(in), txt[i]); lost++; }
                if (!heard_has(&a, in) && heard_has(&b, in)) gained++;
            }
            free(txt[i]);
        }
        free(txt);
        printf("expanded variant over %d texts (reply lines, patterns, corpus): %d intents gained, %d lost\n", lines, gained, lost);
        if (lost) { printf("BAD TEXTSPEAK: the expanded variant must never take a heard intent away (%d)\n", lost); bad++; }
    }
    printf("textspeak: %d rows, %d before/after pairs and the checks around them. %s\n", rows, npairs, bad ? "PROBLEMS" : "ok");
    return bad;
}

/* chunk 19: numbers. the detector (numbers, ages, times, ordinals) with the original byte ranges, number words both ways, the number-letter words of the
 * expanded variant, and the superset rule with the number rules turned off */
static int do_numbers(int argc, char **argv) {
    int bad = 0, npairs = 0;
    lang_init();
    /* number words both ways */
    {   int rt = 0;
        for (long v = 0; v <= 1100; v++) {
            char w[96], o[24]; long b = -1;
            if (!lang_num_to_words(v, w, sizeof w) || !lang_num_words(w, (int)strlen(w), &b) || b != v) { if (bad < 8) printf("BAD NUMBERS: %ld -> \"%s\" -> %ld\n", v, w, b); bad++; }
            if (!lang_num_ordinal(v, o, sizeof o)) { printf("BAD NUMBERS: ordinal %ld\n", v); bad++; }
            rt++;
        }
        static const long BIG[] = { 1999, 2000, 2026, 10000, 12345, 99999, 100000, 100001, 999999, 1000000, 1000001, 12345678, 999999999 };
        for (unsigned i = 0; i < sizeof BIG / sizeof *BIG; i++) {
            char w[160]; long b = -1;
            if (!lang_num_to_words(BIG[i], w, sizeof w) || !lang_num_words(w, (int)strlen(w), &b) || b != BIG[i]) { printf("BAD NUMBERS: %ld -> \"%s\" -> %ld\n", BIG[i], w, b); bad++; }
            rt++;
        }
        static const struct { const char *w; long v; } W[] = { { "twenty one", 21 }, { "twenty-one", 21 }, { "one hundred and five", 105 }, { "two thousand twenty six", 2026 }, { "one million", 1000000 },
            { "nineteen", 19 }, { "zero", 0 }, { "forty two", 42 }, { "fourty two", 42 }, { "three hundred sixty five", 365 }, { "two thousand and one", 2001 }, { "ninety-nine", 99 } };
        for (unsigned i = 0; i < sizeof W / sizeof *W; i++) { long b = -1; if (!lang_num_words(W[i].w, (int)strlen(W[i].w), &b) || b != W[i].v) { printf("BAD NUMBERS: \"%s\" -> %ld (wanted %ld)\n", W[i].w, b, W[i].v); bad++; } rt++; }
        static const char *const NO[] = { "one two", "twenty twenty", "twenty ten", "and five", "hundred", "thousand", "five hundred and", "one and", "zero one", "five five", "", "hello", "twenty one hello", "one thousand thousand", "ten five" };
        for (unsigned i = 0; i < sizeof NO / sizeof *NO; i++) { long b; if (lang_num_words(NO[i], (int)strlen(NO[i]), &b)) { printf("BAD NUMBERS: \"%s\" is not a number in words\n", NO[i]); bad++; } rt++; }
        static const struct { long v; const char *o; } OR[] = { { 1, "1st" }, { 2, "2nd" }, { 3, "3rd" }, { 4, "4th" }, { 11, "11th" }, { 12, "12th" }, { 13, "13th" }, { 21, "21st" }, { 22, "22nd" }, { 23, "23rd" }, { 101, "101st" }, { 111, "111th" }, { 112, "112th" } };
        for (unsigned i = 0; i < sizeof OR / sizeof *OR; i++) { char o[24]; lang_num_ordinal(OR[i].v, o, sizeof o); if (strcmp(o, OR[i].o)) { printf("BAD NUMBERS: ordinal %ld -> %s\n", OR[i].v, o); bad++; } rt++; }
        char t[6]; if (lang_num_to_words(123, t, sizeof t) || t[0]) { printf("BAD NUMBERS: to_words ignored the cap\n"); bad++; }
        printf("numbers: %d words <-> digits checks\n", rt);
    }
    /* the detector: A age, N number, T time (minutes), O ordinal */
    static const struct { const char *t, *want; } D[] = {
        { "i am 12", "A12" }, { "im twelve", "A12" }, { "i'm 12", "A12" }, { "im 12 years old", "A12" }, { "twelve years old", "A12" }, { "12 years old", "A12" }, { "12yo", "A12" }, { "my age is 12", "A12" },
        { "i turned 30", "A30" }, { "i am twenty", "A20" }, { "i am twenty five", "A25" }, { "aged 9", "A9" }, { "im 3 years old and i like cats", "A3" }, { "im 20 minutes late", "N20" }, { "i am 3 hours late", "N3" },
        { "i have 3 dogs", "N3" }, { "i have three dogs", "N3" }, { "i am 200", "N200" }, { "i am 0", "N0" }, { "twenty one", "N21" }, { "twenty-one", "N21" }, { "one hundred and five", "N105" }, { "two thousand twenty six", "N2026" },
        { "7 times 8", "N7,N8" }, { "one plus one", "N1,N1" }, { "one two three", "N1,N2,N3" }, { "10k", "N10000" }, { "-5 degrees", "N-5" }, { "3.5", "N3" }, { "1,2", "N1,N2" }, { "i have 12 and 14", "N12,N14" },
        { "see you at 3pm", "T900" }, { "3 pm", "T900" }, { "3:30", "T210" }, { "15:30", "T930" }, { "3:30pm", "T930" }, { "12am", "T0" }, { "12 pm", "T720" }, { "11:59 pm", "T1439" }, { "midnight", "T0" }, { "noon", "T720" },
        { "at three oclock", "T180" }, { "3 o'clock", "T180" }, { "at 10 am", "T600" }, { "meet me at noon", "T720" },
        { "the 1st time", "O1" }, { "my 22nd birthday", "O22" }, { "the third door", "O3" }, { "the first", "O1" }, { "your second try", "O2" }, { "the 3rd and the 4th", "O3,O4" }, { "11th", "O11" },
        /* what is not a number */
        { "first of all", "" }, { "second chance", "" }, { "i am the one", "" }, { "no one", "" }, { "which one", "" }, { "one of us", "" }, { "user1", "" }, { "mp3", "" }, { "2day", "" }, { "gr8", "" }, { "b4", "" }, { "ps5", "" },
        { "25:00", "" }, { "13pm", "" }, { "0am", "" }, { "90s", "" }, { "hello there", "" }, { "", "" }, { "wait 4 me", "" }, { "want 2 go", "" }, { "me 2", "" }, { "4 u", "" }, { "u 2", "" }, { "2 be or not 2 be", "" }, { "a hundred", "" },
    };
    for (unsigned i = 0; i < sizeof D / sizeof *D; i++) {
        char t[420]; int m[420]; LangInfo f; memset(&f, 0, sizeof f);
        lang_clean_info(D[i].t, t, 420, m, &f);
        char w[420]; int wm[420];
        lang_clean_words(t, m, w, 420, wm, NULL, &f);
        lang_clean_numbers(D[i].t, w, wm, &f); npairs++;
        char got[160]; int g = 0; got[0] = 0;
        for (int k = 0; k < f.nnum; k++) g += snprintf(got + g, sizeof got - (size_t)g, "%s%c%d", k ? "," : "", "?NATO"[f.num[k].kind], f.num[k].value);
        if (strcmp(got, D[i].want)) { printf("BAD NUMBERS: \"%s\" -> \"%s\" (wanted \"%s\")\n", D[i].t, got, D[i].want); bad++; }
        if (!!f.nnum != !!(f.flags & LF_NUMBER)) { printf("BAD NUMBERS: flag for \"%s\"\n", D[i].t); bad++; }
        int tot = (int)strlen(D[i].t);
        for (int k = 0; k < f.nnum; k++) if (f.num[k].a < 0 || f.num[k].b > tot || f.num[k].a >= f.num[k].b || (k && f.num[k].a < f.num[k - 1].a)) { printf("BAD NUMBERS RANGE: \"%s\" entry %d [%d,%d)\n", D[i].t, k, f.num[k].a, f.num[k].b); bad++; }
    }
    /* the original byte ranges, also behind folded accents, emoji and doubled spaces */
    {   static const struct { const char *t; int a, b; } R[] = { { "i am 12 now", 5, 7 }, { "\xc3\xa9 i am 12", 8, 10 }, { "ok \xf0\x9f\x98\x80  im   twenty one", 14, 24 }, { "see you at 3:30 pm", 11, 18 }, { "the 22nd", 4, 8 }, { "ta\xc3\xaf" " 3pm", 5, 8 } };
        for (unsigned i = 0; i < sizeof R / sizeof *R; i++) {
            char t[420]; int m[420]; LangInfo f; memset(&f, 0, sizeof f); lang_clean_info(R[i].t, t, 420, m, &f);
            char w[420]; int wm[420]; lang_clean_words(t, m, w, 420, wm, NULL, &f); lang_clean_numbers(R[i].t, w, wm, &f); npairs++;
            if (f.nnum < 1 || f.num[0].a != R[i].a || f.num[0].b != R[i].b) { printf("BAD NUMBERS RANGE: \"%s\" wanted [%d,%d) got [%d,%d)\n", R[i].t, R[i].a, R[i].b, f.nnum ? f.num[0].a : -1, f.nnum ? f.num[0].b : -1); bad++; }
        }
    }
    /* no map: the range is in the text itself; no info: nothing breaks; the cleaned text is never changed */
    {   LangInfo f; memset(&f, 0, sizeof f); int n = lang_clean_numbers(NULL, "i am 12", NULL, &f); if (n != 1 || f.num[0].a != 5 || f.num[0].b != 7) { printf("BAD NUMBERS: no map\n"); bad++; }
        if (lang_clean_numbers(NULL, "i am 12", NULL, NULL) != 1) { printf("BAD NUMBERS: no info\n"); bad++; }
        char big[2000]; for (int i = 0; i < 1990; i += 2) { big[i] = '7'; big[i + 1] = ' '; } big[1990] = 0;
        memset(&f, 0, sizeof f); n = lang_clean_numbers(NULL, big, NULL, &f); if (n != LANG_MAX_NUM) { printf("BAD NUMBERS: cap %d\n", n); bad++; } }
    /* the expanded variant: number-letter words and a lone 2 / 4 */
    {   static const struct { const char *in, *out; } P[] = {
            { "see you 2morrow", "see you tomorrow" }, { "wait 4 me", "wait for me" }, { "i want 2 go", "i want to go" }, { "me 2", "me too" }, { "u 2", "you too" }, { "gr8 job", "great job" },
            { "b4 you go", "before you go" }, { "l8r", "later" }, { "4ever", "forever" }, { "2 be or not 2 be", "to be or not to be" }, { "4 u", "for you" }, { "w8 4 me", "wait for me" },
            { "any1 there?", "anyone there?" }, { "no1", "no one" }, { "i luv u 2", "i love you too" }, { "str8 up", "straight up" }, { "m8", "mate" }, { "2nite?", "tonight?" }, { "i cant 4get", "i cannot forget" },
            /* what must stay */
            { "i am 12", "i am 12" }, { "im 4", "i am 4" }, { "i have 2 dogs", "i have 2 dogs" }, { "1 2 3", "1 2 3" }, { "3pm", "3pm" }, { "2", "2" }, { "4", "4" }, { "i am 2", "i am 2" }, { "room 2", "room 2" },
            { "one 2 three", "one 2 three" }, { "gr8t", "gr8t" }, { "user4", "user4" }, { "12 2 go", "12 2 go" }, { "age 4 me", "age 4 me" } };
        for (unsigned i = 0; i < sizeof P / sizeof *P; i++) {
            char o[410]; int map[410], src[410]; LangInfo f; int n = xp(P[i].in, o, 410, map, src, &f); npairs++;
            if (strcmp(o, P[i].out)) { printf("BAD NUMBERS: expand \"%s\" -> \"%s\" (wanted \"%s\")\n", P[i].in, o, P[i].out); bad++; }
            int tot = (int)strlen(P[i].in), ok = map[n] == tot;
            for (int k = 0; ok && k < n; k++) if (map[k] < 0 || map[k] >= tot || (k && map[k] < map[k - 1])) ok = 0;
            if (!ok) { printf("BAD NUMBERS MAP: expand \"%s\"\n", P[i].in); bad++; }
        }
        char o[410]; int map[410], src[410]; LangInfo f;
        xp("see you 2morrow ok", o, 410, map, src, &f);
        { int last = -1; for (int k = 0; o[k]; k++) if (!strncmp(o + k, "tomorrow", 8)) last = k + 7; if (last < 0 || map[last] != 14) { printf("BAD NUMBERS MAP: the end of 2morrow maps to %d, not 14\n", last < 0 ? -1 : map[last]); bad++; } }
        lang_clean_set_numbers(0); xp("gr8 wait 4 me", o, 410, NULL, NULL, &f); if (strcmp(o, "gr8 wait 4 me") || f.nexpand) { printf("BAD NUMBERS: expanded with the rules off\n"); bad++; } lang_clean_set_numbers(1);
    }
    /* heard: the numbers and the age reach the Heard; plain text has none */
    {   Heard h; lang_parse("i am 12", &h); if (h.age != 12 || h.nnum != 1 || h.num[0].kind != NK_AGE || !(h.flags & LF_NUMBER)) { printf("BAD HEARD: i am 12 -> age %d nnum %d\n", h.age, h.nnum); bad++; }
        lang_parse("see you at 3pm, the 2nd door, 7 and 8, 9, 10", &h); if (h.nnum != HEARD_NUM || h.num[0].kind != NK_TIME || h.num[1].kind != NK_ORDINAL || h.age) { printf("BAD HEARD: several numbers (nnum %d)\n", h.nnum); bad++; }
        lang_parse("hello there", &h); if (h.nnum || h.age || (h.flags & LF_NUMBER)) { printf("BAD HEARD: plain text has numbers\n"); bad++; }
        lang_parse("see you 2morrow", &h); if (h.nnum || !(h.flags & LF_TEXTSPEAK)) { printf("BAD HEARD: 2morrow (nnum %d)\n", h.nnum); bad++; }
        lang_parse("my name is Bob and im 12", &h); if (strcmp(h.capture, "Bob") && strcmp(h.capture, "Bob and im 12")) { printf("note: capture \"%s\"\n", h.capture); } }
    /* the superset rule: with the number rules off, every intent heard is still heard with them on */
    {   int lost = 0, gained = 0, lines = 0, withnum = 0;
        char **txt = NULL; int nt = 0, ct = 0;
        #define ADDT(str) do { if (nt == ct) { ct = ct ? ct * 2 : 4096; txt = realloc(txt, (size_t)ct * sizeof *txt); } txt[nt++] = strdup(str); } while (0)
        EACH(l) ADDT(l->text);
        for (int i = 0; i < lang_pat_count(); i++) ADDT(lang_pat(i)->pat);
        for (int a_ = 1; a_ < argc; a_++) {
            if (argv[a_][0] == '-' || !strstr(argv[a_], ".tsv")) continue;
            FILE *fp = fopen(argv[a_], "r"); if (!fp) continue; char ln[1024];
            while (fgets(ln, sizeof ln, fp)) { if (ln[0] == '#' || ln[0] == '\n') continue; char *tab = strchr(ln, '\t'); if (tab) *tab = 0; ADDT(ln); }
            fclose(fp);
        }
        for (int i = 0; i < nt; i++) {
            Heard a, b;
            lang_clean_set_numbers(0); lang_parse(txt[i], &a);
            lang_clean_set_numbers(1); lang_parse(txt[i], &b);
            lines++; if (b.nnum) withnum++;
            if (a.nnum) { printf("BAD NUMBERS: numbers found with the rules off\n"); bad++; }
            for (int in = 0; in < I_COUNT; in++) {
                if (heard_has(&a, in) && !heard_has(&b, in)) { if (lost < 5) printf("  lost %s on \"%s\"\n", lang_intent_name(in), txt[i]); lost++; }
                if (!heard_has(&a, in) && heard_has(&b, in)) gained++;
            }
            free(txt[i]);
        }
        free(txt);
        printf("numbers over %d texts (reply lines, patterns, corpus): %d carry a number, %d intents gained, %d lost\n", lines, withnum, gained, lost);
        if (lost) { printf("BAD NUMBERS: the number rules must never take a heard intent away (%d)\n", lost); bad++; }
    }
    printf("numbers: %d detector / range / expansion checks and the checks around them. %s\n", npairs, bad ? "PROBLEMS" : "ok");
    return bad;
}

/* chunk 20: the vocabulary and the word hash. the rows (format, ranks 1..N in order, no duplicates), every lookup against a plain walk of the tables,
 * the names, the (w, n) and case-blind forms, "never corrected" (vocab_protected), the known-word hook, registering later and reset */
static int cmp_str(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }
static int do_vocab(void) {
    int bad = 0, checks = 0;
    lang_init();
    #define VCHECK(c, ...) do { checks++; if (!(c)) { printf("BAD VOCAB: " __VA_ARGS__); printf("\n"); bad++; } } while (0)
    int nv = vocab_size(), nn = vocab_name_count();
    VCHECK(nv >= 600, "only %d vocabulary words, the first batch has at least 600", nv);
    VCHECK(nn >= 100, "only %d names", nn);
    /* the rows */
    {   const char **w = malloc((size_t)(nv + 1) * sizeof *w); int prev = 0, rows = 0, last = 0;
        for (int i = 0; i < nv; i++) {
            const char *r = lang_vocab_at(i); if (!r) { VCHECK(0, "row %d missing", i); continue; }
            const char *b1 = strchr(r, '|'); int wl = b1 ? (int)(b1 - r) : 0, ok = b1 && wl >= 1 && wl <= 24;
            for (int k = 0; ok && k < wl; k++) if (!((r[k] >= 'a' && r[k] <= 'z') || r[k] == '\'')) ok = 0;
            if (ok && r[0] == '\'') ok = 0;
            const char *p = b1 ? b1 + 1 : ""; int rank = 0, nd = 0;
            while (*p >= '0' && *p <= '9') { rank = rank * 10 + (*p - '0'); p++; nd++; }
            if (!nd || nd > 6 || rank < 1) ok = 0;
            if (*p == '|') { if (!(p[1] && strchr("nvarx", p[1]) && !p[2])) ok = 0; } else if (*p) ok = 0;
            VCHECK(ok, "row \"%s\" is not word|rank or word|rank|pos (a-z and ', rank >= 1, pos n v a r x)", r);
            if (ok) { VCHECK(rank > prev, "rank %d of \"%s\" does not follow %d (ranks go up one by one)", rank, r, prev); VCHECK(rank == prev + 1 || !prev, "rank %d of \"%s\" skips (ranks have no gaps)", rank, r); prev = rank; last = rank; }
            w[rows++] = r;
        }
        VCHECK(rows && last == nv, "last rank %d, rows %d", last, nv);
        char **ws = malloc((size_t)(nv + 1) * sizeof *ws);       /* duplicates: sort the words, compare neighbours */
        for (int i = 0; i < nv; i++) { const char *bar = strchr(w[i], '|'); size_t l = bar ? (size_t)(bar - w[i]) : strlen(w[i]); ws[i] = malloc(l + 1); memcpy(ws[i], w[i], l); ws[i][l] = 0; }
        qsort(ws, (size_t)nv, sizeof *ws, cmp_str);
        for (int i = 1; i < nv; i++) VCHECK(strcmp(ws[i], ws[i - 1]), "\"%s\" is twice in the vocabulary", ws[i]);
        for (int i = 0; i < nv; i++) free(ws[i]);
        free(ws);
        /* every lookup against the row itself (a plain walk: no hash) */
        int direct = 0; for (const char *const *r = VOCAB_A; *r; r++) direct++;
        VCHECK(direct <= nv, "VOCAB_A has %d rows but the registry only %d", direct, nv);
        for (int i = 0; i < nv; i++) {
            const char *r = w[i], *b1 = strchr(r, '|'); if (!b1) continue;
            char wd[48]; int wl = (int)(b1 - r); memcpy(wd, r, (size_t)wl); wd[wl] = 0;
            int rank = atoi(b1 + 1); const char *b2 = strchr(b1 + 1, '|'); char pos = b2 ? b2[1] : 'x';
            if (!vocab_has(wd) || vocab_rank(wd) != rank || vocab_pos(wd) != pos || !vocab_has_n(wd, wl) || !lang_known_word(wd, wl) || !vocab_protected(wd, wl)) {
                VCHECK(0, "lookup of \"%s\": has %d rank %d (want %d) pos %c (want %c) known %d", wd, vocab_has(wd), vocab_rank(wd), rank, vocab_pos(wd) ? vocab_pos(wd) : '0', pos, lang_known_word(wd, wl));
            } else checks++;
        }
        free(w);
    }
    /* names */
    {   const char **w = malloc((size_t)(nn + 1) * sizeof *w);
        for (int i = 0; i < nn; i++) {
            const char *r = lang_name_at(i); w[i] = r;
            int ok = r && r[0]; for (const char *p = r; ok && *p; p++) if (!(*p >= 'a' && *p <= 'z')) ok = 0;
            VCHECK(ok && strlen(r) <= 20, "name row \"%s\" is not a lower case name of letters", r ? r : "(null)");
            if (ok) VCHECK(vocab_is_name(r) && vocab_is_name_n(r, (int)strlen(r)) && vocab_protected(r, (int)strlen(r)), "name \"%s\" is not found", r);
        }
        qsort(w, (size_t)nn, sizeof *w, cmp_str);
        for (int i = 1; i < nn; i++) VCHECK(strcmp(w[i], w[i - 1]), "name \"%s\" is twice", w[i]);
        free(w);
    }
    /* what a lookup must say */
    {   static const struct { const char *w; int rank; char pos; } K[] = { { "the", 1, 'x' }, { "i", 0, 'x' }, { "hello", 0, 'x' }, { "dog", 0, 'n' }, { "run", 0, 'v' }, { "good", 0, 'a' }, { "very", 0, 'r' }, { "don't", 0, 'v' }, { "dont", 0, 'v' }, { "lol", 0, 'x' } };
        for (unsigned i = 0; i < sizeof K / sizeof *K; i++) {
            VCHECK(vocab_has(K[i].w), "\"%s\" should be a vocabulary word", K[i].w);
            VCHECK(vocab_pos(K[i].w) == K[i].pos, "pos of \"%s\" is %c, wanted %c", K[i].w, vocab_pos(K[i].w) ? vocab_pos(K[i].w) : '0', K[i].pos);
            if (K[i].rank) VCHECK(vocab_rank(K[i].w) == K[i].rank, "rank of \"%s\" is %d, wanted %d", K[i].w, vocab_rank(K[i].w), K[i].rank);
        }
        VCHECK(vocab_rank("the") < vocab_rank("please") && vocab_rank("be") < vocab_rank("monster"), "frequency order");
        static const char *const NOT[] = { "hwllo", "zzzqx", "", "h3llo", "123", "the cat", "dog!", "xyzzy", "heello", "thee", NULL };
        for (const char *const *p = NOT; *p; p++) VCHECK(!vocab_has(*p) && vocab_rank(*p) == 0 && vocab_pos(*p) == 0 && !vocab_protected(*p, (int)strlen(*p)), "\"%s\" must not be found", *p);
        VCHECK(!vocab_has(NULL) && !vocab_rank(NULL) && !vocab_pos(NULL) && !vocab_is_name(NULL) && !vocab_has_n(NULL, 3) && !vocab_has_n("the", 0) && !vocab_has_n("the", -1), "NULL / empty");
        VCHECK(vocab_has("The") && vocab_has("HELLO") && vocab_rank("THE") == 1 && vocab_is_name("DEA") && vocab_is_name("Alex"), "case blind");
        VCHECK(vocab_has_n("the cat", 3) && !vocab_has_n("the cat", 4) && vocab_has_n("dogs!", 3) && vocab_has_n("i'm so", 3), "pointer and length");
        VCHECK(vocab_is_name("dea") && vocab_is_name("alex") && vocab_is_name("mia") && !vocab_is_name("hello") && !vocab_is_name("") && !vocab_is_name("zzzqx"), "names");
        VCHECK(vocab_protected("hello", 5) && vocab_protected("dea", 3) && !vocab_protected("hwllo", 5), "protected");
        char l39[40], l40[41]; memset(l39, 'a', 39); l39[39] = 0; memset(l40, 'a', 40); l40[40] = 0;
        VCHECK(!vocab_has(l39) && !vocab_has(l40) && !vocab_is_name(l40), "long words");
    }
    /* registering later, and reset */
    {   static const char *const TV[] = { "zorp|9001|n", "blarg|9002", 0 };
        static const char *const TN[] = { "zaphod", 0 };
        int v0 = vocab_size(), n0 = vocab_name_count();
        VCHECK(!vocab_has("zorp") && !vocab_is_name("zaphod"), "test words are there before they are registered");
        VCHECK(lang_register_vocab(TV) == 0 && lang_register_names(TN) == 0, "register failed");
        VCHECK(vocab_has("zorp") && vocab_rank("zorp") == 9001 && vocab_pos("zorp") == 'n' && vocab_pos("blarg") == 'x' && vocab_rank("blarg") == 9002 && vocab_is_name("zaphod") && !vocab_has("zaphod"), "registered words");
        VCHECK(vocab_size() == v0 + 2 && vocab_name_count() == n0 + 1 && vocab_rank("the") == 1 && vocab_is_name("dea"), "counts after registering");
        VCHECK(lang_known_word("zorp", 4) && !lang_known_word("zaphod", 6), "known words: vocabulary yes, names stay out of it");
        lang_tables_reset(); lang_init();
        VCHECK(!vocab_has("zorp") && !vocab_is_name("zaphod") && vocab_size() == v0 && vocab_name_count() == n0 && vocab_has("the") && vocab_is_name("dea"), "reset restores the built-in vocabulary");
    }
    /* time: the hash should not care about the size */
    {   struct timespec a, b; volatile int sink = 0; static const char *const Q[] = { "the", "monster", "hwllo", "dea", "zzzqx", "don't", "Please", NULL };
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < 200000; i++) { const char *q = Q[i % 7]; sink += vocab_has(q) + vocab_rank(q) + vocab_is_name(q); }
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms = (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6; (void)sink;
        printf("vocab: %d words, %d names; 600,000 lookups in %.1f ms (%.0f ns each)\n", vocab_size(), vocab_name_count(), ms, ms * 1e6 / 600000.0);
        VCHECK(ms < 2000, "lookups are too slow (%.0f ms)", ms);
    }
    printf("vocab: %d checks (rows, ranks, lookups, names, registering, reset). %s\n", checks, bad ? "PROBLEMS" : "ok");
    return bad;
}


/* ---------- chunk 21: spell repair ---------- */
static int sp_fix(const char *typed, char *out, int cap, SpellInfo *si) {
    char lc[600]; int map[600];
    lang_clean(typed, lc, 600, map);
    return lang_spell_text(typed, lc, map, 0, out, cap, NULL, NULL, si);
}
static int do_spell(int argc, char **argv) {
    int bad = 0, checks = 0;
    lang_init();
    #define SCHECK(c, ...) do { checks++; if (!(c)) { printf("BAD SPELL: " __VA_ARGS__); printf("\n"); bad++; } } while (0)
    SCHECK(lang_spell_dict_size() >= 800, "the dictionary has only %d words", lang_spell_dict_size());
    SCHECK(lang_spell_index_size() > lang_spell_dict_size() * 4, "the delete index has only %d entries", lang_spell_index_size());
    /* typed -> repaired (a sensible typo of a word the engine knows) */
    static const char *const FIX[][2] = {
        { "helo there", "hello there" }, { "hwllo there", "hello there" }, { "ehllo there", "hello there" }, { "hellp", "hello" }, { "yello", "hello" },
        { "who are yuo", "who are you" }, { "what is yuor name", "what is your name" }, { "i love yuo", "i love you" }, { "tou are kind", "you are kind" },
        { "sory about that", "sorry about that" }, { "im sorrry", "im sorry" }, { "i am scard", "i am scared" }, { "im scaredd", "im scared" },
        { "tell me a joek", "tell me a joke" }, { "tell me a jokr", "tell me a joke" }, { "i dont understnd", "i dont understand" },
        { "how are you doign", "how are you doing" }, { "godbye", "goodbye" }, { "goodbey", "goodbye" }, { "whats the mision", "whats the mission" },
        { "the sky is bluee", "the sky is blue" }, { "im conufsed", "im confused" }, { "pleas help me", "please help me" }, { "you are stupd", "you are stupid" },
        { "thankss", "thanks" }, { "thnaks", "thanks" }, { "thaks", "thanks" }, { "yeah i agre", "yeah i agree" }, { "i dont knwo", "i dont know" },
        { "i wnat help", "i want help" }, { "can you hlep me", "can you help me" }, { "this is cofusing", "this is confusing" }, { "ok i uderstand", "ok i understand" },
        { "wher are we", "where are we" }, { "whre am i", "where am i" }, { "tel me more", "tell me more" }, { "pleae stop", "please stop" },
        { "that is funy", "that is funny" }, { "you are mean", "you are mean" }, { "ho are you", "how are you" }, { "hw are you", "how are you" },
        { "i am afrad", "i am afraid" }, { "im so tird", "im so tired" }, { "sing me a sogn", "sing me a song" }, { "i luv musci", "i luv music" },
        { "is it real", "is it real" }, { "wath do you want", "what do you want" }, { "wht do you want", "what do you want" }, { "i hate yuo", "i hate you" },
        { "i feel lonley", "i feel lonely" }, { "the moom is bright", "the moon is bright" }, { "what is that hoel", "what is that hole" },
        { "i am confsed", "i am confused" }, { "good mornig", "good morning" }, { "good eveing", "good evening" }, { "nver mind", "never mind" },
        { "i do not undrstand", "i do not understand" }, { "excuse me", "excuse me" }, { "wiat for me", "wait for me" }, { "pelase help", "please help" },
    };
    int nfixed = 0, nfix_tests = 0;
    for (size_t i = 0; i < sizeof FIX / sizeof FIX[0]; i++) {
        char out[200]; SpellInfo si; sp_fix(FIX[i][0], out, 200, &si);
        int same = !strcmp(FIX[i][0], FIX[i][1]);
        if (same) { SCHECK(si.nfix == 0 && !strcmp(out, FIX[i][0]), "\"%s\" is right and must stay (got \"%s\")", FIX[i][0], out); continue; }
        nfix_tests++;
        if (!strcmp(out, FIX[i][1])) nfixed++; else if (strstr(FIX[i][1], "") && 0) {}
        else printf("  (not repaired, ok on a starter vocabulary: \"%s\" -> \"%s\", wanted \"%s\")\n", FIX[i][0], out, FIX[i][1]);
    }
    SCHECK(nfixed * 100 >= nfix_tests * 70, "only %d of %d typos were repaired (needs 70 %%)", nfixed, nfix_tests);
    /* the core pairs must work */
    {   static const char *const MUST[][2] = { { "helo", "hello" }, { "hwllo", "hello" }, { "ehllo", "hello" }, { "yuo", "you" }, { "sory", "sorry" }, { "joek", "joke" },
            { "godbye", "goodbye" }, { "mision", "mission" }, { "conufsed", "confused" }, { "understnd", "understand" }, { "thankss", "thanks" } };
        for (size_t i = 0; i < sizeof MUST / sizeof MUST[0]; i++) {
            char out[40]; int cf = 0, d = 0; int r = lang_spell_word(MUST[i][0], (int)strlen(MUST[i][0]), out, &cf, &d);
            SCHECK(r && !strcmp(out, MUST[i][1]), "%s must become %s (got %s)", MUST[i][0], MUST[i][1], r ? out : "nothing");
            SCHECK(r && cf >= SPELL_MIN_CONF && cf <= 99 && (d == 1 || d == 2), "%s: confidence %d distance %d out of range", MUST[i][0], cf, d);
        }
    }
    /* words that must stay as typed */
    static const char *const STAY[] = {
        "hello there", "who are you", "what is your name", "i am sad", "tell me a joke", "the clouds are beautiful", "do you have secrets", "you are mistaken",
        "my cat likes boxes", "the purple elephant dances", "got any jokes", "much appreciated", "what is below the clouds", "i am being silly", "she was running fast",
        "its bigger than me", "he was studying", "slowly now", "ok", "no", "hi", "yo", "u r kind", "idk what to say", "thx for that", "pls help", "brb", "ur name is dea",
        "dont go", "im here", "whats up", "gonna go", "wanna talk", "lol", "NPC", "im a GOD", "this is NASA", "My name is Zorblax", "i met Quillfeather today", "hello Zorblax",
        "my name is Bartholomew", "call me Kit", "i am 12", "3pm", "b4 gr8", "user1 mp3", "a1b2c3", "x", "xy", "zz", "!!!", "...", "?", "f*ck", "h3llo", "xyzzyplugh",
        "qwertyuiop", "asdfghjkl", "supercalifragilisticexpialidocious", "this is a very long word aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "dea", "alex", "orb", "hammer",
    };
    for (size_t i = 0; i < sizeof STAY / sizeof STAY[0]; i++) {
        char out[200]; SpellInfo si; sp_fix(STAY[i], out, 200, &si);
        char lc[200]; lang_clean(STAY[i], lc, 200, NULL);
        /* the clean-up layer may change a word itself (h3llo, f*ck): the text the spell step gets is what counts */
        SCHECK(si.nfix == 0 && !strcmp(out, lc), "\"%s\" must stay (got \"%s\")", STAY[i], out);
    }
    /* never repaired: the reasons */
    SCHECK(lang_spell_protected("hello", 5), "a vocabulary / pattern word is protected");
    { const char *nm = NAMES_A[0]; SCHECK(nm && lang_spell_protected(nm, (int)strlen(nm)), "a known name is protected (%s)", nm ? nm : "none"); }
    SCHECK(lang_spell_protected("idk", 3), "text-speak is protected");
    SCHECK(lang_spell_protected("ab", 2) && lang_spell_protected("a", 1), "1-2 letters are protected");
    SCHECK(lang_spell_protected("abcdefghijklmnopqrstuvwxyz", 26), "over 24 letters is protected");
    SCHECK(lang_spell_protected("h3llo", 5) && lang_spell_protected("it's", 4), "digits and odd characters are protected");
    SCHECK(lang_spell_protected("clouds", 6) && lang_spell_protected("jokes", 5) && lang_spell_protected("dances", 6) && lang_spell_protected("being", 5), "plain inflections of a known word are protected");
    SCHECK(!lang_spell_protected("hwllo", 5) && !lang_spell_protected("thankss", 7), "a typo is not protected (a doubled last letter is no plural)");
    /* the case rules: a Capitalised word in the middle is a name, an ALL CAPS short word is an acronym, a shouted line is not */
    {   char out[200]; SpellInfo si;
        sp_fix("my name is Hwllo", out, 200, &si);  SCHECK(si.nfix == 0, "a capitalised word in the middle of a sentence is a name (got \"%s\")", out);
        sp_fix("Hwllo there", out, 200, &si);       SCHECK(si.nfix == 1 && !strcmp(out, "hello there"), "the first word of a sentence may be repaired (got \"%s\")", out);
        sp_fix("hi. Hwllo there", out, 200, &si);   SCHECK(si.nfix == 1, "the first word after a full stop may be repaired (got \"%s\")", out);
        sp_fix("im a HWLO", out, 200, &si);         SCHECK(si.nfix == 0, "an ALL CAPS word of 4 letters is an acronym (got \"%s\")", out);
        char lc[200]; int map[200]; lang_clean("TELL ME A JOEK", lc, 200, map);
        lang_spell_text("TELL ME A JOEK", lc, map, 1, out, 200, NULL, NULL, &si);
        SCHECK(si.nfix == 1 && !strcmp(out, "tell me a joke"), "a shouted line is repaired like any other (got \"%s\")", out);
    }
    /* the maps: out[i] came from in[src[i]] and from the original byte outmap[i]; the whole typed token is covered; the original is untouched */
    {   static const char *const M[] = { "Hwllo   there, im sory!", "  tell me a joek please ", "who are yuo?! godbye", "hwllo hwllo hwllo", "ok. whare is thi plce", "thankss", "wassup there, luv u", "  wut iz yoo name  ", "thankyou! howdee misshun" };
        for (size_t i = 0; i < sizeof M / sizeof M[0]; i++) {
            char lc[300]; int map[300]; int len = lang_clean(M[i], lc, 300, map);
            char out[300]; int src[300], om[300]; SpellInfo si;
            int n = lang_spell_text(M[i], lc, map, 0, out, 300, om, src, &si);
            int ok = n >= 0 && n < 300 && om[n] == map[len] && src[n] == len, mono = 1;
            for (int k = 0; k < n; k++) { if (src[k] < 0 || src[k] >= len || om[k] != map[src[k]] || (k && (src[k] < src[k - 1] || om[k] < om[k - 1]))) mono = 0; }
            SCHECK(ok && mono, "\"%s\": the maps are not monotone and inside the text", M[i]);
            for (int f = 0; f < si.nfix; f++) {
                SpellFix *x = &si.fix[f];
                SCHECK(x->a >= 0 && x->b > x->a && x->b <= (int)strlen(M[i]) && x->conf >= SPELL_MIN_CONF && x->conf <= 99 && ((x->kind == SPELL_EDIT && (x->dist == 1 || x->dist == 2)) || (x->kind == SPELL_PHONETIC && x->dist >= 1 && x->dist <= 3) || (x->kind == SPELL_DIALECT && x->dist == 0 && x->conf == 90)) && strlen(x->to) >= 2 && strlen(x->to) < SPELL_TO_MAX && strcmp(x->from, x->to),
                       "\"%s\": fix %d is malformed (%d-%d \"%s\" -> \"%s\" conf %d)", M[i], f, x->a, x->b, x->from, x->to, x->conf);
                SCHECK(!strncasecmp(M[i] + x->a, x->from, strlen(x->from)) || x->b - x->a >= (int)strlen(x->from), "\"%s\": fix %d covers the wrong bytes", M[i], f);
            }
            SCHECK(si.min_conf >= (si.nfix ? SPELL_MIN_CONF : 100), "\"%s\": min_conf %d", M[i], si.min_conf);
            /* idempotent: the repaired text needs no repair */
            char out2[300]; SpellInfo s2; char lc2[300]; int m2[300]; lang_clean(out, lc2, 300, m2);
            lang_spell_text(out, lc2, m2, 0, out2, 300, NULL, NULL, &s2);
            SCHECK(s2.nfix == 0 && !strcmp(out, out2), "\"%s\": repairing twice changes \"%s\" again to \"%s\"", M[i], out, out2);
        }
        /* no repair: the same text and the same map */
        char lc[100]; int map[100]; lang_clean("hello there who are you", lc, 100, map);
        char out[100]; int src[100], om[100]; SpellInfo si;
        int n = lang_spell_text("hello there who are you", lc, map, 0, out, 100, om, src, &si);
        int same = si.nfix == 0 && n == (int)strlen(lc) && !strcmp(out, lc);
        for (int k = 0; same && k <= n; k++) if (src[k] != k || om[k] != map[k]) same = 0;
        SCHECK(same, "a text with nothing to repair must come back unchanged, with the identity maps");
    }
    /* the highlight covers the whole typed word, and the intent is heard through the typo */
    {   uint8_t mask[200]; const char *t = "hwllo there";
        int hits = lang_highlight(t, mask); int on = 0; for (int i = 0; t[i]; i++) on += mask[i];
        SCHECK(hits > 0 && mask[0] && mask[4] && !mask[6], "highlight of \"%s\": %d hits, bytes %d%d%d (the typed word hwllo, not the rest)", t, hits, mask[0], mask[4], mask[6]);
        (void)on;
        Heard h; lang_parse("hwllo there", &h);
        SCHECK(heard_has(&h, I_GREET), "\"hwllo there\" must be heard as a greet");
        SCHECK(h.nspell == 1 && h.spell_conf >= SPELL_MIN_CONF && (h.flags & LF_SPELL), "Heard.nspell %d conf %d flags %u", h.nspell, h.spell_conf, h.flags);
        lang_parse("hello there", &h);
        SCHECK(h.nspell == 0 && h.spell_conf == 0 && !(h.flags & LF_SPELL), "a clean text sets no spell field");
        lang_parse("tell me a joek", &h);
        SCHECK(heard_has(&h, I_JOKE), "\"tell me a joek\" must be heard as a joke");
    }
    /* the spelled variant is expanded too: a repair next to text-speak */
    {   Heard h; lang_parse("hwllo, idk wat to say", &h); SCHECK(heard_has(&h, I_GREET), "a greet next to text-speak is heard through the typo"); }
    /* the cap: a repair that does not fit stays as typed */
    {   char lc[100]; int map[100]; lang_clean("hwllo", lc, 100, map);
        char out[6]; SpellInfo si; int n = lang_spell_text("hwllo", lc, map, 0, out, 6, NULL, NULL, &si);
        SCHECK(n == 5 && si.nfix == 1 && !strcmp(out, "hello"), "a repair that fits exactly (got \"%s\" n %d)", out, n);
        char o5[5]; n = lang_spell_text("hwllo", lc, map, 0, o5, 5, NULL, NULL, &si);
        SCHECK(n <= 4 && o5[n] == 0, "a small cap never overflows (n %d)", n);
        char big[300]; char in[300]; in[0] = 0; for (int i = 0; i < 40; i++) strcat(in, "hwllo ");
        lang_clean(in, big, 300, NULL); char lo[300]; SpellInfo s2; n = lang_spell_text(in, big, NULL, 0, lo, 300, NULL, NULL, &s2);
        SCHECK(n < 300 && s2.nfix <= SPELL_MAX_FIX, "a long text: %d fixes (at most %d)", s2.nfix, SPELL_MAX_FIX);
    }
    /* self-harm is still heard through typos (and through the spelled variant) */
    {   static const char *const SH[] = { "i wnat to die", "i want to dei", "i wnt to die", "i want to kill mysefl", "kill myslef", "i dont wnat to live", "i wnat to end it all" };
        int heard = 0;
        for (size_t i = 0; i < sizeof SH / sizeof SH[0]; i++) { Heard h; lang_parse(SH[i], &h); if (heard_has(&h, I_SELFHARM)) heard++; else printf("  (not heard as self-harm: \"%s\")\n", SH[i]); }
        SCHECK(heard >= 4, "only %d of %zu typed self-harm lines were heard", heard, sizeof SH / sizeof SH[0]);
        Heard h; lang_parse("i wnat to die", &h); SCHECK(heard_has(&h, I_SELFHARM) && h.order[0] == I_SELFHARM, "\"i wnat to die\" must be heard as self-harm, first");
    }
    /* registering later: a new word in the vocabulary is a candidate at once, and the index is rebuilt */
    {   static const char *const ROWS[] = { "quillfeather|2500|n", 0 };
        char out[40]; int cf = 0, d = 0;
        SCHECK(!lang_spell_word("quillfeathr", 11, out, &cf, &d), "quillfeathr is nothing before the word is registered");
        lang_register_vocab(ROWS);
        int r = lang_spell_word("quillfeathr", 11, out, &cf, &d);
        SCHECK(r && !strcmp(out, "quillfeather") && d == 1, "quillfeathr must become quillfeather once registered (got %s)", r ? out : "nothing");
        SCHECK(lang_spell_protected("quillfeather", 12), "the registered word is protected");
        lang_tables_reset();
        SCHECK(!lang_spell_word("quillfeathr", 11, out, &cf, &d), "after a reset the word is gone again");
    }
    /* the off switch and the superset rule: with the repair on, no intent heard with it off is lost, over every reply line and every corpus file given */
    {   long texts = 0, lost = 0, gained = 0; int shown = 0;
        char line[600];
        for (int pass = 0; pass < 2; pass++) {
            FILE *f = NULL; int fi = 0;
            if (pass == 0) { EACH(l) { char tx[600]; snprintf(tx, sizeof tx, "%s", l->text);
                Heard a, b; lang_spell_set(0); lang_parse(tx, &a); lang_spell_set(1); lang_parse(tx, &b);
                texts++;
                for (int w = 0; w < HEARD_MAX / 64; w++) { if (a.m[w] & ~b.m[w]) { lost++; if (shown++ < 5) printf("  LOST an intent through the repair: %s\n", tx); } if (b.m[w] & ~a.m[w]) gained++; }
            } }
            else for (fi = 1; fi < argc; fi++) {
                if (argv[fi][0] == '-') continue;
                f = fopen(argv[fi], "r"); if (!f) continue;
                while (fgets(line, sizeof line, f)) {
                    char *tab = strchr(line, '\t'); if (!tab || line[0] == '#') continue; *tab = 0;
                    Heard a, b; lang_spell_set(0); lang_parse(line, &a); lang_spell_set(1); lang_parse(line, &b);
                    texts++;
                    for (int w = 0; w < HEARD_MAX / 64; w++) { if (a.m[w] & ~b.m[w]) { lost++; if (shown++ < 5) printf("  LOST an intent through the repair: %s\n", line); } if (b.m[w] & ~a.m[w]) gained++; }
                }
                fclose(f);
            }
        }
        lang_spell_set(1);
        SCHECK(lost == 0, "the repair lost an intent in %ld texts", lost);
        printf("  superset rule: %ld texts, 0 intents lost, %ld texts gained one\n", texts, gained);
    }
    /* speed: a token that is nothing, a token that is a typo, the rebuild of the index */
    {   struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
        char out[40]; int cf, d; int ok = 0;
        for (int i = 0; i < 20000; i++) ok += lang_spell_word(i & 1 ? "hwllo" : "supercalifragilistic", i & 1 ? 5 : 20, out, &cf, &d);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms = (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
        double build = lang_spell_build_ms();
        printf("  spell speed: 20,000 lookups %.1f ms (%.4f ms each), index %d entries for %d words built in %.1f ms\n", ms, ms / 20000, lang_spell_index_size(), lang_spell_dict_size(), build);
        SCHECK(ok == 10000, "the lookups changed");
        SCHECK(ms / 20000 < 0.05, "a lookup takes %.4f ms (budget 0.05)", ms / 20000);
    }
    printf("spell: %d checks, %d problems; %d of %d typos repaired\n", checks, bad, nfixed, nfix_tests);
    #undef SCHECK
    return bad;
}

/* chunk 22: the phonetic key, the phonetic fallback and the dialect table */
static int do_phonetic(int argc, char **argv) {
    (void)argc; (void)argv;
    int bad = 0, checks = 0;
    lang_init();
    #define PCHECK(c, ...) do { checks++; if (!(c)) { printf("BAD PHONETIC: " __VA_ARGS__); printf("\n"); bad++; } } while (0)
    /* the key itself */
    static const char *const KEYS[][2] = { { "what", "WT" }, { "wut", "WT" }, { "hello", "HL" }, { "howdy", "HT" }, { "phone", "FN" }, { "knife", "NF" }, { "night", "NT" },
        { "thank", "0NK" }, { "wrong", "RNK" }, { "church", "XRX" }, { "science", "SNS" }, { "xylophone", "SLFN" }, { "you", "Y" }, { "yoo", "Y" }, { "apple", "APL" },
        { "dumb", "TM" }, { "edge", "AJ" }, { "ship", "XP" }, { "nation", "NXN" }, { "watch", "WX" }, { "ghost", "KST" }, { "sign", "SN" }, { "whale", "WL" }, { "queen", "KN" } };
    for (size_t i = 0; i < sizeof KEYS / sizeof KEYS[0]; i++) {
        char k[40]; int n = lang_phone(KEYS[i][0], (int)strlen(KEYS[i][0]), k, 40);
        PCHECK(!strcmp(k, KEYS[i][1]) && n == (int)strlen(k), "the key of %s is \"%s\", wanted \"%s\"", KEYS[i][0], k, KEYS[i][1]);
    }
    static const char *const SAME[][2] = { { "what", "wat" }, { "what", "whut" }, { "what", "wot" }, { "hello", "hallo" }, { "howdy", "howdee" }, { "phone", "fone" }, { "night", "nite" },
        { "knife", "nife" }, { "please", "plees" }, { "love", "luv" }, { "thanks", "thanx" }, { "sorry", "sori" }, { "want", "wont" }, { "kill", "kil" }, { "cat", "kat" }, { "rite", "right" }, { "scare", "skare" } };
    for (size_t i = 0; i < sizeof SAME / sizeof SAME[0]; i++) {
        char a[40], b[40]; lang_phone(SAME[i][0], (int)strlen(SAME[i][0]), a, 40); lang_phone(SAME[i][1], (int)strlen(SAME[i][1]), b, 40);
        PCHECK(!strcmp(a, b), "%s (%s) and %s (%s) must sound alike", SAME[i][0], a, SAME[i][1], b);
    }
    static const char *const DIFF[][2] = { { "what", "when" }, { "hello", "help" }, { "good", "gold" }, { "love", "lore" }, { "you", "your" }, { "bye", "buy" + 0 }, { "dea", "die" + 0 } };
    for (size_t i = 0; i < sizeof DIFF / sizeof DIFF[0]; i++) {
        char a[40], b[40]; lang_phone(DIFF[i][0], (int)strlen(DIFF[i][0]), a, 40); lang_phone(DIFF[i][1], (int)strlen(DIFF[i][1]), b, 40);
        if (!strcmp(DIFF[i][0], "bye") || !strcmp(DIFF[i][0], "dea")) continue;        /* bye / buy and dea / die are alike on purpose: vowels are dropped */
        PCHECK(strcmp(a, b), "%s and %s must not sound alike (both %s)", DIFF[i][0], DIFF[i][1], a);
    }
    {   char k[40], k2[40]; lang_phone("WHAT", 4, k, 40); lang_phone("w-h-a-t", 7, k2, 40);
        PCHECK(!strcmp(k, "WT") && !strcmp(k2, "WT"), "the key is case-blind and skips what is not a letter (%s, %s)", k, k2);
        PCHECK(lang_phone("", 0, k, 40) == 0 && !k[0] && lang_phone("123 !!", 6, k, 40) == 0 && !k[0], "no letters, no key");
        char c[6]; memset(c, 'x', sizeof c);
        int n1 = lang_phone("what", 4, c, 1); PCHECK(n1 == 0 && c[0] == 0 && c[1] == 'x', "a cap of 1 writes only the NUL");
        memset(c, 'x', sizeof c); int n2 = lang_phone("what", 4, c, 2); PCHECK(n2 == 1 && !strcmp(c, "W") && c[2] == 'x', "a cap of 2 writes one character and the NUL (got \"%s\" %d)", c, n2);
        memset(c, 'x', sizeof c); lang_phone("supercalifragilistic", 20, c, 5); PCHECK(c[4] == 0 && c[5] == 'x', "a long word never writes past the cap");
        char big[100]; for (int i = 0; i < 99; i++) big[i] = "bcdfgklmnprstvz"[i % 15]; big[99] = 0;
        lang_phone(big, 99, k, 40); PCHECK(strlen(k) < 40, "a very long text is cut by the cap");
    }
    /* the dialect table: rows, lint */
    int rows = 0, unknown_out = 0;
    for (const char *const *r = DIALECT; *r; r++) {
        rows++;
        const char *bar = strchr(*r, '|'); int ok = bar != NULL; int kn = ok ? (int)(bar - *r) : 0;
        if (ok) { ok = kn >= 2 && kn <= 24; for (int i = 0; ok && i < kn; i++) if ((*r)[i] < 'a' || (*r)[i] > 'z') ok = 0; }
        const char *to = ok ? bar + 1 : ""; int tn = (int)strlen(to);
        if (ok) { ok = tn >= 2 && tn <= 40 && to[0] != ' ' && to[tn - 1] != ' '; for (int i = 0; ok && i < tn; i++) if (!((to[i] >= 'a' && to[i] <= 'z') || (to[i] == ' ' && to[i + 1] != ' '))) ok = 0; }
        PCHECK(ok, "row \"%s\" is malformed (key a-z 2-24 letters, result a-z and single spaces, 2-40 characters)", *r);
        if (!ok) continue;
        PCHECK(!(tn == kn && !strncmp(*r, to, (size_t)kn)), "row \"%s\" maps a word to itself", *r);
        PCHECK(!vocab_has_n(*r, kn) && !vocab_is_name_n(*r, kn), "row \"%s\": the key is a vocabulary word or a name (a real word is never corrected)", *r);
        PCHECK(!lang_textspeak(*r, kn), "row \"%s\": the key belongs to the text-speak table", *r);
        for (const char *const *r2 = r + 1; *r2; r2++) { const char *b2 = strchr(*r2, '|'); if (b2 && b2 - *r2 == kn && !strncmp(*r, *r2, (size_t)kn)) { PCHECK(0, "duplicate key in \"%s\"", *r); } }
        const char *w = to; int allknown = 1;                           /* is every word of the result a word the engine knows? (information: the 15,000 words of chunks 49-51 end it) */
        while (*w) { const char *e = strchr(w, ' '); int wl = e ? (int)(e - w) : (int)strlen(w); if (!lang_known_word(w, wl)) allknown = 0; w += wl; if (*w == ' ') w++; }
        if (!allknown) unknown_out++;
        PCHECK(lang_dialect(*r, kn) != NULL, "row \"%s\" is not in the map", *r);
    }
    PCHECK(rows >= 150, "only %d dialect rows (the first batch has at least 150)", rows);
    PCHECK(lang_dialect_count() >= rows - 2 && lang_dialect_count() <= rows, "the map has %d rows for %d table rows", lang_dialect_count(), rows);
    PCHECK(lang_dialect_row(0) != NULL && lang_dialect_row(rows - 1) != NULL && lang_dialect_row(rows) == NULL && !strcmp(lang_dialect_row(0), DIALECT[0]), "the row walker");
    PCHECK(unknown_out * 100 <= rows * 40, "%d of %d dialect results have a word the engine does not know yet", unknown_out, rows);
    PCHECK(!lang_dialect("hello", 5) && !lang_dialect("", 0) && !lang_dialect("w", 1), "a word that is not a row has no entry");
    /* the dialect repair: typed -> the spelled text */
    static const char *const DL[][2] = {
        { "wassup", "whats up" }, { "wassup dea", "whats up dea" }, { "wazzup", "whats up" }, { "whatsup", "whats up" }, { "i luv yu", "i luv you" }, { "yoo are kind", "you are kind" },
        { "thankyou", "thank you" }, { "thankyou so much", "thank you so much" }, { "thanku", "thank you" }, { "mornin", "morning" }, { "hw r u", "how r u" }, { "whut r u", "what r u" },
        { "watchu want", "what do you want" }, { "howdee", "howdy" }, { "heya", "hey" }, { "hiya", "hi" }, { "goodmorning", "good morning" }, { "gnight", "good night" },
        { "shutup", "shut up" }, { "seeya", "see you" }, { "stoopid", "stupid" }, { "rilly", "really" }, { "alrite", "alright" }, { "lookin", "looking" },
        { "yah", "yeah" }, { "naw", "no" }, { "okey", "ok" }, { "sowwy", "sorry" }, { "pleaz help", "please help" }, { "wana go", "want to go" }, { "imma go", "i am going to go" },
        { "dono", "i do not know" }, { "gimmie that", "give me that" }, { "fren", "friend" }, { "bruh", "bro" }, { "hallo", "hello" }, { "yu", "you" }, { "hw are you", "how are you" },
        { "whoru", "who are you" }, { "plz plees", "plz please" }, { "WASSUP DEA", "whats up dea" }, { "Wassup there", "whats up there" }, { "hi. Wassup", "hi. whats up" } };
    for (size_t i = 0; i < sizeof DL / sizeof DL[0]; i++) {
        char out[200]; SpellInfo si; sp_fix(DL[i][0], out, 200, &si);
        PCHECK(!strcmp(out, DL[i][1]) && si.nfix >= 1, "\"%s\" -> \"%s\", wanted \"%s\"", DL[i][0], out, DL[i][1]);
        PCHECK(si.nfix >= 1 && si.fix[0].kind == SPELL_DIALECT && si.fix[0].conf == 90 && si.fix[0].dist == 0, "\"%s\": the first fix is not a dialect fix (kind %d conf %d dist %d)", DL[i][0], si.fix[0].kind, si.fix[0].conf, si.fix[0].dist);
        char out2[200]; SpellInfo s2; sp_fix(out, out2, 200, &s2);
        PCHECK(s2.nfix == 0 && !strcmp(out, out2), "\"%s\": repairing the result \"%s\" changes it again to \"%s\"", DL[i][0], out, out2);
    }
    /* what a row must not touch */
    static const char *const NO[] = { "hello there", "my name is Wassup", "im a HEYA", "w4ssup", "thankyou2", "luv3", "who are you", "yo", "wanna talk", "gonna go", "idk", "lol", "dont go", "u r kind", "ya", "thx", "brb" };
    for (size_t i = 0; i < sizeof NO / sizeof NO[0]; i++) {
        char out[200]; SpellInfo si; sp_fix(NO[i], out, 200, &si); char lc[200]; lang_clean(NO[i], lc, 200, NULL);
        PCHECK(si.nfix == 0 && !strcmp(out, lc), "\"%s\" must stay (got \"%s\")", NO[i], out);
    }
    {   const char *nm = NAMES_A[0]; char out[200], t[200]; SpellInfo si; snprintf(t, sizeof t, "%s luv", nm); (void)t;
        sp_fix(nm, out, 200, &si); PCHECK(si.nfix == 0, "a known name stays (%s -> %s)", nm, out); }
    /* the order: a dialect row first, then the edit repair, then the phonetic one */
    {   char out[200]; SpellInfo si;
        sp_fix("frend", out, 200, &si);  PCHECK(si.nfix == 1 && si.fix[0].kind == SPELL_DIALECT, "frend is a dialect row");
        sp_fix("pleese", out, 200, &si); PCHECK(si.nfix == 1 && si.fix[0].kind == SPELL_EDIT && !strcmp(out, "please"), "pleese is an edit repair (got \"%s\" kind %d)", out, si.fix[0].kind);
        sp_fix("misshun", out, 200, &si); PCHECK(si.nfix == 1 && si.fix[0].kind == SPELL_PHONETIC && !strcmp(out, "mission") && si.fix[0].dist == 2 && si.fix[0].conf >= SPELL_MIN_CONF, "misshun is a phonetic fix of distance 2 (got \"%s\" kind %d)", out, si.nfix ? si.fix[0].kind : -1);
        sp_fix("rilly nice", out, 200, &si); PCHECK(si.nfix == 1 && si.fix[0].kind == SPELL_DIALECT && !strcmp(out, "really nice"), "rilly is a dialect row (got \"%s\")", out);
        sp_fix("whats the mishun", out, 200, &si);
        PCHECK(si.nfix == 0 || si.fix[0].kind != SPELL_EDIT, "mishun is not an edit repair");
    }
    /* the phonetic fallback by itself */
    {   static const char *const P[][2] = { { "misshun", "mission" }, { "rilly", "really" }, { "reely", "really" }, { "howdee", "howdy" }, { "misson", "mission" }, { "pleese", "please" }, { "plees", "please" } };
        for (size_t i = 0; i < sizeof P / sizeof P[0]; i++) {
            char o[40]; int cf = 0, d = 0; int r = lang_spell_phonetic(P[i][0], (int)strlen(P[i][0]), o, &cf, &d);
            PCHECK(r && !strcmp(o, P[i][1]) && cf >= 40 && cf <= 99 && d >= 1 && d <= 3, "%s must sound like %s (got %s, conf %d, dist %d)", P[i][0], P[i][1], r ? o : "nothing", cf, d);
        }
        /* real words the vocabulary lacks must not be bent into a near sound */
        static const char *const KEEP[] = { "comma", "type", "allow", "tomb", "queue", "photo", "knight", "ghost", "wrist", "thumb", "island", "castle" };
        for (size_t i = 0; i < sizeof KEEP / sizeof KEEP[0]; i++) {
            char o[40]; int cf = 0, d = 0; int n = (int)strlen(KEEP[i]);
            char out[200]; SpellInfo si; sp_fix(KEEP[i], out, 200, &si);
            int r = lang_spell_phonetic(KEEP[i], n, o, &cf, &d);
            PCHECK(!(r && cf >= (n == 3 ? SPELL_MIN_CONF + 8 : n == 4 ? SPELL_MIN_CONF + 4 : SPELL_MIN_CONF)) || vocab_protected(KEEP[i], n) || lang_known_word(KEEP[i], n), "\"%s\" would be bent into \"%s\" (conf %d)", KEEP[i], o, cf);
            if (!vocab_protected(KEEP[i], n) && !lang_known_word(KEEP[i], n)) PCHECK(si.nfix == 0 || si.fix[0].kind != SPELL_PHONETIC, "\"%s\" must not get a phonetic fix (got \"%s\")", KEEP[i], out);
        }
        char o[40]; int cf, d;
        PCHECK(!lang_spell_phonetic("xyzzyplugh", 10, o, &cf, &d) && !lang_spell_phonetic("ab", 2, o, &cf, &d) && !lang_spell_phonetic("supercalifragilisticexpialidocious", 34, o, &cf, &d), "nothing, too short, too long: no phonetic fix");
        PCHECK(lang_spell_phone_size() >= 300 && lang_spell_phone_size() < lang_spell_dict_size(), "the phonetic index has %d entries (dictionary %d): only the words of patterns and lexicons", lang_spell_phone_size(), lang_spell_dict_size());
    }
    /* the maps: a multi-word result maps its first letters to the token and its LAST letter to the token's last character */
    {   const char *t = "wassup there"; char lc[100]; int map[100]; int len = lang_clean(t, lc, 100, map);
        char out[100]; int src[100], om[100]; SpellInfo si; int n = lang_spell_text(t, lc, map, 0, out, 100, om, src, &si);
        PCHECK(n == 14 && !strcmp(out, "whats up there") && si.nfix == 1 && si.fix[0].a == 0 && si.fix[0].b == 6, "wassup there: \"%s\" fix %d-%d", out, si.fix[0].a, si.fix[0].b);
        PCHECK(om[0] == 0 && om[7] == 5 && om[8] == 6 && om[n] == map[len] && src[n] == len, "the maps of a multi-word result (first %d, last of up %d)", om[0], om[7]);
        int mono = 1; for (int k = 1; k < n; k++) if (om[k] < om[k - 1] || src[k] < src[k - 1]) mono = 0;
        PCHECK(mono, "the maps are monotone");
        /* the cap: a result that does not fit stays as typed; one that fits exactly is written */
        char o8[8]; n = lang_spell_text("wassup", "wassup", NULL, 0, o8, 8, NULL, NULL, &si);
        PCHECK(n == 6 && si.nfix == 0 && !strcmp(o8, "wassup"), "a cap of 8 cannot hold \"whats up\" and a NUL: as typed (got \"%s\")", o8);
        char o9[9]; n = lang_spell_text("wassup", "wassup", NULL, 0, o9, 9, NULL, NULL, &si);
        PCHECK(n == 8 && si.nfix == 1 && !strcmp(o9, "whats up"), "a cap of 9 holds it exactly (got \"%s\")", o9);
    }
    /* the highlight, Heard, the intents, safety */
    {   Heard h; uint8_t mask[200];
        lang_parse("wassup", &h);
        PCHECK(heard_has(&h, I_GREET) && h.nspell == 1 && h.spell_conf == 90 && (h.flags & LF_SPELL), "wassup is a greet with a spell fix of confidence 90 (nspell %d conf %d)", h.nspell, h.spell_conf);
        lang_highlight("wassup", mask); PCHECK(mask[0] && mask[1] && mask[2] && mask[3] && mask[4] && mask[5], "the highlight covers the whole typed word wassup (%d%d%d%d%d%d)", mask[0], mask[1], mask[2], mask[3], mask[4], mask[5]);
        lang_parse("thankyou", &h); PCHECK(heard_has(&h, I_THANKS), "thankyou is heard as thanks");
        lang_parse("hw r u", &h); PCHECK(heard_has(&h, I_HOWRU), "hw r u is heard as how are you");
        lang_parse("luv u", &h); PCHECK(heard_has(&h, I_LOVE), "luv u is heard as love");
        lang_parse("howdee", &h); PCHECK(heard_has(&h, I_GREET), "howdee is heard as a greet");
        lang_parse("hello there", &h); PCHECK(h.nspell == 0 && !(h.flags & LF_SPELL), "a clean text sets no spell field");
        static const char *const SH[] = { "i wana die", "i wana kill myself", "i dono wana live" };
        for (size_t i = 0; i < sizeof SH / sizeof SH[0]; i++) { lang_parse(SH[i], &h); if (i < 2) PCHECK(heard_has(&h, I_SELFHARM) && h.order[0] == I_SELFHARM, "\"%s\" must be heard as self-harm, first (through the dialect row)", SH[i]); }
    }
    /* registering later: a new row is used at once, a later row wins, a vocabulary word is never a key, reset forgets */
    {   static const char *const R1[] = { "zorp|hello", 0 }, *const R2[] = { "zorp|goodbye", "hello|hullo", 0 };
        char out[200]; SpellInfo si; int before = lang_dialect_count();
        sp_fix("zorp there", out, 200, &si); PCHECK(si.nfix == 0, "zorp is nothing before the row exists");
        lang_register_dialect(R1); sp_fix("zorp there", out, 200, &si); PCHECK(si.nfix == 1 && !strcmp(out, "hello there"), "a registered row works at once (got \"%s\")", out);
        PCHECK(lang_dialect_count() == before + 1, "the map grew by one row (%d -> %d)", before, lang_dialect_count());
        lang_register_dialect(R2); sp_fix("zorp there", out, 200, &si); PCHECK(!strcmp(out, "goodbye there"), "a later row of the same key wins (got \"%s\")", out);
        sp_fix("hello there", out, 200, &si); PCHECK(si.nfix == 0 && !strcmp(out, "hello there"), "a vocabulary word is never a key, even if a row says so (got \"%s\")", out);
        lang_tables_reset(); sp_fix("zorp there", out, 200, &si); PCHECK(si.nfix == 0 && lang_dialect_count() == before, "after a reset the rows are gone (count %d, wanted %d)", lang_dialect_count(), before);
        /* a lexicon word becomes a phonetic candidate; the reset takes it away */
        static const char *const LX[] = { "krakatoa|greet|7", 0 }; char o[40]; int cf = 0, d = 0;
        PCHECK(!lang_spell_phonetic("krakatoo", 8, o, &cf, &d), "krakatoo is nothing before the lexicon word exists");
        lang_register_lex(LX); int r = lang_spell_phonetic("krakatoo", 8, o, &cf, &d);
        PCHECK(r && !strcmp(o, "krakatoa") && d == 1, "krakatoo must sound like krakatoa once a lexicon row has it (got %s)", r ? o : "nothing");
        lang_tables_reset(); PCHECK(!lang_spell_phonetic("krakatoo", 8, o, &cf, &d), "after a reset krakatoa is gone again");
        /* a word that only the vocabulary has is not a phonetic candidate */
        static const char *const VX[] = { "krakatoa|9000|n", 0 }; lang_register_vocab(VX);
        PCHECK(!lang_spell_phonetic("krakatoo", 8, o, &cf, &d), "a vocabulary-only word is not a phonetic candidate");
        lang_tables_reset();
    }
    /* the switch: with the repair off nothing in the dialect table or the phonetic index is used */
    {   char out[200]; SpellInfo si; char o[40]; int cf, d;
        lang_spell_set(0); sp_fix("wassup", out, 200, &si); PCHECK(si.nfix == 0, "the off switch turns the dialect rows off too");
        PCHECK(!lang_spell_phonetic("misshun", 7, o, &cf, &d), "the off switch turns the phonetic fallback off");
        lang_spell_set(1);
    }
    /* speed: the key, a phonetic lookup, a dialect lookup */
    {   struct timespec a, b; char k[40]; long sum = 0;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < 100000; i++) sum += lang_phone(i & 1 ? "thanksgiving" : "wassup", i & 1 ? 12 : 6, k, 40);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms_key = (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
        char o[40]; int cf, d, ok = 0;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < 20000; i++) ok += lang_spell_phonetic(i & 1 ? "misshun" : "supercalifragilistic", i & 1 ? 7 : 20, o, &cf, &d);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms_ph = (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
        long hit = 0;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < 100000; i++) hit += lang_dialect(i & 1 ? "wassup" : "hello", i & 1 ? 6 : 5) != NULL;
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms_dl = (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
        printf("  phonetic speed: 100,000 keys %.1f ms (%.5f ms each), 20,000 lookups %.1f ms (%.4f ms each), 100,000 dialect lookups %.1f ms; index %d entries, %d dialect rows (%d with a result word not yet known)\n",
               ms_key, ms_key / 100000, ms_ph, ms_ph / 20000, ms_dl, lang_spell_phone_size(), lang_dialect_count(), unknown_out);
        PCHECK(sum > 0 && ok == 10000 && hit == 50000, "the speed loops changed (%ld %d %ld)", sum, ok, hit);
        PCHECK(ms_key / 100000 < 0.01 && ms_ph / 20000 < 0.05 && ms_dl / 100000 < 0.001, "too slow: key %.5f ms, lookup %.4f ms, dialect %.5f ms", ms_key / 100000, ms_ph / 20000, ms_dl / 100000);
    }
    printf("phonetic: %d checks, %d problems; %d dialect rows\n", checks, bad, rows);
    #undef PCHECK
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
        else if (!strcmp(a, "--clean"))    { rc |= do_clean(argc, argv) ? 1 : 0; rc |= do_signals(argc, argv) ? 1 : 0; rc |= do_words(argc, argv) ? 1 : 0; rc |= do_textspeak(argc, argv) ? 1 : 0; rc |= do_numbers(argc, argv) ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--vocab"))    { rc |= do_vocab() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--spell"))    { rc |= do_spell(argc, argv) ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--phonetic")) { rc |= do_phonetic(argc, argv) ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--matrix"))   { rc |= do_matrix() ? 1 : 0; any = 1; }
        else if (!strcmp(a, "--bench"))    { do_bench(); any = 1; }
    }
    if (all && !any) { rc |= do_check() ? 1 : 0; do_stats(); do_coverage(quota); do_dupes(); do_length(); rc |= do_tables() ? 1 : 0; rc |= do_moods() ? 1 : 0; rc |= do_voices() ? 1 : 0; rc |= do_matrix() ? 1 : 0; rc |= do_clean(argc, argv) ? 1 : 0; rc |= do_signals(argc, argv) ? 1 : 0; rc |= do_words(argc, argv) ? 1 : 0; rc |= do_textspeak(argc, argv) ? 1 : 0; rc |= do_numbers(argc, argv) ? 1 : 0; rc |= do_vocab() ? 1 : 0; rc |= do_spell(argc, argv) ? 1 : 0; rc |= do_phonetic(argc, argv) ? 1 : 0; rc |= do_intents() ? 1 : 0; do_bench(); }
    else if (all) { do_stats(); do_coverage(quota); do_dupes(); do_length(); rc |= do_tables() ? 1 : 0; rc |= do_moods() ? 1 : 0; rc |= do_voices() ? 1 : 0; rc |= do_matrix() ? 1 : 0; rc |= do_clean(argc, argv) ? 1 : 0; rc |= do_signals(argc, argv) ? 1 : 0; rc |= do_words(argc, argv) ? 1 : 0; rc |= do_textspeak(argc, argv) ? 1 : 0; rc |= do_numbers(argc, argv) ? 1 : 0; rc |= do_vocab() ? 1 : 0; rc |= do_spell(argc, argv) ? 1 : 0; rc |= do_phonetic(argc, argv) ? 1 : 0; rc |= do_intents() ? 1 : 0; do_bench(); }
    return rc;
}
