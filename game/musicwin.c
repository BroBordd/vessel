/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "musicwin.h"
#include "analyze.h"
#include "audio.h"
#include "font.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define BANDS     MUSIC_BANDS
#define EQ_ROWS   16
#define LO_MIDI   36                   /* C2 */

typedef struct { int r, g, b; } C3;
static const C3 COL_BASS = { 80, 170, 255 }, COL_MID = { 110, 230, 160 }, COL_LEAD = { 255, 140, 200 };
static const C3 COL_KICK = { 255, 120, 90 }, COL_SNARE = { 255, 214, 110 }, COL_HAT = { 110, 220, 235 };
static const C3 COL_HELD = { 255, 214, 110 };

static int   W, H, is_open, closing;
static float u, anim, t;
static int   yoff;
static float lvl[BANDS], peak[BANDS], hold[BANDS];
static float nv[AN_NOTES];                              /* smoothed note levels */
static float dr[3];                                     /* drum pad flashes */
static float rg[3];                                     /* smoothed bass / mid / lead */

/* layout */
static int   bt, pad, g, cell, c2;
static int   hi_midi, nwhites, kw, kx0;
static SDL_Rect win, close_r, eq_r, lane_r, hint_r, piano_r;
static int   eq_q, eq_gap, eq_bw, eq_bg;
static int   header_h;

static int   grab;                                      /* what the current touch started on: 0 nothing, 1 piano, 2 close, 3 outside */
static int   held_midi = -1;

static C3 mix(C3 a, C3 b, float k) { return (C3){ (int)(a.r + (b.r - a.r) * k), (int)(a.g + (b.g - a.g) * k), (int)(a.b + (b.b - a.b) * k) }; }
static void col(SDL_Renderer *r, C3 c, int a) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, a); }
static void fillr(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y + yoff, w, h }; SDL_RenderFillRect(r, &q); }
static void box(SDL_Renderer *r, SDL_Rect q, C3 fill, C3 line, int la) {
    col(r, line, la); fillr(r, q.x, q.y, q.w, q.h);
    col(r, fill, 255); fillr(r, q.x + bt, q.y + bt, q.w - 2 * bt, q.h - 2 * bt);
}
static void label_center(SDL_Renderer *r, const char *s, SDL_Rect q, int cl) {
    font_draw(r, s, q.x + (q.w - font_width(s, cl)) / 2, q.y + yoff + (q.h - font_height(cl)) / 2, cl);
}

/* ---------- piano geometry ---------- */
static const int WHITE_IDX[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };   /* pitch class -> white key number in the octave */
static int is_white(int m) { return WHITE_IDX[((m % 12) + 12) % 12] >= 0; }
static int white_index(int m) { int o = (m - LO_MIDI) / 12; return o * 7 + WHITE_IDX[((m % 12) + 12) % 12]; }
static C3  reg_col(int m) { return m < 48 ? COL_BASS : m < 72 ? COL_MID : COL_LEAD; }

static SDL_Rect white_rect(int m) { return (SDL_Rect){ kx0 + white_index(m) * kw, piano_r.y, kw, piano_r.h }; }
static SDL_Rect black_rect(int m) {
    int bw = kw * 62 / 100, bh = piano_r.h * 62 / 100;
    int boundary = kx0 + (white_index(m - 1) + 1) * kw;
    return (SDL_Rect){ boundary - bw / 2, piano_r.y, bw, bh };
}
static int inside(SDL_Rect q, int x, int y) { return x >= q.x && x < q.x + q.w && y >= q.y + yoff && y < q.y + q.h + yoff; }

static int key_at(int x, int y) {
    for (int m = LO_MIDI; m <= hi_midi; m++) if (!is_white(m) && inside(black_rect(m), x, y)) return m;
    for (int m = LO_MIDI; m <= hi_midi; m++) if (is_white(m) && inside(white_rect(m), x, y)) return m;
    return -1;
}
int musicwin_debug_key_at(int x, int y) { return key_at(x, y); }

