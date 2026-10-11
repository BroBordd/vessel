/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "brainwin.h"
#include "font.h"
#include "hud.h"
#include "thought.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* the window starts EMPTY. thoughts arrive through brainwin_acquire() when the story says so (story.c) and
 * leave through brainwin_drop_tag() when they stop being true. nothing is preloaded. */

#define MAX_TEXT   96
#define MAX_LINES  5
#define GW 40                       /* the monitor is GW x GH big "pixels" */
#define GH 24
#define FLICKER_T  0.35f            /* the static between two thoughts */
#define TYPE_CPS   42.0f            /* letters per second */

typedef struct { char text[MAX_TEXT]; int scene; int tag; } Thought;
typedef struct { int start, len; } Ln;

static Thought th[BRAIN_MAX_THOUGHTS];
static int   nth;

static int   W, H, is_open, closing;
static float u, anim, tt;           /* anim: window slide 0..1. tt: running clock for the pictures */
static int   cur, auto_on;
static float tm, flick, typed;      /* seconds on this thought, flicker timer, letters typed so far */
static int   fdown;                 /* a finger is down on the window */
static int   grab;                  /* what the finger went down on: 0 nothing, 1 close, 2 auto, 3 outside, 10+i a number */

/* layout */
static int  bt, pad, g, bz, cell, c2, cap, ctrl, bar_h, hdr, yoff, s, cap_lines, cap_h;
static SDL_Rect win, close_r, mon_r, scr_r, bar_r, cap_r, auto_r, pick_r[BRAIN_MAX_THOUGHTS];
static int  ox, oy;                 /* where the monitor's pixel grid starts on screen (slide included) */

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int mini(int a, int b) { return a < b ? a : b; }
static int inside(SDL_Rect q, int x, int y) { return x >= q.x && x < q.x + q.w && y >= q.y && y < q.y + q.h; }
static void fillr(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y, w, h }; SDL_RenderFillRect(r, &q); }
static unsigned hs(int a, int b, int c) {
    unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u + (unsigned)c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static int mix(int a, int b, float k) { return (int)(a + (b - a) * k); }

/* ---------- wrapping ---------- */
static int wrap(const char *str, int maxc, int maxl, Ln *out) {
    int n = 0, len = (int)strlen(str), i = 0;
    if (maxc < 1) maxc = 1;
    while (i < len && n < maxl) {
        while (i < len && str[i] == ' ') i++;
        if (i >= len) break;
        int end = i + maxc;
        if (end >= len) end = len;
        else {
            int k = end;
            while (k > i && str[k] != ' ') k--;                 /* break at the last space that fits */
            if (k > i) end = k;
        }
        int e = end;
        while (e > i && str[e - 1] == ' ') e--;
        out[n].start = i; out[n].len = e - i; n++;
        i = end;
    }
    return n;
}

/* ---------- layout ---------- */
static void layout(void) {
    bt   = (int)(1.5f * u); if (bt < 1) bt = 1;
    pad  = (int)(6 * u);    if (pad < 3) pad = 3;
    g    = (int)(3 * u);    if (g < 2) g = 2;
    bz   = (int)(3 * u);    if (bz < 2) bz = 2;
    cell = (int)(1.7f * u); if (cell < 2) cell = 2;
    c2   = (int)(1.3f * u); if (c2 < 2) c2 = 2;
    cap  = (int)(2.4f * u); if (cap < 2) cap = 2;
    ctrl = (int)(22 * u);   if (ctrl < 16) ctrl = 16;
    bar_h = (int)(3 * u);   if (bar_h < 2) bar_h = 2;
    hdr  = font_height(cell) * 2;
    int m = (int)(8 * u); if (m < 4) m = 4;

    int avail_w = W - 2 * m - 2 * pad - 2 * bz;
    int s_w = avail_w / GW; if (s_w < 1) s_w = 1;
    s = s_w;
    for (int pass = 0; pass < 2; pass++) {                      /* the window is as wide as the monitor, so the caption's width depends on s */
        int winw = GW * s + 2 * bz + 2 * pad;
        int maxc = (winw - 2 * pad + cap) / (6 * cap);
        cap_lines = 3;
        for (int i = 0; i < nth; i++) {
            Ln tmp[MAX_LINES];
            int n = wrap(th[i].text, maxc, MAX_LINES, tmp);
            if (n > cap_lines) cap_lines = n;
        }
        cap_h = cap_lines * 7 * cap + (cap_lines - 1) * cap;
        int fixed = pad + hdr + pad + 2 * bz + g + bar_h + pad + cap_h + pad + ctrl + pad;
        int s_h = (H - 2 * m - fixed) / GH;
        s = mini(s_w, s_h); if (s < 1) s = 1;
    }
    int winw = GW * s + 2 * bz + 2 * pad;
    int winh = pad + hdr + pad + 2 * bz + GH * s + g + bar_h + pad + cap_h + pad + ctrl + pad;
    win = (SDL_Rect){ (W - winw) / 2, (H - winh) / 2, winw, winh };

    int ix = win.x + pad, y = win.y + pad;
    close_r = (SDL_Rect){ win.x + win.w - pad - hdr, y, hdr, hdr };
    y += hdr + pad;
    mon_r = (SDL_Rect){ ix, y, GW * s + 2 * bz, GH * s + 2 * bz };
    scr_r = (SDL_Rect){ ix + bz, y + bz, GW * s, GH * s };
    y += mon_r.h + g;
    bar_r = (SDL_Rect){ ix, y, mon_r.w, bar_h };
    y += bar_h + pad;
    cap_r = (SDL_Rect){ ix, y, mon_r.w, cap_h };
    y += cap_h + pad;

    int aw = font_width("AUTO", c2) + 2 * pad;                  /* the AUTO box sits at the right end, the numbers from the left */
    auto_r = (SDL_Rect){ ix + mon_r.w - aw, y, aw, ctrl };
    int room = auto_r.x - g - ix;
    int bsz = nth > 0 ? (room - (nth - 1) * g) / nth : ctrl;
    bsz = clampi(bsz, font_width("00", c2) + 2 * bt + 2, ctrl);
    for (int i = 0; i < BRAIN_MAX_THOUGHTS; i++)
        pick_r[i] = i < nth ? (SDL_Rect){ ix + i * (bsz + g), y + (ctrl - bsz) / 2, bsz, bsz } : (SDL_Rect){ 0, 0, 0, 0 };
}

/* ---------- state ---------- */
static float dwell(int i) {
    float d = 4.0f + 0.07f * (float)strlen(th[i].text);          /* time to read it and watch the picture */
    return d < 6.0f ? 6.0f : d > 9.0f ? 9.0f : d;
}
static void go(int i) {
    cur = clampi(i, 0, nth > 0 ? nth - 1 : 0);
    tm = 0; flick = FLICKER_T; typed = 0;
}

static int push(const char *text, BrainScene scene, int tag) {
    if (!text || !*text || nth >= BRAIN_MAX_THOUGHTS) return -1;
    strncpy(th[nth].text, text, MAX_TEXT - 1); th[nth].text[MAX_TEXT - 1] = 0;
    th[nth].scene = (int)scene;
    th[nth].tag = tag;
    nth++;
    layout();
    return nth - 1;
}
int brainwin_add(const char *text, BrainScene scene) { return push(text, scene, 0); }

int brainwin_acquire(const char *text, BrainScene scene, int tag) {
    int i = push(text, scene, tag);
    if (i >= 0 && is_open && auto_on) go(i);                /* looking at the window right now, on AUTO: show the new one */
    return i;
}

/* silently forget every thought with this tag (tag 0 is never dropped). returns how many went */
int brainwin_drop_tag(int tag) {
    if (tag == 0) return 0;
    int keep = 0, before_cur = 0, cur_gone = 0;
    for (int i = 0; i < nth; i++) {
        if (th[i].tag == tag) { if (i == cur) cur_gone = 1; else if (i < cur) before_cur++; continue; }
        if (keep != i) th[keep] = th[i];
        keep++;
    }
    int gone = nth - keep;
    if (!gone) return 0;
    nth = keep;
    if (cur_gone) { cur = clampi(cur - before_cur, 0, nth > 0 ? nth - 1 : 0); if (is_open) go(cur); else { tm = 0; typed = 0; } }
    else cur -= before_cur;
    layout();
    return gone;
}
void brainwin_clear(void) { nth = 0; cur = 0; tm = 0; typed = 0; layout(); }
int brainwin_count(void) { return nth; }
int brainwin_debug_current(void) { return cur; }
int brainwin_debug_auto(void) { return auto_on; }
void brainwin_debug_rects(SDL_Rect *w, SDL_Rect *m, SDL_Rect *a, SDL_Rect *c) { *w = win; *m = scr_r; *a = auto_r; *c = close_r; }
SDL_Rect brainwin_debug_pick(int i) { return pick_r[clampi(i, 0, BRAIN_MAX_THOUGHTS - 1)]; }

void brainwin_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    nth = 0; is_open = closing = 0; anim = 0; tt = 0; grab = 0; fdown = 0;
    cur = 0; auto_on = 1; tm = 0; flick = 0; typed = 0;
    layout();
}

