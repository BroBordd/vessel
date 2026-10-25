/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "world.h"
#include "char.h"
#include "dialog.h"
#include "missions.h"
#include "item.h"
#include "mapwin.h"
#include "ritual.h"
#include "story.h"
#include "font.h"
#include "nowplaying.h"
#include "toast.h"
#include "thought.h"
#include "hud.h"
#include "minimap.h"
#include "audio.h"
#include "gfx.h"
#include "bodies.h"
#include "props.h"
#include "gen.h"
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_MAX 80                  /* biggest map we can hold */
#define TILE_U 8                    /* tile edge in "pixel units" */
#define ZOOM   2.0f                 /* world zoom (pixel-art scale multiplier) */
#define WALK_TILES_PER_SEC 5.0f     /* walking speed at full stick tilt */
#define MAX_MARKS 20

/* the ruins (gen.c): floor, wall, column, rubble, the sealed gate, rock, the pit. all but the floor are solid (the pit is open to the eye).
 * keep the order: MAP_PAL */
enum { T_RFLOOR, T_RWALL, T_RPILLAR, T_RRUBBLE, T_RGATE, T_RVOID, T_RPIT };
enum { MAP_TEMPLE };                /* the one map the story has: the ruins */

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

/* interact button (fixed, bottom-right): it takes hold of a heavy thing (a body, the big rock) and lets go */
static int   bcx, bcy, br;          /* centre and radius */
static int   btn_down, btn_inside;
static int   near_drag, drag_on;    /* something heavy lies in reach (a body or the big rock): the button holds it (a tap) and lets go; drag_on: we hold it and pull it along as we walk */
static int   drag_cand = -1, drag_id = -1;   /* what the button would take hold of (DRAG_ROCK, 1 = vessel 1, n = vessel n's body; -1 none), and what we hold */
static float trail_lx, trail_ly;            /* where the last smear of the blood trail was dropped (world px) */
static float strain_t;              /* > 0: we are pulling, the eyes are shut */
static int   dragged_now;           /* this frame the pull really moved him */
static float drag_snd;              /* the friction sound's level we sent last frame */
static float drag_ox, drag_oy, drag_spd;   /* where his middle wants to be, from us (px, fixed when we took hold); how fast he really slides (cells / s) */
static int   near_inspect; static unsigned inspect_vessel;   /* story chunk 6: a body lies in reach: the hand button shows a magnifying glass and a TAP inspects it (story.c says what it carries) */
static void (*inspect_cb)(unsigned vessel);
static int   near_item;             /* chunk 68: an item lies in reach: the HAND button shows next to the interact button, and a TAP on it takes the item */
static int   hand_pid, btn_pid, stick_pid, mm_pid;     /* multitouch: which finger (0 = the primary, else the app's pointer id + 1) holds each control */
static int   hand_down, hand_inside;      /* the finger is on the hand / still on it */
static int   hbx, hby, hbr;               /* the hand button: centre and radius */
static int   mm_down;                     /* a finger went down on the minimap: let go on it and the map window opens */
static float ui;                    /* screen px per ui unit (screen width / 360) */
static int   ucell;                 /* one chunky pixel of the controls (same grid as the ID card) */

static float cam_x, cam_y;          /* the camera's top-left on screen (what draw reads) */
static float cam_cx, cam_cy;        /* the lazy camera's own centre, in world px (see camera_update) */
static int   cam_snap = 1;          /* 1 = put the camera on the player next frame, no easing (a new map, a teleport) */
static void (*pit_cb)(unsigned vessel);   /* story chunk 11: a dragged body went over the edge (declared early: the drag code uses it) */

/* AREA OF INTEREST (AOI): near the pit the camera pulls back, smoothly, so the far ledge across the crack is in view too, and eases back out when we leave.
 * the scene is drawn into a bigger texture (the view is W/aoi_z x H/aoi_z world px) and shrunk to the screen. CPU (SDL) path only: the GPU replay owns a
 * fixed W x H zoom texture and cannot copy from a bigger one (gfx_is_gpu: no AOI there, the view just stays as it is). */
#define AOI_MIN    0.60f            /* the furthest the view may pull back (the bigger texture is 1/AOI_MIN times the screen: it must stay under the GPU texture limit) */
#define AOI_NEAR   6.0f             /* tiles from the pit: this close the pull-back is complete ... */
#define AOI_FAR    10.0f            /* ... and from this far it has not begun (a smooth step in between) */
#define AOI_PAD    4.0f             /* tiles of room around the player and the far ledge */
#define AOI_SPEED  2.2f             /* exponential ease rate (1/s): a gentle pull */
static float aoi_z = 1.0f, aoi_fx, aoi_fy;      /* view scale (1 = normal, down to AOI_MIN) and the world px point at the middle of the view */
static SDL_Texture *atex; static SDL_Renderer *aren;

static const Person *player_look = &VESSEL;   /* who the player looks like (world_set_player) */
static float blackout_t = -1, blackout_dur; /* world_fade_to_black: seconds so far (-1 = not fading) and the length */
static void (*blackout_cb)(void);
static void (*ritual_cb)(void);       /* the blood circle's arrival is over (ritual.c) */

#define FADE_IN_T 1.2f              /* fade in from black when the world starts (the arrival runs over it) */
#define TEMPLE_SHADE 0.45f          /* the body in the ruins is darkened by this much: a closed-up place, no light */

/* ---------- map generation ---------- */
static uint32_t hash2(int x, int y, uint32_t seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float rnd(int x, int y, uint32_t seed) { return (hash2(x, y, seed) & 0xFFFF) / 65536.0f; }


static int solid_tile(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= mw || ty >= mh) return 1;
    uint8_t v = map[ty][tx];
    return v == T_RWALL || v == T_RPILLAR || v == T_RRUBBLE || v == T_RGATE || v == T_RVOID || v == T_RPIT;
}

/* PULLING THE HEAVY THINGS (story chunk 7: any body, and the big rock): next to one the button takes hold of it from the side nearest to us (our hands float
 * there); a tap lets go. while we hold it we walk slower, it stays on the floor and is dragged along with us (never into a wall, never onto another body or the
 * rock), our eyes shut with the effort, and the stone growls. a body leaves a blood trail (bodies.c). vessel 1's box is ritual.c's, a body's is bodies.c's, the
 * rock's is props.c's: one id says which (DRAG_ROCK, 1, or the vessel number) */
