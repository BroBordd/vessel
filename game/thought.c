/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "thought.h"
#include "char.h"
#include "font.h"
#include "hud.h"
#include <string.h>

#define T_IN        0.30f           /* slide in */
#define T_OUT       0.35f           /* slide out */
#define MAX_TEXT    120
#define MAX_QUEUE   4
#define MAX_LINES   5

/* the bar is drawn in "cells" of q screen px, the same pixel grid as the ID card and the font.
 * sizes in cells: */
#define PAD         3               /* around everything */
#define PORT        12              /* portrait window (the face is 10 x 10 inside it) */
#define GAP         3               /* between the portrait and the text */
#define LGAP        2               /* between text lines */
#define MIN_W       58              /* narrower than this and there is no room: wait */
#define MAX_W       110

typedef struct { char text[MAX_TEXT]; float secs; } Thought;

static int   W, H, q;
static float u;
static Thought queue[MAX_QUEUE];
static int   nq;
static int   active;                        /* cur is on screen */
static Thought cur;
static float cur_t;
static int   no_room;                       /* set by draw: the top row was too narrow last frame */

void thought_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    q = (int)(1.7f * u); if (q < 2) q = 2;      /* the same pixel size as the ID card */
    nq = 0; active = 0; cur_t = 0; no_room = 0;
}

static void start(const Thought *t) { cur = *t; cur_t = 0; active = 1; }

void thought_say(const char *text, float seconds) {
    if (!text || !*text) return;
    Thought t;
    strncpy(t.text, text, MAX_TEXT - 1); t.text[MAX_TEXT - 1] = 0;
    if (seconds <= 0) {
        seconds = 1.6f + 0.07f * (float)strlen(t.text);     /* time to read it, with a little breathing room */
        if (seconds < 2.6f) seconds = 2.6f;
    }
    t.secs = seconds;
    if (!active) start(&t);
    else if (nq < MAX_QUEUE) queue[nq++] = t;
}

int thought_active(void) { return active; }

void thought_update(float dt) {
    if (!active) return;
    if (no_room) return;                        /* the music card is in the way: hold still until it folds away */
    cur_t += dt;
    if (cur_t < T_IN + cur.secs + T_OUT) return;
    active = 0;
    if (nq > 0) {                               /* next in line */
        Thought next = queue[0];
        for (int i = 1; i < nq; i++) queue[i - 1] = queue[i];
        nq--;
        start(&next);
    }
}

/* ---------- text wrapping ---------- */
typedef struct { int start, len; } Line;

/* breaks `s` into lines of at most maxc letters, on spaces where it can. returns the line count */
static int wrap(const char *s, int maxc, Line *out) {
    int n = 0, i = 0, len = (int)strlen(s);
    while (i < len && n < MAX_LINES) {
        while (i < len && s[i] == ' ') i++;                 /* no leading spaces */
        if (i >= len) break;
        int end = i + maxc;
        if (end >= len) end = len;
        else {
            int k = end;                                    /* look back for a space to break on */
            while (k > i && s[k] != ' ') k--;
            if (k > i) end = k;                             /* else: one long word, break it hard */
        }
        int e = end;
        while (e > i && s[e - 1] == ' ') e--;
        out[n].start = i; out[n].len = e - i; n++;
        i = end;
    }
    return n;
}

static void cell(SDL_Renderer *r, int bx, int by, int cx, int cy, int w, int h) {
    SDL_Rect rc = { bx + cx * q, by + cy * q, w * q, h * q };
    SDL_RenderFillRect(r, &rc);
}

static float ease_out(float k) { k = k < 0 ? 0 : k > 1 ? 1 : k; return 1.0f - (1.0f - k) * (1.0f - k); }

void thought_draw(SDL_Renderer *r, int x0, int x1, int y) {
    if (!active) return;

    int avail = (x1 - x0) / q;                              /* in cells */
    int wc = avail < MAX_W ? avail : MAX_W;
    int text_cells = wc - (2 * PAD + PORT + GAP);
    int maxc = (text_cells + 1) / 6;                        /* letters per line (6 cells each, minus the last gap) */
    if (wc < MIN_W || maxc < 6) { no_room = 1; return; }    /* nothing is drawn, and the timer holds (thought_update) */
    no_room = 0;

    Line ln[MAX_LINES];
    int nl = wrap(cur.text, maxc, ln);
    if (nl == 0) return;
    int th = nl * 7 + (nl - 1) * LGAP;                      /* text block height, cells */
    int hc = (th > PORT ? th : PORT) + 2 * PAD;             /* bar height, cells */
    int bw = wc * q, bh = hc * q;

    /* slide in from above the screen edge, slide back out the same way */
    float pin = ease_out(cur_t / T_IN);
    float pout = cur_t - T_IN - cur.secs; pout = pout > 0 ? pout / T_OUT : 0; if (pout > 1) pout = 1;
    int A = (int)(255 * pin * (1.0f - pout * pout));
    if (A <= 0) return;
    int slide = (int)((1.0f - pin) * (float)(bh + y) + pout * pout * (float)(bh + y));
    int bx = x0 + (x1 - x0 - bw) / 2, by = y - slide;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, A * 90 / 255);                   /* drop shadow */
    cell(r, bx + q, by + q, 0, 0, wc, hc);

    /* a pixel frame with its four corner cells cut off, so the bar looks rounded */
    SDL_SetRenderDrawColor(r, 170, 186, 255, A);
    cell(r, bx, by, 1, 0, wc - 2, 1);  cell(r, bx, by, 1, hc - 1, wc - 2, 1);
    cell(r, bx, by, 0, 1, 1, hc - 2);  cell(r, bx, by, wc - 1, 1, 1, hc - 2);
    SDL_SetRenderDrawColor(r, 14, 18, 40, A * 242 / 255);
    cell(r, bx, by, 1, 1, wc - 2, hc - 2);

    /* the portrait: a small window of dusk sky, so it reads as "inside the head" */
    int py = (hc - PORT) / 2;
    SDL_SetRenderDrawColor(r, 90, 104, 170, A);
    cell(r, bx, by, PAD, py, PORT, PORT);
    for (int row = 0; row < PORT - 2; row++) {
        float k = row / (float)(PORT - 3);
        SDL_SetRenderDrawColor(r, (int)(52 + 40 * k), (int)(66 + 50 * k), (int)(120 + 60 * k), A);
        cell(r, bx, by, PAD + 1, py + 1 + row, PORT - 2, 1);
    }
    const Person *who = hud_person();
    if (who) {
        char_draw_portrait(r, who, bx + (PAD + 1) * q, by + (py + 1) * q, q, 0);
        if (A < 255) {                                                  /* fade the face by dimming it toward the panel */
            SDL_SetRenderDrawColor(r, 14, 18, 40, 255 - A);
            cell(r, bx, by, PAD + 1, py + 1, PORT - 2, PORT - 2);
        }
    }

    /* the thought itself */
    int tx = PAD + PORT + GAP, ty = (hc - th) / 2;
    char buf[MAX_TEXT];
    SDL_SetRenderDrawColor(r, 236, 240, 255, A);
    for (int i = 0; i < nl; i++) {
        memcpy(buf, cur.text + ln[i].start, (size_t)ln[i].len); buf[ln[i].len] = 0;
        font_draw(r, buf, bx + tx * q, by + (ty + i * (7 + LGAP)) * q, q);
    }
}
