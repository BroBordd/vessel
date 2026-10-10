/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "mapwin.h"
#include "font.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#define ZOOM_STEP  1.5f
#define MAX_UNITS  16.0f                    /* closest zoom: this many screen px per tile per ui unit */

static int W, H, is_open;
static float u, tt;
static void (*source)(MapView *);
static MapView mv;
static SDL_Rect win, hdr, view, zin_r, zout_r, you_r, close_r;
static int hb, help_cell, help_y, nlines;
static char lines[6][128];
static float ppt, min_ppt, max_ppt, ccx, ccy;               /* zoom (px per tile), the tile at the centre of the view */
static int drag, lx, ly, pressed;                           /* pressed: 1 close, 2 zoom in, 3 zoom out, 4 you */

static const char HELP[] = "DRAG TO MOVE THE MAP. TAP THE PLUS AND MINUS BUTTONS TO ZOOM. TAP YOU TO CENTER ON YOURSELF. TAP X TO CLOSE.";

static int inside(SDL_Rect q, int x, int y) { return x >= q.x && y >= q.y && x < q.x + q.w && y < q.y + q.h; }
static void fill(SDL_Renderer *r, SDL_Rect q) { SDL_RenderFillRect(r, &q); }

/* word-wraps HELP to `room` px at `cell`. returns the number of lines (up to 6) */
static int wrap(int cell, int room) {
    int n = 0; char cur[128] = ""; const char *p = HELP;
    while (*p) {
        char word[40]; int k = 0;
        while (*p && *p != ' ' && k < 39) word[k++] = *p++;
        word[k] = 0; while (*p == ' ') p++;
        char tryl[128]; snprintf(tryl, sizeof tryl, cur[0] ? "%s %s" : "%s%s", cur, word);
        if (cur[0] && font_width(tryl, cell) > room) { if (n < 6) { strncpy(lines[n], cur, 127); lines[n][127] = 0; n++; } snprintf(cur, sizeof cur, "%s", word); }
        else snprintf(cur, sizeof cur, "%s", tryl);
    }
    if (cur[0] && n < 6) { strncpy(lines[n], cur, 127); lines[n][127] = 0; n++; }
    return n;
}

static void layout(void) {
    int m = (int)(10 * u); if (m < 4) m = 4;
    win = (SDL_Rect){ m, m, W - 2 * m, H - 2 * m };
    int bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    hb = (int)(24 * u); if (hb < 16) hb = 16;                            /* header height = button size */
    hdr = (SDL_Rect){ win.x + bt, win.y + bt, win.w - 2 * bt, hb };
    close_r = (SDL_Rect){ hdr.x + hdr.w - hb, hdr.y, hb, hb };
    help_cell = (int)(2.2f * u); if (help_cell < 1) help_cell = 1;
    nlines = wrap(help_cell, win.w - 2 * bt - 2 * (int)(6 * u));
    int lh = font_height(help_cell) + help_cell, pad = (int)(6 * u);
    int help_h = nlines * lh + 2 * pad;
    help_y = win.y + win.h - bt - help_h;
    int cy = help_y - hb - pad, gap = (int)(6 * u); if (gap < 2) gap = 2;
    int x = win.x + bt + pad;
    zout_r = (SDL_Rect){ x, cy, hb, hb };                                x += hb + gap;
    zin_r  = (SDL_Rect){ x, cy, hb, hb };                                x += hb + gap;
    you_r  = (SDL_Rect){ x, cy, (int)(font_width("YOU", help_cell + 1)) + 2 * pad, hb };
    view = (SDL_Rect){ win.x + bt, hdr.y + hb + bt, win.w - 2 * bt, cy - pad - (hdr.y + hb + bt) };
}

void mapwin_init(int w, int h) { W = w; H = h; u = (w < h ? w : h) / 360.0f; is_open = drag = pressed = 0; layout(); }
void mapwin_set_source(void (*fn)(MapView *v)) { source = fn; }
int  mapwin_active(void) { return is_open; }
void mapwin_close(void) { is_open = drag = pressed = 0; }

static void clamp_view(void) {
    if (ppt < min_ppt) ppt = min_ppt;
    if (ppt > max_ppt) ppt = max_ppt;
    if (ccx < 0) ccx = 0;
    if (ccx > mv.mw) ccx = (float)mv.mw;
    if (ccy < 0) ccy = 0;
    if (ccy > mv.mh) ccy = (float)mv.mh;
}
static void refresh(void) { memset(&mv, 0, sizeof mv); if (source) source(&mv); }

