/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the registry and the indexes. see lang_tables.h */
#include "lang_tables.h"
#include "lang_frame.h"
#include "lang_vocab.h"
#include "lang.h"
#include "lang_pat.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TABLES 128

typedef struct { const Line *t; int n; } LineTab;
typedef struct { const Pat *t; int n; const char *const *rows; } PatTab;     /* chunk 26: rows != NULL = a table of "pattern|intent|weight" strings */
static LineTab ltab[MAX_TABLES]; static int nlt;
static PatTab  ptab[MAX_TABLES]; static int npt;
static const char *const *lextab[MAX_TABLES]; static int nlex_t;
static const char *const *tstab[MAX_TABLES]; static int nts_t;       /* chunk 18: text-speak rows */
static const char *const *dialtab[MAX_TABLES]; static int ndial_t;     /* chunk 22: dialect and phonetic spellings */
static const char *const *irrtab[MAX_TABLES]; static int nirr_t;       /* chunk 24: irregular forms and never-touched words */
static const char *const *vocabtab[MAX_TABLES]; static int nvocab_t;
static const char *const *nametab[MAX_TABLES]; static int nname_t;     /* chunk 20: known first names */
static const char *const *mactab[MAX_TABLES]; static int nmac_t;        /* chunk 26: pattern macros */
static int builtin_done, dirty = 1;

static void builtin(void);
int lang_register_lines(const Line *t, int n) { builtin(); if (nlt >= MAX_TABLES) return -1; ltab[nlt].t = t; ltab[nlt].n = n; nlt++; dirty = 1; return 0; }
int lang_register_pats(const Pat *t, int n)   { builtin(); if (npt >= MAX_TABLES) return -1; ptab[npt].t = t; ptab[npt].n = n; ptab[npt].rows = NULL; npt++; dirty = 1; return 0; }
int lang_register_patrows(const char *const *rows) {
    builtin(); if (npt >= MAX_TABLES || !rows) return -1;
    int n = 0; while (rows[n]) n++;
    ptab[npt].t = NULL; ptab[npt].n = n; ptab[npt].rows = rows; npt++; dirty = 1; return 0;
}
int lang_register_macros(const char *const *rows) { builtin(); if (nmac_t >= MAX_TABLES) return -1; mactab[nmac_t++] = rows; dirty = 1; return 0; }
int lang_register_lex(const char *const *rows)   { builtin(); if (nlex_t >= MAX_TABLES) return -1; lextab[nlex_t++] = rows; dirty = 1; return 0; }
int lang_register_textspeak(const char *const *rows) { builtin(); if (nts_t >= MAX_TABLES) return -1; tstab[nts_t++] = rows; dirty = 1; return 0; }
int lang_register_dialect(const char *const *rows) { builtin(); if (ndial_t >= MAX_TABLES) return -1; dialtab[ndial_t++] = rows; dirty = 1; return 0; }
int lang_register_irregular(const char *const *rows) { builtin(); if (nirr_t >= MAX_TABLES) return -1; irrtab[nirr_t++] = rows; dirty = 1; return 0; }
int lang_register_names(const char *const *rows) { builtin(); if (nname_t >= MAX_TABLES) return -1; nametab[nname_t++] = rows; dirty = 1; return 0; }
int lang_register_vocab(const char *const *rows) { builtin(); if (nvocab_t >= MAX_TABLES) return -1; vocabtab[nvocab_t++] = rows; dirty = 1; return 0; }

