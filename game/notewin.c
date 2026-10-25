/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE NOTE WINDOW (notewin.h). the page is drawn in "art pixels" of one text cell (tc screen px), so the paper, its torn edge, the stains and the bloody letters are all
 * the same chunky pixel size. everything that looks random comes from a hash of the position (no state, the same page every time). the letters are the game's pixel
 * font drawn one by one with a small wobble, a dark bleed under each, and now and then a drip that runs down from the stroke. */
#include "notewin.h"
#include "audio.h"
#include "confirm.h"
#include "font.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* what the note says: short, and all of it */
static const char *const PARA[] = {
    "DOIA,",
    "I AM AONIA. VESSEL 1. I DIED SO THAT YOU COULD LIVE.",
    "DO NOT BE AFRAID. I AM YOU.",
    "PAST THE FALLEN HALL THE GROUND IS BROKEN. I COULD NOT CROSS IT.",
    "EVERY DEATH TAKES WHAT WE KNEW. THAT IS THE PRICE.",
    "FIND A WAY OUT.",
};
#define NP ((int)(sizeof PARA / sizeof PARA[0]))
#define MAXL 32

static int W, H, on;
static float t;
static void (*close_cb)(void);
static int down;                                                     /* a finger went down on the button */

static unsigned hs(unsigned a, unsigned b) { unsigned h = a * 374761393u + b * 668265263u + 1013904223u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static float uu(void) { return (W < H ? W : H) / 360.0f; }

typedef struct {
    int tc, cpl, nl, gw, gh, px, py, pw, ph;                         /* text cell, chars per line, lines, paper size in art pixels, paper position and size in px */
    char ln[MAXL][72]; int gap[MAXL];                                /* gap[i]: a paragraph starts at line i (extra space above) */
    int bx, by, bw, bh, bc;                                          /* the CLOSE button */
} Lay;

static void wrap(Lay *L, int cpl) {
    L->nl = 0; memset(L->gap, 0, sizeof L->gap);
    for (int p = 0; p < NP; p++) {
        const char *s = PARA[p]; char cur[72] = ""; int first = 1;
        while (*s) {
            char word[72]; int wl = 0;
            while (*s == ' ') s++;
            while (*s && *s != ' ' && wl < 70) word[wl++] = *s++;
            word[wl] = 0; if (!wl) break;
            char tr[160]; if (cur[0]) snprintf(tr, sizeof tr, "%s %s", cur, word); else snprintf(tr, sizeof tr, "%s", word);
            if (cur[0] && (int)strlen(tr) > cpl) {
                if (L->nl < MAXL) { snprintf(L->ln[L->nl], 72, "%s", cur); L->gap[L->nl] = first && p > 0; L->nl++; first = 0; }
                snprintf(cur, sizeof cur, "%s", word);
            } else snprintf(cur, sizeof cur, "%.71s", tr);
        }
        if (cur[0] && L->nl < MAXL) { snprintf(L->ln[L->nl], 72, "%s", cur); L->gap[L->nl] = first && p > 0; L->nl++; }
    }
}
static int text_h(const Lay *L) {                                    /* in art pixels: the lines, the gaps between paragraphs, the margins (more room under the last line: the blood) */
    int h = 6 + 9;
    for (int i = 0; i < L->nl; i++) h += 10 + (L->gap[i] ? 5 : 0);
    return h;
}
static void layout(Lay *L) {
    float u = uu();
    int availW = W - (int)(16 * u), availH = H - (int)(76 * u), cap = (int)(2.4f * u);
    cap = cap < 1 ? 1 : cap > 8 ? 8 : cap;
    for (int tc = cap; tc >= 1; tc--) {
        int pw = (int)(300 * u); if (pw > availW) pw = availW;
        pw = pw / tc * tc;
        int cpl = (pw / tc - 10) / 6;
        if (cpl < 12 && tc > 1) continue;                            /* too narrow for this size: a smaller one */
        wrap(L, cpl);
        int gh = text_h(L);
        L->tc = tc; L->cpl = cpl; L->gw = pw / tc; L->gh = gh; L->pw = pw; L->ph = gh * tc;
        if (L->ph <= availH || tc == 1) break;
    }
    int bc = (int)(2.0f * u); if (bc < 1) bc = 1; if (bc > 3) bc = 3;
    L->bc = bc; L->bw = font_width("CLOSE", bc) + (int)(26 * u); L->bh = font_height(bc) + (int)(14 * u);
    int gapb = (int)(10 * u), total = L->ph + gapb + L->bh;
    L->px = (W - L->pw) / 2; L->py = (H - total) / 2; if (L->py < (int)(4 * u)) L->py = (int)(4 * u);
    L->bx = (W - L->bw) / 2; L->by = L->py + L->ph + gapb; if (L->by + L->bh > H - 2) L->by = H - 2 - L->bh;
}
static int in_btn(const Lay *L, int x, int y) { return x >= L->bx && x < L->bx + L->bw && y >= L->by && y < L->by + L->bh; }

void notewin_init(int w, int h) { W = w; H = h; on = 0; down = 0; close_cb = NULL; t = 0; }
void notewin_resize(int w, int h) { W = w; H = h; down = 0; }
void notewin_open(void (*cb)(void)) { on = 1; down = 0; t = 0; close_cb = cb; }
int  notewin_active(void) { return on; }
void notewin_close(void) {
    if (!on) return;
    void (*cb)(void) = close_cb; on = 0; down = 0; close_cb = NULL;  /* closed first: the callback may open the next window */
    if (cb) cb();
}
int notewin_touch(int a, int x, int y) {
    if (!on) return 0;
    Lay L; layout(&L);
    if (a == 0) { down = in_btn(&L, x, y); if (down) sfx_tap(); }
    else if (a == 3) down = 0;
    else if (a == 1) { int d = down; down = 0; if (d && in_btn(&L, x, y)) notewin_close(); }
    return 1;                                                        /* modal: every touch is ours while it is open */
}
void notewin_update(float dt) { if (on) t += dt; }
int notewin_close_x(void) { if (!on) return -1; Lay L; layout(&L); return L.bx + L.bw / 2; }
int notewin_close_y(void) { if (!on) return -1; Lay L; layout(&L); return L.by + L.bh / 2; }

static void rect(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y, w, h }; SDL_RenderFillRect(r, &q); }
static void colr(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }

