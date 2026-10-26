/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the dead, see bodies.h. */
#include "bodies.h"
#include "char.h"
#include "story.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static Body list[BODIES_MAX];
static int  n;

#define TRAIL_MAX 220
static struct { float tx, ty, age; unsigned char shape; } trail[TRAIL_MAX];     /* a ring: the newest overwrites the oldest */
static int trail_n, trail_head;
static struct { float tx, ty; } pool[BODIES_MAX]; static int pool_n;     /* the pools of blood: the floor's, not the body's (a pulled body leaves its pool, a body in the pit leaves it too) */
static int pit_t0 = -1, pit_t1 = -1, pit_u0 = -1, pit_u1 = -1;          /* the pit's box in tiles (x0, y0, x1, y1 as t0, u0, t1, u1), -1: none */
static unsigned trail_seed = 12345u;

void bodies_reset(void) { n = 0; memset(list, 0, sizeof list); trail_n = trail_head = 0; pool_n = 0; }
int  bodies_add(unsigned vessel, float tx, float ty, const unsigned char *inv) {
    if (n >= BODIES_MAX) return -1;
    Body *b = &list[n];
    memset(b, 0, sizeof *b);
    b->vessel = vessel; b->tx = tx; b->ty = ty;
    if (pool_n < BODIES_MAX) { pool[pool_n].tx = tx; pool[pool_n].ty = ty; pool_n++; }
    if (inv) memcpy(b->inv, inv, ITEM_COUNT);
    return n++;
}
int bodies_count(void) { return n; }
const Body *bodies_get(int i) { return i >= 0 && i < n ? &list[i] : NULL; }
const Body *bodies_find(unsigned vessel) { for (int i = 0; i < n; i++) if (list[i].vessel == vessel) return &list[i]; return NULL; }

int bodies_take_all(unsigned vessel) {
    const Body *cb = bodies_find(vessel); Body *b = (Body *)cb;
    if (!b) return 0;
    int n = 0;
    for (int k = 0; k < ITEM_COUNT; k++) while (b->inv[k]) { if (item_size(k) == SIZE_MEDIUM && item_medium_held()) break; item_give(k); b->inv[k]--; n++; }
    return n;
}
int bodies_take_kind(unsigned vessel, int kind) {
    Body *b = (Body *)bodies_find(vessel);
    if (!b || kind < 0 || kind >= ITEM_COUNT) return 0;
    int n = 0;
    while (b->inv[kind]) { if (item_size(kind) == SIZE_MEDIUM && item_medium_held()) break; item_give(kind); b->inv[kind]--; n++; }
    return n;
}
/* story chunk 20 (A4): put a whole stack of ours into a body's pockets. the orb and the hammer stay with us (the story needs them). returns how many moved */
int bodies_give(unsigned vessel, int kind) {
    Body *b = (Body *)bodies_find(vessel);
    if (!b || kind < 0 || kind >= ITEM_COUNT || kind == ITEM_ORB || kind == ITEM_HAMMER) return 0;
    int n = 0;
    while (item_count(kind) > 0 && b->inv[kind] < 255) { item_take(kind); b->inv[kind]++; n++; }
    return n;
}
int bodies_remove(unsigned vessel) {
    if (vessel == 1) return 0;
    for (int i = 0; i < n; i++) if (list[i].vessel == vessel) { memmove(&list[i], &list[i + 1], sizeof(Body) * (size_t)(n - i - 1)); n--; return 1; }
    return 0;
}
int bodies_cut(unsigned vessel) { Body *b = (Body *)bodies_find(vessel); if (!b || b->cuts >= BODIES_CUTS) return 0; return ++b->cuts; }
void bodies_cut_done(unsigned vessel) { Body *b = (Body *)bodies_find(vessel); if (b) b->cuts = BODIES_CUTS; }
void bodies_inv_text(const Body *b, char *out, int cap) {
    if (!out || cap < 1) return;
    out[0] = 0;
    if (!b) return;
    int k = 0, have[ITEM_COUNT];
    for (int i = 0; i < ITEM_COUNT; i++) if (b->inv[i]) have[k++] = i;
    if (k == 0) { snprintf(out, (size_t)cap, "has nothing"); return; }
    int len = snprintf(out, (size_t)cap, "has");
    for (int i = 0; i < k && len < cap - 1; i++) {
        const char *sep = i == 0 ? " a " : i == k - 1 ? " and a " : ", a ";
        len += snprintf(out + len, (size_t)(cap - len), "%s", sep);
        for (const char *s = item_name(have[i]); *s && len < cap - 1; s++) out[len++] = (char)tolower((unsigned char)*s);
        out[len] = 0;
    }
}

