/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "menu.h"
#include "font.h"
#include <math.h>
#include <stdint.h>

#define NSNOW   170
#define TWO_PI  6.2831853f
#define TITLE   "VESSEL"
#define TITLE_N 6
#define FLASH_TICKS    8        /* random-color ticks, then one more tick back to white */
#define FLASH_PERIOD   500      /* ms between ticks */

typedef struct { float x, y, vx, vy, z, ph, fr; } Flake;
typedef struct { int x, y, w, h; const char *label; } Button;

static Flake  fl[NSNOW];
static Button btn[2] = { { 0, 0, 0, 0, "PLAY" }, { 0, 0, 0, 0, "EXIT" } };
static int    W, H, title_cell, title_x, title_y, label_cell;
static float  u, t;                 /* u: px per "design unit", t: sim time (s) */
static uint8_t tc[TITLE_N][3];      /* per-letter title color */
static int    flash_on, flash_next;  /* flash_next: index of the next tick to fire (0..FLASH_TICKS) */
static Uint32 flash_t0;
static int    pressed = -1;         /* button index under the finger since touch-down */
static int    inside;               /* finger still over that button */

static uint32_t rng = 2463534242u;
static float frand(void) {          /* xorshift32 -> [0,1) */
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng & 0xFFFFFF) / 16777216.0f;
}

/* leftward wind speed (px/s): steady breeze + two slow sine gusts, never reverses */
static float wind(float time) {
    float g = 70.0f
            + 40.0f * sinf(TWO_PI * time / 7.0f)
            + 18.0f * sinf(TWO_PI * time / 3.1f + 1.7f);
    return -g * u;
}

/* fully saturated random hue: always vivid on black, never white */
static void rand_color(uint8_t *c) {
    float x = frand() * 6.0f, f = x - (int)x, q = 1.0f - f, R, G, B;
    switch ((int)x % 6) {
    case 0:  R = 1; G = f; B = 0; break;
    case 1:  R = q; G = 1; B = 0; break;
    case 2:  R = 0; G = 1; B = f; break;
    case 3:  R = 0; G = q; B = 1; break;
    case 4:  R = f; G = 0; B = 1; break;
    default: R = 1; G = 0; B = q; break;
    }
    c[0] = (uint8_t)(R * 255); c[1] = (uint8_t)(G * 255); c[2] = (uint8_t)(B * 255);
}

static void flash_tick(int k) {
    for (int i = 0; i < TITLE_N; i++) {
        if (k < FLASH_TICKS) rand_color(tc[i]);
        else tc[i][0] = tc[i][1] = tc[i][2] = 255;      /* final tick: all white */
    }
}

/* fire every tick whose time has come (wall clock, so a slow frame can't stretch the 0.5s spacing) */
static void flash_step(void) {
    while (flash_on && SDL_GetTicks() - flash_t0 >= (Uint32)flash_next * FLASH_PERIOD) {
        flash_tick(flash_next++);
        if (flash_next > FLASH_TICKS) flash_on = 0;
    }
}

void menu_title_flash(void) {
    flash_on = 1; flash_next = 0; flash_t0 = SDL_GetTicks();
    flash_step();                                       /* tick 0 fires immediately */
}

static int flake_size(const Flake *f) {
    int s = (int)(u * (1.0f + 3.2f * f->z) + 0.5f);
    return s < 1 ? 1 : s;
}

static int hit(int x, int y) {
    for (int i = 0; i < 2; i++)
        if (x >= btn[i].x && x < btn[i].x + btn[i].w &&
            y >= btn[i].y && y < btn[i].y + btn[i].h) return i;
    return -1;
}

