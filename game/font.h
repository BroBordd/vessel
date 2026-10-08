/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef FONT_H
#define FONT_H
#include <SDL2/SDL.h>

/* tiny 5x7 bitmap font: A-Z, 0-9, space. Draws with the renderer's current color. */
void font_draw(SDL_Renderer *r, const char *s, int x, int y, int cell);
int  font_width(const char *s, int cell);
int  font_height(int cell);

#endif
