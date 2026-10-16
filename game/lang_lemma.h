/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LEMMAS (docs/convo_plan.md, chunk 24). No SDL.
 *
 * lang_lemma(word) gives the dictionary form of an inflected word: loved, loving, loves -> love; children -> child; went -> go;
 * happier -> happy; bigger -> big. Two sources, in this order:
 *   1. the irregular table (lang_lex_irregular.c, rows "form|lemma", chunk 53 grows it). A row "form|form" says the word is
 *      never touched (news, number, forest, early ...);
 *   2. suffix rules (-ness -ies -ied -ier -iest -ily -ing -ed -est -er -ly -es -s) with the dropped e and the doubled letter
 *      put back. A rule only fires when the stem is a REAL word (the vocabulary, or a word a pattern or lexicon knows) and,
 *      when the typed word is a vocabulary word itself, only when the stem is the more common one (so number does not become numb).
 *
 * Like the expanded (chunk 18) and the spelled (chunk 21) text, the lemma text is a VARIANT: lang_lemma_text writes a copy of the
 * cleaned text with every inflected word replaced by its lemma, lang_parse / lang_highlight scan it next to the text as typed and
 * add only the intents the typed text did not hear (the superset rule), so a pattern written with love also hears loved and
 * loving without a star, and the typed text is never lost. */
#ifndef LANG_LEMMA_H
#define LANG_LEMMA_H
#include <stdint.h>

#define LEMMA_MIN_LEN 4         /* the suffix rules leave shorter words alone (the irregular table still sees 2-3 letter words) */
#define LEMMA_MAX_LEN 24
#define LEMMA_MAX_LOG 12        /* replaced words reported per text */

typedef struct { int a, b; char from[LEMMA_MAX_LEN + 1], to[LEMMA_MAX_LEN + 1]; } LemmaFix;     /* a, b: bytes [a, b) of the text the call got */
typedef struct { int nlemma; int nlog; LemmaFix fix[LEMMA_MAX_LOG]; } LemmaInfo;

/* the lemma of w[0..n) (lower case letters; capitals are folded) written to out. Returns 1 when it differs from the word, else 0
 * (and out is a copy of the word). Never longer than LEMMA_MAX_LEN. A word with a digit, an apostrophe or a hyphen is left alone */
int  lang_lemma(const char *w, int n, char *out, int cap);
/* the same, and *past (may be NULL) is set to 1 when the word is a PAST form (loved, hated, went, told: a "-ed" suffix rule, or an irregular row
 * ending "|p"). The lemma text leaves past forms alone: "you answered me" must not hear the demand "answer me" */
int  lang_lemma_ex(const char *w, int n, char *out, int cap, int *past);

/* the lemma variant of a cleaned text (lower case, as lang_clean writes it): every word of 2+ letters is replaced by its lemma when
 * it has one, except past forms (see lang_lemma_ex). src[o] = the index in `in` of the character out[o] stands for (a replaced word maps its last letter to the last letter
 * of the typed word and the others one to one, the same rule as lang_spell_text); rep[o] = 1 when out[o] belongs to a replaced word.
 * src and rep may be NULL. The text as typed is not touched. Never replaced: names (vocab_is_name), words with a digit, an apostrophe or
 * a hyphen, and the lemmas be / have / do / go (is -> be would let "you are nice" hear the pattern "be nice"; the table still maps them for
 * lang_lemma). Returns the length; li (may be NULL) gets the number of words replaced. */
int  lang_lemma_text(const char *in, char *out, int cap, int *src, uint8_t *rep, LemmaInfo *li);

void lang_lemma_set(int on);        /* 0 = the lemma step is off (tests: the superset rule); 1 = on (default) */

#endif
