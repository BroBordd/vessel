/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "world.h"
#include "char.h"
#include "dialog.h"
#include "missions.h"
#include "npc.h"
#include "story.h"
#include "nowplaying.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define MAP_MAX 80                  /* biggest map we can hold */
#define TILE_U 8                    /* tile edge in "pixel units" */
#define ZOOM   2.0f                 /* world zoom (pixel-art scale multiplier) */
#define WALK_TILES_PER_SEC 5.0f     /* walking speed at full stick tilt */

enum { T_GRASS, T_FLOWER, T_WATER, T_SAND, T_TREE, T_CLOUD, T_SKY };

static uint8_t map[MAP_MAX][MAP_MAX];
static int   mw, mh, cur_map;       /* size of the loaded map, and which one it is */
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
static int   stick_on, controls_visible;
static float kx, ky;                /* knob offset, -1..1 */

/* interact button (fixed, bottom-right). shows the face of whoever you can talk to */
static int   bcx, bcy, br;          /* centre and radius */
static int   btn_down, btn_inside;
static int   near_id = -1;          /* npc in talking range, or -1 */
static float ui;                    /* screen px per ui unit (screen width / 360) */

static float cam_x, cam_y;

/* the hole that opens in the cloud map. only exists once the script opens it */
#define HOLE_OPEN_T 1.4f            /* seconds to grow open */
#define HOLE_RANGE  1.6f            /* tiles: this close and the interact button shows an arrow */
#define HOLE_LEAVE  2.1f
static int   hole_on, hole_tx, hole_ty, near_hole, hole_in_range;
static float hole_t;
static void (*hole_cb)(void);

/* the jump / fall / landing cinematic */
enum { PH_PLAY, PH_SINK, PH_FALL, PH_LAND, PH_GETUP };
#define SINK_T  1.0f                /* walking into the hole and sinking */
#define FALL_T  5.5f                /* falling through the sky */
#define LAND_T  1.5f                /* lying face-down after the impact */
#define GETUP_T 1.2f                /* pushing up and standing */
static int   phase;
static float ph_t, sink_x0, sink_y0;
static void (*up_cb)(void);

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
    if (tx < 0 || ty < 0 || tx >= mw || ty >= mh) return 1;
    return map[ty][tx] == T_WATER || map[ty][tx] == T_TREE || map[ty][tx] == T_SKY;
}

static void gen_map(void) {
    mw = mh = 80;
    int cx = mw / 2, cy = mh / 2;
    for (int y = 0; y < mh; y++)
        for (int x = 0; x < mw; x++) {
            float n = vnoise(x / 9.0f, y / 9.0f, 7) * 0.7f + vnoise(x / 4.0f, y / 4.0f, 11) * 0.3f;
            int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            uint8_t v = T_GRASS;
            if (n > 0.64f && d2 > 36) v = T_WATER;
            else if (d2 > 25 && rnd(x, y, 3) < 0.10f) v = T_TREE;
            else if (rnd(x, y, 5) > 0.88f) v = T_FLOWER;
            if (x == 0 || y == 0 || x == mw - 1 || y == mh - 1) v = T_TREE;
            map[y][x] = v;
        }
    /* sand shore around water */
    for (int y = 1; y < mh - 1; y++)
        for (int x = 1; x < mw - 1; x++) {
            if (map[y][x] == T_WATER || map[y][x] == T_TREE) continue;
            if (map[y-1][x] == T_WATER || map[y+1][x] == T_WATER ||
                map[y][x-1] == T_WATER || map[y][x+1] == T_WATER) map[y][x] = T_SAND;
        }
}

