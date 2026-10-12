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
/* chunk 40: the repair. the break played backwards: the same hold, the cracks heal, the stump grows, the rubble lifts, the gem lights again */
static int   self_on;                                       /* chunk 41: the mended shrine can be used once more: a TAP with the hammer, on ourselves */
static void (*self_cb)(void);
static int   rep_on, rep_started, holding_now;
static float rprog, rtick;
static void (*rep_start_cb)(void), (*rep_done_cb)(void);

void shrine_reset(int pixel_scale, int tile_size) {
    on = enabled = polluted = in_range = 0; prog = tick = 0; done_cb = NULL;
    rep_on = rep_started = holding_now = 0; rprog = rtick = 0; rep_start_cb = rep_done_cb = NULL; self_on = 0; self_cb = NULL;
    s = pixel_scale; tile = tile_size;
}
void shrine_place(int tile_x, int tile_y) { on = 1; tx = tile_x; ty = tile_y; enabled = polluted = in_range = 0; prog = 0; rep_on = rep_started = 0; rprog = 0; self_on = 0; self_cb = NULL; }
/* the shrine as it was left by an earlier life (chunk 20): placed, switched off, and fouled for good if it was */
void shrine_restore(int tile_x, int tile_y, int was_polluted) {
    shrine_place(tile_x, tile_y);
    if (was_polluted) { polluted = 1; prog = 1.0f; }
}
void shrine_enable(void (*on_done)(void)) { if (!on || polluted) return; enabled = 1; done_cb = on_done; }

void shrine_repair_enable(void (*on_start)(void), void (*on_done)(void)) {
    if (!on || !polluted) return;
    rep_on = 1; rep_started = 0; rprog = rtick = 0; rep_start_cb = on_start; rep_done_cb = on_done;
}
int   shrine_repairing(void) { return on && rep_on && polluted; }
void shrine_self_enable(void (*on_hit)(void)) { if (!on || polluted) return; self_on = 1; self_cb = on_hit; }
int  shrine_self(void)       { return on && self_on && !polluted; }
void shrine_self_fire(void)  { if (!shrine_self()) return; self_on = 0; in_range = 0; void (*cb)(void) = self_cb; self_cb = NULL; if (cb) cb(); }
static int usable(void)      { return shrine_enabled() || shrine_repairing() || shrine_self(); }
int   shrine_exists(void)   { return on; }
int   shrine_enabled(void)  { return on && enabled && !polluted; }
int   shrine_polluted(void) { return on && polluted; }
float shrine_progress(void) { return shrine_repairing() ? rprog : prog; }
void  shrine_tile(float *x, float *y) { *x = tx + 0.5f; *y = ty + 0.9f; }

int shrine_near(float pxw, float pyw, int allow) {
    if (!usable() || !allow) { in_range = 0; return 0; }
    float dx = pxw - (tx + 0.5f) * tile, dy = pyw - (ty + 0.9f) * tile;
    float d = sqrtf(dx * dx + dy * dy) / tile;
    if (!in_range && d < RANGE) in_range = 1;
    else if (in_range && d > LEAVE) in_range = 0;
    return in_range;
}

static int repair_update(float dt, int holding) {
    holding_now = holding;
    if (holding) {
        if (!rep_started) { rep_started = 1; if (rep_start_cb) rep_start_cb(); }       /* the first time the hold starts */
        rprog += dt / SHRINE_HOLD_T;
        rtick -= dt;
        if (rtick <= 0) { rtick = 0.22f; sfx_blip(0.55f + 0.7f * rprog); }
        if (rprog >= 1.0f) {
            rprog = 0; polluted = 0; prog = 0; rep_on = 0; enabled = 0; in_range = 0; holding_now = 0;
            sfx_blip(1.9f);
            if (rep_done_cb) { void (*cb)(void) = rep_done_cb; rep_done_cb = NULL; cb(); }
            return 1;
        }
    } else {
        rtick = 0;
        if (rprog > 0) { rprog -= dt / DECAY_T; if (rprog < 0) rprog = 0; }
    }
    return 0;
}