/* the paper: a row at a time (the torn left and right edges), ragged top and bottom, shading, fibres, stains, creases */
static void draw_paper(SDL_Renderer *r, const Lay *L) {
    int tc = L->tc, gw = L->gw, gh = L->gh, ox = L->px, oy = L->py;
    for (int y = 0; y < gh; y++) {                                   /* the shadow under the page */
        colr(r, 0, 0, 0, 90); rect(r, ox + tc, oy + (y + 1) * tc, gw * tc, tc);
    }
    for (int y = 0; y < gh; y++) {
        int xl = hs((unsigned)y, 1) % 3 == 0 ? 1 + (int)(hs((unsigned)y, 2) % 2) : (hs((unsigned)y, 3) % 4 == 0 ? 1 : 0);
        int xr = gw - (hs((unsigned)y, 4) % 3 == 0 ? 1 + (int)(hs((unsigned)y, 5) % 2) : (hs((unsigned)y, 6) % 4 == 0 ? 1 : 0));
        float k = 1.0f - 0.12f * (float)y / (float)gh;               /* a little darker towards the bottom */
        int cr = (int)(202 * k), cg = (int)(180 * k), cb = (int)(138 * k);
        if (y < 2 || y >= gh - 2) {                                  /* ragged top and bottom: pieces of three art pixels, some torn away */
            for (int x = xl; x < xr; x += 3) {
                unsigned h = hs((unsigned)(x / 3), (unsigned)(y + 100));
                if (y == 0 && h % 3 == 0) continue;
                if (y == gh - 1 && h % 3 != 0) continue;
                if (y == 1 && h % 7 == 0) continue;
                int w3 = x + 3 > xr ? xr - x : 3;
                colr(r, cr, cg, cb, 255); rect(r, ox + x * tc, oy + y * tc, w3 * tc, tc);
            }
            continue;
        }
        colr(r, cr, cg, cb, 255); rect(r, ox + xl * tc, oy + y * tc, (xr - xl) * tc, tc);
        colr(r, cr * 3 / 4, cg * 3 / 4, cb * 3 / 4, 255);            /* the edge: one darker pixel each side */
        rect(r, ox + xl * tc, oy + y * tc, tc, tc); rect(r, ox + (xr - 1) * tc, oy + y * tc, tc, tc);
    }
    for (int i = 0; i < gw * gh / 36; i++) {                         /* fibres and specks */
        int x = 2 + (int)(hs((unsigned)i, 11) % (unsigned)(gw - 4)), y = 2 + (int)(hs((unsigned)i, 12) % (unsigned)(gh - 4));
        if (hs((unsigned)i, 13) & 1) colr(r, 178, 154, 112, 255); else colr(r, 214, 194, 152, 255);
        rect(r, ox + x * tc, oy + y * tc, tc * (1 + (int)(hs((unsigned)i, 14) % 2)), tc);
    }
    for (int i = 0; i < 4; i++) {                                    /* old stains: brown rings and clouds */
        int w = 6 + (int)(hs((unsigned)i, 21) % 6), h = 4 + (int)(hs((unsigned)i, 22) % 4);
        int x = 2 + (int)(hs((unsigned)i, 23) % (unsigned)(gw - w - 4)), y = 2 + (int)(hs((unsigned)i, 24) % (unsigned)(gh - h - 4));
        colr(r, 160, 128, 84, 70); rect(r, ox + x * tc, oy + y * tc, w * tc, h * tc);
        colr(r, 196, 170, 126, 60); rect(r, ox + (x + 1) * tc, oy + (y + 1) * tc, (w - 2) * tc, (h - 2) * tc);
    }
    {   int cx = gw / 2 + (int)(hs(7, 7) % 3) - 1, cy = gh / 3 + (int)(hs(8, 8) % 3);   /* the folds: a dark line with a light one beside it */
        colr(r, 150, 122, 86, 150); rect(r, ox + cx * tc, oy + 1 * tc, tc, (gh - 2) * tc);
        colr(r, 224, 206, 168, 110); rect(r, ox + (cx + 1) * tc, oy + 1 * tc, tc, (gh - 2) * tc);
        colr(r, 150, 122, 86, 150); rect(r, ox + 1 * tc, oy + cy * tc, (gw - 2) * tc, tc);
        colr(r, 224, 206, 168, 110); rect(r, ox + 1 * tc, oy + (cy + 1) * tc, (gw - 2) * tc, tc);
    }
}

