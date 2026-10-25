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
#include "events.h"
#include "item.h"
#include "bodies.h"
#include "props.h"
#include "toast.h"
#include "audio.h"
#include <stdio.h>
#include "missions.h"
#include "hud.h"
#include "ritual.h"
#include <math.h>
#include "vessel_gen.h"
#include <string.h>
#include <stdlib.h>

static void story_woke(void);                    /* the new body has formed in the circle (defined with story_start) */
static int note_read, task_note = -1;            /* the note (chunk 5): it has been read (or can never be), and the id of the task "Read the note" */

#define VESSEL_NAME  "Aonia"
#define VESSEL2_NAME "Doia"
#define VESSEL3_NAME "Tria"
#define VESSEL_LATIN "Vas"

/*                      name           hair            skin             shirt           pants          boots        long hair  accessories */
const Person SOUL = { "Vas", { 232, 240, 255 }, { 188, 226, 246 }, { 140, 186, 238 }, { 118, 160, 218 }, { 96, 130, 196 }, 0, ACC_HALO | ACC_WORRIED };
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

/* the vessel the player is in now (1 = Aonia, who is dead at the start; the game starts in 2). the body on screen is always story_vessel_look(this): the sprite is made from the index, nothing else is kept */
#define START_VESSEL 2                                              /* vessel 1 (Aonia) lies dead in the ruins when the story starts: the player wakes in vessel 2 */
static unsigned cur_vessel = START_VESSEL;
unsigned story_vessel_index(void) { return cur_vessel; }
void story_vessel_set(unsigned index) {
    cur_vessel = index ? index : START_VESSEL; world_set_player(story_vessel_look(cur_vessel));
    if (cur_vessel >= 20) event_fire(EV_VESSEL_20); else if (cur_vessel >= 10) event_fire(EV_VESSEL_10); else if (cur_vessel >= 5) event_fire(EV_VESSEL_5);   /* the crowd thoughts (once each) */
}
void story_vessel_next(void) { story_vessel_set(cur_vessel + 1); }     /* a death: the soul wakes in the next vessel */

/* ---------- timers ---------- */
#define MAX_TIMERS 8
static struct { float left; void (*fn)(void); } timers[MAX_TIMERS];

void story_after(float seconds, void (*fn)(void)) {
    for (int i = 0; i < MAX_TIMERS; i++)
        if (!timers[i].fn) { timers[i].left = seconds; timers[i].fn = fn; return; }
}

