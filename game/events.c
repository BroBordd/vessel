/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the event thoughts engine, see events.h. */
#include "events.h"
#include "thought.h"
#include "brainwin.h"
#include "dialog.h"
#include "audio.h"
#include "world.h"
#include "ritual.h"
#include <string.h>
#include <math.h>

/* every line uses only the font's letters (letters, digits, space and , . ! ? ' -), and fits 3 lines of about 19 letters */
typedef struct {
    const char *text;
    BrainScene scene;       /* the picture in the brain window */
    float repeat;           /* 0 = once. else seconds before the same thought may come again */
} EvRow;

static const EvRow ROWS[EV_COUNT] = {
    [EV_WAKE]              = { "Where am I?", BRAIN_SCENE_AGAIN, 0 },
    [EV_INSPECT_BODY]      = { "That's me. Was me.", BRAIN_SCENE_AGAIN, 0 },
    [EV_READ_NOTE]         = { "Doia. ...Fine. It's a name.", BRAIN_SCENE_ORB, 0 },
    [EV_FIRST_DRAG]        = { "He's heavier than he looks.", BRAIN_SCENE_ORB, 0 },
    [EV_DRAG_LONG]         = { "Why am I doing this.", BRAIN_SCENE_ORB, 90.0f },
    [EV_CARRY_ROCK]        = { "This is a mistake.", BRAIN_SCENE_ORB, 0 },
    [EV_FIRST_STAB]        = { "I'm sorry. ...Not really.", BRAIN_SCENE_ORB, 0 },
    [EV_ARM_OFF]           = { "That's not how arms work.", BRAIN_SCENE_ORB, 0 },
    [EV_HEAD_OFF]          = { "Don't look at the face.", BRAIN_SCENE_ORB, 0 },
    [EV_TORSO_OPEN]        = { "So that's what's in there.", BRAIN_SCENE_ORB, 0 },
    [EV_FIRST_INTESTINE]   = { "Long. Surprisingly long.", BRAIN_SCENE_ORB, 0 },
    [EV_FIRST_CRAFT]       = { "Don't think about it.", BRAIN_SCENE_ORB, 0 },
    [EV_HOLD_INTESTINE]    = { "That would reach... if something held the other end.", BRAIN_SCENE_ORB, 0 },
    [EV_HOLD_TIED_ROCK]    = { "Throw it. Hope.", BRAIN_SCENE_ORB, 0 },
    [EV_SWING_HELD]        = { "...It held. It held!", BRAIN_SCENE_ORB, 0 },
    [EV_ECHO_SOONER]       = { "It's getting shallower.", BRAIN_SCENE_ORB, 0 },
    [EV_FALL_IN_PIT]       = { "Oh.", BRAIN_SCENE_ORB, 0 },
    [EV_CLIMB_FAR]         = { "Covered in blood. Good enough.", BRAIN_SCENE_ORB, 0 },
    [EV_FIND_CIRCLE2]      = { "Another one. He'd have wanted this.", BRAIN_SCENE_AGAIN, 0 },
    [EV_FILL_CIRCLE]       = { "My consciousness is fading.", BRAIN_SCENE_AGAIN, 0 },
    [EV_SECRET_BODY_ON_CIRCLE] = { "He looks comfortable.", BRAIN_SCENE_AGAIN, 0 },
    [EV_SECRET_HUMAN_SHAPE]    = { "He's not coming back. But he looks better.", BRAIN_SCENE_AGAIN, 0 },
    [EV_SECRET_FESTIVE]    = { "I look... festive.", BRAIN_SCENE_ORB, 0 },
    [EV_SECRET_MEMORIAL]   = { "A little memorial.", BRAIN_SCENE_ORB, 0 },
    [EV_VESSEL_5]          = { "I'm getting good at this.", BRAIN_SCENE_ORB, 0 },
    [EV_VESSEL_10]         = { "A crowd.", BRAIN_SCENE_ORB, 0 },
    [EV_VESSEL_20]         = { "There's no room left.", BRAIN_SCENE_ORB, 0 },
    [EV_SECRET_KNIFE_PIT]  = { "That was dumb.", BRAIN_SCENE_ORB, 0 },
};

#define TAG_BASE   100      /* brain window tag of an event = TAG_BASE + id (never 0, so nothing here is dropped by accident) */
#define QUEUE_MAX  8
#define GAP        1.6f     /* a breath between two thoughts (s), counted from when the first one has folded away */
#define SETTLE     1.2f     /* the player must have been free this long before a thought comes (s) */
#define DRAG_LONG_S 10.0f   /* seconds of really pulling a body that make EV_DRAG_LONG */
#define MEMORY_MAX BRAIN_MAX_THOUGHTS

