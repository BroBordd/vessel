/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "dialog.h"
#include "font.h"
#include "talk.h"
#include "audio.h"
#include <ctype.h>
#include <string.h>

#define DIALOG_CPS  28.0f           /* typing speed, characters per second */
#define MAX_WRAP    640

static int   W, H;
static float u;
static int   active, armed;         /* armed: the finger went down while a dialog was open */
static const DialogLine *lines;
static int   count, idx;
static void (*on_done)(void);
static void (*page_hook)(int);      /* see dialog_on_page */
static void (*reply_hook)(int, const char *);   /* see dialog_on_reply */
static const char *const *sugg; static int nsugg;
static int   blipped;               /* how many letters of this page already had their blip */
static int   reply_mode, gen, btn_press;        /* reply_mode: the current page's DialogLine.reply; gen: bumps on every dialog_play */
static SDL_Rect talk_btn, skip_btn;
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
    int btn_h = 7 * L.cell + 4 * L.cell;                 /* TALK / SKIP buttons: text + padding */
    if (reply_mode) tri_h = btn_h + L.pad / 2;
    if (!L.bottom) {
        int tw = widest * 6 * L.cell - L.cell;
        L.bw = tw + 2 * L.pad;
        L.bh = L.pad + nlines * L.lineh + tri_h + L.pad;
        L.bx = (W - L.bw) / 2;
        L.by = (int)(H * 0.36f) - L.bh / 2;
        L.tx = L.bx + L.pad;
        L.ty = L.by + L.pad;
        if (reply_mode && L.bw < 40 * L.cell + 2 * L.pad) { L.bw = 40 * L.cell + 2 * L.pad; L.bx = (W - L.bw) / 2; L.tx = L.bx + L.pad; }
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
    /* the reply buttons sit bottom-right inside the box */
    int right = L.bx + L.bw - L.pad, by = L.by + L.bh - L.pad - btn_h, gap = 3 * L.cell;
    talk_btn = (SDL_Rect){ right - (font_width("TALK", L.cell) + 4 * L.cell), by, font_width("TALK", L.cell) + 4 * L.cell, btn_h };
    int sw = font_width("SKIP", L.cell) + 4 * L.cell + gap + 4 * L.q;
    skip_btn = (SDL_Rect){ talk_btn.x - gap - sw, by, sw, btn_h };
}

static void start_page(void) {
    reply_mode = lines[idx].reply;
    btn_press = 0; blipped = 0;
    layout_base(lines[idx].who != NULL);
    wrap_text(lines[idx].text, L.maxchars);
    layout_box();
    t = 0;
    if (page_hook) page_hook(idx);
}

static int typing_done(void) { return (int)(t * DIALOG_CPS) >= total; }

/* ---------- public API ---------- */
void dialog_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    active = armed = 0; lines = NULL; on_done = NULL; page_hook = NULL; reply_hook = NULL; sugg = NULL; nsugg = 0; reply_mode = 0;
    talk_init(w, h);
}

void dialog_play(const DialogLine *l, int n, void (*done)(void)) {
    if (!l || n <= 0) return;
    lines = l; count = n; idx = 0; on_done = done; gen++;
    active = 1; armed = 0; open_t = 0;
    start_page();
}

int dialog_active(void) { return active; }

void dialog_on_page(void (*fn)(int page)) { page_hook = fn; }
void dialog_on_reply(void (*fn)(int page, const char *text)) { reply_hook = fn; }
void dialog_suggest(const char *const *list, int count) { sugg = list; nsugg = count; }

static void next_page(void) {
    if (++idx >= count) {
        void (*cb)(void) = on_done;
        active = 0; on_done = NULL; page_hook = NULL; reply_hook = NULL; sugg = NULL; nsugg = 0; reply_mode = 0;
        if (cb) cb();                   /* may start another dialog */
    } else start_page();
}

/* the player answered a REPLY page (text == NULL: skipped) */
static void do_reply(const char *text) {
    int g = gen, page = idx;
    char buf[96];
    if (text) { strncpy(buf, text, sizeof buf - 1); buf[sizeof buf - 1] = 0; }
    if (reply_hook) reply_hook(page, text ? buf : NULL);
    if (g != gen) return;                       /* the hook started a new dialog: it carries on from there */
    next_page();
}

static void on_talk_result(const char *text) { if (text) do_reply(text); }   /* NULL = BACK: buttons are shown again */

