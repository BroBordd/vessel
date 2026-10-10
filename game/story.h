/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef STORY_H
#define STORY_H
#include "char.h"

extern const Person VESSEL;         /* the player, once Dea has named them */
extern const Person SOUL;           /* the soul in limbo: pale and ghostly. its face is the brain card's voice there (thought_set_voice) */
extern const Person VAS;            /* the player before the naming: same look, just "Vas" */
extern const Person DEA;            /* the goddess in the clouds */
extern const Person ALEX;

/* called by the world when the map appears / every frame */
void story_start(void);
void story_update(float dt);

/* run fn once, `seconds` from now. handy for scripting: story_after(2.0f, next_scene); */
void story_after(float seconds, void (*fn)(void));

#endif
