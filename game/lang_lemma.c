/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LEMMAS (docs/convo_plan.md, chunk 24), see lang_lemma.h. No SDL. */
#include "lang_lemma.h"
#include "lang_tables.h"
#include "lang_vocab.h"
#include <string.h>

static int lemma_on = 1;
void lang_lemma_set(int on) { lemma_on = on != 0; }

static int is_letter(char c) { return c >= 'a' && c <= 'z'; }
static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int vowel(char c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }
static int ends(const char *w, int n, const char *suf) { int s = (int)strlen(suf); return n >= s && !memcmp(w + n - s, suf, (size_t)s); }

/* a one syllable stem that ends consonant-vowel-consonant (hat, car, hop, bor, scar): the e was dropped before -ing / -ed / -er
 * (hating, caring, hoped, bored, cuter), a doubled letter would have meant the plain stem (hatting). w, x and y never double */
static int cvc_mono(const char *s, int m) {
    if (m < 3) return 0;
    char c = s[m - 1];
    if (vowel(c) || c == 'w' || c == 'x' || c == 'y') return 0;
    if (!vowel(s[m - 2]) || vowel(s[m - 3])) return 0;
    int groups = 0, in = 0;
    for (int i = 0; i < m; i++) { int v = vowel(s[i]); if (v && !in) groups++; in = v; }
    return groups == 1;
}

/* is the stem a real word, and is it the right way round? the vocabulary decides: a stem a pattern, a lexicon or the vocabulary knows.
 * A typed word that is a vocabulary word itself (number, mother) is only reduced when the stem is not far RARER than the word
 * (loved -> love yes; number -> numb no, once the vocabulary has numb at rank 12,000 against number at 186). A stem that only a pattern or a
 * lexicon knows is always good enough: the engine reads that word */
static int accept(const char *surf, const char *stem, int m) {
    if (!lang_known_word(stem, m)) return 0;
    int rs = vocab_rank(surf);
    if (!rs) return 1;
    int rt = vocab_rank(stem);
    return rt == 0 || rt <= rs * 8;
}
static int try_stem(const char *surf, int n, const char *stem, int m, char *res) {
    if (m < 3 || m > LEMMA_MAX_LEN) return 0;
    if (m == n && !memcmp(surf, stem, (size_t)n)) return 0;
    if (!accept(surf, stem, m)) return 0;
    memcpy(res, stem, (size_t)m); res[m] = 0;
    return 1;
}
/* T(keep, add): the stem w[0..keep) + add */
#define T(keep, add) do { int m_ = build(b, w, (keep), (add)); if (try_stem(w, n, b, m_, res)) return 1; } while (0)
static int build(char *b, const char *w, int keep, const char *add) {
    if (keep < 0 || keep > LEMMA_MAX_LEN) { b[0] = 0; return 0; }
    memcpy(b, w, (size_t)keep); size_t a = strlen(add); memcpy(b + keep, add, a + 1);
    return keep + (int)a;
}

/* -er / -est (bigger, nicer, happiest, players): the plain stem needs 4 letters (sup is not the stem of super, for is not the stem of forest) */
static int comp_lemma(const char *w, int n, int k, char *res) {
    char b[LEMMA_MAX_LEN + 4];
    if (k >= 3 && cvc_mono(w, k)) T(k, "e");
    if (k >= 4) T(k, "");
    if (k >= 3) T(k, "e");
    if (k >= 4 && w[k - 1] == w[k - 2] && !vowel(w[k - 1])) T(k - 1, "");
    return 0;
}

static int suffix_lemma(const char *w, int n, char *res, int *past) {
    char b[LEMMA_MAX_LEN + 4];
    *past = 0;
    if (n >= 7 && ends(w, n, "ness")) { if (w[n - 5] == 'i') T(n - 5, "y"); T(n - 4, ""); }
    if (n >= 5 && ends(w, n, "ies")) { T(n - 3, "y"); T(n - 1, ""); }
    if (n >= 5 && ends(w, n, "ied")) { *past = 1; T(n - 3, "y"); *past = 0; }
    if (n >= 6 && ends(w, n, "iest")) T(n - 4, "y");
    if (n >= 6 && ends(w, n, "ier")) T(n - 3, "y");
    if (n >= 6 && ends(w, n, "ily")) T(n - 3, "y");
    if (n >= 5 && ends(w, n, "ing")) {
        int k = n - 3;
        if (n >= 6 && ends(w, n, "ying")) T(n - 4, "ie");                         /* dying, lying, tying */
        if (k >= 3 && cvc_mono(w, k)) T(k, "e");                                  /* hating, caring, hoping */
        if (k >= 3) T(k, "");
        if (k >= 3) T(k, "e");                                                    /* loving */
        if (k >= 4 && w[k - 1] == w[k - 2] && !vowel(w[k - 1])) T(k - 1, "");     /* running, sitting */
    }
    if (n >= 4 && ends(w, n, "ed")) {
        int k = n - 2;
        *past = 1;                                                                /* a past form: the lemma text leaves it alone (answered me is not answer me) */
        if (k >= 3 && cvc_mono(w, k)) T(k, "e");                                  /* hated, scared, bored */
        if (k >= 3) T(k, "");
        if (k >= 3) T(k, "e");                                                    /* loved */
        if (k >= 4 && w[k - 1] == w[k - 2] && !vowel(w[k - 1])) T(k - 1, "");     /* stopped, planned */
        if (k >= 3 && w[k - 1] == 'i') T(k - 1, "y");
        *past = 0;
    }
    if (n >= 6 && ends(w, n, "est") && comp_lemma(w, n, n - 3, res)) return 1;
    if (n >= 5 && ends(w, n, "er") && comp_lemma(w, n, n - 2, res)) return 1;
    if (n >= 5 && ends(w, n, "ly")) {
        if (n - 2 >= 3) T(n - 2, "");
        if (w[n - 3] == 'l') T(n - 1, "");                                        /* fully -> full */
        T(n - 1, "e");                                                            /* simply -> simple, terribly -> terrible */
    }
    if (n >= 5 && ends(w, n, "es")) { T(n - 1, ""); T(n - 2, ""); }
    if (n >= 4 && w[n - 1] == 's' && w[n - 2] != 's' && w[n - 2] != 'u' && w[n - 2] != 'i') T(n - 1, "");
    return 0;
}

