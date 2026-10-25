/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE EVENT THOUGHTS ENGINE (story chunk 3). the body thinks through the brain button (thought.h), but no story beat scripts a thought: each one is
 * fired by an EVENT, something the player does or something that happens. every event owns one line (the tables of docs/story/vessel.md, sections 10 and 11),
 * fires ONCE unless it is a repeat, and joins the brain window (brainwin.h) when it is thought.
 *
 *   event_fire(EV_FIRST_STAB);          any chunk calls this when its thing happens (a stab, a craft, a swing...)
 *   events_update(dt);                  every frame (story_update): watches the world for the events it can see by itself
 *
 * fired events wait in a small queue and are thought one at a time, only while the player is free to read (the world runs, no talk, no cutscene), with a
 * breath between them, so a thought never covers another and never pops up in a cutscene.
 * the events the engine sees by itself: the new body forming (EV_WAKE is fired by story.c when the circle is done), stepping off the circle, the first drag
 * of a body, a long drag (a repeat, rare), reaching the pit edge. everything else is fired by the chunk that builds that action. */
#ifndef EVENTS_H
#define EVENTS_H

typedef enum {
    EV_NONE = 0,
    /* section 10: thoughts */
    EV_WAKE,            /* the new body has formed in the circle */
    EV_INSPECT_BODY,    /* first looks at Aonia (inspect) */
    EV_READ_NOTE,       /* reads the note */
    EV_FIRST_DRAG,      /* first drag of a body */
    EV_DRAG_LONG,       /* after dragging for 10 seconds (repeat, rare) */
    EV_CARRY_ROCK,      /* carries the big rock */
    EV_FIRST_STAB,      /* first stab of a body */
    EV_ARM_OFF,         /* an arm comes off */
    EV_HEAD_OFF,        /* the head comes off */
    EV_TORSO_OPEN,      /* the torso opens */
    EV_FIRST_INTESTINE, /* first intestine collected */
    EV_FIRST_CRAFT,     /* first craft */
    EV_PIT_EDGE,        /* arrives at the pit edge */
    EV_HOLD_INTESTINE,  /* holds an intestine at the pit */
    EV_HOLD_TIED_ROCK,  /* holds a tied rock at the pit */
    EV_SWING_HELD,      /* Swing wedges on the far side */
    EV_DROP_IN_PIT,     /* drops something into the pit (waits for the echo) */
    EV_ECHO_SOONER,     /* the echo comes back sooner than last time */
    EV_FALL_IN_PIT,     /* falls into the pit */
    EV_CLIMB_FAR,       /* climbs up the far side */
    EV_FIND_CIRCLE2,    /* finds the second circle */
    EV_FILL_CIRCLE,     /* fills the circle */
    /* section 11: secrets */
    EV_SECRET_FAST_DEATH,   /* stabs themselves in the first circle before reading the note */
    EV_SECRET_BODY_ON_CIRCLE, /* drags Aonia back onto the circle */
    EV_SECRET_HUMAN_SHAPE,  /* every part of one body put back in the circle in a human shape */
    EV_SECRET_FESTIVE,      /* wears the necklaces and the skull on a string together */
    EV_SECRET_MEMORIAL,     /* draws a mark in blood next to every body */
    EV_VESSEL_5,            /* reaches vessel 5 */
    EV_VESSEL_10,           /* reaches vessel 10 */
    EV_VESSEL_20,           /* reaches vessel 20 */
    EV_SECRET_KNIFE_PIT,    /* throws the knife into the pit */
    EV_COUNT
} EventId;

void events_reset(void);                    /* a new game: nothing has fired, nothing is waiting */
/* the event happened. a once-event that has fired already does nothing; a repeat obeys its own cooldown. the thought is queued */
void event_fire(EventId id);
int  event_fired(EventId id);               /* it has fired at least once (the thought may still be waiting its turn) */
int  event_pending(void);                   /* thoughts waiting in the queue */
void events_update(float dt);               /* every frame, the world's dt (0 while paused) */
const char *event_text(EventId id);         /* the words, "" for an unknown id */

#endif