/* the old tables, in the order the engine always walked them. they always come first, whatever registers first */
static void builtin(void) {
    if (builtin_done) return;
    builtin_done = 1;
    lang_register_lines(LINES_A, LINES_A_N); lang_register_lines(LINES_B, LINES_B_N);
    lang_register_lines(LINES_C, LINES_C_N); lang_register_lines(LINES_D, LINES_D_N);
    lang_register_lines(LINES_E, LINES_E_N);                     /* chunk 31 */
    lang_register_pats(PATS, PATS_N);
    lang_register_patrows(PATROWS_G15);                          /* chunk 31 */
    lang_register_macros(MACROS_A);                              /* chunk 26 */
    lang_register_lex(LEX_CORE);                                 /* chunk 27 */
    lang_register_lex(SENT_CORE);                                /* chunk 28 */
    lang_register_lex(TOPIC_CORE);                               /* chunk 29 */
    lang_register_lex(lang_frame_wordrows());                    /* chunk 30: the words of the frame table (rows "word|frame|1", read by no layer, known to the spell repair) */
    lang_register_textspeak(TEXTSPEAK);
    lang_register_dialect(DIALECT);                              /* chunk 22 */
    lang_register_irregular(IRREGULAR);                          /* chunk 24 */
    lang_register_vocab(VOCAB_A);                                /* chunk 20 */
    lang_register_vocab(VOCAB_A2);                               /* chunk 49b */
    lang_register_vocab(VOCAB_A3);                               /* chunk 49c */
    lang_register_vocab(VOCAB_A4);                               /* chunk 49d */
    lang_register_vocab(VOCAB_B1);                               /* chunk 50a */
    lang_register_vocab(VOCAB_B2);                               /* chunk 50b */
    lang_register_vocab(VOCAB_B3);                               /* chunk 50c */
    lang_register_vocab(VOCAB_B4);                               /* chunk 50d */
    lang_register_names(NAMES_A);
    lang_register_names(NAMES_B);                                /* chunk 51a */
}

void lang_tables_reset(void) { nlt = npt = nlex_t = nvocab_t = nts_t = nname_t = ndial_t = nirr_t = nmac_t = 0; builtin_done = 0; dirty = 1; }

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
 * a pattern that starts with $ or a space can never match at a word start (the old scan skipped it too) so it is not filed at all.
 * chunk 26: a compiled pattern ([a|b] (x) <m>) can have SEVERAL heads (one per possible first word, lpat_heads): it is filed under each of them
 * (or in the wild list when it can start with anything, or when it has more than LPAT_MAXHEADS), and lang_pat_next skips an id it just gave. */
#define NB0 256
#define NB1 1021
#define NB2 4093
static const int NB[3] = { NB0, NB1, NB2 };
static const Pat **pats; static int npats;
static const LPat **cpat; static unsigned char *pwt;        /* chunk 26: the compiled form (NULL = classic) and the weight of every pattern */
static Pat *rowpat; static char **rowtxt; static int nrowtxt;   /* chunk 26: the Pat structs and the pattern texts made from string rows */
static struct { const char *row; const char *why; } perr[64]; static int nperr, nperr_total;
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
    free(pats); free(wild); free(safety); free(cpat); free(pwt); free(rowpat);
    for (int i = 0; i < nrowtxt; i++) free(rowtxt[i]);
    free(rowtxt);
    pats = NULL; wild = safety = NULL; cpat = NULL; pwt = NULL; rowpat = NULL; rowtxt = NULL;
    npats = nwild = nsafety = nrowtxt = 0;
    for (int t = 0; t < 3; t++) { free(tbl_start[t]); free(tbl_ids[t]); tbl_start[t] = tbl_ids[t] = NULL; }
}

static void perr_add(const char *row, const char *why) { nperr_total++; if (nperr < 64) { perr[nperr].row = row; perr[nperr].why = why; nperr++; } }

