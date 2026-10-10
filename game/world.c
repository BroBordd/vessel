/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "world.h"
#include "char.h"
#include "dialog.h"
#include "missions.h"
#include "npc.h"
#include "shrine.h"
#include "heart.h"
#include "summon.h"
#include "story.h"
#include "font.h"
#include "nowplaying.h"
#include "toast.h"
#include "thought.h"
#include "hud.h"
#include "minimap.h"
#include "audio.h"
#include "jukebox.h"
#include "gfx.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAP_MAX 80                  /* biggest map we can hold */
#define TILE_U 8                    /* tile edge in "pixel units" */
#define ZOOM   2.0f                 /* world zoom (pixel-art scale multiplier) */
#define WALK_TILES_PER_SEC 5.0f
#define MAX_MARKS 20     /* walking speed at full stick tilt */

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
static int   near_shrine;           /* Dia's shrine is in reach and usable: the button shows the sludge drop, and HOLDING it pollutes */
static float ui;                    /* screen px per ui unit (screen width / 360) */
static int   ucell;                 /* one chunky pixel of the controls (same grid as the ID card) */

static float cam_x, cam_y;

/* talking to an npc: the view lerps in on the point halfway between the player and the npc, and back out afterwards */
#define ZOOM_TALK  1.6f             /* how far we zoom in while talking */
#define ZOOM_SPEED 5.0f             /* exponential lerp rate (1/s): framerate independent, eases into place */
static float zoom = 1.0f, zoom_target = 1.0f;
static float zoom_fx, zoom_fy;      /* the point we zoom toward, world px (kept while zooming back out) */
static int   talk_npc = -1;         /* npc we are talking to, or -1 */
static SDL_Texture *ztex; static SDL_Renderer *zren;   /* the scene is drawn here first while zoomed, then scaled up to the screen */

/* the hole that opens in the cloud map. only exists once the script opens it */
#define HOLE_OPEN_T 1.4f            /* seconds to grow open */
#define HOLE_RANGE  1.6f            /* tiles: this close and the interact button shows an arrow */
#define HOLE_LEAVE  2.1f
static int   hole_on, hole_tx, hole_ty, near_hole, hole_in_range;
static float hole_t;
static void (*hole_cb)(void);

/* the death cutscene (vessel 1's ending, chunks 12-15): the world freezes, the camera pushes in to 300 % on the
 * player, everything but the player goes grey and the player goes red. world_death_begin() starts it. */
#define ZOOM_DEATH      3.6f
#define DEATH_ZOOM_T    2.2f        /* seconds for the slow push-in */
#define DEATH_GREY_AT   0.4f        /* the colour drains from here ... */
#define DEATH_GREY_T    1.6f        /* ... over this long */
#define DEATH_TEXT_FADE 1.2f        /* seconds for the words to fade in */
#define DEATH_SETTLE_T  2.6f        /* everything has arrived: on_ready runs */
static const Person *player_look = &VESSEL;   /* who the player looks like (chunk 17: world_set_player) */
static int   death_on;
static float blackout_t = -1, blackout_dur; /* world_fade_to_black: seconds so far (-1 = not fading) and the length (chunk 12) */
static void (*blackout_cb)(void);
static float death_t;
static char  death_msg[48];         /* the words over the cutscene (chunk 15), "" = none */
static float death_msg_t;
static float death_fall_t = -1;     /* the collapse (chunk 15.1): -1 = standing, else seconds since it began */
#define DEATH_FALL_T    1.1f        /* the topple takes this long */
static void (*death_cb)(void);
static void (*summon_cb)(void);       /* the summoning's on_done (chunk 8) */
static int   summon_chime;            /* the chime has been asked for and has not started yet (chunk 9) */
static void zoom_camera(float *ccx, float *ccy);   /* defined with world_draw */

/* the jump / fall / landing cinematic */
enum { PH_PLAY, PH_SINK, PH_FALL, PH_LAND, PH_GETUP };
#define SINK_T  1.0f                /* walking into the hole and sinking */
#define FALL_T  5.5f                /* falling through the sky */
#define FADE_IN_T 0.5f              /* fade in from black when the world starts (the summoning runs over it) */
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

/* ---------- pixel circles (the stick and the interact button: no smooth round edges) ----------
 * everything is built from square cells of `cell` px laid on a grid centred on (cx, cy).
 * a cell is in when its centre is inside the circle, so the edge is a clean staircase. */