static void box(const Body *b, int px, int tile, float *x0, float *y0, float *x1, float *y1) {
    float fx = b->tx * (float)tile, fy = b->ty * (float)tile;
    *x0 = fx - 6.0f * (float)px; *x1 = fx + 6.0f * (float)px; *y0 = fy - 9.0f * (float)px; *y1 = fy;
}

int bodies_hit(float fx, float fy, int px, int tile) {
    for (int i = 0; i < n; i++) {
        if (list[i].vessel == 1 || list[i].falling) continue;
        float x0, y0, x1, y1; box(&list[i], px, tile, &x0, &y0, &x1, &y1);
        if (fx + 3.0f * px > x0 && fx - 3.0f * px < x1 && fy > y0 && fy - 3.0f * px < y1) return 1;
    }
    return 0;
}

int bodies_nearest(float fx, float fy, int px, int tile, float reach, float *gap) {
    int best = -1; float bg = reach;
    for (int i = 0; i < n; i++) {
        if (list[i].vessel == 1 || list[i].falling) continue;
        float x0, y0, x1, y1; box(&list[i], px, tile, &x0, &y0, &x1, &y1);
        float dx = fmaxf(fmaxf(x0 - (fx + 3.0f * px), (fx - 3.0f * px) - x1), 0.0f), dy = fmaxf(fmaxf(y0 - fy, (fy - 3.0f * px) - y1), 0.0f);
        float g = sqrtf(dx * dx + dy * dy) / (float)px;
        if (g <= bg) { bg = g; best = i; }
    }
    if (best >= 0 && gap) *gap = bg;
    return best;
}

void bodies_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken) {
    float k = 1.0f - darken; if (k < 0) k = 0;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < pool_n; i++) {                                              /* the pools first, all of them, on the floor where they dried: a body dragged over a pool lies on it */
        int fx = (int)lroundf(pool[i].tx * (float)tile) - cam_x, fy = (int)lroundf(pool[i].ty * (float)tile) - cam_y;
        SDL_SetRenderDrawColor(r, (Uint8)(104 * k), (Uint8)(14 * k), (Uint8)(22 * k), 215);   /* a flat ellipse of blood, a little bigger than he is */
        for (int j = -4; j <= 4; j++) {
            int w = (int)(sqrtf(1.0f - (float)(j * j) / 25.0f) * 11.0f);
            SDL_Rect q = { fx - w * px, fy - 4 * px + j * px, 2 * w * px, px };
            SDL_RenderFillRect(r, &q);
        }
    }
    for (int i = 0; i < n; i++) {
        const Body *b = &list[i];
        if (b->vessel == 1) continue;
        int fx = (int)lroundf(b->tx * (float)tile) - cam_x, fy = (int)lroundf(b->ty * (float)tile) - cam_y;
        float d = darken;
        int clipped = 0;
        if (b->falling) {                                                           /* the swoop: it slides on a little the way it was going, then drops, faster and faster, into the dark */
            float u = b->fall_t / BODIES_FALL_T; if (u > 1.0f) u = 1.0f;
            float v = (u - 0.12f) / 0.88f; if (v < 0) v = 0;                         /* a beat of hanging in the air first */
            float slide = 1.0f - (1.0f - u) * (1.0f - u);
            fx += (int)lroundf((b->fall_dx * 5.0f * slide + sinf(b->fall_t * 17.0f) * 0.7f * v) * (float)px);
            fy += (int)lroundf((b->fall_dy * 5.0f * slide + 38.0f * v * v) * (float)px);
            d = darken + (1.0f - darken) * 0.92f * v;
            if (pit_t0 >= 0) {                                                       /* it is inside the crack: it goes under the lips, not over the floor beyond */
                SDL_Rect cl = { pit_t0 * tile - cam_x, pit_u0 * tile - cam_y, (pit_t1 - pit_t0 + 1) * tile, (pit_u1 - pit_u0 + 1) * tile };
                SDL_RenderSetClipRect(r, &cl); clipped = 1;
            }
        }
        char_set_darken(d);
        char_draw_lying(r, story_vessel_look(b->vessel), fx, fy, px);
        char_set_darken(0);
        if (clipped) SDL_RenderSetClipRect(r, NULL);
    }
}

