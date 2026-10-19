/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * drives whole conversations through dialog + talk + convo with fake touches (no screen needed).
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/convotest tools/convotest.c $(ls game/*.c | grep -v game/game.c) $(sdl2-config --libs) -lm && build/convotest */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include "dialog.h"
#include "talk.h"
#include "convo.h"
#include "story.h"

#define W 540
#define H 1170
static int ended, fails; static Persona *mind_ref;
static void on_end(void) { ended++; }
static void settle(float s) { for (float t = 0; t < s; t += 0.016f) dialog_update(0.016f); }
static void tap(int x, int y) { dialog_touch(0, x, y); dialog_touch(1, x, y); }
static void sweep(int y0, int y1, int x0, int x1) { for (int y = y0; y > y1 && !talk_active(); y -= 4) for (int x = x0; x > x1 && !talk_active(); x -= 4) tap(x, y); }
static void tap_page(void) { for (int y = H - 40; y > H - 200; y -= 20) tap(W / 2, y); }   /* advance a plain page */
static void say(const char *s) {
    settle(6.0f);
    sweep(H - 30, H - 120, W - 30, W - 120);
    if (!talk_active()) { printf("  !! could not open TALK\n"); fails++; return; }
    talk_debug_type(s);
    settle(0.3f);
    talk_debug_send();

    printf("  said \"%s\" -> overlay closed: %d (mood %d)\n", s, !talk_active(), mind_ref ? mind_ref->mood : 0);
    if (talk_active()) fails++;
}
/* chunk 45: a watch that gets the raw text AND what the game understood (the way story.c's orb talk will use it) */
static int watch_calls, watch_hits, old_calls; static char last_raw[200], last_und[200];
static int orb_watch(const char *raw, const char *und, char *reply, int cap) {
    static const char *const ORB[] = { "orb", "sphere", "ball", "glow", NULL };
    watch_calls++; snprintf(last_raw, sizeof last_raw, "%s", raw); snprintf(last_und, sizeof last_und, "%s", und);
    if (!lang_has_word(und, ORB)) return 0;
    watch_hits++; snprintf(reply, (size_t)cap, "No orb for you."); return 1;
}
static int old_watch(const char *raw, char *reply, int cap) { (void)raw; (void)reply; (void)cap; old_calls++; return 0; }
static void skip(void) { settle(6.0f); tap(W - 140, H - 40); tap(W - 150, H - 45); tap(W - 160, H - 50); }

int main(void) {
    extern const Person DEA, VESSEL;
    static Persona mind = { "Dea", STYLE_DIVINE, 0, 30, 65, 8 };
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_Init(SDL_INIT_VIDEO);
    mind_ref = &mind; dialog_init(W, H); convo_set_player(&VESSEL); lang_seed(99);

    printf("1) questions -> no\n");
    ended = 0; convo_ask_questions(&DEA, &mind, on_end);
    say("No");
    for (int i = 0; i < 4; i++) { settle(6.0f); tap_page(); }
    printf("  chat ended: %d, dialog active: %d\n", ended, dialog_active()); if (ended != 1 || dialog_active()) fails++;

    printf("2) questions -> yes -> ask -> insult -> must apologize -> sorry -> skip\n");
    mind.mood = 0; ended = 0; convo_ask_questions(&DEA, &mind, on_end);
    say("yes");
    for (int i = 0; i < 2; i++) { settle(6.0f); tap_page(); }
    say("who are you?");
    for (int i = 0; i < 2; i++) { settle(6.0f); tap_page(); }
    for (int k = 0; k < 4; k++) { say(k % 2 ? "shut up you idiot" : "you stupid idiot"); for (int i = 0; i < 2; i++) { settle(6.0f); tap_page(); } }
    printf("  mood now %d (after insults)\n", mind.mood);
    skip();                                  /* a required page has no SKIP: this must not end the chat */
    printf("  skip on a required page ignored: dialog active %d\n", dialog_active()); if (!dialog_active()) fails++;
    say("sorry"); for (int i = 0; i < 2; i++) { settle(6.0f); tap_page(); }
    skip(); settle(6.0f); for (int i = 0; i < 3; i++) { tap_page(); settle(6.0f); }
    printf("  chat ended: %d, dialog active: %d\n", ended, dialog_active()); if (ended != 1) fails++;

    printf("3) free chat -> BYE button: the npc answers a goodbye, then the chat ends\n");
    mind.mood = 0; ended = 0; convo_open(&DEA, &mind, 0, on_end);
    skip();
    printf("  after BYE the npc still answers: dialog active %d, ended %d\n", dialog_active(), ended); if (!dialog_active() || ended) fails++;
    for (int i = 0; i < 4; i++) { settle(6.0f); tap_page(); }
    printf("  chat ended: %d, dialog active: %d\n", ended, dialog_active()); if (ended != 1 || dialog_active()) fails++;

    printf("4) typing is twice as fast as it was (56 cps) and a finger on the screen changes nothing (chunk 46)\n");
    {
        static char longtxt[300]; memset(longtxt, 'a', sizeof longtxt - 1);
        for (int i = 4; i < (int)sizeof longtxt - 1; i += 5) longtxt[i] = ' ';        /* 299 letters: ~5.3 s at 56 cps (it was ~10.7 s at 28) */
        static DialogLine one[1]; one[0] = (DialogLine){ &DEA, longtxt, REPLY_NONE };
        dialog_play(one, 1, NULL); settle(3.0f); tap_page();
        printf("  no finger, 3 s: page still typing (dialog active %d)\n", dialog_active()); if (!dialog_active()) fails++;
        settle(3.0f); tap_page();                                                      /* 6 s: done, so this ends it (twice the old speed) */
        printf("  no finger, 6 s: page was done and ended (dialog active %d)\n", dialog_active()); if (dialog_active()) fails++;
        dialog_play(one, 1, NULL);
        dialog_touch(0, 10, 10);                                                       /* finger down and held */
        settle(3.0f);
        dialog_touch(1, 10, 10);                                                       /* lifted: only advances if typing was done */
        printf("  finger held, 3 s: page still typing, a hold does not speed it up (dialog active %d)\n", dialog_active()); if (!dialog_active()) fails++;
        settle(3.0f); tap_page();
    }

    printf("5) a watch with the understood text (chunk 45): typos, glued words and plurals hit, absorb and orbit do not; the chat goes on\n");
    mind.mood = 0; ended = 0; watch_calls = watch_hits = old_calls = 0;
    convo_watch_text(orb_watch, "Give me the orb"); convo_open(&DEA, &mind, 0, on_end);
    { static const char *const T[] = { "give me the obr", "gimme the orbs", "giveme the sphere", "i absorb the orbit", "hello" };
      static const int want[] = { 1, 1, 1, 0, 0 };
      for (int i = 0; i < 5; i++) {
          int before = watch_hits;
          say(T[i]); for (int k = 0; k < 2; k++) { settle(6.0f); tap_page(); }
          printf("  watch saw raw '%s' understood '%s' -> hit %d\n", last_raw, last_und, watch_hits - before);
          if (strcmp(last_raw, T[i])) { printf("  !! the watch got a different raw text\n"); fails++; }
          if (watch_hits - before != want[i]) { printf("  !! wanted hit %d\n", want[i]); fails++; }
      }
      if (strcmp(last_und, "hello")) { printf("  !! understood text of hello was '%s'\n", last_und); fails++; } }
    say("bye"); for (int i = 0; i < 4; i++) { settle(6.0f); tap_page(); }
    printf("  watch calls %d, chat ended: %d\n", watch_calls, ended); if (watch_calls != 6 || ended != 1) fails++;
    ended = 0; convo_open(&DEA, &mind, 0, on_end); watch_calls = 0;      /* a new chat has no watch: the old one was forgotten at the end */
    say("give me the orb"); for (int i = 0; i < 2; i++) { settle(6.0f); tap_page(); }
    printf("  next chat, no watch: calls %d\n", watch_calls); if (watch_calls) fails++;
    say("bye"); for (int i = 0; i < 4; i++) { settle(6.0f); tap_page(); }
    ended = 0; convo_watch(old_watch, NULL); convo_open(&DEA, &mind, 0, on_end);      /* the old watch still works */
    say("hello"); for (int i = 0; i < 2; i++) { settle(6.0f); tap_page(); }
    printf("  old convo_watch calls %d\n", old_calls); if (old_calls != 1) fails++;
    say("bye"); for (int i = 0; i < 4; i++) { settle(6.0f); tap_page(); }

    printf("%s\n", fails ? "FAILED" : "all conversation flows ok");
    return fails;
}
