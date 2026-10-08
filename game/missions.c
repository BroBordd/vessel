/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "missions.h"
#include "font.h"

#define MAX_MISSIONS  6
#define DONE_HOLD     1.0f          /* seconds a ticked mission stays visible */
#define DONE_FADE     0.5f          /* then fades out over this long */

typedef struct { const char *text; int used, done; float done_t, born; } Mission;

static Mission m[MAX_MISSIONS];
static int   W, H;
static float u;

void missions_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    for (int i = 0; i < MAX_MISSIONS; i++) m[i].used = 0;
}

int mission_add(const char *text) {
    for (int i = 0; i < MAX_MISSIONS; i++)
        if (!m[i].used) {
            m[i].used = 1; m[i].done = 0; m[i].done_t = 0; m[i].born = 0; m[i].text = text;
            return i;
        }
    return -1;
}

void mission_complete(int id) {
    if (id < 0 || id >= MAX_MISSIONS || !m[id].used || m[id].done) return;
    m[id].done = 1; m[id].done_t = 0;
}

void missions_update(float dt) {
    for (int i = 0; i < MAX_MISSIONS; i++) {
        if (!m[i].used) continue;
        m[i].born += dt;
        if (m[i].done) {
            m[i].done_t += dt;
            if (m[i].done_t > DONE_HOLD + DONE_FADE) m[i].used = 0;
        }
    }
}

static void fillr(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect q = { x, y, w, h };
    SDL_RenderFillRect(r, &q);
}

/* alpha 0..1 for one row: fades in when added, fades out after completion */
static float row_alpha(const Mission *k) {
    float a = k->born / 0.3f; if (a > 1) a = 1;
    if (k->done && k->done_t > DONE_HOLD) a *= 1.0f - (k->done_t - DONE_HOLD) / DONE_FADE;
    return a < 0 ? 0 : a;
}

void missions_draw(SDL_Renderer *r) {
    int n = 0, widest = 0;
    int cell = (int)(1.8f * u); if (cell < 2) cell = 2;
    for (int i = 0; i < MAX_MISSIONS; i++) {
        if (!m[i].used) continue;
        n++;
        int w = font_width(m[i].text, cell);
        if (w > widest) widest = w;
    }
    if (!n) return;

    int pad = (int)(8 * u), rowh = 10 * cell, box = 7 * cell, gap = 2 * cell;
    int title_h = 9 * cell;
    int pw = pad + box + gap + widest + pad;
    int ph = pad + title_h + n * rowh + pad - cell * 2;
    int x0 = (int)(10 * u), y0 = (int)(14 * u);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
    fillr(r, x0, y0, pw, ph);

    SDL_SetRenderDrawColor(r, 255, 214, 110, 255);
    font_draw(r, "MISSIONS", x0 + pad, y0 + pad, cell);

    int y = y0 + pad + title_h;
    for (int i = 0; i < MAX_MISSIONS; i++) {
        if (!m[i].used) continue;
        int A = (int)(255 * row_alpha(&m[i]));
        int bx = x0 + pad, tx = bx + box + gap, th = font_height(cell);
        int by = y + (th - box) / 2;

        SDL_SetRenderDrawColor(r, 255, 255, 255, A);               /* checkbox */
        fillr(r, bx, by, box, box);
        SDL_SetRenderDrawColor(r, 0, 0, 0, A);
        fillr(r, bx + cell, by + cell, box - 2 * cell, box - 2 * cell);
        if (m[i].done) {
            SDL_SetRenderDrawColor(r, 120, 230, 130, A);
            fillr(r, bx + 2 * cell, by + 2 * cell, box - 4 * cell, box - 4 * cell);
        }

        if (m[i].done) SDL_SetRenderDrawColor(r, 160, 160, 160, A);
        else           SDL_SetRenderDrawColor(r, 255, 255, 255, A);
        font_draw(r, m[i].text, tx, y, cell);
        if (m[i].done) fillr(r, tx, y + th / 2, font_width(m[i].text, cell), cell > 1 ? cell / 2 : 1);   /* strikethrough */
        y += rowh;
    }
}
