/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "convo.h"
#include "dialog.h"
#include "lang.h"
#include "lang_ctx.h"
#include "lang_pat.h"
#include "thought.h"
#include <stdio.h>
#include <string.h>

#define MAX_TEXT_UNDERSTOOD 400                             /* chunk 45: the engine's MAX_TEXT */

typedef enum { ST_FREE, ST_QUESTIONS, ST_SORRY } Stage;

static struct {
    const Person *who, *player;
    Persona *mind;
    Stage stage;
    int turns, demands, running, after;           /* after: 0 nothing special, 1 the player just insulted, 2 the npc just refused (chunk 46: picks the chips) */
    void (*on_end)(void);
} C;

static char npc_buf[480], me_buf[100];
static DialogLine pair[2];

/* what the keyboard suggests, by situation (chunk 46: 12 lists of 6). the words are chosen so they all light up green (convotest checks each one) */
enum { CL_GREET, CL_QUESTIONS, CL_SORRY, CL_INSULT, CL_REFUSED, CL_ASKED, CL_WARM, CL_NEUTRAL, CL_COLD, CL_HOSTILE, CL_DARK, CL_PLAYFUL };
static const char *const CL_NAME[CONVO_CHIP_LISTS] = { "greeting", "questions", "sorry", "after an insult", "after a refusal", "answering a question",
    "free: warm", "free: neutral", "free: cold", "free: hostile", "free: dark", "free: playful" };
static const char *const CHIPS[CONVO_CHIP_LISTS][6] = {
    { "Hello", "Who are you?", "Where am I?", "What should I do?", "How are you?", "Bye" },
    { "Yes", "No", "Yes please", "No thanks", "Sure", "Not now" },
    { "Sorry", "I am sorry", "Forgive me", "Please", "My bad", "Bye" },
    { "Sorry", "I am sorry", "Calm down", "Please", "Forgive me", "Bye" },
    { "Please", "Why?", "Okay", "Fine", "I understand", "Bye" },
    { "Yes", "No", "Okay", "Please", "Why?", "Bye" },
    { "How are you?", "Thank you", "I like you", "Tell me a joke", "What is the hole?", "Bye" },
    { "Who are you?", "Where am I?", "What should I do?", "Why?", "Thank you", "Bye" },
    { "Why?", "What do you want?", "Help me", "Please", "I am scared", "Bye" },
    { "Calm down", "Sorry", "I am scared", "Please", "Why?", "Bye" },
    { "Are you okay?", "I am scared", "Will I die?", "Help me", "What is the hole?", "Bye" },
    { "Tell me a joke", "Haha", "How are you?", "Thank you", "What is the hole?", "Bye" },
};
const char *const *convo_chip_list(int i, int *n, const char **name) {
    if (i < 0 || i >= CONVO_CHIP_LISTS) { if (n) *n = 0; if (name) *name = ""; return NULL; }
    if (n) *n = 6; if (name) *name = CL_NAME[i];
    return CHIPS[i];
}

static struct { int (*fn)(const char *, char *, int); int (*fn2)(const char *, const char *, char *, int); const char *suggest; } W;     /* chunk 63: the watch on this chat (chunk 45: fn2 also gets the understood text) */
void convo_watch(int (*fn)(const char *, char *, int), const char *suggest) { W.fn = fn; W.fn2 = NULL; W.suggest = suggest; }
void convo_watch_text(int (*fn)(const char *, const char *, char *, int), const char *suggest) { W.fn = NULL; W.fn2 = fn; W.suggest = suggest; }

static void finish(void) {
    void (*cb)(void) = C.on_end;
    C.on_end = NULL; C.running = 0;
    W.fn = NULL; W.fn2 = NULL; W.suggest = NULL;
    if (cb) cb();
}

static void on_reply(int page, const char *text);

/* chunk 46: which of the 12 lists the keyboard shows now */
static int chip_pick(void) {
    if (C.stage == ST_QUESTIONS) return CL_QUESTIONS;
    if (C.stage == ST_SORRY) return CL_SORRY;
    if (C.after == 1) return CL_INSULT;
    if (C.after == 2) return CL_REFUSED;
    if (C.turns == 0) return CL_GREET;
    if (*lang_reply_ask()) return CL_ASKED;
    switch (C.mind ? lang_reply_tone() : TONE_NEUTRAL) {
    case TONE_WARM: case TONE_TENDER: case TONE_SHY: case TONE_EXCITED: return CL_WARM;
    case TONE_PLAYFUL: case TONE_CURIOUS: case TONE_MANIC: return CL_PLAYFUL;
    case TONE_COLD: case TONE_SARCASTIC: case TONE_SMUG: case TONE_SULKY: case TONE_SUSPICIOUS: return CL_COLD;
    case TONE_HOSTILE: return CL_HOSTILE;
    case TONE_AFRAID: case TONE_HOLLOW: case TONE_WEARY: case TONE_WORRIED: case TONE_BORED: case TONE_SOLEMN: return CL_DARK;
    default: return CL_NEUTRAL;
    }
}

