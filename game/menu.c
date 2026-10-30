/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "menu.h"
#include "audio.h"
#include "font.h"
#include "space.h"

typedef struct { int x, y, w, h; const char *label; } Button;

#define NBTN 3
enum { B_PLAY, B_LOAD, B_EXIT };
static Button btn[NBTN] = { { 0, 0, 0, 0, "PLAY" }, { 0, 0, 0, 0, "LOAD" }, { 0, 0, 0, 0, "EXIT" } };
static int    load_on;                      /* there is a save: LOAD works (otherwise it is dim and ignored) */
static int    W, H, title_cell, title_x, title_y, label_cell;
static float  u;                    /* px per "design unit" */
static int    pressed = -1;         /* button index under the finger since touch-down */
static int    inside;               /* finger still over that button */
static float  fade;                 /* seconds since PLAY was pressed; < 0 = not pressed yet */

static int hit(int x, int y) {
    for (int i = 0; i < NBTN; i++)
        if (!(i == B_LOAD && !load_on) && x >= btn[i].x && x < btn[i].x + btn[i].w &&
            y >= btn[i].y && y < btn[i].y + btn[i].h) return i;
    return -1;
}

void menu_set_load_enabled(int on) { load_on = on != 0; }

static void menu_layout(void) {
    int bw = (int)(200 * u), bh = (int)(52 * u), gap = (int)(16 * u);
    int cy = (int)(H * 0.62f);
    int top = cy - (NBTN * bh + (NBTN - 1) * gap) / 2;          /* the stack is centred on cy */
    for (int i = 0; i < NBTN; i++) {
        btn[i].w = bw; btn[i].h = bh;
        btn[i].x = (W - bw) / 2;
        btn[i].y = top + i * (bh + gap);
    }
    label_cell = (int)(3 * u); if (label_cell < 1) label_cell = 1;
    title_cell = (int)(7 * u); if (title_cell < 2) title_cell = 2;
    title_x = (W - font_width("VESSEL", title_cell)) / 2;
    title_y = (int)(H * 0.22f) - font_height(title_cell) / 2;
}

void menu_resize(int w, int h) {
    W = w; H = h;
    u = (w < h ? w : h) / 360.0f;
    space_resize(w, h);                         /* the stars keep falling, the fade and the pressed button stay */
    menu_layout();
}

void menu_init(int w, int h) {
    pressed = -1; inside = 0; fade = -1;
    space_init(w, h);                           /* the stars behind the menu */
    W = w; H = h;
    u = (w < h ? w : h) / 360.0f;
    menu_layout();
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
        if (ok) return p == B_PLAY ? MENU_PLAY : p == B_LOAD ? MENU_LOAD : MENU_EXIT;
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
    int A0 = fade < 0 ? 255 : (int)(255 * (1.0f - fade / MENU_FADE_T));  /* the title and the buttons fade out after PLAY */
    if (A0 <= 0) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(r, 255, 255, 255, A0);
    font_draw(r, "VESSEL", title_x, title_y, title_cell);

    int bt = (int)(2 * u); if (bt < 1) bt = 1;
    for (int i = 0; i < NBTN; i++) {
        const Button *b = &btn[i];
        int A = i == B_LOAD && !load_on ? A0 * 2 / 5 : A0;       /* no save yet: LOAD is dim */
        int down = pressed == i && inside;
        SDL_Rect outer = { b->x, b->y, b->w, b->h };
        SDL_Rect inner = { b->x + bt, b->y + bt, b->w - 2 * bt, b->h - 2 * bt };
        SDL_SetRenderDrawColor(r, 255, 255, 255, A);              /* the border as four bars, not a white fill with the black on top of it:\n                                                                    * blended twice they turned whitish while fading out */
        SDL_Rect bars[4] = { { outer.x, outer.y, outer.w, bt }, { outer.x, outer.y + outer.h - bt, outer.w, bt },
                             { outer.x, outer.y + bt, bt, outer.h - 2 * bt }, { outer.x + outer.w - bt, outer.y + bt, bt, outer.h - 2 * bt } };
        for (int k = 0; k < 4; k++) SDL_RenderFillRect(r, &bars[k]);
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