void brainwin_open(void) {
    if (is_open) return;
    is_open = 1; closing = 0; auto_on = 1;
    go(nth - 1);                                            /* the newest thought first (the last in the index) */
}
int brainwin_active(void) { return is_open || closing; }

int brainwin_touch(int a, int x, int y) {
    if (!is_open) return closing;                           /* sliding away: swallow touches, do nothing */
    fdown = (a == 0 || a == 2);
    if (a == 0) {
        grab = 0;
        if (inside(close_r, x, y)) grab = 1;
        else if (inside(auto_r, x, y)) grab = 2;
        else if (!inside(win, x, y)) grab = 3;
        else for (int i = 0; i < nth; i++) if (inside(pick_r[i], x, y)) { grab = 10 + i; break; }
        return 1;
    }
    if (a == 1 || a == 3) {
        int gk = grab; grab = 0;
        if (a != 1) return 1;
        if (gk == 1 && inside(close_r, x, y))  { is_open = 0; closing = 1; }
        else if (gk == 3 && !inside(win, x, y)) { is_open = 0; closing = 1; }
        else if (gk == 2 && inside(auto_r, x, y)) { if (auto_on) auto_on = 0; else { auto_on = 1; tm = 0; } }   /* a toggle */
        else if (gk >= 10 && gk - 10 < nth && inside(pick_r[gk - 10], x, y)) {
            if (gk - 10 != cur) { auto_on = 0; go(gk - 10); }      /* the thought on the monitor already: nothing, it must not replay */
        }
    }
    return 1;
}

void brainwin_close(void) { if (is_open) { is_open = 0; closing = 1; } }

void brainwin_update(float dt) {
    tt += dt;
    float target = is_open ? 1.0f : 0.0f;
    anim += (target - anim) * (1.0f - expf(-14.0f * dt));
    if (fabsf(target - anim) < 0.01f) anim = target;
    if (closing && anim <= 0.0f) closing = 0;
    yoff = (int)((1.0f - anim) * (float)(H - win.y + 4));
    if (!is_open || nth == 0) return;

    if (flick > 0) { flick -= dt; if (flick < 0) flick = 0; }
    typed += dt * TYPE_CPS * (fdown ? 2.0f : 1.0f);      /* a finger on the window: letters come twice as fast */
    tm += dt;
    if (auto_on && nth > 1 && tm >= dwell(cur)) go((cur + 1) % nth);
}

/* ---------- the monitor's pictures: all drawn on a GW x GH grid of big pixels ---------- */
static void px(SDL_Renderer *r, int gx, int gy, int w, int h, int R, int G, int B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    fillr(r, ox + gx * s, oy + gy * s, w * s, h * s);
}
static void gradient(SDL_Renderer *r, int r0, int g0, int b0, int r1, int g1, int b1) {
    for (int gy = 0; gy < GH; gy++) {
        float k = gy / (float)(GH - 1);
        px(r, 0, gy, GW, 1, mix(r0, r1, k), mix(g0, g1, k), mix(b0, b1, k));
    }
}

/* 1. the orb: a bobbing "?" and a vague item that keeps changing shape */
static float shape_d(int k, float x, float y) {
    switch (k & 3) {
    case 0:  return sqrtf(x * x + y * y) - 5.4f;                                   /* ball */
    case 1:  return fmaxf(fabsf(x), fabsf(y)) - 4.6f;                              /* box */
    case 2:  return fabsf(x) + fabsf(y) - 6.6f;                                    /* gem */
    default: return sqrtf(x * x + y * y) - (4.4f + 1.5f * cosf(5.0f * atan2f(y, x)));   /* flower */
    }
}
static void scene_orb(SDL_Renderer *r, float t) {
    static const char *QM[11] = {
        "..####..", ".######.", "###..###", "###..###", "....###.", "...###..",
        "..###...", "..###...", "........", "..###...", "..###...",
    };
    gradient(r, 14, 12, 40, 36, 24, 74);
    for (int i = 0; i < 16; i++) {                                                  /* twinkling stars */
        int gx = (int)(hs(i, 1, 0) % GW), gy = (int)(hs(i, 2, 0) % GH);
        int tw = ((int)(t * 1.5f) + i) % 3;
        if (tw == 0) px(r, gx, gy, 1, 1, 210, 214, 255);
        else if (tw == 1) px(r, gx, gy, 1, 1, 96, 100, 160);
    }
    int bob = (int)floorf(sinf(t * 3.0f) * 1.6f + 0.5f);
    float pulse = 0.78f + 0.22f * sinf(t * 5.0f);
    for (int pass = 0; pass < 2; pass++)                                            /* shadow first, then the mark */
        for (int j = 0; j < 11; j++)
            for (int i = 0; i < 8; i++) {
                if (QM[j][i] != '#') continue;
                if (pass == 0) px(r, 13 + i + 1, 4 + j + bob + 1, 1, 1, 120, 80, 20);
                else px(r, 13 + i, 4 + j + bob, 1, 1, (int)(255 * pulse), (int)(214 * pulse), (int)(110 * pulse));
            }
    float phase = t * 0.6f; int k = (int)phase; float f = phase - k;
    f = (f - 0.5f) * 2.0f; f = f < 0 ? 0 : f; f = f * f * (3.0f - 2.0f * f);       /* hold a shape, then morph into the next */
    int tick = (int)(t * 5.0f);
    for (int iy = -8; iy <= 8; iy++)
        for (int ix = -8; ix <= 8; ix++) {
            float d0 = shape_d(k, (float)ix, (float)iy), d1 = shape_d(k + 1, (float)ix, (float)iy);
            float d = d0 + (d1 - d0) * f;
            int n = (int)(hs(ix, iy, tick) % 100);
            if (d < 0) {
                if (d > -1.2f)  px(r, 30 + ix, 12 + iy, 1, 1, 70, 76, 124);        /* dark rim */
                else if (n < 18) px(r, 30 + ix, 12 + iy, 1, 1, 190, 200, 240);     /* fizzing noise: it is not clear what it is */
                else if (n > 88) px(r, 30 + ix, 12 + iy, 1, 1, 84, 92, 146);
                else px(r, 30 + ix, 12 + iy, 1, 1, 118, 128, 176);
            } else if (d < 1.4f && n < 35) px(r, 30 + ix, 12 + iy, 1, 1, 62, 68, 112);   /* dithered edge */
        }
}