void story_update(float dt) {
    note_watch();                               /* the paper has been taken: read it */
    if (item_has(ITEM_INTESTINE)) event_fire(EV_FIRST_INTESTINE);   /* chunk 9 */
    events_update(dt);                          /* the event thoughts: watches the world, thinks what was fired (events.c) */
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
/* the first words of the game: Vas, a soul, one line per tap. then the stars fade and the ruins come (the blood circle has been waiting under the world, see story_start) */
static const DialogLine WAKE_DLG[] = {
    { &SOUL, "Well. We did it. We died." },
    { &SOUL, "So this is what being dead feels like. Quiet. Almost restful." },
    { &SOUL, "Now it's up to him. Or should I say... me." },
};
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

/* ---------- a death: limbo again, one tap, then the next vessel wakes in the circle ---------- */
/* every later death: the world fades to black, the soul has one line that depends on how we died (a tap lets it go), and the next vessel is drawn up out
 * of the blood. story_die is called by whatever kills us (the knife on ourselves, the pit, the checkpoint); nothing calls it yet but the dev hook below. */
static const char *const DEATH_LINE[DEATH_CAUSES] = {
    [DEATH_SELF]   = "Again. Fine.",
    [DEATH_PIT]    = "I didn't plan that one.",
    [DEATH_CIRCLE] = "One life, one circle. Fair price.",
};
static int dying, enter_pending;
static DeathCause death_cause;
static DialogLine death_dlg[1];
static float die_tx, die_ty;                                    /* where the feet were when we died (tiles) */

static void death_woke(void) {
    unsigned dead = story_vessel_index();
    int gk[16], gx[16], gy[16], gn = 0;                         /* chunk 8: what lay on the floor (rocks, a thrown rock) lies there still after the map is made again. the paper does not come back */
    for (int i = 0; i < item_ground_count() && gn < 16; i++) { int k, x, y; if (item_ground_get(i, &k, &x, &y) && k != ITEM_PAPER) { gk[gn] = k; gx[gn] = x; gy[gn] = y; gn++; } }
    if (death_cause != DEATH_PIT) {                             /* the body stays where it fell (a fall into the pit leaves nothing on the floor: chunk 11 keeps that pile) */
        unsigned char inv[ITEM_COUNT]; item_pack(inv);          /* ... with what it carried */
        float cx, cy; ritual_centre(&cx, &cy);
        float dx = die_tx - cx, dy = die_ty - cy;
        if (fabsf(dx) < 3.4f && fabsf(dy) < 2.6f) {             /* it fell on the circle, where the next body will stand: it lies at the circle's edge instead */
            die_tx = cx + (dx < 0 ? -3.4f : 3.4f); die_ty = cy + (dy < -1.5f ? -1.5f : dy > 1.5f ? 1.5f : dy);
        }
        bodies_add(dead, die_tx, die_ty, inv);
    }
    if (death_cause == DEATH_SELF && dead == START_VESSEL && !note_read) event_fire(EV_SECRET_FAST_DEATH);   /* stabbed ourselves before reading the note: there never is one */
    story_vessel_next();                                        /* the soul wakes in the next body (its look comes from its number) */
    hud_reset(story_vessel_look(story_vessel_index()), 0);      /* a body that is not the first knows its name from the moment it wakes: no flash, no toast */
    item_clear(); note_read = 1;                                /* the pockets are empty; the note belongs to vessel 2 alone */
    if (task_note >= 0) { mission_complete(task_note); task_note = -1; mission_add("Escape the ruins"); }
    world_return_to_clouds();                                   /* the ruins again (the same map), the blackout lifted */
    world_ritual(story_vessel_look(dead), story_woke);          /* the blood runs, the new body is drawn up out of it; the controls come back after */
    for (int i = 0; i < gn; i++) world_place_item(gk[i], gx[i], gy[i]);
    dying = 0;
    limbo_end();                                                /* game.c fades the stars out and the world in */
}
static void death_say(void) {
    death_dlg[0].who = &SOUL; death_dlg[0].text = DEATH_LINE[death_cause]; death_dlg[0].reply = 0;
    dialog_play(death_dlg, 1, death_woke);
}
static void died_black(void) { enter_pending = 1; }            /* the world is black: game.c may switch to the stars */
void story_die(DeathCause cause) {
    if (dying || cause < 0 || cause >= DEATH_CAUSES) return;
    dying = 1; death_cause = cause;
    world_player_tilef(&die_tx, &die_ty);
    world_set_controls_visible(0);
    world_fade_to_black(1.2f, died_black);
}
int story_limbo_take_enter(void) {
    if (!enter_pending) return 0;
    enter_pending = 0;
    limbo_run(NULL, 0, 0.7f, death_say);                        /* a beat of stars, then the line */
    return 1;
}
/* dev: VESSEL_DIE=self|pit|circle dies 6 s after the world starts, to try the limbo on the phone before anything can kill us for real */
static void dev_die(void) {
    const char *e = getenv("VESSEL_DIE");
    if (!e) return;
    story_die(!strcmp(e, "pit") ? DEATH_PIT : !strcmp(e, "circle") ? DEATH_CIRCLE : DEATH_SELF);
}

/* ---------- the note (chunk 5) ---------- */
/* a paper lies by Aonia. TAKING it (the hand button, item.c) reads it, once: the pages play as a dialog in Aonia's face, the card flips from Vas to the
 * vessel's real name (name flash, coin ding, toast: hud_set_person), the paper is gone for good and the task changes. only vessel 2 can read it: it is
 * addressed to Doia, and a later body wakes with no paper on the floor (the map is made again and nothing puts it back). */
static const DialogLine NOTE_DLG[] = {
    { &VESSEL, "Hello, Doia." },
    { &VESSEL, "If you are reading this, it worked. You are Vessel 2." },
    { &VESSEL, "I am Aonia, Vessel 1, the one lying beside you. I died so that you could live." },
    { &VESSEL, "Do not be afraid. I am you." },
    { &VESSEL, "We are trapped in these ruins. Past the fallen hall, the ground has broken open. There is a pit with no bottom, and I could not cross it." },
    { &VESSEL, "I was running out of time, so Vas and I decided. I would die, and you would take my place." },
    { &VESSEL, "Every time we die, we lose what we knew. That is the price. I paid it. Now you have to." },
    { &VESSEL, "I left you everything I had. Find a way out." },
};
static Person card_vas;                                         /* the card before the note: this body's face under the plain name Vas */
static void card_show_vas(void) { card_vas = *story_vessel_look(story_vessel_index()); card_vas.name = VESSEL_LATIN; hud_reset(&card_vas, 0); }

static void note_done(void) {
    hud_set_person(story_vessel_look(story_vessel_index()));    /* Doia: the card flashes, the coin dings, the toast says YOU ARE NOW Doia */
    if (task_note >= 0) mission_complete(task_note);
    task_note = -1;
    mission_add("Escape the ruins");
    event_fire(EV_READ_NOTE);
}
static void note_watch(void) {
    if (note_read || !item_has(ITEM_PAPER)) return;
    note_read = 1;
    item_take(ITEM_PAPER);                                      /* it is read and gone: no notes after this one */
    dialog_play(NOTE_DLG, (int)(sizeof NOTE_DLG / sizeof NOTE_DLG[0]), note_done);
}

/* ---------- inspecting a body (chunk 6) ---------- */
/* the magnifying glass next to a body: a toast says what it carries, and the first look at Aonia is a thought */
static void story_inspect(unsigned vessel) {
    const Body *b = bodies_find(vessel);
    char what[96], line[128];
    bodies_inv_text(b, what, sizeof what);
    int took = bodies_take_all(vessel);                         /* chunk 8: looking close means the pockets are ours now */
    snprintf(line, sizeof line, "%s %s%s", story_vessel_name(vessel), what, took > 1 ? ". You take them." : took ? ". You take it." : "");
    toast_notice("YOU LOOK CLOSE", line);
    if (vessel == 1) event_fire(EV_INSPECT_BODY);
}

/* ---------- cutting bodies, the inventory's actions (chunks 8 and 9) ---------- */
/* each stab on a body takes the next step of section 6: a wound, the arms, the legs, the head, the torso opens. the parts fall on the floor around the body (they are
 * items, so a small one is taken with the hand button and a MEDIUM one (arm, leg, head, spine, bone) is carried, one at a time). a thrown big rock does all the
 * steps at once and scatters what the body carried. a stab on ourselves is a death like any other: the knife stays in the dead body's pockets (death_woke) */
static void part_fall(unsigned v, int kind) { float tx, ty; if (world_body_tile(v, &tx, &ty)) world_drop_item_near(kind, tx, ty); }
static const char *cut_step(unsigned v, int step) {                 /* what step `step` (1..7) does; the words for the toast */
    switch (step) {
    case 1: return "A wound. The blood runs again.";
    case 2: case 3: part_fall(v, ITEM_ARM); event_fire(EV_ARM_OFF); return "An arm comes off.";
    case 4: case 5: part_fall(v, ITEM_LEG); return "A leg comes off.";
    case 6: part_fall(v, ITEM_HEAD); event_fire(EV_HEAD_OFF); return "The head comes off.";
    default:
        part_fall(v, ITEM_RIBS); part_fall(v, ITEM_HEART); part_fall(v, ITEM_LIVER); part_fall(v, ITEM_INTESTINE); part_fall(v, ITEM_SPINE);
        event_fire(EV_TORSO_OPEN); return "The torso opens.";
    }
}
static void story_smash(unsigned v) {                               /* the big rock ran into a body */
    Body *b = (Body *)bodies_find(v);
    if (!b || b->cuts >= BODIES_CUTS) return;
    for (int s = b->cuts + 1; s <= BODIES_CUTS; s++) cut_step(v, s);
    bodies_cut_done(v);
    for (int k = 0; k < ITEM_COUNT; k++) while (b->inv[k]) { part_fall(v, k); b->inv[k]--; }      /* what he carried flies around */
    event_fire(EV_FIRST_STAB);
    toast_notice("SMASHED", "The rock leaves only pieces.");
}
static void story_full(void) { toast_notice("HANDS FULL", "Put something down first."); }
static void need_knife(void) { sfx_error(); toast_notice("NO KNIFE", "You need a knife for that."); }
/* chunk 10: two slots, one recipe (item.c). both ingredients are used up; a result that is carried in the hands needs them free */
static void story_craft(int a, int b) {
    if (dying) return;
    int r = item_recipe(a, b);
    if (r < 0 || (a == b && item_count(a) < 2)) { sfx_error(); toast_notice("DOESN'T FIT", "Those don't go together."); return; }
    int frees = item_size(a) == SIZE_MEDIUM || item_size(b) == SIZE_MEDIUM;
    if (item_size(r) == SIZE_MEDIUM && item_medium_held() && !frees) { sfx_error(); story_full(); return; }
    item_take(a); item_take(b); item_give(r);
    sfx_found();
    toast_notice("YOU MAKE", item_name(r));
    event_fire(EV_FIRST_CRAFT);
}
static void story_action(int kind, int act) {
    if (dying) return;
    switch (act) {
    case ACT_STAB_SELF: sfx_hit(); story_die(DEATH_SELF); return;
    case ACT_STAB: {
        unsigned v = world_body_in_reach();
        if (!v) { sfx_error(); toast_notice("NOTHING TO STAB", "Get close to a body."); return; }
        int step = bodies_cut(v);
        if (!step) { sfx_error(); toast_notice("NOTHING LEFT", "There is nothing left to cut."); return; }
        sfx_hit();
        event_fire(EV_FIRST_STAB);
        toast_notice("YOU STAB", cut_step(v, step));
        return; }
    case ACT_THROW:
        item_take(kind); sfx_blip(1.1f);
        if (world_throw_item(kind)) event_fire(EV_DROP_IN_PIT);          /* the crack took it: the echo is chunk 11 */
        return;
    case ACT_DROP: {
        float x, y; item_take(kind); world_player_tilef(&x, &y); world_drop_item_near(kind, x, y); return; }
    case ACT_SMASH: {                                                                   /* the club: all of section 6 at once, no knife */
        unsigned v = world_body_in_reach();
        if (!v) { sfx_error(); toast_notice("NOTHING TO SMASH", "Get close to a body."); return; }
        sfx_hit(); story_smash(v); return; }
    case ACT_SWING: toast_notice("NOT HERE", "Swing it at the edge of the pit."); return;   /* the swing itself is chunk 11 */
    case ACT_WEAR: { int on = item_wear_toggle(kind); toast_notice(on ? "YOU WEAR IT" : "TAKEN OFF", item_name(kind)); return; }
    case ACT_CUT_FINGERS: if (!item_has(ITEM_KNIFE)) { need_knife(); return; } sfx_hit(); item_give(ITEM_FINGERS); return;      /* the arm stays: fingers do not use it up */
    case ACT_CUT_EYES:    if (!item_has(ITEM_KNIFE)) { need_knife(); return; } sfx_hit(); item_give(ITEM_EYES); return;
    case ACT_CUT_TEETH:   if (!item_has(ITEM_KNIFE)) { need_knife(); return; } sfx_hit(); item_give(ITEM_TEETH); return;
    case ACT_CUT_BONE:    if (!item_has(ITEM_KNIFE)) { need_knife(); return; } sfx_hit(); item_take(kind); item_give(ITEM_BONE); return;   /* the limb is used up */
    case ACT_CLEAN:       if (!item_has(ITEM_KNIFE)) { need_knife(); return; } sfx_hit(); item_take(kind); item_give(ITEM_SKULL); return;  /* so is the head */
    }
}

/* ---------- the world starts ---------- */
static void story_woke(void) {                                 /* the new body has formed in the circle */
    event_fire(EV_WAKE);
    if (!note_read && task_note < 0 && story_vessel_index() == START_VESSEL) task_note = mission_add("Read the note");
}
void story_start(void) {
    memset(timers, 0, sizeof timers);
    { const char *e = getenv("VESSEL_INDEX"); story_vessel_set(e && atoi(e) > 0 ? (unsigned)atoi(e) : START_VESSEL); }   /* a new game starts in vessel 2, drawn by vessel_gen(2); dev: VESSEL_INDEX=n starts in vessel n */
    events_reset(); dying = 0; enter_pending = 0; note_read = 0; task_note = -1;
    card_show_vas();                            /* the card says Vas until the note is read */
    if (getenv("VESSEL_DIE")) story_after(6.0f, dev_die);
    item_clear();
    bodies_reset(); props_reset();               /* nobody lies anywhere; the big rock is put on the ledge by the world once the ruins are there */
    { unsigned char knife[ITEM_COUNT] = { 0 }; knife[ITEM_KNIFE] = 1; bodies_add(1, 0, 0, knife); }   /* Aonia lies at the circle (ritual.c), the knife in his hand */
    world_set_inspect_cb(story_inspect);
    item_set_action_cb(story_action); item_set_full_cb(story_full); item_set_craft_cb(story_craft); world_set_smash_cb(story_smash);
    world_ritual(story_vessel_look(1), story_woke);
    { float cx, cy; ritual_centre(&cx, &cy); world_place_item(ITEM_PAPER, (int)floorf(cx - 2.2f), (int)floorf(cy + 1.6f)); }    /* the paper lies below Aonia, who is left of the circle */
    { float cx, cy; ritual_centre(&cx, &cy);                                                      /* two small rocks lie in the circle room, to throw */
      world_place_item(ITEM_ROCK, (int)floorf(cx + 2.2f), (int)floorf(cy + 1.6f));
      world_place_item(ITEM_ROCK, (int)floorf(cx + 2.4f), (int)floorf(cy - 1.4f)); }
    /* the ruins: a blood circle carved in the floor, vessel 1 dead beside it, the new body drawn up out of the blood. the controls come back after */
}

/* saves are gone with the old story (the first save format is in trash/story_old.c) */
int story_save_exists(void) { return 0; }
int story_load_begin(void) { return 0; }
void story_vessel2_ready(void) {}
void story_vessel3_ready(void) {}
