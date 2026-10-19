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
#include "lang_intents.h"
#include "lang_clean.h"      /* TK_* token kinds and the LF_* shape flags of Heard.flags (chunk 16) */
#include "lang_frame.h"      /* Frame: what a clause does (chunk 30) */
#include "lang_capture.h"    /* Capture: facts the player told (chunk 33) */

/* what the player can mean. keep the order in sync with INTENT_NAME / INTENT_LABEL in lang.c */
typedef enum {
    I_GREET, I_BYE, I_YES, I_NO, I_OK, I_THANKS, I_SORRY, I_HOWRU, I_WHORU, I_WHERE,
    I_WHOAMI, I_MISSION, I_WHY, I_HOW, I_WHAT, I_HELP, I_INSULT, I_PRAISE, I_LOVE, I_WORSHIP,
    I_JOKE, I_SCARED, I_CONFUSED, I_ANGRY, I_SAD, I_INTRO, I_REPEAT, I_SKY, I_MUSIC, I_HOLE,
    I_SECRET, I_LIFE, I_WAIT, I_PLEASE, I_AGREE, I_DISAGREE, I_DONTCARE, I_DEMAND, I_CALLOUT, I_AGE,
    I_REAL, I_ADDRESS, I_SELFHARM, I_ALEX,
    I_POS, I_NEG,                       /* pure sentiment words: only used when nothing else matched */
    /* chunk 12: the 334 intents of docs/convo_plan.md Appendix A, in group order (game/lang_intents.h). they match nothing until their group's patterns exist */
#define X(id, name, label) I_##id,
    INTENT_LIST(X)
#undef X
    I_COUNT
} Intent;
#define I_OLD_COUNT (I_NEG + 1)         /* the 46 intents that existed before chunk 12 */

/* chunk 7: twenty moods. the first four keep 0-3 (until the mood vector moves in chunk 9, only a set vector, a lean or the plain mood gives them). a mood with no line for a key
 * answers through its fallback chain (lang_mood_chain), then the nearest of the old four, then any */
typedef enum {
    TONE_WARM, TONE_NEUTRAL, TONE_COLD, TONE_HOSTILE,
    TONE_PLAYFUL, TONE_SARCASTIC, TONE_WEARY, TONE_WORRIED, TONE_CURIOUS, TONE_SOLEMN,
    TONE_SMUG, TONE_TENDER, TONE_SULKY, TONE_SUSPICIOUS, TONE_AFRAID, TONE_HOLLOW,
    TONE_MANIC, TONE_SHY, TONE_BORED, TONE_EXCITED,
    TONE_COUNT
} Tone;
/* chunk 10: ten voices. Persona.style may combine them (STYLE_DIVINE | STYLE_FORMAL): a line made for any of them qualifies, and the x3 weight goes to
 * lines made only for voices the persona has that include its FIRST voice (the lowest bit). a voice with no line for a key and mood answers with
 * PLAIN lines, then with any. the bits are the masks PLAIN..POET of lang_data.h */
enum { STYLE_PLAIN = 1, STYLE_DIVINE = 2, STYLE_STREET = 4, STYLE_ELDER = 8, STYLE_CHILD = 16, STYLE_FORMAL = 32, STYLE_BEAST = 64,
       STYLE_MACHINE = 128, STYLE_GHOST = 256, STYLE_POET = 512, STYLE_ALL = 1023 };
#define STYLE_COUNT 10

typedef struct {
    const char *name;
    int style;          /* STYLE_*, or several or-ed together (the lowest bit is the first voice). 0 counts as PLAIN */
    int mood;           /* -100..100, how they feel about the player right now (changes in a chat) */
    int warmth;         /* 0..100, how kind they are by nature */
    int toxic;          /* 0..100, how much they like looking down on people */
    int patience;       /* replies before they get bored of a free chat */
    /* chunk 8: the mood state. every field is trailing and zero is "no push", so { "Dea", STYLE_DIVINE, 0, 30, 65, 8 } still compiles.
     * the vector says how the character feels right now (-100..100); a strong value overrides the plain warm..hostile mood (lang_mood_now) */
    int8_t trust, fear, energy, amuse, grief;
    uint32_t lean;      /* the moods this character drifts to: bits (1u << TONE_x), same bits as the masks in lang_data.h. 0 = none */
    uint8_t quirk;      /* Q_* bits of lang_persona.h (chunk 13): named there, the behaviour lands in later chunks */
    /* kept by the engine, leave at 0: a mood that sticks (held) for `hold` more turns */
    uint8_t hold;
    int8_t held;
    uint8_t quiet, flat, sour;      /* silent turns in a row, flattery turns in a row, turns left since the last insult (chunk 9) */
} Persona;
#define LEAN(t) (1u << (t))