#define DRAG_ROCK   0
#define DRAG_SPEED  0.75f       /* walking speed while we hold a body (the owner's tuning) */
#define ROCK_SPEED  0.50f       /* ... and the big rock: SIZE_BIG slows us a lot (item.h) */
#define INSPECT_REACH 5.0f      /* cells: how far from a body's edge the magnifying glass shows */
#define DRAG_REACH  4.0f        /* cells: how far from its edge we may be to take hold */
#define DRAG_TAU    0.22f       /* its lag: like the camera, it eases toward where our hands would have it (seconds) */
#define ROCK_TAU    0.36f       /* the rock is slower to start and to stop */
#define DRAG_SLACK  1.5f        /* cells: the rope is slack this much, it does not move until it is tight (a sandbag has to be started) */
#define DRAG_MAXLAG 8.0f        /* cells: the most it can trail behind (our arms reach that far); at the limit we cannot walk on */
#define ROCK_MAXLAG 5.0f        /* ... the rock has less rope */
#define DRAG_STRAIN 0.30f       /* seconds the eyes stay shut after the last bit of pulling (no flicker) */
#define TRAIL_STEP  2.5f        /* cells of dragging between two smears of blood */
static int v1_box_px(float *x0, float *y0, float *x1, float *y1) {            /* vessel 1's box in world px, 0 if there is no such body */
    float a, b, c, d, rx, ry;
    if (!ritual_scene() || cur_map != MAP_TEMPLE || !ritual_body_box(&a, &b, &c, &d)) return 0;
    ritual_home(&rx, &ry);                                      /* vessel 1 lies by the FIRST circle, wherever the new bodies wake */
    *x0 = rx * tile + a * px; *y0 = ry * tile + b * px; *x1 = rx * tile + c * px; *y1 = ry * tile + d * px;
    return 1;
}
static int target_box_px(int id, float *x0, float *y0, float *x1, float *y1) {   /* the box of a heavy thing in world px (DRAG_ROCK, 1 or a vessel number), 0 if it is not here */
    if (cur_map != MAP_TEMPLE || id < 0) return 0;
    if (id == DRAG_ROCK) return props_rock_box(px, tile, x0, y0, x1, y1);
    if (id == 1) return v1_box_px(x0, y0, x1, y1);
    return bodies_box((unsigned)id, px, tile, x0, y0, x1, y1);
}
static int drag_box_px(float *x0, float *y0, float *x1, float *y1) { return target_box_px(drag_on ? drag_id : drag_cand, x0, y0, x1, y1); }   /* what we hold (or could hold) */
static float body_gap(void) {                                               /* cells between our feet and vessel 1's box (1e9: none) */
    float x0, y0, x1, y1;
    if (!v1_box_px(&x0, &y0, &x1, &y1)) return 1e9f;
    float dx = fmaxf(fmaxf(x0 - (pxp + 3.0f * px), (pxp - 3.0f * px) - x1), 0.0f), dy = fmaxf(fmaxf(y0 - pyp, (pyp - 3.0f * px) - y1), 0.0f);
    return sqrtf(dx * dx + dy * dy) / (float)px;
}
static void drag_pick(void) {                                               /* the nearest heavy thing within DRAG_REACH cells of our feet: vessel 1, another body or the rock */
    drag_cand = -1;
    if (cur_map != MAP_TEMPLE) return;
    float best = DRAG_REACH, g = body_gap();
    if (g <= best) { best = g; drag_cand = 1; }
    int bi = bodies_nearest(pxp, pyp, px, tile, best, &g);
    if (bi >= 0 && g <= best) { best = g; drag_cand = (int)bodies_get(bi)->vessel; }
    g = props_rock_gap(pxp, pyp, px, tile);
    if (g <= best) drag_cand = DRAG_ROCK;
}
static int body_hits(float fx, float fy) {                                  /* would our feet box at (fx, fy) stand on vessel 1, on any other body (bodies.c) or on the rock? */
    float x0, y0, x1, y1;
    if (cur_map == MAP_TEMPLE && (bodies_hit(fx, fy, px, tile) || props_rock_hit(fx, fy, px, tile))) return 1;
    if (!v1_box_px(&x0, &y0, &x1, &y1)) return 0;
    return fx + 3.0f * px > x0 && fx - 3.0f * px < x1 && fy > y0 && fy - 3.0f * px < y1;
}
static int box_overlap(float a0, float b0, float a1, float b1, float c0, float d0, float c1, float d1) { return a0 < c1 && a1 > c0 && b0 < d1 && b1 > d0; }
static int drag_blocked(float sx, float sy) {                               /* would what we hold end up in a wall, or on another body or the rock, if it moved by (sx, sy) px? */
    float x0, y0, x1, y1;
    if (!drag_box_px(&x0, &y0, &x1, &y1)) return 0;
    float n0 = x0 + sx, m0 = y0 + sy, n1 = x1 + sx, m1 = y1 + sy;           /* the box where it would be */
    float wx1 = n1 - 1, wy1 = m1 - 1;
    for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++)
        if (solid_tile((int)floorf((n0 + (wx1 - n0) * 0.5f * i) / tile), (int)floorf((m0 + (wy1 - m0) * 0.5f * j) / tile))) return 1;
    float c0, d0, c1, d1;
    if (drag_id != 1 && v1_box_px(&c0, &d0, &c1, &d1) && box_overlap(n0, m0, n1, m1, c0, d0, c1, d1) && !box_overlap(x0, y0, x1, y1, c0, d0, c1, d1)) return 1;
    if (drag_id != DRAG_ROCK && props_rock_box(px, tile, &c0, &d0, &c1, &d1) && box_overlap(n0, m0, n1, m1, c0, d0, c1, d1) && !box_overlap(x0, y0, x1, y1, c0, d0, c1, d1)) return 1;
    for (int i = 0; i < bodies_count(); i++) {
        const Body *b = bodies_get(i);
        if (!b || b->vessel == 1 || (int)b->vessel == drag_id || !bodies_box(b->vessel, px, tile, &c0, &d0, &c1, &d1)) continue;
        if (box_overlap(n0, m0, n1, m1, c0, d0, c1, d1) && !box_overlap(x0, y0, x1, y1, c0, d0, c1, d1)) return 1;     /* (two that already overlap may still be pulled apart) */
    }
    return 0;
}
static float drag_lag(float ppx, float ppy) {                              /* how far its middle is from where our hands want it, if we stood at (ppx, ppy) */
    float x0, y0, x1, y1;
    if (!drag_box_px(&x0, &y0, &x1, &y1)) return 0;
    return hypotf(ppx + drag_ox - (x0 + x1) * 0.5f, ppy + drag_oy - (y0 + y1) * 0.5f);
}
static int drag_taut(float dx, float dy) {                                  /* a step that would stretch the rope past its limit (and not ease it) */
    float now = drag_lag(pxp, pyp), then = drag_lag(pxp + dx, pyp + dy);
    float lim = (drag_id == DRAG_ROCK ? ROCK_MAXLAG : DRAG_MAXLAG) * px;
    return then > lim && then > now;
}
static void drag_move(float dx, float dy) {                                 /* the held thing moves by (dx, dy) px */
    if (drag_id == DRAG_ROCK) props_rock_shift(dx, dy, tile);
    else if (drag_id == 1) ritual_body_shift(dx / px, dy / px);
    else bodies_shift((unsigned)drag_id, dx, dy, tile);
}
static float smash_cd; static void (*smash_cb)(unsigned vessel);
void world_set_smash_cb(void (*cb)(unsigned vessel)) { smash_cb = cb; }
static unsigned rock_body_hit(float sx, float sy) {                         /* the body the rock would run into if it moved by (sx, sy) px: its vessel number, 0 = none */
    float x0, y0, x1, y1;
    if (!props_rock_box(px, tile, &x0, &y0, &x1, &y1)) return 0;
    float n0 = x0 + sx - px, m0 = y0 + sy - px, n1 = x1 + sx + px, m1 = y1 + sy + px;     /* a cell of reach all round: it is stopped by the body before it overlaps */
    float c0, d0, c1, d1;
    if (v1_box_px(&c0, &d0, &c1, &d1) && box_overlap(n0, m0, n1, m1, c0, d0, c1, d1)) return 1;
    for (int i = 0; i < bodies_count(); i++) {
        const Body *b = bodies_get(i);
        if (!b || b->vessel == 1 || !bodies_box(b->vessel, px, tile, &c0, &d0, &c1, &d1)) continue;
        if (box_overlap(n0, m0, n1, m1, c0, d0, c1, d1)) return b->vessel;
    }
    return 0;
}
static int pit_probe(float x, float y) { int tx = (int)floorf(x / tile), ty = (int)floorf(y / tile); return tx >= 0 && ty >= 0 && tx < mw && ty < mh && map[ty][tx] == T_RPIT; }
static int drag_into_pit(float sx, float sy) {                              /* chunk 11: would what we hold end up over the crack if it moved by (sx, sy) px? */
    float x0, y0, x1, y1;
    if (!drag_box_px(&x0, &y0, &x1, &y1)) return 0;
    float n0 = x0 + sx, m0 = y0 + sy, n1 = x1 + sx, m1 = y1 + sy, wx1 = n1 - 1, wy1 = m1 - 1;
    for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++)
        if (pit_probe(n0 + (wx1 - n0) * 0.5f * i, m0 + (wy1 - m0) * 0.5f * j)) return 1;
    return 0;
}
static void drag_step(float dt) {                                           /* the sandbag: it eases toward our hands, lazily, and stops when the rope goes slack */
    drag_spd = 0;
    float x0, y0, x1, y1;
    if (!drag_on || dt <= 0 || !drag_box_px(&x0, &y0, &x1, &y1)) return;
    float ex = pxp + drag_ox - (x0 + x1) * 0.5f, ey = pyp + drag_oy - (y0 + y1) * 0.5f, len = hypotf(ex, ey);
    if (len < 0.001f) return;
    int rock = drag_id == DRAG_ROCK;
    float eff = len - DRAG_SLACK * px; if (eff < 0) eff = 0;
    float mv = eff * (1.0f - expf(-dt / (rock ? ROCK_TAU : DRAG_TAU)));
    float lim = (rock ? ROCK_MAXLAG : DRAG_MAXLAG) * px;
    if (len - mv > lim) mv = len - lim;
    float mx = ex / len * mv, my = ey / len * mv;
    if (!rock && drag_id != 1 && drag_id >= 0 && mv > 0.2f * px && (drag_into_pit(mx, 0) || drag_into_pit(0, my))) {      /* chunk 11: pushed over the edge: it falls */
        unsigned v = (unsigned)drag_id; drag_on = 0;
        bodies_remove(v);
        if (pit_cb) pit_cb(v);
        return;
    }
    if (rock && mv > 0.3f * px && smash_cd <= 0) {                           /* chunk 9: the rock pushed into a body smashes it */
        unsigned hit = rock_body_hit(mx, my);
        if (hit && smash_cb) { smash_cd = 1.2f; sfx_hit(); smash_cb(hit); }
    }
    if (smash_cd > 0) smash_cd -= dt;
    if (drag_blocked(mx, 0)) mx = 0; else drag_move(mx, 0);
    if (drag_blocked(0, my)) my = 0; else drag_move(0, my);
    drag_spd = hypotf(mx, my) / dt / (float)px;
    if (!rock && (mx != 0 || my != 0) && drag_box_px(&x0, &y0, &x1, &y1)) {      /* a body leaves a smear of blood every few cells (the rock leaves nothing) */
        float bx = (x0 + x1) * 0.5f, by = (y0 + y1) * 0.5f;
        if (hypotf(bx - trail_lx, by - trail_ly) >= TRAIL_STEP * px) { bodies_trail_add(bx, by, tile); trail_lx = bx; trail_ly = by; }
    }
}
static void drag_toggle(void) {
    if (drag_on) { drag_on = 0; return; }
    float x0, y0, x1, y1;
    if (drag_cand < 0 || !target_box_px(drag_cand, &x0, &y0, &x1, &y1)) return;
    drag_id = drag_cand; drag_on = 1; drag_spd = 0;
    drag_ox = (x0 + x1) * 0.5f - pxp; drag_oy = (y0 + y1) * 0.5f - pyp;
    trail_lx = (x0 + x1) * 0.5f; trail_ly = (y0 + y1) * 0.5f;
    float cyy = pyp - 4.0f * px, nx = pxp < x0 ? x0 : pxp > x1 ? x1 : pxp, ny = cyy < y0 ? y0 : cyy > y1 ? y1 : cyy;      /* turn to the point we hold it at */
    float fdx = nx - pxp, fdy = ny - cyy;
    if (fabsf(fdx) > fabsf(fdy)) facing = fdx < 0 ? FACE_LEFT : FACE_RIGHT;
    else                         facing = fdy < 0 ? FACE_UP : FACE_DOWN;
}

