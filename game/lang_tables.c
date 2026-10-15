/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the registry and the indexes. see lang_tables.h */
#include "lang_tables.h"
#include "lang.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TABLES 128

typedef struct { const Line *t; int n; } LineTab;
typedef struct { const Pat *t; int n; } PatTab;
static LineTab ltab[MAX_TABLES]; static int nlt;
static PatTab  ptab[MAX_TABLES]; static int npt;
static const char *const *lextab[MAX_TABLES]; static int nlex_t;
static const char *const *vocabtab[MAX_TABLES]; static int nvocab_t;
static int builtin_done, dirty = 1;

static void builtin(void);
int lang_register_lines(const Line *t, int n) { builtin(); if (nlt >= MAX_TABLES) return -1; ltab[nlt].t = t; ltab[nlt].n = n; nlt++; dirty = 1; return 0; }
int lang_register_pats(const Pat *t, int n)   { builtin(); if (npt >= MAX_TABLES) return -1; ptab[npt].t = t; ptab[npt].n = n; npt++; dirty = 1; return 0; }
int lang_register_lex(const char *const *rows)   { builtin(); if (nlex_t >= MAX_TABLES) return -1; lextab[nlex_t++] = rows; dirty = 1; return 0; }
int lang_register_vocab(const char *const *rows) { builtin(); if (nvocab_t >= MAX_TABLES) return -1; vocabtab[nvocab_t++] = rows; dirty = 1; return 0; }

/* the old tables, in the order the engine always walked them. they always come first, whatever registers first */
static void builtin(void) {
    if (builtin_done) return;
    builtin_done = 1;
    lang_register_lines(LINES_A, LINES_A_N); lang_register_lines(LINES_B, LINES_B_N);
    lang_register_lines(LINES_C, LINES_C_N); lang_register_lines(LINES_D, LINES_D_N);
    lang_register_pats(PATS, PATS_N);
}

void lang_tables_reset(void) { nlt = npt = nlex_t = nvocab_t = 0; builtin_done = 0; dirty = 1; }

/* ---------- the key index ---------- */
typedef struct { const char *key; int start, n; } KeyRange;
static const Line **sorted;          /* every line, sorted by key, registration order inside a key */
static KeyRange *keys; static int nkeys, nlines;

typedef struct { const Line *l; int seq; } LineEnt;
static int cmp_ent(const void *a, const void *b) {
    const LineEnt *x = a, *y = b;
    int c = strcmp(x->l->key, y->l->key);
    return c ? c : (x->seq > y->seq) - (x->seq < y->seq);
}

static void build_keys(void) {
    free(sorted); free(keys); sorted = NULL; keys = NULL; nkeys = nlines = 0;
    for (int t = 0; t < nlt; t++) nlines += ltab[t].n;
    if (!nlines) return;
    LineEnt *e = malloc((size_t)nlines * sizeof *e);
    sorted = malloc((size_t)nlines * sizeof *sorted);
    keys = malloc((size_t)nlines * sizeof *keys);
    if (!e || !sorted || !keys) { free(e); free(sorted); free(keys); sorted = NULL; keys = NULL; nlines = 0; return; }
    int n = 0;
    for (int t = 0; t < nlt; t++) for (int i = 0; i < ltab[t].n; i++) { e[n].l = &ltab[t].t[i]; e[n].seq = n; n++; }
    qsort(e, (size_t)n, sizeof *e, cmp_ent);
    for (int i = 0; i < n; i++) {
        sorted[i] = e[i].l;
        if (!nkeys || strcmp(keys[nkeys - 1].key, e[i].l->key)) { keys[nkeys].key = e[i].l->key; keys[nkeys].start = i; keys[nkeys].n = 0; nkeys++; }
        keys[nkeys - 1].n++;
    }
    free(e);
}