static int half_cells(float rad_cells, int row) {          /* how many cells reach out each side on this row */
    float yc = row + 0.5f, d = rad_cells * rad_cells - yc * yc;
    return d <= 0 ? 0 : (int)floorf(sqrtf(d) + 0.5f);
}
static void pdisc(SDL_Renderer *r, int cx, int cy, int rad, int cell) {
    float R = (float)rad / cell;
    int n = (int)ceilf(R);
    for (int j = -n; j < n; j++) {
        int m = half_cells(R, j);
        if (m > 0) fill(r, cx - m * cell, cy + j * cell, 2 * m * cell, cell);
    }
}
static void pring(SDL_Renderer *r, int cx, int cy, int rad, int thick_cells, int cell) {
    float Ro = (float)rad / cell, Ri = Ro - thick_cells;
    int n = (int)ceilf(Ro);
    for (int j = -n; j < n; j++) {
        int mo = half_cells(Ro, j), mi = Ri > 0 ? half_cells(Ri, j) : 0;
        if (mo <= 0) continue;
        if (mi <= 0) { fill(r, cx - mo * cell, cy + j * cell, 2 * mo * cell, cell); continue; }
        fill(r, cx - mo * cell, cy + j * cell, (mo - mi) * cell, cell);
        fill(r, cx + mi * cell, cy + j * cell, (mo - mi) * cell, cell);
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
    float wave = sinf(t * 0.7f + tx * 0.5f + ty * 0.4f);                /* the whole floor breathes slowly */
    int b = (int)(wave * 4.0f);
    col(r, 240 + b / 2, 245 + b / 2, 255, 255);
    fill(r, x, y, tile, tile);
    for (int k = 0; k < 3; k++) {                                       /* soft blobs drifting like mist */
        float a = t * (0.35f + 0.1f * k) + (float)((h >> (k * 5)) & 31) * 0.4f;
        int w = 3 + (int)((h >> (9 + k)) & 1), hh = 2;
        int bx = 2 + (int)(sinf(a) * 2.2f), by = 1 + k * 2 + (int)(cosf(a * 0.8f) * 1.2f);
        if (bx < 0) bx = 0;
        if (bx > 8 - w) bx = 8 - w;
        if (by < 0) by = 0;
        if (by > 6) by = 6;
        if (k == 1) col(r, 255, 255, 255, 255); else col(r, 218, 230, 250, 255);
        fill(r, x + bx * px, y + by * px, w * px, hh * px);
        fill(r, x + (bx + 1) * px, y + (by - 1 < 0 ? 0 : by - 1) * px, (w - 2) * px, px);
    }
}

/* mario-style scalloped edge: pixel-art puffs (square cells, no smooth curves) bulging out of every cloud tile that touches the sky.
 * two passes (outline, then fill) so neighbouring puffs merge into one bumpy rim. */
static void draw_cloud_rim(SDL_Renderer *r, int cx, int cy, int tx0, int ty0, int tx1, int ty1) {
    static const int dx4[4] = { 0, 0, -1, 1 }, dy4[4] = { -1, 1, 0, 0 };
    for (int pass = 0; pass < 2; pass++)
        for (int ty = ty0 - 1; ty <= ty1 + 1; ty++)
            for (int tx = tx0 - 1; tx <= tx1 + 1; tx++) {
                if (tx < 0 || ty < 0 || tx >= mw || ty >= mh || map[ty][tx] != T_CLOUD) continue;
                int x = tx * tile - cx, y = ty * tile - cy;
                for (int d = 0; d < 4; d++) {
                    int nx = tx + dx4[d], ny = ty + dy4[d];
                    if (nx >= 0 && ny >= 0 && nx < mw && ny < mh && map[ny][nx] != T_SKY) continue;
                    for (int k = 0; k < 1; k++) {                       /* one big puff per edge: a round pixel circle, so the rim reads as bumps */
                        float along = 4.0f * px;                        /* the middle of the edge */
                        float bob = sinf(t * 1.6f + tx * 0.9f + ty * 1.3f + k * 2.0f + d);
                        int rad = (int)((3.6f + 0.3f * bob) * px) + (pass == 0 ? px : 0);
                        int bx, by;
                        if (d == 0)      { bx = x + (int)along;  by = y; }
                        else if (d == 1) { bx = x + (int)along;  by = y + tile; }
                        else if (d == 2) { bx = x;               by = y + (int)along; }
                        else             { bx = x + tile;        by = y + (int)along; }
                        if (pass == 0) col(r, 170, 196, 236, 255); else col(r, 252, 253, 255, 255);
                        pdisc(r, bx, by, rad, px);                  /* a pixel circle: a staircase on the sprite-pixel grid */
                    }
                }
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
    else {                                              /* grass: a slow wind shimmer rolls across the field */
        int sh = (int)(sinf(t * 1.1f + tx * 0.45f + ty * 0.3f) * 4.0f);
        col(r, 76 + (h & 3) * 3 + sh / 2, 162 + ((h >> 2) & 3) * 3 + sh, 78 + sh / 2, 255);
    }
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
        float ph = t * 2.4f + tx * 0.8f + ty * 0.55f;
        int sw1 = (int)floorf(sinf(ph) * 1.4f + 0.5f), sw2 = (int)floorf(sinf(ph + 2.1f) * 1.4f + 0.5f);
        int x1 = (int)(h & 7) + sw1, x2 = (int)((h >> 8) & 7) + sw2;
        if (x1 < 0) x1 = 0;
        if (x1 > 7) x1 = 7;
        if (x2 < 0) x2 = 0;
        if (x2 > 7) x2 = 7;
        fill(r, x + x1 * px, y + ((h >> 3) & 7) * px, px, 2 * px);        /* blades lean with the wind */
        fill(r, x + x2 * px, y + ((h >> 11) & 7) * px, px, 2 * px);
        if (v == T_FLOWER) {
            int fx = 2 + (h & 3) + sw1 / 1, fy = 2 + ((h >> 4) & 3);
            if (fx < 0) fx = 0;
            if (fx > 6) fx = 6;
            if (((int)(t * 2.0f + (h & 7)) & 3) == 0 && fy > 0) fy--;        /* little bob */
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
            int ts = (int)floorf(sinf(t * 1.3f + tx * 0.7f + ty) * 0.9f + 0.5f);   /* canopy sways */
            col(r, 28, 104, 40, 255);                    /* canopy */
            fill(r, x + (1 + ts) * px, y + 1 * px, 6 * px, 4 * px);
            fill(r, x + (2 + ts) * px, y, 4 * px, 6 * px);
            col(r, 52, 142, 56, 255);
            fill(r, x + (2 + ts) * px, y + 1 * px, 2 * px, 2 * px);
        }
    }
}

/* ---------- API ---------- */
/* swaps in a whole new map: npcs and the hole are cleared, the player goes to that map's spawn */
static void load_map(int which) {
    cur_map = which;
    if (which == MAP_CLOUD) gen_cloud_map(); else gen_map();
    npc_reset(px, tile);
    shrine_reset(px, tile);
    near_shrine = 0;
    hole_on = near_hole = hole_in_range = 0; hole_cb = NULL;
    near_id = -1; btn_down = btn_inside = 0; talk_npc = -1;
    pxp = (mw / 2 + 0.5f) * tile;
    pyp = (mh / 2 + (which == MAP_CLOUD ? 4.9f : 0.9f)) * tile;     /* the cloud spawn sits 4 tiles below the middle */
    facing = FACE_DOWN;
}

const char *world_map_name(int which) { return which == MAP_CLOUD ? SKYLAND_NAME : GRASSLANDS; }

void world_init(int w, int h) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    px = (int)(u * 3.0f * ZOOM); if (px < 2) px = 2;
    tile = TILE_U * px;
    t = 0; walk = 0; moving = 0; facing = 0; stick_on = 0; kx = ky = 0;
    controls_visible = 1; btn_down = btn_inside = 0; near_id = -1;
    phase = PH_PLAY; ph_t = 0; up_cb = NULL;
    zoom = zoom_target = 1.0f; talk_npc = -1;
    death_on = 0; death_cb = NULL; gfx_set_filter(0, 0); heart_reset(); death_msg[0] = 0; death_fall_t = -1;
    summon_cancel(); summon_cb = NULL; summon_chime = 0;
    blackout_t = -1; blackout_cb = NULL;

    ui = u;
    ucell = (int)(3.2f * u); if (ucell < 3) ucell = 3;
    int margin = (int)(26 * u);
    sr = (int)(58 * u); kr = (int)(24 * u);
    sx = margin + sr; sy = H - margin - sr;
    br = (int)(44 * u);
    bcx = W - margin - br; bcy = H - margin - br;

    dialog_init(w, h);
    missions_init(w, h);
    toast_init(w, h);
    thought_init(w, h);
    hud_init(w, h, &VAS);                       /* the ID card starts out as plain "Vas" */
    {   /* the minimap hangs right under the ID card: same right edge, same width */
        int cx, cy, cw, ch; hud_card_rect(&cx, &cy, &cw, &ch);
        minimap_init(w, h, cx + cw, cy + ch + (int)(5 * u), cw);
    }
    load_map(MAP_CLOUD);                        /* the story starts up in the clouds */
    story_start();                              /* the script takes it from here (story.c) */
}