/* the npc speaks. reply: REPLY_NONE ends the chat after this line, otherwise the player may answer.
 * echo: the player's own line, shown first as the player (NULL for none) */
static void speak(const char *line, int reply, const char *echo) {
    int n = 0;
    snprintf(npc_buf, sizeof npc_buf, "%s", line);
    if (echo) { snprintf(me_buf, sizeof me_buf, "%s", echo); pair[n++] = (DialogLine){ C.player, me_buf, REPLY_NONE }; }
    pair[n++] = (DialogLine){ C.who, npc_buf, reply };
    dialog_on_page(NULL);
    if (reply) {
        dialog_on_reply(on_reply);
        const char *const *ch = CHIPS[chip_pick()];
        if (W.suggest && C.stage == ST_FREE) {              /* chunk 63: the story's own suggestion comes first */
            static const char *sug[7]; sug[0] = W.suggest;
            for (int i = 0; i < 6; i++) sug[i + 1] = ch[i];
            dialog_suggest(sug, 7);
        } else dialog_suggest(ch, 6);
    } else { dialog_on_reply(NULL); dialog_suggest(NULL, 0); }
    dialog_play(pair, n, finish);
}

static void say_key(const char *key, int reply, const char *echo) {
    char buf[480];
    if (!lang_line(C.mind, key, buf, sizeof buf)) snprintf(buf, sizeof buf, "...");
    lang_ctx_set_ask(lang_reply_ask(), lang_reply_ask_text());      /* chunk 36: a scripted question (ask.questions) is read like any other: its tag decides what a bare yes means */
    speak(buf, reply, echo);
}


static void on_reply(int page, const char *text) {
    (void)page;
    Heard h; memset(&h, 0, sizeof h);
    if (!text) text = "Bye";                                /* the BYE button: the player says goodbye, and the npc answers that */
    else if ((W.fn || W.fn2) && C.stage != ST_SORRY) {      /* chunk 63: the story may answer this line itself (Alex and the orb) */
        char own[480], und[MAX_TEXT_UNDERSTOOD];
        int hit;
        if (W.fn2) { lang_understood(text, und, sizeof und); hit = W.fn2(text, und, own, sizeof own); }      /* chunk 45: raw text + what the game understood */
        else hit = W.fn(text, own, sizeof own);
        if (hit) { C.stage = ST_FREE; C.after = 2; C.turns++; if (C.mind) lang_idle_turn(C.mind); lang_ctx_set_ask_line(own); speak(own, REPLY_OPTIONAL, text); return; }
    }
    lang_parse(text, &h);
    Persona *ps = C.mind;

    if (C.stage == ST_SORRY) {                              /* the npc demanded an apology: no skipping here */
        lang_apply(ps, &h);
        if (heard_has(&h, I_SORRY)) { ps->mood += 22; if (ps->mood > 100) ps->mood = 100; C.stage = ST_FREE; C.after = 0; say_key("accept.sorry", REPLY_OPTIONAL, text); return; }
        if (++C.demands >= 3) { say_key("end.angry", REPLY_NONE, text); return; }
        say_key("demand.again", REPLY_REQUIRED, text);
        return;
    }

    if (C.stage == ST_QUESTIONS) {
        static const int polite[] = { I_NO, I_OK, I_THANKS, I_PLEASE, I_POS, I_ADDRESS, I_DECLINE_TO_ASK, -1 };      /* chunk 36: the ask tag of the question adds the DECLINE / AGREE intents next to the old ones */
        static const int yes[] = { I_YES, I_OK, I_PLEASE, I_POS, I_ADDRESS, I_HELP, I_AGREE_TO_ASK, -1 };
        if ((heard_has(&h, I_NO) || heard_has(&h, I_DECLINE_TO_ASK)) && heard_only(&h, polite)) { lang_apply(ps, &h); say_key("end.noquestions", REPLY_NONE, text); return; }
        if ((heard_has(&h, I_YES) || heard_has(&h, I_AGREE_TO_ASK)) && heard_only(&h, yes)) { lang_apply(ps, &h); C.stage = ST_FREE; C.turns = 0; say_key("ask.go", REPLY_OPTIONAL, text); return; }
        C.stage = ST_FREE;                                  /* they just asked something: answer it */
    }

    char reply[480];
    lang_reply(ps, &h, reply, sizeof reply);
    lang_ctx_note(&h, text, lang_reply_key(), ps->mood);      /* chunk 35 */
    C.turns++;
    C.after = heard_has(&h, I_INSULT) ? 1 : (!strncmp(lang_reply_key(), "dk.", 3) || heard_has(&h, I_DECLINE_TO_ASK)) ? 2 : 0;      /* chunk 46: what the chips answer to */
    int worn = (heard_has(&h, I_REPEAT_PLAYER) || heard_has(&h, I_SPAM));
    if (worn) C.turns++;                                    /* chunk 38: the same line again and spam tire the npc twice as fast */

    if (heard_has(&h, I_SELFHARM)) { speak(reply, REPLY_OPTIONAL, text); return; }
    if (!strcmp(lang_reply_key(), "end.silence")) { speak(reply, REPLY_NONE, text); return; }      /* chunk 41: four nudges and still only dots: the npc gives up and the chat ends */
    if (heard_has(&h, I_BYE)) {                                /* the player is leaving: wave and end */
        speak(reply, REPLY_NONE, text); return;
    }
    if (heard_has(&h, I_INSULT) && ps->mood <= -55) {        /* too far: demand an apology, and the player MUST talk */
        char dem[160]; lang_line(ps, "demand.sorry", dem, sizeof dem);
        lang_append_flow(reply, sizeof reply, dem, 1);      /* chunk 44: inside the phone cap (an apology demand always stands) */
        C.stage = ST_SORRY; C.demands = 0;
        speak(reply, REPLY_REQUIRED, text); return;
    }
    if (C.turns >= ps->patience || (worn && h.rep_level >= 5)) {      /* the npc has had enough (chunk 38: rep.5 is the npc giving up) */
        char end[200]; lang_line(ps, "end.bored", end, sizeof end);
        lang_append_flow(reply, sizeof reply, end, 1);      /* chunk 44: inside the phone cap */
        speak(reply, REPLY_NONE, text); return;
    }
    speak(reply, REPLY_OPTIONAL, text);
}