static void draw_text(SDL_Renderer *r, const Lay *L) {
    int tc = L->tc, ox = L->px + 5 * tc, y = L->py + 6 * tc, bleed = tc / 3 > 0 ? tc / 3 : 1;
    for (int li = 0; li < L->nl; li++) {
        if (L->gap[li]) y += 5 * tc;
        int n = (int)strlen(L->ln[li]), last = li == L->nl - 1;
        for (int k = 0; k < n; k++) {
            char c = L->ln[li][k]; if (c == ' ') continue;
            unsigned h = hs((unsigned)li * 131u + 5u, (unsigned)k);
            int jx = ((int)(h % 3) - 1) * (tc > 1 ? tc / 3 : 0);
            int jy = (int)lroundf(sinf((float)k * 0.42f + (float)li * 1.7f) * (float)tc * 0.7f);   /* the line wanders */
            int x = ox + k * 6 * tc + jx, yy = y + jy;
            char s[2] = { c, 0 };
            colr(r, 84, 4, 10, 255);  font_draw(r, s, x + bleed, yy + bleed, tc);                   /* the dark bleed under the letter */
            int shade = (int)(h >> 8) % 3;
            colr(r, 146 + shade * 12, 12 + shade * 4, 22 + shade * 3, 255); font_draw(r, s, x, yy, tc);   /* the blood */
            if (h % 6 == 0) {                                                                         /* a drip from the stroke: a stem and a darker bead */
                int dx = x + tc * (1 + (int)((h >> 4) % 3)), len = tc * (1 + (int)((h >> 12) % 2)), top = yy + 7 * tc;
                colr(r, 140, 10, 20, 255); rect(r, dx, top, tc, len);
                colr(r, 96, 5, 12, 255);   rect(r, dx, top + len, tc, tc);
            }
            if (last && h % 4 == 0) {                                                                 /* under the last line the blood ran further */
                int dx = x + tc * (1 + (int)((h >> 4) % 3)), len = tc * (3 + (int)((h >> 12) % 5)), top = yy + 7 * tc;
                colr(r, 140, 10, 20, 255); rect(r, dx, top, tc, len);
                colr(r, 96, 5, 12, 255);   rect(r, dx, top + len, tc, tc);
            }
        }
        y += 10 * tc;
    }
    {   int cx = L->px + L->pw - 11 * tc, cy = L->py + L->ph - 5 * tc;                              /* a splash in the lower corner, where the hand left the page */
        colr(r, 128, 8, 18, 255); rect(r, cx - 2 * tc, cy - tc, 5 * tc, 3 * tc); rect(r, cx - tc, cy - 2 * tc, 3 * tc, 5 * tc);
        colr(r, 160, 18, 28, 255); rect(r, cx - tc, cy - tc, tc, tc);
        for (int i = 0; i < 14; i++) {
            int dx = (int)(hs((unsigned)i, 31) % 15) - 7, dy = (int)(hs((unsigned)i, 32) % 9) - 4;
            if (i & 1) colr(r, 128, 8, 18, 255); else colr(r, 100, 6, 14, 255);
            rect(r, cx + dx * tc, cy + dy * tc, tc * (1 + (int)(hs((unsigned)i, 33) % 2)), tc);
        }
    }
}

void notewin_draw(SDL_Renderer *r) {
    if (!on) return;
    Lay L; layout(&L);
    float f = t / 0.2f; if (f > 1.0f) f = 1.0f;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    colr(r, 0, 0, 0, (int)(185 * f)); rect(r, 0, 0, W, H);          /* the game, dimmed */
    draw_paper(r, &L);
    draw_text(r, &L);
    confirm_button_draw(r, L.bx, L.by, L.bw, L.bh, "CLOSE", L.bc, down, 255);
}
