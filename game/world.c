/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "world.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define MAP_W 80
#define MAP_H 80
#define TILE_U 8                    /* tile edge in "pixel units" */

enum { T_GRASS, T_FLOWER, T_WATER, T_SAND, T_TREE };

static uint8_t map[MAP_H][MAP_W];
static int   W, H;
static int   px;                    /* screen px per pixel-unit (pixel-art scale) */
static int   tile;                  /* tile edge in screen px */
static float t;                     /* time (s) */

/* player: feet-center position in world px */
static float pxp, pyp;
static int   facing;                /* 0 down, 1 up, 2 left, 3 right */
static float walk;                  /* walk cycle phase */
static int   moving;

/* analog stick (fixed, bottom-left) */
static int   sx, sy, sr, kr;        /* centre, base radius, knob radius */
static int   stick_on;
static float kx, ky;                /* knob offset, -1..1 */

static float cam_x, cam_y;

/* ---------- map generation ---------- */
static uint32_t hash2(int x, int y, uint32_t seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float rnd(int x, int y, uint32_t seed) { return (hash2(x, y, seed) & 0xFFFF) / 65536.0f; }

static float vnoise(float x, float y, uint32_t seed) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = rnd(xi, yi, seed),     b = rnd(xi + 1, yi, seed);
    float c = rnd(xi, yi + 1, seed), d = rnd(xi + 1, yi + 1, seed);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

static int solid_tile(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return 1;
    return map[ty][tx] == T_WATER || map[ty][tx] == T_TREE;
}

static void gen_map(void) {
    int cx = MAP_W / 2, cy = MAP_H / 2;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            float n = vnoise(x / 9.0f, y / 9.0f, 7) * 0.7f + vnoise(x / 4.0f, y / 4.0f, 11) * 0.3f;
            int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            uint8_t v = T_GRASS;
            if (n > 0.64f && d2 > 36) v = T_WATER;
            else if (d2 > 25 && rnd(x, y, 3) < 0.10f) v = T_TREE;
            else if (rnd(x, y, 5) > 0.88f) v = T_FLOWER;
            if (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1) v = T_TREE;
            map[y][x] = v;
        }
    /* sand shore around water */
    for (int y = 1; y < MAP_H - 1; y++)
        for (int x = 1; x < MAP_W - 1; x++) {
            if (map[y][x] == T_WATER || map[y][x] == T_TREE) continue;
            if (map[y-1][x] == T_WATER || map[y+1][x] == T_WATER ||
                map[y][x-1] == T_WATER || map[y][x+1] == T_WATER) map[y][x] = T_SAND;
        }
}

/* ---------- drawing helpers ---------- */
static void fill(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect q = { x, y, w, h };
    SDL_RenderFillRect(r, &q);
}
static void col(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }

static void disc(SDL_Renderer *r, int cx, int cy, int rad) {
    for (int dy = -rad; dy <= rad; dy++) {
        int hw = (int)sqrtf((float)(rad * rad - dy * dy));
        fill(r, cx - hw, cy + dy, hw * 2 + 1, 1);
    }
}
static void ring(SDL_Renderer *r, int cx, int cy, int ro, int ri) {
    for (int dy = -ro; dy <= ro; dy++) {
        int ho = (int)sqrtf((float)(ro * ro - dy * dy));
        int hi = abs(dy) < ri ? (int)sqrtf((float)(ri * ri - dy * dy)) : -1;
        if (hi < 0) fill(r, cx - ho, cy + dy, ho * 2 + 1, 1);
        else {
            fill(r, cx - ho, cy + dy, ho - hi, 1);
            fill(r, cx + hi + 1, cy + dy, ho - hi, 1);
        }
    }
}

