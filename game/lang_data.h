/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * shared shapes for the big tables (lines_*.c and lang_pats.c) */
#ifndef LANG_DATA_H
#define LANG_DATA_H

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

typedef struct { const char *key; unsigned char tones, styles; const char *text; } Line;
typedef struct { const char *pat; int intent; } Pat;

extern const Line LINES_A[], LINES_B[], LINES_C[], LINES_D[];
extern const int  LINES_A_N, LINES_B_N, LINES_C_N, LINES_D_N;
extern const Pat  PATS[];
extern const int  PATS_N;

#endif