void world_summon(void (*on_done)(void)) {
    summon_cb = on_done;
    summon_begin();
    summon_chime = 1;                                           /* the rising chime starts with the first frame the light is really running (world_update): world_init runs while limbo still shows */
    world_set_controls_visible(0);                              /* nothing to do while the light is on */
    moving = 0; walk = 0; facing = FACE_DOWN;
}
int world_summoning(void) { return summon_active(); }

void world_set_controls_visible(int on) {
    controls_visible = on;
    if (!on) { stick_on = 0; kx = ky = 0; btn_down = 0; }
}
float world_debug_zoom(void) { return zoom; }
int world_player_tile_x(void) { return (int)(pxp / tile); }
int world_player_tile_y(void) { return (int)(pyp / tile); }

float world_dist_to_npc(int npc_id) {
    if (npc_id < 0 || npc_id >= npc_count()) return 1e9f;
    float nx, ny; npc_tile(npc_id, &nx, &ny);
    float dx = pxp / tile - nx, dy = pyp / tile - ny;
    return sqrtf(dx * dx + dy * dy);
}

/* flood fill over walkable tiles from the player (4 neighbours: the feet box is smaller than a
 * tile, so any chain of open tiles can be walked), then pick the open spot nearest to the middle
 * of the wanted distance range. */
int world_find_far_spot(int min_tiles, int max_tiles, int *out_tx, int *out_ty) {
    return world_find_spot_away(min_tiles, max_tiles, 0, 0, 0, out_tx, out_ty);
}

int world_find_spot_away(int min_tiles, int max_tiles, int avoid_tx, int avoid_ty, int avoid_dist, int *out_tx, int *out_ty) {
    static uint8_t seen[MAP_MAX][MAP_MAX];
    static short   qx[MAP_MAX * MAP_MAX], qy[MAP_MAX * MAP_MAX];
    int x0 = world_player_tile_x(), y0 = world_player_tile_y();
    if (solid_tile(x0, y0)) return 0;
    memset(seen, 0, sizeof seen);
    int head = 0, tail = 0;
    qx[tail] = (short)x0; qy[tail] = (short)y0; tail++; seen[y0][x0] = 1;

    float want = (min_tiles + max_tiles) / 2.0f, best_score = 1e9f, far_d = -1;
    int best_x = -1, best_y = -1, far_x = -1, far_y = -1;
    static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
    while (head < tail) {
        int x = qx[head], y = qy[head]; head++;
        int open = 1;                                           /* all 8 neighbours walkable too */
        for (int j = -1; j <= 1 && open; j++)
            for (int i = -1; i <= 1; i++) if (solid_tile(x + i, y + j)) { open = 0; break; }
        if (open && avoid_dist > 0 && sqrtf((float)((x - avoid_tx) * (x - avoid_tx) + (y - avoid_ty) * (y - avoid_ty))) < avoid_dist) open = 0;
        if (open) {
            float d = sqrtf((float)((x - x0) * (x - x0) + (y - y0) * (y - y0)));
            if (d > far_d) { far_d = d; far_x = x; far_y = y; }
            if (d >= min_tiles && d <= max_tiles) {
                float score = fabsf(d - want);
                if (score < best_score) { best_score = score; best_x = x; best_y = y; }
            }
        }
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (solid_tile(nx, ny) || seen[ny][nx]) continue;
            seen[ny][nx] = 1; qx[tail] = (short)nx; qy[tail] = (short)ny; tail++;
        }
    }
    if (best_x < 0) { best_x = far_x; best_y = far_y; }         /* nothing in range: the farthest open spot */
    if (best_x < 0) return 0;
    *out_tx = best_x; *out_ty = best_y;
    return 1;
}

