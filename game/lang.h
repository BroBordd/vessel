/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE LANGUAGE ENGINE. No SDL in here, so it can be tested on any machine (tools/langtest.c).
 *
 *   player text --lang_parse--> Heard (which intents, how positive, question or not ...)
 *   Heard + Persona --lang_reply--> a finished sentence for ANY npc
 *
 * The same words work for every npc: the lines in lines_*.c are templates with slots
 * ({player} {npc} {pet} {sigh} ...) and are filtered by the npc's TONE, which comes from
 *   - the npc's current mood towards the player (moves with what you say)
 *   - the npc's own nature (warmth, toxic)
 *   - the player's charisma (moves with how you behave)
 * and by the npc's STYLE (plain, divine, street) so a goddess does not talk like a street kid.
 *
 * Highlighting: every pattern in lang_pats.c is a case-insensitive wildcard (* = any run of
 * letters inside one sentence, ? = one letter). lang_highlight marks the characters that matched.
 */
#ifndef LANG_H
#define LANG_H
#include <stdint.h>

/* what the player can mean. keep the order in sync with INTENT_NAME / INTENT_LABEL in lang.c */
typedef enum {
    I_GREET, I_BYE, I_YES, I_NO, I_OK, I_THANKS, I_SORRY, I_HOWRU, I_WHORU, I_WHERE,
    I_WHOAMI, I_MISSION, I_WHY, I_HOW, I_WHAT, I_HELP, I_INSULT, I_PRAISE, I_LOVE, I_WORSHIP,
    I_JOKE, I_SCARED, I_CONFUSED, I_ANGRY, I_SAD, I_INTRO, I_REPEAT, I_SKY, I_MUSIC, I_HOLE,
    I_SECRET, I_LIFE, I_WAIT, I_PLEASE, I_AGREE, I_DISAGREE, I_DONTCARE, I_DEMAND, I_CALLOUT, I_AGE,
    I_REAL, I_ADDRESS, I_SELFHARM, I_ALEX,
    I_POS, I_NEG,                       /* pure sentiment words: only used when nothing else matched */
    I_COUNT
} Intent;

typedef enum { TONE_WARM, TONE_NEUTRAL, TONE_COLD, TONE_HOSTILE } Tone;
enum { STYLE_PLAIN = 1, STYLE_DIVINE = 2, STYLE_STREET = 4 };

typedef struct {
    const char *name;
    int style;          /* STYLE_* */
    int mood;           /* -100..100, how they feel about the player right now (changes in a chat) */
    int warmth;         /* 0..100, how kind they are by nature */
    int toxic;          /* 0..100, how much they like looking down on people */
    int patience;       /* replies before they get bored of a free chat */
} Persona;

typedef struct {
    uint64_t mask;      /* bit per Intent */
    int order[12], n;   /* intents in the order they were said */
    int positive, negative;
    int is_question, words;
    char capture[24];   /* "my name is X" -> X */
} Heard;

/* ---- setup ---- */
void lang_seed(unsigned s);
void lang_set_player(const char *name);          /* what {player} turns into */
int  lang_charisma(void);                        /* 0..100, starts at 50 */
void lang_add_charisma(int d);

/* ---- understanding ---- */
void lang_parse(const char *text, Heard *h);
/* mask[i] = 1 for every character of text that belongs to something the npcs understand.
 * returns how many separate phrases were highlighted. mask needs strlen(text)+1 bytes */
int  lang_highlight(const char *text, uint8_t *mask);
void lang_heard_label(const Heard *h, char *out, int cap);   /* "HEARD: HELLO + WHO ARE YOU" */

/* ---- answering ---- */
Tone lang_tone(const Persona *ps);               /* how they feel like talking right now (a bit random) */
void lang_apply(Persona *ps, const Heard *h);    /* moves mood + charisma for what was said */
int  lang_reply(Persona *ps, const Heard *h, char *out, int cap);   /* applies, then answers. returns 1 if a real intent was answered */
int  lang_line(const Persona *ps, const char *key, char *out, int cap);   /* any named line, formatted. 0 if none */
int  lang_line_tone(const Persona *ps, const char *key, Tone t, char *out, int cap);

#endif