static int btn_at(int x, int y) {
    if (x >= talk_btn.x && x < talk_btn.x + talk_btn.w && y >= talk_btn.y && y < talk_btn.y + talk_btn.h) return 1;
    if (reply_mode == REPLY_OPTIONAL && x >= skip_btn.x && x < skip_btn.x + skip_btn.w && y >= skip_btn.y && y < skip_btn.y + skip_btn.h) return 2;
    return 0;
}

void dialog_touch(int a, int x, int y) {
    if (!active) return;
    if (talk_active()) { talk_touch(a, x, y); return; }
    if (reply_mode) {                           /* TALK / SKIP buttons, once the text has finished typing */
        if (!typing_done()) return;
        if (a == 0) btn_press = btn_at(x, y);
        else if (a == 2) { if (btn_press && btn_at(x, y) != btn_press) btn_press = 0; }
        else if (a == 3) btn_press = 0;
        else if (a == 1) {
            int b = btn_press && btn_at(x, y) == btn_press ? btn_press : 0;
            btn_press = 0;
            if (b == 1) talk_open(lines[idx].who ? lines[idx].who->name : "", lines[idx].text, sugg, nsugg, reply_mode == REPLY_REQUIRED, on_talk_result);
            else if (b == 2) do_reply(NULL);
        }
        return;
    }
    if (a == 0) armed = 1;
    else if (a == 3) armed = 0;
    else if (a == 1) {
        if (armed && typing_done()) next_page();   /* only once the > is showing */
        armed = 0;
    }
}

void dialog_update(float dt) {
    talk_update(dt);                            /* the keyboard may still be sliding away after the dialog ended */
    if (!active) return;
    open_t += dt; t += dt;
    if (!talk_active() && open_t > 0.1f) {          /* a blip for every letter that just appeared (one voice, so at most one per frame) */
        int upto = (int)(t * DIALOG_CPS); if (upto > total) upto = total;
        int play = -1;
        for (; blipped < upto; blipped++) if (isalnum((unsigned char)wrapped[blipped])) play = blipped;
        if (play >= 0) {
            const char *nm = lines[idx].who ? lines[idx].who->name : "";
            unsigned h = 7; for (; *nm; nm++) h = h * 31 + (unsigned char)*nm;
            float voice = 0.82f + (float)(h % 9) * 0.05f;                         /* every speaker has their own pitch */
            float letter = ((unsigned char)wrapped[play] * 7 % 5) * 0.035f;        /* and the letters wobble around it */
            sfx_blip(voice + letter);
        }
    }
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
    if (!active) { talk_draw(r); return; }
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

    /* reply buttons: TALK (always) and SKIP (when saying nothing is allowed) */
    if (!typing && reply_mode) {
        float since = t - (float)total / DIALOG_CPS;
        int pulse = 170 + (int)(70.0f * ((int)(since * 4.0f) & 1 ? 1.0f : 0.0f));
        for (int k = 0; k < 2; k++) {
            if (k == 1 && reply_mode != REPLY_OPTIONAL) break;
            SDL_Rect b = k == 0 ? talk_btn : skip_btn; int down = btn_press == k + 1;
            SDL_SetRenderDrawColor(r, 255, 255, 255, k == 0 ? pulse : 200); fillr(r, b.x, b.y, b.w, b.h);
            if (down) SDL_SetRenderDrawColor(r, 70, 80, 130, 255); else SDL_SetRenderDrawColor(r, 24, 28, 48, 255);
            fillr(r, b.x + L.bt, b.y + L.bt, b.w - 2 * L.bt, b.h - 2 * L.bt);
            if (k == 0) {
                SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
                font_draw(r, "TALK", b.x + 2 * L.cell, b.y + (b.h - 7 * L.cell) / 2, L.cell);
            } else {
                SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                font_draw(r, "SKIP", b.x + 2 * L.cell, b.y + (b.h - 7 * L.cell) / 2, L.cell);
                if (((int)(since * 2.5f) & 1) == 0) triangle(r, b.x + b.w - 2 * L.cell - 4 * L.q, b.y + (b.h - 7 * L.q) / 2, L.q);
            }
        }
    }
    /* blinking continue arrow, only once the page has finished typing */
    if (!typing && !reply_mode) {
        float since = t - (float)total / DIALOG_CPS;
        if (((int)(since * 2.5f) & 1) == 0) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, A);
            triangle(r, L.bx + L.bw - L.pad - 4 * L.q, L.by + L.bh - L.pad - 7 * L.q, L.q);
        }
    }
    talk_draw(r);
}