/* 2. the map scrolling by */
static void scene_grass(SDL_Renderer *r, float t) {
    static const char *TREE[7] = { ".GGG.", "GgGGG", "GGgGG", "GGGGG", ".GGG.", "..T..", "..T.." };
    static const int TREES[][2] = { { 4, 2 }, { 17, 9 }, { 30, 3 }, { 40, 19 }, { 47, 4 }, { 77, 12 }, { 86, 3 }, { 88, 19 } };
    const int WW = 96;
    int scroll = (int)(t * 3.0f);
    for (int gy = 0; gy < GH; gy++)
        for (int gx = 0; gx < GW; gx++) {
            int wx = (gx + scroll) % WW;
            unsigned hh = hs(wx, gy, 7);
            int R, G, B;
            if (((wx / 2) + (gy / 2)) & 1) { R = 70; G = 150; B = 72; } else { R = 76; G = 162; B = 78; }
            if (hh % 23 == 0 && (((int)(hh >> 8) + (int)(t * 2.0f)) & 1)) { R = 112; G = 192; B = 102; }   /* blades stirring */
            else if (hh % 61 == 0) { R = 240; G = 120; B = 170; }                                     /* a flower */
            float yc = 17.0f + 2.2f * sinf(wx * 0.1309f);
            if (fabsf(gy - yc) < 1.6f) { if (hh % 9 == 0) { R = 200; G = 180; B = 120; } else { R = 224; G = 204; B = 144; } }
            int dx = ((wx - 60 + 48) % WW + WW) % WW - 48;
            float e = (dx / 9.0f) * (dx / 9.0f) + ((gy - 8) / 4.2f) * ((gy - 8) / 4.2f);
            if (e < 1.0f) {
                if ((wx + gy * 2 + (int)(t * 4.0f)) % 7 == 0) { R = 96; G = 150; B = 226; } else { R = 48; G = 98; B = 196; }
            } else if (e < 1.3f) { R = 224; G = 204; B = 144; }
            px(r, gx, gy, 1, 1, R, G, B);
        }
    for (unsigned i = 0; i < sizeof TREES / sizeof TREES[0]; i++) {
        int sx = ((TREES[i][0] - scroll + 8) % WW + WW) % WW - 8, sy = TREES[i][1];
        if (sx >= GW) continue;
        for (int j = 0; j < 7; j++)
            for (int k = 0; k < 5; k++) {
                char c = TREE[j][k];
                if (c == '.') continue;
                if (c == 'G') px(r, sx + k, sy + j, 1, 1, 28, 104, 40);
                else if (c == 'g') px(r, sx + k, sy + j, 1, 1, 60, 140, 60);
                else px(r, sx + k, sy + j, 1, 1, 110, 76, 40);
            }
    }
}

/* 3. the sky they came from */
static void cloud(SDL_Renderer *r, float cx, int cy, float k, int R, int G, int B, int R2, int G2, int B2) {
    static const float BL[3][4] = { { -4, 1, 5, 3 }, { 1, -1, 6, 4 }, { 6, 1, 4, 3 } };
    for (int dy = (int)(-5 * k); dy <= (int)(4 * k); dy++)
        for (int dx = (int)(-10 * k); dx <= (int)(11 * k); dx++) {
            int in = 0;
            for (int b = 0; b < 3 && !in; b++) {
                float a = (dx - BL[b][0] * k) / (BL[b][2] * k), c = (dy - BL[b][1] * k) / (BL[b][3] * k);
                if (a * a + c * c <= 1.0f) in = 1;
            }
            if (!in) continue;
            if (dy > (int)(1 * k)) px(r, (int)cx + dx, cy + dy, 1, 1, R2, G2, B2);
            else px(r, (int)cx + dx, cy + dy, 1, 1, R, G, B);
        }
}
static void scene_clouds(SDL_Renderer *r, float t) {
    gradient(r, 92, 148, 232, 176, 216, 252);
    static const int FAR[][2] = { { 10, 5 }, { 36, 8 }, { 58, 4 } }, NEAR[][2] = { { 6, 16 }, { 32, 19 } };
    for (int i = 0; i < 3; i++) {
        float x = fmodf((float)FAR[i][0] - t * 1.5f + 560.0f, 70.0f) - 14.0f;
        cloud(r, x, FAR[i][1], 0.7f, 206, 224, 252, 182, 206, 244);
    }
    for (int i = 0; i < 2; i++) {
        float x = fmodf((float)NEAR[i][0] - t * 3.5f + 560.0f, 56.0f) - 14.0f;
        cloud(r, x, NEAR[i][1], 1.2f, 252, 253, 255, 214, 226, 250);
    }
}

/* 4. the task coin */
static void scene_coin(SDL_Renderer *r, float t) {
    gradient(r, 12, 22, 38, 24, 48, 60);
    for (int i = 0; i < 12; i++) {
        int gx = 2 + (int)(hs(i, 3, 1) % (GW - 4)), gy = 2 + (int)(hs(i, 4, 1) % (GH - 4));
        float ph = fmodf(t * 1.6f + i * 0.7f, 2.0f);
        if (ph < 0.5f) { px(r, gx, gy - 1, 1, 3, 255, 236, 150); px(r, gx - 1, gy, 3, 1, 255, 236, 150); }
        else if (ph < 0.9f) px(r, gx, gy, 1, 1, 214, 190, 100);
    }
    int cs = (GH * s * 55 / 100) / 7; if (cs < 1) cs = 1;
    int bob = (int)floorf(sinf(t * 2.5f) * 1.2f + 0.5f);
    int cx = ox + (GW * s - 7 * cs) / 2, cy = oy + (GH * s - 7 * cs) / 2 + bob * s;
    int sh = 5 - (bob > 0 ? 1 : 0);                                                  /* shadow under it, shrinking as it floats up */
    px(r, GW / 2 - sh, GH - 5, 2 * sh, 1, 8, 16, 26);
    hud_draw_coin(r, cx, cy, cs, t);
}