void convo_set_player(const Person *p) { C.player = p; }
const Person *convo_player(void) { return C.player; }
int  convo_active(void) { return C.running; }

/* the chat lock (chunk 27) */
static struct { int on; char thought[120]; const Person *reply_who, *allow; char reply[120]; DialogLine line[1]; } L;
void convo_lock(const char *thought) { L.on = 1; snprintf(L.thought, sizeof L.thought, "%s", thought ? thought : ""); }
void convo_lock_reply(const Person *who, const char *line) { L.reply_who = who; snprintf(L.reply, sizeof L.reply, "%s", line ? line : ""); }
void convo_lock_allow(const Person *who) { L.allow = who; }
void convo_unlock(void) { memset(&L, 0, sizeof L); }
int  convo_locked(void) { return L.on; }

/* a locked chat: no window. the one the vessel talks to answers with their own line if they have one, otherwise the vessel thinks why
 * it cannot speak. either way the story goes on (on_end). returns 1 when the call was swallowed */
static int locked_chat(const Person *who, void (*on_end)(void)) {
    if (!L.on) return 0;
    if (who && who == L.allow) return 0;                    /* chunk 63: this one can still be talked to */
    if (who && who == L.reply_who && *L.reply) {
        L.line[0].who = who; L.line[0].text = L.reply; L.line[0].reply = REPLY_NONE;
        dialog_play(L.line, 1, on_end);
        return 1;
    }
    if (*L.thought) thought_say(L.thought, 0);
    if (on_end) on_end();
    return 1;
}

static void begin(const Person *who, Persona *mind, Stage st, void (*on_end)(void)) {
    C.who = who; C.mind = mind; C.stage = st; C.turns = 0; C.demands = 0; C.after = 0; C.on_end = on_end; C.running = 1;
    lang_set_player(C.player ? C.player->name : "Vas");
    lpat_set_npc(mind ? mind->name : NULL);      /* chunk 45: <npc> in a pattern matches the name of who is talking */
    lang_ctx_reset();      /* chunk 35: a new chat, a clean memory (the facts that outlive a chat stay) */
}

void convo_open(const Person *who, Persona *mind, int again, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_FREE, on_end);
    say_key(again ? "open.again" : "open", REPLY_OPTIONAL, NULL);
}

void convo_open_line(const Person *who, Persona *mind, const char *line, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_FREE, on_end);
    lang_ctx_set_ask_line(line);      /* chunk 36 */
    speak(line, REPLY_OPTIONAL, NULL);
}

void convo_ask_line(const Person *who, Persona *mind, const char *line, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_QUESTIONS, on_end);
    lang_ctx_set_ask_line(line);      /* chunk 36 */
    speak(line, REPLY_OPTIONAL, NULL);
}

void convo_ask_questions(const Person *who, Persona *mind, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_QUESTIONS, on_end);
    say_key("ask.questions", REPLY_OPTIONAL, NULL);
}
