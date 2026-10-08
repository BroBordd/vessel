/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef LOADING_H
#define LOADING_H
#include <SDL2/SDL.h>

#define LOADING_SECONDS 2.0f

void loading_init(int w, int h);
/* returns 1 once LOADING_SECONDS have elapsed */
int  loading_update(float dt);
void loading_draw(SDL_Renderer *r);

#endif