/* 5. the grave: a stone at dusk (chunk 23) */
static void scene_grave(SDL_Renderer *r, float t) {
    gradient(r, 34, 30, 74, 150, 104, 128);
    for (int i = 0; i < 14; i++) {                                                  /* the first stars */
        int gx = (int)(hs(i, 5, 2) % GW), gy = (int)(hs(i, 6, 2) % 11);
        if (((int)(t * 1.2f) + i) % 3) px(r, gx, gy, 1, 1, 226, 222, 250);
    }
    for (int dy = -3; dy <= 3; dy++)                                                /* a pale moon */
        for (int dx = -3; dx <= 3; dx++)
            if (dx * dx + dy * dy <= 9) px(r, 33 + dx, 5 + dy, 1, 1, dx + dy < -2 ? 252 : 232, dx + dy < -2 ? 248 : 228, dx + dy < -2 ? 226 : 206);
    for (int gy = 15; gy < GH; gy++)                                                /* the ground */
        for (int gx = 0; gx < GW; gx++) {
            unsigned hh = hs(gx, gy, 9);
            int R = (((gx / 2) + (gy / 2)) & 1) ? 38 : 44, G = (((gx / 2) + (gy / 2)) & 1) ? 84 : 94, B = 56;
            if (hh % 19 == 0 && (((int)(hh >> 8) + (int)(t * 1.5f)) & 1)) { R = 70; G = 130; B = 82; }   /* blades stirring */
            px(r, gx, gy, 1, 1, R, G, B);
        }
    for (int dy = 0; dy < 5; dy++)                                                  /* the mound of earth in front of the stone */
        for (int dx = -9; dx <= 9; dx++) {
            float e = (dx / 9.0f) * (dx / 9.0f) + (dy / 4.6f) * (dy / 4.6f);
            if (e <= 1.0f) px(r, 20 + dx, 15 + dy, 1, 1, dy < 2 ? 112 : 88, dy < 2 ? 80 : 62, dy < 2 ? 56 : 44);
        }
    for (int gy = 5; gy <= 15; gy++) {                                              /* the slab: rounded top, light left edge, dark right edge */
        int x0 = 15, x1 = 24;
        if (gy == 5) { x0 = 17; x1 = 22; } else if (gy == 6) { x0 = 16; x1 = 23; }
        for (int gx = x0; gx <= x1; gx++) {
            int R = 150, G = 152, B = 162;
            if (gx == x0) { R = 192; G = 194; B = 204; }
            else if (gx == x1) { R = 108; G = 110; B = 124; }
            else if (hs(gx, gy, 3) % 11 == 0) { R = 136; G = 138; B = 148; }
            px(r, gx, gy, 1, 1, R, G, B);
        }
    }
    px(r, 18, 9, 4, 1, 92, 94, 108);                                                /* two engraved lines */
    px(r, 17, 11, 6, 1, 92, 94, 108);
    px(r, 19, 13, 2, 1, 118, 120, 134);
    px(r, 15, 15, 10, 1, 70, 72, 84);                                               /* the plinth's shadow */
    int sway = (int)floorf(sinf(t * 1.6f) * 1.2f + 0.5f);                          /* a flower on the mound, swaying */
    px(r, 28, 14, 1, 5, 52, 128, 66);
    px(r, 28 + sway, 12, 1, 1, 244, 140, 182); px(r, 27 + sway, 13, 3, 1, 250, 160, 196); px(r, 28 + sway, 13, 1, 1, 255, 226, 120);
    for (int i = 0; i < 6; i++) {                                                   /* fireflies drifting by */
        float ph = t * 0.35f + i * 0.37f;
        int gx = (int)(fmodf(ph * 9.0f + i * 7.0f, (float)GW)), gy = 10 + (int)(hs(i, 7, 2) % 11) + (int)(sinf(t * 1.3f + i) * 1.6f);
        if (((int)(t * 2.0f) + i) & 1) px(r, gx, gy, 1, 1, 255, 232, 140);
        else px(r, gx, gy, 1, 1, 214, 196, 112);
    }
}

/* ---------- shared bits for the story pictures ---------- */
typedef struct { char c; unsigned char R, G, B; } Pal;
static void pxa(SDL_Renderer *r, int gx, int gy, int w, int h, int R, int G, int B, int A) {
    SDL_SetRenderDrawColor(r, R, G, B, A);
    fillr(r, ox + gx * s, oy + gy * s, w * s, h * s);
}
static void spr(SDL_Renderer *r, int gx, int gy, const char *const *rows, int n, const Pal *pal, int np) {
    for (int j = 0; j < n; j++)
        for (int i = 0; rows[j][i]; i++) {
            char c = rows[j][i];
            if (c == '.') continue;
            for (int k = 0; k < np; k++) if (pal[k].c == c) { px(r, gx + i, gy + j, 1, 1, pal[k].R, pal[k].G, pal[k].B); break; }
        }
}
static void sparkle(SDL_Renderer *r, int gx, int gy, int R, int G, int B, int A) {      /* a little plus */
    pxa(r, gx, gy - 1, 1, 3, R, G, B, A); pxa(r, gx - 1, gy, 3, 1, R, G, B, A);
}
static void ground(SDL_Renderer *r, int y0, int R, int G, int B, float t) {            /* a checkered meadow strip from row y0 down */
    for (int gy = y0; gy < GH; gy++)
        for (int gx = 0; gx < GW; gx++) {
            unsigned hh = hs(gx, gy, 11);
            int chk = (((gx / 2) + (gy / 2)) & 1) ? 6 : 0;
            int rr = R + chk, gg = G + chk * 2, bb = B + chk;
            if (hh % 21 == 0 && (((int)(hh >> 8) + (int)(t * 1.6f)) & 1)) { rr += 26; gg += 34; bb += 20; }   /* blades stirring */
            px(r, gx, gy, 1, 1, rr, gg, bb);
        }
}
/* Alex as a CHIBI: a big head (11 wide), a tiny body, big dark eyes with a white glint, a blush. no hands: a chibi holds nothing, so nothing
 * is ever drawn in her reach. the same colours as her in-game look (hair 236,196,84, teal shirt, plum trousers). 11 x 16. t blinks her eyes */
