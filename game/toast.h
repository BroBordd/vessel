/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef TOAST_H
#define TOAST_H
#include <SDL2/SDL.h>

/* "New task added": a small panel that drops in under the missions list with a spinning coin,
 * the words NEW TASK ADDED and the task's name, then slides away by itself.
 *
 * task_toast() is the one call the story needs: it adds the task to the checklist right away,
 * plays the coin ding and shows the toast. returns the mission id (for mission_complete), or -1
 * if the list is full (then there is no ding and no toast). `task` must stay alive, like mission_add.
 * if a toast is already showing, the new one waits its turn (the ding plays when it appears). */
int  task_toast(const char *task);

/* engine hooks, called by the world */
void toast_init(int w, int h);              /* also clears anything pending */
void toast_update(float dt);
void toast_draw(SDL_Renderer *r, int x, int y);   /* x,y: top-left of the panel's resting spot (screen px) */

#endif