/* the deep temple: gen.c makes it from a seed. VESSEL_SEED=n in the environment pins the seed (the same temple every run), else it is new each game.
 * the seed is printed so a good (or broken) temple can be asked for again */
static uint32_t temple_seed;
static int temple_sx, temple_sy;            /* where we wake */
static int temple_gx = -1, temple_gy = -1;  /* the sealed gate */
static int pit_x0 = -1, pit_y0 = -1, pit_x1 = -1, pit_y1 = -1;   /* the pit's box (tiles), -1: none */
/* story chunk 11: THE BRIDGE OF BODIES. every body that goes into the pit turns the lowest crack tile of the middle column (edge_tx, from the near ledge up to the far
 * ledge) into floor. pit_pile counts them and outlives the map: a new map (a death) is filled again from it (gen_temple_map). when the column is floor the far
 * ledge can be walked to */
static int pit_pile;

static int far_tx = -1, far_ty = -1, edge_tx = -1, edge_ty = -1;  /* the far ledge's spot (the second circle) and the near ledge's middle */
static int pit_fill_one(void) {
    if (pit_x0 < 0 || edge_tx < 0) return 0;
    for (int y = pit_y1; y >= pit_y0; y--) if (map[y][edge_tx] == T_RPIT) { map[y][edge_tx] = T_RFLOOR; return 1; }
    return 0;
}
static void gen_temple_map(void) {
    GenInfo gi;
    static uint8_t g[MAP_MAX][MAP_MAX];
    if (!gen_temple(&g[0][0], MAP_MAX, MAP_MAX, temple_seed, &gi)) {                       /* (cannot happen with this size: a plain room is the fallback) */
        memset(&gi, 0, sizeof gi); gi.w = gi.h = 20; gi.spawn_x = gi.spawn_y = 10; gi.gate_x = gi.gate_y = -1; gi.pit_x0 = gi.pit_y0 = gi.pit_x1 = gi.pit_y1 = gi.far_x = gi.far_y = gi.edge_x = gi.edge_y = -1;
        for (int y = 0; y < 20; y++) for (int x = 0; x < 20; x++) g[y][x] = (x == 0 || y == 0 || x == 19 || y == 19) ? GEN_WALL : GEN_FLOOR;
    }
    mw = gi.w; mh = gi.h;
    static const uint8_t to_tile[] = { T_RVOID, T_RFLOOR, T_RWALL, T_RPILLAR, T_RRUBBLE, T_RGATE, T_RPIT };
    for (int y = 0; y < MAP_MAX; y++) for (int x = 0; x < MAP_MAX; x++) map[y][x] = (y < mh && x < mw) ? to_tile[g[y][x]] : T_RVOID;
    temple_sx = gi.spawn_x; temple_sy = gi.spawn_y; temple_gx = gi.gate_x; temple_gy = gi.gate_y;
    pit_x0 = gi.pit_x0; pit_y0 = gi.pit_y0; pit_x1 = gi.pit_x1; pit_y1 = gi.pit_y1; far_tx = gi.far_x; far_ty = gi.far_y; edge_tx = gi.edge_x; edge_ty = gi.edge_y;
    for (int i = 0; i < pit_pile; i++) pit_fill_one();                  /* story chunk 11: the bodies that went in before stay in */
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

/* ---------- the deep temple: dark stone, lit only round us (a torch's worth, with a slight flicker) ---------- */
static float ruin_k(int tx, int ty) {
    float dx = tx + 0.5f - pxp / tile, dy = ty + 0.5f - pyp / tile;
    float k = 1.0f - sqrtf(dx * dx + dy * dy) / 8.5f;
    if (k < 0.0f) k = 0.0f;
    k = k * k * (3.0f - 2.0f * k);
    k *= 0.95f + 0.05f * sinf(t * 7.0f) * sinf(t * 3.1f + tx * 0.2f);
    return 0.06f + 0.94f * k;
}
static void rc(SDL_Renderer *r, int R, int G, int B, float k) { col(r, (int)(R * k), (int)(G * k), (int)(B * k), 255); }
static int ruin_solid(int tx, int ty) { return tx < 0 || ty < 0 || tx >= mw || ty >= mh || map[ty][tx] == T_RWALL || map[ty][tx] == T_RVOID || map[ty][tx] == T_RGATE; }
static void ruin_floor(SDL_Renderer *r, int tx, int ty, int x, int y, float k) {
    float h = rnd(tx, ty, 41);
    int v = 54 + (int)(h * 14.0f);
    rc(r, v, v - 3, v + 10, k); fill(r, x, y, tile, tile);
    rc(r, v - 14, v - 16, v - 4, k); fill(r, x, y, tile, px); fill(r, x, y, px, tile);              /* the joints between the flags */
    if (h > 0.86f) { fill(r, x + 2 * px, y + 3 * px, px, px); fill(r, x + 3 * px, y + 4 * px, 2 * px, px); fill(r, x + 5 * px, y + 5 * px, px, 2 * px); }   /* a crack */
    if (h < 0.07f) { rc(r, 40, 72, 48, k); fill(r, x + (1 + (int)(h * 100.0f) % 4) * px, y + 4 * px, 2 * px, px); fill(r, x + 5 * px, y + 5 * px, px, px); }    /* moss */
}
static void draw_ruin_tile(SDL_Renderer *r, int v, int tx, int ty, int x, int y) {
    float k = ruin_k(tx, ty);
    if (v == T_RVOID) { col(r, 0, 0, 0, 255); fill(r, x, y, tile, tile); return; }
    if (v == T_RFLOOR) { ruin_floor(r, tx, ty, x, y, k); return; }
    if (v == T_RPIT) {                                                              /* the crack: black, deeper the further in, the far lip's cliff face on its north side, a ragged near lip */
        uint32_t hh = hash2(tx, ty, 77);
        col(r, 8, 6, 16, 255); fill(r, x, y, tile, tile);
        col(r, 4, 3, 9, 255); fill(r, x, y + 4 * px, tile, 4 * px);
        if (hh % 9 == 0) { col(r, 48, 36, 86, (int)(70 * k) + 20); fill(r, x + (int)((hh >> 5) % 6) * px, y + (int)((hh >> 9) % 6) * px, px, px); }       /* a far-away glint */
        int above_pit = ty > 0 && map[ty - 1][tx] == T_RPIT, below_pit = ty + 1 < mh && map[ty + 1][tx] == T_RPIT;
        if (!above_pit) {                                                            /* the north lip is a cliff: its face runs down into the dark */
            rc(r, 60, 54, 72, k); fill(r, x, y, tile, 2 * px);
            rc(r, 42, 38, 54, k); fill(r, x, y + 2 * px, tile, 2 * px);
            rc(r, 26, 22, 36, k); fill(r, x, y + 4 * px, tile, px);
            rc(r, 16, 13, 24, k); for (int i = 0; i < 4; i++) fill(r, x + (int)((hh >> (i * 3)) % 8) * px, y + 2 * px, px, (2 + (int)((hh >> (i + 9)) % 3)) * px);   /* cracks in the face */
        }
        if (!below_pit) {                                                            /* the south lip: crumbling chips of floor stick out over the edge */
            for (int i = 0; i < 8; i++) {
                int hgt = (int)((hh >> (i * 2)) % 4u);
                if (hgt) { int v2 = 54 + (int)((hh >> 3) % 12u); rc(r, v2, v2 - 3, v2 + 10, k); fill(r, x + i * px, y + tile - hgt * px, px, hgt * px); }
            }
        }
        return;
    }
    if (v == T_RWALL || v == T_RGATE) {
        int front = !ruin_solid(tx, ty + 1);                                       /* floor below: the wall's face shows */
        rc(r, 34, 30, 44, k); fill(r, x, y, tile, tile);
        if (front) { rc(r, 30, 26, 38, k); fill(r, x, y, tile, 3 * px); rc(r, 60, 54, 72, k); fill(r, x, y + 3 * px, tile, 5 * px); }
        rc(r, 22, 18, 30, k);
        for (int row = 0; row < 2; row++) {                                        /* bricks: a joint along each course, and the upright joints staggered */
            int by = y + (front ? 3 : 0) * px + row * (front ? 2 : 4) * px;
            fill(r, x, by, tile, px);
            fill(r, x + (((tx + row) & 1) ? 2 : 5) * px, by, px, (front ? 2 : 4) * px);
        }
        if (v == T_RGATE) {                                                         /* the sealed gate: a black door, runes breathing violet */
            float pulse = 0.5f + 0.5f * sinf(t * 1.6f);
            rc(r, 12, 8, 18, 0.35f + 0.65f * k); fill(r, x + px, y + px, 6 * px, 7 * px);
            col(r, 170, 70, 220, (int)(70 + 110 * pulse));
            fill(r, x + 3 * px, y + 2 * px, 2 * px, px); fill(r, x + 3 * px, y + 4 * px, 2 * px, px); fill(r, x + 2 * px, y + 3 * px, px, px); fill(r, x + 5 * px, y + 3 * px, px, px);
            col(r, 150, 60, 210, (int)(30 + 40 * pulse)); fill(r, x, y + tile - 2 * px, tile, 2 * px);
        }
        return;
    }
    ruin_floor(r, tx, ty, x, y, k);                                                 /* a column and rubble stand on the floor */
    if (v == T_RPILLAR) {
        col(r, 0, 0, 0, 70); fill(r, x + 5 * px, y + 5 * px, 3 * px, 3 * px);
        rc(r, 96, 90, 108, k); fill(r, x + 2 * px, y, 4 * px, tile);
        rc(r, 134, 128, 144, k); fill(r, x + 2 * px, y, px, tile);
        rc(r, 62, 58, 76, k); fill(r, x + 5 * px, y, px, tile);
        rc(r, 124, 118, 134, k); fill(r, x + px, y, 6 * px, 2 * px);                 /* the capital */
        rc(r, 110, 104, 120, k); fill(r, x + px, y + 6 * px, 6 * px, 2 * px);        /* the plinth */
    } else {
        col(r, 0, 0, 0, 60); fill(r, x + px, y + 5 * px, 6 * px, 2 * px);
        for (int i = 0; i < 4; i++) {
            int rx = x + (int)(rnd(tx, ty, 50 + i) * 5.0f) * px, ry = y + 1 * px + (int)(rnd(tx, ty, 60 + i) * 4.0f) * px, rw = (2 + (i & 1)) * px;
            rc(r, 86, 82, 92, k); fill(r, rx, ry, rw, 2 * px);
            rc(r, 120, 114, 126, k); fill(r, rx, ry, rw, px);
        }
    }
}

/* FOG OF WAR on the minimap and the map window: a tile is drawn only once we have seen it (within REVEAL_R tiles with a clear line, walls stop the sight).
 * map_disp is what they get: the real tiles where seen, 255 (colour index off the palette = black) elsewhere. reset with every map */
#define REVEAL_R 7
static uint8_t seen[MAP_MAX][MAP_MAX];
static uint8_t map_disp[MAP_MAX][MAP_MAX];
static int opaque_tile(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= mw || ty >= mh) return 1;
    uint8_t v = map[ty][tx];
    return v == T_RWALL || v == T_RVOID || v == T_RGATE;
}
static void reveal(void) {
    int cx = (int)(pxp / tile), cy = (int)(pyp / tile);
    for (int dy = -REVEAL_R; dy <= REVEAL_R; dy++)
        for (int dx = -REVEAL_R; dx <= REVEAL_R; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || y < 0 || x >= mw || y >= mh || seen[y][x] || dx * dx + dy * dy > REVEAL_R * REVEAL_R + REVEAL_R) continue;
            int n = abs(dx) > abs(dy) ? abs(dx) : abs(dy), ok = 1;
            for (int i = 1; i < n && ok; i++)                                  /* the line from our tile to that one: anything solid-walled in between hides it */
                if (opaque_tile(cx + (int)floorf(dx * (float)i / n + 0.5f), cy + (int)floorf(dy * (float)i / n + 0.5f))) ok = 0;
            if (ok) seen[y][x] = 1;
        }
    if (cx >= 0 && cy >= 0 && cx < mw && cy < mh) seen[cy][cx] = 1;
}
static const uint8_t *map_for_display(void) {
    for (int y = 0; y < mh; y++) for (int x = 0; x < mw; x++) map_disp[y][x] = seen[y][x] ? map[y][x] : 255;
    return &map_disp[0][0];
}

