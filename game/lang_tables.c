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
static const char *const *numtab_t[MAX_TABLES]; static int nnum_t;      /* chunk 52: number-letter words and number aliases */
static const char *const *irrtab[MAX_TABLES]; static int nirr_t;       /* chunk 24: irregular forms and never-touched words */
static const char *const *vocabtab[MAX_TABLES]; static int nvocab_t;
static const char *const *nametab[MAX_TABLES]; static int nname_t;     /* chunk 20: known first names */
static const char *const *masktab[MAX_TABLES]; static int nmask_t;      /* chunk 54a: roots of censored words */
static const char *const *modtab[MAX_TABLES]; static int nmod_t;        /* chunk 58: modifiers */
static const char *const *emotab[MAX_TABLES]; static int nemo_t;        /* chunk 55: emoticons and emoji */
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
int lang_register_numbers(const char *const *rows) { builtin(); if (nnum_t >= MAX_TABLES) return -1; numtab_t[nnum_t++] = rows; dirty = 1; return 0; }
int lang_register_mods(const char *const *rows) { builtin(); if (nmod_t >= MAX_TABLES) return -1; modtab[nmod_t++] = rows; dirty = 1; return 0; }
int lang_register_emoji(const char *const *rows) { builtin(); if (nemo_t >= MAX_TABLES) return -1; emotab[nemo_t++] = rows; dirty = 1; return 0; }
int lang_register_masks(const char *const *rows) { builtin(); if (nmask_t >= MAX_TABLES) return -1; masktab[nmask_t++] = rows; dirty = 1; return 0; }
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
    lang_register_patrows(PATROWS_SAFETY);                       /* chunk 54b */
    lang_register_macros(MACROS_A);                              /* chunk 26 */
    lang_register_lex(LEX_CORE);                                 /* chunk 27 */
    lang_register_lex(SENT_CORE);                                /* chunk 28 */
    lang_register_lex(SENT_POS_A);                               /* chunk 56a */
    lang_register_lex(SENT_POS_B);                               /* chunk 56b */
    lang_register_lex(SENT_POS_C);                               /* chunk 56c */
    lang_register_lex(SENT_NEG_A);                               /* chunk 57a */
    lang_register_lex(TOPIC_CORE);                               /* chunk 29 */
    lang_register_lex(lang_frame_wordrows());                    /* chunk 30: the words of the frame table (rows "word|frame|1", read by no layer, known to the spell repair) */
    lang_register_textspeak(TEXTSPEAK);
    lang_register_dialect(DIALECT);                              /* chunk 22 */
    lang_register_irregular(IRREGULAR);                          /* chunk 24 */
    lang_register_numbers(NUMBERS);                              /* chunk 52 */
    lang_register_lex(LEX_MASKS);                                /* chunk 54a: swears, rude insults, sexual words and their respellings */
    lang_register_names(NAMES_GUARD);                            /* chunk 54a: essex, assassin ... (never split or repaired into a profanity row) */
    lang_register_masks(MASKROOTS);                              /* chunk 54a: roots a censored token is matched against */
    lang_register_mods(MODS);                                    /* chunk 58: intensifiers, diminishers, negators, contrast words */
    lang_register_emoji(EMOJI);                                  /* chunk 55: emoticons and emoji with their weights */
    lang_register_vocab(VOCAB_A);                                /* chunk 20 */
    lang_register_vocab(VOCAB_A2);                               /* chunk 49b */
    lang_register_vocab(VOCAB_A3);                               /* chunk 49c */
    lang_register_vocab(VOCAB_A4);                               /* chunk 49d */
    lang_register_vocab(VOCAB_B1);                               /* chunk 50a */
    lang_register_vocab(VOCAB_B2);                               /* chunk 50b */
    lang_register_vocab(VOCAB_B3);                               /* chunk 50c */
    lang_register_vocab(VOCAB_B4);                               /* chunk 50d */
    lang_register_vocab(VOCAB_C1);                               /* chunk 51b */
    lang_register_vocab(VOCAB_C2);                               /* chunk 51c */
    lang_register_vocab(VOCAB_C3);                               /* chunk 51d */
    lang_register_vocab(VOCAB_C4);                               /* chunk 51e */
    lang_register_names(NAMES_A);
    lang_register_names(NAMES_B);                                /* chunk 51a */
}

void lang_tables_reset(void) { nlt = npt = nlex_t = nvocab_t = nts_t = nname_t = ndial_t = nirr_t = nmac_t = nnum_t = nmask_t = nemo_t = nmod_t = 0; builtin_done = 0; dirty = 1; }

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

