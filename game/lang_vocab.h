/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE VOCABULARY AND THE WORD HASH (docs/convo_plan.md, chunk 20). No SDL.
 *
 * The words the engine knows as WORDS (not as patterns): the spell repair (chunk 21), the word splitter (chunk 23) and the lemmas
 * (chunk 24) all ask this layer. Rows are compact strings, registered like every other table (lang_register_vocab):
 *     "word|rank|pos"    rank = frequency order, 1 = the most common, strictly increasing, 1..N with no gaps; pos = n noun, v verb,
 *                        a adjective, r adverb, x other (articles, pronouns, prepositions, interjections, short forms); the pos may be left out
 *     names (lang_register_names): "name"   a lower case first name (letters only), not a vocabulary word
 * A word is looked up by an FNV hash with chains (lang_tables.c): the cost does not depend on the size. Lookups are case-blind (the word is
 * lower-cased into a 40 byte buffer) and take a NUL-ended word or a pointer and a length; a word of more than 39 characters is never found.
 * Ships with a starter list (lang_vocab_a.c, 826 words) and a starter list of names (lang_vocab_names.c, 374); chunks 49-51 bring the 15,000
 * words and >= 1,500 names.
 *
 *   A word that is in the vocabulary or is a known name is NEVER "corrected": vocab_protected(w, n) is the question the spell repair asks first.
 */
#ifndef LANG_VOCAB_H
#define LANG_VOCAB_H

extern const char *const VOCAB_A[];      /* lang_vocab_a.c: ranks 1-826 */
extern const char *const NAMES_A[];      /* lang_vocab_names.c: starter first names */

int  vocab_has(const char *w);                 /* 1 if w is a vocabulary word */
int  vocab_has_n(const char *w, int n);        /* the same for w[0..n) */
int  vocab_rank(const char *w);                /* 1..N (1 = most common), 0 = not a vocabulary word */
char vocab_pos(const char *w);                 /* 'n' 'v' 'a' 'r' 'x'; 'x' when the row has no pos; 0 = not a vocabulary word */
int  vocab_is_name(const char *w);             /* 1 if w is a known first name */
int  vocab_is_name_n(const char *w, int n);
int  vocab_protected(const char *w, int n);    /* w[0..n) is a vocabulary word or a known name: never corrected */
int  vocab_size(void);                         /* rows in the vocabulary, all tables */
int  vocab_name_count(void);                   /* names known */

#endif