int lang_key_rows(const char *key, const Line *const **rows) {
    lang_init();
    int lo = 0, hi = nkeys - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2, c = strcmp(key, keys[mid].key);
        if (!c) { *rows = sorted + keys[mid].start; return keys[mid].n; }
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }
    *rows = NULL;
    return 0;
}
int lang_key_count(void) { lang_init(); return nkeys; }
const char *lang_key_name(int i, int *nrows) { lang_init(); if (i < 0 || i >= nkeys) return NULL; if (nrows) *nrows = keys[i].n; return keys[i].key; }
int lang_line_count(void) { lang_init(); return nlines; }

/* ---------- the pattern index ---------- */
/* a pattern is filed by its HEAD: the literal letters before the first space, * ? $ or the end, at most 3. head 3 -> table 2 keyed by
 * 3 letters, head 2 -> table 1 by 2 letters, head 1 -> table 0 by 1 letter, head 0 (starts with * or ?) -> the wild list, always a candidate.
 * a pattern that starts with $ or a space can never match at a word start (the old scan skipped it too) so it is not filed at all. */
#define NB0 256
#define NB1 1021
#define NB2 4093
static const int NB[3] = { NB0, NB1, NB2 };
static const Pat **pats; static int npats;
static int *tbl_start[3], *tbl_ids[3];       /* CSR: bucket b holds ids tbl_ids[tbl_start[b] .. tbl_start[b + 1]) */
static int *wild, nwild, *safety, nsafety;

static unsigned hb(int t, const unsigned char *s) {
    if (t == 0) return s[0];
    if (t == 1) return ((unsigned)s[0] * 31u + s[1]) % NB1;
    return (((unsigned)s[0] * 31u + s[1]) * 31u + s[2]) % NB2;
}
static int head_len(const char *p) {
    int n = 0;
    while (n < 3 && p[n] && p[n] != '*' && p[n] != '?' && p[n] != '$' && p[n] != ' ') n++;
    return n;
}

static void free_pats(void) {
    free(pats); free(wild); free(safety); pats = NULL; wild = safety = NULL; npats = nwild = nsafety = 0;
    for (int t = 0; t < 3; t++) { free(tbl_start[t]); free(tbl_ids[t]); tbl_start[t] = tbl_ids[t] = NULL; }
}

static void build_pats(void) {
    free_pats();
    for (int t = 0; t < npt; t++) npats += ptab[t].n;
    pats = malloc((size_t)(npats ? npats : 1) * sizeof *pats);
    wild = malloc((size_t)(npats ? npats : 1) * sizeof *wild);
    safety = malloc((size_t)(npats ? npats : 1) * sizeof *safety);
    int *cnt[3];
    for (int t = 0; t < 3; t++) {
        tbl_start[t] = calloc((size_t)NB[t] + 1, sizeof(int));
        tbl_ids[t] = malloc((size_t)(npats ? npats : 1) * sizeof(int));
        cnt[t] = calloc((size_t)NB[t], sizeof(int));
    }
    if (!pats || !wild || !safety || !tbl_start[0] || !tbl_start[1] || !tbl_start[2] || !tbl_ids[0] || !tbl_ids[1] || !tbl_ids[2] || !cnt[0] || !cnt[1] || !cnt[2]) {
        for (int t = 0; t < 3; t++) free(cnt[t]);
        free_pats(); return;
    }
    int n = 0;
    for (int t = 0; t < npt; t++) for (int i = 0; i < ptab[t].n; i++) pats[n++] = &ptab[t].t[i];
    for (int k = 0; k < n; k++) {                                 /* pass 1: count */
        const char *p = pats[k]->pat;
        if (pats[k]->intent == I_SELFHARM) safety[nsafety++] = k;
        int h = head_len(p);
        if (h > 0) cnt[h - 1][hb(h - 1, (const unsigned char *)p)]++;
    }
    for (int t = 0; t < 3; t++) {
        for (int b = 0; b < NB[t]; b++) { tbl_start[t][b + 1] = tbl_start[t][b] + cnt[t][b]; cnt[t][b] = 0; }
    }
    for (int k = 0; k < n; k++) {                                 /* pass 2: fill in ascending id order, so every bucket is ascending */
        const char *p = pats[k]->pat;
        int h = head_len(p);
        if (h > 0) { int b = (int)hb(h - 1, (const unsigned char *)p); tbl_ids[h - 1][tbl_start[h - 1][b] + cnt[h - 1][b]++] = k; }
        else if (*p == '*' || *p == '?') wild[nwild++] = k;
    }
    for (int t = 0; t < 3; t++) free(cnt[t]);
}

