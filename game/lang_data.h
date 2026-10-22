/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * shared shapes for the big tables (lines_*.c and lang_pats.c) */
#ifndef LANG_DATA_H
#define LANG_DATA_H
#include <stdint.h>

/* tone masks: which moods a line fits. the first four keep bits 1, 2, 4, 8 so every old table compiles (chunk 7: 20 moods) */
#define WARM 1u
#define NEUT 2u
#define COLD 4u
#define HOST 8u
#define PLAY (1u << 4)    /* playful */
#define SARC (1u << 5)    /* sarcastic */
#define WEAR (1u << 6)    /* weary */
#define WORR (1u << 7)    /* worried */
#define CURI (1u << 8)    /* curious */
#define SOLE (1u << 9)    /* solemn */
#define SMUG (1u << 10)   /* smug */
#define TEND (1u << 11)   /* tender */
#define SULK (1u << 12)   /* sulky */
#define SUSP (1u << 13)   /* suspicious */
#define AFRA (1u << 14)   /* afraid */
#define HOLL (1u << 15)   /* hollow */
#define MANI (1u << 16)   /* manic */
#define SHY  (1u << 17)   /* shy */
#define BORE (1u << 18)   /* bored */
#define EXCI (1u << 19)   /* excited */
/* the groups of docs/convo_plan.md: nice + (+ rude) + dark cover all 20; the old rows that said NICE or RUDE meant the old four, so they now say so */
#define NICE (WARM | NEUT | PLAY | TEND | CURI | EXCI | SHY)
#define RUDE (COLD | HOST | SARC | SMUG | SULK | SUSP)
#define DARK (AFRA | HOLL | MANI | SOLE | WEAR | WORR | BORE)
#define ALLT (NICE | RUDE | DARK)
/* style (voice) masks: who can say it. a line made for one voice is picked 3x more often by that voice. chunk 10: ten voices, ALLS is all ten */
#define PLAIN   1
#define DIVINE  2
#define STREET  4
#define ELDER   8
#define CHILD   16
#define FORMAL  32
#define BEAST   64
#define MACHINE 128
#define GHOST   256
#define POET    512
#define ALLS    1023

/* chunk 5: tones is 32 bits (20 moods + room) and styles is 16 bits (10 voices + room). the old rows compile unchanged */
typedef struct { const char *key; uint32_t tones; uint16_t styles; const char *text; const char *ask; } Line;      /* ask (chunk 36): the ask tag of the line (yn name why any choice more ready agree), NULL = none; a line that ends in a question gets one by rule (lang_line_ask) */
typedef struct { const char *pat; int intent; } Pat;

/* a key may have at most this many rows (the picker has no cap, the lint fails above it) */
#define LANG_MAX_KEY_ROWS 4000

const char *lang_line_ask(const Line *l);      /* chunk 36: the line's own ask tag, else the tag its text gets by rule ("" none) */
extern const Line LINES_A[], LINES_B[], LINES_C[], LINES_D[], LINES_E[];
extern const int  LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N, LINES_E_N;
extern const char *const PATROWS_G15[];       /* chunk 31: the fourth wall */
extern const char *const PATROWS_SAFETY[];   /* chunk 54b: widened self-harm detection rows, game/lang_pats_safety.c */
extern const Pat  PATS[];
extern const char *const DIALECT[];     /* chunk 22: rows "from|to" (a dialect or phonetic spelling, the standard words), game/lang_lex_dialect.c */
extern const char *const LEX_CORE[];      /* chunk 27: rows "word|intent|weight", game/lang_lex_core.c */
extern const char *const TOPIC_CORE[];     /* chunk 29: rows "word|topic.<n>|weight", game/lang_lex_topic_core.c */
extern const char *const TOPIC_A[];        /* chunk 59: world topic rows, game/lang_lex_topic_a.c */
extern const char *const SENT_CORE[];      /* chunk 28: rows "word|sent|N" (-5..5), game/lang_lex_sent_core.c */
extern const char *const SENT_POS_A[];     /* chunk 56a: positive valence words, game/lang_lex_sent_pos_a.c */
extern const char *const SENT_POS_B[];     /* chunk 56b: more positive valence words, game/lang_lex_sent_pos_b.c */
extern const char *const SENT_POS_C[];     /* chunk 56c: positive valence words picked from the VADER lexicon, game/lang_lex_sent_pos_c.c */
extern const char *const MODS[];           /* chunk 58: rows "word|mod|kind" (intensifiers, diminishers, negators, contrast and conjunction words), game/lang_lex_sent_mod.c */
extern const char *const SENT_NEG_A[];     /* chunk 57a: negative valence words, game/lang_lex_sent_neg_a.c */
extern const char *const MACROS_A[];    /* chunk 26: rows "name|alt|alt|..." (the pattern macros <name>), game/lang_pats_macros.c */
extern const char *const LEX_MASKS[];     /* chunk 54a: rows "word|intent|weight" (swear_only, insult, sexual_blocked, slur_blocked spellings), game/lang_lex_masks.c */
extern const char *const NAMES_GUARD[];   /* chunk 54a: real words and places that contain a profanity row (essex, assassin ...): registered as names so they are never split or repaired, game/lang_lex_masks.c */
extern const char *const MASKROOTS[];    /* chunk 54a: rows "root|intent|weight" a censored token (f*ck) is matched against by its letters, game/lang_lex_masks.c */
extern const char *const EMOJI[];      /* chunk 55: rows "form|kind|weight" (emoticons and emoji as sentiment tokens), game/lang_lex_emoji.c */
extern const char *const NUMBERS[];    /* chunk 52: rows "form|result|kind" (number-letter words, number-word and ordinal aliases), game/lang_lex_numbers.c */
extern const char *const IRREGULAR[];  /* chunk 24: rows "form|lemma" ("form|form" = never touched), game/lang_lex_irregular.c */
extern const char *const TEXTSPEAK[];   /* chunk 18: rows "from|to", game/lang_lex_textspeak.c */
extern const int  PATS_N;

#endif
