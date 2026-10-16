/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * SPELL REPAIR (docs/convo_plan.md, chunk 21; the phonetic fallback and the dialect table, chunk 22), see lang_spell.h. No SDL. */
#include "lang_spell.h"
#include "lang_tables.h"
#include "lang_vocab.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { const char *w; int n, rank; int val; } DW;
static DW *dict; static int ndict, capdict;
static uint64_t *idx; static int nidx;             /* (hash << 32) | dictionary index, sorted */
static uint64_t *pidx; static int npidx;           /* chunk 22: (hash of the phonetic key << 32) | dictionary index, the words of patterns and lexicons only */
static int built_gen = -1, spell_on = 1;
static int vocab_top;                              /* ranks of words that are not in the vocabulary start after this */

static uint32_t fnv(const char *s, int n) { uint32_t h = 2166136261u; for (int i = 0; i < n; i++) { h ^= (unsigned char)s[i]; h *= 16777619u; } return h; }

static void dict_add(const char *w, int n, int val, void *u) {
    (void)u;
    if (n < SPELL_MIN_LEN || n > SPELL_MAX_LEN) return;
    for (int i = 0; i < n; i++) if (w[i] < 'a' || w[i] > 'z') return;       /* letters only: no contractions, no digits */
    if (ndict >= capdict) { capdict = capdict ? capdict * 2 : 4096; DW *d = realloc(dict, (size_t)capdict * sizeof *dict); if (!d) { capdict = ndict; return; } dict = d; }
    dict[ndict].w = w; dict[ndict].n = n; dict[ndict].val = val; dict[ndict].rank = vocab_rank(w);    /* w is NOT NUL ended inside a row: rank is set again below */
    ndict++;
}
static void each_cb(const char *w, int n, int val, void *u) { dict_add(w, n, val, u); }

static int cmp64(const void *a, const void *b) { uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b; return x < y ? -1 : x > y; }
static void idx_add(const char *s, int n, int di, uint64_t **arr, int *cnt, int *cap) {
    if (*cnt >= *cap) { *cap = *cap ? *cap * 2 : 65536; uint64_t *a = realloc(*arr, (size_t)*cap * sizeof **arr); if (!a) { *cap = *cnt; return; } *arr = a; }
    (*arr)[(*cnt)++] = ((uint64_t)fnv(s, n) << 32) | (uint32_t)di;
}