/* what the minimap and the map window show: the tile colours (same order as the T_* enum). no marks yet */
static const Rgb MAP_PAL[] = {
    { 74, 70, 92 }, { 40, 36, 52 }, { 120, 114, 132 }, { 96, 92, 104 }, { 170, 70, 220 }, { 0, 0, 0 }, { 22, 14, 40 } };      /* floor, wall, column, rubble, gate, rock, pit */
#define MAP_NPAL 7
static int gather_marks(MiniMark *marks) { (void)marks; return 0; }
static void map_source(MapView *v) {                        /* mapwin.h asks for this every frame while the window is open */
    static MiniMark marks[MAX_MARKS];
    v->tiles = map_for_display(); v->stride = MAP_MAX; v->mw = mw; v->mh = mh; v->pal = MAP_PAL; v->npal = MAP_NPAL;
    v->ptx = pxp / tile; v->pty = pyp / tile; v->facing = facing;
    v->nmarks = gather_marks(marks); v->marks = marks;
}

/* ---------- API ---------- */
/* makes the ruins (again): the player goes to the spawn, nothing is known yet but what can be seen from there */
static void load_map(void) {
    cur_map = MAP_TEMPLE; cam_snap = 1;
    gen_temple_map();
    ritual_cancel(); ritual_cb = NULL;           /* the circle is part of the map */
    item_reset(px, tile);                        /* what lay on the old map is gone; the pockets stay */
    near_drag = drag_on = 0; drag_cand = drag_id = -1; strain_t = 0; near_item = near_inspect = hand_down = hand_inside = 0; mm_down = 0;
    btn_down = btn_inside = 0;
    pxp = (temple_sx + 0.5f) * tile; pyp = (temple_sy + 0.5f) * tile;   /* the first room's middle */
    {                                            /* dev: VESSEL_AT=edge | far puts us on the near ledge of the pit / on the far ledge (screenshots, tests) */
        const char *e = getenv("VESSEL_AT"); int fx, fy, ex, ey;
        if (e && world_pit_spots(&fx, &fy, &ex, &ey)) {
            if (!strcmp(e, "edge")) { pxp = (ex + 0.5f) * tile; pyp = (ey + 0.5f) * tile; }
            else if (!strcmp(e, "far")) { pxp = (fx + 0.5f) * tile; pyp = (fy + 0.5f) * tile; }
        }
    }
    facing = FACE_DOWN;
    memset(seen, 0, sizeof seen); reveal();      /* a new map: nothing is known yet but what we can see from the spawn */
}

/* the layout numbers of the world that depend on the screen (size, density unit, stick, buttons). world_init and world_resize share it. a changed
 * tile size (only when the SHORT side changed: a rotation keeps it) moves everything kept in world px with it, so the player stays on the same tile */
static float cam_axis(float c, int map_px, int screen);
static void world_layout(int w, int h) {
    int old_tile = tile;
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    px = (int)(u * 3.0f * ZOOM); if (px < 2) px = 2;
    tile = TILE_U * px;
    if (old_tile > 0 && tile != old_tile) {
        float k = (float)tile / (float)old_tile;
        pxp *= k; pyp *= k; cam_cx *= k; cam_cy *= k;
    }
    ui = u;
    ucell = (int)(3.2f * u); if (ucell < 3) ucell = 3;
    int margin = (int)(26 * u);
    sr = (int)(58 * u); kr = (int)(24 * u);
    sx = margin + sr; sy = H - margin - sr;
    br = (int)(44 * u);
    bcx = W - margin - br; bcy = H - margin - br;
    hbr = br * 3 / 4; hbx = bcx - br - hbr - (int)(10 * u); hby = bcy + br - hbr;                      /* chunk 68: the hand, on the interact button's left, resting on the same line */
}

/* where the inventory hangs. portrait (tall): under the minimap, same right edge and width, as it always did. a wide, short screen (landscape, a
 * short split-screen window) has no room under the minimap: card + minimap + bag already reach ~230u of 360u and the open panel would run over the
 * interact button, so there the bag and its panel sit BESIDE the minimap instead: same top, same width, just left of it. cx, cy, cw, ch = the ID card. */
static void item_anchor(int cx, int cy, int cw, int ch, int *right_edge, int *top) {
    if (H < (int)(480 * ui)) { *right_edge = cx - (int)(5 * ui); *top = cy + ch + (int)(5 * ui); }
    else                     { *right_edge = cx + cw;            *top = minimap_bottom() + (int)(5 * ui); }
}

/* the screen changed shape (a rotation, a resize): layout only, every bit of state stays (the map, the player, windows, the story).
 * the resizes run in world_init's order: hud, then minimap (needs the card), then item_ui (needs minimap_bottom) */
void world_resize(int w, int h) {
    if (w == W && h == H) return;
    world_layout(w, h);
    if (stick_on) { stick_on = 0; kx = ky = 0; }                /* a finger held on a control is let go (game.c also cancels the touches) */
    dialog_resize(w, h);
    missions_resize(w, h);
    toast_resize(w, h);
    thought_resize(w, h);
    hud_resize(w, h);
    {
        int cx, cy, cw, ch; hud_card_rect(&cx, &cy, &cw, &ch);
        minimap_resize(w, h, cx + cw, cy + ch + (int)(5 * ui), cw);
        mapwin_resize(w, h);
        int ire, itop; item_anchor(cx, cy, cw, ch, &ire, &itop);
        item_ui_resize(w, h, ire, itop, cw);
    }
    if (atex) { SDL_DestroyTexture(atex); atex = NULL; }          /* the AOI target is bigger than the screen too: made again at the new size */
    cam_x = cam_axis(cam_cx - W / 2.0f, mw * tile, W);           /* the lazy camera keeps its centre (cam_cx) and is only re-clamped to the new view */
    cam_y = cam_axis(cam_cy - H / 2.0f, mh * tile, H);
    cam_cx = cam_x + W / 2.0f; cam_cy = cam_y + H / 2.0f;
}

void world_init(int w, int h) {
    tile = 0;                                                   /* a fresh start: nothing to rescale */
    world_layout(w, h);
    t = 0; walk = 0; moving = 0; facing = 0; stick_on = 0; kx = ky = 0;
    controls_visible = 1; btn_down = btn_inside = 0;
    aoi_z = 1.0f;
    blackout_t = -1; blackout_cb = NULL;
    float u = ui;

    dialog_init(w, h);
    missions_init(w, h);
    toast_init(w, h);
    thought_init(w, h);
    hud_init(w, h, &VAS);                       /* the ID card starts out as plain "Vas" */
    {   /* the minimap hangs right under the ID card: same right edge, same width */
        int cx, cy, cw, ch; hud_card_rect(&cx, &cy, &cw, &ch);
        minimap_init(w, h, cx + cw, cy + ch + (int)(5 * u), cw);
        mapwin_init(w, h); mapwin_set_source(map_source);
        int ire, itop; item_anchor(cx, cy, cw, ch, &ire, &itop);
        item_ui_init(w, h, ire, itop, cw);       /* the inventory hangs under the minimap (beside it on a wide, short screen: item_anchor) */
    }
    temple_seed = (uint32_t)time(NULL);
    { const char *e = getenv("VESSEL_SEED"); if (e && atoi(e) > 0) temple_seed = (uint32_t)atoi(e); }
    printf("temple seed %u\n", temple_seed);
    load_map();                                 /* the story starts in the ruins (gen.c) */
    story_start();                              /* the script takes it from here (story.c) */
    world_set_controls_visible(0);              /* story_start has begun the blood circle (world_ritual): the controls come back when the new body has formed */
}

