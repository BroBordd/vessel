/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE SCRIPT. Every line of text, every character and every scripted event lives here.
 * Building blocks you can call from anywhere in this file:
 *
 *   dialog_play(lines, count, on_done)      show pages of text (centred window or face popup)
 *   mission_add("Text") / mission_complete(id)   the top-left checklist
 *   npc_add(&PERSON, tile_x, tile_y, on_talk)    put a character on the map
 *   world_set_controls_visible(0 or 1)      show / hide the analog stick + interact button
 *   music_set_volume(0.0 .. 1.0)            instant music volume
 *   story_after(seconds, fn)                run something later
 */
#include "story.h"
#include "dialog.h"
#include "missions.h"
#include "npc.h"
#include "world.h"
#include "audio.h"

#define VESSEL_NAME "Aonia"         /* vessel one. later: Doia, Tria, Ceathia ... */

/* ---------- characters ---------- */
/*                      name           hair            skin             shirt           pants          boots        long hair */
const Person VESSEL = { VESSEL_NAME, { 96, 58, 36 }, { 248, 208, 170 }, { 214, 60, 60 }, { 52, 70, 140 }, { 40, 30, 30 }, 0 };
const Person ALEX   = { "Alex",      { 236, 196, 84 }, { 244, 200, 164 }, { 60, 170, 170 }, { 70, 60, 110 }, { 40, 30, 30 }, 1 };

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
        if (timers[i].left <= 0) {
            void (*fn)(void) = timers[i].fn;
            timers[i].fn = NULL;
            fn();
        }
    }
}

/* ---------- state ---------- */
static int mission_talk_alex = -1;
static int alex_met;

/* ---------- scene 2: Alex ---------- */
static const DialogLine ALEX_CHAT[] = {
    { &ALEX,   "Hi! You must be new here." },
    { &VESSEL, "Hello. Yes, I just arrived." },
    { &ALEX,   "Welcome. Try not to stand out." },
};

static const DialogLine ALEX_AGAIN[] = {
    { &ALEX, "Keep your head down. I will find you when it is time." },
};

/* runs when the interact button is pressed next to Alex */
static void on_talk_alex(int npc_id) {
    (void)npc_id;
    if (alex_met) { dialog_play(ALEX_AGAIN, 1, NULL); return; }   /* already introduced */
    alex_met = 1;
    mission_complete(mission_talk_alex);
    dialog_play(ALEX_CHAT, 3, NULL);
}

/* ---------- scene 1: welcome ---------- */
static const DialogLine INTRO[] = {
    { NULL, "Welcome, " VESSEL_NAME ".\nYou ought to serve your purpose." },
    { NULL, "You have been sent to fulfill a mission. Your life will end the moment your job is done. Stay secret." },
};

static void intro_done(void) {
    world_set_controls_visible(1);              /* stick and interact button become usable */
    music_set_volume(0.65f);                    /* music drops to 65%, instantly */
    mission_talk_alex = mission_add("Talk to Alex");
    /* Alex stands 3 tiles right and 4 tiles up from where we spawned */
    npc_add(&ALEX, world_player_tile_x() + 3, world_player_tile_y() - 4, on_talk_alex);
}

static void intro(void) { dialog_play(INTRO, 2, intro_done); }

/* ---------- entry point ---------- */
void story_start(void) {
    for (int i = 0; i < MAX_TIMERS; i++) timers[i].fn = NULL;
    mission_talk_alex = -1;
    alex_met = 0;
    music_set_volume(1.0f);
    world_set_controls_visible(0);
    story_after(1.0f, intro);                   /* wait for the fade-in, then the welcome window */
}