/* ---------- layout ---------- */
static void layout(void) {
    bt  = (int)(1.5f * u); if (bt < 1) bt = 1;
    pad = (int)(6 * u);    if (pad < 3) pad = 3;
    g   = (int)(3 * u);    if (g < 2) g = 2;
    cell = (int)(1.7f * u); if (cell < 2) cell = 2;
    c2   = (int)(1.3f * u); if (c2 < 2) c2 = 2;
    int m = (int)(8 * u); if (m < 4) m = 4;

    hi_midi = W < H ? 84 : 96;                               /* portrait: 4 octaves, landscape: 5 */
    int top_white = white_index(hi_midi);
    nwhites = top_white + 1;

    header_h = pad + font_height(cell) + cell * 2 + font_height(cell) + pad;
    int lane_h = font_height(c2) + g + (int)(28 * u);
    int hint_h = font_height(c2) + g;
    int piano_h = (int)(80 * u);
    int eq_h = (int)(110 * u);
    int full = header_h + eq_h + lane_h + hint_h + piano_h + 3 * pad;
    int avail = H - 2 * m;
    if (full > avail) { int cut = full - avail; int e = cut < eq_h - (int)(30 * u) ? cut : eq_h - (int)(30 * u); eq_h -= e; cut -= e; piano_h -= cut; full = avail; }
    int wy = (H - full) / 2;
    win = (SDL_Rect){ m, wy, W - 2 * m, full };

    int inner_x = win.x + pad, inner_w = win.w - 2 * pad;
    int y = win.y + header_h;
    close_r = (SDL_Rect){ win.x + win.w - pad - font_height(cell) * 2, win.y + pad, font_height(cell) * 2, font_height(cell) * 2 };

    eq_gap = 1; eq_q = (eq_h - (EQ_ROWS - 1) * eq_gap) / EQ_ROWS; if (eq_q < 2) eq_q = 2;
    eq_h = EQ_ROWS * eq_q + (EQ_ROWS - 1) * eq_gap;
    eq_bg = eq_q / 3; if (eq_bg < 1) eq_bg = 1;
    eq_bw = (inner_w - (BANDS - 1) * eq_bg) / BANDS; if (eq_bw < 1) eq_bw = 1;
    int eq_w = BANDS * eq_bw + (BANDS - 1) * eq_bg;
    eq_r = (SDL_Rect){ inner_x + (inner_w - eq_w) / 2, y, eq_w, eq_h };
    y += eq_h + pad;

    lane_r = (SDL_Rect){ inner_x, y, inner_w, lane_h };
    y += lane_h + pad;
    hint_r = (SDL_Rect){ inner_x, y, inner_w, hint_h };
    y += hint_h;
    piano_r = (SDL_Rect){ inner_x, y, inner_w, piano_h };
    kw = inner_w / nwhites; if (kw < 2) kw = 2;
    kx0 = inner_x + (inner_w - kw * nwhites) / 2;
    /* the real height of the window: everything up to the bottom of the piano plus a pad */
    win.h = piano_r.y + piano_r.h + pad - win.y;
    win.y = (H - win.h) / 2; { int dy = win.y - wy;
        close_r.y += dy; eq_r.y += dy; lane_r.y += dy; hint_r.y += dy; piano_r.y += dy; }
}

void musicwin_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    is_open = closing = 0; anim = 0; t = 0; grab = 0; held_midi = -1;
    memset(lvl, 0, sizeof lvl); memset(peak, 0, sizeof peak); memset(hold, 0, sizeof hold);
    memset(nv, 0, sizeof nv); memset(dr, 0, sizeof dr); memset(rg, 0, sizeof rg);
    layout();
}

void musicwin_open(void) { layout(); is_open = 1; closing = 0; grab = 0; held_midi = -1; t = 0; }
int  musicwin_active(void) { return is_open || closing; }

static void release(void) { if (held_midi >= 0) { sfx_note_off(held_midi); held_midi = -1; } }
static void close_win(void) { release(); is_open = 0; closing = 1; grab = 0; }

int musicwin_touch(int a, int x, int y) {
    if (!is_open) return closing;                          /* sliding away: swallow touches, do nothing */
    if (a == 0) {
        int k = key_at(x, y);
        if (k >= 0) { grab = 1; held_midi = k; sfx_note_on(k); }
        else if (inside(close_r, x, y)) grab = 2;
        else if (!inside(win, x, y)) grab = 3;
        else grab = 0;
        return 1;
    }
    if (a == 2) {
        if (grab == 1) {
            int k = key_at(x, y);
            if (k != held_midi) { release(); if (k >= 0) { held_midi = k; sfx_note_on(k); } }
        }
        return 1;
    }
    if (a == 1 || a == 3) {
        if (grab == 1) release();
        else if (a == 1 && grab == 2 && inside(close_r, x, y)) close_win();
        else if (a == 1 && grab == 3 && !inside(win, x, y)) close_win();
        grab = 0;
        return 1;
    }
    return 1;
}