static void draw_tile(SDL_Renderer *r, int tx, int ty, int x, int y) {
    uint8_t v = map[ty][tx];
    uint32_t h = hash2(tx, ty, 99);
    int bg = v == T_WATER ? 0 : v == T_SAND ? 1 : 2;

    if (bg == 0)      col(r, 48, 98, 196, 255);
    else if (bg == 1) col(r, 224, 204, 144, 255);
    else              col(r, 76 + (h & 3) * 3, 162 + ((h >> 2) & 3) * 3, 78, 255);
    fill(r, x, y, tile, tile);

    if (v == T_WATER) {
        col(r, 96, 152, 232, 255);
        int off = (int)((sinf(t * 1.6f + tx * 0.9f + ty * 0.5f) * 0.5f + 0.5f) * 3);
        fill(r, x + (1 + off) * px, y + 2 * px, 3 * px, px);
        fill(r, x + (4 - off) * px, y + 5 * px, 3 * px, px);
    } else if (v == T_SAND) {
        col(r, 200, 178, 118, 255);
        fill(r, x + (h & 7) * px, y + ((h >> 3) & 7) * px, px, px);
        fill(r, x + ((h >> 6) & 7) * px, y + ((h >> 9) & 7) * px, px, px);
    } else {
        col(r, 58, 140, 62, 255);                       /* grass tufts */
        fill(r, x + (h & 7) * px, y + ((h >> 3) & 7) * px, px, 2 * px);
        fill(r, x + ((h >> 8) & 7) * px, y + ((h >> 11) & 7) * px, px, 2 * px);
        if (v == T_FLOWER) {
            int fx = 2 + (h & 3), fy = 2 + ((h >> 4) & 3);
            col(r, (h >> 9) & 1 ? 250 : 240, (h >> 9) & 1 ? 230 : 120, (h >> 9) & 1 ? 90 : 170, 255);
            fill(r, x + fx * px, y + fy * px, px, px);
            fill(r, x + (fx + 1) * px, y + fy * px, px, px);
            fill(r, x + fx * px, y + (fy + 1) * px, px, px);
            fill(r, x + (fx + 1) * px, y + (fy + 1) * px, px, px);
            col(r, 255, 255, 255, 255);
            fill(r, x + fx * px, y + fy * px, px, px);
        } else if (v == T_TREE) {
            col(r, 0, 0, 0, 70);                         /* ground shadow */
            fill(r, x + 1 * px, y + 6 * px, 6 * px, 2 * px);
            col(r, 102, 66, 32, 255);                    /* trunk */
            fill(r, x + 3 * px, y + 5 * px, 2 * px, 3 * px);
            col(r, 28, 104, 40, 255);                    /* canopy */
            fill(r, x + 1 * px, y + 1 * px, 6 * px, 4 * px);
            fill(r, x + 2 * px, y, 4 * px, 6 * px);
            col(r, 52, 142, 56, 255);
            fill(r, x + 2 * px, y + 1 * px, 2 * px, 2 * px);
        }
    }
}

/* 8x10 sprite; legend: h hair, s skin, c shirt, p pants, b boots, e eye */
static const char *spr[10] = {
    "..hhhh..",
    ".hhhhhh.",
    ".hssssh.",
    ".hssssh.",
    "..cccc..",
    ".cccccc.",
    ".sccccs.",
    "..pppp..",
    "..p..p..",
    "..b..b..",
};

static void spr_color(SDL_Renderer *r, char c) {
    switch (c) {
    case 'h': col(r, 74, 44, 28, 255);    break;
    case 's': col(r, 244, 200, 160, 255); break;
    case 'c': col(r, 214, 60, 60, 255);   break;
    case 'p': col(r, 52, 70, 140, 255);   break;
    case 'b': col(r, 40, 30, 30, 255);    break;
    case 'e': col(r, 20, 20, 30, 255);    break;
    }
}

static void draw_player(SDL_Renderer *r, int sxp, int syp) {   /* sxp,syp: feet-center on screen */
    int frame = moving ? ((int)(walk) & 1) : 0;
    int bob = (moving && frame) ? 1 : 0;
    int ox = sxp - 4 * px, oy = syp - 10 * px - bob * px;

    col(r, 0, 0, 0, 80);                                       /* shadow */
    fill(r, sxp - 4 * px, syp - px, 8 * px, 2 * px);

    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 8; x++) {
            char c = spr[y][x];
            if (c == '.') continue;
            int dx = x, dy = y;
            if (y >= 8) {                                      /* leg animation */
                if (moving && frame == 0 && x < 4)  dy = y - 1;       /* lift left foot */
                if (moving && frame == 1 && x >= 4) dy = y - 1;       /* lift right foot */
                if (dy < 8) continue;
            }
            if (facing == 1 && (y == 2 || y == 3) && c == 's') c = 'h';    /* back of head */
            spr_color(r, c);
            fill(r, ox + dx * px, oy + dy * px, px, px);
        }

    /* eyes */
    col(r, 20, 20, 30, 255);
    if (facing == 0) {
        fill(r, ox + 2 * px, oy + 3 * px, px, px);
        fill(r, ox + 5 * px, oy + 3 * px, px, px);
    } else if (facing == 2) {
        fill(r, ox + 2 * px, oy + 3 * px, px, px);
    } else if (facing == 3) {
        fill(r, ox + 5 * px, oy + 3 * px, px, px);
    }
}

