/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * SENTENCE FRAMES (docs/convo_plan.md, chunk 30). No SDL.
 *
 * A frame is what a clause DOES, read with verb, pronoun and punctuation rules (no learning):
 *   act       what the speaker is doing: greet, farewell, ask, tell, command, plead, offer, threaten, joke, confess, compliment, complain,
 *             boast, apologise, thank, agree, refuse, wonder, test-the-bot
 *   subject   who the clause is about: I, you, we, it, they, the npc's name
 *   tense     now, past, future
 *   modality  want, need, can, must, would, might
 *   politeness plain, please, formal, rude
 *   verb, object   the main verb (as a lemma) and the last content word
 *   qword     who / what / where / when / why / how / which / yes-no (chunk 32 types questions properly; this is the first cut)
 *   neg       how many negators the clause has
 *
 * A table of rows turns a frame into an intent (ASK + you + feel -> howru; TELL + I + tired -> tired_me; COMMAND + give -> give_me ...).
 * A row names an act mask, a subject mask, a modality mask, a question word, a verb set and an object set (words of the clause, "a&b" means
 * both words, "*" means anything), the intent, a confidence 30-69 and flags. The first row that fits wins.
 *
 * A frame only ever ADDS an intent, and only when phrases and the lexicon found nothing (or only something under confidence 40, or only a
 * sentiment word): lang.c does that gating, Heard.src says SRC_FRAME. A clause that holds a negator is skipped by every row that does not
 * say it handles negation. Self-harm is never touched by a frame. */
#ifndef LANG_FRAME_H
#define LANG_FRAME_H
#include <stdint.h>

enum { SA_NONE, SA_GREET, SA_FAREWELL, SA_ASK, SA_TELL, SA_COMMAND, SA_PLEAD, SA_OFFER, SA_THREATEN, SA_JOKE, SA_CONFESS, SA_COMPLIMENT,
       SA_COMPLAIN, SA_BOAST, SA_APOLOGISE, SA_THANK, SA_AGREE, SA_REFUSE, SA_WONDER, SA_TEST, SA_COUNT };
enum { SU_NONE, SU_I, SU_YOU, SU_WE, SU_IT, SU_THEY, SU_NPC, SU_COUNT };
enum { TN_NOW, TN_PAST, TN_FUTURE };
enum { MD_NONE, MD_WANT, MD_NEED, MD_CAN, MD_MUST, MD_WOULD, MD_MIGHT, MD_COUNT };
enum { PO_PLAIN, PO_PLEASE, PO_FORMAL, PO_RUDE };
enum { QW_NONE, QW_WHO, QW_WHAT, QW_WHERE, QW_WHEN, QW_WHY, QW_HOW, QW_WHICH, QW_YESNO };

typedef struct {
    uint8_t act, subject, tense, modality, politeness, qword, neg;
    char verb[16];          /* the main verb as a lemma (give, feel, be), "" = none */
    char object[20];        /* the last content word of the clause (tired, moon, name), "" = none */
} Frame;

#define FRAME_MAX_WORDS 40

/* read bytes [a, b) of a cleaned text (lower case, as lang_clean writes it) as one clause. fills f and returns f->act (SA_NONE for a clause with
 * no words). *intent = the intent of the first table row that fits (-1 = none), *conf its confidence, *row its index; the three may be NULL */
int  lang_frame_read(const char *lc, int a, int b, Frame *f, int *intent, int *conf, int *row);

/* ---- the table (tools / lint) ---- */
#define FRAME_F_NEGOK   1       /* the row may fire in a clause that holds a negator (i do not know -> idk) */
#define FRAME_F_NEGONLY 2       /* the row fires only in a negated clause (and implies NEGOK) */
int  lang_frame_rows(void);
int  lang_frame_row(int i, unsigned *acts, int *intent, const char **intent_name, int *conf, int *flags);   /* 0 if i is out of range; intent -1 = unknown name */
const char *lang_frame_act_name(int act);       /* "ask" ... */
const char *const *lang_frame_wordrows(void);   /* the table's words as lexicon rows "word|frame|1" (NULL ended): registered by lang_tables so the spell repair knows them */
void lang_frame_set(int on);                    /* 0 = frames off (tests: the superset rule); 1 = on (default) */
int  lang_frame_enabled(void);

/* ---- question typing (chunk 32) ----
 * every clause that asks something gets a type and a focus word. A choice question ("tea or coffee?") keeps its two options; a rhetorical one
 * ("why would i?") is not answered like a real question; a tag ("you like it, don't you?") is a statement with a request for agreement.
 * the focus and the options are what the PLAYER typed: they may only be echoed into a reply when lang_safe_echo says yes (a cast name, a topic
 * word of the lexicon, or a short word of a curated list), never anything else. */
enum { QT_NONE, QT_WHO, QT_WHAT, QT_WHERE, QT_WHEN, QT_WHY, QT_HOW, QT_WHICH, QT_HOWMUCH, QT_YESNO, QT_TAG, QT_CHOICE, QT_RHETORICAL, QT_COUNT };
typedef struct { uint8_t type; char focus[24], opt1[24], opt2[24]; } Question;
int  lang_question(const char *lc, int a, int b, Question *q);      /* reads bytes [a, b) as one clause; returns q->type (QT_NONE if it asks nothing) */
const char *lang_question_name(int type);                             /* "who" ... "choice" ... */
int  lang_safe_echo(const char *word);                                /* 1 = may be echoed into a reply: a cast name, a topic word, a word of the curated list */
int  lang_frame_fortune(const char *lc, int a, int b);               /* 1 for will i X / am i going to X / am i gonna X / shall i X (never a clause about death) */
int  lang_is_cast(const char *word);                                  /* dea dia alex aonia doia tria vas */
int  lang_choice_pick(int rude, int roll);                            /* roll 0-99 -> 0 first option, 1 second, 2 both, 3 neither (a rude mood leans to neither) */

#endif