int shrine_update(float dt, int holding) {
    if (shrine_repairing()) return repair_update(dt, holding);
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
static int g_top = -12;                                   /* chunk 40: where the pillar's top is right now (the cracks stay under it) */
static void cbox(SDL_Renderer *r, int x, int y, int ux, int uy, int uw, int uh) { if (uy >= g_top) box(r, x, y, ux, uy, uw, uh); }

void shrine_draw(SDL_Renderer *r, int cam_x, int cam_y, float t) {
    if (!on) return;
    int x = (int)((tx + 0.5f) * tile) - cam_x, y = (int)((ty + 0.9f) * tile) - cam_y;
    float p = prog;
    float rr = (rep_on && polluted) ? rprog : 0.0f;                                  /* chunk 40: how far the repair has come */
    if (rr > 0) p = 1.0f - rr;                                                      /* the cracks heal as it mends */
    float shake = ((enabled && p > 0 && !polluted) || (rr > 0 && holding_now)) ? 1.0f : 0.0f;                    /* the stone shudders under the blows */
    int sx = x + (int)floorf(sinf(t * 60.0f) * shake * 0.6f + 0.5f) * s;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    col(r, 0, 0, 0, 70);   box(r, sx, y, -8, -1, 16, 2);                           /* ground shadow */

    if (polluted && rr <= 0) {                                                     /* BROKEN: a stump, a gap in the steps, rubble, dull shards */
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

    int top = -12, step2 = 1, cap = 1; float gemk = 1.0f;
    if (rr > 0) {                                                                  /* chunk 40: mending. the stump grows first, then the narrow step and the cap, last the gem */
        float gk = rr / 0.6f; if (gk > 1) gk = 1;
        top = -8 - (int)(4.0f * gk); step2 = rr > 0.25f; cap = rr >= 0.6f;
        gemk = (rr - 0.6f) / 0.4f; if (gemk < 0) gemk = 0;
        int lift = (int)(rr * 9.0f), al = (int)(255.0f * (1.0f - rr)); float in = 1.0f - rr;      /* the rubble rises into the stone and fades */
        col(r, 134, 140, 166, al);
        box(r, sx, y, (int)(-10 * in), -2 - lift, 3, 2); box(r, sx, y, (int)(7 * in), -1 - lift, 3, 2); box(r, sx, y, (int)(-1 * in), -1 - lift, 2, 1);
        box(r, sx, y, (int)(4 * in), -4 - lift, 2, 2); box(r, sx, y, (int)(-9 * in), -4 - lift, 2, 1);
        col(r, 170, 178, 204, al);
        box(r, sx, y, (int)(-10 * in), -2 - lift, 3, 1); box(r, sx, y, (int)(7 * in), -1 - lift, 3, 1); box(r, sx, y, (int)(4 * in), -4 - lift, 2, 1);
    }
    g_top = top;
    col(r, 112, 118, 144, 255); box(r, sx, y, -7, -3, 14, 3);                      /* wide step */
    col(r, 158, 166, 192, 255); box(r, sx, y, -7, -3, 14, 1);
    col(r, 86, 92, 118, 255);   box(r, sx, y, -7, -1, 14, 1);
    if (step2) {
        col(r, 134, 140, 166, 255); box(r, sx, y, -5, -5, 10, 2);                  /* narrow step */
        col(r, 170, 178, 204, 255); box(r, sx, y, -5, -5, 10, 1);
    }
    col(r, 148, 154, 180, 255); box(r, sx, y, -2, top, 4, -5 - top);               /* pillar */
    col(r, 112, 118, 146, 255); box(r, sx, y, 1, top, 1, -5 - top);
    if (cap) { col(r, 186, 194, 218, 255); box(r, sx, y, -3, -13, 6, 1); }         /* cap */

    /* the gem and its glow: icy blue when whole, going dull and grey as the stone takes the blows */
    float bob = sinf(t * 2.0f) * 0.7f;
    int gy = -19 + (int)floorf(bob + 0.5f);
    int gr = mix(110, 100, p), gg = mix(224, 100, p), gb = mix(255, 124, p);
    int glow = (int)((50 + 30 * sinf(t * 3.0f)) * (1.0f - 0.7f * p) * gemk);
    if (gemk > 0) {
        col(r, gr, gg, gb, glow); box(r, sx, y, -4, gy - 2, 8, 8);
        col(r, gr, gg, gb, (int)(255 * gemk));
        box(r, sx, y, -1, gy, 2, 1); box(r, sx, y, -2, gy + 1, 4, 2); box(r, sx, y, -1, gy + 3, 2, 1);
        col(r, 255, 255, 255, (int)(230 * (1.0f - p) * gemk)); box(r, sx, y, -1, gy + 1, 1, 1);   /* a glint that dies out */
    }
    if (cap) {
        col(r, gr / 2 + 40, gg / 2 + 40, gb / 2 + 40, 255);                        /* runes on the pillar echo the gem */
        box(r, sx, y, -1, -10, 2, 1); box(r, sx, y, -1, -8, 2, 1);
    }

    if (p > 0) {                                                                   /* cracks spread with every blow */
        col(r, 30, 30, 44, 255);
        if (p > 0.12f) cbox(r, sx, y, 0, -12, 1, 2);
        if (p > 0.25f) cbox(r, sx, y, -1, -10, 1, 2);
        if (p > 0.38f) cbox(r, sx, y, 0, -8, 1, 2);
        if (p > 0.5f)  { cbox(r, sx, y, -2, -13, 2, 1); cbox(r, sx, y, 1, -6, 1, 1); }
        if (p > 0.62f) { cbox(r, sx, y, -1, -6, 1, 2); cbox(r, sx, y, -3, -5, 2, 1); }
        if (p > 0.74f) { cbox(r, sx, y, 2, -5, 2, 1); cbox(r, sx, y, 4, -4, 1, 2); }
        if (p > 0.85f && gemk > 0) { box(r, sx, y, 0, gy + 1, 1, 2); box(r, sx, y, -1, gy + 2, 1, 1); }    /* the gem itself splits */
        for (int k = 0; k < 4; k++) {                                              /* chips and dust flying off */
            float ph = fmodf(t * 1.7f + k * 0.29f, 1.0f);
            if (ph > p + 0.15f) continue;
            int dir = (k & 1) ? 1 : -1;
            col(r, 190, 196, 216, (int)(230 * (1.0f - ph)));
            box(r, sx, y, dir * (3 + (int)(ph * 6.0f)) + k % 2, -9 - (int)(ph * 5.0f) + (int)(ph * ph * 12.0f), 1, 1);
        }
    }
}
