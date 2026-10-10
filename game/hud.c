/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "hud.h"
#include "audio.h"
#include "font.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* the whole card is drawn in "cells" of q screen px (the same size as one font pixel), so the
 * art, the lettering and the portrait all share one pixel grid. size in cells: */
#define CW 62
#define CH 33

#define START_HP    100
#define FLASH_T     1.6f            /* how long the name flashes after it changes */
#define TOAST_IN    0.25f
#define TOAST_HOLD  2.8f
#define TOAST_OUT   0.45f

static int   W, H, q, card_x, card_y;       /* card_x,card_y: top-left of the card in screen px */
static float u, t;
static const Person *who;
static int   hp, hp_max;
static float hp_shown;              /* trails behind hp when it drops */
static float flash_t = -1;          /* seconds since the name changed, -1 = idle */
static float toast_t = -1;
static char  toast_name[32];

static void col(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }

/* fills a rectangle given in card cells */
static void cl(SDL_Renderer *r, int cx, int cy, int w, int h) {
    SDL_Rect rc = { card_x + cx * q, card_y + cy * q, w * q, h * q };
    SDL_RenderFillRect(r, &rc);
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int mix(int a, int b, float k) { return (int)(a + (b - a) * k); }

void hud_init(int w, int h, const Person *p) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    q = (int)(1.7f * u); if (q < 2) q = 2;
    card_x = W - (int)(10 * u) - CW * q;
    card_y = (int)(8 * u);                                  /* same top margin as the now-playing card */
    t = 0; who = p;
    hp = hp_max = START_HP; hp_shown = (float)hp;
    flash_t = toast_t = -1;
}

const Person *hud_person(void) { return who; }

void hud_card_rect(int *x, int *y, int *w, int *h) { *x = card_x; *y = card_y; *w = CW * q; *h = CH * q; }

void hud_set_hp(int v, int vmax) {
    if (vmax < 1) vmax = 1;
    hp_max = vmax;
    hp = clampi(v, 0, vmax);
    if (hp_shown < hp) hp_shown = (float)hp;            /* healing shows instantly, damage leaves a trail */
}

void hud_set_person(const Person *p) {
    if (!p) return;
    int changed = !who || strcmp(who->name, p->name) != 0;
    who = p;
    if (!changed) return;
    flash_t = 0; toast_t = 0;
    snprintf(toast_name, sizeof toast_name, "%s", p->name);
    sfx_coin();
}

void hud_update(float dt) {
    t += dt;
    if (flash_t >= 0) { flash_t += dt; if (flash_t > FLASH_T) flash_t = -1; }
    if (toast_t >= 0) { toast_t += dt; if (toast_t > TOAST_IN + TOAST_HOLD + TOAST_OUT) toast_t = -1; }
    if (hp_shown > hp) {
        hp_shown -= dt * (float)hp_max * 0.35f;         /* the pale trail drains away */
        if (hp_shown < hp) hp_shown = (float)hp;
    }
}

/* ---------- pieces ---------- */
static const int HOLO[6][3] = {                          /* the shimmering strip under the header */
    { 255, 214, 110 }, { 255, 150, 170 }, { 180, 140, 255 },
    { 110, 200, 255 }, { 120, 235, 170 }, { 255, 240, 150 },
};

static void draw_portrait_window(SDL_Renderer *r, int hot) {
    /* gold frame, then a little patch of sky behind the face (they come from the clouds) */
    if (hot) col(r, 255, 255, 255, 255); else col(r, 214, 170, 78, 255);
    cl(r, 2, 12, 14, 19);
    for (int row = 0; row < 17; row++) {
        float k = row / 16.0f;
        col(r, mix(92, 176, k), mix(148, 216, k), mix(232, 252, k), 255);
        cl(r, 3, 13 + row, 12, 1);
    }
    SDL_Rect clip = { card_x + 3 * q, card_y + 13 * q, 12 * q, 17 * q };
    SDL_RenderSetClipRect(r, &clip);                    /* two pixel clouds drifting through the window */
    for (int k = 0; k < 2; k++) {
        int cx = 3 + ((int)(t * (2.0f + k) + k * 9) % 19) - 4, cy = 14 + k * 4;
        col(r, 255, 255, 255, 230);
        cl(r, cx + 1, cy, 3, 1);
        cl(r, cx, cy + 1, 5, 1);
    }
    SDL_RenderSetClipRect(r, NULL);
    col(r, 0, 0, 0, 55);                                /* inner edge shadow so the window feels recessed */
    cl(r, 3, 13, 12, 1);
    cl(r, 3, 14, 1, 16);
    char_draw_portrait(r, who, card_x + 4 * q, card_y + 20 * q, q, 0);
}

