/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE TABLE REGISTRY AND THE INDEXES (docs/convo_plan.md, chunk 6). No SDL.
 *
 * every data table of the language engine is registered here, and the engine asks the indexes built from them instead of walking
 * the tables: so 50,000 rows cost the same as 1,000.
 *   - reply lines (Line rows):  a KEY INDEX, rows sorted by key (in registration order inside a key) -> lang_key_rows
 *   - patterns (Pat rows):      a FIRST-LETTERS INDEX: a pattern is filed under the first 1, 2 or 3 letters of its first word, so a
 *                               word of the text meets a handful of candidates, in the original order -> lang_pat_cursor
 *   - lexicon and vocabulary:   compact rows "word|..." (chunk 8 format) with a WORD HASH -> lang_lex_find / lang_vocab_find
 * the four old line tables and PATS are registered by themselves. new files add one line (lang_register_*) in their own register
 * function; the indexes are built lazily at first use (lang_init) and again if something is registered later. */
#ifndef LANG_TABLES_H
#define LANG_TABLES_H
#include "lang_data.h"

/* ---- registry (each returns 0 on success, -1 if the registry is full) ---- */
int  lang_register_lines(const Line *t, int n);
int  lang_register_pats(const Pat *t, int n);
int  lang_register_patrows(const char *const *rows); /* chunk 26: rows "pattern|intent name|weight" (weight 1-9, optional; [a|b] (x) <m> allowed), the array ends with 0 */
int  lang_register_macros(const char *const *rows);  /* chunk 26: rows "name|alt|alt|..." (the macros <name> of a pattern), the array ends with 0 */
int  lang_register_lex(const char *const *rows);     /* rows "word|intent|weight", the array ends with 0 */
int  lang_register_vocab(const char *const *rows);   /* rows "word|rank|pos", the array ends with 0 */
int  lang_register_irregular(const char *const *rows); /* chunk 24: rows "form|lemma" (an irregular form, its dictionary form; "form|form" = never touched), the array ends with 0 */
int  lang_register_names(const char *const *rows);   /* chunk 20: rows "name" (lower case first names), the array ends with 0 */
int  lang_register_textspeak(const char *const *rows); /* chunk 18: rows "from|to" (a token as typed, its standard form), the array ends with 0 */
int  lang_register_numbers(const char *const *rows); /* chunk 52: rows "form|result|kind" (see lang_lex_numbers.c), the array ends with 0 */
int  lang_register_dialect(const char *const *rows); /* chunk 22: rows "from|to" (a dialect or phonetic spelling, its standard words), the array ends with 0 */
void lang_tables_reset(void);                        /* tests: forget everything registered but the built-in tables */

/* ---- init ---- */
void   lang_init(void);        /* builds the indexes if anything changed (cheap when nothing did) */
double lang_init_ms(void);     /* forces a full rebuild and returns how long it took, in milliseconds */

/* ---- reply lines ---- */
int  lang_key_rows(const char *key, const Line *const **rows);   /* rows of this key in table order; returns how many (0 = none) */
int  lang_key_count(void);                                       /* distinct keys */
const char *lang_key_name(int i, int *nrows);                    /* the i-th key in sorted order (tools) */
int  lang_line_count(void);                                      /* all rows */

/* ---- patterns ---- */
int  lang_pat_count(void);
const Pat *lang_pat(int id);
/* the candidates for a pattern starting at s (a word start of the lowercase text), always in ascending id (= registration) order */
typedef struct { const int *l[4], *e[4]; int last; } PatCur;
void lang_pat_cursor(PatCur *c, const char *s);
int  lang_pat_next(PatCur *c);                           /* next candidate id, or -1 */
int  lang_pat_safety(const int **ids);                   /* ids of every I_SELFHARM pattern; returns how many */
/* chunk 26: a pattern with [ ( or < is COMPILED (lang_pat.h); lang_pat_compiled(id) is NULL for a classic pattern (the old matcher runs it).
 * the weight is 1-9 (5 for every old pattern). a pattern or row that does not compile / parse is left out of the tables and listed here */
const struct LPat *lang_pat_compiled(int id);
int  lang_pat_weight(int id);
int  lang_pat_error_count(void);                          /* how many rows were left out (the first 64 are kept) */
const char *lang_pat_error(int i, const char **why);      /* the i-th left-out row and the reason, or NULL */

/* ---- words ---- */
int  lang_lex_find(const char *word, const char **rows, int max);   /* every lexicon row of this word (in registration order) */
const char *lang_vocab_find(const char *word);                      /* the vocabulary row of this word, or NULL */
/* chunk 17: is w[0..n) a word some table knows (a pattern word, a lexicon word, a vocabulary word)? used by the clean-up layer to pick the
 * reading of leet and elongated words; lang_init plugs it into lang_clean_set_known */
int  lang_known_word(const char *w, int n);
/* chunk 18: the standard form of the token w[0..n) ("idk" -> "i do not know"), or NULL; lang_init plugs it into lang_clean_set_expand */
const char *lang_textspeak(const char *w, int n);
/* chunk 22: the standard words of a dialect or phonetic spelling ("wassup" -> "whats up"), or NULL; the spell repair reads it first. lang_dialect_count = rows
 * in the map (a later row of the same key wins), lang_dialect_row(i) walks every registered row in order (tools) */
const char *lang_dialect(const char *w, int n);
const char *lang_irregular(const char *w, int n);    /* chunk 24: the row's lemma for w[0..n), or NULL */
int  lang_irregular_count(void);
const char *lang_irregular_row(int i);
int  lang_dialect_count(void);
/* chunk 52: the numbers table: the row of w[0..n) as its result text (words or digits) and kind (l u t y o), or NULL; lang_init plugs it into lang_clean_set_numtab.
 * lang_numtab_count = rows in the map, lang_numtab_row(i) walks every registered row in order (tools) */
const char *lang_numtab(const char *w, int n, int *kind);
int  lang_numtab_count(void);
const char *lang_numtab_row(int i);
const char *lang_dialect_row(int i);
/* chunk 21: is w[0..n) a word of a pattern or a lexicon row (the spell repair prefers those); every known word with that flag (the spell dictionary is built
 * from it); a counter that changes whenever the tables were rebuilt (the spell index rebuilds itself when it sees a new number) */
int  lang_value_word(const char *w, int n);
void lang_known_each(void (*fn)(const char *w, int n, int val, void *u), void *u);
int  lang_tables_gen(void);
int  lang_lex_count(void);
int  lang_vocab_count(void);
/* chunk 20: the names, and the i-th row of the vocabulary / the names in registration order (tools: the lint walks them); NULL past the end */
const char *lang_name_find(const char *name);
int  lang_name_count(void);
const char *lang_vocab_at(int i);
const char *lang_name_at(int i);

#endif