static unsigned char fired[EV_COUNT];
static float cooldown[EV_COUNT];            /* repeat events: seconds until they may fire again */
static EventId queue[QUEUE_MAX]; static int nq;
static EventId held[MEMORY_MAX]; static int nheld;      /* the thoughts in the brain window, oldest first (so the oldest can go when the window is full) */
static float free_t, gap_t, now_t;
static int   was_active;
static float drag_t, drag_rep;              /* seconds of pulling in a row (total), and when the long drag may fire next */

void events_reset(void) {
    memset(fired, 0, sizeof fired); memset(cooldown, 0, sizeof cooldown);
    nq = 0; nheld = 0; free_t = gap_t = now_t = 0; was_active = 0; drag_t = 0; drag_rep = 0;
    brainwin_clear();
}

const char *event_text(EventId id) { return id > EV_NONE && id < EV_COUNT && ROWS[id].text ? ROWS[id].text : ""; }
int event_fired(EventId id) { return id > EV_NONE && id < EV_COUNT && fired[id]; }
int event_pending(void) { return nq; }

void event_fire(EventId id) {
    if (id <= EV_NONE || id >= EV_COUNT || !ROWS[id].text) return;
    if (ROWS[id].repeat <= 0) { if (fired[id]) return; }                 /* once */
    else if (cooldown[id] > 0) return;                                  /* a repeat that is still resting */
    for (int i = 0; i < nq; i++) if (queue[i] == id) return;            /* already waiting */
    fired[id] = 1;
    if (ROWS[id].repeat > 0) cooldown[id] = ROWS[id].repeat;
    if (nq >= QUEUE_MAX) {                                              /* too many at once: the oldest waiting one is forgotten (a repeat first, if there is one) */
        int drop = 0;
        for (int i = 0; i < nq; i++) if (ROWS[queue[i]].repeat > 0) { drop = i; break; }
        memmove(queue + drop, queue + drop + 1, (size_t)(nq - drop - 1) * sizeof *queue); nq--;
    }
    queue[nq++] = id;
}

/* the thought is really thought: the card, the soft chime, and a place in the brain window */
static void think(EventId id) {
    const EvRow *r = &ROWS[id];
    thought_say(r->text, 0);
    sfx_thought();
    for (int i = 0; i < nheld; i++) if (held[i] == id) {                 /* a repeat is already in the window: it stays there once */
        memmove(held + i, held + i + 1, (size_t)(nheld - i - 1) * sizeof *held); nheld--; brainwin_drop_tag(TAG_BASE + (int)id); break;
    }
    if (brainwin_count() >= BRAIN_MAX_THOUGHTS && nheld > 0) {            /* the window is full: the oldest thought makes room */
        brainwin_drop_tag(TAG_BASE + (int)held[0]);
        memmove(held, held + 1, (size_t)(nheld - 1) * sizeof *held); nheld--;
    }
    if (brainwin_acquire(r->text, r->scene, TAG_BASE + (int)id) >= 0 && nheld < MEMORY_MAX) held[nheld++] = id;
}

/* the things the engine can see by itself */
static void watch(float dt) {
    if (world_dragging_rock() && world_drag_moving()) event_fire(EV_CARRY_ROCK);      /* the big rock, heavier still (chunk 7) */
    if (world_dragging() && world_drag_moving()) {
        event_fire(EV_FIRST_DRAG);
        drag_t += dt;
        if (drag_t >= DRAG_LONG_S && now_t >= drag_rep) { event_fire(EV_DRAG_LONG); drag_t = 0; drag_rep = now_t + 60.0f; }
    }
}

void events_update(float dt) {
    if (dt <= 0) return;                                                 /* paused: nothing moves on */
    now_t += dt;
    for (int i = 1; i < EV_COUNT; i++) if (cooldown[i] > 0) cooldown[i] -= dt;
    int free_now = world_controls_on() && !world_ritual_running() && !dialog_active() && !brainwin_active();
    if (free_now) { free_t += dt; watch(dt); } else free_t = 0;
    int act = thought_active();
    if (was_active && !act) gap_t = GAP;                                  /* one just folded away: a breath */
    was_active = act;
    if (gap_t > 0) gap_t -= dt;
    if (nq > 0 && !act && gap_t <= 0 && free_t >= SETTLE) {               /* nothing on the card, the player is free: the next one */
        EventId id = queue[0]; memmove(queue, queue + 1, (size_t)(nq - 1) * sizeof *queue); nq--;
        think(id);
    }
}
