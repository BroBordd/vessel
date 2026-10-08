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
 *   npc_set_facing(id, facing)              how an npc stands when nobody is near
 *   world_open_hole(x, y, on_enter)         cloud hole in the floor you can jump into
 *   world_fall_to_green(on_up)              jump, fall, land face-first, get up, then on_up()
 */
#include "story.h"
#include "dialog.h"
#include "missions.h"
#include "npc.h"
#include "world.h"
#include "audio.h"

#define VESSEL_NAME  "Aonia"        /* vessel one. later: Doia, Tria, Ceathia ... (Dea plays on the word "one") */
#define VESSEL_LATIN "Vas"          /* what Dea calls us before the naming. "Vas" is Latin for vessel */

/* ---------- characters ---------- */
/*                      name           hair            skin             shirt           pants          boots        long hair  accessories */
const Person VESSEL = { VESSEL_NAME,  { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
const Person VAS    = { VESSEL_LATIN, { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
const Person ALEX   = { "Alex",       { 236, 196, 84 }, { 244, 200, 164 }, { 60, 170, 170 }, { 70, 60, 110 }, { 40, 30, 30 }, 1, 0 };
/* "dea" is Latin for goddess. white shades, a halo, all in white and gold */
const Person DEA    = { "Dea",        { 252, 240, 190 }, { 252, 228, 206 }, { 255, 244, 200 }, { 250, 232, 170 }, { 226, 190, 90 }, 1,
                        ACC_SHADES | ACC_HALO };

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
static int mission_talk_dea = -1, mission_talk_alex = -1;
static int dea_spoken, alex_met;
static int dea_tx, dea_ty;                  /* where Dea stands (the hole opens a few tiles below her) */

/* ---------- scene 3: down on the ground, Alex ---------- */
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

/* the player just got back on their feet after the fall: the real mission starts here */
static void landed(void) {
    music_set_volume(0.65f);
    mission_talk_alex = mission_add("Talk to Alex");
    /* Alex stands 3 tiles right and 4 tiles up from where we landed */
    npc_add(&ALEX, world_player_tile_x() + 3, world_player_tile_y() - 4, on_talk_alex);
}

/* ---------- scene 2: Dea ---------- */
static const DialogLine DEA_TALK[] = {
    { &DEA,    "Hello, " VESSEL_LATIN ". I have been waiting for you." },
    { &VAS,    "Where am I?" },
    { &DEA,    "Between places. You are a vessel, and a vessel needs a name." },
    { &DEA,    "From now on, you are " VESSEL_NAME ". It plays on the word one, for you are the first." },
    { &VESSEL, VESSEL_NAME ". I will remember it." },
    { &DEA,    "Your purpose is to carry out missions. Nothing more, nothing less." },
    { &DEA,    "Finish each one, and stay secret. When your job is done, your life ends." },
    { &DEA,    "Now go." },
};

static const DialogLine DEA_AGAIN[] = {
    { &DEA, "Now go, " VESSEL_NAME ". The hole is waiting." },
};

/* pressed the arrow button at the hole */
static void on_enter_hole(void) { world_fall_to_green(landed); }

static void dea_talk_done(void) {
    world_open_hole(dea_tx, dea_ty + 3, on_enter_hole);         /* a hole of clouds opens in the floor in front of her */
}

static void on_talk_dea(int npc_id) {
    (void)npc_id;
    if (dea_spoken) { dialog_play(DEA_AGAIN, 1, NULL); return; }
    dea_spoken = 1;
    mission_complete(mission_talk_dea);
    dialog_play(DEA_TALK, 8, dea_talk_done);
}

/* ---------- scene 1: welcome, up in the clouds ---------- */
static const DialogLine INTRO[] = {
    { NULL, "Hello, " VESSEL_LATIN "!" },
    { NULL, "You have been summoned. Talk to Goddess." },
};

static void intro_done(void) {
    world_set_controls_visible(1);              /* stick and interact button become usable */
    music_set_volume(0.65f);                    /* music drops to 65%, instantly */
    mission_talk_dea = mission_add("Talk to Goddess");
    /* Dea stands 8 tiles above where we spawned, looking down at us */
    dea_tx = world_player_tile_x();
    dea_ty = world_player_tile_y() - 8;
    int id = npc_add(&DEA, dea_tx, dea_ty, on_talk_dea);
    npc_set_facing(id, FACE_DOWN);
}

static void intro(void) { dialog_play(INTRO, 2, intro_done); }

/* ---------- entry point ---------- */
void story_start(void) {
    for (int i = 0; i < MAX_TIMERS; i++) timers[i].fn = NULL;
    mission_talk_dea = mission_talk_alex = -1;
    dea_spoken = alex_met = 0;
    music_set_volume(1.0f);
    world_set_controls_visible(0);
    story_after(1.0f, intro);                   /* wait for the fade-in, then the welcome window */
}