/* the blood circle (ritual.c): carved into the floor where the player will stand, the dead vessel `dead` beside it. the body is drawn up out
 * of the blood when the grooves are full; on_done (may be NULL) runs then and the controls come back */
void world_ritual(const Person *dead, void (*on_done)(void)) {
    ritual_cb = on_done;
    ritual_begin(pxp / (float)tile, pyp / (float)tile, dead);
    world_set_controls_visible(0);
    moving = 0; walk = 0; facing = FACE_DOWN;
}
/* story chunk 13: the body wakes at the SECOND circle (the far ledge), whose grooves are full already. the player has been put on its centre first (world_player_to_tile) */
void world_ritual_c2(void (*on_done)(void)) {
    ritual_cb = on_done;
    ritual_begin_c2();
    world_set_controls_visible(0);
    moving = 0; walk = 0; facing = FACE_DOWN;
}
void world_player_to_tile(float tx, float ty) {             /* the feet go there (tiles), the camera does not slide */
    pxp = tx * (float)tile; pyp = ty * (float)tile; cam_snap = 1; facing = FACE_DOWN;
    memset(seen, 0, sizeof seen); reveal();
}
int world_ritual_running(void) { return ritual_active(); }

void world_set_controls_visible(int on) {
    controls_visible = on;
    if (!on) { stick_on = 0; kx = ky = 0; btn_down = 0; }
}
void world_player_tilef(float *x, float *y) { if (x) *x = pxp / (float)tile; if (y) *y = pyp / (float)tile; }
int world_controls_on(void) { return controls_visible; }
int world_dragging(void) { return drag_on && drag_id != DRAG_ROCK; }          /* a body (the thoughts about bodies) */
int world_dragging_rock(void) { return drag_on && drag_id == DRAG_ROCK; }
int world_drag_moving(void) { return dragged_now; }

/* the pit (gen.c): how far (tiles) the player's feet are from the nearest pit tile; 1e9 when this map has none. the story uses it for "arrives at the edge" */
float world_dist_to_pit(void) {
    if (cur_map != MAP_TEMPLE || pit_x0 < 0) return 1e9f;
    float best = 1e9f, fx = pxp / tile, fy = pyp / tile;
    for (int y = pit_y0; y <= pit_y1; y++) for (int x = pit_x0; x <= pit_x1; x++) {
        if (map[y][x] != T_RPIT) continue;
        float dx = fx - (x + 0.5f), dy = fy - (y + 0.5f), d = sqrtf(dx * dx + dy * dy);
        if (d < best) best = d;
    }
    return best;
}
/* story chunk 8: the body nearest to us within the magnifying glass's reach (the one a stab goes to): its vessel number (1 = Aonia), 0 when none lies close */
/* ---- story chunk 11: the pit ---- */
void world_pit_reset(void) { pit_pile = 0; }
int  world_pit_pile(void) { return pit_pile; }
int  world_pit_left(void) {                                                 /* crack tiles still open in the bridge column */
    int n = 0;
    if (cur_map != MAP_TEMPLE || pit_x0 < 0 || edge_tx < 0) return 0;
    for (int y = pit_y0; y <= pit_y1; y++) if (map[y][edge_tx] == T_RPIT) n++;
    return n;
}
int  world_pit_add(void) { pit_pile++; if (cur_map == MAP_TEMPLE) pit_fill_one(); return world_pit_left(); }
static float pit_push_t;
float world_pit_push_time(void) { return pit_push_t; }
void  world_pit_push_clear(void) { pit_push_t = 0; }
/* (declared above, before the drag code uses it) */
void world_set_pit_cb(void (*cb)(unsigned vessel)) { pit_cb = cb; }
/* THE SWING across the crack: the player flies from here to the far ledge in an arc of time (controls off), then done runs. 0 = there is no far ledge */
static int swing_on; static float swing_t, swing_sx, swing_sy, swing_ex, swing_ey; static void (*swing_done)(void);
int world_swing_across(void (*done)(void)) {
    if (cur_map != MAP_TEMPLE || far_tx < 0 || swing_on) return 0;
    swing_on = 1; swing_t = 0; swing_sx = pxp; swing_sy = pyp; swing_ex = (far_tx + 0.5f) * tile; swing_ey = (far_ty + 0.5f) * tile; swing_done = done;
    drag_on = 0; moving = 0; stick_on = 0; kx = ky = 0;
    world_set_controls_visible(0);
    facing = swing_ey < swing_sy ? FACE_UP : FACE_DOWN;
    return 1;
}
/* story chunk 9: the middle of a body (feet, tiles) for the parts to fall around. 0 when that vessel has no body here */
int world_body_tile(unsigned vessel, float *tx, float *ty) {
    float x0, y0, x1, y1;
    if (!target_box_px((int)vessel, &x0, &y0, &x1, &y1)) return 0;
    *tx = (x0 + x1) * 0.5f / tile; *ty = y1 / tile;
    return 1;
}
/* story chunk 15: the nearest tile that is not solid around (tx, ty) (rings outward), for things that are simply put on the floor. 0 = none found */
int world_free_tile_near(float tx, float ty, int *ox, int *oy) {
    int cx = (int)floorf(tx), cy = (int)floorf(ty);
    for (int r = 0; r <= 6; r++)
        for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) {
            if (r > 0 && abs(i) != r && abs(j) != r) continue;
            if (solid_tile(cx + i, cy + j)) continue;
            *ox = cx + i; *oy = cy + j; return 1;
        }
    return 0;
}
/* it falls on the nearest free floor tile around (tx, ty): first the tile itself, then rings outward. `n` tries the next one each call so parts do not pile on one tile */
void world_drop_item_near(int kind, float tx, float ty) {
    static int turn; int cx = (int)floorf(tx), cy = (int)floorf(ty), skip = turn++ % 4;
    for (int r = 0; r <= 4; r++)
        for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) {
            if (r > 0 && abs(i) != r && abs(j) != r) continue;
            if (solid_tile(cx + i, cy + j)) continue;
            if (r == 0 || skip-- <= 0) { world_drop_item(kind, cx + i, cy + j); return; }
        }
    world_drop_item(kind, cx, cy);
}
unsigned world_body_in_reach(void) {
    if (cur_map != MAP_TEMPLE) return 0;
    float gap = INSPECT_REACH, g1 = body_gap(), gl = 1e9f; int bi = bodies_nearest(pxp, pyp, px, tile, INSPECT_REACH, &gl);
    if (g1 <= gap && g1 <= gl) return 1;
    return bi >= 0 ? bodies_get(bi)->vessel : 0;
}
/* story chunk 12: the big rock under the club. in reach when it is as near as a body is for a stab; breaking it leaves the rock gone and says where it lay */
int world_rock_in_reach(void) {
    if (cur_map != MAP_TEMPLE || world_dragging_rock()) return 0;
    return props_rock_gap(pxp, pyp, px, tile) <= INSPECT_REACH;
}
int world_rock_break(float *tx, float *ty) { return props_rock_break(tx, ty); }
/* story chunk 8: THROW. the thing leaves our hand and lands at random on a tile in the 3x3 round us (like minecraft: it can be our own tile): never in a wall. if that tile is
 * the crack it goes in and is gone. returns 1 when it fell into the pit, 0 when it landed on the floor (a drop that lands with a thunk, and can be taken again) */
int world_throw_item(int kind) {
    static unsigned long long seed;
    int x0 = (int)floorf(pxp / tile), y0 = (int)floorf(pyp / tile), cx[9], cy[9], n = 0;
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
        int nx = x0 + i, ny = y0 + j;
        if (nx < 0 || ny < 0 || nx >= mw || ny >= mh || solid_tile(nx, ny)) continue;
        cx[n] = nx; cy[n] = ny; n++;
    }
    seed = seed * 6364136223846793005ULL + (unsigned long long)SDL_GetTicks() + 1442695040888963407ULL;
    int k = n ? (int)((seed >> 33) % (unsigned)n) : -1;
    int tx = k >= 0 ? cx[k] : x0, ty = k >= 0 ? cy[k] : y0;
    if (cur_map == MAP_TEMPLE && map[ty][tx] == T_RPIT) return 1;
    world_drop_item(kind, tx, ty);
    return 0;
}
int world_pit_spots(int *far_x, int *far_y, int *edge_x, int *edge_y) {        /* the far ledge's spot and the near ledge's middle (tiles); 0 = no pit */
    if (cur_map != MAP_TEMPLE || pit_x0 < 0) return 0;
    if (far_x) *far_x = far_tx;
    if (far_y) *far_y = far_ty;
    if (edge_x) *edge_x = edge_tx;
    if (edge_y) *edge_y = edge_ty;
    return 1;
}

void world_set_inspect_cb(void (*cb)(unsigned vessel)) { inspect_cb = cb; }
void world_place_item(int kind, int tile_x, int tile_y) { item_place(kind, tile_x, tile_y); }
int  world_has_item(int kind) { return item_has(kind); }
void world_give_item(int kind) { item_give(kind); }
void world_take_item(int kind) { item_take(kind); }
void world_clear_items(void) { item_clear(); }
void world_drop_item(int kind, int tile_x, int tile_y) { item_drop(kind, tile_x, tile_y); }