static void draw_alex(SDL_Renderer *r, int gx, int gy, float t) {
    static const char *A[16] = {
        "...HHHHH...", "..HHHHHHH..", ".HHHHHHHHH.", ".HHHHHHHHH.", ".HHSSSSSHH.", ".HSSSSSSSH.",
        ".HSEwSEwSH.", ".HSEESEESH.", ".HSEESEESH.", ".HKSSSSSKH.", "..HSSSSSH..", "...SSSSS...",
        "..TTTTTTT..", "...TTTTT...", "...PP.PP...", "...BB.BB...",
    };
    static const Pal PA[] = { { 'H', 236, 196, 84 }, { 'S', 244, 200, 164 }, { 'T', 60, 170, 170 }, { 'P', 70, 60, 110 }, { 'B', 40, 30, 30 },
                              { 'E', 40, 30, 30 }, { 'w', 255, 255, 255 }, { 'K', 244, 150, 150 } };
    pxa(r, gx - 1, gy + 16, 13, 1, 0, 0, 0, 60);                                    /* shadow */
    spr(r, gx, gy, A, 16, PA, 8);
    if (fmodf(t, 3.7f) < 0.12f) {                                                   /* a blink: the eyes shut to a line */
        px(r, gx + 3, gy + 6, 2, 2, 244, 200, 164); px(r, gx + 6, gy + 6, 2, 2, 244, 200, 164);
        px(r, gx + 3, gy + 8, 2, 1, 40, 30, 30);    px(r, gx + 6, gy + 8, 2, 1, 40, 30, 30);
    }
}
/* the orb: a violet-gold ball with a glint and a pulsing glow, cx,cy = its centre, rad about 3 */
static void draw_orb_ball(SDL_Renderer *r, int cx, int cy, int rad, float t) {
    float pu = 0.7f + 0.3f * sinf(t * 4.0f);
    for (int dy = -rad - 3; dy <= rad + 3; dy++)
        for (int dx = -rad - 3; dx <= rad + 3; dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy));
            if (d > rad && d <= rad + 3) pxa(r, cx + dx, cy + dy, 1, 1, 200, 170, 255, (int)(60 * pu * (1.0f - (d - rad) / 3.0f)));
        }
    for (int dy = -rad; dy <= rad; dy++)
        for (int dx = -rad; dx <= rad; dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy));
            if (d > rad + 0.3f) continue;
            if (d > rad - 0.9f) px(r, cx + dx, cy + dy, 1, 1, 96, 70, 190);        /* rim */
            else if (dx + dy < -rad / 2) px(r, cx + dx, cy + dy, 1, 1, 200, 180, 255);
            else px(r, cx + dx, cy + dy, 1, 1, 150, 120, 240);
        }
    px(r, cx - rad / 2, cy - rad / 2, 1, 1, 255, 255, 255);                          /* the glint */
}
/* DIA'S SHRINE as a picture (the in-game one, shrine.c, at 1 grid pixel per unit). (cx, fy) = feet centre. p 0..1 = how cracked */
static void shrine_g(SDL_Renderer *r, int cx, int fy, float t, float p) {
#define SB(ux, uy, uw, uh, R, G, B, A) pxa(r, cx + (ux), fy + (uy), (uw), (uh), (R), (G), (B), (A))
    SB(-8, -1, 16, 2, 0, 0, 0, 70);
    SB(-7, -3, 14, 3, 112, 118, 144, 255); SB(-7, -3, 14, 1, 158, 166, 192, 255); SB(-7, -1, 14, 1, 86, 92, 118, 255);
    SB(-5, -5, 10, 2, 134, 140, 166, 255); SB(-5, -5, 10, 1, 170, 178, 204, 255);
    SB(-2, -12, 4, 7, 148, 154, 180, 255); SB(1, -12, 1, 7, 112, 118, 146, 255); SB(-3, -13, 6, 1, 186, 194, 218, 255);
    int gy = -19 + (int)floorf(sinf(t * 2.0f) * 0.7f + 0.5f);
    int gr = mix(110, 100, p), gg = mix(224, 100, p), gb = mix(255, 124, p);
    int glow = (int)((70 + 40 * sinf(t * 3.0f)) * (1.0f - 0.7f * p));
    for (int dy = -4; dy <= 4; dy++)                                                 /* a round glow, not a square one */
        for (int dx = -4; dx <= 4; dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy));
            if (d <= 4.2f && d > 1.5f) SB(dx, gy + 1 + dy, 1, 1, gr, gg, gb, (int)(glow * (1.0f - d / 4.4f) * 0.8f));
        }
    SB(-1, gy, 2, 1, gr, gg, gb, 255); SB(-2, gy + 1, 4, 2, gr, gg, gb, 255); SB(-1, gy + 3, 2, 1, gr, gg, gb, 255);
    SB(-1, gy + 1, 1, 1, 255, 255, 255, (int)(230 * (1.0f - p)));
    SB(-1, -10, 2, 1, gr / 2 + 40, gg / 2 + 40, gb / 2 + 40, 255); SB(-1, -8, 2, 1, gr / 2 + 40, gg / 2 + 40, gb / 2 + 40, 255);
    if (p > 0) {
        if (p > 0.12f) SB(0, -12, 1, 2, 30, 30, 44, 255);
        if (p > 0.25f) SB(-1, -10, 1, 2, 30, 30, 44, 255);
        if (p > 0.38f) SB(0, -8, 1, 2, 30, 30, 44, 255);
        if (p > 0.5f)  { SB(-2, -13, 2, 1, 30, 30, 44, 255); SB(1, -6, 1, 1, 30, 30, 44, 255); }
        if (p > 0.62f) { SB(-1, -6, 1, 2, 30, 30, 44, 255); SB(-3, -5, 2, 1, 30, 30, 44, 255); }
        if (p > 0.74f) { SB(2, -5, 2, 1, 30, 30, 44, 255); SB(4, -4, 1, 2, 30, 30, 44, 255); }
        if (p > 0.85f) { SB(0, gy + 1, 1, 2, 30, 30, 44, 255); SB(-1, gy + 2, 1, 1, 30, 30, 44, 255); }
    }
#undef SB
}

/* 6. "Maybe she has seen the orb." Alex on the grass, a thought bubble of her own with the orb in it (and a "?" in between) */
static void scene_alex(SDL_Renderer *r, float t) {
    gradient(r, 120, 186, 244, 214, 238, 252);
    for (int i = 0; i < 2; i++) cloud(r, fmodf(8.0f + i * 24.0f - t * 1.2f + 560.0f, 56.0f) - 8.0f, 3 + i * 3, 0.7f, 252, 253, 255, 224, 234, 252);
    ground(r, 16, 66, 148, 70, t);
    static const char *TREE[7] = { ".GGG.", "GgGGG", "GGgGG", "GGGGG", ".GGG.", "..T..", "..T.." };
    for (int k = 0; k < 2; k++) {                                                    /* a tree at each side */
        int sx = k ? 33 : 2, sy = 9;
        for (int j = 0; j < 7; j++) for (int i = 0; i < 5; i++) {
            char c = TREE[j][i];
            if (c == '.') continue;
            if (c == 'G') px(r, sx + i, sy + j, 1, 1, 28, 104, 40); else if (c == 'g') px(r, sx + i, sy + j, 1, 1, 60, 140, 60); else px(r, sx + i, sy + j, 1, 1, 110, 76, 40);
        }
    }
    int step = ((int)(t * 1.4f) & 1) ? 0 : 0;
    int bob = (sinf(t * 2.4f) > 0.6f) ? 1 : 0;                                       /* she shifts her weight now and then */
    (void)step;
    draw_alex(r, 10, 4 + bob, t);
    /* her thought bubble: three trail dots, then the bubble with the orb / a question mark */
    px(r, 25, 9, 1, 1, 255, 255, 255); px(r, 27, 7, 2, 2, 255, 255, 255);
    for (int dy = -5; dy <= 5; dy++)
        for (int dx = -9; dx <= 9; dx++) {
            float e = (dx / 9.0f) * (dx / 9.0f) + (dy / 5.0f) * (dy / 5.0f);
            if (e <= 1.0f) px(r, 31 + dx, 4 + dy, 1, 1, e > 0.78f ? 168 : 255, e > 0.78f ? 176 : 255, e > 0.78f ? 214 : 255);
        }
    if (((int)(t / 1.4f)) & 1) {
        static const char *Q[7] = { ".###.", "#...#", "...#.", "..#..", "..#..", ".....", "..#.." };
        float pu = 0.7f + 0.3f * sinf(t * 5.0f);
        for (int j = 0; j < 7; j++) for (int i = 0; i < 5; i++) if (Q[j][i] == '#') px(r, 29 + i, 1 + j, 1, 1, (int)(120 * pu), (int)(90 * pu), (int)(210 * pu));
    } else draw_orb_ball(r, 31, 4, 2, t);
}

/* 7. "Alex has the orb. All I need is a broken shrine." the orb floats beside Alex (chibi: no hands); the shrine beside her is cracking, again and again */
static void scene_alexorb(SDL_Renderer *r, float t) {
    gradient(r, 54, 34, 92, 246, 156, 118);
    for (int i = 0; i < 10; i++) {
        int gx = (int)(hs(i, 8, 3) % GW), gy = (int)(hs(i, 9, 3) % 8);
        if (((int)(t * 1.3f) + i) % 3) px(r, gx, gy, 1, 1, 255, 236, 220);
    }
    ground(r, 21, 40, 82, 56, t);
    float cyc = fmodf(t, 5.0f), p = cyc < 1.0f ? 0.0f : (cyc < 4.0f ? (cyc - 1.0f) / 3.0f : 1.0f);
    int boom = (cyc >= 4.0f && cyc < 4.25f);
    shrine_g(r, 32, 21, t, p);
    int oy2 = (int)floorf(sinf(t * 3.0f) * 0.8f + 0.5f);
    for (int i = 0; i < 6; i++) {                                                    /* rays out of the orb */
        float a = t * 0.7f + i * 1.0472f;
        for (int k = 5; k < 8; k++) pxa(r, 21 + (int)floorf(cosf(a) * k + 0.5f), 5 + oy2 + (int)floorf(sinf(a) * k + 0.5f), 1, 1, 255, 240, 190, 235 - (k - 5) * 70);
    }
    draw_alex(r, 6, 5, t);                                                           /* chibi: she cannot hold it, so it floats by itself */
    draw_orb_ball(r, 21, 5 + oy2, 3, t);
    for (int k = 0; k < 6; k++) {                                                    /* a dotted line from the orb to the shrine: "all I need" */
        int on = (((int)(t * 4.0f)) + k) % 6 < 3;
        if (on) px(r, 25 + k * 2, 7 + k / 2 + oy2 / 2, 1, 1, 255, 236, 170);
    }
    if (boom) { SDL_SetRenderDrawColor(r, 255, 255, 255, 120); fillr(r, ox, oy, GW * s, GH * s); }
}