void menu_init(int w, int h) {
    W = w; H = h;
    u = (w < h ? w : h) / 360.0f;
    t = 0; pressed = -1; inside = 0;
    flash_on = 0;
    for (int i = 0; i < TITLE_N; i++) tc[i][0] = tc[i][1] = tc[i][2] = 255;

    for (int i = 0; i < NSNOW; i++) {
        float r = frand();
        Flake *f = &fl[i];
        f->z  = 0.2f + 0.8f * r * r;            /* many far/small, few near/big */
        f->x  = frand() * W;
        f->y  = frand() * H;
        f->ph = frand() * TWO_PI;
        f->fr = 0.8f + 1.2f * frand();
        f->vx = wind(0) * f->z;
        f->vy = (30.0f + 60.0f * f->z) * u;
    }

    int bw = (int)(200 * u), bh = (int)(52 * u), gap = (int)(16 * u);
    int cy = (int)(H * 0.62f);
    for (int i = 0; i < 2; i++) {
        btn[i].w = bw; btn[i].h = bh;
        btn[i].x = (W - bw) / 2;
        btn[i].y = i == 0 ? cy - bh - gap / 2 : cy + gap / 2;
    }
    label_cell = (int)(3 * u); if (label_cell < 1) label_cell = 1;
    title_cell = (int)(7 * u); if (title_cell < 2) title_cell = 2;
    title_x = (W - font_width(TITLE, title_cell)) / 2;
    title_y = (int)(H * 0.22f) - font_height(title_cell) / 2;
}

MenuAction menu_touch(int a, int x, int y) {
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

void menu_update(float dt) {
    flash_step();
    t += dt;
    float w = wind(t);
    for (int i = 0; i < NSNOW; i++) {
        Flake *f = &fl[i];
        float z = f->z;

        /* target velocity: wind scaled by depth (parallax) + lateral sway; terminal fall speed + bob */
        float tvx = w * (0.35f + 0.65f * z)
                  + sinf(t * f->fr + f->ph) * 14.0f * u * (1.2f - z);
        float tvy = (30.0f + 60.0f * z) * u
                  + cosf(t * f->fr * 0.7f + f->ph) * 6.0f * u;

        /* first-order drag: v += (target - v) * (1 - e^(-k dt)); light flakes react faster */
        float k = 1.0f - expf(-dt * (3.2f - 2.2f * z));
        f->vx += (tvx - f->vx) * k;
        f->vy += (tvy - f->vy) * k;
        f->x  += f->vx * dt;
        f->y  += f->vy * dt;

        int s = flake_size(f);
        if (f->y > H)       { f->y = (float)-s; f->x = frand() * (W + s); }
        if (f->x < -s)      { f->x = W + s * 0.5f; f->y = frand() * H; }
        if (f->x > W + s)   { f->x = (float)-s; }
    }
}

void menu_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    for (int i = 0; i < NSNOW; i++) {
        const Flake *f = &fl[i];
        int g = (int)(60 + 195 * f->z), s = flake_size(f);
        SDL_Rect q = { (int)f->x, (int)f->y, s, s };
        SDL_SetRenderDrawColor(r, g, g, g, 255);
        SDL_RenderFillRect(r, &q);
    }

    for (int i = 0; i < TITLE_N; i++) {
        char ch[2] = { TITLE[i], 0 };
        SDL_SetRenderDrawColor(r, tc[i][0], tc[i][1], tc[i][2], 255);
        font_draw(r, ch, title_x + i * 6 * title_cell, title_y, title_cell);
    }

    int bt = (int)(2 * u); if (bt < 1) bt = 1;
    for (int i = 0; i < 2; i++) {
        const Button *b = &btn[i];
        int down = pressed == i && inside;
        SDL_Rect outer = { b->x, b->y, b->w, b->h };
        SDL_Rect inner = { b->x + bt, b->y + bt, b->w - 2 * bt, b->h - 2 * bt };
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &outer);
        if (down) SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        else      SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderFillRect(r, &inner);

        int tx = b->x + (b->w - font_width(b->label, label_cell)) / 2;
        int ty = b->y + (b->h - font_height(label_cell)) / 2;
        if (down) SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        else      SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        font_draw(r, b->label, tx, ty, label_cell);
    }
}
