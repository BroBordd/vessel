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
#define THUMP_T      0.24f      /* a thump lasts this long */
#define THUMP_BIG    0.30f      /* ... and grows the heart by this much at its peak */

static int   state, nblood;
static float st, pulse_t, burst_t;          /* st: since it appeared; pulse_t: since the last thump; burst_t: since the break */

void heart_reset(void) { state = HEART_OFF; nblood = 0; st = pulse_t = burst_t = 0; }
void heart_show(void)  { state = HEART_BEATING; nblood = 0; st = 0; pulse_t = 0; }
void heart_pulse(void) { if (state == HEART_BEATING) pulse_t = 0; }
int  heart_state(void) { return state; }
int  heart_particles(void) { return nblood; }

/* the break: a jagged crack down the heart; the halves part, tilt, sink and fade (no blood) */
static const int CRACK[HH] = { 4, 5, 4, 5, 4, 5, 4, 4 };   /* column where the right half starts, per row */
#define CRACK_T   0.16f     /* the crack shows on the whole heart for this long */
#define BREAK_END 1.5f      /* then the halves are gone */

void heart_burst(void) {
    if (state != HEART_BEATING) return;
    state = HEART_BURST; burst_t = 0; nblood = 2;           /* nblood: the two halves, for the tests */
}

void heart_update(float dt) {
    if (state == HEART_OFF) return;
    st += dt; pulse_t += dt;
    if (state != HEART_BURST) return;
    burst_t += dt;
    if (burst_t >= BREAK_END) nblood = 0;
}

static void fillr(SDL_Renderer *r, int x, int y, int w, int h) { SDL_Rect q = { x, y, w, h }; SDL_RenderFillRect(r, &q); }

/* the heart (or one half of it) at scale s, middle at cx, cy, one cell = cs screen px.
 * half: 0 = whole, -1 = left of the crack, +1 = right. lean: each row shifts sideways by lean cells per row from the
 * middle (the halves tilt away from each other). crack: paint the cells beside the crack white. */
static void draw_shape(SDL_Renderer *r, int cx, int cy, float cs, float s, int A, int half, float lean, int crack) {
    float c = cs * s;
    for (int pass = 0; pass < 2; pass++)                    /* pass 0: the dark outline, pass 1: the body */
        for (int gy = 0; gy < HH; gy++) for (int gx = 0; gx < HW; gx++) {
            if (SHAPE[gy][gx] != 'X') continue;
            if (half < 0 && gx >= CRACK[gy]) continue;
            if (half > 0 && gx < CRACK[gy]) continue;
            float sh = lean * (gy - HH / 2.0f) * c;         /* a lean: the bottom swings in, the top out */
            int x0 = cx + (int)floorf((gx - HW / 2.0f) * c + sh), x1 = cx + (int)floorf((gx + 1 - HW / 2.0f) * c + sh);
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
            if (crack && (gx == CRACK[gy] || gx == CRACK[gy] - 1)) { R = 255; G = 255; B = 255; }          /* the crack */
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
        draw_shape(r, cx, cy, cs, s, A, 0, 0, 0);
        return;
    }
    if (burst_t >= BREAK_END) return;
    if (burst_t < CRACK_T) {                                /* the flatline hits: a white crack runs down the heart */
        draw_shape(r, cx, cy, cs, 1.0f, 255, 0, 0, 1);
        return;
    }
    float t = burst_t - CRACK_T;
    float sep = cs * 3.2f * (1.0f - expf(-5.0f * t));       /* the halves part and slow down */
    float fall = cs * 9.0f * t * t;                         /* then sink */
    float lean = 0.10f * (1.0f - expf(-4.0f * t));
    float fade_from = 0.55f;
    int A = t < fade_from ? 255 : (int)(255 * (1.0f - (t - fade_from) / (BREAK_END - CRACK_T - fade_from)));
    if (A < 0) A = 0;
    draw_shape(r, cx - (int)sep, cy + (int)fall, cs, 1.0f, A, -1, -lean, 0);   /* left half: top swings out left */
    draw_shape(r, cx + (int)sep, cy + (int)fall, cs, 1.0f, A, +1, +lean, 0);
}