/* 8. "Breaking a goddess's shrine. I doubt that ends well." a storm gathers over the shrine; lightning every few seconds */
static void scene_shrine(SDL_Renderer *r, float t) {
    gradient(r, 22, 12, 40, 118, 34, 66);
    for (int i = 0; i < 3; i++) cloud(r, fmodf(6.0f + i * 22.0f - t * 2.0f + 560.0f, 66.0f) - 12.0f, 2 + (i & 1) * 3, 1.3f, 58, 36, 76, 32, 20, 52);
    ground(r, 21, 22, 40, 36, t);
    for (int i = 0; i < 28; i++) {                                                   /* rain */
        float yy = fmodf((float)(hs(i, 12, 4) % 1000) * 0.03f + t * 26.0f, (float)(GH + 6)) - 3.0f;
        int xx = (int)(hs(i, 13, 4) % (GW + 8)) - 8 + (int)(yy * 0.25f);
        pxa(r, xx, (int)yy, 1, 2, 160, 176, 226, 140);
    }
    shrine_g(r, 20, 21, t, 0.0f);
    float T = 4.2f, ph = fmodf(t, T);
    int cyc = (int)(t / T);
    if (ph < 0.26f) {                                                                /* lightning */
        float f = ph < 0.07f ? 1.0f : ph < 0.14f ? 0.25f : ph < 0.2f ? 0.8f : 0.2f;
        int bx = 6 + (int)(hs(cyc, 14, 5) % 8) * (cyc & 1 ? 1 : 3) % 28;
        for (int y = 0; y < 15; y++) {
            bx += (int)(hs(cyc, y, 6) % 3) - 1;
            pxa(r, bx - 1, y, 3, 1, 255, 244, 190, (int)(70 * f));
            pxa(r, bx, y, 1, 1, 255, 252, 226, (int)(255 * f));
        }
        SDL_SetRenderDrawColor(r, 255, 244, 230, (int)(80 * f)); fillr(r, ox, oy, GW * s, GH * s);
    }
}

/* 9. "A hammer. ..." the hammer, big, turning a little in the light of a sunburst */
static void scene_hammer(SDL_Renderer *r, float t) {
    for (int gy = 0; gy < GH; gy++)                                                  /* sunburst: rays spinning slowly behind it */
        for (int gx = 0; gx < GW; gx++) {
            float dx = gx + 0.5f - 20.0f, dy = gy + 0.5f - 12.0f;
            float d = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx) + t * 0.18f;
            int ray = (int)floorf(a / (6.2832f / 16.0f)) & 1;
            float k = d / 24.0f; if (k > 1) k = 1;
            float v = ray ? 1.0f : 0.9f;
            px(r, gx, gy, 1, 1, (int)(mix(255, 214, k) * v), (int)(mix(214, 98, k) * v), (int)(mix(128, 96, k) * v));
        }
    for (int i = 0; i < 12; i++) {                                                   /* sparkles around */
        float ph = fmodf(t * 1.1f + i * 0.43f, 2.0f);
        int gx = 2 + (int)(hs(i, 15, 7) % (GW - 4)), gy = 1 + (int)(hs(i, 16, 7) % (GH - 2));
        if (ph < 0.4f) sparkle(r, gx, gy, 255, 250, 220, 230); else if (ph < 0.8f) pxa(r, gx, gy, 1, 1, 255, 244, 200, 200);
    }
    float cy0 = 12.5f + sinf(t * 2.0f) * 0.9f, ang = -0.5f + 0.12f * sinf(t * 2.2f), ca = cosf(ang), sa = sinf(ang);
    int sh = 6 - (int)floorf(sinf(t * 2.0f) * 0.9f + 0.5f);
    pxa(r, 20 - sh, 22, 2 * sh, 1, 90, 40, 40, 90); pxa(r, 20 - sh + 1, 21, 2 * sh - 2, 1, 90, 40, 40, 60);   /* its shadow */
    for (int gy = 0; gy < GH; gy++)
        for (int gx = 0; gx < GW; gx++) {
            float x = gx + 0.5f - 20.0f, y = gy + 0.5f - cy0;
            float lx = x * ca + y * sa, ly = -x * sa + y * ca;                       /* the point in the hammer's own frame */
            int R = -1, G = 0, B = 0;
            if (fabsf(lx) <= 2.2f && ly >= -3.0f && ly <= 9.0f) {                    /* handle */
                if (fabsf(lx) > 1.4f || ly > 8.1f) { R = 36; G = 40; B = 58; }
                else if (lx < 0.1f) { R = 196; G = 136; B = 74; } else { R = 132; G = 84; B = 42; }
            }
            if (fabsf(lx) <= 3.4f && ly >= -3.6f && ly <= -1.4f) { R = 36; G = 40; B = 58; if (fabsf(lx) < 2.2f && ly > -3.0f && ly < -2.0f) { R = 122; G = 130; B = 156; } }   /* collar */
            if (fabsf(lx) <= 7.6f && ly >= -8.6f && ly <= -3.4f) {                   /* head */
                if (fabsf(lx) > 6.6f || ly < -7.6f || ly > -4.4f) { R = 36; G = 40; B = 58; }
                else if (ly < -5.8f) { R = 206; G = 214; B = 232; }
                else { R = 122; G = 130; B = 156; }
                if (R == 206 && lx > 4.5f) { R = 170; G = 180; B = 208; }
            }
            if (R >= 0) px(r, gx, gy, 1, 1, R, G, B);
        }
    float gl = fmodf(t, 2.6f);                                                       /* a glint travelling over the head */
    if (gl < 0.4f) {
        float lx = -5.5f + gl * 28.0f, ly = -6.8f;
        int gx = (int)(20.0f + lx * ca - ly * sa), gy = (int)(cy0 + lx * sa + ly * ca);
        sparkle(r, gx, gy, 255, 255, 255, 255);
    }
}