int lang_lemma_ex(const char *w, int n, char *out, int cap, int *past) {
    char s[LEMMA_MAX_LEN + 1];
    if (past) *past = 0;
    if (cap > 0 && out) { int c = n < cap - 1 ? n : cap - 1; if (c < 0) c = 0; memcpy(out, w, (size_t)c); out[c] = 0; }
    if (!lemma_on || n < 2 || n > LEMMA_MAX_LEN) return 0;
    for (int i = 0; i < n; i++) {
        char c = w[i]; if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        if (!is_letter(c)) return 0;
        s[i] = c;
    }
    s[n] = 0;
    char res[LEMMA_MAX_LEN + 4]; res[0] = 0;
    int pst = 0;
    if (lang_textspeak(s, n) || lang_dialect(s, n)) return 0;       /* hows, wanna, wassup: the clean-up and spell layers read those, not the suffix rules */
    const char *ir = lang_irregular(s, n);
    if (ir) {                                                    /* a row of the irregular table: "form|lemma" or "form|lemma|p" (a past form); "form|form" means: never touched */
        const char *bar = strchr(ir, '|');
        size_t l = bar ? (size_t)(bar - ir) : strlen(ir);
        if (bar && bar[1] == 'p') pst = 1;
        if (l == (size_t)n && !memcmp(ir, s, (size_t)n)) return 0;
        if (l == 0 || l > LEMMA_MAX_LEN) return 0;
        memcpy(res, ir, l); res[l] = 0;
    } else {
        if (n < LEMMA_MIN_LEN) return 0;
        if (vocab_is_name_n(s, n)) return 0;
        if (!suffix_lemma(s, n, res, &pst)) return 0;
    }
    if (past) *past = pst;
    if (!out || cap <= 0) return 1;
    size_t l = strlen(res); if ((int)l > cap - 1) return 0;
    memcpy(out, res, l + 1);
    return 1;
}
int lang_lemma(const char *w, int n, char *out, int cap) { return lang_lemma_ex(w, n, out, cap, NULL); }

static int aux_lemma(const char *l) { return !strcmp(l, "be") || !strcmp(l, "have") || !strcmp(l, "do") || !strcmp(l, "go"); }

int lang_lemma_text(const char *in, char *out, int cap, int *src, uint8_t *rep, LemmaInfo *li) {
    int len = (int)strlen(in), o = 0, i = 0;
    if (li) { li->nlemma = 0; li->nlog = 0; }
#define PUT(ch, from, r) do { if (o < cap - 1) { out[o] = (ch); if (src) src[o] = (from); if (rep) rep[o] = (uint8_t)(r); o++; } } while (0)
    while (i < len) {
        if (!is_letter(in[i])) { PUT(in[i], i, 0); i++; continue; }
        int j = i; while (j < len && is_letter(in[j])) j++;
        int n = j - i;
        int odd = (i > 0 && (in[i - 1] == '\'' || in[i - 1] == '-' || is_digit(in[i - 1]))) || (j < len && (in[j] == '\'' || in[j] == '-' || is_digit(in[j])));
        char lem[LEMMA_MAX_LEN + 4]; int changed = 0, ll = 0;
        int past = 0;
        if (!odd && n >= 2 && n <= LEMMA_MAX_LEN && lang_lemma_ex(in + i, n, lem, (int)sizeof lem, &past)) { ll = (int)strlen(lem); changed = ll > 0 && !past && o + ll <= cap - 1 && !aux_lemma(lem); }
        if (changed) {
            for (int q = 0; q < ll; q++) PUT(lem[q], q == ll - 1 ? j - 1 : i + (q < n - 1 ? q : n - 1), 1);
            if (li) {
                li->nlemma++;
                if (li->nlog < LEMMA_MAX_LOG) {
                    LemmaFix *f = &li->fix[li->nlog++];
                    f->a = i; f->b = j; memcpy(f->from, in + i, (size_t)n); f->from[n] = 0; memcpy(f->to, lem, (size_t)ll + 1);
                }
            }
        } else for (int q = i; q < j; q++) PUT(in[q], q, 0);
        i = j;
    }
    if (o < cap) out[o] = 0; else out[cap - 1] = 0;
    if (src) src[o] = len;
    if (rep) rep[o] = 0;
#undef PUT
    return o;
}
