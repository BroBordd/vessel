/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef MISSIONS_H
#define MISSIONS_H
#include <SDL2/SDL.h>

/* checklist in the top-left corner. keep the text extremely short: "Talk to Alex". */
int  mission_add(const char *text);       /* returns an id, or -1 if the list is full. text must stay alive */
void mission_complete(int id);            /* ticks it off, then it fades away */

/* engine hooks, called by the world */
void missions_init(int w, int h);         /* also clears the list */
void missions_update(float dt);
void missions_draw(SDL_Renderer *r);

#endif
