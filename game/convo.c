/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "convo.h"
#include "dialog.h"
#include "lang.h"
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

static void finish(void) {
    void (*cb)(void) = C.on_end;
    C.on_end = NULL; C.running = 0;
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
        else dialog_suggest(SUG_FREE[C.turns % 3], 5);
    } else { dialog_on_reply(NULL); dialog_suggest(NULL, 0); }
    dialog_play(pair, n, finish);
}

static void say_key(const char *key, int reply, const char *echo) {
    char buf[480];
    if (!lang_line(C.mind, key, buf, sizeof buf)) snprintf(buf, sizeof buf, "...");
    speak(buf, reply, echo);
}

static int only(const Heard *h, uint64_t allowed) { return (h->mask & ~allowed) == 0; }
#define M(i) (1ull << (i))

static void on_reply(int page, const char *text) {
    (void)page;
    Heard h; memset(&h, 0, sizeof h);
    if (text) lang_parse(text, &h);
    Persona *ps = C.mind;

    if (C.stage == ST_SORRY) {                              /* the npc demanded an apology: no skipping here */
        lang_apply(ps, &h);
        if (h.mask & M(I_SORRY)) { ps->mood += 22; if (ps->mood > 100) ps->mood = 100; C.stage = ST_FREE; say_key("accept.sorry", REPLY_OPTIONAL, text); return; }
        if (++C.demands >= 3) { say_key("end.angry", REPLY_NONE, text); return; }
        say_key("demand.again", REPLY_REQUIRED, text);
        return;
    }

    if (!text) {                                            /* SKIP: the player says nothing */
        if (C.stage == ST_QUESTIONS) say_key("end.noquestions", REPLY_NONE, NULL);
        else say_key("end.silence", REPLY_NONE, NULL);
        return;
    }

    if (C.stage == ST_QUESTIONS) {
        uint64_t polite = M(I_NO) | M(I_OK) | M(I_THANKS) | M(I_PLEASE) | M(I_POS) | M(I_ADDRESS);
        uint64_t yes = M(I_YES) | M(I_OK) | M(I_PLEASE) | M(I_POS) | M(I_ADDRESS) | M(I_HELP);
        if ((h.mask & M(I_NO)) && only(&h, polite)) { lang_apply(ps, &h); say_key("end.noquestions", REPLY_NONE, text); return; }
        if ((h.mask & M(I_YES)) && only(&h, yes))   { lang_apply(ps, &h); C.stage = ST_FREE; C.turns = 0; say_key("ask.go", REPLY_OPTIONAL, text); return; }
        C.stage = ST_FREE;                                  /* they just asked something: answer it */
    }

    char reply[480];
    lang_reply(ps, &h, reply, sizeof reply);
    C.turns++;

    if (h.mask & M(I_SELFHARM)) { speak(reply, REPLY_OPTIONAL, text); return; }
    if (h.mask & M(I_BYE)) {                                /* the player is leaving: wave and end */
        speak(reply, REPLY_NONE, text); return;
    }
    if ((h.mask & M(I_INSULT)) && ps->mood <= -55) {        /* too far: demand an apology, and the player MUST talk */
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
int  convo_active(void) { return C.running; }

static void begin(const Person *who, Persona *mind, Stage st, void (*on_end)(void)) {
    C.who = who; C.mind = mind; C.stage = st; C.turns = 0; C.demands = 0; C.on_end = on_end; C.running = 1;
    lang_set_player(C.player ? C.player->name : "Vas");
}

void convo_open(const Person *who, Persona *mind, int again, void (*on_end)(void)) {
    begin(who, mind, ST_FREE, on_end);
    say_key(again ? "open.again" : "open", REPLY_OPTIONAL, NULL);
}

void convo_ask_questions(const Person *who, Persona *mind, void (*on_end)(void)) {
    begin(who, mind, ST_QUESTIONS, on_end);
    say_key("ask.questions", REPLY_OPTIONAL, NULL);
}