/* a new life (story.c, after a death): the ruins again (the same seed gives the same map), the player at the spawn, the fades undone; the controls
 * stay hidden (the blood circle gives them back). t restarts so the world fades in from black */
void world_reload_ruins(void) {
    load_map();
    blackout_t = -1; blackout_cb = NULL;
    toast_init(W, H);                                         /* no half-shown "New task" panel from the last life */
    aoi_z = 1.0f; moving = 0; walk = 0; facing = FACE_DOWN;
    t = 0;
    world_set_controls_visible(0);
}

void world_set_player(const Person *who) { player_look = who ? who : &VESSEL; }
const Person *world_player(void) { return player_look; }

void world_fade_to_black(float seconds, void (*on_black)(void)) {
    blackout_t = 0; blackout_dur = seconds > 0.05f ? seconds : 0.05f; blackout_cb = on_black;
}

static int button_hit(int x, int y) {
    float dx = (float)(x - bcx), dy = (float)(y - bcy);
    return sqrtf(dx * dx + dy * dy) <= br * 1.3f;                /* generous hit area */
}

static int hand_hit(int x, int y) {
    float dx = (float)(x - hbx), dy = (float)(y - hby);
    return sqrtf(dx * dx + dy * dy) <= hbr * 1.3f;
}

void world_touch(int a, int x, int y) {
    if (a == 3) {                                       /* a cancel (paused, the app left): EVERY finger lets go of what it holds */
        const int ps[4] = { stick_pid, btn_pid, hand_pid, mm_pid };
        for (int i = 0; i < 4; i++) world_touch_p(ps[i], 3, x, y);
    }
    world_touch_p(0, a, x, y);
}

/* one finger of possibly two (a thumb on the stick and one on the interact button). every control remembers WHICH finger holds it and only
 * listens to that one; a finger that goes down takes the control it lands on if that is free */
void world_touch_p(int pid, int a, int x, int y) {
    if (dialog_active()) {                              /* dialogs eat all touches */
        if (pid) return;
        dialog_touch(a, x, y);
        stick_on = 0; kx = ky = 0; btn_down = 0;
        return;
    }
    if (!controls_visible) return;
    if (pid == 0 && item_ui_touch(a, x, y)) return;                 /* the inventory under the minimap: tap to unfold / fold */
    if (item_ui_open() && a == 0) return;                          /* chunk 16: the window is modal: no other finger starts a control */
    if (a == 0 && !mm_down && minimap_hit(x, y)) { mm_down = 1; mm_pid = pid; return; }          /* the minimap: tap it for the map window */
    if (mm_down && pid == mm_pid) {
        if (a == 1) { mm_down = 0; if (minimap_hit(x, y)) { stick_on = 0; kx = ky = 0; btn_down = btn_inside = 0; mapwin_open(); } }
        else if (a == 3) mm_down = 0;
        return;
    }

    /* chunk 68: the hand button: press on it, release on it = take what lies in reach (a pick-up is a tap, not a tool use) */
    if (a == 0 && !hand_down && (near_item || near_inspect) && hand_hit(x, y)) { hand_down = 1; hand_inside = 1; hand_pid = pid; return; }
    if (hand_down && pid == hand_pid) {
        if (a == 2) hand_inside = hand_hit(x, y);
        if (a == 1 || a == 3) { int fire = a == 1 && hand_inside && (near_item || near_inspect); hand_down = hand_inside = 0; if (fire) { if (near_item) item_grab(pxp, pyp); else if (inspect_cb) inspect_cb(inspect_vessel); } }
        return;
    }

    /* interact button: press on it, release on it = take hold of the heavy thing in reach / let go of it */
    if (a == 0 && !btn_down && near_drag && button_hit(x, y)) { btn_down = 1; btn_inside = 1; btn_pid = pid; return; }
    if (btn_down && pid == btn_pid) {
        if (a == 2) btn_inside = button_hit(x, y);
        if (a == 1 || a == 3) {
            int fire = a == 1 && btn_inside && near_drag;
            btn_down = btn_inside = 0;
            if (fire) drag_toggle();                                                      /* take hold of a body / the rock, or let go */
        }
        return;
    }

    if (a == 0 && (!stick_on || pid == stick_pid)) {
        float dx = (float)(x - sx), dy = (float)(y - sy);
        stick_on = sqrtf(dx * dx + dy * dy) <= sr * 1.5f;      /* generous grab zone */
        stick_pid = pid;
    }
    if (!stick_on || pid != stick_pid) return;                 /* not the finger on the stick */
    if (a == 1 || a == 3) { stick_on = 0; kx = ky = 0; return; }
    if (a == 0 || a == 2) {
        float dx = (float)(x - sx) / sr, dy = (float)(y - sy) / sr;
        float m = sqrtf(dx * dx + dy * dy);
        if (m > 1.0f) { dx /= m; dy /= m; }
        kx = dx; ky = dy;
    }
}

static int blocked(float fx, float fy) {           /* feet box centred at fx,fy */
    if (body_hits(fx, fy)) return 1;                  /* a body lies there: not a place to stand */
    float hw = 3.0f * px, hh = 1.5f * px;
    float xs[2] = { fx - hw, fx + hw - 1 }, ys[2] = { fy - 2 * hh, fy - 1 };
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            if (solid_tile((int)floorf(xs[i] / tile), (int)floorf(ys[j] / tile))) return 1;
    return 0;
}

/* the camera along one axis: follows the player but never shows past the map's edge; a map smaller than the screen (the room) is centred */
static float cam_axis(float c, int map_px, int screen) {
    float m = (float)(map_px - screen);
    if (m < 0) return m / 2.0f;
    if (c > m) c = m;
    if (c < 0) c = 0;
    return c;
}

/* the lazy camera, the pixel-game standard: a DEAD ZONE (the player walks about inside a box around the view's centre and the camera does not
 * move at all) plus SMOOTHING (once he leaves the box the camera eases after him, it does not snap). CAM_DEAD: half the box as a share of the view,
 * 0.125 = a box a quarter of the screen wide and tall. CAM_SMOOTH: ease rate per second, 6 = the classic lerp 0.1 per frame at 60 fps. */
#define CAM_DEAD   0.125f
#define CAM_SMOOTH 6.0f
static void camera_update(float dt) {
    reveal();
    float dx = pxp - cam_cx, dy = pyp - cam_cy;
    if (cam_snap || fabsf(dx) > W || fabsf(dy) > H) {           /* a new map / a door / a far jump: no easing across the world */
        cam_cx = pxp; cam_cy = pyp; cam_snap = 0;
    } else {
        float dzx = W * CAM_DEAD, dzy = H * CAM_DEAD;
        float gx = dx > dzx ? dx - dzx : dx < -dzx ? dx + dzx : 0.0f;       /* how far he is outside the box, per axis */
        float gy = dy > dzy ? dy - dzy : dy < -dzy ? dy + dzy : 0.0f;
        float k = 1.0f - expf(-CAM_SMOOTH * dt);
        cam_cx += gx * k; cam_cy += gy * k;
    }
    cam_x = cam_axis(cam_cx - W / 2.0f, mw * tile, W);           /* never past the map's edge; a small map (the room) stays centred */
    cam_y = cam_axis(cam_cy - H / 2.0f, mh * tile, H);
    cam_cx = cam_x + W / 2.0f; cam_cy = cam_y + H / 2.0f;        /* the clamped place is the state, so the camera never winds up behind an edge */
}

static void rock_place(void) {                                  /* the big rock lies on the south ledge, in front of the pit, to one side of the way through: three tiles of floor under it */
    static const int off[8] = { 6, -6, 5, -5, 7, -7, 4, -4 };
    int ty = edge_ty + 1;
    for (int k = 0; k < 8; k++) {
        int tx = edge_tx + off[k];
        if (!solid_tile(tx - 1, ty) && !solid_tile(tx, ty) && !solid_tile(tx + 1, ty) && !solid_tile(tx, ty + 1)) { props_rock_place(tx + 0.5f, ty + 0.9f); return; }
    }
    props_rock_place(edge_tx + 6.5f, ty + 0.9f);               /* (cannot happen on the ruins: the ledge is wide) */
}

