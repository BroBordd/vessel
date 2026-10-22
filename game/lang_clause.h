/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * CLAUSES, NEGATION SCOPE, CONTRAST AND INTENSITY (docs/convo_plan.md, chunk 25). No SDL.
 *
 * lang_clauses splits a cleaned text (lower case, as lang_clean writes it) into clauses at . ! ? ; and at the conjunctions
 * but however yet although though because then (and then), at "so" when it joins two clauses (so i left, not so sad) and at a comma
 * before a pronoun or a verb ("i hate it, i love you"). A conjunction opens the NEW clause. Per clause it reports whether the clause opens with a
 * contrast word (but, however, yet: the part after it wins) and the net intensity words in it (very, so, really, extremely +1 each, a bit,
 * kinda, a little -1 each).
 *
 * lang_neg_count says how many negators stand in the NEGATION SCOPE of a position: the 4 words before it, inside its clause, no punctuation
 * between. An odd number flips the meaning (not bad), an even number cancels (i do not not love you). "never mind" is a phrase, not a
 * negation. */
#ifndef LANG_CLAUSE_H
#define LANG_CLAUSE_H

#define CLAUSE_MAX 8        /* clauses kept; the rest of a long text is part of the last one */
#define NEG_SCOPE 4         /* words a negator reaches */

typedef struct {
    int a, b;               /* bytes [a, b) of the cleaned text */
    int contrast;           /* 1 = the clause opens with but / however / yet: what follows wins over what came before */
    int boost;              /* net intensity words in the clause: +1 very so really extremely ..., -1 a bit kinda ... */
} Clause;

/* returns the number of clauses (1..CLAUSE_MAX; 0 for an empty text). Clauses cover the text without gaps: cl[0].a == 0, cl[n-1].b == len */
int lang_clauses(const char *lc, int len, Clause *cl, int max);

/* negators in the scope of position `start` (a byte of lc); `clause_a` = the first byte of its clause (0 for the whole text) */
int lang_neg_count(const char *lc, int start, int clause_a);

/* is this word (lower case, w[0..n)) a negator? */
int lang_is_negator(const char *w, int n);

/* ---- chunk 58: the modifier table (lang_lex_sent_mod.c, registered by lang_tables.c) ----
 * LangModFn(w, n, &m): what the word w[0..n) does in a clause: MOD_NEG = a negator, MOD_CONTRAST = a clause that opens with it wins (but), MOD_CONJ = it opens a new clause
 * (but although because); boost = the intensity it adds to the clause (+1..+3 very, extremely; -1..-2 a bit, slightly), needs_a = the boost counts only after the word "a"
 * (a bit, a little, a tad). returns 1 when the word is a modifier. lang_init plugs it in; lang_clause_set_mods(NULL) = the built-in lists (the fallback: the table holds
 * all of their words with the same effect, langcheck --mods tests it); lang_clause_builtin_mod reads the built-in lists (tests). */
#define MOD_NEG      1
#define MOD_CONTRAST 2
#define MOD_CONJ     4
typedef struct { unsigned char flags, needs_a; signed char boost; } LangMod;
typedef int (*LangModFn)(const char *w, int n, LangMod *m);
void lang_clause_set_mods(LangModFn fn);
int  lang_clause_builtin_mod(const char *w, int n, LangMod *m);

#endif
