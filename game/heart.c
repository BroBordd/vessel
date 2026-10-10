/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "heart.h"
#include <math.h>
#include <stdint.h>

#define HW 9
#define HH 8
static const char *const SHAPE[HH] = {      /* X = heart. 9 x 8 cells */
    ".XXX.XXX.",
    "XXXXXXXXX",
    "XXXXXXXXX",
    "XXXXXXXXX",
    ".XXXXXXX.",
    "..XXXXX..",
    "...XXX...",
    "....X....",
};
#define CELL_PER_PX  0.2f       /* one heart cell is this many sprite pixels: under 2 sprite pixels wide, a human heart in a 6 px wide, 2 row torso */
#define SPRAY_PER_PX 0.55f      /* the blood still flies as far as it did from the big heart: its cell unit for distances */
#define THUMP_T      0.24f      /* a thump lasts this long */
#define THUMP_BIG    0.30f      /* ... and grows the heart by this much at its peak */
#define FIRE_T       0.14f      /* the white flash at the burst */
#define MAX_BLOOD    140
#define GRAVITY      46.0f      /* cells / s^2 */

typedef struct { float x, y, vx, vy, age, life, size; unsigned char c; } Blood;   /* x, y: cells from the heart's middle */

static int   state, nblood;
static float st, pulse_t, burst_t;          /* st: since it appeared; pulse_t: since the last thump; burst_t: since the burst */
static Blood blood[MAX_BLOOD];
static uint32_t rng = 0x2545F491u;

static float rnd(void) {                    /* 0..1, xorshift: the same blood every time (the tests rely on it) */
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng & 0xFFFFFF) / (float)0x1000000;
}

void heart_reset(void) { state = HEART_OFF; nblood = 0; st = pulse_t = burst_t = 0; }
void heart_show(void)  { state = HEART_BEATING; nblood = 0; st = 0; pulse_t = 0; }
void heart_pulse(void) { if (state == HEART_BEATING) pulse_t = 0; }
int  heart_state(void) { return state; }
int  heart_particles(void) { return nblood; }

void heart_burst(void) {
    if (state != HEART_BEATING) return;
    state = HEART_BURST; burst_t = 0; nblood = 0;
    for (int i = 0; i < MAX_BLOOD; i++) {
        int gx, gy;                                         /* start on a random cell of the heart, so it bursts out of its shape */
        do { gx = (int)(rnd() * HW); gy = (int)(rnd() * HH); } while (gx >= HW || gy >= HH || SHAPE[gy][gx] != 'X');
        Blood *b = &blood[nblood++];
        b->x = gx + 0.5f - HW / 2.0f; b->y = gy + 0.5f - HH / 2.0f;
        float a = atan2f(b->y - 0.3f, b->x) + (rnd() - 0.5f) * 1.1f;     /* mostly away from the middle */
        float sp = 9.0f + rnd() * 30.0f;
        b->vx = cosf(a) * sp; b->vy = sinf(a) * sp - 8.0f;               /* a little lift: it sprays up, then falls */
        b->age = 0; b->life = 1.0f + rnd() * 1.4f;
        b->size = rnd() < 0.35f ? 1.0f : 0.5f;                           /* some chunks are a whole cell, most are splashes */
        b->c = (unsigned char)(rnd() * 4.0f); if (b->c > 3) b->c = 3;
    }
}

void heart_update(float dt) {
    if (state == HEART_OFF) return;
    st += dt; pulse_t += dt;
    if (state != HEART_BURST) return;
    burst_t += dt;
    int keep = 0;
    for (int i = 0; i < nblood; i++) {
        Blood b = blood[i];
        b.age += dt;
        if (b.age >= b.life) continue;
        float drag = expf(-1.6f * dt);
        b.vx *= drag; b.vy = b.vy * drag + GRAVITY * dt;
        b.x += b.vx * dt; b.y += b.vy * dt;
        blood[keep++] = b;
    }
    nblood = keep;
}

static void fillr(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y, w, h }; SDL_RenderFillRect(r, &q); }

/* the heart at scale s (1 = resting), middle at cx, cy, one cell = cs screen px. alpha 0..255 */
static void draw_shape(SDL_Renderer *r, int cx, int cy, float cs, float s, int A, int flash) {
    float c = cs * s;
    for (int pass = 0; pass < 2; pass++)                    /* pass 0: the dark outline (every cell and its neighbours), pass 1: the body */
        for (int gy = 0; gy < HH; gy++) for (int gx = 0; gx < HW; gx++) {
            if (SHAPE[gy][gx] != 'X') continue;
            int x0 = cx + (int)floorf((gx - HW / 2.0f) * c), x1 = cx + (int)floorf((gx + 1 - HW / 2.0f) * c);
            int y0 = cy + (int)floorf((gy - HH / 2.0f) * c), y1 = cy + (int)floorf((gy + 1 - HH / 2.0f) * c);
            if (pass == 0) {
                int o = (int)(c * 0.5f); if (o < 1) o = 1;
                SDL_SetRenderDrawColor(r, 50, 0, 8, A);
                fillr(r, x0 - o, y0 - o, x1 - x0 + 2 * o, y1 - y0 + 2 * o);
                continue;
            }
            int R = 226, G = 32, B = 52;                    /* body */
            if (gy >= 5 || gx >= 7) { R = 170; G = 16; B = 36; }                         /* the shaded underside */
            if ((gy == 1 && (gx == 1 || gx == 2)) || (gy == 2 && gx == 1)) { R = 255; G = 150; B = 160; }   /* a shine */
            if (flash) { R = 255; G = 255; B = 255; }
            SDL_SetRenderDrawColor(r, R, G, B, A);
            fillr(r, x0, y0, x1 - x0, y1 - y0);
        }
}

void heart_draw(SDL_Renderer *r, int cx, int cy, int pixel) {
    if (state == HEART_OFF) return;
    float cs = pixel * CELL_PER_PX; if (cs < 2.0f) cs = 2.0f;
    if (state == HEART_BEATING) {
        float k = 1.0f - pulse_t / THUMP_T; if (k < 0) k = 0;
        float s = 1.0f + THUMP_BIG * k * k;
        int A = st < 0.12f ? (int)(255 * st / 0.12f) : 255;
        draw_shape(r, cx, cy, cs, s, A, 0);
        return;
    }
    if (burst_t < FIRE_T) {                                 /* the flash: the heart, bigger, white, fading */
        float k = burst_t / FIRE_T;
        draw_shape(r, cx, cy, cs, 1.3f + 0.7f * k, (int)(255 * (1.0f - k)), 1);
    }
    static const unsigned char PAL[4][3] = { { 110, 0, 12 }, { 166, 8, 24 }, { 222, 28, 44 }, { 250, 76, 80 } };
    for (int i = 0; i < nblood; i++) {
        const Blood *b = &blood[i];
        float left = 1.0f - b->age / b->life;
        int A = left < 0.3f ? (int)(255 * left / 0.3f) : 255;
        SDL_SetRenderDrawColor(r, PAL[b->c][0], PAL[b->c][1], PAL[b->c][2], A);
        int sz = (int)(pixel * 0.4f * b->size * 1.4f + 0.5f); if (sz < 2) sz = 2;     /* splashes of about half a sprite pixel, chunks a little over one */
        fillr(r, cx + (int)(b->x * pixel * SPRAY_PER_PX) - sz / 2, cy + (int)(b->y * pixel * SPRAY_PER_PX) - sz / 2, sz, sz);
    }
}
