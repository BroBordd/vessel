/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "pausebtn.h"
#include "nowplaying.h"
#include "thought.h"
#include "font.h"
#include <math.h>

static int   W, H, enabled, paused, pdown, rdown;
static float u, vis, pa;            /* vis: button fade-in, pa: overlay fade */

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int inside(SDL_Rect q, int x, int y) { return x >= q.x && x < q.x + q.w && y >= q.y && y < q.y + q.h; }

static SDL_Rect btn_rect(void) {                      /* follows the brain button / card (which follows the music one), so it glides along */
    int x, y, w, h; thought_rect(&x, &y, &w, &h);
    int bs = nowplaying_button_size();
    return (SDL_Rect){ x + w + nowplaying_gap(), y, bs, bs };
}
static SDL_Rect resume_rect(void) {
    int w = (int)(120 * u), h = (int)(32 * u);
    return (SDL_Rect){ (W - w) / 2, H / 2 + (int)(14 * u), w, h };
}

void pausebtn_init(int w, int h) { W = w; H = h; u = (w < h ? w : h) / 360.0f; enabled = paused = pdown = rdown = 0; vis = pa = 0; }
void pausebtn_set_enabled(int on) { enabled = on; if (!on) paused = 0; }
int  pausebtn_paused(void) { return paused; }

int pausebtn_touch(int a, int x, int y) {
    if (!enabled) return 0;
    SDL_Rect b = btn_rect(), rs = resume_rect();
    if (a == 0) {
        if (inside(b, x, y)) { pdown = 1; return 1; }
        if (paused && inside(rs, x, y)) { rdown = 1; return 1; }
        return paused ? 1 : 0;                       /* paused: the world below does not get touches */
    }
    if (pdown) {
        if (a == 1 || a == 3) {
            pdown = 0;
            if (a == 1 && inside(b, x, y)) { paused = !paused; return paused ? 2 : 1; }
        }
        return 1;
    }
    if (rdown) {
        if (a == 1 || a == 3) { rdown = 0; if (a == 1 && inside(rs, x, y)) paused = 0; }
        return 1;
    }
    return paused ? 1 : 0;
}

void pausebtn_update(float dt) {
    vis += ((enabled ? 1.0f : 0.0f) - vis) * (1.0f - expf(-dt * 9.0f));
    if (fabsf((enabled ? 1.0f : 0.0f) - vis) < 0.004f) vis = enabled ? 1.0f : 0.0f;
    float tp = paused ? 1.0f : 0.0f;
    pa += (tp - pa) * (1.0f - expf(-dt * 12.0f));
    if (fabsf(tp - pa) < 0.004f) pa = tp;
}

static void fillr(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y, w, h }; SDL_RenderFillRect(r, &q); }

void pausebtn_draw_overlay(SDL_Renderer *r) {
    if (pa <= 0.0f) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 4, 6, 16, (int)(165 * pa));
    fillr(r, 0, 0, W, H);
    int ca = clampi((int)(255 * pa), 0, 255);
    int cell = (int)(4.0f * u); if (cell < 3) cell = 3;
    int tw = font_width("PAUSED", cell);
    int ty = H / 2 - (int)(30 * u) - (int)((1.0f - pa) * 10 * u);       /* drifts up into place */
    SDL_SetRenderDrawColor(r, 255, 214, 110, ca);
    font_draw(r, "PAUSED", (W - tw) / 2, ty, cell);

    SDL_Rect rs = resume_rect();
    ui_button_frame(r, rs.x, rs.y, rs.w, rs.h, rdown, ca);
    int c2 = (int)(2.0f * u); if (c2 < 2) c2 = 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, ca);
    font_draw(r, "RESUME", rs.x + (rs.w - font_width("RESUME", c2)) / 2, rs.y + (rs.h - font_height(c2)) / 2, c2);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void pausebtn_draw(SDL_Renderer *r) {
    if (vis <= 0.0f) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect b = btn_rect();
    int a = clampi((int)(255 * vis), 0, 255);
    b.x += (int)((1.0f - vis) * -6 * u);                                   /* tucks in from the left as it appears */
    ui_button_frame(r, b.x, b.y, b.w, b.h, pdown, a);
    int ic = b.w / 10; if (ic < 1) ic = 1;
    int ox = b.x + (b.w - 10 * ic) / 2, oy = b.y + (b.h - 10 * ic) / 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, a);
    if (!paused) {                                                         /* two bars */
        fillr(r, ox + 2 * ic, oy, 2 * ic, 10 * ic);
        fillr(r, ox + 6 * ic, oy, 2 * ic, 10 * ic);
    } else {                                                               /* play triangle, pixel stairs */
        for (int i = 0; i < 5; i++) {
            int hh = (10 - i * 2) * ic;
            fillr(r, ox + i * 2 * ic, oy + (10 * ic - hh) / 2, 2 * ic, hh);
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
