/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE EVENT THOUGHTS ENGINE (story chunk 3). the body thinks through the brain button (thought.h), but no story beat scripts a thought: each one is
 * fired by an EVENT, something the player does or something that happens. every event owns one line (the tables of docs/story/vessel.md, sections 10 and 11),
 * fires ONCE unless it is a repeat, and joins the brain window (brainwin.h) when it is thought.
 *
 *   event_fire(EV_SWING_HELD);          any chunk calls this when its thing happens (a swing, a find...)
 *   events_update(dt);                  every frame (story_update): hands the queued thoughts out, one at a time
 *
 * fired events wait in a small queue and are thought one at a time, only while the player is free to read (the world runs, no talk, no cutscene), with a
 * breath between them, so a thought never covers another and never pops up in a cutscene.
 * only thoughts that carry something stay: where we are, a hint, a find, a secret. the body forgets everything at death, so there are no
 * reactions, no quips, no counting of lives. every event is fired by the chunk that builds that action (EV_WAKE by story.c when the circle is done). */
#ifndef EVENTS_H
#define EVENTS_H

typedef enum {
    EV_NONE = 0,
    /* section 10: thoughts */
    EV_WAKE,            /* the new body has formed in the circle */
    EV_READ_NOTE,       /* reads the note */
    EV_HOLD_INTESTINE,  /* holds an intestine at the pit */
    EV_HOLD_TIED_ROCK,  /* holds a tied rock at the pit */
    EV_SWING_HELD,      /* Swing wedges on the far side */
    EV_ECHO_SOONER,     /* the echo comes back sooner than last time */
    EV_FIND_CIRCLE2,    /* finds the second circle */
    EV_FILL_CIRCLE,     /* fills the circle */
    /* section 11: secrets */
    EV_SECRET_BODY_ON_CIRCLE, /* drags Aonia back onto the circle */
    EV_SECRET_HUMAN_SHAPE,  /* every part of one body put back in the circle in a human shape */
    EV_SECRET_FESTIVE,      /* wears the necklaces and the skull on a string together */
    EV_SECRET_MEMORIAL,     /* draws a mark in blood next to every body */
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
