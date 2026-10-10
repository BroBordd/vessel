/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "brainwin.h"
#include "font.h"
#include "hud.h"
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
    go(cur);
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

static void draw_scene(SDL_Renderer *r, int scene, float t) {
    switch (scene) {
    case BRAIN_SCENE_ORB:    scene_orb(r, t);    break;
    case BRAIN_SCENE_GRASS:  scene_grass(r, t);  break;
    case BRAIN_SCENE_CLOUDS: scene_clouds(r, t); break;
    default:                 scene_coin(r, t);   break;
    }
}

/* ---------- the face and thought bubble in the corner of the monitor ---------- */
static void draw_face_badge(SDL_Renderer *r, float t) {
    const Person *who = hud_person();
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