#define HEARD_MAX 512   /* intents that fit in a Heard (a 512 bit set; chunk 12: 380 intents) */
#define HEARD_NUM 4     /* numbers kept in a Heard (chunk 19) */
/* chunk 27: where a heard intent came from: a PHRASE (a pattern, 85-100), the LEXICON (a word score, 35-69: weaker than any phrase), a sentence FRAME (chunk 30), a FUZZY reading (a spelled or lemma variant, 30-80) */
enum { SRC_NONE, SRC_PHRASE, SRC_LEX, SRC_FRAME, SRC_FUZZY };
#define HEARD_ORDER 24  /* intents remembered in the order they were said */
#define HEARD_CLAUSES 8     /* clauses kept in a Heard (chunk 25) */
/* chunk 25: one clause of the text as typed. a, b = bytes [a, b) of the CLEANED text (lower case, spaces folded: not the typed bytes) */
typedef struct {
    int16_t a, b;
    int8_t polarity;        /* -2..2: positive minus negative feeling words in the clause, after negation (not bad = +1) */
    uint8_t intensity;      /* 0 says nothing, 1 plain, 2 boosted (very, so, really ...), 3 strongly boosted; a bit / kinda lowers it; not bad stays at 1 */
    uint8_t contrast;       /* 1 = the clause opens with but / however / yet: it wins over what came before */
    uint8_t n;              /* intents listed (at most 4) */
    int16_t intent[4];
} HeardClause;
typedef struct {
    uint64_t m[HEARD_MAX / 64];   /* bit per Intent: use heard_has / heard_set / heard_only (the old `mask` name is gone since chunk 12) */
    int order[HEARD_ORDER], n;   /* intents in the order they were said */
    int positive, negative;     /* chunk 28: WEIGHTS 0-9 of the positive and the negative feeling in the text (valence lexicon, negation, boosters, but), no longer counts */
    int is_question, words;
    char capture[24];   /* "my name is X" -> X */
    char ask[12];       /* chunk 36: the ask tag of the npc line this text answers (from lang_ctx), empty = none; set by lang_parse */
    /* chunk 16: what the text carries besides words (see lang_clean.h). none of it changes which intents are heard yet */
    unsigned flags;     /* LF_SHOUT LF_NOISE LF_LONG LF_LAUGH LF_EMOTE */
    uint8_t noise;      /* 0-100: how much of the text is keyboard mash or gibberish */
    uint8_t excl, quest;   /* number of ! and ? */
    uint8_t laugh;      /* the longest laugh in letters (a long laugh is more amused), 0 = none */
    uint8_t tok[TK_COUNT];   /* tokens seen per kind: emoticons, emoji, laughs, censored words (a run of the same emoji counts once) */
    /* chunk 19: numbers (see lang_clean.h). none of it changes which intents are heard */
    uint8_t nnum;       /* numbers captured (at most HEARD_NUM kept) */
    int16_t age;        /* the age the player said (1-120), 0 = none */
    LangNum num[HEARD_NUM];
    uint8_t nspell, spell_conf;   /* chunk 21: words repaired by the spell step, and the lowest confidence among them (0-100; 0 when none) */
    uint8_t nclauses, intensity;   /* chunk 25: clauses[0..nclauses) and the overall intensity 0-3 (the last clause that has a feeling, else the strongest) */
    HeardClause clauses[HEARD_CLAUSES];
    uint8_t nlemma;     /* chunk 24: words of the text as typed that have a lemma (loved -> love); flags says LF_LEMMA. the lemma variant only adds intents */
    uint8_t src[HEARD_ORDER], conf[HEARD_ORDER];   /* chunk 27: for each intent of order[]: where it came from (SRC_*) and how sure the engine is (0-100) */
    uint8_t ntopic;     /* chunk 29: topics found (0-3), strongest first: topic[] are ids (lang_topic_name), topic_w[] their scores, thing the strongest topic's own word (moon) */
    int16_t topic[3]; uint8_t topic_w[3]; char thing[24];
    int8_t valence;     /* chunk 28: -100..100, the net feeling of the whole text (after negation, scaling words, CAPS / ! / sooo boosters, emoticons, a but shift and sarcasm) */
    uint8_t sarcasm;    /* chunk 28: 1 = a sarcasm cue fired (oh great, yeah right, wow thanks, great job + a negative word, sure. /s): the positive part was read as mocking */
    uint8_t qtype;      /* chunk 32: QT_* of the last clause of the text that asks something (QT_NONE = it asks nothing); focus = its last content word (moon), opt1 / opt2 = the two options of a QT_CHOICE ("tea or coffee?"); all lower case letters, typed by the player: echo only through lang_safe_echo */
    char focus[24], opt1[24], opt2[24];
    Frame frame;        /* chunk 30: the frame of the last clause of the text as typed that does something (act, subject, tense, modality, politeness, verb, object, question word); a variant text only when the typed text has none. act SA_NONE = no clause said anything */
    uint8_t ncap;       /* chunk 33: facts captured (0-6): cap[] of kind CAP_NAME / AGE / PLACE / LIKE / DISLIKE / THING / NUMBER / TIME / FEELING, text as typed (a name keeps its capitals, the rest is lower case). a blocked name is never captured: it is heard as I_NAME_SWEAR_NAME */
    Capture cap[HEARD_CAPS];
    uint8_t confidence; /* chunk 34: 0-100, how sure the engine is of the intent it will answer (that intent's conf; 30 when only a topic or a frame was heard; 0 = nothing) */
    uint8_t kind;       /* chunk 34: HK_* what the reading rests on */
    uint8_t rep_level;  /* chunk 38: with I_REPEAT_PLAYER / I_SPAM the step of the rep.1..5 ladder this turn is on, with I_COPYCAT the step of copycat.1..3 (0 = none) */
    uint8_t stretch;    /* chunk 17: the longest elongated run of one letter (sooooo = 5), 0 = none; flags says LF_STRETCH LF_LEET LF_SPACED LF_MASKED LF_PUNCT too */
} Heard;

