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

/* LIMBO (vessel 2): the soul thinks on the space screen. see story.c.
 * limbo_run: says the lines (the first at once, then one every `gap` seconds), then `gap` seconds later runs on_done.
 *   the thoughts wear the soul's face, ring no coin and do not join the brain window. `thoughts` must stay alive.
 * limbo_end: leaves limbo (voice back to the ID card person); game.c then shows the world again. */
void limbo_run(const char *const *thoughts, int n, float gap, void (*on_done)(void));
void limbo_end(void);
/* engine hooks for game.c: while the state is ST_LIMBO call story_limbo_update(dt) every frame; story_limbo_take_end()
 * is 1 once after limbo_end() (time to leave ST_LIMBO). story_limbo_active: a limbo_run has begun and limbo_end has not run. */
void story_limbo_update(float dt);
int  story_limbo_take_end(void);
int  story_limbo_active(void);

#endif
