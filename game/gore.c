/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "gore.h"
#include "audio.h"
#include <math.h>

#define MAX_DROPS   200
#define MAX_STAINS  320
#define GRAVITY     16.0f       /* tiles per second squared */

typedef struct { float x, y, z, vx, vy, vz; int size, shade, live, flesh; } Drop;      /* flesh: 0 = blood, else 1..5 = a chunk of skin, shirt, trousers, hair, boots */
typedef struct { int x, y, size, shade; } Stain;

static Drop  drops[MAX_DROPS];
static Stain stains[MAX_STAINS];
#define MAX_FLESH 48
typedef struct { int x, y, size, kind; } Flesh;           /* a chunk of the body that landed: it never goes (only gore_reset, a new map, clears them) */
static Flesh flesh[MAX_FLESH];
static int   n_flesh;
static const Person *flesh_who;
static float flesh_ox, flesh_oy;                          /* where the body burst: a chunk that lands in a wall falls back towards it */
static int   n_stains, s = 3, tile = 48;
static int (*floor_cb)(int, int);
static unsigned seed = 0x2545F491u;

/* the head */
static struct { int on, settled, bounces; const Person *who; float x, y, z, vx, vy, vz; } head;

/* chunk 38.1: the sparks */
#define MAX_SPARKS     48
#define SPARKS_N       12       /* squares per burst */
#define SPARK_SQUEEZE  0.12f    /* the twitch before the pfft */
#define SPARK_SHAKE_T  0.20f    /* the body shakes this long on a burst */
typedef struct { float x, y, z, vx, life, max; int size, shade, live; } Spark;
static Spark sparks[MAX_SPARKS];
static struct { int on, enabled, bursts; float x, y, z, dx, wait, shake, squeeze; } sp;

static const Uint8 SHADES[4][3] = { { 150, 14, 26 }, { 178, 22, 34 }, { 120, 8, 20 }, { 200, 36, 44 } };

static float rnd(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return (float)(seed & 0xffffu) / 65536.0f; }
static int   floor_at(float x, float y) { return !floor_cb || floor_cb((int)floorf(x / tile), (int)floorf(y / tile)); }

void gore_reset(int pixel_scale, int tile_size) {
    s = pixel_scale; tile = tile_size; n_stains = 0; n_flesh = 0;
    for (int i = 0; i < MAX_DROPS; i++) drops[i].live = 0;
    head.on = head.settled = 0;
    for (int i = 0; i < MAX_SPARKS; i++) sparks[i].live = 0;
    sp.on = sp.enabled = sp.bursts = 0; sp.shake = sp.squeeze = 0;
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
                           (2.0f + rnd() * 4.0f) * tile, rnd() < 0.3f ? 2 * s : s, (int)(rnd() * 4.0f) & 3, 1, 0 };
        made++;
    }
}

void gore_explode(float x, float y, float height, const Person *who) {
    int made = 0;
    for (int i = 0; i < MAX_DROPS && made < 120; i++) {
        if (drops[i].live) continue;
        float ang = rnd() * 6.2831853f, sp = (0.8f + rnd() * 6.0f) * tile;
        float big = rnd();
        drops[i] = (Drop){ x, y, height * (0.4f + rnd() * 1.2f), cosf(ang) * sp, sinf(ang) * sp * 0.8f,
                           (2.5f + rnd() * 6.0f) * tile, big < 0.12f ? 3 * s : big < 0.5f ? 2 * s : s, (int)(rnd() * 4.0f) & 3, 1, 0 };
        made++;
    }
    flesh_who = who; flesh_ox = x; flesh_oy = y;                   /* the body itself: chunks flung far and wide */
    for (int k = 0, i = 0; k < 30 && who; k++) {
        while (i < MAX_DROPS && drops[i].live) i++;
        if (i >= MAX_DROPS) break;
        float ang = rnd() * 6.2831853f, sp = (1.5f + rnd() * 6.5f) * tile, big = rnd();
        drops[i] = (Drop){ x, y, height * (0.4f + rnd() * 1.2f), cosf(ang) * sp, sinf(ang) * sp * 0.8f,
                           (3.0f + rnd() * 6.0f) * tile, big < 0.25f ? 4 * s : big < 0.65f ? 3 * s : 2 * s, 0, 1, 1 + k % 5 };
        i++;
    }
}

