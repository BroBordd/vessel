/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "nowplaying.h"
#include "audio.h"
#include "font.h"
#include "musicwin.h"
#include <math.h>
#include <string.h>

#define SHOW_SECONDS 4.5f           /* how long the card stays open after a track starts */
#define LERP_SPEED   7.0f           /* exponential: quick at first, eases to a stop */
#define EQ_ROWS      8

static int   W, H, expanded, pdown;
static float u, c, st;              /* c: 0 = open card, 1 = collapsed button (lerped every frame) */
static unsigned seen;
static char  title[48];
static float lvl[MUSIC_BANDS], peak[MUSIC_BANDS], hold[MUSIC_BANDS];

/* layout, recomputed when the title changes */
static int cell, q, gap, pad, pw, ph, left, top, eq_w, eq_h, bs, bgap;

/* the pixel music note, 10 x 10 cells */
static const char *NOTE[10] = {
    "...#######",
    "...#######",
    "...##....#",
    "...#.....#",
    "...#.....#",
    "...#.....#",
    ".###...###",
    "####..####",
    "####..####",
    ".##....##.",
};

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void layout(void) {
    cell = (int)(1.7f * u); if (cell < 2) cell = 2;
    q    = (int)(2.4f * u); if (q < 2)    q = 2;                 /* one equalizer pixel */
    gap  = q >= 5 ? 2 : 1;
    pad  = (int)(6 * u);    if (pad < 3)  pad = 3;
    left = (int)(10 * u);
    bs   = (int)(26 * u);   if (bs < 16) bs = 16;
    top  = (int)(8 * u) + bs / 2;                                 /* + half a button: clears phone notches / cutouts */
    bgap = (int)(5 * u);    if (bgap < 3) bgap = 3;
    eq_w = MUSIC_BANDS * q * 2 - q;                               /* 1 pixel wide bars, 1 pixel apart */
    eq_h = EQ_ROWS * q + (EQ_ROWS - 1) * gap;
    int tw = font_width("NOW PLAYING", cell);
    int tt = font_width(title, cell);
    if (tt > tw) tw = tt;
    int inner = tw > eq_w ? tw : eq_w;
    pw = inner + 2 * pad;
    ph = pad + font_height(cell) + cell * 2 + font_height(cell) + pad + eq_h + pad;
    if (pw < ui_panel_w(W, H)) pw = ui_panel_w(W, H);       /* as wide as the mission list and the task toast */
    if (pw < bs) pw = bs;
    if (ph < bs) ph = bs;
}

void nowplaying_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    expanded = 0; c = 1.0f; st = 0; seen = 0; pdown = 0;
    title[0] = 0;
    memset(lvl, 0, sizeof lvl); memset(peak, 0, sizeof peak); memset(hold, 0, sizeof hold);
    layout();
}

