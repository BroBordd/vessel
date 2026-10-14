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
 *   --all              all of the above. exit code 1 only if --check found a problem */
#include "lang.h"
#include "lang_data.h"
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
    }
    if (all && !any) { rc |= do_check() ? 1 : 0; do_stats(); do_coverage(quota); do_dupes(); do_length(); }
    else if (all) { do_stats(); do_coverage(quota); do_dupes(); do_length(); }
    return rc;
}