/* chunk 34: what a reading rests on (Heard.kind) */
enum { HK_NONE, HK_INTENT, HK_WEAK, HK_TOPIC, HK_FRAME };   /* nothing / a real intent / only a catch-all intent (what, how, why ...) / a topic word only / a sentence frame only */
/* chunk 34: the reply plan, up to 3 steps. ANSWER = the top intent, ACK = a second intent (answered short), HEDGE = an opener that says the npc is not sure,
 * ECHO = reflect the topic back, DEFLECT = steer away in character (rhetorical questions), QUESTION = ask back, UNKNOWN = the unknown ladder */
enum { ROLE_ANSWER, ROLE_ACK, ROLE_HEDGE, ROLE_ECHO, ROLE_DEFLECT, ROLE_QUESTION, ROLE_UNKNOWN, ROLE_COUNT };
#define PLAN_MAX 3
typedef struct { int n; struct { uint8_t role; int16_t intent; char key[40]; } s[PLAN_MAX]; } ReplyPlan;   /* intent = -1 when the step is not about one intent */

static inline int  heard_has(const Heard *h, int i) { return (int)((h->m[i >> 6] >> (i & 63)) & 1u); }
static inline void heard_set(Heard *h, int i) { h->m[i >> 6] |= 1ull << (i & 63); }
/* 1 if every intent heard is in allowed[] (a list ended by -1) */
static inline int  heard_only(const Heard *h, const int *allowed) {
    for (int i = 0; i < HEARD_MAX; i++) {
        if (!heard_has(h, i)) continue;
        int ok = 0;
        for (const int *a = allowed; *a >= 0; a++) if (*a == i) { ok = 1; break; }
        if (!ok) return 0;
    }
    return 1;
}

