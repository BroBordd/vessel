/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "toast.h"
#include "audio.h"
#include "font.h"
#include "hud.h"
#include "missions.h"
#include <stdio.h>

#define TOAST_IN    0.25f           /* same timing as the name toast on the ID card */
#define TOAST_HOLD  2.8f
#define TOAST_OUT   0.45f
#define MAX_PENDING 4

#define MAX_BODY    48
typedef struct { const char *head; char body[MAX_BODY]; } Toast;      /* head must be a string literal; body is copied */

static int   W, H, q;
static float u;
static Toast pending[MAX_PENDING];          /* waiting for the current toast to finish */
static int   npend;
static Toast cur;                           /* the toast on screen */
static int   showing;
static float cur_t;

void toast_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    q = (int)(1.7f * u); if (q < 2) q = 2;      /* the same pixel size as the ID card's toast */
    showing = 0; cur_t = 0; npend = 0;
}

static void show(const Toast *t) {
    cur = *t; showing = 1; cur_t = 0;
    sfx_coin();
}

static void enqueue(const char *head, const char *body) {
    Toast t; t.head = head;
    snprintf(t.body, sizeof t.body, "%s", body);
    if (!showing) show(&t);
    else if (npend < MAX_PENDING) pending[npend++] = t;
}

int task_toast(const char *task) {
    int id = mission_add(task);
    if (id < 0) return -1;
    enqueue("NEW TASK ADDED", task);
    return id;
}

void toast_notice(const char *head, const char *body) { enqueue(head, body); }

void toast_update(float dt) {
    if (!showing) return;
    cur_t += dt;
    if (cur_t < TOAST_IN + TOAST_HOLD + TOAST_OUT) return;
    showing = 0;
    if (npend > 0) {                            /* next in line */
        Toast next = pending[0];
        for (int i = 1; i < npend; i++) pending[i - 1] = pending[i];
        npend--;
        show(&next);
    }
}

void toast_draw(SDL_Renderer *r, int x, int y) {
    if (!showing) return;

    float pin = cur_t / TOAST_IN; if (pin > 1) pin = 1;
    pin = 1.0f - (1.0f - pin) * (1.0f - pin);                       /* ease out */
    float pout = cur_t - TOAST_IN - TOAST_HOLD; pout = pout > 0 ? pout / TOAST_OUT : 0;
    int A = (int)(255 * pin * (1.0f - pout));
    if (A <= 0) return;

    int tw = font_width(cur.head, q), nw = font_width(cur.body, q);
    if (nw > tw) tw = nw;
    int wp = 13 * q + tw + 4 * q;                                   /* coin column + text + right pad */
    if (wp < ui_panel_w(W, H)) wp = ui_panel_w(W, H);               /* the same width as the mission list and the music card */
    int hp = 8 * q + font_height(q) * 2;                           /* pad 3, head, gap 2, name, pad 3 */
    if (hp < 13 * q) hp = 13 * q;

    int bx = x, by = y + q * 2 + (int)((-(1.0f - pin) * 5.0f - pout * 4.0f) * q);
    if (bx + wp > W - (int)(4 * u)) bx = W - (int)(4 * u) - wp;     /* never off the screen */
    if (bx < (int)(4 * u)) bx = (int)(4 * u);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, A * 90 / 255);               /* drop shadow */
    SDL_Rect sh = { bx + q, by + q, wp, hp }; SDL_RenderFillRect(r, &sh);
    SDL_SetRenderDrawColor(r, 255, 214, 110, A);                    /* gold frame */
    SDL_Rect fr = { bx, by, wp, hp }; SDL_RenderFillRect(r, &fr);
    SDL_SetRenderDrawColor(r, 12, 14, 30, A * 245 / 255);
    SDL_Rect in = { bx + q, by + q, wp - 2 * q, hp - 2 * q }; SDL_RenderFillRect(r, &in);

    int cx = bx + 4 * q, cy = by + (hp - 7 * q) / 2;
    hud_draw_coin(r, cx, cy, q, cur_t);
    if (A < 255) {                                                  /* fade the coin by dimming toward the panel */
        SDL_SetRenderDrawColor(r, 12, 14, 30, 255 - A);
        SDL_Rect cv = { cx, cy, 7 * q, 7 * q }; SDL_RenderFillRect(r, &cv);
    }

    int tx = bx + 13 * q, ty = by + 3 * q;
    SDL_SetRenderDrawColor(r, 255, 255, 255, A);   font_draw(r, cur.head, tx, ty, q);
    SDL_SetRenderDrawColor(r, 255, 214, 70, A);    font_draw(r, cur.body, tx, ty + font_height(q) + q * 2, q);
}