/* "pattern|intent|weight": the first | outside [ ] and ( ) ends the pattern. returns 0 and fills the outputs (the pattern as a new malloc'd string) */
static int parse_patrow(const char *row, char **pat, int *intent, int *weight, const char **why) {
    int depth = 0, i = 0, cut = -1;
    for (; row[i]; i++) {
        if (row[i] == '[' || row[i] == '(') depth++;
        else if ((row[i] == ']' || row[i] == ')') && depth > 0) depth--;
        else if (row[i] == '|' && depth == 0) { cut = i; break; }
    }
    if (cut <= 0) { *why = "no pattern or no intent"; return -1; }
    const char *in = row + cut + 1; int n = 0; while (in[n] && in[n] != '|') n++;
    char name[48]; if (n < 1 || n > 40) { *why = "bad intent name"; return -1; }
    memcpy(name, in, (size_t)n); name[n] = 0;
    int id = lang_intent_by_name(name); if (id < 0) { *why = "unknown intent"; return -1; }
    int w = 5;
    if (in[n] == '|') {
        const char *ws = in + n + 1;
        if (!ws[0] || ws[1] || ws[0] < '1' || ws[0] > '9') { *why = "bad weight"; return -1; }
        w = ws[0] - '0';
    }
    char *p = malloc((size_t)cut + 1); if (!p) { *why = "out of memory"; return -1; }
    memcpy(p, row, (size_t)cut); p[cut] = 0;
    *pat = p; *intent = id; *weight = w;
    return 0;
}

static void build_pats(void) {
    free_pats();
    lpat_reset(); nperr = nperr_total = 0;
    for (int t = 0; t < nmac_t; t++) for (const char *const *r = mactab[t]; *r; r++) if (lpat_macro_add(*r) < 0) perr_add(*r, "bad macro row");
    int total = 0;
    for (int t = 0; t < npt; t++) total += ptab[t].n;
    size_t cap = (size_t)(total ? total : 1);
    pats = malloc(cap * sizeof *pats); cpat = calloc(cap, sizeof *cpat); pwt = malloc(cap);
    wild = malloc(cap * sizeof *wild); safety = malloc(cap * sizeof *safety);
    rowpat = malloc(cap * sizeof *rowpat); rowtxt = malloc(cap * sizeof *rowtxt);
    int *cnt[3];
    for (int t = 0; t < 3; t++) {
        tbl_start[t] = calloc((size_t)NB[t] + 1, sizeof(int));
        tbl_ids[t] = NULL;
        cnt[t] = calloc((size_t)NB[t], sizeof(int));
    }
    if (!pats || !cpat || !pwt || !wild || !safety || !rowpat || !rowtxt || !tbl_start[0] || !tbl_start[1] || !tbl_start[2] || !cnt[0] || !cnt[1] || !cnt[2]) {
        for (int t = 0; t < 3; t++) free(cnt[t]);
        free_pats(); return;
    }
    int n = 0, nrow = 0;
    for (int t = 0; t < npt; t++) {
        if (!ptab[t].rows) {
            for (int i = 0; i < ptab[t].n; i++) {
                const Pat *pp = &ptab[t].t[i];
                if (lpat_is_v2(pp->pat)) {
                    const char *why; const LPat *lp = lpat_compile(pp->pat, &why);
                    if (!lp) { perr_add(pp->pat, why); continue; }
                    cpat[n] = lp;
                }
                pats[n] = pp; pwt[n] = 5; n++;
            }
        } else {
            for (int i = 0; i < ptab[t].n; i++) {
                char *txt; int in, w; const char *why = "";
                const char *row = ptab[t].rows[i];
                if (parse_patrow(row, &txt, &in, &w, &why) < 0) { perr_add(row, why); continue; }
                const LPat *lp = NULL;
                if (lpat_is_v2(txt)) { lp = lpat_compile(txt, &why); if (!lp) { perr_add(row, why); free(txt); continue; } }
                rowtxt[nrowtxt++] = txt;
                rowpat[nrow].pat = txt; rowpat[nrow].intent = in;
                pats[n] = &rowpat[nrow++]; cpat[n] = lp; pwt[n] = (unsigned char)w; n++;
            }
        }
    }
    npats = n;
    /* pass 1: count (and the safety list); pass 2: fill in ascending id order, so every bucket is ascending */
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            for (int t = 0; t < 3; t++) {
                for (int b = 0; b < NB[t]; b++) { tbl_start[t][b + 1] = tbl_start[t][b] + cnt[t][b]; cnt[t][b] = 0; }
                tbl_ids[t] = malloc((size_t)(tbl_start[t][NB[t]] ? tbl_start[t][NB[t]] : 1) * sizeof(int));
                if (!tbl_ids[t]) { for (int u = 0; u < 3; u++) free(cnt[u]); free_pats(); return; }
            }
        }
        for (int k = 0; k < n; k++) {
            const char *p = pats[k]->pat;
            if (pass == 0 && pats[k]->intent == I_SELFHARM) safety[nsafety++] = k;
            if (!cpat[k]) {
                int h = head_len(p);
                if (h > 0) {
                    int b = (int)hb(h - 1, (const unsigned char *)p);
                    if (pass == 0) cnt[h - 1][b]++; else tbl_ids[h - 1][tbl_start[h - 1][b] + cnt[h - 1][b]++] = k;
                } else if (*p == '*' || *p == '?') { if (pass == 1) wild[nwild++] = k; }
            } else {
                char hs[LPAT_MAXHEADS][4]; int w = 0, huge = 0, nh = lpat_heads(cpat[k], hs, LPAT_MAXHEADS, &w, &huge);
                if (w) { if (pass == 1) wild[nwild++] = k; continue; }
                for (int j = 0; j < nh; j++) {
                    int h = (int)strlen(hs[j]); if (h < 1 || h > 3) continue;
                    int b = (int)hb(h - 1, (const unsigned char *)hs[j]);
                    if (pass == 0) cnt[h - 1][b]++; else tbl_ids[h - 1][tbl_start[h - 1][b] + cnt[h - 1][b]++] = k;
                }
            }
        }
    }
    for (int t = 0; t < 3; t++) free(cnt[t]);
}

