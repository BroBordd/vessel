/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "space.h"
#include <math.h>
#include <stdint.h>

#define NSNOW   170
#define TWO_PI  6.2831853f

typedef struct { float x, y, vx, vy, z, ph, fr; } Flake;

static Flake  fl[NSNOW];
static int    W, H;
static float  u, t;                 /* u: px per "design unit", t: sim time (s) */

static uint32_t rng = 2463534242u;
static float frand(void) {          /* xorshift32 -> [0,1) */
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng & 0xFFFFFF) / 16777216.0f;
}

/* leftward wind speed (px/s): steady breeze + two slow sine gusts, never reverses */
static float wind(float time) {
    float g = 70.0f
            + 40.0f * sinf(TWO_PI * time / 7.0f)
            + 18.0f * sinf(TWO_PI * time / 3.1f + 1.7f);
    return -g * u;
}

static int flake_size(const Flake *f) {
    int s = (int)(u * (1.0f + 3.2f * f->z) + 0.5f);
    return s < 1 ? 1 : s;
}

void space_init(int w, int h) {
    W = w; H = h;
    u = (w < h ? w : h) / 360.0f;
    t = 0;

    for (int i = 0; i < NSNOW; i++) {
        float r = frand();
        Flake *f = &fl[i];
        f->z  = 0.2f + 0.8f * r * r;            /* many far/small, few near/big */
        f->x  = frand() * W;
        f->y  = frand() * H;
        f->ph = frand() * TWO_PI;
        f->fr = 0.8f + 1.2f * frand();
        f->vx = wind(0) * f->z;
        f->vy = (30.0f + 60.0f * f->z) * u;
    }
}

void space_update(float dt) {
    t += dt;
    float w = wind(t);
    for (int i = 0; i < NSNOW; i++) {
        Flake *f = &fl[i];
        float z = f->z;

        /* target velocity: wind scaled by depth (parallax) + lateral sway; terminal fall speed + bob */
        float tvx = w * (0.35f + 0.65f * z)
                  + sinf(t * f->fr + f->ph) * 14.0f * u * (1.2f - z);
        float tvy = (30.0f + 60.0f * z) * u
                  + cosf(t * f->fr * 0.7f + f->ph) * 6.0f * u;

        /* first-order drag: v += (target - v) * (1 - e^(-k dt)); light flakes react faster */
        float k = 1.0f - expf(-dt * (3.2f - 2.2f * z));
        f->vx += (tvx - f->vx) * k;
        f->vy += (tvy - f->vy) * k;
        f->x  += f->vx * dt;
        f->y  += f->vy * dt;

        int s = flake_size(f);
        if (f->y > H)       { f->y = (float)-s; f->x = frand() * (W + s); }
        if (f->x < -s)      { f->x = W + s * 0.5f; f->y = frand() * H; }
        if (f->x > W + s)   { f->x = (float)-s; }
    }
}

void space_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    for (int i = 0; i < NSNOW; i++) {
        const Flake *f = &fl[i];
        int g = (int)(60 + 195 * f->z), s = flake_size(f);
        SDL_Rect q = { (int)f->x, (int)f->y, s, s };
        SDL_SetRenderDrawColor(r, g, g, g, 255);
        SDL_RenderFillRect(r, &q);
    }
}