int lang_pat_count(void) { lang_init(); return npats; }
const Pat *lang_pat(int id) { return pats[id]; }
int lang_pat_safety(const int **ids) { lang_init(); *ids = safety; return nsafety; }

void lang_pat_cursor(PatCur *c, const char *s) {
    const unsigned char *u = (const unsigned char *)s;
    lang_init();
    for (int i = 0; i < 4; i++) c->l[i] = c->e[i] = NULL;
    c->l[3] = wild; c->e[3] = wild + nwild;
    if (!u[0] || !tbl_start[0]) return;
    int b = (int)hb(0, u);
    c->l[0] = tbl_ids[0] + tbl_start[0][b]; c->e[0] = tbl_ids[0] + tbl_start[0][b + 1];
    if (!u[1]) return;
    b = (int)hb(1, u);
    c->l[1] = tbl_ids[1] + tbl_start[1][b]; c->e[1] = tbl_ids[1] + tbl_start[1][b + 1];
    if (!u[2]) return;
    b = (int)hb(2, u);
    c->l[2] = tbl_ids[2] + tbl_start[2][b]; c->e[2] = tbl_ids[2] + tbl_start[2][b + 1];
}

int lang_pat_next(PatCur *c) {            /* a 4 way merge of ascending lists */
    int best = -1, bi = -1;
    for (int i = 0; i < 4; i++)
        if (c->l[i] && c->l[i] < c->e[i] && (best < 0 || *c->l[i] < best)) { best = *c->l[i]; bi = i; }
    if (bi >= 0) c->l[bi]++;
    return best;
}

/* ---------- the word hashes (lexicon, vocabulary) ---------- */
typedef struct { const char *row; int next; } WEnt;
typedef struct { int *head; int nhead; WEnt *e; int n; } WIdx;
static WIdx lexi, vocabi;

static int word_len(const char *row) { const char *b = strchr(row, '|'); return b ? (int)(b - row) : (int)strlen(row); }
static unsigned whash(const char *w, int len) { unsigned h = 2166136261u; for (int i = 0; i < len; i++) h = (h ^ (unsigned char)w[i]) * 16777619u; return h; }

static void free_widx(WIdx *x) { free(x->head); free(x->e); x->head = NULL; x->e = NULL; x->nhead = x->n = 0; }

static void build_widx(WIdx *x, const char *const *const *tabs, int nt) {
    free_widx(x);
    int total = 0;
    for (int t = 0; t < nt; t++) for (const char *const *r = tabs[t]; *r; r++) total++;
    if (!total) return;
    int nh = 16; while (nh < total * 2) nh *= 2;
    x->head = malloc((size_t)nh * sizeof(int)); x->e = malloc((size_t)total * sizeof *x->e);
    if (!x->head || !x->e) { free_widx(x); return; }
    for (int i = 0; i < nh; i++) x->head[i] = -1;
    x->nhead = nh;
    const char **rows = malloc((size_t)total * sizeof *rows);
    if (!rows) { free_widx(x); return; }
    int n = 0;
    for (int t = 0; t < nt; t++) for (const char *const *r = tabs[t]; *r; r++) rows[n++] = *r;
    for (int i = n - 1; i >= 0; i--) {         /* backwards, so each chain lists its rows in registration order */
        unsigned h = whash(rows[i], word_len(rows[i])) & (unsigned)(nh - 1);
        x->e[i].row = rows[i]; x->e[i].next = x->head[h]; x->head[h] = i;
    }
    x->n = n;
    free(rows);
}