/* a round island of cloud floating in the sky. the middle 7 tiles are always solid floor. */
static void gen_cloud_map(void) {
    mw = mh = 32;
    int cx = mw / 2, cy = mh / 2;
    for (int y = 0; y < mh; y++)
        for (int x = 0; x < mw; x++) {
            float n = vnoise(x / 4.0f, y / 4.0f, 21);
            float r = 9.0f + (n - 0.5f) * 5.0f;
            float d = sqrtf((float)((x - cx) * (x - cx) + (y - cy) * (y - cy)));
            map[y][x] = (d < r || d <= 7.5f) ? T_CLOUD : T_SKY;
            if (x == 0 || y == 0 || x == mw - 1 || y == mh - 1) map[y][x] = T_SKY;
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

static void draw_sky_tile(SDL_Renderer *r, int tx, int ty, int x, int y) {
    uint32_t h = hash2(tx, ty, 41);
    col(r, 126, 188, 246, 255);
    fill(r, x, y, tile, tile);
    if (h % 5 == 0) {                                   /* pale wisps drifting past */
        int off = ((int)(t * 2.0f) + (int)(h >> 8)) % 8;
        col(r, 176, 214, 250, 255);
        fill(r, x + off * px, y + ((h >> 4) & 3) * 2 * px + px, (off > 4 ? 8 - off : 3) * px, px);
    }
    if (h % 17 == 0) {                                  /* far away puffs */
        col(r, 236, 244, 255, 190);
        fill(r, x + 1 * px, y + 4 * px, 6 * px, 2 * px);
        fill(r, x + 2 * px, y + 3 * px, 3 * px, 3 * px);
    }
}

static void draw_cloud_tile(SDL_Renderer *r, int tx, int ty, int x, int y) {
    uint32_t h = hash2(tx, ty, 77);
    col(r, 238 + (h & 1) * 3, 243 + (h & 1) * 2, 254, 255);
    fill(r, x, y, tile, tile);
    col(r, 255, 255, 255, 255);                         /* bright puffs */
    fill(r, x + (h & 7) * px, y + ((h >> 3) & 7) * px, 2 * px, px);
    fill(r, x + ((h >> 6) & 7) * px, y + ((h >> 9) & 7) * px, px, px);
    col(r, 208, 222, 244, 255);                         /* faint blue-grey shading */
    fill(r, x + ((h >> 12) & 7) * px, y + ((h >> 15) & 7) * px, 2 * px, px);
    if (ty + 1 < mh && map[ty + 1][tx] == T_SKY) {      /* underside of the cloud where it meets the sky */
        col(r, 196, 212, 240, 255);
        fill(r, x, y + 6 * px, tile, 2 * px);
        col(r, 214, 226, 246, 255);
        fill(r, x, y + 5 * px, tile, px);
    }
}

static void draw_tile(SDL_Renderer *r, int tx, int ty, int x, int y) {
    uint8_t v = map[ty][tx];
    if (v == T_SKY)   { draw_sky_tile(r, tx, ty, x, y);   return; }
    if (v == T_CLOUD) { draw_cloud_tile(r, tx, ty, x, y); return; }
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

/* ---------- API ---------- */
/* swaps in a whole new map: npcs and the hole are cleared, the player goes to that map's spawn */
static void load_map(int which) {
    cur_map = which;
    if (which == MAP_CLOUD) gen_cloud_map(); else gen_map();
    npc_reset(px, tile);
    hole_on = near_hole = hole_in_range = 0; hole_cb = NULL;
    near_id = -1; btn_down = btn_inside = 0;
    pxp = (mw / 2 + 0.5f) * tile;
    pyp = (mh / 2 + (which == MAP_CLOUD ? 4.9f : 0.9f)) * tile;     /* the cloud spawn sits 4 tiles below the middle */
    facing = FACE_DOWN;
}

void world_init(int w, int h) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    px = (int)(u * 3.0f * ZOOM); if (px < 2) px = 2;
    tile = TILE_U * px;
    t = 0; walk = 0; moving = 0; facing = 0; stick_on = 0; kx = ky = 0;
    controls_visible = 1; btn_down = btn_inside = 0; near_id = -1;
    phase = PH_PLAY; ph_t = 0; up_cb = NULL;

    ui = u;
    int margin = (int)(26 * u);
    sr = (int)(58 * u); kr = (int)(24 * u);
    sx = margin + sr; sy = H - margin - sr;
    br = (int)(44 * u);
    bcx = W - margin - br; bcy = H - margin - br;

    dialog_init(w, h);
    missions_init(w, h);
    load_map(MAP_CLOUD);                        /* the story starts up in the clouds */
    story_start();                              /* the script takes it from here (story.c) */
}

void world_set_controls_visible(int on) {
    controls_visible = on;
    if (!on) { stick_on = 0; kx = ky = 0; btn_down = 0; }
}
int world_player_tile_x(void) { return (int)(pxp / tile); }
int world_player_tile_y(void) { return (int)(pyp / tile); }

void world_open_hole(int tx, int ty, void (*on_enter)(void)) {
    hole_on = 1; hole_tx = tx; hole_ty = ty; hole_t = 0; hole_cb = on_enter; hole_in_range = 0;
}

void world_fall_to_green(void (*on_up)(void)) {
    up_cb = on_up; ph_t = 0; facing = FACE_DOWN;
    world_set_controls_visible(0);
    phase = hole_on ? PH_SINK : PH_FALL;
    sink_x0 = pxp; sink_y0 = pyp;
}

static int button_hit(int x, int y) {
    float dx = (float)(x - bcx), dy = (float)(y - bcy);
    return sqrtf(dx * dx + dy * dy) <= br * 1.3f;                /* generous hit area */
}

void world_touch(int a, int x, int y) {
    if (dialog_active()) {                              /* dialogs eat all touches */
        dialog_touch(a, x, y);
        stick_on = 0; kx = ky = 0; btn_down = 0;
        return;
    }
    if (!controls_visible || phase != PH_PLAY) return;

    /* interact button: press on it, release on it = interact */
    int can = near_id >= 0 || near_hole;
    if (a == 0 && can && button_hit(x, y)) { btn_down = 1; btn_inside = 1; return; }
    if (btn_down) {
        if (a == 2) btn_inside = button_hit(x, y);
        if (a == 1 || a == 3) {
            int fire = a == 1 && btn_inside && can;
            btn_down = btn_inside = 0;
            if (fire) {
                if (near_hole) { if (hole_cb) hole_cb(); }
                else npc_interact(near_id);
            }
        }
        return;
    }

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
    return npc_collides(fx, fy, hw, 2 * hh);
}

/* walks through the jump -> fall -> land -> get-up cinematic */
static void phase_update(float dt) {
    ph_t += dt;
    switch (phase) {
    case PH_SINK: {                                     /* step to the middle of the hole, then sink in */
        float hx = (hole_tx + 0.5f) * tile, hy = (hole_ty + 0.5f) * tile + 0.15f * tile;
        float k = ph_t / 0.4f; if (k > 1) k = 1;
        pxp = sink_x0 + (hx - sink_x0) * k;
        pyp = sink_y0 + (hy - sink_y0) * k;
        if (ph_t >= SINK_T) { phase = PH_FALL; ph_t = 0; }
        break;
    }
    case PH_FALL:
        if (ph_t >= FALL_T) {
            load_map(MAP_GREEN);                        /* we are on the ground now. face-first. */
            phase = PH_LAND; ph_t = 0;
        }
        break;
    case PH_LAND:
        if (ph_t >= LAND_T) { phase = PH_GETUP; ph_t = 0; }
        break;
    case PH_GETUP:
        if (ph_t >= GETUP_T) {
            phase = PH_PLAY; ph_t = 0;
            world_set_controls_visible(1);              /* the analog appears when he is back on his feet */
            if (up_cb) { void (*cb)(void) = up_cb; up_cb = NULL; cb(); }
        }
        break;
    default: break;
    }
}

static void update_hole_range(void) {
    near_hole = 0;
    if (!hole_on || hole_t < HOLE_OPEN_T || phase != PH_PLAY || dialog_active()) { hole_in_range = 0; return; }
    float dx = pxp - (hole_tx + 0.5f) * tile, dy = pyp - (hole_ty + 0.5f) * tile;
    float d = sqrtf(dx * dx + dy * dy) / tile;
    if (!hole_in_range && d < HOLE_RANGE) hole_in_range = 1;
    else if (hole_in_range && d > HOLE_LEAVE) hole_in_range = 0;
    near_hole = hole_in_range;
}

void world_update(float dt) {
    t += dt;
    story_update(dt);
    dialog_update(dt);
    missions_update(dt);
    if (hole_on && hole_t < HOLE_OPEN_T) hole_t += dt;

    if (phase != PH_PLAY) {                                   /* cinematic: no input, no walking */
        kx = ky = 0; stick_on = 0; moving = 0; walk = 0; btn_down = 0;
        phase_update(dt);
        near_id = -1; near_hole = 0;
    } else {
        if (dialog_active()) { kx = ky = 0; stick_on = 0; }   /* frozen while talking */
        float mag = sqrtf(kx * kx + ky * ky);
        moving = mag > 0.15f;
        if (moving) {
            float speed = WALK_TILES_PER_SEC * tile * (mag > 1 ? 1 : mag);
            float vx = kx / mag * speed * dt, vy = ky / mag * speed * dt;
            if (!blocked(pxp + vx, pyp)) pxp += vx;
            if (!blocked(pxp, pyp + vy)) pyp += vy;
            if (fabsf(kx) > fabsf(ky)) facing = kx < 0 ? FACE_LEFT : FACE_RIGHT;
            else                       facing = ky < 0 ? FACE_UP : FACE_DOWN;
            walk += dt * 8.0f;
        } else walk = 0;

        npc_update(pxp, pyp);
        update_hole_range();
        near_id = (dialog_active() || near_hole) ? -1 : npc_nearby();
        if (near_id < 0 && !near_hole) btn_down = btn_inside = 0;
    }

    cam_x = pxp - W / 2.0f; cam_y = pyp - H / 2.0f;
    float mx = (float)(mw * tile - W), my = (float)(mh * tile - H);
    if (cam_x > mx) cam_x = mx;
    if (cam_x < 0)  cam_x = 0;
    if (cam_y > my) cam_y = my;
    if (cam_y < 0)  cam_y = 0;
}

/* ellipse helpers for the hole */
static void ell(SDL_Renderer *r, int cx, int cy, int rx, int ry) {
    if (rx < 1 || ry < 1) return;
    for (int dy = -ry; dy <= ry; dy++) {
        float f = (float)dy / ry;
        int hw = (int)(rx * sqrtf(1.0f - f * f));
        fill(r, cx - hw, cy + dy, hw * 2 + 1, 1);
    }
}

/* the hole in the clouds. front_only redraws just the near lip, so a sinking player is "inside" it */
static void draw_hole(SDL_Renderer *r, int cx, int cy, int front_only) {
    float p = hole_t / HOLE_OPEN_T; if (p > 1) p = 1;
    p = 1.0f - (1.0f - p) * (1.0f - p);                     /* opens fast, settles slowly */
    int rx = (int)(p * 1.4f * tile), ry = (int)(p * 0.75f * tile);
    if (rx < 2) return;
    int n = 16, pr = (int)(p * 0.21f * tile); if (pr < 1) pr = 1;

    if (!front_only) {
        for (int i = 0; i < n; i++) {                       /* puffy rim, behind */
            float a = i * 6.2831853f / n;
            if (sinf(a) > 0.05f) continue;                  /* the far half first */
            if (i & 1) col(r, 255, 255, 255, 255); else col(r, 226, 234, 250, 255);
            disc(r, cx + (int)(cosf(a) * rx), cy + (int)(sinf(a) * ry), pr);
        }
        col(r, 30, 62, 148, 255);                           /* down into the sky */
        ell(r, cx, cy, rx - pr / 2, ry - pr / 3);
        col(r, 20, 40, 110, 255);
        ell(r, cx, cy - ry / 8, (int)((rx - pr) * 0.82f), (int)((ry - pr / 2) * 0.78f));
        col(r, 66, 148, 84, 255);                           /* the green ground, far below */
        ell(r, cx, cy + ry / 4, (int)(rx * 0.38f), (int)(ry * 0.34f));
        col(r, 255, 255, 255, 190);                         /* a slow swirl of light */
        for (int i = 0; i < 4; i++) {
            float a = t * 1.6f + i * 1.5708f;
            fill(r, cx + (int)(cosf(a) * rx * 0.62f) - px, cy + (int)(sinf(a) * ry * 0.62f), 2 * px, px);
        }
    }
    for (int i = 0; i < n; i++) {                           /* near half of the rim, in front */
        float a = i * 6.2831853f / n;
        if (sinf(a) <= 0.05f) continue;
        if (i & 1) col(r, 255, 255, 255, 255); else col(r, 226, 234, 250, 255);
        disc(r, cx + (int)(cosf(a) * rx), cy + (int)(sinf(a) * ry), pr);
    }
}

/* the sky rushing past while we fall. screen space, no map. */
static void draw_fall(SDL_Renderer *r) {
    float p = ph_t / FALL_T; if (p > 1) p = 1;
    int bands = 16;
    for (int i = 0; i < bands; i++) {                       /* sky, a touch lighter toward the bottom */
        float k = i / (float)(bands - 1);
        col(r, (int)(92 + k * 110), (int)(158 + k * 72), (int)(238 + k * 12), 255);
        fill(r, 0, i * H / bands, W, H / bands + 1);
    }

    float v = ph_t * ph_t * 0.55f + ph_t * 1.4f;            /* distance fallen, speeds up */
    float scroll = v * 240.0f * ui;
    for (int i = 0; i < 18; i++) {                          /* clouds streaking upward, parallax by size */
        uint32_t h = hash2(i, 7, 3);
        float par = 0.5f + (h & 255) / 255.0f * 1.1f;
        int cw = (int)((46 + ((h >> 8) & 63)) * ui * par), ch = cw / 3;
        int cxp = (int)(((h >> 16) & 1023) / 1023.0f * W) - cw / 2;
        float span = (float)(H + 2 * cw);
        float yy = fmodf((((h >> 4) & 1023) / 1023.0f) * span - scroll * par, span);
        if (yy < 0) yy += span;
        int cyp = (int)yy - cw;
        int a = (int)(150 + par * 70); if (a > 255) a = 255;
        col(r, 255, 255, 255, a);
        fill(r, cxp, cyp + ch / 3, cw, ch * 2 / 3);
        fill(r, cxp + cw / 6, cyp, cw * 2 / 3, ch);
        fill(r, cxp + cw / 3, cyp - ch / 4, cw / 3, ch / 2);
        col(r, 214, 226, 246, a);
        fill(r, cxp + cw / 8, cyp + ch - ch / 6, cw * 3 / 4, ch / 6);
    }
    for (int i = 0; i < 14; i++) {                          /* wind streaks */
        uint32_t h = hash2(i, 13, 5);
        int sxp = (int)(((h >> 3) & 1023) / 1023.0f * W);
        int len = (int)((26 + (h & 31)) * ui * (0.6f + ph_t * 0.35f));
        float span = (float)(H + len);
        float yy = fmodf((((h >> 13) & 1023) / 1023.0f) * span - scroll * 2.2f, span);
        if (yy < 0) yy += span;
        col(r, 255, 255, 255, 110);
        fill(r, sxp, (int)yy - len, ui > 1.5f ? 2 : 1, len);
    }

    if (p > 0.82f) {                                        /* the ground rushing up to meet us */
        float g = (p - 0.82f) / 0.18f;
        int top = H - (int)(g * g * H * 1.02f);
        col(r, 70, 150, 76, 255);
        fill(r, 0, top, W, H - top);
        col(r, 58, 130, 62, 255);
        int band = (int)(22 * ui) + 1;
        for (int y = top + band; y < H; y += band * 2) fill(r, 0, y, W, band);
    }

    /* the vessel, seen from above as they drop: back to us, legs kicking */
    int fx = W / 2 + (int)(sinf(ph_t * 3.1f) * 7.0f * ui), fy = (int)(H * 0.52f);
    char_draw_air(r, &VESSEL, fx, fy, FACE_UP, 1, ph_t * 14.0f, px);
}

/* round button with the face of whoever you are next to (or an arrow down into the hole). tap it. */
static void draw_interact_button(SDL_Renderer *r) {
    int down = btn_down && btn_inside;
    int rw = (int)(2.5f * ui); if (rw < 2) rw = 2;
    int pulse = 170 + (int)(70.0f * sinf(t * 5.0f));              /* ring breathes to draw the eye */

    if (down) col(r, 70, 80, 130, 240); else col(r, 24, 28, 48, 235);
    disc(r, bcx, bcy, br);
    col(r, 255, 255, 255, down ? 255 : pulse);
    ring(r, bcx, bcy, br, br - rw);

    if (near_hole) {                                             /* pixel arrow pointing down */
        int a = br / 7; if (a < 2) a = 2;
        int bob = (int)(sinf(t * 6.0f) * a * 0.4f);
        col(r, 255, 255, 255, 255);
        fill(r, bcx - a, bcy - 4 * a + bob, 2 * a, 4 * a);
        fill(r, bcx - 3 * a, bcy + bob, 6 * a, a);
        fill(r, bcx - 2 * a, bcy + a + bob, 4 * a, a);
        fill(r, bcx - a, bcy + 2 * a + bob, 2 * a, a);
        return;
    }
    int ps = (int)(br * 1.25f / 10); if (ps < 1) ps = 1;
    int side = 10 * ps;
    char_draw_portrait(r, npc_person(near_id), bcx - side / 2, bcy - side / 2, ps, 0);
}

void world_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    if (phase == PH_FALL) {                                       /* open sky, no map */
        draw_fall(r);
        if (t < 1.0f) { col(r, 0, 0, 0, (int)(255 * (1.0f - t))); fill(r, 0, 0, W, H); }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        return;
    }

    int cx = (int)cam_x, cy = (int)cam_y;
    if (phase == PH_LAND && ph_t < 0.5f) {                        /* thud: the screen shakes */
        float k = (0.5f - ph_t) / 0.5f;
        cx += (int)(sinf(ph_t * 95.0f) * 7.0f * ui * k);
        cy += (int)(cosf(ph_t * 80.0f) * 9.0f * ui * k);
    }
    int tx0 = cx / tile, ty0 = cy / tile;
    if (cx < 0) tx0 = (cx - tile + 1) / tile;
    if (cy < 0) ty0 = (cy - tile + 1) / tile;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    int tx1 = (cx + W) / tile, ty1 = (cy + H) / tile;

    for (int ty = ty0; ty <= ty1 && ty < mh; ty++)
        for (int tx = tx0; tx <= tx1 && tx < mw; tx++)
            draw_tile(r, tx, ty, tx * tile - cx, ty * tile - cy);

    int hcx = 0, hcy = 0;
    if (hole_on) {
        hcx = (int)((hole_tx + 0.5f) * tile) - cx;
        hcy = (int)((hole_ty + 0.5f) * tile) - cy;
        draw_hole(r, hcx, hcy, 0);
    }

    /* characters, back to front so whoever is lower on screen draws on top */
    for (int i = 0; i < npc_count(); i++)
        if (npc_foot_y(i) <= pyp) npc_draw(r, i, cx, cy);

    int sx0 = (int)pxp - cx, sy0 = (int)pyp - cy;
    if (phase == PH_SINK) {                                       /* sinking: cut off at the hole's middle */
        float k = (ph_t - 0.4f) / (SINK_T - 0.4f); if (k < 0) k = 0;
        SDL_Rect clip = { 0, 0, W, hcy + (int)(0.12f * tile) };
        SDL_RenderSetClipRect(r, &clip);
        char_draw_air(r, &VESSEL, sx0, sy0 + (int)(k * 13.0f * px), FACE_DOWN, 0, 0.0f, px);
        SDL_RenderSetClipRect(r, NULL);
        draw_hole(r, hcx, hcy, 1);
    } else if (phase == PH_LAND || phase == PH_GETUP) {
        float k = phase == PH_GETUP ? ph_t / GETUP_T : 0.0f;
        if (k < 0.62f) {
            char_draw_prone(r, &VESSEL, sx0, sy0, px, k / 0.62f);
        } else {
            char_draw(r, &VESSEL, sx0, sy0, FACE_DOWN, 0, 0.0f, px);
        }
        if (phase == PH_LAND && ph_t < 0.6f) {                    /* dust kicked up by the impact */
            for (int i = 0; i < 10; i++) {
                float a = i * 0.6283f + 0.3f, d = (0.15f + ph_t * 1.6f) * tile * (0.6f + (i % 3) * 0.25f);
                col(r, 236, 230, 214, (int)(190 * (1.0f - ph_t / 0.6f)));
                int sz = px * (2 + i % 2);
                fill(r, sx0 + (int)(cosf(a) * d) - sz / 2, sy0 - 2 * px + (int)(sinf(a) * d * 0.5f), sz, sz);
            }
        }
    } else {
        char_draw(r, &VESSEL, sx0, sy0, facing, moving, walk, px);
    }

    for (int i = 0; i < npc_count(); i++)
        if (npc_foot_y(i) > pyp) npc_draw(r, i, cx, cy);

    if (controls_visible && !dialog_active()) {
        col(r, 255, 255, 255, 40);  disc(r, sx, sy, sr);
        col(r, 255, 255, 255, 150); ring(r, sx, sy, sr, sr - (px > 1 ? px : 2));
        int kcx = sx + (int)(kx * (sr - kr * 0.3f)), kcy = sy + (int)(ky * (sr - kr * 0.3f));
        col(r, 255, 255, 255, stick_on ? 220 : 130); disc(r, kcx, kcy, kr);
        if (near_id >= 0 || near_hole) draw_interact_button(r);
    }

    missions_set_offset(nowplaying_offset());                     /* slide under the now-playing card */
    missions_draw(r);

    if (t < 1.0f) {                                               /* fade in from black */
        col(r, 0, 0, 0, (int)(255 * (1.0f - t)));
        fill(r, 0, 0, W, H);
    }
    if (phase == PH_LAND && ph_t < 0.4f) {                        /* flash of white on impact */
        col(r, 255, 255, 255, (int)(255 * (1.0f - ph_t / 0.4f)));
        fill(r, 0, 0, W, H);
    }
    dialog_draw(r);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