/* ---------- the phonetic key (chunk 22): a Metaphone-style sound skeleton ---------- */
static int pv(char c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }
int lang_phone(const char *w, int n, char *out, int cap) {
    char s[2 * SPELL_MAX_LEN + 2]; int m = 0, o = 0, i = 0;
    for (int k = 0; k < n && m < (int)sizeof s - 1; k++) { char c = w[k]; if (c >= 'A' && c <= 'Z') c = (char)(c + 32); if (c >= 'a' && c <= 'z') s[m++] = c; }
    s[m] = 0;
#define AT(k) ((k) >= 0 && (k) < m ? s[k] : 0)
#define EMIT(ch) do { if (o < cap - 1) out[o] = (ch); o++; } while (0)
    if (m >= 2) {                                                  /* silent or special starts: kn gn pn wr, ae, wh */
        char a = s[0], b = s[1];
        if (((a == 'k' || a == 'g' || a == 'p') && b == 'n') || (a == 'w' && b == 'r') || (a == 'a' && b == 'e')) i = 1;
        else if (a == 'w' && b == 'h') { EMIT('W'); i = 2; }
    }
    if (m >= 1 && s[0] == 'x') { EMIT('S'); i = 1; }
    for (; i < m; i++) {
        char c = s[i], nx = AT(i + 1), pr = AT(i - 1);
        if (c == pr && c != 'c') continue;                         /* a doubled letter is one sound */
        if (pv(c)) { if (i == 0) EMIT('A'); continue; }            /* a vowel counts only at the start of the word */
        switch (c) {
        case 'b': if (!(pr == 'm' && i == m - 1)) EMIT('B'); break;                              /* dumb */
        case 'c':
            if (nx == 'i' || nx == 'e' || nx == 'y') { if (pr == 's') break; if (nx == 'i' && AT(i + 2) == 'a') EMIT('X'); else EMIT('S'); }
            else if (nx == 'h') { EMIT('X'); i++; }
            else if (nx == 'k') { EMIT('K'); i++; }
            else EMIT('K');
            break;
        case 'd': if (nx == 'g' && (AT(i + 2) == 'e' || AT(i + 2) == 'i' || AT(i + 2) == 'y')) { EMIT('J'); i += 2; } else EMIT('T'); break;
        case 'g':
            if (nx == 'h') { if (pv(AT(i + 2))) EMIT('K'); i++; }                                 /* night, though: silent; ghost: K */
            else if (nx == 'n') { if (i + 2 < m) EMIT('K'); }                                     /* sign: silent at the end */
            else if (nx == 'i' || nx == 'e' || nx == 'y') EMIT('J');
            else EMIT('K');
            break;
        case 'h': if (pv(nx) && pr != 'c' && pr != 's' && pr != 'p' && pr != 't' && pr != 'g') EMIT('H'); break;
        case 'k': if (pr != 'c') EMIT('K'); break;
        case 'p': if (nx == 'h') { EMIT('F'); i++; } else EMIT('P'); break;
        case 'q': EMIT('K'); break;
        case 's': if (nx == 'h') { EMIT('X'); i++; } else if (nx == 'i' && (AT(i + 2) == 'o' || AT(i + 2) == 'a')) EMIT('X'); else EMIT('S'); break;
        case 't':
            if (nx == 'i' && (AT(i + 2) == 'o' || AT(i + 2) == 'a')) EMIT('X');
            else if (nx == 'h') { EMIT('0'); i++; }
            else if (nx == 'c' && AT(i + 2) == 'h') { }                                           /* watch */
            else EMIT('T');
            break;
        case 'v': EMIT('F'); break;
        case 'w': if (pv(nx)) EMIT('W'); break;
        case 'x': EMIT('K'); EMIT('S'); break;
        case 'y': if (pv(nx)) EMIT('Y'); break;
        case 'z': EMIT('S'); break;
        default: EMIT((char)(c - 32)); break;                       /* f j l m n r */
        }
    }
#undef AT
#undef EMIT
    if (cap > 0) out[o < cap ? o : cap - 1] = 0;
    return o < cap ? o : cap - 1;
}

static void build(void) {
    free(dict); free(idx); free(pidx); dict = NULL; idx = NULL; pidx = NULL; ndict = capdict = nidx = npidx = 0;
    lang_known_each(each_cb, NULL);
    vocab_top = vocab_size();
    for (int i = 0; i < ndict; i++) {                                   /* the real rank: a vocabulary word keeps its rank, the others come after it */
        char b[SPELL_MAX_LEN + 1]; memcpy(b, dict[i].w, (size_t)dict[i].n); b[dict[i].n] = 0;
        int r = vocab_rank(b);
        dict[i].rank = r ? r : vocab_top + 300;
    }
    uint64_t *arr = NULL; int cnt = 0, cap = 0;
    for (int i = 0; i < ndict; i++) {
        const char *w = dict[i].w; int n = dict[i].n; char t[SPELL_MAX_LEN + 1];
        idx_add(w, n, i, &arr, &cnt, &cap);
        for (int a = 0; a < n; a++) {                                    /* the 1-deletes */
            int k = 0; for (int q = 0; q < n; q++) if (q != a) t[k++] = w[q];
            idx_add(t, k, i, &arr, &cnt, &cap);
        }
        if (n >= 6) for (int a = 0; a < n; a++) for (int b = a + 1; b < n; b++) {     /* the 2-deletes */
            int k = 0; for (int q = 0; q < n; q++) if (q != a && q != b) t[k++] = w[q];
            idx_add(t, k, i, &arr, &cnt, &cap);
        }
    }
    if (arr) {
        qsort(arr, (size_t)cnt, sizeof *arr, cmp64);
        int m = 0; for (int i = 0; i < cnt; i++) if (!m || arr[i] != arr[m - 1]) arr[m++] = arr[i];       /* the same hash for the same word once */
        cnt = m;
    }
    idx = arr; nidx = cnt;
    uint64_t *pa = NULL; int pc = 0, pcap = 0;                           /* chunk 22: the phonetic index, only the words a pattern or a lexicon row knows */
    for (int i = 0; i < ndict; i++) {
        if (!dict[i].val) continue;
        char k[SPELL_MAX_LEN * 2 + 2]; int kl = lang_phone(dict[i].w, dict[i].n, k, (int)sizeof k);
        if (kl < 2) continue;
        idx_add(k, kl, i, &pa, &pc, &pcap);
    }
    if (pa) {
        qsort(pa, (size_t)pc, sizeof *pa, cmp64);
        int m = 0; for (int i = 0; i < pc; i++) if (!m || pa[i] != pa[m - 1]) pa[m++] = pa[i];
        pc = m;
    }
    pidx = pa; npidx = pc;
}
static void ensure(void) {
    int g = lang_tables_gen();
    if (g != built_gen) { build(); built_gen = g; }
}
double lang_spell_build_ms(void) {
    struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
    lang_tables_gen(); build(); built_gen = lang_tables_gen();
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
}
int lang_spell_dict_size(void) { ensure(); return ndict; }
int lang_spell_index_size(void) { ensure(); return nidx; }
int lang_spell_phone_size(void) { ensure(); return npidx; }
void lang_spell_set(int on) { spell_on = on; }

