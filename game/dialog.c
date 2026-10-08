/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "dialog.h"
#include "font.h"
#include <string.h>

#define DIALOG_CPS  28.0f           /* typing speed, characters per second */
#define MAX_WRAP    640

static int   W, H;
static float u;
static int   active, armed;         /* armed: the finger went down while a dialog was open */
static const DialogLine *lines;
static int   count, idx;
static void (*on_done)(void);
static float open_t, t;             /* open_t: since the dialog opened, t: since this page began */
static char  wrapped[MAX_WRAP];
static int   nlines, widest, total;

typedef struct {
    int bottom, cell, lineh, pad, bt, q, ps, side, margin, maxchars;
    int bx, by, bw, bh, tx, ty, nx, ny, fx, fy;
} Layout;
static Layout L;

/* ---------- text wrapping ---------- */
static void wrap_text(const char *src, int maxchars) {
    char *o = wrapped, *end = wrapped + MAX_WRAP - 2;
    int col = 0;
    nlines = 1; widest = 0;
    while (*src && o < end) {
        if (*src == '\n') { *o++ = '\n'; if (col > widest) widest = col; col = 0; nlines++; src++; continue; }
        if (*src == ' ')  { src++; continue; }
        const char *q = src;
        while (*q && *q != ' ' && *q != '\n') q++;
        int wl = (int)(q - src);
        if (col > 0 && col + 1 + wl > maxchars) {
            *o++ = '\n'; if (col > widest) widest = col; col = 0; nlines++;
        } else if (col > 0) { *o++ = ' '; col++; }
        if (o + wl >= end) break;
        memcpy(o, src, (size_t)wl); o += wl; col += wl; src = q;
    }
    if (col > widest) widest = col;
    *o = 0;
    total = (int)(o - wrapped);
}

/* ---------- layout ---------- */
static void layout_base(int bottom) {
    L.bottom = bottom;
    L.cell   = (int)(2.4f * u); if (L.cell < 2) L.cell = 2;
    L.lineh  = 10 * L.cell;
    L.pad    = (int)(12 * u);   if (L.pad < 4)  L.pad = 4;
    L.bt     = (int)(2 * u);    if (L.bt < 1)   L.bt = 1;
    L.q      = (L.cell * 2 + 1) / 3; if (L.q < 1) L.q = 1;
    L.margin = (int)(10 * u);
    L.ps     = (int)(4.5f * u); if (L.ps < 2)   L.ps = 2;
    L.side   = 10 * L.ps;
    int avail = bottom ? W - 2 * L.margin - 3 * L.pad - L.side
                       : (int)(W * 0.86f) - 2 * L.pad;
    L.maxchars = avail / (6 * L.cell);
    if (L.maxchars < 8)  L.maxchars = 8;
    if (L.maxchars > 90) L.maxchars = 90;
}

static void layout_box(void) {
    int tri_h = 7 * L.q;
    if (!L.bottom) {
        int tw = widest * 6 * L.cell - L.cell;
        L.bw = tw + 2 * L.pad;
        L.bh = L.pad + nlines * L.lineh + tri_h + L.pad;
        L.bx = (W - L.bw) / 2;
        L.by = (int)(H * 0.36f) - L.bh / 2;
        L.tx = L.bx + L.pad;
        L.ty = L.by + L.pad;
    } else {
        int name_h = 9 * L.cell;
        int text_h = name_h + nlines * L.lineh + tri_h;
        int inner  = text_h > L.side ? text_h : L.side;
        L.bw = W - 2 * L.margin;
        L.bh = inner + 2 * L.pad;
        L.bx = L.margin;
        L.by = H - L.margin - L.bh;
        L.fx = L.bx + L.pad;           L.fy = L.by + L.pad;
        L.nx = L.fx + L.side + L.pad;  L.ny = L.by + L.pad;
        L.tx = L.nx;                   L.ty = L.ny + name_h;
    }
}

static void start_page(void) {
    layout_base(lines[idx].who != NULL);
    wrap_text(lines[idx].text, L.maxchars);
    layout_box();
    t = 0;
}

