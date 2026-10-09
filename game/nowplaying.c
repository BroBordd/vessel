/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "nowplaying.h"
#include "audio.h"
#include "font.h"
#include "musicwin.h"
#include <math.h>
#include <string.h>

#define SHOW_SECONDS 4.5f           /* from the moment it starts sliding in */
#define OUT_SECONDS  0.7f
#define IN_SPEED     7.0f           /* higher = snappier; the approach is exponential so it slows near the end */
#define EQ_ROWS      8

enum { HIDDEN, IN, OUT };

static int   W, H, state;
static float u, x, st;
static unsigned seen;
static int   pdown;                                           /* a touch that started on the card */
static char  title[48];
static float lvl[MUSIC_BANDS], peak[MUSIC_BANDS], hold[MUSIC_BANDS];

/* layout, recomputed when the title changes */
static int cell, q, gap, pad, pw, ph, left, top, eq_w, eq_h;

static void layout(void) {
    cell = (int)(1.7f * u); if (cell < 2) cell = 2;
    q    = (int)(2.4f * u); if (q < 2)    q = 2;                 /* one equalizer pixel */
    gap  = q >= 5 ? 2 : 1;
    pad  = (int)(6 * u);    if (pad < 3)  pad = 3;
    left   = (int)(10 * u);
    top   = (int)(8 * u);
    eq_w = MUSIC_BANDS * q * 2 - q;                               /* 1 pixel wide bars, 1 pixel apart */
    eq_h = EQ_ROWS * q + (EQ_ROWS - 1) * gap;
    int tw = font_width("NOW PLAYING", cell);
    int tt = font_width(title, cell);
    if (tt > tw) tw = tt;
    int inner = tw > eq_w ? tw : eq_w;
    pw = inner + 2 * pad;
    ph = pad + font_height(cell) + cell * 2 + font_height(cell) + pad + eq_h + pad;
}

void nowplaying_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    state = HIDDEN; x = -9999; st = 0; seen = 0;
    title[0] = 0;
    memset(lvl, 0, sizeof lvl); memset(peak, 0, sizeof peak); memset(hold, 0, sizeof hold);
    layout();
}

void nowplaying_update(float dt) {
    unsigned s = music_serial();
    if (s != seen) {                                              /* a track just started */
        seen = s;
        strncpy(title, music_title(), sizeof title - 1); title[sizeof title - 1] = 0;
        layout();
        if (state == HIDDEN) x = (float)-pw;                      /* start fully off the left edge */
        state = IN; st = 0;
    }
    if (state == HIDDEN) return;

    st += dt;
    if (state == IN) {
        x += ((float)left - x) * (1.0f - expf(-dt * IN_SPEED));     /* lerp: fast at first, crawls to a stop */
        if (st >= SHOW_SECONDS) { state = OUT; st = 0; }
    } else {
        float p = st / OUT_SECONDS; if (p > 1) p = 1;
        x = (float)left - (float)(left + pw + 4) * p * p;             /* accelerates away */
        if (p >= 1) { state = HIDDEN; x = -9999; }
    }

    float b[MUSIC_BANDS];
    music_spectrum(b);
    for (int i = 0; i < MUSIC_BANDS; i++) {
        if (b[i] > lvl[i]) lvl[i] += (b[i] - lvl[i]) * (dt * 22.0f > 1 ? 1 : dt * 22.0f);   /* quick attack */
        else               { lvl[i] -= dt * 1.8f; if (lvl[i] < b[i]) lvl[i] = b[i]; }       /* slower fall */
        if (lvl[i] >= peak[i]) { peak[i] = lvl[i]; hold[i] = 0.25f; }
        else if (hold[i] > 0)  hold[i] -= dt;
        else                   { peak[i] -= dt * 0.9f; if (peak[i] < lvl[i]) peak[i] = lvl[i]; }
    }
}

int nowplaying_offset(void) {
    if (state == HIDDEN) return 0;
    float vis = (x + pw) / (float)(left + pw);                      /* 0 = off screen, 1 = in place */
    if (vis < 0) vis = 0;
    if (vis > 1) vis = 1;
    return (int)((ph + top - (int)(6 * u)) * vis);                 /* missions start at 14u, so overlap is only the excess */
}

static void fillr(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect q2 = { x, y, w, h };
    SDL_RenderFillRect(r, &q2);
}

void nowplaying_draw(SDL_Renderer *r) {
    if (state == HIDDEN) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int bx = (int)x, by = top;

    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);                /* frame */
    fillr(r, bx, by, pw, ph);
    SDL_SetRenderDrawColor(r, 10, 12, 24, 235);
    fillr(r, bx + 1, by + 1, pw - 2, ph - 2);

    int tx = bx + pad, ty = by + pad;
    SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
    font_draw(r, "NOW PLAYING", tx, ty, cell);
    ty += font_height(cell) + cell * 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    font_draw(r, title, tx, ty, cell);

    /* equalizer: one column of square pixels per band, bottom up, with a falling peak pixel */
    int ex = bx + pad + 0, ey = by + ph - pad - eq_h;
    for (int i = 0; i < MUSIC_BANDS; i++) {
        int n = (int)(lvl[i] * EQ_ROWS + 0.5f);
        int cx = ex + i * q * 2;
        for (int k = 0; k < n; k++) {
            float f = (float)k / (EQ_ROWS - 1);
            if      (f < 0.5f)  SDL_SetRenderDrawColor(r, 110, 230, 160, 255);
            else if (f < 0.8f)  SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
            else                SDL_SetRenderDrawColor(r, 255, 110, 110, 255);
            fillr(r, cx, ey + eq_h - (k + 1) * q - k * gap, q, q);
        }
        int pk = (int)(peak[i] * EQ_ROWS + 0.5f);
        if (pk > n && pk <= EQ_ROWS && pk > 0) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 230);
            fillr(r, cx, ey + eq_h - pk * q - (pk - 1) * gap, q, q);
        }
        SDL_SetRenderDrawColor(r, 255, 255, 255, 40);              /* faint base so silent bars still read as an equalizer */
        fillr(r, cx, ey + eq_h, q, 1);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static int on_card(int tx, int ty) { return state != HIDDEN && tx >= (int)x && tx < (int)x + pw && ty >= top && ty < top + ph; }

int nowplaying_touch(int a, int tx, int ty) {
    if (a == 0) { if (on_card(tx, ty)) { pdown = 1; return 1; } return 0; }
    if (!pdown) return 0;
    if (a == 1) { pdown = 0; if (on_card(tx, ty)) musicwin_open(); return 1; }
    if (a == 3) pdown = 0;
    return 1;
}