void world_update(float dt) {
    if (item_ui_open()) return;                               /* story chunk 16: the inventory window is open: the world waits, as when paused */
    dragged_now = 0;
    if (mapwin_active() && (dialog_active() || !controls_visible)) mapwin_close();   /* a talk or a cutscene starts: the window must not be in the way */
    t += dt;
    if (ritual_take_chime()) sfx_summon();                                                                     /* the circle begins to charge */
    if (ritual_update(dt)) { void (*cb)(void) = ritual_cb; ritual_cb = NULL; world_set_controls_visible(1); if (cb) cb(); }   /* the new body has formed: the controls are back */
    if (blackout_t >= 0 && blackout_t < blackout_dur) {       /* fading to black (chunk 12): on_black runs on the frame it is dark */
        blackout_t += dt;
        if (blackout_t >= blackout_dur) { void (*cb)(void) = blackout_cb; blackout_cb = NULL; if (cb) cb(); }
    }
    story_update(dt);
    dialog_update(dt);
    missions_update(dt);
    toast_update(dt);
    hud_update(dt);

    if (drag_on && (dialog_active() || !controls_visible)) drag_on = 0;   /* he is let go of when anything takes our hands */
    if (dialog_active()) { kx = ky = 0; stick_on = 0; }                  /* frozen while talking */
    float mag = sqrtf(kx * kx + ky * ky);
    moving = mag > 0.15f;
    if (moving) {
        float speed = WALK_TILES_PER_SEC * tile * (mag > 1 ? 1 : mag) * (drag_on ? (drag_id == DRAG_ROCK ? ROCK_SPEED : DRAG_SPEED) : 1.0f) * (item_medium_held() ? CARRY_SPEED_MEDIUM : 1.0f);
        float vx = kx / mag * speed * dt, vy = ky / mag * speed * dt;
        if (drag_on) {                                                      /* it trails behind us on a rope: we cannot outrun it (drag_step) */
            if (!blocked(pxp + vx, pyp) && !drag_taut(vx, 0)) pxp += vx;
            if (!blocked(pxp, pyp + vy) && !drag_taut(0, vy)) pyp += vy;
        } else {
            if (!blocked(pxp + vx, pyp)) pxp += vx;
            if (!blocked(pxp, pyp + vy)) pyp += vy;
        }
        if (fabsf(kx) > fabsf(ky)) facing = kx < 0 ? FACE_LEFT : FACE_RIGHT;
        else                       facing = ky < 0 ? FACE_UP : FACE_DOWN;
        walk += dt * 8.0f * (drag_on ? (drag_id == DRAG_ROCK ? 0.4f : 0.55f) : 1.0f);
    } else walk = 0;
    drag_step(dt);
    dragged_now = drag_spd > 0.8f;

    item_update(dt);
    bodies_trail_update(dt);
    if (swing_on) {                                                      /* chunk 11: flying across the crack */
        swing_t += dt / 1.5f;
        float k = swing_t > 1.0f ? 1.0f : swing_t, e = k * k * (3.0f - 2.0f * k);
        pxp = swing_sx + (swing_ex - swing_sx) * e; pyp = swing_sy + (swing_ey - swing_sy) * e;
        moving = 1;
        if (swing_t >= 1.0f) { swing_on = 0; moving = 0; world_set_controls_visible(1); if (swing_done) { void (*d)(void) = swing_done; swing_done = NULL; d(); } }
    }
    {   int push = 0;                                                    /* chunk 11: walking into the crack for a moment is a fall */
        if (cur_map == MAP_TEMPLE && !swing_on && controls_visible && (fabsf(kx) > 0.3f || fabsf(ky) > 0.3f)) {
            float dx = kx > 0.3f ? 1.0f : kx < -0.3f ? -1.0f : 0.0f, dy = ky > 0.3f ? 1.0f : ky < -0.3f ? -1.0f : 0.0f;
            push = pit_probe(pxp + dx * 0.6f * tile, pyp + dy * 0.6f * tile);
        }
        if (push) pit_push_t += dt; else pit_push_t = 0;
    }
    if (cur_map == MAP_TEMPLE && edge_tx >= 0 && !props_rock_placed()) rock_place();
    near_item = !dialog_active() && item_in_reach(pxp, pyp);      /* chunk 68: the hand shows near an item; nothing is taken by walking */
    near_inspect = 0;                                            /* story chunk 6: a body in reach (vessel 1 is ritual.c's, the rest are bodies.c's): the hand button turns into a magnifying glass */
    if (!near_item && !dialog_active() && cur_map == MAP_TEMPLE) {
        float gap = INSPECT_REACH, g1 = body_gap(), gl = 1e9f; int bi = bodies_nearest(pxp, pyp, px, tile, INSPECT_REACH, &gl);
        if (g1 <= gap && g1 <= gl) { near_inspect = 1; inspect_vessel = 1; }
        else if (bi >= 0) { near_inspect = 1; inspect_vessel = bodies_get(bi)->vessel; }
    }
    if (!near_item && !near_inspect) hand_down = hand_inside = 0;
    drag_pick();
    near_drag = !dialog_active() && (drag_on || drag_cand >= 0);   /* a body or the rock lies in reach (or we hold one) */
    if (!near_drag) btn_down = btn_inside = 0;
    {   float lv = drag_on ? fminf(drag_spd / 28.0f, 1.0f) : 0.0f; if (lv < 0.04f) lv = 0.0f;               /* the scrape: as loud as he is fast */
        if (lv > 0 || drag_snd > 0) sfx_drag(lv);
        drag_snd = lv;
        strain_t = drag_on && dragged_now ? DRAG_STRAIN : (strain_t > dt ? strain_t - dt : 0.0f);
    }

    camera_update(dt);

    {   /* the area of interest: near the pit the view pulls back to take in the far ledge too, and eases back when we go away */
        float tz = 1.0f, tfx = cam_cx, tfy = cam_cy;
        if (!gfx_is_gpu() && cur_map == MAP_TEMPLE && pit_x0 >= 0 && far_tx >= 0) {
            float k = (AOI_FAR - world_dist_to_pit()) / (AOI_FAR - AOI_NEAR);
            k = k < 0 ? 0 : k > 1 ? 1 : k; k = k * k * (3.0f - 2.0f * k);
            float fpx = (far_tx + 0.5f) * tile, fpy = (far_ty + 0.5f) * tile;
            float hw = fabsf(pxp - fpx) * 0.5f + AOI_PAD * tile, hh = fabsf(pyp - fpy) * 0.5f + AOI_PAD * tile;   /* half the box that holds us, the far ledge and some air */
            float fit = fminf((float)W / (2.0f * hw), (float)H / (2.0f * hh));
            fit = fit > 1.0f ? 1.0f : fit < AOI_MIN ? AOI_MIN : fit;
            tz = 1.0f + (fit - 1.0f) * k;
            tfx = cam_cx + ((pxp + fpx) * 0.5f - cam_cx) * k;                 /* the view's middle slides from where the camera is to between us and the ledge */
            tfy = cam_cy + ((pyp + fpy) * 0.5f - cam_cy) * k;
        }
        if (aoi_z >= 0.999f) { aoi_fx = cam_cx; aoi_fy = cam_cy; }            /* not pulled back: the middle is just the camera's, so the pull starts without a jump */
        float ea = 1.0f - expf(-AOI_SPEED * dt);
        aoi_z += (tz - aoi_z) * ea; aoi_fx += (tfx - aoi_fx) * ea; aoi_fy += (tfy - aoi_fy) * ea;
        if (tz >= 1.0f && aoi_z > 0.998f) aoi_z = 1.0f;
    }
}

/* round button with the face of whoever you are next to (or an arrow down into the hole). tap it. */
/* chunk 54: the analog stick, readable on anything (white clouds, the night, grass, limbo's black). two tones, so one of them always
 * contrasts: a dark pixel outline ring round a light ring, a dark half-clear backing disc under the whole stick, and a light knob with a
 * dark rim. (kcx, kcy) is the knob's centre, `on` = the finger is on it. */
#define UI_DARK 10, 12, 24
static void draw_stick(SDL_Renderer *r, int cx, int cy, int rad, int krad, int kcx, int kcy, int on) {
    col(r, UI_DARK, 120);  pdisc(r, cx, cy, rad + ucell, ucell);                   /* the backing, under the outline too */
    col(r, UI_DARK, 235);  pring(r, cx, cy, rad + ucell, 1, ucell);                /* the dark outline, just outside the light ring */
    col(r, 255, 255, 255, 235); pring(r, cx, cy, rad, 1, ucell);                   /* the light ring */
    col(r, UI_DARK, 235);  pdisc(r, kcx, kcy, krad + ucell, ucell);                /* the knob's dark rim ... */
    col(r, 255, 255, 255, on ? 255 : 220); pdisc(r, kcx, kcy, krad, ucell);        /* ... round the light knob */
}

static void draw_hand_button(SDL_Renderer *r) {                  /* chunk 68: the same two-tone edge as the interact button, a little smaller; an open hand */
    int down = hand_down && hand_inside;
    int pulse = 170 + (int)(70.0f * sinf(t * 5.0f));
    col(r, UI_DARK, 235); pring(r, hbx, hby, hbr + ucell, 1, ucell);
    if (down) col(r, 70, 80, 130, 240); else col(r, 24, 28, 48, 235);
    pdisc(r, hbx, hby, hbr, ucell);
    col(r, 255, 255, 255, down ? 255 : pulse);
    pring(r, hbx, hby, hbr, 1, ucell);
    int a = hbr / 7; if (a < 2) a = 2;
    int bob = down ? a : (int)(sinf(t * 3.0f) * a * 0.4f);
    int side = ITEM_ICON_CELLS * a;
    if (near_item) item_draw_hand_icon(r, hbx - side / 2, hby - side / 2 + bob, a);
    else           item_draw_search_icon(r, hbx - side / 2, hby - side / 2 + bob, a);        /* story chunk 6: a body lies in reach */
}