/* ---------- the cost ---------- */
static const char *const KEYROWS[3] = { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
static int key_pos(char c, int *row, int *col) {
    for (int r = 0; r < 3; r++) for (int i = 0; KEYROWS[r][i]; i++) if (KEYROWS[r][i] == c) { *row = r; *col = i; return 1; }
    return 0;
}
static int neighbour(char a, char b) {                         /* the keys touch on a QWERTY keyboard (same row +-1, or the row above / below +-1 with the usual stagger) */
    int ra, ca, rb, cb;
    if (!key_pos(a, &ra, &ca) || !key_pos(b, &rb, &cb)) return 0;
    if (ra == rb) return ca - cb == 1 || cb - ca == 1;
    int dr = rb - ra, dc = cb - ca;
    if (dr == 1)  return dc == 0 || dc == -1;                    /* the key below: the same column or one to the left (q -> a, w -> a ...) */
    if (dr == -1) return dc == 0 || dc == 1;
    return 0;
}
static int is_vowel(char c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }
static double sub_cost(char a, char b) { if (a == b) return 0; if (neighbour(a, b)) return 0.70; if (is_vowel(a) && is_vowel(b)) return 0.90; return 1.1; }

/* weighted optimal string alignment; also the unit distance. returns the weighted cost, *ud = unit distance (capped) */
static double wcost(const char *s, int n, const char *t, int m, int *ud) {
    enum { MX = SPELL_MAX_LEN + 2 };
    static double d[MX][MX]; static int u[MX][MX];
    for (int i = 0; i <= n; i++) { d[i][0] = i; u[i][0] = i; }
    for (int j = 0; j <= m; j++) { d[0][j] = j; u[0][j] = j; }
    for (int i = 1; i <= n; i++) for (int j = 1; j <= m; j++) {
        double sc = sub_cost(s[i - 1], t[j - 1]); int su = s[i - 1] != t[j - 1];
        double del = (i > 1 && s[i - 1] == s[i - 2]) ? 0.55 : 0.9;          /* an extra doubled letter */
        double ins = (j > 1 && t[j - 1] == t[j - 2]) ? 0.55 : 0.9;          /* a missing doubled letter */
        double best = d[i - 1][j - 1] + sc; int bu = u[i - 1][j - 1] + su;
        if (d[i - 1][j] + del < best) best = d[i - 1][j] + del;
        if (u[i - 1][j] + 1 < bu) bu = u[i - 1][j] + 1;
        if (d[i][j - 1] + ins < best) best = d[i][j - 1] + ins;
        if (u[i][j - 1] + 1 < bu) bu = u[i][j - 1] + 1;
        if (i > 1 && j > 1 && s[i - 1] == t[j - 2] && s[i - 2] == t[j - 1] && s[i - 1] != s[i - 2]) {
            if (d[i - 2][j - 2] + 0.55 < best) best = d[i - 2][j - 2] + 0.55;
            if (u[i - 2][j - 2] + 1 < bu) bu = u[i - 2][j - 2] + 1;
        }
        d[i][j] = best; u[i][j] = bu;
    }
    *ud = u[n][m];
    double c = d[n][m];
    if (s[0] != t[0]) {                                           /* the first letter is rarely wrong, less so when it was only dropped or doubled */
        if (n > 1 && m > 1 && s[0] == t[1] && s[1] == t[0]) c += 0.2;
        else if ((m == n + 1 && !memcmp(s, t + 1, (size_t)n)) || (n == m + 1 && !memcmp(s + 1, t, (size_t)m))) c += 0.3;
        else c += 0.6;
    }
    return c;
}

/* ---------- the lookup ---------- */
typedef struct { int di; double cost, score; int ud; } Cand;
static void add_cand(Cand *c, int *nc, int di) {
    for (int i = 0; i < *nc; i++) if (c[i].di == di) return;
    if (*nc < 96) { c[*nc].di = di; (*nc)++; }
}
static void look(uint32_t h, Cand *c, int *nc) {
    int lo = 0, hi = nidx;                                       /* the first entry with this hash */
    uint64_t key = (uint64_t)h << 32;
    while (lo < hi) { int mid = (lo + hi) / 2; if (idx[mid] < key) lo = mid + 1; else hi = mid; }
    for (int i = lo; i < nidx && (uint32_t)(idx[i] >> 32) == h; i++) add_cand(c, nc, (int)(uint32_t)idx[i]);
}

int lang_spell_word(const char *w, int n, char *out, int *conf, int *dist) {
    if (!spell_on || n < SPELL_MIN_LEN || n > SPELL_MAX_LEN) return 0;
    ensure();
    if (!nidx) return 0;
    Cand c[96]; int nc = 0; char t[SPELL_MAX_LEN + 1];
    look(fnv(w, n), c, &nc);                                     /* the token itself is a delete of a longer word (a missing letter) */
    for (int a = 0; a < n; a++) {                                /* 1-deletes of the token */
        int k = 0; for (int q = 0; q < n; q++) if (q != a) t[k++] = w[q];
        look(fnv(t, k), c, &nc);
    }
    int maxd = n >= 8 ? 2 : 1;
    if (maxd == 2) for (int a = 0; a < n; a++) for (int b = a + 1; b < n; b++) {
        int k = 0; for (int q = 0; q < n; q++) if (q != a && q != b) t[k++] = w[q];
        look(fnv(t, k), c, &nc);
    }
    int keep = 0;
    for (int i = 0; i < nc; i++) {
        DW *d = &dict[c[i].di];
        if (d->n == n && !memcmp(d->w, w, (size_t)n)) continue;                    /* the token itself */
        if (abs(d->n - n) > maxd) continue;
        c[i].cost = wcost(w, n, d->w, d->n, &c[i].ud);
        if (c[i].ud > maxd || c[i].ud < 1) continue;
        if (!d->val && (c[i].ud != 1 || d->rank >= 3000)) continue;                 /* a word nobody uses must be a near miss of a common word */
        c[i].score = c[i].cost + 0.06 * log2((double)(d->rank > 0 ? d->rank : 1)) - (d->val ? 0.15 : 0.0);
        c[keep++] = c[i];
    }
    if (!keep) return 0;
    int b1 = 0; for (int i = 1; i < keep; i++) if (c[i].score < c[b1].score) b1 = i;
    double s2 = 1e9; for (int i = 0; i < keep; i++) if (i != b1 && c[i].score < s2) s2 = c[i].score;
    DW *d = &dict[c[b1].di];
    int cf = 0;
    double cost = c[b1].cost;
    if (c[b1].ud == 1) cf = cost <= 0.60 ? 92 : cost <= 0.80 ? 84 : cost <= 1.0 ? 74 : cost <= 1.3 ? 66 : 58;      /* a swap, a neighbour key, a plain edit, a plain edit at the first letter */
    else cf = cost <= 1.5 ? 66 : 54;
    if (n == 3) cf -= 16; else if (n == 4) cf -= 7; else if (n == 5) cf -= 2;
    if (d->val) cf += 4;
    if (d->n != n && (d->n > n ? !memcmp(w, d->w + 1, (size_t)n) : !memcmp(w + 1, d->w, (size_t)d->n))) cf -= 12;      /* a first letter added or dropped: bright / right, plate / late are real words */
    if (d->rank < 300) cf += 6; else if (d->rank > vocab_top) cf -= 6;
    double margin = s2 - c[b1].score;
    if (keep > 1) cf -= margin < 0.10 ? 18 : margin < 0.25 ? 9 : margin < 0.5 ? 3 : 0;
    if (cf > 99) cf = 99;
    if (cf < 0) cf = 0;
    memcpy(out, d->w, (size_t)d->n); out[d->n] = 0;
    if (conf) *conf = cf;
    if (dist) *dist = c[b1].ud;
    return 1;
}

/* the two words end alike: the same last letter, or one ends in a silent e and the other in the letter before it (plees / please) */
static int tail_ok(const char *w, int n, const char *t, int m) {
    if (w[n - 1] == t[m - 1]) return 1;
    if (n < 5 || m < 5) return 0;                                    /* a short word has no room for a silent e (type / tip) */
    if (t[m - 1] == 'e' && w[n - 1] == t[m - 2]) return 1;
    if (w[n - 1] == 'e' && t[m - 1] == w[n - 2]) return 1;
    return 0;
}
/* chunk 22: the phonetic fallback. only a word that a pattern or a lexicon row knows, the same key, the same first letter, a unit distance of at most 2
 * (3 for 8+ letters; from 2 up, and for every word of up to 5 letters, the two words must end alike); ranked like the edit repair; the confidence falls with the distance, with the words that share the key and with a thin margin */
int lang_spell_phonetic(const char *w, int n, char *out, int *conf, int *dist) {
    if (!spell_on || n < SPELL_MIN_LEN || n > SPELL_MAX_LEN) return 0;
    ensure();
    if (!npidx) return 0;
    char k[SPELL_MAX_LEN * 2 + 2]; int kl = lang_phone(w, n, k, (int)sizeof k);
    if (kl < 2) return 0;
    uint32_t h = fnv(k, kl);
    int lo = 0, hi = npidx; uint64_t key = (uint64_t)h << 32;
    while (lo < hi) { int mid = (lo + hi) / 2; if (pidx[mid] < key) lo = mid + 1; else hi = mid; }
    Cand c[64]; int nc = 0, keep = 0, maxd = n >= 8 ? 3 : 2;      /* a real word the vocabulary lacks must not be bent into a near sound (comma / come): see tail_ok */
    for (int i = lo; i < npidx && (uint32_t)(pidx[i] >> 32) == h && nc < 64; i++) c[nc++].di = (int)(uint32_t)pidx[i];
    for (int i = 0; i < nc; i++) {
        DW *d = &dict[c[i].di];
        if (d->w[0] != w[0] || (d->n == n && !memcmp(d->w, w, (size_t)n)) || abs(d->n - n) > maxd) continue;
        char k2[SPELL_MAX_LEN * 2 + 2]; int k2l = lang_phone(d->w, d->n, k2, (int)sizeof k2);
        if (k2l != kl || memcmp(k, k2, (size_t)kl)) continue;                          /* a hash clash */
        c[i].cost = wcost(w, n, d->w, d->n, &c[i].ud);
        if (c[i].ud < 1 || c[i].ud > maxd) continue;
        if ((c[i].ud >= 3 || n <= 5) && c[i].ud >= 2 && !tail_ok(w, n, d->w, d->n)) continue;        /* a far fix of a short word must end the same way */
        c[i].score = c[i].cost + 0.06 * log2((double)(d->rank > 0 ? d->rank : 1)) - 0.15;
        c[keep++] = c[i];
    }
    if (!keep) return 0;
    int b1 = 0; for (int i = 1; i < keep; i++) if (c[i].score < c[b1].score) b1 = i;
    double s2 = 1e9; for (int i = 0; i < keep; i++) if (i != b1 && c[i].score < s2) s2 = c[i].score;
    DW *d = &dict[c[b1].di];
    int cf = 78 - 5 * c[b1].ud;
    if (keep == 1) cf += 6;
    if (n == 3) cf -= 16; else if (n == 4) cf -= 7; else if (n == 5) cf -= 2;
    if (d->rank < 300) cf += 6; else if (d->rank > vocab_top) cf -= 6;
    double margin = s2 - c[b1].score;
    if (keep > 1) cf -= margin < 0.10 ? 18 : margin < 0.25 ? 9 : margin < 0.5 ? 3 : 0;
    if (cf > 99) cf = 99;
    if (cf < 0) cf = 0;
    memcpy(out, d->w, (size_t)d->n); out[d->n] = 0;
    if (conf) *conf = cf;
    if (dist) *dist = c[b1].ud;
    return 1;
}

/* ---------- the protected words and the text ---------- */
/* the plain inflections of a word the engine knows are not typos (clouds, jokes, dances, appreciated, being, bigger, slowly): the vocabulary is
 * short until chunks 49-51, and the lemma rules of chunk 24 will replace this guard. s es ed d ing er est ly, with a dropped e or a doubled letter */
static int stem_known(const char *w, int n, int k) {            /* the stem w[0..k) or its variants is a known word */
    if (k < 2) return 0;
    if (lang_known_word(w, k)) return 1;
    char t[SPELL_MAX_LEN + 2]; memcpy(t, w, (size_t)k); t[k] = 'e'; t[k + 1] = 0;
    if (k + 1 <= SPELL_MAX_LEN && lang_known_word(t, k + 1)) return 1;                       /* danc-ing -> dance */
    if (k >= 3 && w[k - 1] == w[k - 2] && lang_known_word(w, k - 1)) return 1;               /* run-ning -> run */
    if (k >= 3 && w[k - 1] == 'i') { memcpy(t, w, (size_t)k - 1); t[k - 1] = 'y'; if (lang_known_word(t, k)) return 1; }   /* stud-ied -> study */
    return 0;
}
static int inflection_of_known(const char *w, int n) {
    static const char *const SUF[] = { "s", "es", "ed", "d", "ing", "er", "est", "ly", "ness", "ers", "ies", "n", "en", NULL };
    for (int i = 0; SUF[i]; i++) {
        int sl = (int)strlen(SUF[i]);
        if (sl == 1 && n >= 2 && w[n - 1] == w[n - 2]) continue;        /* thankss, helll: a doubled last letter is a typo, not a plural */
        if (n - sl >= 2 && !memcmp(w + n - sl, SUF[i], (size_t)sl) && stem_known(w, n, n - sl)) return 1;
    }
    return 0;
}
int lang_spell_protected(const char *w, int n) {
    if (n < SPELL_MIN_LEN || n > SPELL_MAX_LEN) return 1;
    for (int i = 0; i < n; i++) if (w[i] < 'a' || w[i] > 'z') return 1;
    if (vocab_protected(w, n)) return 1;
    if (lang_known_word(w, n)) return 1;
    if (lang_textspeak(w, n)) return 1;
    if (inflection_of_known(w, n)) return 1;
    return 0;
}

static int is_letter(char c) { return c >= 'a' && c <= 'z'; }

int lang_spell_text(const char *orig, const char *in, const int *inmap, int shout, char *out, int cap, int *outmap, int *src, SpellInfo *si) {
    int len = (int)strlen(in), o = 0;
    if (si) { si->nfix = 0; si->min_conf = 100; }
    int sentence_start = 1;                                       /* the next word begins a sentence */
    int i = 0;
#define PUT(ch, from) do { if (o < cap - 1) { out[o] = (ch); if (src) src[o] = (from); if (outmap) outmap[o] = inmap ? inmap[(from)] : (from); o++; } } while (0)
    while (i < len) {
        if (in[i] == ' ') { PUT(' ', i); i++; continue; }
        int j = i; while (j < len && in[j] != ' ') j++;             /* the chunk between two spaces */
        int a = i, b = j;
        while (a < b && !is_letter(in[a]) && !(in[a] >= '0' && in[a] <= '9')) a++;           /* edge punctuation stays outside the token */
        while (b > a && !is_letter(in[b - 1]) && !(in[b - 1] >= '0' && in[b - 1] <= '9')) b--;
        int n = b - a, fixed = 0, kind = SPELL_EDIT;
        char fix[SPELL_TO_MAX]; int cf = 0, dd = 0; fix[0] = 0;
        int letters = n >= 2 && n <= SPELL_MAX_LEN; for (int q = a; letters && q < b; q++) if (!is_letter(in[q])) letters = 0;
        if (spell_on && letters) {
            int skip = 0;
            if (orig && inmap) {                                   /* the case the player typed */
                int oa = inmap[a], ob = inmap[b - 1] + 1, up = 0, lo = 0;
                for (int q = oa; q < ob && orig[q]; q++) { if (orig[q] >= 'A' && orig[q] <= 'Z') up++; else if (orig[q] >= 'a' && orig[q] <= 'z') lo++; }
                if (!shout && lo == 0 && up > 0 && n <= 4) skip = 1;                         /* an acronym */
                if (!shout && up > 0 && orig[oa] >= 'A' && orig[oa] <= 'Z' && !sentence_start) skip = 1;     /* a name in the middle of a sentence */
            }
            const char *dl = (!skip && !vocab_protected(in + a, n) && !lang_textspeak(in + a, n)) ? lang_dialect(in + a, n) : NULL;
            if (dl && strlen(dl) < SPELL_TO_MAX) {                  /* chunk 22: a row of the dialect table (wassup -> whats up) */
                strcpy(fix, dl); cf = 90; dd = 0; kind = SPELL_DIALECT; fixed = 1;
            } else if (!skip && n >= SPELL_MIN_LEN && !lang_spell_protected(in + a, n)) {
                int need = n == 3 ? SPELL_MIN_CONF + 8 : n == 4 ? SPELL_MIN_CONF + 4 : SPELL_MIN_CONF;
                if (lang_spell_word(in + a, n, fix, &cf, &dd) && cf >= need) fixed = 1;
                else if (lang_spell_phonetic(in + a, n, fix, &cf, &dd) && cf >= need) { fixed = 1; kind = SPELL_PHONETIC; }   /* chunk 22: only when the edit repair found nothing it trusts */
            }
        }
        int fl = (int)strlen(fix);
        if (fixed && o + (a - i) + fl + (j - b) > cap - 1) fixed = 0;                         /* does not fit: as typed */
        if (fixed && si && si->nfix >= SPELL_MAX_FIX) fixed = 0;
        for (int q = i; q < a; q++) PUT(in[q], q);
        if (fixed) {
            for (int q = 0; q < fl; q++) { int from = (q == fl - 1) ? b - 1 : a + (q < n - 1 ? q : n - 1); PUT(fix[q], from); }
            if (si) {
                SpellFix *f = &si->fix[si->nfix++];
                f->a = inmap ? inmap[a] : a; f->b = inmap ? inmap[b - 1] + 1 : b; f->conf = cf; f->dist = dd; f->kind = kind;
                memcpy(f->from, in + a, (size_t)n); f->from[n] = 0; memcpy(f->to, fix, (size_t)fl + 1);
                if (cf < si->min_conf) si->min_conf = cf;
            }
        } else for (int q = a; q < b; q++) PUT(in[q], q);
        for (int q = b; q < j; q++) PUT(in[q], q);
        sentence_start = 0;
        for (int q = a; q < j; q++) if (in[q] == '.' || in[q] == '!' || in[q] == '?') sentence_start = 1;
        i = j;
    }
    if (o < cap) out[o] = 0; else out[cap - 1] = 0;
    if (outmap) outmap[o] = inmap ? inmap[len] : len;
    if (src) src[o] = len;
#undef PUT
    return o;
}