int bodies_box(unsigned vessel, int px, int tile, float *x0, float *y0, float *x1, float *y1) {
    if (vessel == 1) return 0;
    for (int i = 0; i < n; i++) if (list[i].vessel == vessel && !list[i].falling) { box(&list[i], px, tile, x0, y0, x1, y1); return 1; }
    return 0;
}
void bodies_shift(unsigned vessel, float dx, float dy, int tile) {
    if (vessel == 1 || tile <= 0) return;
    for (int i = 0; i < n; i++) if (list[i].vessel == vessel && !list[i].falling) { list[i].tx += dx / (float)tile; list[i].ty += dy / (float)tile; return; }
}

void bodies_trail_add(float wx, float wy, int tile) {
    if (tile <= 0) return;
    trail_seed = trail_seed * 1664525u + 1013904223u;                           /* the shape of each smear: width, height and a nudge, from a small generator */
    trail[trail_head].tx = wx / (float)tile; trail[trail_head].ty = wy / (float)tile; trail[trail_head].age = 0;
    trail[trail_head].shape = (unsigned char)(trail_seed >> 24);
    trail_head = (trail_head + 1) % TRAIL_MAX;
    if (trail_n < TRAIL_MAX) trail_n++;
}
void bodies_trail_update(float dt) {
    if (dt <= 0) return;
    for (int i = 0; i < trail_n; i++) trail[i].age += dt;
}
void bodies_trail_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken) {
    float k = 1.0f - darken; if (k < 0) k = 0;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < trail_n; i++) {
        float f = 1.0f - trail[i].age / BODIES_TRAIL_LIFE;
        if (f <= 0) continue;
        int sh = trail[i].shape, w = 2 + (sh & 3), h = 2 + ((sh >> 2) & 1), ox = ((sh >> 3) & 3) - 1, oy = ((sh >> 5) & 1) - 1;
        int x = (int)lroundf(trail[i].tx * (float)tile) - cam_x + (ox - w / 2) * px, y = (int)lroundf(trail[i].ty * (float)tile) - cam_y + (oy - h / 2) * px;
        SDL_SetRenderDrawColor(r, (Uint8)(96 * k), (Uint8)(12 * k), (Uint8)(20 * k), (Uint8)(210.0f * f));
        SDL_Rect q = { x, y, w * px, h * px };
        SDL_RenderFillRect(r, &q);
    }
}

void bodies_set_pit(int tx0, int ty0, int tx1, int ty1) { pit_t0 = tx0; pit_u0 = ty0; pit_t1 = tx1; pit_u1 = ty1; }
void bodies_fall(unsigned vessel, float dir_x, float dir_y) {
    Body *b = (Body *)bodies_find(vessel);
    if (!b || vessel == 1 || b->falling) return;
    b->falling = 1; b->fall_t = 0; b->fall_dx = dir_x; b->fall_dy = dir_y;
}
void bodies_update(float dt, void (*gone)(unsigned vessel)) {
    if (dt <= 0) return;
    for (int i = 0; i < n; i++) {
        if (!list[i].falling) continue;
        list[i].fall_t += dt;
        if (list[i].fall_t < BODIES_FALL_T) continue;
        unsigned v = list[i].vessel;
        bodies_remove(v);                                                           /* gone (its pool stays) */
        if (gone) gone(v);
        i--;                                                                        /* the list shifted */
    }
}
