/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "convo.h"
#include "dialog.h"
#include "lang.h"
#include "thought.h"
#include <stdio.h>
#include <string.h>

typedef enum { ST_FREE, ST_QUESTIONS, ST_SORRY } Stage;

static struct {
    const Person *who, *player;
    Persona *mind;
    Stage stage;
    int turns, demands, running;
    void (*on_end)(void);
} C;

static char npc_buf[480], me_buf[100];
static DialogLine pair[2];

/* what the keyboard suggests, by situation. the words are chosen so most of them light up green */
static const char *const SUG_QUESTIONS[] = { "No", "Yes" };
static const char *const SUG_SORRY[]     = { "Sorry", "I am sorry" };
static const char *const SUG_FREE[3][5]  = {
    { "Hello", "Who are you?", "Where am I?", "What should I do?", "Bye" },
    { "How are you?", "Why?", "I am scared", "Thank you", "Bye" },
    { "Tell me a joke", "You are nice", "Will I die?", "What is the hole?", "Bye" },
};

static struct { int (*fn)(const char *, char *, int); const char *suggest; } W;     /* chunk 63: the watch on this chat */
void convo_watch(int (*fn)(const char *, char *, int), const char *suggest) { W.fn = fn; W.suggest = suggest; }

static void finish(void) {
    void (*cb)(void) = C.on_end;
    C.on_end = NULL; C.running = 0;
    W.fn = NULL; W.suggest = NULL;
    if (cb) cb();
}

static void on_reply(int page, const char *text);

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
        if (C.stage == ST_QUESTIONS) dialog_suggest(SUG_QUESTIONS, 2);
        else if (C.stage == ST_SORRY) dialog_suggest(SUG_SORRY, 2);
        else if (W.suggest) {                               /* chunk 63: the story's own suggestion comes first */
            static const char *sug[6]; sug[0] = W.suggest;
            for (int i = 0; i < 5; i++) sug[i + 1] = SUG_FREE[C.turns % 3][i];
            dialog_suggest(sug, 6);
        } else dialog_suggest(SUG_FREE[C.turns % 3], 5);
    } else { dialog_on_reply(NULL); dialog_suggest(NULL, 0); }
    dialog_play(pair, n, finish);
}

static void say_key(const char *key, int reply, const char *echo) {
    char buf[480];
    if (!lang_line(C.mind, key, buf, sizeof buf)) snprintf(buf, sizeof buf, "...");
    speak(buf, reply, echo);
}


static void on_reply(int page, const char *text) {
    (void)page;
    Heard h; memset(&h, 0, sizeof h);
    if (!text) text = "Bye";                                /* the BYE button: the player says goodbye, and the npc answers that */
    else if (W.fn && C.stage != ST_SORRY) {                 /* chunk 63: the story may answer this line itself (Alex and the orb) */
        char own[480];
        if (W.fn(text, own, sizeof own)) { C.stage = ST_FREE; C.turns++; speak(own, REPLY_OPTIONAL, text); return; }
    }
    lang_parse(text, &h);
    Persona *ps = C.mind;

    if (C.stage == ST_SORRY) {                              /* the npc demanded an apology: no skipping here */
        lang_apply(ps, &h);
        if (heard_has(&h, I_SORRY)) { ps->mood += 22; if (ps->mood > 100) ps->mood = 100; C.stage = ST_FREE; say_key("accept.sorry", REPLY_OPTIONAL, text); return; }
        if (++C.demands >= 3) { say_key("end.angry", REPLY_NONE, text); return; }
        say_key("demand.again", REPLY_REQUIRED, text);
        return;
    }

    if (C.stage == ST_QUESTIONS) {
        static const int polite[] = { I_NO, I_OK, I_THANKS, I_PLEASE, I_POS, I_ADDRESS, -1 };
        static const int yes[] = { I_YES, I_OK, I_PLEASE, I_POS, I_ADDRESS, I_HELP, -1 };
        if (heard_has(&h, I_NO) && heard_only(&h, polite)) { lang_apply(ps, &h); say_key("end.noquestions", REPLY_NONE, text); return; }
        if (heard_has(&h, I_YES) && heard_only(&h, yes)) { lang_apply(ps, &h); C.stage = ST_FREE; C.turns = 0; say_key("ask.go", REPLY_OPTIONAL, text); return; }
        C.stage = ST_FREE;                                  /* they just asked something: answer it */
    }

    char reply[480];
    lang_reply(ps, &h, reply, sizeof reply);
    C.turns++;

    if (heard_has(&h, I_SELFHARM)) { speak(reply, REPLY_OPTIONAL, text); return; }
    if (heard_has(&h, I_BYE)) {                                /* the player is leaving: wave and end */
        speak(reply, REPLY_NONE, text); return;
    }
    if (heard_has(&h, I_INSULT) && ps->mood <= -55) {        /* too far: demand an apology, and the player MUST talk */
        char dem[160]; lang_line(ps, "demand.sorry", dem, sizeof dem);
        size_t n = strlen(reply); if (n + strlen(dem) + 2 < sizeof reply) { strcat(reply, " "); strcat(reply, dem); }
        C.stage = ST_SORRY; C.demands = 0;
        speak(reply, REPLY_REQUIRED, text); return;
    }
    if (C.turns >= ps->patience) {                          /* the npc has had enough */
        char end[200]; lang_line(ps, "end.bored", end, sizeof end);
        size_t n = strlen(reply); if (n + strlen(end) + 2 < sizeof reply) { strcat(reply, " "); strcat(reply, end); }
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
    C.who = who; C.mind = mind; C.stage = st; C.turns = 0; C.demands = 0; C.on_end = on_end; C.running = 1;
    lang_set_player(C.player ? C.player->name : "Vas");
}

void convo_open(const Person *who, Persona *mind, int again, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_FREE, on_end);
    say_key(again ? "open.again" : "open", REPLY_OPTIONAL, NULL);
}

void convo_open_line(const Person *who, Persona *mind, const char *line, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_FREE, on_end);
    speak(line, REPLY_OPTIONAL, NULL);
}

void convo_ask_line(const Person *who, Persona *mind, const char *line, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_QUESTIONS, on_end);
    speak(line, REPLY_OPTIONAL, NULL);
}

void convo_ask_questions(const Person *who, Persona *mind, void (*on_end)(void)) {
    if (locked_chat(who, on_end)) return;
    begin(who, mind, ST_QUESTIONS, on_end);
    say_key("ask.questions", REPLY_OPTIONAL, NULL);
}
