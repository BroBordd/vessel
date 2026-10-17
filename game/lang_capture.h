/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * CAPTURING FACTS (docs/convo_plan.md, chunk 33). No SDL.
 *
 * What the player tells about themselves, read with word rules (no learning):
 *   NAME     my name is X, call me X, i am called X, i go by X, i am X / im X (only a Capitalised X, or X in the name list)
 *   AGE      from the numbers layer (i am 12, 12 years old)
 *   PLACE    i live in X, i am from X, i come from X, i was born in X, my home is X
 *   LIKE     i like / love / enjoy / adore X, my favorite Y is X (and i do not like X is a DISLIKE)
 *   DISLIKE  i hate / dislike / despise X, i cant stand X, i am sick of X
 *   THING    i want X, i need X, i wanna X
 *   NUMBER / TIME   the first plain number / the first time of the day of the text (numbers layer)
 *   FEELING  i am tired, im very sad, i feel lost (a word of a fixed list)
 * Every captured text is what the PLAYER typed: a name or a thing is never captured when it holds a blocked word (swear, sexual, self-harm),
 * and it only comes back into a reply through the slots of lang.c, which echo a name only after lang_name_blocked said no and the rest only
 * through lang_safe_echo. Self-harm is never touched: lang.c does not call this when it heard self-harm. */
#ifndef LANG_CAPTURE_H
#define LANG_CAPTURE_H
#include <stdint.h>

enum { CAP_NONE, CAP_NAME, CAP_AGE, CAP_PLACE, CAP_LIKE, CAP_DISLIKE, CAP_THING, CAP_NUMBER, CAP_TIME, CAP_FEELING, CAP_COUNT };
#define HEARD_CAPS 6
#define CAP_TEXT 24             /* a captured text is at most 22 characters */
typedef struct { uint8_t type; char text[CAP_TEXT]; int16_t value; } Capture;   /* value: the age, the number, the minutes after midnight; 0 for the rest */

/* reads the cleaned text lc (lower case, as lang_parse sees it) and its case twin lcase (same length, capitals kept where the player typed them),
 * writes up to max captures of the kinds NAME PLACE LIKE DISLIKE THING FEELING (the number kinds come from lang.c), returns how many.
 * shout = 1 when the text is mostly capitals (a capital then says nothing, only the name list counts). *blocked = 1 when a name was offered
 * and refused (a blocked word): the caller answers it with r.name_swear_name. out may be NULL for a count only */
int  lang_capture_read(const char *lc, const char *lcase, int len, int shout, Capture *out, int max, int *blocked);
const char *lang_capture_type_name(int type);      /* "name" ... */
int  lang_name_blocked(const char *text);          /* 1 if the text holds a blocked word (whole word, or a long one inside a word) */
int  lang_name_listed(const char *word);           /* 1 if word (lower case) is in the built-in list of common first names */
int  lang_namelist_count(void);
/* turns a name the old intro pattern captured (case kept) into a clean name: the first word, plus a second Capitalised one, letters / - / ' only,
 * 2-22 characters. returns 1 if it is usable (name is rewritten), 0 if not (name is emptied). a blocked word makes it 0 and sets *blocked */
int  lang_name_clean(char *name, int *blocked);
/* tests: extra blocked words for this run (up to 16; whole word), lang_blocked_clear_test() forgets them */
int  lang_blocked_test(const char *word);
void lang_blocked_clear_test(void);

#endif