static void draw_interact_button(SDL_Renderer *r) {
    int down = btn_down && btn_inside;
    int pulse = 170 + (int)(70.0f * sinf(t * 5.0f));              /* ring breathes to draw the eye */

    col(r, UI_DARK, 235); pring(r, bcx, bcy, br + ucell, 1, ucell);   /* chunk 54: a dark outline outside the light ring (the button's body is already dark) */
    if (down) col(r, 70, 80, 130, 240); else col(r, 24, 28, 48, 235);
    pdisc(r, bcx, bcy, br, ucell);
    col(r, 255, 255, 255, down ? 255 : pulse);
    pring(r, bcx, bcy, br, 1, ucell);

    if (near_drag) {                                             /* vessel 1 lying down and two hands: they hover over him, and shut on him (he shudders) while we hold him */
        int a = br / 9; if (a < 2) a = 2;
        int jit = drag_on && dragged_now ? (int)(sinf(t * 30.0f) * a * 0.4f) : 0;
        int hv = drag_on ? 0 : 2 * a + (int)(sinf(t * 4.0f) * a * 0.5f);
        if ((drag_on ? drag_id : drag_cand) == DRAG_ROCK) {                                    /* the big rock: a grey lump (the hands are the same) */
            col(r, 150, 146, 164, 255); fill(r, bcx - 5 * a + jit, bcy,         4 * a, a);
            col(r, 120, 116, 134, 255); fill(r, bcx - 6 * a + jit, bcy + a,     12 * a, 2 * a);
            col(r, 84, 80, 98, 255);    fill(r, bcx - 5 * a + jit, bcy + 3 * a, 10 * a, a);
        } else {
        col(r, 214, 218, 232, 255); fill(r, bcx - 5 * a + jit, bcy + a, 10 * a, 2 * a);        /* his body ... */
        col(r, 60, 66, 100, 255);   fill(r, bcx + 5 * a + jit, bcy + a, 2 * a, 2 * a);        /* ... his legs ... */
        col(r, 240, 196, 160, 255); fill(r, bcx - 7 * a + jit, bcy, 3 * a, 3 * a);            /* ... and his head, to the left */
        }
        col(r, 255, 255, 255, 255);
        fill(r, bcx - 3 * a, bcy - a - hv, 2 * a, 2 * a);                                      /* the hands */
        fill(r, bcx + a,     bcy - a - hv, 2 * a, 2 * a);
        if (drag_on && dragged_now) { col(r, 255, 255, 255, 160); fill(r, bcx + 8 * a, bcy + 2 * a, 2 * a, a / 2 + 1); fill(r, bcx + 9 * a, bcy + a, a, a / 2 + 1); }   /* the dust behind him */
        return;
    }
}

/* everything that lives on the map (tiles, bodies, characters) seen from a camera at (camx, camy) */
static void draw_scene(SDL_Renderer *r, float camx, float camy) {
    int cx = (int)camx, cy = (int)camy;
    int tx0 = cx / tile, ty0 = cy / tile;
    if (cx < 0) tx0 = (cx - tile + 1) / tile;
    if (cy < 0) ty0 = (cy - tile + 1) / tile;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    int tx1 = (cx + W) / tile, ty1 = (cy + H) / tile;

    col(r, 0, 0, 0, 255); fill(r, 0, 0, W, H);                  /* the map can be smaller than the screen: black round it */
    for (int ty = ty0; ty <= ty1 && ty < mh; ty++)
        for (int tx = tx0; tx <= tx1 && tx < mw; tx++)
            draw_ruin_tile(r, map[ty][tx], tx, ty, tx * tile - cx, ty * tile - cy);

    float rit_tx = 0, rit_ty = 0; ritual_centre(&rit_tx, &rit_ty);
    bodies_trail_draw(r, cx, cy, px, tile, TEMPLE_SHADE);                                                                      /* the blood trails, under everything else on the floor (story chunk 7) */
    if (ritual_scene()) {                                                                                                      /* the carved circle, the blood, the dead vessel (the first circle) */
        float hx = 0, hy = 0; ritual_home(&hx, &hy);
        int hsx = (int)(hx * tile) - cx, hsy = (int)(hy * tile) - cy;
        if (hsx > -40 * px && hsx < W + 40 * px && hsy > -30 * px && hsy < H + 30 * px) ritual_draw_floor(r, hsx, hsy, px, TEMPLE_SHADE);
    }
    if (ritual_c2_on()) {                                                                                                      /* the second circle on the far ledge (story chunk 13) */
        float qx = 0, qy = 0; ritual_c2_centre(&qx, &qy);
        int qsx = (int)(qx * tile) - cx, qsy = (int)(qy * tile) - cy;
        if (qsx > -40 * px && qsx < W + 40 * px && qsy > -30 * px && qsy < H + 30 * px) ritual_c2_draw(r, qsx, qsy, px, TEMPLE_SHADE);
    }
    bodies_draw(r, cx, cy, px, tile, TEMPLE_SHADE);                                                                            /* the other dead (story chunk 6) */
    props_draw(r, cx, cy, px, tile, TEMPLE_SHADE);                                                                             /* the big rock (story chunk 7) */
    item_draw_ground(r, cx, cy, t);                                                                                            /* lying on the floor: under the player */

    int sx0 = (int)pxp - cx, sy0 = (int)pyp - cy;
    char_set_darken(TEMPLE_SHADE);                                /* the ruins are shut in: a shadow over the body (off again after it is drawn) */
    if (ritual_active()) {                                        /* the vessel is drawn up out of the blood, from the feet up (ritual_draw_over puts the blood pixels on it) */
        int rows = ritual_player_rows();
        if (rows > 0) {
            SDL_Rect clip = { 0, sy0 - rows * px, W, rows * px + 4 * px };
            SDL_RenderSetClipRect(r, &clip);
            char_draw(r, player_look, sx0, sy0, FACE_DOWN, 0, 0.0f, px);
            SDL_RenderSetClipRect(r, NULL);
        }
    } else {
        float gx0, gy0, gx1, gy1;
        if (drag_on && drag_box_px(&gx0, &gy0, &gx1, &gy1)) {      /* holding it: the hands float to the nearest side */
            float cyy = (float)sy0 - 4.0f * px, nx = (float)sx0, ny = cyy;
            gx0 -= cx; gx1 -= cx; gy0 -= cy; gy1 -= cy;
            nx = nx < gx0 ? gx0 : nx > gx1 ? gx1 : nx; ny = ny < gy0 ? gy0 : ny > gy1 ? gy1 : ny;
            char_set_grip(1, nx, ny);
        }
        if (strain_t > 0) char_set_strain(1);                      /* pulling is hard: the eyes shut */
        char_draw(r, player_look, sx0, sy0, facing, moving, walk, px);
        char_set_grip(0, 0, 0); char_set_strain(0);
    }
    char_set_darken(0);
    if (ritual_scene()) ritual_draw_over(r, (int)(rit_tx * tile) - cx, (int)(rit_ty * tile) - cy, px, player_look);
}

void world_draw(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    if (aoi_z < 0.995f && !gfx_is_gpu()) {
        int Wv = (int)(W / aoi_z + 0.5f), Hv = (int)(H / aoi_z + 0.5f);          /* the view in world px: bigger than the screen */
        if (aren != r || !atex) {
            if (atex) SDL_DestroyTexture(atex);
            atex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, (int)(W / AOI_MIN) + 2, (int)(H / AOI_MIN) + 2); aren = r;
#if SDL_VERSION_ATLEAST(2, 0, 12)
            if (atex) SDL_SetTextureScaleMode(atex, SDL_ScaleModeLinear);        /* shrunk by a changing factor: linear keeps the pixel art from shimmering */
#endif
        }
        if (atex) {
            float ccx = cam_axis(aoi_fx - Wv / 2.0f, mw * tile, Wv), ccy = cam_axis(aoi_fy - Hv / 2.0f, mh * tile, Hv);
            int sw = W, sh = H;
            W = Wv; H = Hv;                                                      /* the scene is laid out for the bigger view ... */
            SDL_SetRenderTarget(r, atex);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            draw_scene(r, ccx, ccy);
            SDL_SetRenderTarget(r, NULL);
            W = sw; H = sh;                                                      /* ... and the screen is the screen again */
            SDL_Rect src = { 0, 0, Wv, Hv };
            SDL_RenderCopy(r, atex, &src, NULL);
        } else draw_scene(r, cam_x, cam_y);
    } else {
        draw_scene(r, cam_x, cam_y);
    }

    if (controls_visible && !dialog_active()) {
        float reach = sr - kr * 0.3f;                                     /* the knob hops from cell to cell */
        int kcx = sx + (int)floorf(kx * reach / ucell + (kx < 0 ? -0.5f : 0.5f)) * ucell, kcy = sy + (int)floorf(ky * reach / ucell + (ky < 0 ? -0.5f : 0.5f)) * ucell;
        draw_stick(r, sx, sy, sr, kr, kcx, kcy, stick_on);
        if (near_drag) draw_interact_button(r);
        if (near_item || near_inspect) draw_hand_button(r);                                              /* chunk 68: the hand, when something lies in reach */
    }

    if (controls_visible) {                                       /* minimap: top-right, under the ID card */
        MiniMark marks[MAX_MARKS]; int nm = gather_marks(marks);
        minimap_draw(r, map_for_display(), MAP_MAX, mw, mh, MAP_PAL, MAP_NPAL, pxp / tile, pyp / tile, facing, marks, nm, t);
        item_ui_draw(r, t);                                       /* the inventory, right under it */
    }
    {   /* slide under the music button / card AND the brain button / card, whichever reaches lower: the
         * "New task added" toast hangs under the list, so it is pushed down with it and a thought sits clean */
        int o1 = nowplaying_offset(), o2 = thought_offset();
        missions_set_offset(o1 > o2 ? o1 : o2);
    }
    missions_draw(r);
    toast_draw(r, (int)(10 * (W < H ? W : H) / 360.0f), missions_bottom() + (int)(4 * (W < H ? W : H) / 360.0f));   /* "New task added", under the list */
    hud_draw(r);                                                  /* ID card, top-right */
    if (controls_visible) item_ui_draw_window(r);                 /* story chunk 16: the inventory window, over the HUD */

    if (t < FADE_IN_T) {                                          /* fade in from black */
        col(r, 0, 0, 0, (int)(255 * (1.0f - t / FADE_IN_T)));
        fill(r, 0, 0, W, H);
    }
    if (blackout_t >= 0) {                                        /* fade to black over everything, HUD included */
        float a = blackout_t / blackout_dur; if (a > 1) a = 1;
        col(r, 0, 0, 0, (int)(255 * a));
        fill(r, 0, 0, W, H);
    }
    dialog_draw(r);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
