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
 * Chunk 36: the ask tag of the npc's last line (and the question as spoken) is kept here; lang_parse reads a short answer relative to it.
 * Chunks 37-41 read the rest (follow-ups, repetition, callbacks, threads, initiative).
 */
#ifndef LANG_CTX_H
#define LANG_CTX_H
#include "lang.h"

#define CTX_TURNS 8
#define CTX_LIKES 3
#define CTX_TOPICS 8
typedef struct { char name[24]; uint8_t insulted, thanked; } CtxFacts;    /* what is saved between chats */
/* chunk 39: callbacks. The kinds the npc may bring back (reply key cb.<name>; the two _past kinds use facts of EARLIER chats) */
enum { CB_INSULTED, CB_THANKED, CB_COMPLIMENTED, CB_NAME, CB_SCARED, CB_LIED, CB_ASKED_BEFORE, CB_APOLOGY_TAKEN, CB_SAME_TOPIC, CB_INSULTED_PAST, CB_NAME_PAST, CB_COUNT };
#define CB_GAP 4          /* at most one callback in 4 replies */
#define CB_MAX_PER_KIND 2 /* and a kind comes back at most twice per chat (the _past kinds once) */
typedef struct { int kind; char fact[24]; } CbCand;
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
    char last_text[160];                         /* chunk 37: the npc's last reply as spoken ("" after self-harm): follow-ups resay or extend it */
    uint8_t rep_n, copy_n;                       /* chunk 38: how many repeat / spam turns and copycat turns in a row (the ladder step of the last one); a different turn or an apology resets them */
    CtxFacts prior;                              /* chunk 39: the persisted facts as this chat began (what earlier chats left) */
    uint8_t apologized;                          /* chunk 39: the player said sorry after insulting the npc this chat */
    uint8_t cb_cnt[CB_COUNT]; int cb_n, cb_at;   /* chunk 39: callbacks made this chat per kind, in all, and the reply count right after the last one */
    char ask_text[96];                           /* the question sentence of that line as spoken (chunk 36: a choice needs its two options) */
} LangCtx;

void lang_ctx_reset(void);
void lang_ctx_note(const Heard *h, const char *text, const char *reply_key, int mood);
const LangCtx *lang_ctx(void);
const CtxTurn  *lang_ctx_turn(int back);         /* 0 = the newest player turn, NULL if there is none that far back */
const CtxReply *lang_ctx_reply(int back);
int  lang_ctx_topic_count(int topic);            /* how often the player asked about a topic this chat */
void lang_ctx_set_ask(const char *tag, const char *text);   /* chunk 36: the npc just asked something (tag "" clears); lang_ctx_note does this from lang_reply_ask() */
void lang_ctx_set_last(const char *key, const char *text);  /* chunk 37: the npc just said `text` with reply key `key` (tests, corpus @last lines; lang_ctx_note does this itself) */
void lang_ctx_set_ask_line(const char *text);    /* chunk 36: a scripted line spoken without lang_reply (the story's own question): the tag it gets by rule */
/* chunk 38: does this reading repeat, spam or copy? Called by lang_parse after the other layers, so h already holds what was heard.
 * REP_REPEAT: the same text as the last turn (after cleaning, or a near twin of 3+ words), or the same intents for the third turn running with half the words the same.
 * REP_SPAM: one word typed 4+ times (60 % of the words), a character 12+ times in a row, or a long text pasted twice.
 * REP_COPYCAT: the npc's own last reply typed back (equal, a 4+ word piece of it, or a near twin).
 * *level = the ladder step this turn would be on (1-5 rep, 1-3 copycat; spam starts at 2). Never for self-harm, sorry, distress, confusion, an answer to a question or a follow-up. */
enum { REP_NONE, REP_REPEAT, REP_SPAM, REP_COPYCAT };
int  lang_ctx_repeat_kind(const Heard *h, const char *raw, const char *lc, int *level);
/* chunk 39: which callbacks fit the turn that is being answered (call BEFORE lang_ctx_note of that turn). Fills out[] (>= CB_COUNT) in the order they should be
 * tried: the ones about the topic just asked, then the apology, insults, fear, thanks, praise, the name, a lie, the persisted ones; a kind used less comes first.
 * Returns 0 when none fits: nothing for self-harm, sadness, fear, confusion or a call for help, nothing less than CB_GAP replies after the last callback,
 * nothing about a thing the player is doing this very turn, and nothing on the turn right after it (an insult is answered, not recalled).
 * `forgetful` (Q_FORGETFUL) drops the name kinds. Pure: no random numbers, no state change. */
int  lang_ctx_callbacks(const Heard *h, int forgetful, CbCand *out);
const char *lang_cb_key(int kind);                 /* "cb.insulted" ... */
int  lang_cb_by_key(const char *key);              /* -1 if it is no callback key */
void lang_ctx_set_counts(int insulted, int thanked, int complimented, int lied, int scared);   /* tests */
void lang_ctx_facts_save(CtxFacts *out);
void lang_ctx_facts_load(const CtxFacts *in);    /* a NULL or empty struct forgets everything */
const CtxFacts *lang_ctx_facts(void);
#endif
