/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "shrine.h"
#include "audio.h"
#include <math.h>

#define RANGE   1.9f                /* tiles: this close and the interact button shows the sludge drop */
#define LEAVE   2.4f                /* tiles: this far and it goes away again */
#define DECAY_T 4.0f                /* seconds for a full bar of ooze to creep back after letting go */

static int   on, enabled, polluted, in_range, s, tile, tx, ty;
static float prog, tick;
static void (*done_cb)(void);

void shrine_reset(int pixel_scale, int tile_size) {
    on = enabled = polluted = in_range = 0; prog = tick = 0; done_cb = NULL;
    s = pixel_scale; tile = tile_size;
}
void shrine_place(int tile_x, int tile_y) { on = 1; tx = tile_x; ty = tile_y; enabled = polluted = in_range = 0; prog = 0; }
/* the shrine as it was left by an earlier life (chunk 20): placed, switched off, and fouled for good if it was */
void shrine_restore(int tile_x, int tile_y, int was_polluted) {
    shrine_place(tile_x, tile_y);
    if (was_polluted) { polluted = 1; prog = 1.0f; }
}
void shrine_enable(void (*on_done)(void)) { if (!on || polluted) return; enabled = 1; done_cb = on_done; }

int   shrine_exists(void)   { return on; }
int   shrine_enabled(void)  { return on && enabled && !polluted; }
int   shrine_polluted(void) { return on && polluted; }
float shrine_progress(void) { return prog; }
void  shrine_tile(float *x, float *y) { *x = tx + 0.5f; *y = ty + 0.9f; }

int shrine_near(float pxw, float pyw, int allow) {
    if (!shrine_enabled() || !allow) { in_range = 0; return 0; }
    float dx = pxw - (tx + 0.5f) * tile, dy = pyw - (ty + 0.9f) * tile;
    float d = sqrtf(dx * dx + dy * dy) / tile;
    if (!in_range && d < RANGE) in_range = 1;
    else if (in_range && d > LEAVE) in_range = 0;
    return in_range;
}

int shrine_update(float dt, int holding) {
    if (!shrine_enabled()) return 0;
    if (holding) {
        prog += dt / SHRINE_HOLD_T;
        tick -= dt;
        if (tick <= 0) { tick = 0.22f; sfx_blip(0.55f + 0.7f * prog); }     /* a rising tick while the ooze comes */
        if (prog >= 1.0f) {
            prog = 1.0f; polluted = 1; in_range = 0;
            sfx_blip(0.45f);
            if (done_cb) { void (*cb)(void) = done_cb; done_cb = NULL; cb(); }
            return 1;
        }
    } else {
        tick = 0;
        if (prog > 0) { prog -= dt / DECAY_T; if (prog < 0) prog = 0; }
    }
    return 0;
}

float shrine_foot_y(void) { return (ty + 0.9f) * tile; }

int shrine_collides(float fx, float fy, float half_w, float h) {
    if (!on) return 0;
    return fabsf(fx - (tx + 0.5f) * tile) < half_w + 6.0f * s && fabsf(fy - shrine_foot_y()) < h + 1.0f * s;
}

/* ---------- drawing: placeholder pixel art, 14 units wide, 20 tall. (x, y) = feet centre. ---------- */
static void box(SDL_Renderer *r, int x, int y, int ux, int uy, int uw, int uh) {      /* units, relative to the feet centre */
    SDL_Rect q = { x + ux * s, y + uy * s, uw * s, uh * s };
    SDL_RenderFillRect(r, &q);
}
static void col(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }
static int mix(int a, int b, float k) { return (int)(a + (b - a) * k + 0.5f); }