static void draw_hp(SDL_Renderer *r) {
    float f = hp_max ? (float)hp / hp_max : 0, fs = hp_max ? hp_shown / hp_max : 0;
    col(r, 255, 214, 110, 255);
    font_draw(r, "HP", card_x + 18 * q, card_y + 22 * q, q);

    col(r, 90, 98, 150, 255); cl(r, 30, 20, 30, 11);    /* bar frame */
    col(r, 22, 12, 30, 255);  cl(r, 31, 21, 28, 9);     /* empty well */

    int span = 26;
    int fw = hp > 0 ? clampi((int)(f * span + 0.5f), 1, span) : 0;
    int tw = clampi((int)(fs * span + 0.5f), 0, span);
    if (tw > fw) { col(r, 255, 232, 232, 235); cl(r, 32 + fw, 22, tw - fw, 7); }   /* the trail */

    if (fw > 0) {
        int R, G, B;
        if (f > 0.6f)       { R = 88;  G = 208; B = 112; }
        else if (f > 0.3f)  { R = 246; G = 190; B = 72;  }
        else {                                           /* low: the bar throbs red */
            float k = 0.5f + 0.5f * sinf(t * 9.0f);
            R = mix(200, 255, k); G = mix(64, 110, k); B = mix(64, 110, k);
        }
        col(r, R, G, B, 255);                                          cl(r, 32, 22, fw, 7);
        col(r, clampi(R + 60, 0, 255), clampi(G + 60, 0, 255), clampi(B + 60, 0, 255), 255);
                                                                       cl(r, 32, 22, fw, 2);   /* highlight */
        col(r, R * 6 / 10, G * 6 / 10, B * 6 / 10, 255);               cl(r, 32, 28, fw, 1);   /* underside */
    }

    char num[16]; snprintf(num, sizeof num, "%d", hp);
    int nw = (int)strlen(num) * 6 - 1;                  /* in cells */
    int nx = 45 - nw / 2;
    col(r, 8, 10, 24, 220);  font_draw(r, num, card_x + (nx + 1) * q, card_y + 23 * q, q);
    col(r, 255, 255, 255, 255); font_draw(r, num, card_x + nx * q, card_y + 22 * q, q);
}

static void draw_name(SDL_Renderer *r) {
    int blink = flash_t >= 0 && ((int)(flash_t * 12.0f) & 1) == 0;
    if (flash_t >= 0) {                                 /* a bright plate behind the name, fading out */
        col(r, 255, 224, 130, (int)(95 * (1.0f - flash_t / FLASH_T)));
        cl(r, 17, 11, 44, 9);
    }
    SDL_Rect clip = { card_x + 17 * q, card_y + 11 * q, 44 * q, 9 * q };
    SDL_RenderSetClipRect(r, &clip);                    /* a very long name never spills out of the card */
    col(r, 8, 10, 30, 230);
    font_draw(r, who->name, card_x + 19 * q, card_y + 13 * q, q);
    if (flash_t >= 0) { if (blink) col(r, 255, 255, 255, 255); else col(r, 255, 206, 60, 255); }
    else col(r, 255, 244, 200, 255);
    font_draw(r, who->name, card_x + 18 * q, card_y + 12 * q, q);
    SDL_RenderSetClipRect(r, NULL);
}

/* 7x7 coin; squashed horizontally over time so it looks like it is spinning */
void hud_draw_coin(SDL_Renderer *r, int tx, int ty, int q, float tt) {
    static const char *P[7] = {
        "..ooo..", ".oOhOo.", "oOhOOOo", "oOhxOOo", "oOhOOOo", ".oOOOo.", "..ooo..",
    };
    int w = (int)(7.0f * fabsf(cosf(tt * 7.0f)) + 0.5f); if (w < 1) w = 1;
    for (int dc = 0; dc < w; dc++) {
        int sc = dc * 7 / w;
        for (int row = 0; row < 7; row++) {
            char c = P[row][sc];
            if (c == '.') continue;
            if (c == 'o') SDL_SetRenderDrawColor(r, 176, 112, 20, 255);
            else if (c == 'O') SDL_SetRenderDrawColor(r, 255, 206, 56, 255);
            else if (c == 'h') SDL_SetRenderDrawColor(r, 255, 244, 168, 255);
            else SDL_SetRenderDrawColor(r, 214, 150, 30, 255);
            SDL_Rect rc = { tx + (7 - w) / 2 * q + dc * q, ty + row * q, q, q };
            SDL_RenderFillRect(r, &rc);
        }
    }
}

