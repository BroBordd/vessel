/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE BRAIN WINDOW: opens when the player taps the brain button (or its thought card, thought.h).
 * it is the vessel's inner monologue, as a little monitor:
 *   - a monitor at the top shows what is in the vessel's head, as a looping pixel-art scene, with the
 *     player's face + a thought bubble in the corner ("our icon")
 *   - under it, the thought itself, typed out ("I wonder what that orb looks like.")
 *   - the window does not sit on one thought: it loops through an ARRAY of thoughts, giving each a fair
 *     time (a flicker between them, a bar shows how long is left), the header says how many there are
 *   - numbered boxes pick one thought to look at: picking it pins it (the loop stops). AUTO starts the
 *     loop again.
 *
 * the window starts EMPTY. thoughts are EVENT BASED: the story acquires one when something happens
 * (brainwin_acquire) and silently drops it when it stops being true, e.g. leaving the map it was
 * about (brainwin_drop_tag). see the THOUGHT_ tags in story.c. up to BRAIN_MAX_THOUGHTS at once. to add a
 * new picture, add a scene_xxx() in brainwin.c next to the others and a BRAIN_SCENE_ entry here. */
#ifndef BRAINWIN_H
#define BRAINWIN_H
#include <SDL2/SDL.h>

typedef enum {
    BRAIN_SCENE_ORB,        /* a bobbing "?" and a vague item that keeps changing shape */
    BRAIN_SCENE_GRASS,      /* the map, scrolling by: grass, trees, a pond and a path */
    BRAIN_SCENE_CLOUDS,     /* the sky they came from: two layers of drifting clouds */
    BRAIN_SCENE_COIN,       /* the spinning task coin, with sparkles */
    BRAIN_SCENE_COUNT
} BrainScene;

#define BRAIN_MAX_THOUGHTS 12

void brainwin_init(int w, int h);                       /* empties the window */
int  brainwin_add(const char *text, BrainScene scene);  /* appends a thought (tag 0: never dropped), returns its index or -1 when full. text is copied (up to ~90 letters) */
/* appends a thought that belongs to `tag` (any non-zero int the story picks). if the window is open
 * on AUTO it jumps to the new thought. no sound here: the story plays the ding. */
int  brainwin_acquire(const char *text, BrainScene scene, int tag);
void brainwin_clear(void);                              /* forgets everything (a new game) */
int  brainwin_drop_tag(int tag);                        /* silently forgets every thought with this tag. returns how many went */
int  brainwin_count(void);

void brainwin_open(void);
int  brainwin_active(void);                             /* open (or still sliding away) */
/* a = Android MotionEvent action. returns 1 when the touch was used (always, while the window is open) */
int  brainwin_touch(int a, int x, int y);
void brainwin_update(float dt);
void brainwin_draw(SDL_Renderer *r);

/* test hooks */
int  brainwin_debug_current(void);                      /* index of the thought on the monitor */
int  brainwin_debug_auto(void);                         /* 1 = looping, 0 = pinned */
void brainwin_debug_rects(SDL_Rect *win, SDL_Rect *mon, SDL_Rect *auto_btn, SDL_Rect *close_btn);
SDL_Rect brainwin_debug_pick(int i);                    /* the numbered box of thought i */

#endif
