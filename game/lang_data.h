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
typedef struct { const char *key; uint32_t tones; uint16_t styles; const char *text; } Line;
typedef struct { const char *pat; int intent; } Pat;

/* a key may have at most this many rows (the picker has no cap, the lint fails above it) */
#define LANG_MAX_KEY_ROWS 4000

extern const Line LINES_A[], LINES_B[], LINES_C[], LINES_D[];
extern const int  LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N;
extern const Pat  PATS[];
extern const char *const DIALECT[];     /* chunk 22: rows "from|to" (a dialect or phonetic spelling, the standard words), game/lang_lex_dialect.c */
extern const char *const TEXTSPEAK[];   /* chunk 18: rows "from|to", game/lang_lex_textspeak.c */
extern const int  PATS_N;

#endif
