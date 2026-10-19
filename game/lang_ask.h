/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * ASK TAGS (docs/convo_plan.md, chunk 36). No SDL.
 *
 * What the npc just asked decides how a short answer is read. A line the npc speaks may carry an ask tag (yn name why any choice more ready
 * agree): the Line's own `ask` field, or one given by rule (lang_ask_tag_text). lang_ctx keeps the tag of the last line; lang_parse then reads
 * a bare yes / no / name / because ... / one of two options relative to it. This file holds the two pure halves:
 *   - the rules that give a question its tag (lang_ask_tag_text, lang_ask_sentence), declared in lang.h and lang_data.h
 *   - lang_ask_read: given the tag and the player's cleaned text, is it an answer of that kind? (no Heard, no random numbers)
 * The engine (lang.c, ask_pass) turns an answer into an intent: AGREE_TO_ASK, DECLINE_TO_ASK, BECAUSE, ANSWER_CHOICE or the name capture. */
#ifndef LANG_ASK_H
#define LANG_ASK_H
#include "lang.h"

enum { ASK_NONE, ASK_YES, ASK_NO, ASK_NAME, ASK_BECAUSE, ASK_CHOICE };
typedef struct {
    int kind;            /* ASK_* */
    char name[24];       /* ASK_NAME: the word as typed (title cased); lang_name_clean decides if it is safe */
    int pick;            /* ASK_CHOICE: 1 first, 2 second, 3 both, 4 neither */
} AskAnswer;

/* tag: the ask tag in force. ask_text: the question the npc asked (any case, it is cleaned here). lc: the player's cleaned lower case text, lcase the
 * same text with the capitals as typed (same length; may be NULL = no capitals known). Returns the kind (ASK_NONE when the text is no answer of
 * the kind the tag asks for). A text of more than 8 words is never a bare answer. */
int lang_ask_read(const char *tag, const char *ask_text, const char *lc, const char *lcase, AskAnswer *out);
#endif