/* ---------- API ---------- */
void world_init(int w, int h) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    px = (int)(u * 3.0f); if (px < 2) px = 2;
    tile = TILE_U * px;
    t = 0; walk = 0; moving = 0; facing = 0; stick_on = 0; kx = ky = 0;

    gen_map();
    pxp = (MAP_W / 2 + 0.5f) * tile;
    pyp = (MAP_H / 2 + 0.9f) * tile;

    int margin = (int)(26 * u);
    sr = (int)(58 * u); kr = (int)(24 * u);
    sx = margin + sr; sy = H - margin - sr;
}

void world_touch(int a, int x, int y) {
    if (a == 0) {
        float dx = (float)(x - sx), dy = (float)(y - sy);
        stick_on = sqrtf(dx * dx + dy * dy) <= sr * 1.5f;      /* generous grab zone */
    }
    if (a == 1 || a == 3) { stick_on = 0; kx = ky = 0; return; }
    if ((a == 0 || a == 2) && stick_on) {
        float dx = (float)(x - sx) / sr, dy = (float)(y - sy) / sr;
        float m = sqrtf(dx * dx + dy * dy);
        if (m > 1.0f) { dx /= m; dy /= m; }
        kx = dx; ky = dy;
    }
}

static int blocked(float fx, float fy) {           /* feet box centred at fx,fy */
    float hw = 3.0f * px, hh = 1.5f * px;
    float xs[2] = { fx - hw, fx + hw - 1 }, ys[2] = { fy - 2 * hh, fy - 1 };
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            if (solid_tile((int)floorf(xs[i] / tile), (int)floorf(ys[j] / tile))) return 1;
    return 0;
}

void world_update(float dt) {
    t += dt;
    float mag = sqrtf(kx * kx + ky * ky);
    moving = mag > 0.15f;
    if (moving) {
        float speed = 32.0f * px * (mag > 1 ? 1 : mag);       /* 4 tiles/s at full tilt */
        float vx = kx / mag * speed * dt, vy = ky / mag * speed * dt;
        if (!blocked(pxp + vx, pyp)) pxp += vx;
        if (!blocked(pxp, pyp + vy)) pyp += vy;
        if (fabsf(kx) > fabsf(ky)) facing = kx < 0 ? 2 : 3;
        else                       facing = ky < 0 ? 1 : 0;
        walk += dt * 8.0f;
    } else walk = 0;

    cam_x = pxp - W / 2.0f; cam_y = pyp - H / 2.0f;
    float mx = (float)(MAP_W * tile - W), my = (float)(MAP_H * tile - H);
    if (cam_x > mx) cam_x = mx;
    if (cam_x < 0)  cam_x = 0;
    if (cam_y > my) cam_y = my;
    if (cam_y < 0)  cam_y = 0;
}

void world_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int cx = (int)cam_x, cy = (int)cam_y;
    int tx0 = cx / tile, ty0 = cy / tile;
    int tx1 = (cx + W) / tile, ty1 = (cy + H) / tile;
    int py_feet = (int)pyp;

    for (int ty = ty0; ty <= ty1 && ty < MAP_H; ty++) {
        for (int tx = tx0; tx <= tx1 && tx < MAP_W; tx++)
            draw_tile(r, tx, ty, tx * tile - cx, ty * tile - cy);
    }
    draw_player(r, (int)pxp - cx, py_feet - cy);

    /* analog stick */
    col(r, 255, 255, 255, 40);  disc(r, sx, sy, sr);
    col(r, 255, 255, 255, 150); ring(r, sx, sy, sr, sr - (px > 1 ? px : 2));
    int kcx = sx + (int)(kx * (sr - kr * 0.3f)), kcy = sy + (int)(ky * (sr - kr * 0.3f));
    col(r, 255, 255, 255, stick_on ? 220 : 130); disc(r, kcx, kcy, kr);

    if (t < 1.0f) {                                             /* fade in from black */
        col(r, 0, 0, 0, (int)(255 * (1.0f - t)));
        fill(r, 0, 0, W, H);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
