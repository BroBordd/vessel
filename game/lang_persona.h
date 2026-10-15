/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * PERSONA PRESETS AND QUIRKS (docs/convo_plan.md, chunk 13). No SDL.
 *
 * ten ready-made characters, so a later vessel's npc is one line in story.c:
 *     static Persona KEEPER_MIND;   persona_init(&KEEPER_MIND, PERSONA_OLD_KEEPER, "Orrin");
 * every preset has a voice (or two), numbers (mood, warmth, toxic, patience), a `lean` (the moods it drifts to) and a quirk bit field.
 * persona_init copies a preset into a Persona of the npc (a Persona keeps state: the mood, the vector, the sticking mood) and zeroes that state.
 *
 * the quirks are only NAMED here. what each one does lands in the chunks that own the behaviour, so a quirk set today changes nothing yet:
 *   Q_ASKS_BACK  asks the player questions back                        (chunk 41, npc initiative)
 *   Q_SWEARS     mild words (damn, hell) allowed in the lines          (a rude voice only, never slurs; Open decisions)
 *   Q_RHYMES     now and then ends a reply with a rhyme                (chunk 13 names it; the rhyme pools come with the string chunks)
 *   Q_FORGETFUL  loses the player's name now and then                  (chunk 39, callbacks)
 *   Q_LIES       flips a few answers                                   (the group chunks mark which keys may flip)
 *   Q_LONG       longer answers                                        (chunk 42, pacing)
 *   Q_TERSE      shorter answers                                       (chunk 42, pacing) */
#ifndef LANG_PERSONA_H
#define LANG_PERSONA_H
#include "lang.h"

enum { Q_ASKS_BACK = 1, Q_SWEARS = 2, Q_RHYMES = 4, Q_FORGETFUL = 8, Q_LIES = 16, Q_LONG = 32, Q_TERSE = 64 };     /* Persona.quirk is a uint8_t: room for one more */
#define QUIRK_COUNT 7

typedef enum {
    PERSONA_GODDESS,         /* divine, toxic, bored: Dea's numbers */
    PERSONA_STREET_FRIEND,   /* street, warm, curious: Alex's numbers */
    PERSONA_OLD_KEEPER,      /* elder, solemn, forgetful */
    PERSONA_LOST_CHILD,      /* child, shy, afraid, asks back */
    PERSONA_CLERK_GUARD,     /* formal, suspicious, terse */
    PERSONA_BEAST,           /* beast: fragments, hostile, afraid */
    PERSONA_MACHINE,         /* machine: flat, curious, asks back */
    PERSONA_WHISPER_GHOST,   /* ghost: hollow, tender, forgetful */
    PERSONA_POET,            /* poet: tender, solemn, rhymes */
    PERSONA_TRICKSTER,       /* street + poet: playful, smug, lies */
    PERSONA_COUNT
} PersonaId;

const Persona *persona_preset(int id);                           /* the template, or NULL if out of range (read only) */
const char    *persona_preset_name(int id);                      /* "goddess", "street_friend" ... */
const char    *persona_quirk_name(int bit);                      /* "asks_back" ... for the quirk bit (1, 2, 4 ...), "?" if none */
/* make `out` a fresh Persona from preset `id` under `name` (the name is not copied: it must outlive the persona, a string literal is fine).
 * all the state (the mood moves, the vector, hold, quiet ...) starts at the preset's. returns 0 if the id is bad (out is then untouched) */
int            persona_init(Persona *out, int id, const char *name);

#endif
