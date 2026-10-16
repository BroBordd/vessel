/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE LEXICON LAYER. see lang_lex.h */
#include "lang_lex.h"
#include "lang.h"
#include "lang_tables.h"
#include "lang_clause.h"
#include "lang_lemma.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static int threshold = LEX_THRESHOLD;
void lang_lex_set_threshold(int t) { threshold = t > 0 ? t : LEX_THRESHOLD; }
int  lang_lex_threshold(void) { return threshold; }

static int is_word(int c) { return isalnum(c) || c == '\''; }

/* the intent and weight of one lexicon row "word|intent|weight"; -1 if the row is not an intent row */
static int row_intent(const char *row, int *weight) {
    const char *a = strchr(row, '|'); if (!a) return -1;
    const char *b = strchr(a + 1, '|');
    char name[48]; int n = b ? (int)(b - a - 1) : (int)strlen(a + 1);
    if (n < 1 || n > 40) return -1;
    memcpy(name, a + 1, (size_t)n); name[n] = 0;
    int in = lang_intent_by_name(name);
    if (in < 0 || in == I_POS || in == I_NEG || in == I_SELFHARM) return -1;      /* sentiment is chunk 28; the safety words stay phrases */
    int w = b ? atoi(b + 1) : 5;
    if (w < 1) w = 1;
    if (w > 9) w = 9;
    *weight = w;
    return in;
}

typedef struct { int intent, neg, score10, start, end, n, maxw, maxdisc; } Acc;

int lang_lex_scan(const char *lc, int len, const unsigned char *covered, LexHit *out, int max) {
    Clause cl[CLAUSE_MAX]; int ncl = lang_clauses(lc, len, cl, CLAUSE_MAX), nout = 0;
    if (ncl <= 0) return 0;
    for (int c = 0; c < ncl && nout < max; c++) {
        /* the words of the clause */
        int ws[64], we[64], nw = 0;
        for (int i = cl[c].a; i < cl[c].b && nw < 64;) {
            if (!is_word((unsigned char)lc[i])) { i++; continue; }
            int j = i; while (j < cl[c].b && is_word((unsigned char)lc[j])) j++;
            ws[nw] = i; we[nw] = j; nw++; i = j;
        }
        Acc acc[LEX_MAXHITS]; int na = 0;
        for (int k = 0; k < nw; k++) {
            int n = we[k] - ws[k];
            if (n < 1 || n > 30) continue;
            int cov = 0; if (covered) for (int q = ws[k]; q < we[k]; q++) if (covered[q]) { cov = 1; break; }
            if (cov) continue;                                           /* a phrase hit blocks the words it covers */
            char w[32]; memcpy(w, lc + ws[k], (size_t)n); w[n] = 0;
            const char *rows[12]; int nr = lang_lex_find(w, rows, 12);
            if (!nr) {                                                   /* the lemma: loved -> love */
                char lem[32]; if (lang_lemma(w, n, lem, sizeof lem) && strcmp(lem, w)) nr = lang_lex_find(lem, rows, 12);
            }
            int neg = lang_neg_count(lc, ws[k], cl[c].a) & 1;
            int late = (nw - k) <= 3;                                    /* the last three words count in full, the others at 80 % */
            for (int r = 0; r < nr; r++) {
                int wt, in = row_intent(rows[r], &wt); if (in < 0) continue;
                int s10 = wt * (late ? 10 : 8), a;
                for (a = 0; a < na; a++) if (acc[a].intent == in && acc[a].neg == neg) break;
                if (a == na) { if (na >= LEX_MAXHITS) continue; acc[na].intent = in; acc[na].neg = neg; acc[na].score10 = 0; acc[na].start = ws[k]; acc[na].end = we[k]; acc[na].n = 0; acc[na].maxw = 0; acc[na].maxdisc = 0; na++; }
                acc[a].score10 += s10; acc[a].end = we[k]; acc[a].n++;
                if (wt > acc[a].maxw) { acc[a].maxw = wt; acc[a].maxdisc = s10; }
            }
        }
        for (int a = 0; a < na && nout < max; a++) {
            int sc = (acc[a].score10 - acc[a].maxdisc + acc[a].maxw * 10 + 5) / 10;     /* the strongest word of an intent counts in full wherever it stands, the others by position */
            if (cl[c].boost > 0) sc += cl[c].boost;                      /* very, so, really ... */
            else if (cl[c].boost < 0 && sc > 0) sc += cl[c].boost;       /* a bit, kinda */
            if (sc < threshold) continue;
            out[nout].intent = acc[a].intent; out[nout].score = sc; out[nout].neg = acc[a].neg;
            out[nout].clause = c; out[nout].start = acc[a].start; out[nout].end = acc[a].end; nout++;
        }
    }
    return nout;
}
