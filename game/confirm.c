/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE CONFIRM WINDOW (confirm.h): same look as the inventory's action window (gold frame, dark body, pixel letters), a title, the text wrapped to the
 * width, and two buttons side by side: YES (red) on the right under the thumb's way out, NO on the left. a press must start and end on the same button. */
#include "confirm.h"
#include "audio.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#define LINES 6
static int W, H;
static int on;
static char title[32], msg[160], yes_t[16], no_t[16];
static void (*yes_cb)(void);
static int down = -1;                                                /* the button the finger went down on: 0 NO, 1 YES, -1 none */

static float uu(void) { return (W < H ? W : H) / 360.0f; }
static int bt(void)  { int v = (int)(1.5f * uu()); return v < 1 ? 1 : v; }
static int pad(void) { int v = (int)(4 * uu());    return v < 2 ? 2 : v; }
static int qs(void)  { int v = (int)(2 * uu());    return v < 1 ? 1 : v; }
static int cell_for(const char *s, int room, int max) {
    int c = (int)(2.0f * uu()); if (c < 1) c = 1; if (c > max) c = max;
    while (c > 1 && font_width(s, c) > room) c--;
    return c;
}

/* the layout, worked out from the screen and the words each time it is needed (cheap) */
typedef struct { int x, y, w, h, tc, mc, bc, nl, bh; char ln[LINES][80]; int bx[2], bw; } Lay;
static void layout(Lay *L) {
    int in = bt() + pad();
    L->w = W - 2 * (int)(14 * uu()); { int cap = (int)(260 * uu()); if (L->w > cap) L->w = cap; } if (L->w < 120) L->w = W - 8;
    int room = L->w - 2 * in;
    L->tc = cell_for(title, room, 3);
    L->mc = (int)uu(); if (L->mc < 1) L->mc = 1; if (L->mc > 2) L->mc = 2;
    L->bc = cell_for("MMMMMMMM", (room - pad()) / 2 - 2 * pad(), 3);
    L->nl = 0; L->ln[0][0] = 0;                                      /* greedy word wrap */
    const char *p = msg;
    while (*p) {
        char word[64]; int wl = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && wl < 63) word[wl++] = *p++;
        word[wl] = 0; if (!wl) break;
        char tr[160];
        if (L->ln[L->nl][0]) snprintf(tr, sizeof tr, "%s %s", L->ln[L->nl], word); else snprintf(tr, sizeof tr, "%s", word);
        if (L->ln[L->nl][0] && font_width(tr, L->mc) > room) { if (L->nl + 1 >= LINES) break; L->nl++; snprintf(L->ln[L->nl], 80, "%s", word); }
        else snprintf(L->ln[L->nl], 80, "%s", tr);
    }
    if (L->ln[0][0]) L->nl++;
    L->bh = font_height(L->bc) + 2 * pad();
    int th = font_height(L->tc) + 2 * pad();
    int mh = L->nl * (font_height(L->mc) + qs());
    L->h = 2 * in + th + pad() + mh + pad() + L->bh;
    L->x = (W - L->w) / 2; L->y = (H - L->h) / 2;
    L->bw = (room - pad()) / 2;
    L->bx[0] = L->x + in; L->bx[1] = L->x + in + L->bw + pad();
}
static int in_r(int px, int py, int x, int y, int w, int h) { return px >= x && px < x + w && py >= y && py < y + h; }
static int btn_y(const Lay *L) { return L->y + L->h - (bt() + pad()) - L->bh; }
static int hit(int px, int py) {                                     /* 0 NO, 1 YES, -1 none */
    Lay L; layout(&L); int by = btn_y(&L);
    for (int i = 0; i < 2; i++) if (in_r(px, py, L.bx[i], by, L.bw, L.bh)) return i;
    return -1;
}

void confirm_init(int w, int h) { W = w; H = h; on = 0; down = -1; yes_cb = NULL; }
void confirm_resize(int w, int h) { W = w; H = h; down = -1; }

void confirm_ask(const char *t, const char *m, const char *y, const char *n, void (*on_yes)(void)) {
    snprintf(title, sizeof title, "%s", t ? t : "");
    snprintf(msg, sizeof msg, "%s", m ? m : "");
    snprintf(yes_t, sizeof yes_t, "%s", y ? y : "YES");
    snprintf(no_t, sizeof no_t, "%s", n ? n : "NO");
    yes_cb = on_yes; on = 1; down = -1;
}
int  confirm_active(void) { return on; }
void confirm_cancel(void) { on = 0; down = -1; yes_cb = NULL; }