void musicwin_update(float dt) {
    if (!is_open && !closing) return;
    t += dt;
    float target = is_open ? 1.0f : 0.0f;
    anim += (target - anim) * (1.0f - expf(-14.0f * dt));
    if (fabsf(target - anim) < 0.01f) anim = target;
    if (closing && anim <= 0.0f) closing = 0;
    yoff = (int)((1.0f - anim) * (float)(H - win.y + 4));

    float b[BANDS];
    music_spectrum(b);
    for (int i = 0; i < BANDS; i++) {                                           /* same feel as the card */
        if (b[i] > lvl[i]) lvl[i] += (b[i] - lvl[i]) * (dt * 22.0f > 1 ? 1 : dt * 22.0f);
        else               { lvl[i] -= dt * 1.8f; if (lvl[i] < b[i]) lvl[i] = b[i]; }
        if (lvl[i] >= peak[i]) { peak[i] = lvl[i]; hold[i] = 0.25f; }
        else if (hold[i] > 0)  hold[i] -= dt;
        else                   { peak[i] -= dt * 0.9f; if (peak[i] < lvl[i]) peak[i] = lvl[i]; }
    }
    float nt[AN_NOTES]; an_notes(nt);
    for (int i = 0; i < AN_NOTES; i++) {
        if (nt[i] > nv[i]) nv[i] += (nt[i] - nv[i]) * (dt * 30.0f > 1 ? 1 : dt * 30.0f);
        else               { nv[i] -= dt * 4.5f; if (nv[i] < nt[i]) nv[i] = nt[i]; }
    }
    float k, s, h; an_drums(&k, &s, &h);
    float hits[3] = { k, s, h };
    for (int i = 0; i < 3; i++) { dr[i] -= dt * 5.0f; if (dr[i] < 0) dr[i] = 0; if (hits[i] > dr[i]) dr[i] = hits[i]; }
    float rr[3]; an_registers(&rr[0], &rr[1], &rr[2]);
    for (int i = 0; i < 3; i++) {
        if (rr[i] > rg[i]) rg[i] += (rr[i] - rg[i]) * (dt * 20.0f > 1 ? 1 : dt * 20.0f);
        else               { rg[i] -= dt * 2.5f; if (rg[i] < rr[i]) rg[i] = rr[i]; }
    }
}

/* ---------- drawing ---------- */
static void draw_eq(SDL_Renderer *r) {
    for (int i = 0; i < BANDS; i++) {
        int n = (int)(lvl[i] * EQ_ROWS + 0.5f);
        int cx = eq_r.x + i * (eq_bw + eq_bg);
        for (int k = 0; k < n; k++) {
            float f = (float)k / (EQ_ROWS - 1);
            if      (f < 0.5f) SDL_SetRenderDrawColor(r, 110, 230, 160, 255);
            else if (f < 0.8f) SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
            else               SDL_SetRenderDrawColor(r, 255, 110, 110, 255);
            fillr(r, cx, eq_r.y + eq_r.h - (k + 1) * eq_q - k * eq_gap, eq_bw, eq_q);
        }
        int pk = (int)(peak[i] * EQ_ROWS + 0.5f);
        if (pk > n && pk <= EQ_ROWS && pk > 0) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 230);
            fillr(r, cx, eq_r.y + eq_r.h - pk * eq_q - (pk - 1) * eq_gap, eq_bw, eq_q);
        }
        SDL_SetRenderDrawColor(r, 255, 255, 255, 40);
        fillr(r, cx, eq_r.y + eq_r.h, eq_bw, 1);
    }
}

