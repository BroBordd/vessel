/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * DIALOGUE MEMORY (docs/convo_plan.md, chunk 35). No SDL.
 *
 * One context per chat: the last 8 player turns (cleaned text, intents, valence), the last 8 npc replies (key, mood), a fact store
 * (name, age, likes, dislikes, how often the player insulted / thanked / praised / lied / was scared, the topics asked about) and counters.
 *   lang_ctx_reset()                       a new chat starts: the chat part is cleared, the persisted facts stay
 *   lang_ctx_note(h, text, key, mood)      after every turn: the player's reading, the key the npc answered with, the npc's mood
 *   lang_ctx_facts_save / _load            the facts that outlive one chat (name, insulted, thanked): a plain struct for the save file (chunk 48)
 * Nothing here changes a reply yet: chunks 36-41 read it (ask tags, follow-ups, repetition, callbacks, threads, initiative).
 */
#ifndef LANG_CTX_H
#define LANG_CTX_H
#include "lang.h"

#define CTX_TURNS 8
#define CTX_LIKES 3
#define CTX_TOPICS 8
typedef struct { char text[96]; uint8_t n; int16_t intent[4]; int8_t valence; } CtxTurn;     /* the cleaned text (cut at 95 bytes), up to 4 intents (self-harm first, then by order said) */
typedef struct { char key[40]; int8_t mood; } CtxReply;
typedef struct {
    int turns;                                   /* player turns this chat */
    CtxTurn turn[CTX_TURNS]; int nturn;          /* ring: the last min(turns, 8), lang_ctx_turn(0) is the newest */
    CtxReply rep[CTX_TURNS]; int nrep, reps;    /* reps = npc replies this chat */
    char name[24]; int age;                      /* what the player said about themselves this chat (age 0 = unknown) */
    char likes[CTX_LIKES][24]; int nlikes;
    char dislikes[CTX_LIKES][24]; int ndislikes;
    uint8_t said_scared, insulted_count, thanked_count, complimented_count, lied_count;   /* saturate at 255 */
    int16_t asked_about[CTX_TOPICS]; uint8_t asked_n[CTX_TOPICS]; int nasked;               /* topic ids (lang_topic_name) asked about and how often */
    char ask[12];                                /* the ask tag of the npc's last line (chunk 36); empty = none */
} LangCtx;
typedef struct { char name[24]; uint8_t insulted, thanked; } CtxFacts;    /* what is saved between chats */

void lang_ctx_reset(void);
void lang_ctx_note(const Heard *h, const char *text, const char *reply_key, int mood);
const LangCtx *lang_ctx(void);
const CtxTurn  *lang_ctx_turn(int back);         /* 0 = the newest player turn, NULL if there is none that far back */
const CtxReply *lang_ctx_reply(int back);
int  lang_ctx_topic_count(int topic);            /* how often the player asked about a topic this chat */
void lang_ctx_facts_save(CtxFacts *out);
void lang_ctx_facts_load(const CtxFacts *in);    /* a NULL or empty struct forgets everything */
const CtxFacts *lang_ctx_facts(void);
#endif
