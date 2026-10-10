/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef FONT_H
#define FONT_H
#include <SDL2/SDL.h>

/* tiny 5x7 bitmap font: A-Z, 0-9, space. Draws with the renderer's current color. */
void font_draw(SDL_Renderer *r, const char *s, int x, int y, int cell);
int  font_width(const char *s, int cell);
int  font_height(int cell);

/* the one width the mission list, the "New task added" toast and the open music card share (screen px).
 * room for 18 letters of text next to the toast's coin; each panel may still grow past it for longer text. */
static inline int ui_panel_w(int w, int h) {
    float u = (w < h ? w : h) / 360.0f;
    int q = (int)(1.7f * u); if (q < 2) q = 2;              /* the toast's pixel size */
    return 13 * q + font_width("MMMMMMMMMMMMMMMMMM", q) + 4 * q;   /* coin column + 18 letters + right pad */
}

#endif