void gore_head_launch(const Person *who, float x, float y, float height, float dir_x, float dir_y) {
    head.on = 1; head.settled = 0; head.bounces = 0; head.who = who;
    head.x = x; head.y = y; head.z = height;
    head.vx = dir_x * 3.2f * tile; head.vy = dir_y * 3.2f * tile; head.vz = 4.2f * tile;
}

void gore_sparks_start(float x, float y, float height, float dir_x, float delay) {
    sp.on = 1; sp.x = x; sp.y = y; sp.z = height; sp.dx = dir_x < 0 ? -1.0f : 1.0f; sp.wait = delay; sp.shake = sp.squeeze = 0;
}
void gore_sparks_enable(int on) { sp.enabled = on != 0; }
int  gore_sparks_running(void)  { return sp.on && sp.enabled; }
int  gore_spark_bursts(void)    { return sp.bursts; }
int  gore_sparks_flying(void)   { int c = 0; for (int i = 0; i < MAX_SPARKS; i++) c += sparks[i].live; return c; }
void gore_spark_shake(int *ox, int *oy) {
    *ox = *oy = 0;
    if (!sp.on) return;
    if (sp.shake > 0)        { *ox = ((int)(sp.shake / 0.035f) & 1 ? -s : s); *oy = ((int)(sp.shake / 0.035f) & 1 ? 0 : -s); }   /* the burst: a jolt, left-right and up */
    else if (sp.squeeze > 0) { *ox = ((int)(sp.squeeze / 0.05f) & 1 ? -s : 0); }                                              /* the squeeze: a small twitch */
}
static void spark_burst(void) {
    int made = 0;
    for (int i = 0; i < MAX_SPARKS && made < SPARKS_N; i++) {
        if (sparks[i].live) continue;
        float life = 0.25f + rnd() * 0.17f;
        sparks[i] = (Spark){ sp.x, sp.y, sp.z + (rnd() - 0.5f) * s, sp.dx * (3.5f + rnd() * 5.0f) * tile, life, life, rnd() < 0.25f ? 2 * s : s, (int)(rnd() * 4.0f) & 3, 1 };
        made++;
    }
    sp.bursts++;
    sp.shake = SPARK_SHAKE_T; sp.squeeze = 0;
    sfx_pfft();
}
static void sparks_update(float dt) {
    for (int i = 0; i < MAX_SPARKS; i++) {                                  /* the squares fly straight and shrink at the end */
        Spark *k = &sparks[i];
        if (!k->live) continue;
        k->x += k->vx * dt; k->life -= dt;
        if (k->life <= 0) k->live = 0;
    }
    if (!sp.on || !sp.enabled) { sp.shake = sp.squeeze = 0; return; }
    if (sp.shake > 0)   { sp.shake -= dt; if (sp.shake < 0) sp.shake = 0; }
    if (sp.squeeze > 0) { sp.squeeze -= dt; if (sp.squeeze < 0) sp.squeeze = 0; }
    sp.wait -= dt;
    if (sp.wait <= SPARK_SQUEEZE && sp.squeeze <= 0 && sp.wait > 0 && sp.shake <= 0) sp.squeeze = sp.wait;   /* the twitch runs up to the pfft */
    if (sp.wait <= 0) { spark_burst(); sp.wait = 1.0f + rnd() * 0.6f; }
}