static int typing_done(void) { return (int)(t * DIALOG_CPS) >= total; }

/* ---------- public API ---------- */
void dialog_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    active = armed = 0; lines = NULL; on_done = NULL;
}

void dialog_play(const DialogLine *l, int n, void (*done)(void)) {
    if (!l || n <= 0) return;
    lines = l; count = n; idx = 0; on_done = done;
    active = 1; armed = 0; open_t = 0;
    start_page();
}

int dialog_active(void) { return active; }

static void next_page(void) {
    if (++idx >= count) {
        void (*cb)(void) = on_done;
        active = 0; on_done = NULL;
        if (cb) cb();                   /* may start another dialog */
    } else start_page();
}

void dialog_touch(int a, int x, int y) {
    (void)x; (void)y;
    if (!active) return;
    if (a == 0) armed = 1;
    else if (a == 3) armed = 0;
    else if (a == 1) {
        if (armed && typing_done()) next_page();   /* only once the > is showing */
        armed = 0;
    }
}

void dialog_update(float dt) {
    if (!active) return;
    open_t += dt; t += dt;
}

/* ---------- drawing ---------- */
static void fillr(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect q = { x, y, w, h };
    SDL_RenderFillRect(r, &q);
}

/* filled triangle pointing right, 4q wide and 7q tall */
static void triangle(SDL_Renderer *r, int x, int y, int q) {
    for (int c = 0; c < 4; c++) {
        int h = (7 - 2 * c) * q;
        fillr(r, x + c * q, y + (7 * q - h) / 2, q, h);
    }
}

void dialog_draw(SDL_Renderer *r) {
    if (!active) return;
    float a = open_t / 0.15f; if (a > 1) a = 1;
    int A = (int)(255 * a);

    if (!L.bottom) {                                   /* dim the world behind a centred window */
        SDL_SetRenderDrawColor(r, 0, 0, 0, (int)(110 * a));
        fillr(r, 0, 0, W, H);
    }

    SDL_SetRenderDrawColor(r, 255, 255, 255, A);
    fillr(r, L.bx, L.by, L.bw, L.bh);
    SDL_SetRenderDrawColor(r, 0, 0, 0, (int)(235 * a));
    fillr(r, L.bx + L.bt, L.by + L.bt, L.bw - 2 * L.bt, L.bh - 2 * L.bt);

    int typing = !typing_done();
    if (L.bottom) {
        const Person *p = lines[idx].who;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);              /* portrait frame */
        fillr(r, L.fx - L.bt, L.fy - L.bt, L.side + 2 * L.bt, L.side + 2 * L.bt);
        SDL_SetRenderDrawColor(r, 34, 40, 66, 255);
        fillr(r, L.fx, L.fy, L.side, L.side);
        char_draw_portrait(r, p, L.fx, L.fy, L.ps, typing && ((int)(t * 10.0f) & 1));
        SDL_SetRenderDrawColor(r, 255, 214, 110, A);
        font_draw(r, p->name, L.nx, L.ny, L.cell);
    }

    /* typewriter text, one wrapped line at a time */
    int remaining = (int)(t * DIALOG_CPS);
    SDL_SetRenderDrawColor(r, 255, 255, 255, A);
    const char *p = wrapped;
    int ly = L.ty;
    while (*p && remaining > 0) {
        const char *e = strchr(p, '\n');
        int len = e ? (int)(e - p) : (int)strlen(p);
        int n = remaining < len ? remaining : len;
        char tmp[96]; if (n > 95) n = 95;
        memcpy(tmp, p, (size_t)n); tmp[n] = 0;
        font_draw(r, tmp, L.tx, ly, L.cell);
        remaining -= len + 1;
        ly += L.lineh;
        p += len;
        if (*p == '\n') p++; else break;
    }

    /* blinking continue arrow, only once the page has finished typing */
    if (!typing) {
        float since = t - (float)total / DIALOG_CPS;
        if (((int)(since * 2.5f) & 1) == 0) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, A);
            triangle(r, L.bx + L.bw - L.pad - 4 * L.q, L.by + L.bh - L.pad - 7 * L.q, L.q);
        }
    }
}
