/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the story, restarted. the first story is archived in trash/story_old.c. this keeps the interface of story.h (game.c and world.c call it)
 * and the people the rest of the game draws, and does the bare minimum: the soul's opening line in limbo, then the world, a dark temple
 * (gen.c builds it), with nothing to do in it yet. */
#include "story.h"
#include "dialog.h"
#include "thought.h"
#include "world.h"
#include "jukebox.h"
#include "vessel_gen.h"
#include <string.h>
#include <stdlib.h>

#define VESSEL_NAME  "Aonia"
#define VESSEL2_NAME "Doia"
#define VESSEL3_NAME "Tria"
#define VESSEL_LATIN "Vas"

/*                      name           hair            skin             shirt           pants          boots        long hair  accessories */
const Person SOUL = { "Soul", { 232, 240, 255 }, { 188, 226, 246 }, { 140, 186, 238 }, { 118, 160, 218 }, { 96, 130, 196 }, 0, ACC_HALO | ACC_WORRIED };
const Person VESSEL = { VESSEL_NAME,  { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
const Person VESSEL2 = { VESSEL2_NAME, { 206, 210, 228 }, { 226, 172, 132 }, { 70, 160, 96 }, { 78, 78, 92 }, { 40, 30, 30 }, 1, 0 };
const Person VESSEL3 = { VESSEL3_NAME, { 214, 108, 40 }, { 150, 100, 72 }, { 126, 84, 190 }, { 40, 44, 56 }, { 60, 40, 30 }, 0, 0 };
const Person VAS    = { VESSEL_LATIN, { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
const Person ALEX   = { "Alex",       { 236, 196, 84 }, { 244, 200, 164 }, { 60, 170, 170 }, { 70, 60, 110 }, { 40, 30, 30 }, 1, 0 };
const Person DEA    = { "Dea",        { 252, 240, 190 }, { 252, 228, 206 }, { 255, 244, 200 }, { 250, 232, 170 }, { 226, 190, 90 }, 1,
                        ACC_SHADES | ACC_HALO };

/* ---------- the endless line of vessels ---------- */
/* how many vessels, counting from 1, are hand-made people of the story (1 = VESSEL, Aonia, the base look everyone is drawn from; 2 = VESSEL2, 3 = VESSEL3, unused by the restarted story). the rest, and all if this is 0, come from the generator. */
#define STORY_CANON 1
const Person *story_vessel_look(unsigned index) {
    if (index == 1 && STORY_CANON >= 1) return &VESSEL;
    if (index == 2 && STORY_CANON >= 2) return &VESSEL2;
    if (index == 3 && STORY_CANON >= 3) return &VESSEL3;
    return vessel_gen_person(index);
}
const char *story_vessel_name(unsigned index) { return story_vessel_look(index)->name; }

/* the vessel the player is in now (1 = the first, Aonia). the body on screen is always story_vessel_look(this): the sprite is made from the index, nothing else is kept */
static unsigned cur_vessel = 1;
unsigned story_vessel_index(void) { return cur_vessel; }
void story_vessel_set(unsigned index) { cur_vessel = index ? index : 1; world_set_player(story_vessel_look(cur_vessel)); }
void story_vessel_next(void) { story_vessel_set(cur_vessel + 1); }     /* a death: the soul wakes in the next vessel */

/* ---------- timers ---------- */
#define MAX_TIMERS 8
static struct { float left; void (*fn)(void); } timers[MAX_TIMERS];

void story_after(float seconds, void (*fn)(void)) {
    for (int i = 0; i < MAX_TIMERS; i++)
        if (!timers[i].fn) { timers[i].left = seconds; timers[i].fn = fn; return; }
}

void story_update(float dt) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].fn) continue;
        timers[i].left -= dt;
        if (timers[i].left <= 0) { void (*fn)(void) = timers[i].fn; timers[i].fn = NULL; fn(); }
    }
}

/* ---------- limbo (the soul on the stars screen; the world does not run, so it has its own little clock) ---------- */
static struct {
    const char *const *lines; int n, i;
    float gap, left;
    void (*on_done)(void);
    int running, on, ended;
} limbo;

void limbo_run(const char *const *thoughts, int n, float gap, void (*on_done)(void)) {
    limbo.lines = thoughts; limbo.n = thoughts ? n : 0; limbo.i = 0;
    limbo.gap = gap > 0 ? gap : 0; limbo.left = 0;
    limbo.on_done = on_done;
    limbo.running = 1; limbo.on = 1; limbo.ended = 0;
    thought_set_voice(&SOUL);
    if (limbo.n > 0) { thought_say(limbo.lines[0], 0); limbo.i = 1; }
    limbo.left = limbo.gap;
}

void limbo_end(void) {
    limbo.running = 0; limbo.on = 0; limbo.ended = 1;
    thought_set_voice(NULL);
}

int story_limbo_active(void) { return limbo.on; }
int story_limbo_take_end(void) { int e = limbo.ended; limbo.ended = 0; return e; }
int story_limbo_take_enter(void) { return 0; }             /* nothing sends us back to limbo yet (no deaths in this story so far) */

void story_limbo_update(float dt) {
    if (!limbo.running) return;
    limbo.left -= dt;
    if (limbo.left > 0) return;
    if (limbo.i < limbo.n) { thought_say(limbo.lines[limbo.i++], 0); limbo.left += limbo.gap; return; }
    limbo.running = 0;
    if (limbo.on_done) { void (*fn)(void) = limbo.on_done; limbo.on_done = NULL; fn(); }
}

/* ---------- the beginning: the soul wakes in the dark ---------- */
#define LIMBO_BEAT 1.0f
static const DialogLine WAKE_DLG[] = { { &SOUL, "Where am I?" }, { &SOUL, "It is so dark here." } };
static int limbo_stay, limbo_opening;

static void wake_done(void) {
    limbo_opening = 0;
    jukebox_scene_fade(1.0f);                  /* the menu music goes; the temple's own starts when the world shows (game.c) */
    limbo_end();
}
static void wake_say(void) { dialog_play(WAKE_DLG, (int)(sizeof WAKE_DLG / sizeof WAKE_DLG[0]), limbo_stay ? NULL : wake_done); }

int story_limbo_opening(void) { return limbo_opening; }
void story_limbo_begin(int stay) {
    limbo_stay = stay; limbo_opening = 1;
    limbo_run(NULL, 0, LIMBO_BEAT, wake_say);
}

/* ---------- the world starts ---------- */
void story_start(void) {
    memset(timers, 0, sizeof timers);
    { const char *e = getenv("VESSEL_INDEX"); story_vessel_set(e && atoi(e) > 0 ? (unsigned)atoi(e) : 1); }   /* a new game starts in vessel 1; dev: VESSEL_INDEX=n starts in vessel n */
    world_set_controls_visible(1);             /* nothing to wait for: the stick and the button are there from the first frame */
}

/* saves are gone with the old story (the first save format is in trash/story_old.c) */
int story_save_exists(void) { return 0; }
int story_load_begin(void) { return 0; }
void story_vessel2_ready(void) {}
void story_vessel3_ready(void) {}