int confirm_touch(int a, int x, int y) {
    if (!on) return 0;
    if (a == 0) { down = hit(x, y); return 1; }
    if (a == 3) { down = -1; return 1; }
    if (a == 1) {
        int d = down; down = -1;
        if (d >= 0 && hit(x, y) == d) {
            sfx_tap();
            void (*cb)(void) = d == 1 ? yes_cb : NULL;
            on = 0; yes_cb = NULL;                                   /* closed first: the callback may open the next window */
            if (cb) cb();
        }
    }
    return 1;                                                        /* modal: every touch is ours while it is open */
}

static void box(SDL_Renderer *r, int x, int y, int w, int h, int br, int bg, int bb, int fr, int fg, int fb, int fa) {
    SDL_Rect o = { x, y, w, h }, in = { x + bt(), y + bt(), w - 2 * bt(), h - 2 * bt() };
    SDL_SetRenderDrawColor(r, br, bg, bb, 255); SDL_RenderFillRect(r, &o);
    SDL_SetRenderDrawColor(r, fr, fg, fb, fa); SDL_RenderFillRect(r, &in);
}
static void txt(SDL_Renderer *r, const char *s, int x, int y, int c, int R, int G, int B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255); font_draw(r, s, x, y, c);
}

void confirm_draw(SDL_Renderer *r) {
    if (!on) return;
    Lay L; layout(&L);
    int in = bt() + pad();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 150); SDL_Rect all = { 0, 0, W, H }; SDL_RenderFillRect(r, &all);       /* the game, dimmed */
    box(r, L.x, L.y, L.w, L.h, 214, 170, 70, 6, 8, 20, 250);
    int ty = L.y + in;
    txt(r, title, L.x + in, ty + pad(), L.tc, 235, 240, 255);
    ty += font_height(L.tc) + 2 * pad() + pad();
    for (int i = 0; i < L.nl; i++) txt(r, L.ln[i], L.x + in, ty + i * (font_height(L.mc) + qs()), L.mc, 178, 190, 222);
    int by = btn_y(&L);
    for (int i = 0; i < 2; i++) {
        const char *t = i ? yes_t : no_t; int tw = font_width(t, L.bc), pressed = down == i;
        if (i) box(r, L.bx[i], by, L.bw, L.bh, pressed ? 255 : 226, pressed ? 120 : 84, pressed ? 130 : 92, pressed ? 90 : 56, 16, 24, 255);        /* YES: red */
        else   box(r, L.bx[i], by, L.bw, L.bh, 236, 190, 78, pressed ? 70 : 40, pressed ? 60 : 34, pressed ? 34 : 20, 255);                         /* NO: gold, like the action buttons */
        if (i) txt(r, t, L.bx[i] + (L.bw - tw) / 2, by + (L.bh - font_height(L.bc)) / 2, L.bc, 255, 214, 218);
        else   txt(r, t, L.bx[i] + (L.bw - tw) / 2, by + (L.bh - font_height(L.bc)) / 2, L.bc, 255, 238, 170);
    }
}

int confirm_yes_x(void) { if (!on) return -1; Lay L; layout(&L); return L.bx[1] + L.bw / 2; }
int confirm_yes_y(void) { if (!on) return -1; Lay L; layout(&L); return btn_y(&L) + L.bh / 2; }
int confirm_no_x(void)  { if (!on) return -1; Lay L; layout(&L); return L.bx[0] + L.bw / 2; }
int confirm_no_y(void)  { if (!on) return -1; Lay L; layout(&L); return btn_y(&L) + L.bh / 2; }

/* a plain button in the look of the NO button (gold frame, dark body), for other screens (the self-stab scene's CONTINUE). alpha 0..255 fades the whole button */
void confirm_button_draw(SDL_Renderer *r, int x, int y, int w, int h, const char *label, int cell, int pressed, int alpha) {
    if (alpha <= 0) return;
    if (alpha > 255) alpha = 255;
    int b = bt();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect fr[4] = { { x, y, w, b }, { x, y + h - b, w, b }, { x, y + b, b, h - 2 * b }, { x + w - b, y + b, b, h - 2 * b } };   /* the frame, then the body inside it (no overlap: it fades evenly) */
    SDL_SetRenderDrawColor(r, 236, 190, 78, alpha);
    for (int i = 0; i < 4; i++) SDL_RenderFillRect(r, &fr[i]);
    SDL_Rect in = { x + b, y + b, w - 2 * b, h - 2 * b };
    SDL_SetRenderDrawColor(r, pressed ? 70 : 40, pressed ? 60 : 34, pressed ? 34 : 20, alpha); SDL_RenderFillRect(r, &in);
    int tw = font_width(label, cell);
    SDL_SetRenderDrawColor(r, 255, 238, 170, alpha); font_draw(r, label, x + (w - tw) / 2, y + (h - font_height(cell)) / 2, cell);
}
