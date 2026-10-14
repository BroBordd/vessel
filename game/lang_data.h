/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * shared shapes for the big tables (lines_*.c and lang_pats.c) */
#ifndef LANG_DATA_H
#define LANG_DATA_H
#include <stdint.h>

/* tone masks: which moods a line fits */
#define WARM 1
#define NEUT 2
#define COLD 4
#define HOST 8
#define ALLT 15
#define NICE (WARM | NEUT)
#define RUDE (COLD | HOST)
/* style masks: who can say it. a line made for one style is picked 3x more often by that style */
#define PLAIN  1
#define DIVINE 2
#define STREET 4
#define ALLS   7

/* chunk 5: tones is 32 bits (20 moods + room) and styles is 16 bits (10 voices + room). the old rows compile unchanged */
typedef struct { const char *key; uint32_t tones; uint16_t styles; const char *text; } Line;
typedef struct { const char *pat; int intent; } Pat;

/* a key may have at most this many rows (the picker has no cap, the lint fails above it) */
#define LANG_MAX_KEY_ROWS 4000

extern const Line LINES_A[], LINES_B[], LINES_C[], LINES_D[];
extern const int  LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N;
extern const Pat  PATS[];
extern const int  PATS_N;

#endif
