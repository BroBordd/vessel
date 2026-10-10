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
void missions_clear(void);                /* empties the list (a new life), keeps the layout */
int  missions_count(void);                /* entries on the list right now, ticked ones that have not faded yet included */
void missions_update(float dt);
void missions_set_offset(int px);         /* push the list down (screen px), e.g. under the now-playing card */
void missions_draw(SDL_Renderer *r);
int  missions_bottom(void);               /* screen y just under the list (or where it would start, if empty); valid after missions_draw */

#endif