void mapwin_open(void) {
    layout(); refresh();
    if (mv.mw <= 0 || mv.mh <= 0) return;
    float fx = (float)view.w / mv.mw, fy = (float)view.h / mv.mh;
    min_ppt = fx < fy ? fx : fy; if (min_ppt < 1) min_ppt = 1;
    max_ppt = MAX_UNITS * u; if (max_ppt < min_ppt * 2) max_ppt = min_ppt * 2;
    ppt = min_ppt; ccx = mv.mw * 0.5f; ccy = mv.mh * 0.5f;
    is_open = 1; drag = pressed = 0;
}

static void zoom_by(float k) { ppt *= k; clamp_view(); }
static void center_on_you(void) { ccx = mv.ptx; ccy = mv.pty; if (ppt < min_ppt * 2.5f) ppt = min_ppt * 2.5f > max_ppt ? max_ppt : min_ppt * 2.5f; clamp_view(); }

int mapwin_touch(int a, int x, int y) {
    if (!is_open) return 0;
    if (a == 0) {
        pressed = inside(close_r, x, y) ? 1 : inside(zin_r, x, y) ? 2 : inside(zout_r, x, y) ? 3 : inside(you_r, x, y) ? 4 : 0;
        drag = !pressed && inside(view, x, y); lx = x; ly = y;
    } else if (a == 2 && drag) {
        ccx -= (x - lx) / ppt; ccy -= (y - ly) / ppt; lx = x; ly = y; clamp_view();
    } else if (a == 1) {
        int p = pressed;
        if (p == 1 && inside(close_r, x, y)) mapwin_close();
        else if (p == 2 && inside(zin_r, x, y)) zoom_by(ZOOM_STEP);
        else if (p == 3 && inside(zout_r, x, y)) zoom_by(1.0f / ZOOM_STEP);
        else if (p == 4 && inside(you_r, x, y)) center_on_you();
        pressed = drag = 0;
    } else if (a == 3) pressed = drag = 0;
    return 1;
}

void mapwin_update(float dt) { tt += dt; if (is_open) { refresh(); if (mv.mw <= 0) mapwin_close(); } }

float mapwin_debug_zoom(void) { return ppt; }
void  mapwin_debug_center(float *tx, float *ty) { *tx = ccx; *ty = ccy; }
void  mapwin_debug_rects(SDL_Rect *w, SDL_Rect *v, SDL_Rect *zi, SDL_Rect *zo, SDL_Rect *y, SDL_Rect *c) { *w = win; *v = view; *zi = zin_r; *zo = zout_r; *y = you_r; *c = close_r; }

/* ---------- drawing ---------- */
static void button(SDL_Renderer *r, SDL_Rect q, int down) {
    SDL_SetRenderDrawColor(r, 255, 255, 255, 235); fill(r, q);
    int b = (int)(1.5f * u); if (b < 1) b = 1;
    SDL_Rect in = { q.x + b, q.y + b, q.w - 2 * b, q.h - 2 * b };
    if (down) SDL_SetRenderDrawColor(r, 70, 80, 130, 255); else SDL_SetRenderDrawColor(r, 24, 28, 48, 255);
    fill(r, in);
}
static void glyph_bar(SDL_Renderer *r, SDL_Rect q, int vertical) {      /* the + / - strokes, white */
    int t = q.h / 8; if (t < 2) t = 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_Rect h = { q.x + q.w / 4, q.y + q.h / 2 - t / 2, q.w / 2, t }; fill(r, h);
    if (vertical) { SDL_Rect v = { q.x + q.w / 2 - t / 2, q.y + q.h / 4, t, q.h / 2 }; fill(r, v); }
}
static void text_center(SDL_Renderer *r, const char *s, SDL_Rect q, int cell) {
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    font_draw(r, s, q.x + (q.w - font_width(s, cell)) / 2, q.y + (q.h - font_height(cell)) / 2, cell);
}

static int SX(int i) { return view.x + view.w / 2 + (int)floorf((i - ccx) * ppt); }
static int SY(int j) { return view.y + view.h / 2 + (int)floorf((j - ccy) * ppt); }