int lang_pat_count(void) { lang_init(); return npats; }
const Pat *lang_pat(int id) { return pats[id]; }
int lang_pat_safety(const int **ids) { lang_init(); *ids = safety; return nsafety; }
const struct LPat *lang_pat_compiled(int id) { lang_init(); return id >= 0 && id < npats ? cpat[id] : NULL; }
int lang_pat_weight(int id) { lang_init(); return id >= 0 && id < npats ? pwt[id] : 5; }
int lang_pat_error_count(void) { lang_init(); return nperr_total; }
const char *lang_pat_error(int i, const char **why) { lang_init(); if (i < 0 || i >= nperr) return NULL; if (why) *why = perr[i].why; return perr[i].row; }

void lang_pat_cursor(PatCur *c, const char *s) {
    const unsigned char *u = (const unsigned char *)s;
    lang_init();
    for (int i = 0; i < 4; i++) c->l[i] = c->e[i] = NULL;
    c->last = -1;
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

int lang_pat_next(PatCur *c) {            /* a 4 way merge of ascending lists; a pattern filed under several heads can meet the text twice: skip the repeat */
    for (;;) {
        int best = -1, bi = -1;
        for (int i = 0; i < 4; i++)
            if (c->l[i] && c->l[i] < c->e[i] && (best < 0 || *c->l[i] < best)) { best = *c->l[i]; bi = i; }
        if (bi < 0) return -1;
        c->l[bi]++;
        if (best == c->last) continue;
        c->last = best;
        return best;
    }
}

/* ---------- the word hashes (lexicon, vocabulary) ---------- */
typedef struct { const char *row; int next; } WEnt;
typedef struct { int *head; int nhead; WEnt *e; int n; } WIdx;
static WIdx lexi, vocabi, namei;

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
const char *lang_name_find(const char *word) { lang_init(); const char *r[1]; return widx_find(&namei, word, r, 1) ? r[0] : NULL; }
int lang_name_count(void) { lang_init(); return namei.n; }
const char *lang_vocab_at(int i) { lang_init(); return i >= 0 && i < vocabi.n ? vocabi.e[i].row : NULL; }
const char *lang_name_at(int i) { lang_init(); return i >= 0 && i < namei.n ? namei.e[i].row : NULL; }
int lang_lex_count(void) { lang_init(); return lexi.n; }
int lang_vocab_count(void) { lang_init(); return vocabi.n; }

/* ---------- the known-word set (chunk 17) ---------- */
typedef struct { const char *w; int n; unsigned char val; } KW;      /* val (chunk 21): the word is in a pattern or a lexicon row, not only in the vocabulary */
static KW *kw; static int kwcap, kwn;

static KW *kw_slot(const char *w, int n) {
    if (!kwcap) return NULL;
    for (unsigned h = whash(w, n) & (unsigned)(kwcap - 1);; h = (h + 1) & (unsigned)(kwcap - 1)) {
        if (!kw[h].w) return NULL;
        if (kw[h].n == n && !memcmp(kw[h].w, w, (size_t)n)) return &kw[h];
    }
}
static int kw_find(const char *w, int n) { return kw_slot(w, n) != NULL; }
static void kw_put(const char *w, int n, unsigned char val) {
    for (unsigned h = whash(w, n) & (unsigned)(kwcap - 1);; h = (h + 1) & (unsigned)(kwcap - 1))
        if (!kw[h].w) { kw[h].w = w; kw[h].n = n; kw[h].val = val; kwn++; return; }
}
static void kw_add(const char *w, int n, unsigned char val) {
    if (n < 1 || n > 40) return;
    KW *e = kw_slot(w, n);
    if (e) { if (val) e->val = 1; return; }
    if (kwn * 2 >= kwcap) {                                    /* grow */
        KW *old = kw; int oc = kwcap;
        kwcap = kwcap ? kwcap * 2 : 4096; kw = calloc((size_t)kwcap, sizeof *kw); kwn = 0;
        if (!kw) { kw = old; kwcap = oc; return; }
        for (int i = 0; i < oc; i++) if (old[i].w) kw_put(old[i].w, old[i].n, old[i].val);
        free(old);
    }
    kw_put(w, n, val);
}
/* every word (letters and inner apostrophes) of a text: the pieces between * ? $ [ ] ( ) | digits and spaces */
static void kw_words(const char *s, unsigned char val) {
    for (int i = 0; s[i];) {
        if (!((s[i] >= 'a' && s[i] <= 'z') || s[i] == '\'')) { i++; continue; }
        int j = i; while ((s[j] >= 'a' && s[j] <= 'z') || s[j] == '\'') j++;
        int a = i, b = j; while (a < b && s[a] == '\'') a++; while (b > a && s[b - 1] == '\'') b--;
        kw_add(s + a, b - a, val);
        i = j;
    }
}
/* the words of a pattern: like kw_words, but a <macro> name is not a word (its alternatives are added from the macro table) */
static void kw_words_pat(const char *s) {
    for (int i = 0; s[i];) {
        if (s[i] == '<') { while (s[i] && s[i] != '>') i++; if (s[i]) i++; continue; }
        if (!((s[i] >= 'a' && s[i] <= 'z') || s[i] == '\'')) { i++; continue; }
        int j = i; while ((s[j] >= 'a' && s[j] <= 'z') || s[j] == '\'') j++;
        int a2 = i, b2 = j; while (a2 < b2 && s[a2] == '\'') a2++; while (b2 > a2 && s[b2 - 1] == '\'') b2--;
        kw_add(s + a2, b2 - a2, 1);
        i = j;
    }
}
static void build_known(void) {
    free(kw); kw = NULL; kwcap = kwn = 0;
    for (int k = 0; k < npats; k++) kw_words_pat(pats[k]->pat);                       /* chunk 26: the words of the patterns that made it into the tables (string rows too) */
    /* the words INSIDE macros are not added here on purpose (chunk 26): the starter macros hold text-speak and names (plss, er, alex) that would become spell and glue targets;
     * chunks 49-51 make most of them vocabulary words, and a pattern's own words are added above */
    for (int t = 0; t < nlex_t; t++) for (const char *const *r = lextab[t]; *r; r++) kw_add(*r, word_len(*r), 1);
    for (int t = 0; t < nvocab_t; t++) for (const char *const *r = vocabtab[t]; *r; r++) kw_add(*r, word_len(*r), 0);
}
int lang_known_word(const char *w, int n) { lang_init(); return kw_find(w, n); }
/* chunk 21: a word of a pattern or a lexicon row (not only of the vocabulary) / every known word, for the spell dictionary */
int lang_value_word(const char *w, int n) { lang_init(); KW *e = kw_slot(w, n); return e && e->val; }
void lang_known_each(void (*fn)(const char *w, int n, int val, void *u), void *u) {
    lang_init();
    for (int i = 0; i < kwcap; i++) if (kw[i].w) fn(kw[i].w, kw[i].n, kw[i].val, u);
}
static int tables_gen;
int lang_tables_gen(void) { lang_init(); return tables_gen; }

/* ---------- the text-speak map (chunk 18): token as typed -> standard form ---------- */
typedef struct { const char *from; int n; const char *to; } TS;
static TS *tsm; static int tscap, tsn;
static const char *ts_find(const char *w, int n) {
    if (!tscap) return NULL;
    for (unsigned h = whash(w, n) & (unsigned)(tscap - 1);; h = (h + 1) & (unsigned)(tscap - 1)) {
        if (!tsm[h].from) return NULL;
        if (tsm[h].n == n && !memcmp(tsm[h].from, w, (size_t)n)) return tsm[h].to;
    }
}
static void ts_put(const char *from, int n, const char *to) {
    for (unsigned h = whash(from, n) & (unsigned)(tscap - 1);; h = (h + 1) & (unsigned)(tscap - 1)) {
        if (!tsm[h].from) { tsm[h].from = from; tsm[h].n = n; tsm[h].to = to; tsn++; return; }
        if (tsm[h].n == n && !memcmp(tsm[h].from, from, (size_t)n)) { tsm[h].to = to; return; }     /* a later row wins */
    }
}
static void build_textspeak(void) {
    free(tsm); tsm = NULL; tscap = tsn = 0;
    int rows = 0;
    for (int t = 0; t < nts_t; t++) for (const char *const *r = tstab[t]; *r; r++) rows++;
    if (!rows) return;
    tscap = 16; while (tscap < rows * 2) tscap *= 2;
    tsm = calloc((size_t)tscap, sizeof *tsm);
    if (!tsm) { tscap = 0; return; }
    for (int t = 0; t < nts_t; t++) for (const char *const *r = tstab[t]; *r; r++) {
        const char *bar = strchr(*r, '|');
        if (bar && bar > *r && bar[1]) ts_put(*r, (int)(bar - *r), bar + 1);
    }
}
const char *lang_textspeak(const char *w, int n) { lang_init(); return ts_find(w, n); }

/* ---------- the dialect map (chunk 22): a dialect or phonetic spelling -> its standard words ---------- */
static TS *dlm; static int dlcap, dln;
static const char *dl_find(const char *w, int n) {
    if (!dlcap) return NULL;
    for (unsigned h = whash(w, n) & (unsigned)(dlcap - 1);; h = (h + 1) & (unsigned)(dlcap - 1)) {
        if (!dlm[h].from) return NULL;
        if (dlm[h].n == n && !memcmp(dlm[h].from, w, (size_t)n)) return dlm[h].to;
    }
}
static void dl_put(const char *from, int n, const char *to) {
    for (unsigned h = whash(from, n) & (unsigned)(dlcap - 1);; h = (h + 1) & (unsigned)(dlcap - 1)) {
        if (!dlm[h].from) { dlm[h].from = from; dlm[h].n = n; dlm[h].to = to; dln++; return; }
        if (dlm[h].n == n && !memcmp(dlm[h].from, from, (size_t)n)) { dlm[h].to = to; return; }     /* a later row wins */
    }
}
static void build_dialect(void) {
    free(dlm); dlm = NULL; dlcap = dln = 0;
    int rows = 0;
    for (int t = 0; t < ndial_t; t++) for (const char *const *r = dialtab[t]; *r; r++) rows++;
    if (!rows) return;
    dlcap = 16; while (dlcap < rows * 2) dlcap *= 2;
    dlm = calloc((size_t)dlcap, sizeof *dlm);
    if (!dlm) { dlcap = 0; return; }
    for (int t = 0; t < ndial_t; t++) for (const char *const *r = dialtab[t]; *r; r++) {
        const char *bar = strchr(*r, '|');
        if (bar && bar > *r && bar[1]) dl_put(*r, (int)(bar - *r), bar + 1);
    }
}
const char *lang_dialect(const char *w, int n) { lang_init(); return dl_find(w, n); }
int lang_dialect_count(void) { lang_init(); return dln; }
const char *lang_dialect_row(int i) {
    builtin();
    for (int t = 0; t < ndial_t; t++) for (const char *const *r = dialtab[t]; *r; r++) { if (!i--) return *r; }
    return NULL;
}

/* ---------- the irregular forms (chunk 24): "form|lemma"; "form|form" = never lemmatised ---------- */
static TS *irm; static int ircap, irn;
static const char *ir_find(const char *w, int n) {
    if (!ircap) return NULL;
    for (unsigned h = whash(w, n) & (unsigned)(ircap - 1);; h = (h + 1) & (unsigned)(ircap - 1)) {
        if (!irm[h].from) return NULL;
        if (irm[h].n == n && !memcmp(irm[h].from, w, (size_t)n)) return irm[h].to;
    }
}
static void ir_put(const char *from, int n, const char *to) {
    for (unsigned h = whash(from, n) & (unsigned)(ircap - 1);; h = (h + 1) & (unsigned)(ircap - 1)) {
        if (!irm[h].from) { irm[h].from = from; irm[h].n = n; irm[h].to = to; irn++; return; }
        if (irm[h].n == n && !memcmp(irm[h].from, from, (size_t)n)) { irm[h].to = to; return; }     /* a later row wins */
    }
}
static void build_irregular(void) {
    free(irm); irm = NULL; ircap = irn = 0;
    int rows = 0;
    for (int t = 0; t < nirr_t; t++) for (const char *const *r = irrtab[t]; *r; r++) rows++;
    if (!rows) return;
    ircap = 16; while (ircap < rows * 2) ircap *= 2;
    irm = calloc((size_t)ircap, sizeof *irm);
    if (!irm) { ircap = 0; return; }
    for (int t = 0; t < nirr_t; t++) for (const char *const *r = irrtab[t]; *r; r++) {
        const char *bar = strchr(*r, '|');
        if (bar && bar > *r && bar[1]) ir_put(*r, (int)(bar - *r), bar + 1);
    }
}
const char *lang_irregular(const char *w, int n) { lang_init(); return ir_find(w, n); }
int lang_irregular_count(void) { lang_init(); return irn; }
const char *lang_irregular_row(int i) {
    builtin();
    for (int t = 0; t < nirr_t; t++) for (const char *const *r = irrtab[t]; *r; r++) { if (!i--) return *r; }
    return NULL;
}

/* ---------- init ---------- */
void lang_init(void) {
    if (!dirty) return;
    builtin();                                 /* may register: it sets dirty again, cleared below */
    build_keys();
    build_pats();
    build_widx(&lexi, lextab, nlex_t);
    build_widx(&vocabi, vocabtab, nvocab_t);
    build_widx(&namei, nametab, nname_t);
    build_known();
    build_textspeak();
    build_dialect();
    build_irregular();
    lang_clean_set_known(lang_known_word);
    lang_clean_set_expand(lang_textspeak);
    tables_gen++;
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
