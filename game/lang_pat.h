/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * PATTERN SYNTAX V2 (docs/convo_plan.md, chunk 26). No SDL.
 *
 * a pattern is lower case text that may now hold, next to the old  * ? $  :
 *   [a|b|c]     one of (an alternative may hold spaces, ( ) groups and other [ ] groups; an empty alternative [a|] means optional)
 *   (x)         optional word or group: (of the). the group swallows ONE neighbouring space, so  "good (to) meet you"  matches
 *               "good meet you" and "good to meet you"
 *   <m>         a macro (lang_pats_macros.c rows "name|alt|alt|..."): <you> = you|u|ya|yourself ...
 *               built in: <x> = any 1-3 words, <number> = digits (+ the words of a "number" row), <time> = 3pm 10:30 (+ words of a "time" row),
 *               <npc> = the npc's name (lpat_set_npc) + the words of an "npc" row
 *   a row may carry a weight 1-9 (default 5): of two patterns that reach the same end of the text the heavier wins, then the one with fewer *
 * a pattern with none of  [ ( <  is "classic": the old matcher (lang.c) runs it, byte for byte as before. the others are COMPILED here at init
 * into a tree of nodes (never expanded) and matched by lpat_match; lpat_heads says under which first letters the index must file it. */
#ifndef LANG_PAT_H
#define LANG_PAT_H

typedef struct LPat LPat;

/* ---- macros and the arena (everything compiled lives in one arena, freed by lpat_reset) ---- */
void lpat_reset(void);                       /* forget every macro and every compiled pattern */
int  lpat_macro_add(const char *row);        /* "name|alt|alt|..." (name may be written <name>); a later row of the same name replaces; 0 ok, -1 bad row */
int  lpat_macro_count(void);
const char *lpat_macro_name(int i);
int  lpat_macro_alts(int i);
const char *lpat_macro_alt(int i, int j);
void lpat_set_npc(const char *name);         /* what <npc> matches besides the row (lower-cased); NULL or "" = nothing */

/* ---- compile ---- */
int  lpat_is_v2(const char *src);            /* does the text hold [ ( or < (so it needs compiling)? */
LPat *lpat_compile(const char *src, const char **why);   /* NULL and *why set on an error: unbalanced, unknown macro, empty group, too deep, too big */
int  lpat_stars(const LPat *p);              /* how many * the pattern holds (the tie break of the scan: fewer wins) */

/* ---- match: the longest end of text that p can reach from s (written to *best, which starts at s); *pref = how many characters were
 * matched before the first * or $ (what a capture such as "my name is *" starts after). false if the step budget ran out */
int  lpat_match(const LPat *p, const char *s, const char *base, const char **best, int *pref);

/* ---- index: the first letters (1-3) a text must start with. *wild = 1: the pattern can start with anything (file it in the wild list)
 * this is also set when the set is bigger than max (a "huge" first word set, which slows the index: the lint warns) ---- */
#define LPAT_MAXHEADS 48
int  lpat_heads(const LPat *p, char (*out)[4], int max, int *wild, int *huge);

/* ---- surface forms (tools): up to maxforms texts the pattern stands for; * ? $ and <x> stay as written; returns how many ---- */
int  lpat_forms(const LPat *p, int maxforms, void (*fn)(const char *form, void *u), void *u);

#endif