/* 10. "I cannot break it with my hands. I need something heavy." a fist bumping the shrine, getting nowhere; the thought of an anvil */
static void scene_heavy(SDL_Renderer *r, float t) {
    gradient(r, 58, 66, 108, 156, 156, 190);
    ground(r, 21, 52, 96, 68, t);
    float P = 1.8f, p = fmodf(t, P) / P;
    float k = p < 0.2f ? 0.0f : p < 0.7f ? (p - 0.2f) / 0.5f : p < 0.8f ? 1.0f - (p - 0.7f) / 0.1f : 0.0f;
    k = k * k * (3.0f - 2.0f * k);
    int hit = (p >= 0.8f || p < 0.1f);                                               /* it has just landed */
    int shake = (p >= 0.8f && p < 0.88f) ? 1 : 0;
    shrine_g(r, 23 + shake, 21, t, 0.0f);
    int fx = 14 - (int)(9.0f * k), fy = 11;                                          /* the fist: its right edge touches the pillar when k = 0 */
    for (int gx = 0; gx < fx; gx++) px(r, gx, fy + 1, 1, 4, 214, 60, 60);            /* the sleeve and arm run in from the left */
    px(r, fx - 1, fy + 1, 1, 4, 232, 90, 90);
    px(r, fx, fy, 6, 6, 36, 40, 58);                                                 /* the fist, outlined */
    px(r, fx + 1, fy + 1, 4, 4, 248, 208, 170);
    px(r, fx + 3, fy + 1, 1, 4, 226, 176, 138); px(r, fx + 1, fy + 3, 4, 1, 226, 176, 138);   /* knuckle lines */
    px(r, fx + 1, fy + 1, 2, 1, 255, 230, 200);
    if (hit) {                                                                       /* "tink": a star and dust at the stone, which is not even scratched */
        float ti = fmodf(p - 0.8f + 1.0f, 1.0f);
        if (ti < 0.3f) {
            int a = (int)(230 * (1.0f - ti / 0.3f));
            sparkle(r, 20, 11, 255, 250, 200, a); sparkle(r, 19, 9, 255, 255, 255, a);
            pxa(r, 18, 14, 1, 1, 255, 236, 170, a); pxa(r, 17, 12, 1, 1, 255, 236, 170, a);
        }
    }
    /* the thought: an anvil, in a bubble up in the corner */
    px(r, 30, 13, 1, 1, 255, 255, 255); px(r, 31, 11, 2, 2, 255, 255, 255);
    for (int dy = -5; dy <= 5; dy++)
        for (int dx = -7; dx <= 7; dx++) {
            float e = (dx / 7.0f) * (dx / 7.0f) + (dy / 5.0f) * (dy / 5.0f);
            if (e <= 1.0f) px(r, 34 + dx, 6 + dy, 1, 1, e > 0.74f ? 168 : 255, e > 0.74f ? 176 : 255, e > 0.74f ? 214 : 255);
        }
    static const char *AN[6] = { "##########", "..########", "....####..", "....####..", "...######.", "..########" };
    int ob = (int)floorf(sinf(t * 2.0f) * 0.7f + 0.5f);
    for (int j = 0; j < 6; j++) for (int i = 0; i < 10; i++) {
        if (AN[j][i] != '#') continue;
        px(r, 29 + i, 3 + j + ob, 1, 1, j == 0 ? 120 : 70, j == 0 ? 126 : 76, j == 0 ? 150 : 100);
    }
}

/* 11. "Here we go again." a ring of light running round a little grave: it starts over */
static void scene_again(SDL_Renderer *r, float t) {
    gradient(r, 10, 18, 36, 34, 66, 84);
    for (int i = 0; i < 14; i++) {
        int gx = (int)(hs(i, 17, 8) % GW), gy = (int)(hs(i, 18, 8) % GH);
        int tw = ((int)(t * 1.4f) + i) % 3;
        if (tw == 0) px(r, gx, gy, 1, 1, 200, 214, 250); else if (tw == 1) px(r, gx, gy, 1, 1, 78, 94, 140);
    }
    float head = fmodf(t * 0.5f, 1.0f);
    for (int i = 0; i < 64; i++) {
        float f = i / 64.0f, a = f * 6.2832f - 1.5708f;
        float behind = fmodf(head - f + 1.0f, 1.0f);                                 /* 0 at the head, 1 = a full turn behind it */
        int A = (int)(40 + 215 * (1.0f - behind) * (1.0f - behind));
        int gx = 20 + (int)floorf(cosf(a) * 9.0f + 0.5f), gy = 12 + (int)floorf(sinf(a) * 9.0f + 0.5f);
        pxa(r, gx, gy, 1, 1, 255, 214, 120, A);
        if (behind < 0.25f) pxa(r, 20 + (int)floorf(cosf(a) * 8.0f + 0.5f), 12 + (int)floorf(sinf(a) * 8.0f + 0.5f), 1, 1, 255, 236, 170, A / 2);
    }
    float ha = head * 6.2832f - 1.5708f;                                             /* the arrow head, pointing along the ring */
    int hx = 20 + (int)floorf(cosf(ha) * 9.0f + 0.5f), hy = 12 + (int)floorf(sinf(ha) * 9.0f + 0.5f);
    px(r, hx - 1, hy - 1, 3, 3, 255, 244, 190); px(r, hx, hy, 1, 1, 255, 255, 255);
    for (int dy = 0; dy < 4; dy++)                                                   /* the mound */
        for (int dx = -6; dx <= 6; dx++) {
            float e = (dx / 6.0f) * (dx / 6.0f) + (dy / 3.6f) * (dy / 3.6f);
            if (e <= 1.0f) px(r, 20 + dx, 15 + dy, 1, 1, dy < 2 ? 96 : 76, dy < 2 ? 76 : 58, dy < 2 ? 60 : 46);
        }
    for (int gy = 8; gy <= 15; gy++) {                                               /* the stone */
        int x0 = 18, x1 = 22;
        if (gy == 8) { x0 = 19; x1 = 21; }
        for (int gx = x0; gx <= x1; gx++) px(r, gx, gy, 1, 1, gx == x0 ? 190 : gx == x1 ? 104 : 148, gx == x0 ? 192 : gx == x1 ? 106 : 150, gx == x0 ? 202 : gx == x1 ? 120 : 160);
    }
    px(r, 19, 11, 3, 1, 90, 92, 106);
    int sway = (int)floorf(sinf(t * 1.6f) * 1.0f + 0.5f);
    px(r, 25, 14, 1, 3, 52, 128, 66); px(r, 25 + sway, 13, 1, 1, 244, 140, 182); px(r, 24 + sway, 12, 3, 1, 250, 160, 196); px(r, 25 + sway, 12, 1, 1, 255, 226, 120);
}

static void draw_scene(SDL_Renderer *r, int scene, float t) {
    switch (scene) {
    case BRAIN_SCENE_ORB:    scene_orb(r, t);    break;
    case BRAIN_SCENE_GRASS:  scene_grass(r, t);  break;
    case BRAIN_SCENE_CLOUDS: scene_clouds(r, t); break;
    case BRAIN_SCENE_GRAVE:  scene_grave(r, t);  break;
    case BRAIN_SCENE_ALEX:    scene_alex(r, t);    break;
    case BRAIN_SCENE_ALEXORB: scene_alexorb(r, t); break;
    case BRAIN_SCENE_SHRINE:  scene_shrine(r, t);  break;
    case BRAIN_SCENE_HAMMER:  scene_hammer(r, t);  break;
    case BRAIN_SCENE_HEAVY:   scene_heavy(r, t);   break;
    case BRAIN_SCENE_AGAIN:   scene_again(r, t);   break;
    default:                 scene_coin(r, t);   break;
    }
}

/* ---------- the face and thought bubble in the corner of the monitor ---------- */
static void draw_face_badge(SDL_Renderer *r, float t) {
    const Person *who = thought_voice();
    if (!who) return;
    int q = s / 2; if (q < 1) q = 1;
    int pw = 12 * q, bx = ox + s, by = oy + GH * s - s - pw;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 235); fillr(r, bx - 1, by - 1, pw + 2, pw + 2);
    for (int row = 0; row < 12; row++) {                                            /* a patch of dusk sky behind the face */
        float k = row / 11.0f;
        SDL_SetRenderDrawColor(r, mix(52, 92, k), mix(66, 116, k), mix(120, 180, k), 255);
        fillr(r, bx, by + row * q, pw, q);
    }
    char_draw_portrait(r, who, bx + q, by + q, q, 0);
    for (int i = 0; i < 3; i++) {                                                   /* the thought trail: three dots rising out of the head, lighting up one after another */
        int on = ((int)(t * 2.0f) % 4) > i;
        int d = s * (i + 2) / 3; if (d < 2) d = 2;
        int dx = bx + pw / 2 + i * (s / 2) - d / 2, dy = by - s * (2 * i + 2) + (i > 0 ? i * s / 2 : 0);
        SDL_SetRenderDrawColor(r, 255, 255, 255, on ? 235 : 60);
        fillr(r, dx, dy, d, d);
    }
}

