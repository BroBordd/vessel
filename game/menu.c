/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "menu.h"
#include "font.h"
#include "space.h"

typedef struct { int x, y, w, h; const char *label; } Button;

static Button btn[2] = { { 0, 0, 0, 0, "PLAY" }, { 0, 0, 0, 0, "EXIT" } };
static int    W, H, title_cell, title_x, title_y, label_cell;
static float  u;                    /* px per "design unit" */
static int    pressed = -1;         /* button index under the finger since touch-down */
static int    inside;               /* finger still over that button */
static float  fade;                 /* seconds since PLAY was pressed; < 0 = not pressed yet */

static int hit(int x, int y) {
    for (int i = 0; i < 2; i++)
        if (x >= btn[i].x && x < btn[i].x + btn[i].w &&
            y >= btn[i].y && y < btn[i].y + btn[i].h) return i;
    return -1;
}

void menu_init(int w, int h) {
    W = w; H = h;
    u = (w < h ? w : h) / 360.0f;
    pressed = -1; inside = 0; fade = -1;
    space_init(w, h);                           /* the stars behind the menu */

    int bw = (int)(200 * u), bh = (int)(52 * u), gap = (int)(16 * u);
    int cy = (int)(H * 0.62f);
    for (int i = 0; i < 2; i++) {
        btn[i].w = bw; btn[i].h = bh;
        btn[i].x = (W - bw) / 2;
        btn[i].y = i == 0 ? cy - bh - gap / 2 : cy + gap / 2;
    }
    label_cell = (int)(3 * u); if (label_cell < 1) label_cell = 1;
    title_cell = (int)(7 * u); if (title_cell < 2) title_cell = 2;
    title_x = (W - font_width("VESSEL", title_cell)) / 2;
    title_y = (int)(H * 0.22f) - font_height(title_cell) / 2;
}

MenuAction menu_touch(int a, int x, int y) {
    if (fade >= 0) return MENU_NONE;            /* PLAY already pressed: the menu is on its way out */
    switch (a) {
    case 0:                                     /* down */
        pressed = hit(x, y);
        inside = pressed >= 0;
        break;
    case 2:                                     /* move */
        if (pressed >= 0) inside = hit(x, y) == pressed;
        break;
    case 1: {                                   /* up: fire if released on the same button */
        int p = pressed, ok = pressed >= 0 && hit(x, y) == pressed;
        pressed = -1; inside = 0;
        if (ok) return p == 0 ? MENU_PLAY : MENU_EXIT;
        break;
    }
    case 3:                                     /* cancel */
        pressed = -1; inside = 0;
        break;
    }
    return MENU_NONE;
}

void menu_fade_out(void) { if (fade < 0) { fade = 0; pressed = -1; inside = 0; } }
int  menu_faded(void)    { return fade >= MENU_FADE_T; }

void menu_update(float dt) {
    space_update(dt);
    if (fade >= 0 && fade < MENU_FADE_T) { fade += dt; if (fade > MENU_FADE_T) fade = MENU_FADE_T; }
}

void menu_draw(SDL_Renderer *r) {
    space_draw(r);                              /* clears to black, then the drifting stars */
    int A = fade < 0 ? 255 : (int)(255 * (1.0f - fade / MENU_FADE_T));   /* the title and the buttons fade out after PLAY */
    if (A <= 0) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(r, 255, 255, 255, A);
    font_draw(r, "VESSEL", title_x, title_y, title_cell);

    int bt = (int)(2 * u); if (bt < 1) bt = 1;
    for (int i = 0; i < 2; i++) {
        const Button *b = &btn[i];
        int down = pressed == i && inside;
        SDL_Rect outer = { b->x, b->y, b->w, b->h };
        SDL_Rect inner = { b->x + bt, b->y + bt, b->w - 2 * bt, b->h - 2 * bt };
        SDL_SetRenderDrawColor(r, 255, 255, 255, A);
        SDL_RenderFillRect(r, &outer);
        if (down) SDL_SetRenderDrawColor(r, 255, 255, 255, A);
        else      SDL_SetRenderDrawColor(r, 0, 0, 0, A);
        SDL_RenderFillRect(r, &inner);

        int tx = b->x + (b->w - font_width(b->label, label_cell)) / 2;
        int ty = b->y + (b->h - font_height(label_cell)) / 2;
        if (down) SDL_SetRenderDrawColor(r, 0, 0, 0, A);
        else      SDL_SetRenderDrawColor(r, 255, 255, 255, A);
        font_draw(r, b->label, tx, ty, label_cell);
    }
}
