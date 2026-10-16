/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE LEXICON LAYER (docs/convo_plan.md, chunk 27). No SDL.
 *
 * rows "word|intent|weight" (lang_register_lex; weight 1-9) say how strongly a single word points at an intent. lang_lex_scan reads a CLEANED text
 * clause by clause: it looks up every word (and its lemma: loved -> love), skips the words a phrase already covers, adds the weights per intent,
 * counts the last three words of a clause in full and the earlier ones at 80 %, adds the clause's intensity (very, so ...), and reports the intents
 * whose score reaches the threshold (6). a negator in the scope of a word (lang_neg_count) is reported, the caller decides what a negated hit means.
 * a lexicon hit is always WEAKER than a phrase hit (Heard.src / Heard.conf). rows whose second field is not an intent (topics, sentiment) are
 * ignored here; pos and neg rows belong to chunk 28. */
#ifndef LANG_LEX_H
#define LANG_LEX_H

#define LEX_THRESHOLD 6
#define LEX_MAXHITS 16

typedef struct {
    int intent;
    int score;          /* >= the threshold */
    int neg;            /* 1 = the words that made it stand in the scope of an odd number of negators */
    int clause;         /* index of the clause (0 for a one clause text) */
    int start, end;     /* bytes of the cleaned text: the first and last word that counted */
} LexHit;

/* covered: one byte per byte of lc, non zero where a phrase hit already covers the text (NULL = nothing covered). returns the hits (at most max) */
int  lang_lex_scan(const char *lc, int len, const unsigned char *covered, LexHit *out, int max);
void lang_lex_set_threshold(int t);     /* tests; 0 restores LEX_THRESHOLD */
int  lang_lex_threshold(void);

#endif