static void draw_lanes(SDL_Renderer *r) {
    int half = (lane_r.w - g * 3) / 2;
    int lx = lane_r.x, rx = lane_r.x + half + g * 3;
    int ly = lane_r.y + font_height(c2) + g, lh = lane_r.h - font_height(c2) - g;
    SDL_SetRenderDrawColor(r, 130, 138, 170, 255);
    font_draw(r, "DRUMS", lx, lane_r.y + yoff, c2);
    font_draw(r, "INSTRUMENTS", rx, lane_r.y + yoff, c2);

    static const char *DN[3] = { "KICK", "SNARE", "HAT" };
    C3 dc[3] = { COL_KICK, COL_SNARE, COL_HAT };
    int pw = (half - 2 * g) / 3;
    for (int i = 0; i < 3; i++) {
        SDL_Rect q = { lx + i * (pw + g), ly, pw, lh };
        float v = dr[i];
        box(r, q, mix((C3){ 20, 24, 44 }, dc[i], v), mix((C3){ 90, 96, 130 }, dc[i], v), 220);
        if (v > 0.55f) SDL_SetRenderDrawColor(r, 20, 16, 8, 255); else SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        label_center(r, DN[i], q, c2);
    }

    static const char *IN[3] = { "BASS", "MID", "LEAD" };
    C3 ic[3] = { COL_BASS, COL_MID, COL_LEAD };
    int rh = (lh - 2 * g) / 3, lc = rh / 8; if (lc < 1) lc = 1;
    int lw = font_width("LEAD", lc) + g * 2;
    for (int i = 0; i < 3; i++) {
        int y = ly + i * (rh + g);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        font_draw(r, IN[i], rx, y + yoff + (rh - font_height(lc)) / 2, lc);
        SDL_Rect bar = { rx + lw, y, half - lw, rh };
        col(r, (C3){ 90, 96, 130 }, 200); fillr(r, bar.x, bar.y, bar.w, bar.h);
        col(r, (C3){ 14, 18, 36 }, 255);  fillr(r, bar.x + 1, bar.y + 1, bar.w - 2, bar.h - 2);
        int fw = (int)((bar.w - 2) * rg[i]);
        col(r, ic[i], 255); fillr(r, bar.x + 1, bar.y + 1, fw, bar.h - 2);
    }
}

static void draw_piano(SDL_Renderer *r) {
    int cl = (int)(1.0f * u); if (cl < 1) cl = 1;
    int sep = kw >= 6 ? 1 : 0;
    for (int m = LO_MIDI; m <= hi_midi; m++) {                   /* whites first, blacks over them */
        if (!is_white(m)) continue;
        SDL_Rect q = white_rect(m);
        float a = nv[m - LO_MIDI]; a = a > 1 ? 1 : powf(a, 0.7f);
        C3 base = { 236, 238, 246 }, c = mix(base, reg_col(m), a);
        if (m == held_midi) c = COL_HELD;
        col(r, (C3){ 10, 12, 24 }, 255); fillr(r, q.x, q.y, q.w, q.h);
        col(r, c, 255); fillr(r, q.x + sep, q.y, q.w - sep, q.h - sep);
        if (m % 12 == 0 && kw >= 8 * cl) {                        /* name the C keys */
            char s[16]; snprintf(s, sizeof s, "C%d", m / 12 - 1);
            SDL_SetRenderDrawColor(r, 70, 76, 104, 255);
            font_draw(r, s, q.x + (q.w - font_width(s, cl)) / 2, q.y + yoff + q.h - font_height(cl) - cl * 2, cl);
        }
    }
    for (int m = LO_MIDI; m <= hi_midi; m++) {
        if (is_white(m)) continue;
        SDL_Rect q = black_rect(m);
        float a = nv[m - LO_MIDI]; a = a > 1 ? 1 : powf(a, 0.7f);
        C3 c = mix((C3){ 24, 26, 40 }, reg_col(m), a);
        if (m == held_midi) c = COL_HELD;
        col(r, (C3){ 0, 0, 0 }, 255); fillr(r, q.x - 1, q.y, q.w + 2, q.h + 1);
        col(r, c, 255); fillr(r, q.x, q.y, q.w, q.h);
    }
}

void musicwin_draw(SDL_Renderer *r) {
    if (!is_open && !closing) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int dy = yoff;
    SDL_SetRenderDrawColor(r, 0, 0, 0, (int)(150 * anim));              /* dim everything behind */
    SDL_Rect all = { 0, 0, W, H }; SDL_RenderFillRect(r, &all);

    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);                      /* frame */
    fillr(r, win.x, win.y, win.w, win.h);
    SDL_SetRenderDrawColor(r, 10, 12, 24, 245);
    fillr(r, win.x + bt, win.y + bt, win.w - 2 * bt, win.h - 2 * bt);

    int tx = win.x + pad, ty = win.y + pad;
    SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
    font_draw(r, "NOW PLAYING", tx, ty + dy, cell);
    ty += font_height(cell) + cell * 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    font_draw(r, music_title(), tx, ty + dy, cell);

    int cp = grab == 2;                                                  /* close button */
    box(r, close_r, cp ? (C3){ 70, 80, 130 } : (C3){ 24, 28, 48 }, (C3){ 255, 255, 255 }, 170);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    label_center(r, "X", close_r, cell);

    draw_eq(r);
    draw_lanes(r);
    SDL_SetRenderDrawColor(r, 130, 138, 170, 255);
    font_draw(r, "TAP OR SLIDE ON THE KEYS TO PLAY", hint_r.x, hint_r.y + dy, c2);
    draw_piano(r);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