void mapwin_draw(SDL_Renderer *r) {
    if (!is_open || mv.mw <= 0) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 170); SDL_Rect all = { 0, 0, W, H }; fill(r, all);          /* the world dims behind the window */
    SDL_SetRenderDrawColor(r, 255, 255, 255, 240); fill(r, win);                                    /* frame */
    SDL_SetRenderDrawColor(r, 10, 12, 28, 252);
    SDL_Rect inner = { win.x + (hdr.x - win.x), win.y + (hdr.y - win.y), hdr.w, win.h - 2 * (hdr.y - win.y) }; fill(r, inner);
    int tcell = (int)(3.0f * u); if (tcell < 2) tcell = 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    font_draw(r, "MAP", hdr.x + (int)(8 * u), hdr.y + (hb - font_height(tcell)) / 2, tcell);
    button(r, close_r, pressed == 1); text_center(r, "X", close_r, tcell);

    /* the map itself, clipped to its viewport, beyond the map edge is darkness */
    SDL_SetRenderDrawColor(r, 4, 5, 12, 255); fill(r, view);
    SDL_RenderSetClipRect(r, &view);
    int tx0 = (int)floorf(ccx - view.w / 2.0f / ppt) - 1, tx1 = (int)ceilf(ccx + view.w / 2.0f / ppt) + 1;
    int ty0 = (int)floorf(ccy - view.h / 2.0f / ppt) - 1, ty1 = (int)ceilf(ccy + view.h / 2.0f / ppt) + 1;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > mv.mw) tx1 = mv.mw;
    if (ty1 > mv.mh) ty1 = mv.mh;
    for (int j = ty0; j < ty1; j++) {
        int i = tx0;
        while (i < tx1) {                                                                           /* runs of one tile are one rectangle */
            int v = mv.tiles[j * mv.stride + i], run = 1;
            while (i + run < tx1 && mv.tiles[j * mv.stride + i + run] == v) run++;
            Rgb c = v < mv.npal ? mv.pal[v] : (Rgb){ 0, 0, 0 };
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
            SDL_Rect q = { SX(i), SY(j), SX(i + run) - SX(i), SY(j + 1) - SY(j) }; fill(r, q);
            i += run;
        }
    }
    int ms = (int)ppt; int mmin = (int)(4 * u); if (ms < mmin) ms = mmin; if (ms > (int)(14 * u)) ms = (int)(14 * u);
    for (int k = 0; k < mv.nmarks; k++) {
        int mx = view.x + view.w / 2 + (int)((mv.marks[k].tx - ccx) * ppt), my = view.y + view.h / 2 + (int)((mv.marks[k].ty - ccy) * ppt);
        int kd = mv.marks[k].kind, ER, EG, EB, CR, CG, CB;
        switch (kd) {
            case 1:  ER = 20; EG = 40; EB = 120; CR = 235; CG = 242; CB = 255; break;         /* the hole */
            case 2:  ER = 30; EG = 10; EB = 60;  CR = 176; CG = 120; CB = 255; break;         /* the shrine */
            case 3:  ER = 20; EG = 22; EB = 28;  CR = 150; CG = 154; CB = 164; break;         /* a grave */
            case 4:  ER = 40; EG = 24; EB = 8;   CR = 232; CG = 164; CB = 72;  break;         /* the hammer */
            default: ER = 20; EG = 14; EB = 0;   CR = 255; CG = 214; CB = 110; break;         /* an npc */
        }
        SDL_Rect e = { mx - ms / 2 - 1, my - ms / 2 - 1, ms + 2, ms + 2 }, c = { mx - ms / 2, my - ms / 2, ms, ms };
        SDL_SetRenderDrawColor(r, ER, EG, EB, 255); fill(r, e);
        SDL_SetRenderDrawColor(r, CR, CG, CB, 255); fill(r, c);
    }
    {   int px = view.x + view.w / 2 + (int)((mv.ptx - ccx) * ppt), py = view.y + view.h / 2 + (int)((mv.pty - ccy) * ppt);
        int d = ms + 2, blink = ((int)(tt * 3.0f) & 1);                                           /* the player: blinking, with a nose */
        SDL_Rect e = { px - d / 2 - 1, py - d / 2 - 1, d + 2, d + 2 }, c = { px - d / 2, py - d / 2, d, d };
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255); fill(r, e);
        SDL_SetRenderDrawColor(r, 255, blink ? 255 : 120, blink ? 255 : 120, 255); fill(r, c);
        int nx = mv.facing == 2 ? -1 : mv.facing == 3 ? 1 : 0, ny = mv.facing == 0 ? 1 : mv.facing == 1 ? -1 : 0, nn = d / 3 + 1;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_Rect n = { px + nx * d - nn / 2, py + ny * d - nn / 2, nn, nn }; fill(r, n);
    }
    SDL_RenderSetClipRect(r, NULL);

    button(r, zout_r, pressed == 3); glyph_bar(r, zout_r, 0);
    button(r, zin_r, pressed == 2);  glyph_bar(r, zin_r, 1);
    button(r, you_r, pressed == 4);  text_center(r, "YOU", you_r, help_cell + 1);
    int lh = font_height(help_cell) + help_cell, pad = (int)(6 * u);
    SDL_SetRenderDrawColor(r, 190, 198, 224, 255);
    for (int k = 0; k < nlines; k++) font_draw(r, lines[k], win.x + (int)(1.5f * u) + pad, help_y + pad + k * lh, help_cell);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