int  world_shrine_exists(void) { return shrine_exists(); }
int  world_shrine_polluted(void) { return shrine_polluted(); }
void world_shrine_tile(float *tx, float *ty) { shrine_tile(tx, ty); }
void world_place_shrine(int tile_x, int tile_y) { shrine_place(tile_x, tile_y); }
void world_enable_shrine(void (*on_done)(void)) { shrine_enable(on_done); }

/* a new life up in the clouds (chunk 14). the cloud map again, the player at its spawn, the hole closed, the colours, camera and fades
 * back to normal; the controls stay hidden (the summoning gives them back). the Grasslands are not touched: gen_map() makes the same
 * map every time, and what stood on it (Alex, the shrine) is remembered by story.c. t restarts so the world fades in from black */
void world_return_to_clouds(void) {
    load_map(MAP_CLOUD);
    summon_cancel(); summon_cb = NULL; summon_chime = 0;
    death_on = 0; death_cb = NULL; death_msg[0] = 0; death_fall_t = -1; heart_reset(); gfx_set_filter(0, 0); gfx_set_gold(0);
    blackout_t = -1; blackout_cb = NULL;
    toast_init(W, H);                                         /* no half-shown "New task" panel from the last life */
    phase = PH_PLAY; ph_t = 0; up_cb = NULL;
    zoom = zoom_target = 1.0f; moving = 0; walk = 0; facing = FACE_DOWN;
    t = 0;
    world_set_controls_visible(0);
}

void world_set_player(const Person *who) { player_look = who ? who : &VESSEL; }
const Person *world_player(void) { return player_look; }

void world_fade_to_black(float seconds, void (*on_black)(void)) {
    blackout_t = 0; blackout_dur = seconds > 0.05f ? seconds : 0.05f; blackout_cb = on_black;
}
void world_death_begin(void (*on_ready)(void)) {
    death_on = 1; death_t = 0; death_cb = on_ready; heart_reset(); death_msg[0] = 0; death_fall_t = -1;
    world_set_controls_visible(0);
    facing = FACE_DOWN; moving = 0; walk = 0; talk_npc = -1;      /* facing us, standing still */
}
int world_death_active(void) { return death_on; }
void world_death_fall(void) { if (death_on && death_fall_t < 0) death_fall_t = 0; }
void world_death_text(const char *text) {                          /* the words over the cutscene: fade in, then stay */
    snprintf(death_msg, sizeof death_msg, "%s", text ? text : ""); death_msg_t = 0;
}
void world_death_cancel(void) {                                   /* back to normal (the stand-in ending of chunk 12-14) */
    death_on = 0; death_cb = NULL; gfx_set_filter(0, 0); heart_reset(); death_msg[0] = 0; death_fall_t = -1;
    zoom_target = 1.0f;
    world_set_controls_visible(1);
}
void world_death_player(int *feet_x, int *feet_y, int *chest_y, int *pixel) {
    float ccx, ccy; zoom_camera(&ccx, &ccy);
    float srcx = (float)(int)(W / 2.0f - W / (2.0f * zoom)), srcy = (float)(int)(H / 2.0f - H / (2.0f * zoom));
    float fx = ((int)pxp - (int)ccx - srcx) * zoom, fy = ((int)pyp - (int)ccy - srcy) * zoom;
    *feet_x = (int)fx; *feet_y = (int)fy;
    *pixel = (int)(px * zoom + 0.5f);
    *chest_y = (int)(fy - 3.0f * px * zoom);                      /* the shirt rows: 3 sprite pixels above the feet */
}

void world_open_hole(int tx, int ty, void (*on_enter)(void)) {
    hole_on = 1; hole_tx = tx; hole_ty = ty; hole_t = 0; hole_cb = on_enter; hole_in_range = 0;
}