void gore_update(float dt) {
    sparks_update(dt);
    for (int i = 0; i < MAX_DROPS; i++) {
        Drop *d = &drops[i];
        if (!d->live) continue;
        d->x += d->vx * dt; d->y += d->vy * dt; d->z += d->vz * dt; d->vz -= GRAVITY * tile * dt;
        if (d->z > 0) continue;
        d->live = 0;                                               /* landed */
        if (d->flesh) {                                            /* a chunk of the body: it stays (back towards the burst if it hit a wall) */
            for (int k = 0; k < 8 && !floor_at(d->x, d->y); k++) { d->x = (d->x + flesh_ox) * 0.5f; d->y = (d->y + flesh_oy) * 0.5f; }
            if (n_flesh < MAX_FLESH) flesh[n_flesh++] = (Flesh){ (int)d->x, (int)d->y, d->size, d->flesh };
            add_stain(d->x, d->y, d->size, 0);                     /* and the blood it leaves under it */
            continue;
        }
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

static Rgb flesh_rgb(int kind) {
    const Person *p = flesh_who;
    if (!p) return (Rgb){ 150, 14, 26 };
    switch (kind) { case 1: return p->skin; case 2: return p->shirt; case 3: return p->pants; case 4: return p->hair; default: return p->boots; }
}
/* a chunk: its own colour with a dark red cut edge (a raw pixel at the corner when it is small) */
static void flesh_sq(SDL_Renderer *r, int kind, int x, int y, int size) {
    Rgb c = flesh_rgb(kind);
    SDL_SetRenderDrawColor(r, SHADES[2][0], SHADES[2][1], SHADES[2][2], 255);
    SDL_Rect o = { x, y, size, size }; SDL_RenderFillRect(r, &o);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
    if (size >= 3 * s) { SDL_Rect i = { x + s, y + s, size - 2 * s, size - 2 * s }; SDL_RenderFillRect(r, &i); }
    else               { SDL_Rect i = { x, y, size - s, size - s }; SDL_RenderFillRect(r, &i); }
}
static void sq(SDL_Renderer *r, int shade, int x, int y, int size) {
    SDL_SetRenderDrawColor(r, SHADES[shade][0], SHADES[shade][1], SHADES[shade][2], 255);
    SDL_Rect q = { x, y, size, size };
    SDL_RenderFillRect(r, &q);
}

void gore_draw_floor(SDL_Renderer *r, int cx, int cy) {
    for (int i = 0; i < n_stains; i++) sq(r, stains[i].shade, stains[i].x - cx - stains[i].size / 2, stains[i].y - cy - stains[i].size / 2, stains[i].size);
    for (int i = 0; i < n_flesh; i++) flesh_sq(r, flesh[i].kind, flesh[i].x - cx - flesh[i].size / 2, flesh[i].y - cy - flesh[i].size / 2, flesh[i].size);   /* the body's chunks, over the blood */
}

void gore_draw_air(SDL_Renderer *r, int cx, int cy) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].live) continue;
        if (drops[i].flesh) { flesh_sq(r, drops[i].flesh, (int)drops[i].x - cx - drops[i].size / 2, (int)(drops[i].y - drops[i].z) - cy - drops[i].size / 2, drops[i].size); continue; }
        sq(r, drops[i].shade, (int)drops[i].x - cx - drops[i].size / 2, (int)(drops[i].y - drops[i].z) - cy - drops[i].size / 2, drops[i].size);
    }
    for (int i = 0; i < MAX_SPARKS; i++) {
        if (!sparks[i].live) continue;
        int sz = sparks[i].life > sparks[i].max * 0.4f ? sparks[i].size : s;
        sq(r, sparks[i].shade, (int)sparks[i].x - cx - sz / 2, (int)(sparks[i].y - sparks[i].z) - cy - sz / 2, sz);
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
int  gore_flesh_count(void)  { return n_flesh; }
int  gore_drops_flying(void) { int c = 0; for (int i = 0; i < MAX_DROPS; i++) c += drops[i].live; return c; }
int  gore_head_exists(void)  { return head.on; }
int  gore_head_settled(void) { return head.on && head.settled; }
void gore_head_pos(float *x, float *y) { *x = head.x; *y = head.y; }
int  gore_busy(void)         { return gore_drops_flying() > 0 || (head.on && !head.settled); }   /* (the endless sparks do not count) */