static void draw_toast(SDL_Renderer *r) {
    if (toast_t < 0) return;
    const char *pre = "YOU ARE NOW ";
    int tcells = ((int)strlen(pre) + (int)strlen(toast_name)) * 6 - 1;
    int wc = tcells + 17, hc = 13;

    float pin = toast_t / TOAST_IN; if (pin > 1) pin = 1;
    pin = 1.0f - (1.0f - pin) * (1.0f - pin);
    float pout = toast_t - TOAST_IN - TOAST_HOLD; pout = pout > 0 ? pout / TOAST_OUT : 0;
    float a = pin * (1.0f - pout);
    int A = (int)(255 * a);
    if (A <= 0) return;

    int bx = card_x + CW * q - wc * q;                      /* right-aligned with the card */
    if (bx < (int)(4 * u)) bx = (int)(4 * u);
    int by = card_y + (CH + 3) * q + (int)((-(1.0f - pin) * 5.0f - pout * 4.0f) * q);

    col(r, 0, 0, 0, A * 90 / 255);                      /* drop shadow */
    SDL_Rect sh = { bx + q, by + q, wc * q, hc * q }; SDL_RenderFillRect(r, &sh);
    col(r, 255, 214, 110, A);
    SDL_Rect fr = { bx, by, wc * q, hc * q }; SDL_RenderFillRect(r, &fr);
    col(r, 12, 14, 30, A * 245 / 255);
    SDL_Rect in = { bx + q, by + q, (wc - 2) * q, (hc - 2) * q }; SDL_RenderFillRect(r, &in);

    /* the coin and the text fade with the toast: fade is done by dimming toward the panel colour */
    int cx = bx + 4 * q, cy = by + 3 * q;
    hud_draw_coin(r, cx, cy, q, toast_t);
    if (A < 255) { col(r, 12, 14, 30, 255 - A); SDL_Rect cv = { cx, cy, 7 * q, 7 * q }; SDL_RenderFillRect(r, &cv); }   /* fade */
    int tx = bx + 13 * q, ty = by + 3 * q;
    col(r, 255, 255, 255, A);   font_draw(r, pre, tx, ty, q);
    col(r, 255, 214, 70, A);    font_draw(r, toast_name, tx + (int)strlen(pre) * 6 * q, ty, q);
}

void hud_draw(SDL_Renderer *r) {
    if (!who) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int hot = flash_t >= 0 && ((int)(flash_t * 12.0f) & 1) == 0;

    col(r, 0, 0, 0, 90);   cl(r, 1, 1, CW, CH);                          /* shadow */
    if (hot) col(r, 255, 255, 255, 255); else col(r, 232, 196, 96, 255);
    cl(r, 1, 0, CW - 2, 1); cl(r, 1, CH - 1, CW - 2, 1);                 /* border, corners clipped off */
    cl(r, 0, 1, 1, CH - 2); cl(r, CW - 1, 1, 1, CH - 2);

    col(r, 30, 40, 92, 255);  cl(r, 1, 1, CW - 2, 9);                    /* header */
    for (int row = 11; row < CH - 1; row++) {                            /* body: slow vertical gradient */
        float k = (row - 11) / (float)(CH - 13);
        col(r, mix(18, 30, k), mix(22, 38, k), mix(50, 82, k), 255);
        cl(r, 1, row, CW - 2, 1);
    }
    col(r, 255, 214, 110, 255);
    font_draw(r, "VESSEL ID", card_x + 3 * q, card_y + 2 * q, q);

    int shift = (int)(t * 7.0f);
    for (int x = 1; x < CW - 1; x++) {                                   /* holo strip */
        const int *c = HOLO[((x >> 1) + shift) % 6];
        col(r, c[0], c[1], c[2], 255);
        cl(r, x, 10, 1, 1);
    }

    draw_portrait_window(r, hot);
    draw_name(r);
    draw_hp(r);

    float ph = fmodf(t, 4.0f) / 1.1f;                                    /* a diagonal glint sweeps across now and then */
    if (ph < 1.0f) {
        int pos = (int)(ph * (CW + CH + 8)) - CH;
        col(r, 255, 255, 255, 40);
        for (int row = 1; row < CH - 1; row++) {
            int a = pos - row, b = a + 4;
            if (a < 1) a = 1;
            if (b > CW - 1) b = CW - 1;
            if (b > a) cl(r, a, row, b - a, 1);
        }
    }

    draw_toast(r);
}