void nowplaying_update(float dt) {
    unsigned s = music_serial();
    if (s != seen) {                                              /* a track just started: open the card */
        seen = s;
        strncpy(title, music_title(), sizeof title - 1); title[sizeof title - 1] = 0;
        layout();
        expanded = 1; st = 0;
    }
    if (expanded) { st += dt; if (st >= SHOW_SECONDS) expanded = 0; }
    float target = expanded ? 0.0f : 1.0f;
    c += (target - c) * (1.0f - expf(-dt * LERP_SPEED));
    if (fabsf(target - c) < 0.002f) c = target;

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

/* current size of the card: bs x bs when collapsed, pw x ph when open */
static int cur_w(void) { return bs + (int)((pw - bs) * (1.0f - c) + 0.5f); }
static int cur_h(void) { return bs + (int)((ph - bs) * (1.0f - c) + 0.5f); }

void nowplaying_rect(int *x, int *y, int *w, int *h) { *x = left; *y = top; *w = cur_w(); *h = cur_h(); }
int  nowplaying_button_size(void) { return bs; }
int  nowplaying_gap(void) { return bgap; }

int nowplaying_offset(void) {
    int off = cur_h() + top + bgap - (int)(14 * u);                /* missions start at 14u on their own */
    return off > 0 ? off : 0;
}

static void fillr(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect q2 = { x, y, w, h };
    SDL_RenderFillRect(r, &q2);
}

void ui_button_frame(SDL_Renderer *r, int x, int y, int w, int h, int pressed, int a) {
    SDL_SetRenderDrawColor(r, 255, 255, 255, a * 200 / 255);      /* frame */
    fillr(r, x, y, w, h);
    if (pressed) SDL_SetRenderDrawColor(r, 38, 46, 84, a * 245 / 255);
    else         SDL_SetRenderDrawColor(r, 10, 12, 24, a * 235 / 255);
    fillr(r, x + 1, y + 1, w - 2, h - 2);
}

static void draw_note(SDL_Renderer *r, int x, int y, int size, int a) {
    int ic = size / 10; if (ic < 1) ic = 1;
    int ox = x + (size - 10 * ic) / 2, oy = y + (size - 10 * ic) / 2;
    SDL_SetRenderDrawColor(r, 255, 214, 110, a);
    for (int j = 0; j < 10; j++)
        for (int i = 0; i < 10; i++)
            if (NOTE[j][i] == '#') fillr(r, ox + i * ic, oy + j * ic, ic, ic);
}

void nowplaying_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int bx = left, by = top, w = cur_w(), h = cur_h();
    ui_button_frame(r, bx, by, w, h, pdown, 255);

    float open = 1.0f - c;
    int ca = clampi((int)((open - 0.45f) / 0.55f * 255.0f), 0, 255);      /* card contents fade in late, out early */
    int ia = clampi((int)((c - 0.45f) / 0.55f * 255.0f), 0, 255);          /* the note fades in as it closes */
    SDL_Rect clip = { bx + 1, by + 1, w - 2, h - 2 };
    SDL_RenderSetClipRect(r, &clip);

    if (ca > 0) {
        int tx = bx + pad, ty = by + pad;
        SDL_SetRenderDrawColor(r, 255, 214, 110, ca);
        font_draw(r, "NOW PLAYING", tx, ty, cell);
        ty += font_height(cell) + cell * 2;
        SDL_SetRenderDrawColor(r, 255, 255, 255, ca);
        font_draw(r, title, tx, ty, cell);

        /* equalizer: one column of square pixels per band, bottom up, with a falling peak pixel */
        int ex = bx + pad, ey = by + ph - pad - eq_h;
        for (int i = 0; i < MUSIC_BANDS; i++) {
            int n = (int)(lvl[i] * EQ_ROWS + 0.5f);
            int cx = ex + i * q * 2;
            for (int k = 0; k < n; k++) {
                float f = (float)k / (EQ_ROWS - 1);
                if      (f < 0.5f)  SDL_SetRenderDrawColor(r, 110, 230, 160, ca);
                else if (f < 0.8f)  SDL_SetRenderDrawColor(r, 255, 214, 110, ca);
                else                SDL_SetRenderDrawColor(r, 255, 110, 110, ca);
                fillr(r, cx, ey + eq_h - (k + 1) * q - k * gap, q, q);
            }
            int pk = (int)(peak[i] * EQ_ROWS + 0.5f);
            if (pk > n && pk <= EQ_ROWS && pk > 0) {
                SDL_SetRenderDrawColor(r, 255, 255, 255, ca * 230 / 255);
                fillr(r, cx, ey + eq_h - pk * q - (pk - 1) * gap, q, q);
            }
            SDL_SetRenderDrawColor(r, 255, 255, 255, ca * 40 / 255);   /* faint base so silent bars still read as an equalizer */
            fillr(r, cx, ey + eq_h, q, 1);
        }
    }
    if (ia > 0) draw_note(r, bx, by, bs, ia);
    SDL_RenderSetClipRect(r, NULL);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static int on_card(int tx, int ty) { return tx >= left && tx < left + cur_w() && ty >= top && ty < top + cur_h(); }

int nowplaying_touch(int a, int tx, int ty) {
    if (a == 0) { if (on_card(tx, ty)) { pdown = 1; return 1; } return 0; }
    if (!pdown) return 0;
    if (a == 1) { pdown = 0; if (on_card(tx, ty)) musicwin_open(); return 1; }
    if (a == 3) pdown = 0;
    return 1;
}