void shrine_draw(SDL_Renderer *r, int cam_x, int cam_y, float t) {
    if (!on) return;
    int x = (int)((tx + 0.5f) * tile) - cam_x, y = (int)((ty + 0.9f) * tile) - cam_y;
    float p = prog;
    float shake = (enabled && p > 0 && !polluted) ? 1.0f : 0.0f;                    /* the stone shudders under the blows */
    int sx = x + (int)floorf(sinf(t * 60.0f) * shake * 0.6f + 0.5f) * s;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    col(r, 0, 0, 0, 70);   box(r, sx, y, -8, -1, 16, 2);                           /* ground shadow */

    if (polluted) {                                                                /* BROKEN: a stump, a gap in the steps, rubble, dull shards */
        col(r, 112, 118, 144, 255); box(r, sx, y, -7, -3, 6, 3); box(r, sx, y, 3, -3, 4, 2);
        col(r, 158, 166, 192, 255); box(r, sx, y, -7, -3, 6, 1); box(r, sx, y, 3, -3, 4, 1);
        col(r, 86, 92, 118, 255);   box(r, sx, y, -7, -1, 6, 1); box(r, sx, y, 3, -1, 4, 1);
        col(r, 134, 140, 166, 255); box(r, sx, y, -5, -5, 4, 2);
        col(r, 170, 178, 204, 255); box(r, sx, y, -5, -5, 4, 1);
        col(r, 148, 154, 180, 255); box(r, sx, y, -2, -8, 4, 5);                   /* the stump, jagged on top */
        col(r, 112, 118, 146, 255); box(r, sx, y, 1, -8, 1, 5);
        col(r, 148, 154, 180, 255); box(r, sx, y, -2, -9, 2, 1);
        col(r, 30, 30, 44, 255);    box(r, sx, y, 0, -8, 1, 2); box(r, sx, y, -1, -6, 1, 2);   /* cracks left in it */
        col(r, 134, 140, 166, 255);                                                /* rubble on the ground */
        box(r, sx, y, -10, -2, 3, 2); box(r, sx, y, 7, -1, 3, 2); box(r, sx, y, -1, -1, 2, 1); box(r, sx, y, 4, -4, 2, 2); box(r, sx, y, -9, -4, 2, 1);
        col(r, 170, 178, 204, 255); box(r, sx, y, -10, -2, 3, 1); box(r, sx, y, 7, -1, 3, 1); box(r, sx, y, 4, -4, 2, 1);
        col(r, 96, 102, 126, 255);  box(r, sx, y, 8, -9 + 8, 1, 1); box(r, sx, y, -6, -1, 1, 1);   /* chips */
        col(r, 100, 100, 124, 255); box(r, sx, y, 2, -2, 1, 1); box(r, sx, y, -4, -2, 2, 1);      /* the gem's dull shards */
        col(r, 70, 70, 92, 255);    box(r, sx, y, 2, -1, 1, 1);
        return;
    }

    col(r, 112, 118, 144, 255); box(r, sx, y, -7, -3, 14, 3);                      /* wide step */
    col(r, 158, 166, 192, 255); box(r, sx, y, -7, -3, 14, 1);
    col(r, 86, 92, 118, 255);   box(r, sx, y, -7, -1, 14, 1);
    col(r, 134, 140, 166, 255); box(r, sx, y, -5, -5, 10, 2);                      /* narrow step */
    col(r, 170, 178, 204, 255); box(r, sx, y, -5, -5, 10, 1);
    col(r, 148, 154, 180, 255); box(r, sx, y, -2, -12, 4, 7);                      /* pillar */
    col(r, 112, 118, 146, 255); box(r, sx, y, 1, -12, 1, 7);
    col(r, 186, 194, 218, 255); box(r, sx, y, -3, -13, 6, 1);                      /* cap */

    /* the gem and its glow: icy blue when whole, going dull and grey as the stone takes the blows */
    float bob = sinf(t * 2.0f) * 0.7f;
    int gy = -19 + (int)floorf(bob + 0.5f);
    int gr = mix(110, 100, p), gg = mix(224, 100, p), gb = mix(255, 124, p);
    int glow = (int)((50 + 30 * sinf(t * 3.0f)) * (1.0f - 0.7f * p));
    col(r, gr, gg, gb, glow); box(r, sx, y, -4, gy - 2, 8, 8);
    col(r, gr, gg, gb, 255);
    box(r, sx, y, -1, gy, 2, 1); box(r, sx, y, -2, gy + 1, 4, 2); box(r, sx, y, -1, gy + 3, 2, 1);
    col(r, 255, 255, 255, (int)(230 * (1.0f - p))); box(r, sx, y, -1, gy + 1, 1, 1);   /* a glint that dies out */
    col(r, gr / 2 + 40, gg / 2 + 40, gb / 2 + 40, 255);                            /* runes on the pillar echo the gem */
    box(r, sx, y, -1, -10, 2, 1); box(r, sx, y, -1, -8, 2, 1);

    if (p > 0) {                                                                   /* cracks spread with every blow */
        col(r, 30, 30, 44, 255);
        if (p > 0.12f) box(r, sx, y, 0, -12, 1, 2);
        if (p > 0.25f) box(r, sx, y, -1, -10, 1, 2);
        if (p > 0.38f) box(r, sx, y, 0, -8, 1, 2);
        if (p > 0.5f)  { box(r, sx, y, -2, -13, 2, 1); box(r, sx, y, 1, -6, 1, 1); }
        if (p > 0.62f) { box(r, sx, y, -1, -6, 1, 2); box(r, sx, y, -3, -5, 2, 1); }
        if (p > 0.74f) { box(r, sx, y, 2, -5, 2, 1); box(r, sx, y, 4, -4, 1, 2); }
        if (p > 0.85f) { box(r, sx, y, 0, gy + 1, 1, 2); box(r, sx, y, -1, gy + 2, 1, 1); }    /* the gem itself splits */
        for (int k = 0; k < 4; k++) {                                              /* chips and dust flying off */
            float ph = fmodf(t * 1.7f + k * 0.29f, 1.0f);
            if (ph > p + 0.15f) continue;
            int dir = (k & 1) ? 1 : -1;
            col(r, 190, 196, 216, (int)(230 * (1.0f - ph)));
            box(r, sx, y, dir * (3 + (int)(ph * 6.0f)) + k % 2, -9 - (int)(ph * 5.0f) + (int)(ph * ph * 12.0f), 1, 1);
        }
    }
}