/* the monitor itself: bezel, picture, scanlines, flicker between thoughts */
static void draw_monitor(SDL_Renderer *r, int dy) {
    SDL_SetRenderDrawColor(r, 86, 92, 120, 255); fillr(r, mon_r.x, mon_r.y + dy, mon_r.w, mon_r.h);
    SDL_SetRenderDrawColor(r, 28, 30, 44, 255);  fillr(r, mon_r.x + 1, mon_r.y + dy + 1, mon_r.w - 2, mon_r.h - 2);
    SDL_Rect clip = { scr_r.x, scr_r.y + dy, scr_r.w, scr_r.h };
    SDL_RenderSetClipRect(r, &clip);
    ox = scr_r.x; oy = scr_r.y + dy;
    draw_scene(r, th[cur].scene, tt);
    draw_face_badge(r, tt);

    int lh = s / 5; if (lh < 1) lh = 1;                                             /* scanlines: one thin dark line under every pixel row */
    SDL_SetRenderDrawColor(r, 0, 0, 0, 46);
    for (int gy = 0; gy < GH; gy++) fillr(r, ox, oy + gy * s + s - lh, GW * s, lh);

    if (flick > 0) {                                                                /* static between thoughts */
        float k = flick / FLICKER_T;
        int tick = (int)(tt * 40.0f);
        for (int gy = 0; gy < GH; gy++) {
            unsigned hh = hs(gy, tick, 5);
            if (hh % 3 == 0) {
                int x0 = (int)(hh >> 4) % GW, w = 4 + (int)(hh >> 12) % (GW - 4);
                int v = 150 + (int)(hh >> 20) % 100;
                SDL_SetRenderDrawColor(r, v, v, v, (int)(200 * k));
                fillr(r, ox + x0 * s, oy + gy * s, mini(w, GW - x0) * s, s);
            }
        }
        SDL_SetRenderDrawColor(r, 0, 0, 0, (int)(110 * k));
        fillr(r, ox, oy, GW * s, GH * s);
    }
    SDL_RenderSetClipRect(r, NULL);
}

/* nothing in the head yet: a dead monitor with a little static, and a caption */
static void draw_blank(SDL_Renderer *r, int dy) {
    SDL_SetRenderDrawColor(r, 86, 92, 120, 255); fillr(r, mon_r.x, mon_r.y + dy, mon_r.w, mon_r.h);
    SDL_SetRenderDrawColor(r, 18, 20, 30, 255);  fillr(r, mon_r.x + 1, mon_r.y + dy + 1, mon_r.w - 2, mon_r.h - 2);
    SDL_Rect clip = { scr_r.x, scr_r.y + dy, scr_r.w, scr_r.h };
    SDL_RenderSetClipRect(r, &clip);
    int tick = (int)(tt * 6.0f);
    for (int gy = 0; gy < GH; gy++) for (int gx = 0; gx < GW; gx++) {
        unsigned hh = hs(gx, gy, tick);
        if (hh % 23 == 0) { int v = 40 + (int)(hh >> 8) % 50; SDL_SetRenderDrawColor(r, v, v, v + 8, 255); fillr(r, scr_r.x + gx * s, scr_r.y + dy + gy * s, s, s); }
    }
    SDL_RenderSetClipRect(r, NULL);
    const char *msg = "Nothing on my mind yet.";
    SDL_SetRenderDrawColor(r, 150, 158, 200, 255);
    font_draw(r, msg, cap_r.x, cap_r.y + dy, cap);
}

/* ---------- drawing ---------- */
static void box(SDL_Renderer *r, SDL_Rect b, int dy, int fr, int fg, int fb, int hot, int a) {
    SDL_SetRenderDrawColor(r, 255, 255, 255, a * 170 / 255); fillr(r, b.x, b.y + dy, b.w, b.h);
    if (hot) SDL_SetRenderDrawColor(r, fr, fg, fb, a); else SDL_SetRenderDrawColor(r, 24, 28, 48, a);
    fillr(r, b.x + 1, b.y + dy + 1, b.w - 2, b.h - 2);
}
static void label(SDL_Renderer *r, const char *str, SDL_Rect b, int dy, int cl) {
    font_draw(r, str, b.x + (b.w - font_width(str, cl)) / 2, b.y + dy + (b.h - font_height(cl)) / 2, cl);
}

void brainwin_draw(SDL_Renderer *r) {
    if (!is_open && !closing) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int dy = yoff;
    SDL_SetRenderDrawColor(r, 0, 0, 0, (int)(150 * anim));                          /* dim everything behind */
    fillr(r, 0, 0, W, H);

    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);                                  /* frame */
    fillr(r, win.x, win.y + dy, win.w, win.h);
    SDL_SetRenderDrawColor(r, 10, 12, 24, 245);
    fillr(r, win.x + bt, win.y + dy + bt, win.w - 2 * bt, win.h - 2 * bt);

    int tx = win.x + pad, ty = win.y + pad + dy + (hdr - font_height(cell)) / 2;
    SDL_SetRenderDrawColor(r, 255, 150, 170, 255);
    font_draw(r, "BRAIN", tx, ty, cell);
    char buf[48];
    snprintf(buf, sizeof buf, "%d THOUGHT%s", nth, nth == 1 ? "" : "S");
    SDL_SetRenderDrawColor(r, 150, 158, 200, 255);
    font_draw(r, buf, tx + font_width("BRAIN", cell) + 2 * pad, win.y + pad + dy + (hdr - font_height(c2)) / 2, c2);

    box(r, close_r, dy, 70, 80, 130, grab == 1, 255);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    label(r, "X", close_r, dy, cell);

    if (nth == 0) draw_blank(r, dy);
    if (nth > 0) {
        draw_monitor(r, dy);

        float k = auto_on ? tm / dwell(cur) : 1.0f; k = k > 1 ? 1 : k;              /* time left on this thought */
        SDL_SetRenderDrawColor(r, 40, 46, 80, 255); fillr(r, bar_r.x, bar_r.y + dy, bar_r.w, bar_r.h);
        if (auto_on) SDL_SetRenderDrawColor(r, 255, 150, 170, 255); else SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
        fillr(r, bar_r.x, bar_r.y + dy, (int)(bar_r.w * (auto_on ? k : 1.0f)), bar_r.h);

        Ln ln[MAX_LINES];
        int maxc = (cap_r.w + cap) / (6 * cap);
        int n = wrap(th[cur].text, maxc, MAX_LINES, ln);
        int left = (int)typed;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        for (int i = 0; i < n && left > 0; i++) {
            char tmp[MAX_TEXT];
            int len = ln[i].len < left ? ln[i].len : left;
            memcpy(tmp, th[cur].text + ln[i].start, (size_t)len); tmp[len] = 0;
            font_draw(r, tmp, cap_r.x, cap_r.y + dy + i * 8 * cap, cap);
            left -= ln[i].len + 1;
        }
    }

    for (int i = 0; i < nth; i++) {                                                 /* the numbers */
        char nb[8]; snprintf(nb, sizeof nb, "%d", i + 1);
        int hot = i == cur;
        box(r, pick_r[i], dy, 255, 214, 110, hot || grab == 10 + i, 255);
        if (hot) SDL_SetRenderDrawColor(r, 20, 14, 0, 255); else SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        label(r, nb, pick_r[i], dy, c2);
    }
    box(r, auto_r, dy, 110, 230, 160, auto_on || grab == 2, 255);
    if (auto_on) SDL_SetRenderDrawColor(r, 6, 24, 12, 255); else SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    label(r, "AUTO", auto_r, dy, c2);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