/* ---------- the numbers table (chunk 52): "form|result|kind" ---------- */
typedef struct { const char *from; int n; const char *to; int rn; int kind; } NT;
static NT *ntm; static int ntcap, ntn;
static const NT *nt_find(const char *w, int n) {
    if (!ntcap) return NULL;
    for (unsigned h = whash(w, n) & (unsigned)(ntcap - 1);; h = (h + 1) & (unsigned)(ntcap - 1)) {
        if (!ntm[h].from) return NULL;
        if (ntm[h].n == n && !memcmp(ntm[h].from, w, (size_t)n)) return &ntm[h];
    }
}
static void nt_put(const char *from, int n, const char *to, int rn, int kind) {
    for (unsigned h = whash(from, n) & (unsigned)(ntcap - 1);; h = (h + 1) & (unsigned)(ntcap - 1)) {
        if (!ntm[h].from) { ntm[h].from = from; ntm[h].n = n; ntm[h].to = to; ntm[h].rn = rn; ntm[h].kind = kind; ntn++; return; }
        if (ntm[h].n == n && !memcmp(ntm[h].from, from, (size_t)n)) { ntm[h].to = to; ntm[h].rn = rn; ntm[h].kind = kind; return; }     /* a later row wins */
    }
}
static char *nt_text; static size_t nt_cap;
static void build_numtab(void) {
    free(ntm); ntm = NULL; ntcap = ntn = 0; free(nt_text); nt_text = NULL; nt_cap = 0;
    size_t rows = 0, bytes = 0;
    for (int t = 0; t < nnum_t; t++) for (const char *const *r = numtab_t[t]; *r; r++) { rows++; bytes += strlen(*r) + 1; }
    if (!rows) return;
    ntcap = 16; while ((size_t)ntcap < rows * 2) ntcap *= 2;
    ntm = calloc((size_t)ntcap, sizeof *ntm); nt_text = malloc(bytes + 1);
    if (!ntm || !nt_text) { free(ntm); ntm = NULL; ntcap = 0; return; }
    char *o = nt_text;
    for (int t = 0; t < nnum_t; t++) for (const char *const *r = numtab_t[t]; *r; r++) {
        const char *b1 = strchr(*r, '|'); const char *b2 = b1 ? strchr(b1 + 1, '|') : NULL;
        if (!b1 || !b2 || b1 == *r || b2 == b1 + 1 || !b2[1]) continue;
        int rn = (int)(b2 - b1 - 1); memcpy(o, b1 + 1, (size_t)rn); o[rn] = 0;                  /* the result as its own NUL ended string */
        nt_put(*r, (int)(b1 - *r), o, rn, b2[1]); o += rn + 1;
    }
}
const char *lang_numtab(const char *w, int n, int *kind) { lang_init(); const NT *e = nt_find(w, n); if (!e) return NULL; if (kind) *kind = e->kind; return e->to; }
int lang_numtab_count(void) { lang_init(); return ntn; }
const char *lang_numtab_row(int i) {
    builtin();
    for (int t = 0; t < nnum_t; t++) for (const char *const *r = numtab_t[t]; *r; r++) { if (!i--) return *r; }
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


/* ---------- censored words (chunk 54a): rows "root|intent|weight" ---------- */
typedef struct { char root[28]; int rn, intent, weight; } MaskRow;
static MaskRow *mrows; static int nmrows;
static int mask_char(char c) { return !(c >= 'a' && c <= 'z'); }
static void build_masks(void) {
    free(mrows); mrows = NULL; nmrows = 0;
    int rows = 0;
    for (int t = 0; t < nmask_t; t++) for (const char *const *r = masktab[t]; *r; r++) rows++;
    if (!rows) return;
    mrows = calloc((size_t)rows, sizeof *mrows);
    if (!mrows) return;
    for (int t = 0; t < nmask_t; t++) for (const char *const *r = masktab[t]; *r; r++) {
        const char *b1 = strchr(*r, '|'); const char *b2 = b1 ? strchr(b1 + 1, '|') : NULL;
        if (!b1 || b1 == *r || b1 - *r >= (int)sizeof mrows[0].root) continue;
        char nm[40]; int ln = b2 ? (int)(b2 - b1 - 1) : (int)strlen(b1 + 1);
        if (ln <= 0 || ln >= (int)sizeof nm) continue;
        memcpy(nm, b1 + 1, (size_t)ln); nm[ln] = 0;
        int in = lang_intent_by_name(nm); if (in < 0) continue;
        MaskRow *m = &mrows[nmrows++];
        m->rn = (int)(b1 - *r); memcpy(m->root, *r, (size_t)m->rn); m->root[m->rn] = 0;
        m->intent = in; m->weight = b2 ? atoi(b2 + 1) : 5;
    }
}
/* does t[0..n) (letters and mask characters) fit r[0..rn)? a letter must equal the root's next letter; a run of e mask characters stands for 1 to e + 2
 * letters (f**k, f*ck and f*k are all fuck). the token starts and ends with a letter (checked by the caller), so a run is always followed by one */
static int mask_fit(const char *t, int n, const char *r, int rn) {
    if (n == 0) return rn == 0;
    if (!mask_char(t[0])) return rn > 0 && t[0] == r[0] && mask_fit(t + 1, n - 1, r + 1, rn - 1);
    int e = 1; while (e < n && mask_char(t[e])) e++;
    for (int k = 1; k <= e + 2 && k <= rn; k++) if (mask_fit(t + e, n - e, r + k, rn - k)) return 1;
    return 0;
}
int lang_mask_match(const char *tok, int n, int *intent, int *weight) {
    lang_init();
    char t[48];
    if (!tok || n < 3 || n >= (int)sizeof t || !mrows) return 0;
    int letters = 0;
    for (int i = 0; i < n; i++) { char c = tok[i]; if (c >= 'A' && c <= 'Z') c = (char)(c + 32); t[i] = c; if (!mask_char(c)) letters++; }
    t[n] = 0;
    if (letters < 2 || mask_char(t[0]) || mask_char(t[n - 1])) return 0;
    int best = -1, bd = 1 << 20;
    for (int i = 0; i < nmrows; i++) {
        if (!mask_fit(t, n, mrows[i].root, mrows[i].rn)) continue;
        int d = mrows[i].rn > n ? mrows[i].rn - n : n - mrows[i].rn;        /* the root whose length is the closest to the token wins, then the first row */
        if (d < bd) { bd = d; best = i; }
    }
    if (best < 0) return 0;
    if (intent) *intent = mrows[best].intent;
    if (weight) *weight = mrows[best].weight;
    return 1;
}
int lang_mask_count(void) { lang_init(); return nmrows; }
const char *lang_mask_row(int i) {
    builtin();
    for (int t = 0; t < nmask_t; t++) for (const char *const *r = masktab[t]; *r; r++) { if (!i--) return *r; }
    return NULL;
}

/* ---------- emoticons and emoji (chunk 55): rows "form|kind|weight" ---------- */
typedef struct { int cp, kind, weight; } EmojiEnt;
static EmojiEnt *emj; static int emjcap, emjn;                     /* emoji: open addressing by code point */
static LangEmo *emo_rows; static char *emo_text; static int emo_n;  /* emoticons, grouped by first character */
static int emo_start[128], emo_cnt[128];
static int emo_bad_n;
static int emo_kind_of(char c) {
    switch (c) { case 'p': return TK_POS; case 'n': return TK_NEG; case 'l': return TK_LOVE; case 's': return TK_SHOCK;
                 case 'c': return TK_CRY; case 'a': return TK_ANGRY; case 'h': return TK_LAUGH; default: return 0; }
}
/* the single UTF-8 code point that fills s[0..n): its value, or -1 */
static int emo_utf8(const unsigned char *s, int n) {
    int cp, len;
    if (n >= 2 && (s[0] & 0xE0) == 0xC0) { cp = s[0] & 0x1F; len = 2; }
    else if (n >= 3 && (s[0] & 0xF0) == 0xE0) { cp = s[0] & 0x0F; len = 3; }
    else if (n >= 4 && (s[0] & 0xF8) == 0xF0) { cp = s[0] & 0x07; len = 4; }
    else return -1;
    if (len != n) return -1;
    for (int i = 1; i < len; i++) { if ((s[i] & 0xC0) != 0x80) return -1; cp = (cp << 6) | (s[i] & 0x3F); }
    return cp < 0x80 ? -1 : cp;
}
typedef struct { const char *form; int fn, kind, weight, seq; } EmoRaw;
static int cmp_emoraw(const void *a, const void *b) {
    const EmoRaw *x = a, *y = b;
    int c = strncmp(x->form, y->form, (size_t)(x->fn < y->fn ? x->fn : y->fn));
    if (!c) c = x->fn - y->fn;
    return c ? c : x->seq - y->seq;
}
static int cmp_emorows(const void *a, const void *b) {
    const LangEmo *x = a, *y = b;
    return strcmp(x->s, y->s);
}
static void build_emoji(void) {
    free(emj); emj = NULL; emjcap = emjn = 0; free(emo_rows); emo_rows = NULL; free(emo_text); emo_text = NULL; emo_n = 0; emo_bad_n = 0;
    memset(emo_start, 0, sizeof emo_start); memset(emo_cnt, 0, sizeof emo_cnt);
    int rows = 0;
    for (int t = 0; t < nemo_t; t++) for (const char *const *r = emotab[t]; *r; r++) rows++;
    if (!rows) return;
    EmoRaw *raw = calloc((size_t)rows, sizeof *raw);
    emjcap = 16; while (emjcap < rows * 2) emjcap *= 2;
    emj = calloc((size_t)emjcap, sizeof *emj);
    if (!raw || !emj) { free(raw); free(emj); emj = NULL; emjcap = 0; return; }
    int nraw = 0, seq = 0;
    for (int t = 0; t < nemo_t; t++) for (const char *const *r = emotab[t]; *r; r++, seq++) {
        const char *b1 = strchr(*r, '|'); const char *b2 = b1 ? strchr(b1 + 1, '|') : NULL;
        if (!b1 || !b2 || b1 == *r || b2 != b1 + 2) { emo_bad_n++; continue; }
        int kind = emo_kind_of(b1[1]), w = 0, ok = *(b2 + 1) != 0;
        for (const char *d = b2 + 1; *d; d++) { if (*d < '0' || *d > '9' || w > 99) { ok = 0; break; } w = w * 10 + (*d - '0'); }
        if (!kind || !ok || w > 50) { emo_bad_n++; continue; }
        int fn = (int)(b1 - *r);
        if ((unsigned char)(*r)[0] >= 0x80) {                                  /* an emoji: exactly one code point */
            int cp = emo_utf8((const unsigned char *)*r, fn);
            if (cp < 0) { emo_bad_n++; continue; }
            for (unsigned h = ((unsigned)cp * 2654435761u) & (unsigned)(emjcap - 1);; h = (h + 1) & (unsigned)(emjcap - 1)) {
                if (!emj[h].cp) { emj[h].cp = cp; emj[h].kind = kind; emj[h].weight = w; emjn++; break; }
                if (emj[h].cp == cp) { emj[h].kind = kind; emj[h].weight = w; break; }      /* a later row wins */
            }
        } else {                                                                /* an ASCII emoticon: lower case, 2-12 characters */
            int ok2 = fn >= 2 && fn <= 12;
            for (int i = 0; i < fn && ok2; i++) { unsigned char c = (unsigned char)(*r)[i]; if (c < 33 || c >= 127 || (c >= 'A' && c <= 'Z')) ok2 = 0; }
            if (!ok2) { emo_bad_n++; continue; }
            raw[nraw].form = *r; raw[nraw].fn = fn; raw[nraw].kind = kind; raw[nraw].weight = w; raw[nraw].seq = seq; nraw++;
        }
    }
    if (nraw) {
        qsort(raw, (size_t)nraw, sizeof *raw, cmp_emoraw);
        emo_rows = calloc((size_t)nraw, sizeof *emo_rows); emo_text = malloc((size_t)nraw * 14);
        if (!emo_rows || !emo_text) { free(emo_rows); free(emo_text); emo_rows = NULL; emo_text = NULL; free(raw); return; }
        char *o = emo_text;
        for (int i = 0; i < nraw; i++) {
            if (i + 1 < nraw && raw[i].fn == raw[i + 1].fn && !strncmp(raw[i].form, raw[i + 1].form, (size_t)raw[i].fn)) continue;      /* a later row of the same form wins */
            memcpy(o, raw[i].form, (size_t)raw[i].fn); o[raw[i].fn] = 0;
            emo_rows[emo_n].s = o; emo_rows[emo_n].kind = raw[i].kind; emo_rows[emo_n].weight = raw[i].weight; emo_n++;
            o += raw[i].fn + 1;
        }
        qsort(emo_rows, (size_t)emo_n, sizeof *emo_rows, cmp_emorows);          /* sorted by form, so equal first characters are together */
        for (int i = 0; i < emo_n; i++) { int c = (unsigned char)emo_rows[i].s[0]; if (!emo_cnt[c]) emo_start[c] = i; emo_cnt[c]++; }
    }
    free(raw);
}
int lang_emoji_kind(int cp, int *weight) {
    lang_init();
    if (!emj || cp < 0x80) return 0;
    for (unsigned h = ((unsigned)cp * 2654435761u) & (unsigned)(emjcap - 1);; h = (h + 1) & (unsigned)(emjcap - 1)) {
        if (!emj[h].cp) return 0;
        if (emj[h].cp == cp) { if (weight) *weight = emj[h].weight; return emj[h].kind; }
    }
}
const LangEmo *lang_emoticons(int c, int *n) {
    lang_init();
    if (n) *n = 0;
    if (c <= 0 || c >= 128 || !emo_cnt[c]) return NULL;
    if (n) *n = emo_cnt[c];
    return emo_rows + emo_start[c];
}
int lang_emoji_count(void) { lang_init(); return emjn; }
int lang_emoticon_count(void) { lang_init(); return emo_n; }
int lang_emoji_bad(void) { lang_init(); return emo_bad_n; }
const char *lang_emoji_row(int i) {
    builtin();
    for (int t = 0; t < nemo_t; t++) for (const char *const *r = emotab[t]; *r; r++) { if (!i--) return *r; }
    return NULL;
}

/* ---------- modifiers (chunk 58): rows "word|mod|kind" ---------- */
typedef struct { char w[20]; unsigned char flags, needs_a; int boost; } ModEnt;
static ModEnt *mods; static int nmods, mod_bad_n;
static int cmp_modent(const void *a, const void *b) { return strcmp(((const ModEnt *)a)->w, ((const ModEnt *)b)->w); }
static void build_mods(void) {
    free(mods); mods = NULL; nmods = 0; mod_bad_n = 0;
    int rows = 0;
    for (int t = 0; t < nmod_t; t++) for (const char *const *r = modtab[t]; *r; r++) rows++;
    if (!rows) return;
    ModEnt *tmp = calloc((size_t)rows, sizeof *tmp); if (!tmp) return;
    int n = 0;
    for (int t = 0; t < nmod_t; t++) for (const char *const *r = modtab[t]; *r; r++) {
        const char *b1 = strchr(*r, '|'); const char *b2 = b1 ? strchr(b1 + 1, '|') : NULL;
        int wl = b1 ? (int)(b1 - *r) : 0, ok = b1 && b2 && wl >= 1 && wl < 20 && !strncmp(b1 + 1, "mod", 3) && b1[4] == '|';
        for (int i = 0; ok && i < wl; i++) { char c = (*r)[i]; if (!((c >= 'a' && c <= 'z') || c == '\'' || c == '-')) ok = 0; }
        if (!ok) { mod_bad_n++; continue; }
        const char *k = b2 + 1; ModEnt e; memset(&e, 0, sizeof e); memcpy(e.w, *r, (size_t)wl);
        if (!strcmp(k, "n")) e.flags = MOD_NEG; else if (!strcmp(k, "c")) e.flags = MOD_CONTRAST; else if (!strcmp(k, "j")) e.flags = MOD_CONJ;
        else if ((k[0] == 'b' || k[0] == 'd') && k[1] >= '1' && k[1] <= (k[0] == 'b' ? '3' : '2') && (!k[2] || (k[2] == 'a' && !k[3] && k[0] == 'd'))) {
            e.boost = (k[0] == 'b' ? 1 : -1) * (k[1] - '0'); e.needs_a = k[2] == 'a';
        } else { mod_bad_n++; continue; }
        int j; for (j = 0; j < n; j++) if (!strcmp(tmp[j].w, e.w)) break;       /* several rows of a word add up; a later boost row replaces an earlier one */
        if (j == n) tmp[n++] = e;
        else { tmp[j].flags |= e.flags; if (e.boost) { tmp[j].boost = e.boost; tmp[j].needs_a = e.needs_a; } }
    }
    qsort(tmp, (size_t)n, sizeof *tmp, cmp_modent);
    mods = tmp; nmods = n;
}
int lang_mod_find(const char *w, int n, LangMod *m) {
    lang_init();
    if (n < 1 || n >= 20 || !mods) return 0;
    char k[20]; memcpy(k, w, (size_t)n); k[n] = 0;
    int lo = 0, hi = nmods - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2, c = strcmp(k, mods[mid].w);
        if (!c) { m->flags = mods[mid].flags; m->needs_a = mods[mid].needs_a; m->boost = (signed char)mods[mid].boost; return 1; }
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }
    return 0;
}
int lang_mod_count(void) { lang_init(); return nmods; }
int lang_mod_bad(void) { lang_init(); return mod_bad_n; }
const char *lang_mod_row(int i) {
    builtin();
    for (int t = 0; t < nmod_t; t++) for (const char *const *r = modtab[t]; *r; r++) { if (!i--) return *r; }
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
    build_numtab();
    build_masks();
    build_emoji();
    build_mods();
    lang_clean_set_numtab(lang_numtab);
    lang_clean_set_emoji(lang_emoji_kind, lang_emoticons);
    lang_clause_set_mods(lang_mod_find);
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
