/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE SCRIPT. Every line of text, every character and every scripted event lives here.
 * Building blocks you can call from anywhere in this file:
 *
 *   dialog_play(lines, count, on_done)      show pages of text (centred window or face popup)
 *   mission_add("Text") / mission_complete(id)   the top-left checklist
 *   task_toast("Text")                      like mission_add, plus a coin ding and a "New task added" toast (toast.h)
 *   npc_add(&PERSON, tile_x, tile_y, on_talk)    put a character on the map
 *   world_set_controls_visible(0 or 1)      show / hide the analog stick + interact button
 *   story_after(seconds, fn)                run something later
 *   npc_set_facing(id, facing)              how an npc stands when nobody is near
 *   world_open_hole(x, y, on_enter)         cloud hole in the floor you can jump into
 *   world_fall_to_green(on_up)              jump, fall, land face-first, get up, then on_up()
 *   hud_set_person(&PERSON)                 change who the ID card shows. a new name flashes, dings + toasts
 *   hud_set_hp(hp, max)                     the HP bar on the ID card
 *   dialog_on_page(fn)                      run fn(page) as each page of the NEXT dialog_play begins
 *   dialog_on_highlight(fn)                 run fn(page, span) when a {highlighted} span of the NEXT dialog_play has been typed out
 *   convo_ask_questions(&P, &MIND, on_end)  "do you have any questions?" then a free typed chat (convo.h)
 *   convo_open(&P, &MIND, again, on_end)    the npc speaks first, the player may type or skip
 */
#include "story.h"
#include "dialog.h"
#include "missions.h"
#include "toast.h"
#include "npc.h"
#include "world.h"
#include "audio.h"
#include "hud.h"
#include "convo.h"
#include "lang.h"

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

/* ---------- minds: how each npc talks (see lang.h) ----------
 *                          name    style          mood warmth toxic patience(replies) */
static Persona DEA_MIND  = { "Dea",  STYLE_DIVINE,   0,   30,   65,   8 };     /* the goddess: toxic, condescending, "i do not care" */
static Persona ALEX_MIND = { "Alex", STYLE_STREET,  10,   70,    5,  12 };

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
static int mission_talk_dea = -1;
static int mission_find_orb = -1;           /* added by Dea's highlighted orb line (chunk 5) */
static int dea_spoken;
static int dea_tx, dea_ty;                  /* where Dea stands (the hole opens a few tiles below her) */

/* ---------- scene 3: down on the ground ---------- */
/* nobody stands near the landing spot: the player lands alone in GRASSLANDS.
 * Alex is placed far away in a later chunk (docs/story/vessel-1.md, chunks 8-9). */

/* what the player says out loud after the fall (face popup). the controls are hidden while a
 * dialog is open and come back by themselves when it ends (world.c) */
static const DialogLine LANDING[] = {
    { &VESSEL, "Ow. That hurts." },
    { &VESSEL, "She could at least have warned me." },
};

/* the player just got back on their feet after the fall */
static void landed(void) {
    dialog_play(LANDING, (int)(sizeof LANDING / sizeof LANDING[0]), NULL);
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
    { &DEA,    "Now, your first mission. I {dropped my orb in the Grasslands}. Find it and bring it back to me." },
};
#define DEA_TALK_COUNT  ((int)(sizeof DEA_TALK / sizeof DEA_TALK[0]))
/* then Dea asks "do you have any questions?" (convo.c) and the hole opens when the chat is over */

/* pressed the arrow button at the hole */
static void on_enter_hole(void) { world_fall_to_green(landed); }

static void open_the_hole(void) {
    world_open_hole(dea_tx, dea_ty + 3, on_enter_hole);         /* a hole of clouds opens in the floor in front of her */
}
static void dea_talk_done(void) { convo_ask_questions(&DEA, &DEA_MIND, open_the_hole); }

/* page 4 of DEA_TALK is the first line spoken as VESSEL: that is the moment the name sticks */
#define DEA_NAMING_PAGE 4
static void dea_talk_page(int page) {
    if (page == DEA_NAMING_PAGE) { hud_set_person(&VESSEL); convo_set_player(&VESSEL); }   /* ID card flashes, coin ding, "You are now Aonia" */
}

/* the last page of DEA_TALK holds the highlighted orb sentence. the moment those words have been
 * typed out, the task coins in: ding + "New task added" toast + "Find the orb" on the checklist */
#define DEA_ORB_PAGE  (DEA_TALK_COUNT - 1)
static void dea_talk_highlight(int page, int span) {
    if (page == DEA_ORB_PAGE && span == 0 && mission_find_orb < 0)
        mission_find_orb = task_toast("Find the orb");
}

static void on_talk_dea(int npc_id) {
    (void)npc_id;
    if (dea_spoken) { convo_open(&DEA, &DEA_MIND, 1, NULL); return; }       /* free chat. she nags about the hole */
    dea_spoken = 1;
    mission_complete(mission_talk_dea);
    dialog_on_page(dea_talk_page);
    dialog_on_highlight(dea_talk_highlight);
    dialog_play(DEA_TALK, DEA_TALK_COUNT, dea_talk_done);
}

/* ---------- scene 1: welcome, up in the clouds ---------- */
static const DialogLine HELLO[]  = { { NULL, "Hello, " VESSEL_LATIN "!" } };
static const DialogLine SUMMON[] = { { NULL, "You have been summoned. Talk to Goddess." } };

static void intro_done(void) {
    world_set_controls_visible(1);              /* stick and interact button become usable */
    mission_talk_dea = mission_add("Talk to Goddess");
    /* Dea stands 8 tiles above where we spawned, looking down at us */
    dea_tx = world_player_tile_x();
    dea_ty = world_player_tile_y() - 8;
    int id = npc_add(&DEA, dea_tx, dea_ty, on_talk_dea);
    npc_set_facing(id, FACE_DOWN);
}

static void hello_done(void) {
    dialog_play(SUMMON, 1, intro_done);
}

static void intro(void) { dialog_play(HELLO, 1, hello_done); }

/* ---------- entry point ---------- */
void story_start(void) {
    for (int i = 0; i < MAX_TIMERS; i++) timers[i].fn = NULL;
    mission_talk_dea = -1;
    mission_find_orb = -1;
    dea_spoken = 0;
    DEA_MIND.mood = 0; ALEX_MIND.mood = 10;
    convo_set_player(&VAS);
    world_set_controls_visible(0);
    story_after(1.0f, intro);                   /* wait for the fade-in, then the welcome window */
}