/* ---- the intent catalogue (chunk 12) ---- */
_Static_assert(I_COUNT <= HEARD_MAX, "more intents than a Heard can hold: raise HEARD_MAX");
const char *lang_intent_name(int in);                       /* "greet", "greet_morning" ... NULL if out of range */
const char *lang_intent_label(int in);                      /* "HELLO", "GREET MORNING" ... */
int         lang_intent_by_name(const char *name);          /* -1 if unknown */
/* the reply key that answers an intent: "r.<name>", or the key KEYMAP maps its name to (merged near-twins). buf is filled and returned */
const char *lang_intent_key(int in, char *buf, int cap);
int         lang_keymap_count(void);                        /* rows of KEYMAP, and the i-th: name -> key (tools / lint) */
int         lang_keymap_row(int i, const char **name, const char **key);
/* tests only: a temporary mapping on top of KEYMAP (up to 8); lang_keymap_clear_test() forgets them */
int         lang_keymap_test(const char *name, const char *key);
void        lang_keymap_clear_test(void);

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
void lang_match_classic(const char *pat, const char *s, const char *base, const char **best);   /* chunk 26 (tests): the old wildcard matcher */
const Capture *lang_heard_capture(const Heard *h, int type, int nth);   /* chunk 33: the nth (0 = first) capture of a kind, NULL if none */
void lang_set_heard_slots(const Heard *h);       /* chunk 33: what {name} {age} {likes} {dislikes} {thing} {topic} turn into for lang_line; lang_reply calls it itself */
void lang_heard_label(const Heard *h, char *out, int cap);   /* "HEARD: HELLO + WHO ARE YOU" */

/* ---- answering ---- */
/* how they feel like talking right now: one of the 20 moods. a mood picked by the vector or by `lean` sticks for 2-4 turns (ps->hold);
 * otherwise it is the plain warm / neutral / cold / hostile of mood + charisma + warmth - toxic, a bit random */
Tone lang_mood_now(const Persona *ps);
void lang_apply(Persona *ps, const Heard *h);    /* moves mood + charisma for what was said; one call = one turn (the sticking mood counts them) */
void lang_idle_turn(Persona *ps);                /* a turn where the player said nothing the engine heard: the vector drifts back to 0, a long silence makes them BORED or WORRIED */
void lang_threat(Persona *ps);                   /* the player threatened them: AFRAID (gentle by nature) or HOSTILE (toxic). no intent calls this yet */
/* chunk 34: what lang_reply will do with this reading (pure: no random numbers, no state). returns the number of steps (1-3) */
int  lang_plan(const Persona *ps, const Heard *h, ReplyPlan *out);
const char *lang_role_name(int role);            /* "answer" "ack" "hedge" "echo" "deflect" "question" "unknown" */
const char *lang_kind_name(int kind);            /* "none" "intent" "weak" "topic" "frame" */
int  lang_reply(Persona *ps, const Heard *h, char *out, int cap);   /* applies, then answers. returns 1 if a real intent was answered */
const char *lang_reply_key(void);                 /* chunk 35: the reply key the last lang_reply answered with ("r.greet", "r.unknown_q" ...), "" before the first reply */
int  lang_line(const Persona *ps, const char *key, char *out, int cap);   /* any named line, formatted. 0 if none */
int  lang_line_tone(const Persona *ps, const char *key, Tone t, char *out, int cap);
/* moods (chunk 7): the order of moods tried for a line of mood t: its chain, then the nearest of the old four. returns how many (no repeats) */
int  lang_mood_chain(int t, int *out, int cap);
const char *lang_tone_name(int t);               /* "warm" ... "excited" */
char lang_tone_code(int t);                      /* the letter code of docs/convo_plan.md: W N C H P S E O Q L M T K U A Z X Y B V */
/* which mood's lines answer for (key, mood t, style mask or 0 = any): t itself, or the one reached through the fallbacks; -1 if the key has nothing */
int  lang_mood_resolve(const char *key, int t, int style);


/* chunk 36: ask tags. a line the npc speaks may carry an ask tag (the Line's own, or one by rule: a question that starts with do / did / is / are /
 * will / can ... is `yn`, one with ` or ` is `choice`, `your name` / `call you` is `name`, one that starts with why is `why`, one that starts
 * with any / anything is `any`). lang_reply and lang_line remember the tag of the last question they said; lang_ctx keeps it for the next turn
 * and lang_parse reads a short answer (yes / no / a name / because ... / one of two options) relative to it. no tag = the old behaviour */
int  lang_ask_tag_valid(const char *tag);                 /* 1 for yn name why any choice more ready agree */
const char *lang_ask_tag_text(const char *text);          /* the tag a line text gets by rule ("" none); text may hold {slots} */
const char *lang_reply_ask(void);                         /* the tag of the last question the last lang_reply / lang_line said ("" none) */
const char *lang_reply_text(void);                        /* chunk 37: the last reply lang_reply said, whole ("" for self-harm) */
const char *lang_reply_ask_text(void);                    /* that question sentence as spoken, at most 95 bytes */
void lang_ask_sentence(const char *text, char *out, int cap);   /* the last question sentence of a spoken text ("" if it asks nothing) */
#endif
