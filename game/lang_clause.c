/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * CLAUSES (docs/convo_plan.md, chunk 25), see lang_clause.h. No SDL. */
#include "lang_clause.h"
#include <string.h>

static int wc(char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '\''; }
static int stop(char c) { return c == '.' || c == '!' || c == '?' || c == ';'; }
static int is_w(const char *w, int n, const char *const *list) {
    for (int i = 0; list[i]; i++) if ((int)strlen(list[i]) == n && !memcmp(list[i], w, (size_t)n)) return 1;
    return 0;
}

static const char *const NEG[] = { "not", "no", "never", "dont", "don't", "cant", "can't", "wont", "won't", "isnt", "isn't", "arent", "aren't",
    "didnt", "didn't", "hardly", "without", "aint", "ain't", "doesnt", "doesn't", "wasnt", "wasn't", "werent", "weren't", "havent", "haven't",
    "hasnt", "hasn't", "hadnt", "hadn't", "shouldnt", "shouldn't", "wouldnt", "wouldn't", "couldnt", "couldn't", "cannot", "barely",
    "scarcely", "nor", "neither", NULL };
int lang_is_negator(const char *w, int n) { return is_w(w, n, NEG); }

/* words that start a new clause when a comma or "so" comes before them */
static const char *const SUBJ[] = { "i", "you", "we", "they", "he", "she", "it", "im", "ive", "id", "ill", "u", "ur", "dont", "do", "lets", "let", "now",
    "please", "just", "stop", "tell", "give", "show", "go", "come", "be", "can", "will", "why", "what", "how", "who", "yes", "no", "yeah", "ok", "okay", NULL };
static const char *const CONTRAST[] = { "but", "however", "yet", NULL };
static const char *const CONJ[] = { "but", "however", "yet", "although", "though", "because", "then", NULL };
static const char *const BOOST[] = { "very", "so", "really", "extremely", "super", "totally", "absolutely", "deeply", "incredibly", "terribly",
    "truly", "utterly", "completely", "awfully", "insanely", "hella", NULL };
static const char *const DIM[] = { "kinda", "sorta", "somewhat", "slightly", "barely", "bit", "little", "mildly", "semi", NULL };

static int next_word(const char *lc, int len, int i, int *a, int *b) {       /* the word starting at or after i */
    while (i < len && !wc(lc[i])) { if (stop(lc[i])) return 0; i++; }
    if (i >= len) return 0;
    int j = i; while (j < len && wc(lc[j])) j++;
    *a = i; *b = j; return 1;
}

int lang_clauses(const char *lc, int len, Clause *cl, int max) {
    if (len <= 0 || max <= 0) return 0;
    int starts[CLAUSE_MAX * 4], ns = 0;                 /* byte where a clause begins; the word that opens it is the first word at or after it */
    starts[ns++] = 0;
    int i = 0, prev_comma = 0, skip_then = -1;
    while (i < len) {
        char c = lc[i];
        if (stop(c)) { int k = i + 1; while (k < len && (stop(lc[k]) || lc[k] == ' ')) k++; if (k < len && ns < (int)(sizeof starts / sizeof *starts)) starts[ns++] = k; i = k; prev_comma = 0; continue; }
        if (c == ',') { prev_comma = 1; i++; continue; }
        if (!wc(c)) { i++; continue; }
        int j = i; while (j < len && wc(lc[j])) j++;
        int n = j - i; const char *w = lc + i;
        int split = 0;
        if (i == skip_then) split = 0;                                   /* the then of "and then" */
        else if (is_w(w, n, CONJ)) split = 1;
        else if (n == 3 && !memcmp(w, "and", 3)) { int a, b; if (next_word(lc, len, j, &a, &b) && b - a == 4 && !memcmp(lc + a, "then", 4)) { split = 1; skip_then = a; } }
        else if (n == 2 && !memcmp(w, "so", 2)) { int a, b; if (next_word(lc, len, j, &a, &b) && is_w(lc + a, b - a, SUBJ)) split = 1; }
        else if (prev_comma && is_w(w, n, SUBJ)) split = 1;
        if (split && i > 0 && starts[ns - 1] < i && ns < (int)(sizeof starts / sizeof *starts)) {
            /* a conjunction right after a comma: the comma belongs to the clause before, the clause opens at the word */
            starts[ns++] = i;
        }
        prev_comma = 0;
        i = j;
    }
    /* clauses: [starts[k], starts[k+1]), the last one reaches len; more than max are merged into the last */
    int nc = ns < max ? ns : max;
    for (int k = 0; k < nc; k++) {
        cl[k].a = starts[k]; cl[k].b = (k + 1 < ns && k + 1 < nc) ? starts[k + 1] : len;
        cl[k].contrast = 0; cl[k].boost = 0;
        /* a clause that is only spaces and punctuation still counts, it just says nothing */
        int a, b;
        if (next_word(lc, cl[k].b, cl[k].a, &a, &b) && is_w(lc + a, b - a, CONTRAST)) cl[k].contrast = 1;
        int p = cl[k].a, pa = 0, pb = 0;                         /* pa..pb: the word before */
        while (p < cl[k].b) {
            if (!next_word(lc, cl[k].b, p, &a, &b)) break;
            const char *w = lc + a; int n = b - a;
            int after_a = pb > pa && pb - pa == 1 && lc[pa] == 'a';
            if (is_w(w, n, BOOST)) {
                int a2, b2;                                      /* "so" that opens a clause is a conjunction, not an intensifier */
                int conj_so = n == 2 && !memcmp(w, "so", 2) && next_word(lc, cl[k].b, b, &a2, &b2) && is_w(lc + a2, b2 - a2, SUBJ);
                if (!conj_so) cl[k].boost++;
            } else if (is_w(w, n, DIM)) {
                int needs_a = (n == 3 && !memcmp(w, "bit", 3)) || (n == 6 && !memcmp(w, "little", 6));     /* "a bit", "a little"; barely is a negator */
                if (!(n == 6 && !memcmp(w, "barely", 6)) && (!needs_a || after_a)) cl[k].boost--;
            }
            pa = a; pb = b; p = b;
        }
        if (cl[k].boost > 2) cl[k].boost = 2;
        if (cl[k].boost < -2) cl[k].boost = -2;
    }
    return nc;
}

int lang_neg_count(const char *lc, int start, int clause_a) {
    int cnt = 0, words = 0, i = start - 1, len = (int)strlen(lc);
    while (i >= clause_a && words < NEG_SCOPE) {
        while (i >= clause_a && lc[i] == ' ') i--;
        if (i < clause_a || !wc(lc[i])) break;
        int e = i;
        while (i >= clause_a && wc(lc[i])) i--;
        const char *w = lc + i + 1; int n = e - i;
        if (lang_is_negator(w, n)) {
            int is_mind = 0;                                      /* never mind is a phrase */
            if (n == 5 && !memcmp(w, "never", 5)) { int a, b; if (next_word(lc, len, e + 1, &a, &b) && b - a == 4 && !memcmp(lc + a, "mind", 4)) is_mind = 1; }
            if (!is_mind) cnt++;
        }
        words++;
    }
    return cnt;
}