void world_fall_to_green(void (*on_up)(void)) {
    up_cb = on_up; ph_t = 0; facing = FACE_DOWN;
    world_set_controls_visible(0);
    phase = hole_on ? PH_SINK : PH_FALL;
    sink_x0 = pxp; sink_y0 = pyp;
    jukebox_scene_fade(SINK_T + FALL_T - 0.3f);         /* the cloud music dies away as we drop (map music only; a custom track keeps playing) */
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
    int can = near_id >= 0 || near_hole || near_shrine;
    if (a == 0 && can && button_hit(x, y)) {
        btn_down = 1; btn_inside = 1;
        if (near_shrine) {                                              /* turn to face it before the sabotage starts */
            float sfx_, sfy_; shrine_tile(&sfx_, &sfy_);
            float fdx = sfx_ * tile - pxp, fdy = sfy_ * tile - pyp;
            if (fabsf(fdx) > fabsf(fdy)) facing = fdx < 0 ? FACE_LEFT : FACE_RIGHT;
            else                         facing = fdy < 0 ? FACE_UP : FACE_DOWN;
        }
        return;
    }
    if (btn_down) {
        if (a == 2) btn_inside = button_hit(x, y);
        if (a == 1 || a == 3) {
            int fire = a == 1 && btn_inside && can && !near_shrine;      /* the shrine is a hold, not a tap (world_update) */
            btn_down = btn_inside = 0;
            if (fire) {
                if (near_hole) { if (hole_cb) hole_cb(); }
                else {
                    talk_npc = near_id;
                    float nx, ny; npc_tile(near_id, &nx, &ny);       /* turn to face them before the talk starts */
                    float fdx = nx * tile - pxp, fdy = ny * tile - pyp;
                    if (fabsf(fdx) > fabsf(fdy)) facing = fdx < 0 ? FACE_LEFT : FACE_RIGHT;
                    else                         facing = fdy < 0 ? FACE_UP : FACE_DOWN;
                    npc_interact(near_id);
                }
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
    return npc_collides(fx, fy, hw, 2 * hh) || shrine_collides(fx, fy, hw, 2 * hh);
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
            jukebox_scene("divine_tale.ogg", 1);          /* the ground has its own music (map music only) */
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
    if (death_msg[0]) death_msg_t += dt;
    if (death_on && death_fall_t >= 0) death_fall_t += dt;
    if (death_on) death_t += dt; else t += dt;                /* the world is frozen once the death starts: tiles, water, wind stop */
    if (summon_chime && summon_active()) { summon_chime = 0; sfx_summon(); }                   /* the world is running: the light and its chime begin together */
    if (summon_update(dt)) { void (*cb)(void) = summon_cb; summon_cb = NULL; if (cb) cb(); }   /* the light has ended: the player shows again */
    if (blackout_t >= 0 && blackout_t < blackout_dur) {       /* fading to black (chunk 12): on_black runs on the frame it is dark */
        blackout_t += dt;
        if (blackout_t >= blackout_dur) { void (*cb)(void) = blackout_cb; blackout_cb = NULL; if (cb) cb(); }
    }
    heart_update(dt);                                         /* (the heart and its blood are not part of the world: they move on) */
    story_update(dt);
    dialog_update(dt);
    missions_update(dt);
    toast_update(dt);
    hud_update(dt);
    if (hole_on && hole_t < HOLE_OPEN_T) hole_t += dt;

    if (phase != PH_PLAY) {                                   /* cinematic: no input, no walking */
        kx = ky = 0; stick_on = 0; moving = 0; walk = 0; btn_down = 0;
        phase_update(dt);
        near_id = -1; near_hole = 0; near_shrine = 0;
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
        near_shrine = shrine_near(pxp, pyp, !dialog_active() && !near_hole);
        near_id = (dialog_active() || near_hole || near_shrine) ? -1 : npc_nearby();
        if (near_id < 0 && !near_hole && !near_shrine) btn_down = btn_inside = 0;
        if (shrine_update(dt, near_shrine && btn_down && btn_inside)) btn_down = btn_inside = 0;   /* done: let go of the button */
    }

    if (death_on) {
        float k = death_t / DEATH_ZOOM_T; if (k > 1) k = 1;
        k = k * k * (3.0f - 2.0f * k);                            /* eases in and out: a slow, heavy push */
        zoom = 1.0f + (ZOOM_DEATH - 1.0f) * k;
        zoom_fx = pxp; zoom_fy = pyp - 6.0f * px;                 /* the middle of the sprite (12 units tall) */
    } else if (talk_npc >= 0 && dialog_active() && phase == PH_PLAY) {   /* zoom toward the middle of the two of us */
        float nx, ny; npc_tile(talk_npc, &nx, &ny);
        zoom_fx = (pxp + nx * tile) / 2.0f;
        zoom_fy = (pyp + ny * tile) / 2.0f - 5.0f * px;           /* aim at torsos, not feet */
        zoom_target = ZOOM_TALK;
    } else {
        zoom_target = 1.0f;
        if (!dialog_active()) talk_npc = -1;
    }
    if (!death_on) {
        zoom += (zoom_target - zoom) * (1.0f - expf(-ZOOM_SPEED * dt));
        if (fabsf(zoom_target - zoom) < 0.002f) zoom = zoom_target;
    }
    if (death_on && death_cb && death_t >= DEATH_SETTLE_T) { void (*cb)(void) = death_cb; death_cb = NULL; cb(); }

    cam_x = pxp - W / 2.0f; cam_y = pyp - H / 2.0f;
    float mx = (float)(mw * tile - W), my = (float)(mh * tile - H);
    if (cam_x > mx) cam_x = mx;
    if (cam_x < 0)  cam_x = 0;
    if (cam_y > my) cam_y = my;
    if (cam_y < 0)  cam_y = 0;
}

/* the hole in the clouds, built from the same square pixels as everything else: every cell is one
 * sprite pixel (px screen px) on the tiles' own grid, so it is pixel art and not a smooth oval.
 * (cx, cy) is the hole's centre on screen; it always lands exactly on a pixel-grid corner.
 * three passes keep the depth right: far rim, then the opening, then the near rim in front.
 * front_only redraws just the near rim, so a sinking player is "inside" the hole. */
static void draw_hole(SDL_Renderer *r, int cx, int cy, int front_only) {
    float p = hole_t / HOLE_OPEN_T; if (p > 1) p = 1;
    p = 1.0f - (1.0f - p) * (1.0f - p);                     /* opens fast, settles slowly */
    p = floorf(p * 14.0f) / 14.0f;                          /* ...in chunky steps, like a sprite animation */
    float rx = p * 11.0f, ry = p * 5.5f;                    /* radii in pixel units (a tile is 8) */
    if (rx < 1.0f) return;
    const float rim = 1.7f, line = 1.0f;                    /* puffy rim thickness, and its outline */
    float rxo = rx + rim, ryo = ry + rim, rxl = rxo + line, ryl = ryo + line;
    int R = (int)rxl + 1, Rv = (int)ryl + 1;
    int tw = (int)(t * 3.0f);                               /* the rim sparkles between two tones */

    for (int pass = 0; pass < 3; pass++) {
        if (front_only && pass < 2) continue;
        for (int j = -Rv; j < Rv; j++)
            for (int i = -R; i < R; i++) {
                float fx = i + 0.5f, fy = j + 0.5f;         /* centre of this cell */
                float din = fx * fx / (rx * rx) + fy * fy / (ry * ry);
                int chk = (i + j) & 1;
                if (din <= 1.0f) {                          /* the opening itself */
                    if (pass != 1) continue;
                    float d = din + (chk ? 0.05f : 0.0f);   /* checker dither between the depth bands */
                    if      (d < 0.28f) col(r, 16, 32, 96, 255);
                    else if (d < 0.62f) col(r, 26, 54, 134, 255);
                    else                col(r, 40, 80, 170, 255);
                    float gy = fy - ry * 0.25f, gx = rx * 0.38f, gyy = ry * 0.34f;
                    if (fx * fx / (gx * gx) + gy * gy / (gyy * gyy) <= 1.0f) {      /* the green ground, far below */
                        if (chk) col(r, 66, 148, 84, 255); else col(r, 58, 134, 74, 255);
                    }
                } else {
                    float dout = fx * fx / (rxo * rxo) + fy * fy / (ryo * ryo);
                    float dlin = fx * fx / (rxl * rxl) + fy * fy / (ryl * ryl);
                    if (dlin > 1.0f) continue;
                    if (pass != (j >= 0 ? 2 : 0)) continue;  /* far half behind, near half in front */
                    float rxm = rx + rim * 0.5f, rym = ry + rim * 0.5f;
                    int lip = fx * fx / (rxm * rxm) + fy * fy / (rym * rym) <= 1.0f;   /* inner half of the rim sits in shade */
                    if (dout > 1.0f)               col(r, 124, 156, 214, 255);          /* outline */
                    else if (lip)                  { if ((chk + tw) & 1) col(r, 196, 212, 242, 255); else col(r, 176, 196, 236, 255); }
                    else if ((chk + tw) & 1)       col(r, 255, 255, 255, 255);          /* rim, sparkling */
                    else                           col(r, 226, 234, 250, 255);
                }
                fill(r, cx + i * px, cy + j * px, px, px);
            }
        if (pass == 1) {                                    /* a slow swirl of light, snapped to the grid */
            col(r, 255, 255, 255, 190);
            for (int i = 0; i < 4; i++) {
                float a = t * 1.6f + i * 1.5708f;
                int ix = (int)floorf(cosf(a) * rx * 0.62f), iy = (int)floorf(sinf(a) * ry * 0.62f);
                fill(r, cx + ix * px, cy + iy * px, 2 * px, px);
            }
        }
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
    char_draw_air(r, player_look, fx, fy, FACE_UP, 1, ph_t * 14.0f, px);
}

/* round button with the face of whoever you are next to (or an arrow down into the hole). tap it. */
static void draw_interact_button(SDL_Renderer *r) {
    int down = btn_down && btn_inside;
    int rw = (int)(2.5f * ui); if (rw < 2) rw = 2;
    int pulse = 170 + (int)(70.0f * sinf(t * 5.0f));              /* ring breathes to draw the eye */

    (void)rw;
    if (down) col(r, 70, 80, 130, 240); else col(r, 24, 28, 48, 235);
    pdisc(r, bcx, bcy, br, ucell);
    col(r, 255, 255, 255, down ? 255 : pulse);
    pring(r, bcx, bcy, br, 1, ucell);

    if (near_shrine) {                                           /* a sludge drop. holding fills the button from the bottom with ooze */
        float p = shrine_progress();
        if (p > 0) {
            int hgt = (int)(2 * br * p);
            SDL_Rect clip = { bcx - br, bcy + br - hgt, 2 * br, hgt };
            SDL_RenderSetClipRect(r, &clip);
            col(r, 88, 160, 60, 245); pdisc(r, bcx, bcy, br, ucell);
            SDL_RenderSetClipRect(r, NULL);
            col(r, 255, 255, 255, 255); pring(r, bcx, bcy, br, 1, ucell);
        }
        int a = br / 7; if (a < 2) a = 2;
        static const int DW[8] = { 1, 1, 3, 3, 5, 5, 5, 3 };           /* the drop, top to bottom, in cells */
        int wob = (int)(sinf(t * 6.0f) * a * 0.3f);
        col(r, 0, 0, 0, 120);
        for (int k = 0; k < 8; k++) fill(r, bcx - DW[k] * a / 2 + a / 3, bcy - 4 * a + k * a + wob + a / 3, DW[k] * a, a);
        col(r, p > 0.5f ? 40 : 190, p > 0.5f ? 70 : 255, p > 0.5f ? 40 : 120, 255);
        for (int k = 0; k < 8; k++) fill(r, bcx - DW[k] * a / 2, bcy - 4 * a + k * a + wob, DW[k] * a, a);
        return;
    }
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

/* the death cutscene's colour: how far the world has drained (0..1) */
static float death_grey(void) {
    if (!death_on) return 0.0f;
    float k = (death_t - DEATH_GREY_AT) / DEATH_GREY_T;
    if (k < 0) k = 0;
    if (k > 1) k = 1;
    return k * k * (3.0f - 2.0f * k);
}
/* everything but the player goes grey, the player goes red (gfx.h: colour filter). off outside the cutscene */
static void scene_filter(int player) {
    if (!death_on) return;
    int g = (int)(256.0f * death_grey());
    if (player) gfx_set_filter(0, g); else gfx_set_filter(g, 0);
}

/* everything that lives on the map (tiles, hole, characters) seen from a camera at (camx, camy) */
static void draw_scene(SDL_Renderer *r, float camx, float camy) {
    int cx = (int)camx, cy = (int)camy;
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

    scene_filter(0);
    for (int ty = ty0; ty <= ty1 && ty < mh; ty++)
        for (int tx = tx0; tx <= tx1 && tx < mw; tx++)
            draw_tile(r, tx, ty, tx * tile - cx, ty * tile - cy);

    if (cur_map == MAP_CLOUD) draw_cloud_rim(r, cx, cy, tx0, ty0, tx1, ty1);

    int hcx = 0, hcy = 0;
    if (hole_on) {
        hcx = (int)((hole_tx + 0.5f) * tile) - cx;
        hcy = (int)((hole_ty + 0.5f) * tile) - cy;
        draw_hole(r, hcx, hcy, 0);
    }
    summon_draw_back(r, (int)pxp - cx, (int)pyp - cy, px, W, H);  /* the ring's far half and the column of light */

    /* characters, back to front so whoever is lower on screen draws on top. the shrine sorts with them. */
    if (shrine_foot_y() <= pyp) shrine_draw(r, cx, cy, t);
    for (int i = 0; i < npc_count(); i++)
        if (npc_foot_y(i) <= pyp) npc_draw(r, i, cx, cy);

    int sx0 = (int)pxp - cx, sy0 = (int)pyp - cy;
    scene_filter(1);                                              /* the player: red, in a grey world */
    if (summon_active()) {                                        /* the vessel forms out of the light: a gold silhouette growing up from the feet, then its real colours */
        int rows = summon_player_rows();
        if (rows > 0) {
            SDL_Rect clip = { 0, sy0 - rows * px, W, rows * px + 4 * px };      /* the rows revealed so far (+ the shadow under the feet) */
            SDL_RenderSetClipRect(r, &clip);
            gfx_set_gold(summon_player_gold());
            char_draw(r, player_look, sx0, sy0, FACE_DOWN, 0, 0.0f, px);
            gfx_set_gold(0);
            SDL_RenderSetClipRect(r, NULL);
        }
    } else if (phase == PH_SINK) {                                       /* sinking: cut off at the hole's middle */
        float k = (ph_t - 0.4f) / (SINK_T - 0.4f); if (k < 0) k = 0;
        SDL_Rect clip = { 0, 0, W, hcy + (int)(0.12f * tile) };
        SDL_RenderSetClipRect(r, &clip);
        char_draw_air(r, player_look, sx0, sy0 + (int)(k * 13.0f * px), FACE_DOWN, 0, 0.0f, px);
        SDL_RenderSetClipRect(r, NULL);
        draw_hole(r, hcx, hcy, 1);
    } else if (phase == PH_LAND || phase == PH_GETUP) {
        float k = phase == PH_GETUP ? ph_t / GETUP_T : 0.0f;
        if (k < 0.62f) {
            char_draw_prone(r, player_look, sx0, sy0, px, k / 0.62f);
        } else {
            char_draw(r, player_look, sx0, sy0, FACE_DOWN, 0, 0.0f, px);
        }
        if (phase == PH_LAND && ph_t < 0.6f) {                    /* dust kicked up by the impact */
            for (int i = 0; i < 10; i++) {
                float a = i * 0.6283f + 0.3f, d = (0.15f + ph_t * 1.6f) * tile * (0.6f + (i % 3) * 0.25f);
                col(r, 236, 230, 214, (int)(190 * (1.0f - ph_t / 0.6f)));
                int sz = px * (2 + i % 2);
                fill(r, sx0 + (int)(cosf(a) * d) - sz / 2, sy0 - 2 * px + (int)(sinf(a) * d * 0.5f), sz, sz);
            }
        }
    } else if (death_on && death_fall_t >= 0) {                   /* the vessel topples backwards and stays down */
        float k = death_fall_t / DEATH_FALL_T; if (k > 1) k = 1;
        char_draw_fall(r, player_look, sx0, sy0, px, 1.5708f * k * k);
    } else {
        char_draw(r, player_look, sx0, sy0, facing, moving, walk, px);
    }

    summon_draw_front(r, (int)pxp - cx, (int)pyp - cy, px);       /* the ring's near half, in front of the feet */
    scene_filter(0);
    for (int i = 0; i < npc_count(); i++)
        if (npc_foot_y(i) > pyp) npc_draw(r, i, cx, cy);
    if (shrine_foot_y() > pyp) shrine_draw(r, cx, cy, t);
    gfx_set_filter(0, 0); gfx_set_gold(0);                        /* never leaks into the HUD */
}

/* where the camera sits for the zoomed pass: it slides from the player to the focus point as the zoom grows
 * (never past the map edge). the scene is drawn from there at normal scale, then the middle W/zoom x H/zoom of it
 * is stretched to the screen */
static void zoom_camera(float *ccx, float *ccy) {
    float zk = (zoom - 1.0f) / (ZOOM_TALK - 1.0f);
    if (zk > 1) zk = 1;
    if (zk < 0) zk = 0;
    float x = (cam_x + W / 2.0f) + (zoom_fx - (cam_x + W / 2.0f)) * zk - W / 2.0f;
    float y = (cam_y + H / 2.0f) + (zoom_fy - (cam_y + H / 2.0f)) * zk - H / 2.0f;
    float mx = (float)(mw * tile - W), my = (float)(mh * tile - H);
    if (x > mx) x = mx;
    if (x < 0)  x = 0;
    if (y > my) y = my;
    if (y < 0)  y = 0;
    *ccx = x; *ccy = y;
}

void world_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    if (phase == PH_FALL) {                                       /* open sky, no map */
        draw_fall(r);
        hud_draw(r);
        if (t < 1.0f) { col(r, 0, 0, 0, (int)(255 * (1.0f - t))); fill(r, 0, 0, W, H); }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        return;
    }

    float zk = (zoom - 1.0f) / (ZOOM_TALK - 1.0f);
    if (zk > 0.004f && !(phase == PH_LAND || phase == PH_GETUP || phase == PH_SINK)) {
        if (zren != r || !ztex) {
            if (ztex) SDL_DestroyTexture(ztex);
            ztex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, W, H); zren = r;
        }
    }
    if (zk > 0.004f && ztex && phase == PH_PLAY) {
        /* the camera centre slides from the player to the focus point as the zoom grows (never past the map edge),
         * the scene is drawn at normal scale, then the middle W/zoom x H/zoom of it is stretched to the screen */
        float ccx, ccy; zoom_camera(&ccx, &ccy);
        SDL_SetRenderTarget(r, ztex);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        draw_scene(r, ccx, ccy);
        SDL_SetRenderTarget(r, NULL);
        SDL_Rect src = { (int)(W / 2.0f - W / (2.0f * zoom)), (int)(H / 2.0f - H / (2.0f * zoom)), (int)(W / zoom), (int)(H / zoom) };
        SDL_RenderCopy(r, ztex, &src, NULL);
    } else {
        draw_scene(r, cam_x, cam_y);
    }
    if (death_on && heart_state() != HEART_OFF) {                 /* the heart over the chest, on top of the zoomed scene */
        int fx, fy, cy, ps; world_death_player(&fx, &fy, &cy, &ps);
        heart_draw(r, fx + ps, cy, ps);                           /* a sprite pixel to the viewer's right: a human heart sits left of the middle */
    }
    if (death_on && death_msg[0]) {                               /* "Aonia has died.": white, unfiltered, in the lower third */
        gfx_set_filter(0, 0);
        float k = death_msg_t / DEATH_TEXT_FADE; if (k > 1) k = 1;
        float u1 = (W < H ? W : H) / 360.0f;
        int n = (int)strlen(death_msg), cell = (int)(5.0f * u1);
        while (cell > 2 && font_width(death_msg, cell) > W * 9 / 10) cell--;     /* never wider than the screen */
        (void)n;
        int tw = font_width(death_msg, cell), tx = (W - tw) / 2, ty = H * 4 / 5 - font_height(cell) / 2;
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        col(r, 0, 0, 0, (int)(110 * k)); fill(r, 0, ty - 3 * cell, W, font_height(cell) + 6 * cell);     /* a dark band under the words */
        col(r, 20, 0, 4, (int)(255 * k)); font_draw(r, death_msg, tx + cell, ty + cell, cell);           /* shadow */
        col(r, 255, 255, 255, (int)(255 * k)); font_draw(r, death_msg, tx, ty, cell);
    }


    if (controls_visible && !dialog_active()) {
        col(r, 255, 255, 255, 40);  pdisc(r, sx, sy, sr, ucell);
        col(r, 255, 255, 255, 150); pring(r, sx, sy, sr, 1, ucell);
        float reach = sr - kr * 0.3f;                                     /* the knob hops from cell to cell */
        int kcx = sx + (int)floorf(kx * reach / ucell + (kx < 0 ? -0.5f : 0.5f)) * ucell, kcy = sy + (int)floorf(ky * reach / ucell + (ky < 0 ? -0.5f : 0.5f)) * ucell;
        col(r, 255, 255, 255, stick_on ? 220 : 130); pdisc(r, kcx, kcy, kr, ucell);
        if (near_id >= 0 || near_hole || near_shrine) draw_interact_button(r);
    }

    if (controls_visible && phase == PH_PLAY) {                   /* minimap: top-right, under the ID card */
        static const Rgb PAL[] = {                                /* same order as the T_* enum */
            { 76, 162, 78 }, { 240, 120, 170 }, { 48, 98, 196 }, { 224, 204, 144 }, { 28, 104, 40 }, { 244, 248, 255 }, { 96, 150, 226 } };
        MiniMark marks[MAX_MARKS]; int nm = 0;
        for (int i = 0; i < npc_count() && nm < MAX_MARKS - 1; i++) { npc_tile(i, &marks[nm].tx, &marks[nm].ty); marks[nm].kind = 0; nm++; }
        if (shrine_enabled() && nm < MAX_MARKS - 1) { shrine_tile(&marks[nm].tx, &marks[nm].ty); marks[nm].kind = 2; nm++; }
        if (hole_on && hole_t >= HOLE_OPEN_T) { marks[nm].tx = hole_tx + 0.5f; marks[nm].ty = hole_ty + 0.5f; marks[nm].kind = 1; nm++; }
        minimap_draw(r, &map[0][0], MAP_MAX, mw, mh, PAL, 7, pxp / tile, pyp / tile, facing, marks, nm, t);
    }
    {   /* slide under the music button / card AND the brain button / card, whichever reaches lower: the
         * "New task added" toast hangs under the list, so it is pushed down with it and a thought sits clean */
        int o1 = nowplaying_offset(), o2 = thought_offset();
        missions_set_offset(o1 > o2 ? o1 : o2);
    }
    missions_draw(r);
    toast_draw(r, (int)(10 * (W < H ? W : H) / 360.0f), missions_bottom() + (int)(4 * (W < H ? W : H) / 360.0f));   /* "New task added", under the list */
    hud_draw(r);                                                  /* ID card, top-right */

    if (t < FADE_IN_T) {                                          /* fade in from black */
        col(r, 0, 0, 0, (int)(255 * (1.0f - t / FADE_IN_T)));
        fill(r, 0, 0, W, H);
    }
    if (blackout_t >= 0) {                                        /* fade to black over everything, HUD included */
        float a = blackout_t / blackout_dur; if (a > 1) a = 1;
        col(r, 0, 0, 0, (int)(255 * a));
        fill(r, 0, 0, W, H);
    }
    if (phase == PH_LAND && ph_t < 0.4f) {                        /* flash of white on impact */
        col(r, 255, 255, 255, (int)(255 * (1.0f - ph_t / 0.4f)));
        fill(r, 0, 0, W, H);
    }
    dialog_draw(r);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