static int widx_find(const WIdx *x, const char *word, const char **out, int max) {
    if (!x->n) return 0;
    int len = (int)strlen(word), n = 0;
    for (int i = x->head[whash(word, len) & (unsigned)(x->nhead - 1)]; i >= 0 && n < max; i = x->e[i].next)
        if (word_len(x->e[i].row) == len && !memcmp(x->e[i].row, word, (size_t)len)) out[n++] = x->e[i].row;
    return n;
}

int lang_lex_find(const char *word, const char **rows, int max) { lang_init(); return widx_find(&lexi, word, rows, max); }
const char *lang_vocab_find(const char *word) { lang_init(); const char *r[1]; return widx_find(&vocabi, word, r, 1) ? r[0] : NULL; }
int lang_lex_count(void) { lang_init(); return lexi.n; }
int lang_vocab_count(void) { lang_init(); return vocabi.n; }

/* ---------- the known-word set (chunk 17) ---------- */
typedef struct { const char *w; int n; } KW;
static KW *kw; static int kwcap, kwn;

static int kw_find(const char *w, int n) {
    if (!kwcap) return 0;
    for (unsigned h = whash(w, n) & (unsigned)(kwcap - 1);; h = (h + 1) & (unsigned)(kwcap - 1)) {
        if (!kw[h].w) return 0;
        if (kw[h].n == n && !memcmp(kw[h].w, w, (size_t)n)) return 1;
    }
}
static void kw_put(const char *w, int n) {
    for (unsigned h = whash(w, n) & (unsigned)(kwcap - 1);; h = (h + 1) & (unsigned)(kwcap - 1))
        if (!kw[h].w) { kw[h].w = w; kw[h].n = n; kwn++; return; }
}
static void kw_add(const char *w, int n) {
    if (n < 1 || n > 40 || kw_find(w, n)) return;
    if (kwn * 2 >= kwcap) {                                    /* grow */
        KW *old = kw; int oc = kwcap;
        kwcap = kwcap ? kwcap * 2 : 4096; kw = calloc((size_t)kwcap, sizeof *kw); kwn = 0;
        if (!kw) { kw = old; kwcap = oc; return; }
        for (int i = 0; i < oc; i++) if (old[i].w) kw_put(old[i].w, old[i].n);
        free(old);
    }
    kw_put(w, n);
}
/* every word (letters and inner apostrophes) of a text: the pieces between * ? $ [ ] ( ) | digits and spaces */
static void kw_words(const char *s) {
    for (int i = 0; s[i];) {
        if (!((s[i] >= 'a' && s[i] <= 'z') || s[i] == '\'')) { i++; continue; }
        int j = i; while ((s[j] >= 'a' && s[j] <= 'z') || s[j] == '\'') j++;
        int a = i, b = j; while (a < b && s[a] == '\'') a++; while (b > a && s[b - 1] == '\'') b--;
        kw_add(s + a, b - a);
        i = j;
    }
}
static void build_known(void) {
    free(kw); kw = NULL; kwcap = kwn = 0;
    for (int t = 0; t < npt; t++) for (int i = 0; i < ptab[t].n; i++) kw_words(ptab[t].t[i].pat);
    for (int t = 0; t < nlex_t; t++) for (const char *const *r = lextab[t]; *r; r++) kw_add(*r, word_len(*r));
    for (int t = 0; t < nvocab_t; t++) for (const char *const *r = vocabtab[t]; *r; r++) kw_add(*r, word_len(*r));
}
int lang_known_word(const char *w, int n) { lang_init(); return kw_find(w, n); }

/* ---------- init ---------- */
void lang_init(void) {
    if (!dirty) return;
    builtin();                                 /* may register: it sets dirty again, cleared below */
    build_keys();
    build_pats();
    build_widx(&lexi, lextab, nlex_t);
    build_widx(&vocabi, vocabtab, nvocab_t);
    build_known();
    lang_clean_set_known(lang_known_word);
    dirty = 0;
}

double lang_init_ms(void) {
    struct timespec a, b;
    dirty = 1;
    clock_gettime(CLOCK_MONOTONIC, &a);
    lang_init();
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
}
