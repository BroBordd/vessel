/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "gore.h"
#include <math.h>

#define MAX_DROPS   128
#define MAX_STAINS  320
#define GRAVITY     16.0f       /* tiles per second squared */

typedef struct { float x, y, z, vx, vy, vz; int size, shade, live; } Drop;
typedef struct { int x, y, size, shade; } Stain;

static Drop  drops[MAX_DROPS];
static Stain stains[MAX_STAINS];
static int   n_stains, s = 3, tile = 48;
static int (*floor_cb)(int, int);
static unsigned seed = 0x2545F491u;

/* the head */
static struct { int on, settled, bounces; const Person *who; float x, y, z, vx, vy, vz; } head;

static const Uint8 SHADES[4][3] = { { 150, 14, 26 }, { 178, 22, 34 }, { 120, 8, 20 }, { 200, 36, 44 } };

static float rnd(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return (float)(seed & 0xffffu) / 65536.0f; }
static int   floor_at(float x, float y) { return !floor_cb || floor_cb((int)floorf(x / tile), (int)floorf(y / tile)); }

void gore_reset(int pixel_scale, int tile_size) {
    s = pixel_scale; tile = tile_size; n_stains = 0;
    for (int i = 0; i < MAX_DROPS; i++) drops[i].live = 0;
    head.on = head.settled = 0;
}
void gore_set_floor(int (*floor_ok)(int, int)) { floor_cb = floor_ok; }

static void add_stain(float x, float y, int size, int shade) {
    if (n_stains >= MAX_STAINS) { for (int i = 1; i < MAX_STAINS; i++) stains[i - 1] = stains[i]; n_stains = MAX_STAINS - 1; }   /* the oldest fades out of the list */
    stains[n_stains++] = (Stain){ (int)x, (int)y, size, shade };
}

void gore_burst(float x, float y, float height, float dir_x, float dir_y) {
    int made = 0;
    for (int i = 0; i < MAX_DROPS && made < 90; i++) {
        if (drops[i].live) continue;
        float ang = rnd() * 6.2831853f, sp = (0.8f + rnd() * 4.2f) * tile;
        drops[i] = (Drop){ x, y, height, cosf(ang) * sp + dir_x * 1.6f * tile, sinf(ang) * sp * 0.8f + dir_y * 1.6f * tile,
                           (2.0f + rnd() * 4.0f) * tile, rnd() < 0.3f ? 2 * s : s, (int)(rnd() * 4.0f) & 3, 1 };
        made++;
    }
}

void gore_head_launch(const Person *who, float x, float y, float height, float dir_x, float dir_y) {
    head.on = 1; head.settled = 0; head.bounces = 0; head.who = who;
    head.x = x; head.y = y; head.z = height;
    head.vx = dir_x * 3.2f * tile; head.vy = dir_y * 3.2f * tile; head.vz = 4.2f * tile;
}

void gore_update(float dt) {
    for (int i = 0; i < MAX_DROPS; i++) {
        Drop *d = &drops[i];
        if (!d->live) continue;
        d->x += d->vx * dt; d->y += d->vy * dt; d->z += d->vz * dt; d->vz -= GRAVITY * tile * dt;
        if (d->z > 0) continue;
        d->live = 0;                                               /* landed */
        if (!floor_at(d->x, d->y)) continue;                       /* on a wall or a chair: gone */
        add_stain(d->x, d->y, d->size, d->shade);
        if (rnd() < 0.3f) add_stain(d->x + (rnd() < 0.5f ? -s : s), d->y + (rnd() < 0.5f ? -s : s), s, d->shade);   /* a little splash round it */
    }
    if (head.on && !head.settled) {
        float nx = head.x + head.vx * dt, ny = head.y + head.vy * dt;
        if (floor_at(nx, ny)) { head.x = nx; head.y = ny; } else { head.vx = head.vy = 0; }   /* a wall stops it */
        head.z += head.vz * dt; head.vz -= GRAVITY * tile * dt;
        if (head.z <= 0) {                                         /* lands: a bounce, a smear of blood, then it lies still */
            head.z = 0;
            add_stain(head.x, head.y, 2 * s, 0); add_stain(head.x - 2 * s, head.y + s, s, 2);
            if (head.vz > -1.2f * tile || head.bounces >= 2) { head.settled = 1; head.vx = head.vy = head.vz = 0; }
            else { head.vz = -head.vz * 0.35f; head.vx *= 0.55f; head.vy *= 0.55f; head.bounces++; }
        }
    }
}

static void sq(SDL_Renderer *r, int shade, int x, int y, int size) {
    SDL_SetRenderDrawColor(r, SHADES[shade][0], SHADES[shade][1], SHADES[shade][2], 255);
    SDL_Rect q = { x, y, size, size };
    SDL_RenderFillRect(r, &q);
}

void gore_draw_floor(SDL_Renderer *r, int cx, int cy) {
    for (int i = 0; i < n_stains; i++) sq(r, stains[i].shade, stains[i].x - cx - stains[i].size / 2, stains[i].y - cy - stains[i].size / 2, stains[i].size);
}

void gore_draw_air(SDL_Renderer *r, int cx, int cy) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].live) continue;
        sq(r, drops[i].shade, (int)drops[i].x - cx - drops[i].size / 2, (int)(drops[i].y - drops[i].z) - cy - drops[i].size / 2, drops[i].size);
    }
    if (head.on) {
        int hx = (int)head.x - cx, hy = (int)head.y - cy;
        float k = head.z / (4.0f * tile); if (k > 1) k = 1;
        int sw = (int)((6.0f - 2.0f * k) * s);                       /* the shadow shrinks with height */
        SDL_SetRenderDrawColor(r, 0, 0, 0, 80);
        SDL_Rect sh = { hx - sw / 2, hy - s / 2, sw, s }; SDL_RenderFillRect(r, &sh);
        char_draw_head(r, head.who, hx, hy - (int)head.z - 4 * s, s);
    }
}

int  gore_stain_count(void)  { return n_stains; }
int  gore_drops_flying(void) { int c = 0; for (int i = 0; i < MAX_DROPS; i++) c += drops[i].live; return c; }
int  gore_head_exists(void)  { return head.on; }
int  gore_head_settled(void) { return head.on && head.settled; }
void gore_head_pos(float *x, float *y) { *x = head.x; *y = head.y; }
int  gore_busy(void)         { return gore_drops_flying() > 0 || (head.on && !head.settled); }
