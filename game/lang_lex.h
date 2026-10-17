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

/* ---- chunk 28: sentiment with boosters. rows "word|sent|N" (N -5..5) of the lexicon, read per clause ---- */
#define SENT_FIRE 30        /* |valence| that makes I_POS / I_NEG heard when nothing else was */
typedef struct {
    int valence;            /* -100..100, after the sarcasm flip */
    int pos, neg;           /* weights 0-9: the positive and the negative evidence (after negation, scaling, boosters, contrast, sarcasm) */
    int p10, n10;           /* the same in tenths of a point */
    int evidence;           /* p10 + n10 before the sarcasm flip: how much feeling this text carries (picks between a text and its variants) */
    int words;              /* sentiment words found */
    int sarcasm;            /* 1 = a sarcasm cue fired and the positive part was read as mocking */
    int cue;                /* index of the cue (lang_sent_cue_name), -1 = none */
    int ctx;                /* 1 = the cue was a praise-shaped one that only counts next to a negative word (great job + idiot) */
    int to;                 /* the intent the cue points at (I_THANKS_SARCASTIC, I_NO_WAY, I_FINE_WHATEVER), -1 = none */
} SentInfo;
/* lc: a cleaned text (lower case). flags, excl, stretch, tok[TK_COUNT]: what lang_parse already knows of the typed text (Heard.flags, .excl, .stretch, .tok);
 * tok may be NULL. fills si, returns the valence */
int  lang_sent_scan(const char *lc, int len, unsigned flags, int excl, int stretch, const unsigned char *tok, SentInfo *si);
int  lang_sent_value(const char *word, int n, int *val);     /* the valence row of a word (or of its lemma): 1 and *val if there is one */
int  lang_sent_cue_count(void);
const char *lang_sent_cue_name(int i);

/* ---- chunk 29: topic words. rows "word|topic.<n>|weight" ---- */
#define TOPIC_MIN 4         /* score a topic needs to count */
#define TOPIC_MAX 3         /* topics kept in a Heard */
typedef struct { int id, score; char word[24]; } TopicHit;     /* id: lang_topic_name(id) is "moon"; word: the strongest word of the text for it */
int  lang_topic_scan(const char *lc, int len, TopicHit *out, int max);   /* strongest first; returns how many reached TOPIC_MIN */
const char *lang_topic_name(int id);                           /* the name without the topic. prefix, NULL if unknown */
int  lang_topic_id(const char *name);                          /* -1 if no row ever named it */

#endif
