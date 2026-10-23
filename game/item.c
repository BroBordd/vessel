/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "item.h"
#include "font.h"
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* 12 x 12 icons. G outline, L light steel, D dark steel, W light wood, w dark wood, V violet, v dark violet, Y gold, y pale gold glow */
static const char *HAMMER[ITEM_ICON_CELLS] = {
    "..GGGGGGGG..",
    ".GLLLLLLLLG.",
    ".GLLLLLLLDG.",
    ".GDDDDDDDDG.",
    "..GGGWWGGG..",
    "....GWWG....",
    "....GWwG....",
    "....GWwG....",
    "....GWwG....",
    "....GWwG....",
    "....GWwG....",
    ".....GG.....",
};
/* the orb (chunk 39): a violet ball with a gold glow in the middle, like draw_orb_ball in brainwin.c */
static const char *ORB[ITEM_ICON_CELLS] = {
    "....GGGG....",
    "..GGvVVvGG..",
    ".GvVVVVVVvG.",
    ".GVVYyyYVVG.",
    "GvVYyyyyYVvG",
    "GVVYyyyyYVVG",
    "GVVVYyyYVVVG",
    "GvVVVYYVVVvG",
    ".GVVVVVVVVG.",
    ".GvvVVVVvvG.",
    "..GGvvvvGG..",
    "....GGGG....",
};
/* the magnifying glass (chunk 32). B glass, b glint on the glass. the rim is light steel (L), not the dark outline, so it reads on the dark button */
static const char *SEARCH[ITEM_ICON_CELLS] = {
    "...LLLLL....",
    "..LBBBBBL...",
    ".LBbbBBBBL..",
    ".LBbBBBBBL..",
    ".LBBBBBBBL..",
    ".LBBBBBBBL..",
    "..LBBBBBLL..",
    "...LLLLLWL..",
    ".......LLWL.",
    ".........LWL",
    "..........LL",
    "............",
};
static const char *HAND[ITEM_ICON_CELLS] = {
    "...G.G.G....",
    "..GSGSGSG.G.",
    "..GSGSGSGSG.",
    "..GSGSGSGSG.",
    ".GGSSSSSSSG.",
    "GSSSSSSSSSG.",
    "GSSSSSSSSSG.",
    ".GSSSSSSSSG.",
    "..GSSSSSSG..",
    "..GSSSSSsG..",
    "...GSSSssG..",
    "....GGGGG...",
};
/* chunk 70: the bag (the collapsed inventory button). an old leather pouch tied with a gold cord. not an item */
static const char *BAG[ITEM_ICON_CELLS] = {
    "....GYYG....",
    "....GyYG....",
    ".....GG.....",
    "...GWWWWG...",
    "..GWWWWWWG..",
    ".GWWWWWWWWG.",
    "GWWWWYYWWWwG",
    "GWWWWYyWWWwG",
    "GWWWWWWWWWwG",
    "GWWWWWWWWwwG",
    ".GwWWWWWwwG.",
    "..GGGGGGGG..",
};
static const char *const *ICONS[ITEM_COUNT] = { HAMMER, ORB };
static const char *NAMES[ITEM_COUNT] = { "HAMMER", "ORB" };

const char *item_name(int kind) { return kind >= 0 && kind < ITEM_COUNT ? NAMES[kind] : ""; }

static int s = 1, tile = 16;                /* pixel scale and tile size of the current map */
static int on, on_kind, tx, ty;             /* what lies on the map */
static int have[ITEM_COUNT];
static float new_t = 99.0f;                 /* seconds since the last pickup LANDED in the bag (the slot flashes, the badge ticks); negative while the icon is still flying */
static int   newest = -1;                   /* chunk 70: the kind that came last (gold frame) */
static float fly_t = 99.0f; static int fly_kind, fly_x, fly_y;      /* chunk 70: the icon flying from the ground to the bag */
static int   gs_x, gs_y, gs_seen;           /* where the item lying on the map was drawn last (screen px): the flight starts there */
#define INV_FLY_T   0.4f
#define INV_EASE_T  0.25f
#define INV_IDLE_T  4.0f
static int   ui_open, ui_down;              /* the panel is asked open / which part of the inventory the finger went down on (1 bag, 2 panel) */
static float ui_p, ui_idle;                 /* how far the panel is out (0..1), seconds since the last touch on it */
static int   ui_sel = -1;                   /* the slot whose name shows */
static float drop_t = 99.0f;                /* seconds since the item on the map was dropped (chunk 32); 99 = it was simply placed */
#define DROP_BOUNCE_T 0.2f
#define DROP_HEIGHT   14                    /* units it falls from */

static int locked[ITEM_COUNT], n_blocked;
static void (*blocked_cb)(void);
void item_set_locked(int kind, int lk) { if (kind >= 0 && kind < ITEM_COUNT) locked[kind] = lk ? 1 : 0; }
int  item_locked(int kind) { return kind >= 0 && kind < ITEM_COUNT && locked[kind]; }
void item_set_blocked_cb(void (*cb)(void)) { blocked_cb = cb; }
int  item_blocked_count(void) { return n_blocked; }
void item_reset(int pixel_scale, int tile_size) { s = pixel_scale; tile = tile_size; on = 0; drop_t = 99.0f; memset(locked, 0, sizeof locked); blocked_cb = NULL; }
void item_place(int kind, int tile_x, int tile_y) { if (kind < 0 || kind >= ITEM_COUNT) return; on = 1; on_kind = kind; tx = tile_x; ty = tile_y; drop_t = 99.0f; gs_seen = 0; }
void item_drop(int kind, int tile_x, int tile_y) { item_place(kind, tile_x, tile_y); if (on) drop_t = 0; }
int  item_dropping(void) { return on && drop_t < ITEM_DROP_T; }
int  item_exists(void) { return on; }
void item_tile(float *x, float *y) { *x = tx + 0.5f; *y = ty + 0.5f; }
int  item_has(int kind) { return kind >= 0 && kind < ITEM_COUNT && have[kind]; }
void item_give(int kind) { if (kind < 0 || kind >= ITEM_COUNT) return; have[kind] = 1; new_t = 0; newest = kind; }
void item_take(int kind) { if (kind >= 0 && kind < ITEM_COUNT) have[kind] = 0; }
void item_clear(void) { memset(have, 0, sizeof have); new_t = 99.0f; newest = -1; fly_t = 99.0f; ui_open = ui_down = 0; ui_p = 0; ui_sel = -1; }

static int owned(void);
void item_update(float dt) {
    if (new_t < 99.0f) new_t += dt;
    if (fly_t < INV_FLY_T) fly_t += dt;
    if (owned() == 0) { ui_open = ui_down = 0; ui_p = 0; }
    if (ui_open) { ui_idle += dt; if (ui_idle >= INV_IDLE_T) ui_open = 0; }        /* chunk 70: it never stays open by itself */
    if (ui_open && ui_p < 1.0f) { ui_p += dt / INV_EASE_T; if (ui_p > 1.0f) ui_p = 1.0f; }
    if (!ui_open && ui_p > 0.0f) { ui_p -= dt / INV_EASE_T; if (ui_p < 0.0f) ui_p = 0.0f; }
    if (on && drop_t < 99.0f) {                                 /* chunk 32: a dropped item falls first, and thunks when it lands */
        float before = drop_t; drop_t += dt;
        if (before < ITEM_DROP_T && drop_t >= ITEM_DROP_T) sfx_blip(0.42f);
        if (drop_t > ITEM_DROP_T + DROP_BOUNCE_T + 1.0f) drop_t = 99.0f;
    }
}
int item_in_reach(float pxw, float pyw) {
    if (!on || item_dropping()) return 0;
    float dx = pxw - (tx + 0.5f) * tile, dy = pyw - (ty + 0.5f) * tile;
    return sqrtf(dx * dx + dy * dy) / tile < ITEM_REACH_R;
}
int item_grab(float pxw, float pyw) {                           /* chunk 68: the tap on the hand */
    if (!item_in_reach(pxw, pyw)) return -1;
    int k = on_kind;
    if (locked[k]) {                                            /* chunk 33: it stays where it is, every press says so */
        n_blocked++; if (blocked_cb) blocked_cb();
        return -1;
    }
    on = 0; item_give(k);
    if (gs_seen) { fly_kind = k; fly_x = gs_x; fly_y = gs_y; fly_t = 0; new_t = -INV_FLY_T; }      /* chunk 70: it flies to the bag, the flash and the badge wait for it */
    return k;
}

/* ---------- drawing ---------- */
static void rgb_of(char c, int *R, int *G, int *B) {
    switch (c) {
        case 'G': *R = 36;  *G = 40;  *B = 58;  break;
        case 'L': *R = 206; *G = 214; *B = 232; break;
        case 'D': *R = 122; *G = 130; *B = 156; break;
        case 'W': *R = 196; *G = 136; *B = 74;  break;
        case 'B': *R = 150; *G = 200; *B = 236; break;
        case 'b': *R = 244; *G = 250; *B = 255; break;
        case 'V': *R = 150; *G = 96;  *B = 220; break;
        case 'v': *R = 88;  *G = 52;  *B = 150; break;
        case 'S': *R = 236; *G = 190; *B = 150; break;
        case 's': *R = 200; *G = 140; *B = 104; break;
        case 'Y': *R = 236; *G = 190; *B = 78;  break;
        case 'y': *R = 255; *G = 238; *B = 170; break;
        default:  *R = 132; *G = 84;  *B = 42;  break;      /* w */
    }
}

/* the picture, optionally turned on its side (the handle points right): used for the one lying on the grass */
static void draw_rows(SDL_Renderer *r, const char *const *rows, int x, int y, int cell, int lying) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int j = 0; j < ITEM_ICON_CELLS; j++)
        for (int i = 0; i < ITEM_ICON_CELLS; i++) {
            char c = rows[j][i];
            if (c == '.') continue;
            int R, G, B; rgb_of(c, &R, &G, &B);
            SDL_SetRenderDrawColor(r, R, G, B, 255);
            SDL_Rect q = { x + (lying ? j : i) * cell, y + (lying ? i : j) * cell, cell, cell };
            SDL_RenderFillRect(r, &q);
        }
}
/* chunk 69: the same picture at HALF the size with square cells: every 2 x 2 block of the 12 x 12 icon becomes one cell (the commonest colour in the block; a
 * block with nothing in it stays empty), so it is 6 x 6 cells at the same cell size, centred where the big one was */
static void draw_rows_half(SDL_Renderer *r, const char *const *rows, int x, int y, int cell, int lying) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int j = 0; j < ITEM_ICON_CELLS / 2; j++)
        for (int i = 0; i < ITEM_ICON_CELLS / 2; i++) {
            char c4[4] = { rows[2 * j][2 * i], rows[2 * j][2 * i + 1], rows[2 * j + 1][2 * i], rows[2 * j + 1][2 * i + 1] };
            char best = '.'; int bestn = 0;
            for (int a = 0; a < 4; a++) {
                if (c4[a] == '.') continue;
                int n = 0; for (int b = 0; b < 4; b++) if (c4[b] == c4[a]) n++;
                if (n > bestn) { bestn = n; best = c4[a]; }
            }
            if (best == '.') continue;
            int R, G, B; rgb_of(best, &R, &G, &B);
            SDL_SetRenderDrawColor(r, R, G, B, 255);
            SDL_Rect q = { x + (lying ? j : i) * cell, y + (lying ? i : j) * cell, cell, cell };
            SDL_RenderFillRect(r, &q);
        }
}
static void draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell, int lying) { draw_rows(r, ICONS[kind], x, y, cell, lying); }
void item_draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell) {
    if (kind < 0 || kind >= ITEM_COUNT || cell < 1) return;
    draw_icon(r, kind, x, y, cell, 0);
}
void item_draw_bag_icon(SDL_Renderer *r, int x, int y, int cell) {
    if (cell < 1) return;
    draw_rows(r, BAG, x, y, cell, 0);
}
void item_draw_hand_icon(SDL_Renderer *r, int x, int y, int cell) {
    if (cell < 1) return;
    draw_rows(r, HAND, x, y, cell, 0);
}
void item_draw_search_icon(SDL_Renderer *r, int x, int y, int cell) {
    if (cell < 1) return;
    draw_rows(r, SEARCH, x, y, cell, 0);
}

void item_draw_ground(SDL_Renderer *r, int cam_x, int cam_y, float t) {
    if (!on) return;
    int x = (int)((tx + 0.5f) * tile) - cam_x, y = (int)((ty + 0.5f) * tile) - cam_y;      /* centre of the spot */
    int bob = (int)floorf(sinf(t * 3.0f) * 0.9f + 0.5f);
    int lift = 0;                                                                           /* chunk 32: how high a dropped item still is (units) */
    if (drop_t < ITEM_DROP_T) { float k = drop_t / ITEM_DROP_T; lift = (int)(DROP_HEIGHT * (1.0f - k * k) + 0.5f); bob = 0; }
    else if (drop_t < ITEM_DROP_T + DROP_BOUNCE_T) { lift = (int)(3.0f * sinf((drop_t - ITEM_DROP_T) / DROP_BOUNCE_T * 3.14159f) + 0.5f); bob = 0; }
    gs_x = x; gs_y = y - 2 * s + bob * s - lift * s; gs_seen = 1;                           /* chunk 70: where the flight to the bag starts */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 70);
    SDL_Rect sh = { x - 7 * s / 2, y + 3 * s / 2, 7 * s, s }; SDL_RenderFillRect(r, &sh);   /* ground shadow (chunk 69: half) */
    draw_rows_half(r, ICONS[on_kind], x - 3 * s, y - 3 * s + bob * s - lift * s, s, 1);     /* chunk 69: half the size, centred on the tile */
    if (drop_t >= ITEM_DROP_T && drop_t < ITEM_DROP_T + 0.5f) {                             /* landed: a burst of glints */
        float k = (drop_t - ITEM_DROP_T) / 0.5f; int a = (int)(255 * (1.0f - k)), d = (int)(3 + 9 * k);
        SDL_SetRenderDrawColor(r, 255, 255, 255, a);
        for (int q = 0; q < 4; q++) {
            int ox = (q & 1 ? d : -d), oy = (q & 2 ? d : -d) / 2;
            SDL_Rect v = { x + ox * s, y - 4 * s + oy * s, s, s }; SDL_RenderFillRect(r, &v);
        }
    }
    float ph = fmodf(t * 0.8f, 1.0f);                                                       /* a glint, so it can be spotted on the grass */
    if (ph < 0.35f) {
        int a = (int)(255 * sinf(ph / 0.35f * 3.14159f));
        int gx = x - 4 * s, gy = y - 7 * s + bob * s;
        SDL_SetRenderDrawColor(r, 255, 255, 255, a);
        SDL_Rect v = { gx, gy - s, s, 3 * s }, h = { gx - s, gy, 3 * s, s };
        SDL_RenderFillRect(r, &v); SDL_RenderFillRect(r, &h);
    }
}

/* ---------- the inventory widget (chunk 70) ----------
 * closed: one small bag button under the minimap (nothing at all while the pockets are empty) with a badge that counts what is in them.
 * a tap on it, and the panel eases out under it (INV_EASE_T): framed square slots, the newest in gold, the name of the slot you touched last under the grid.
 * it folds back by itself after INV_IDLE_T without a touch, on a tap outside it, or on a tap on the bag. a new pick-up flies from the ground to the bag, then the
 * badge ticks and the slot flashes. pixel art only: square cells, no blend modes other than plain alpha. */
#define INV_COLS 3
static int W, H, ux, uy, uw, bt, bag, icell, pad, psl, pcell, qs;

void item_ui_resize(int w, int h, int right_edge, int top, int width) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    pad = (int)(4 * u); if (pad < 2) pad = 2;
    qs = (int)(2 * u); if (qs < 1) qs = 1;                                                  /* one step of the little ornaments */
    uw = width; ux = right_edge - width; uy = top;
    bag = width / 3; if (bag < 24) bag = 24;
    icell = (bag - 2 * bt - 2 * pad) / ITEM_ICON_CELLS; if (icell < 1) icell = 1;
    psl = (uw - 2 * bt - (INV_COLS + 1) * pad) / INV_COLS; if (psl < 24) psl = 24;
    pcell = (psl - 2 * bt - pad) / ITEM_ICON_CELLS; if (pcell < 1) pcell = 1;
}

void item_ui_init(int w, int h, int right_edge, int top, int width) {
    item_ui_resize(w, h, right_edge, top, width);
    ui_open = ui_down = 0; ui_p = ui_idle = 0; ui_sel = -1;
}

static int owned(void) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) n += have[k]; return n; }
static int shown(void) { return owned() - (fly_t < INV_FLY_T ? 1 : 0); }                 /* the badge does not count what is still in the air */
static int fcell(const char *name, int room) {              /* the biggest letter size (up to 3) that still fits `room` */
    int c = (int)(2.0f * (W < H ? W : H) / 360.0f); if (c < 1) c = 1; if (c > 3) c = 3;
    while (c > 1 && font_width(name, c) > room) c--;
    return c;
}
static int slots_n(void) { int n = owned(); int rows = (n + INV_COLS - 1) / INV_COLS; if (rows < 1) rows = 1; return rows * INV_COLS; }
static int label_h(void) { return 2 * qs + font_height(fcell("MMMMMM", uw - 2 * bt - 2 * pad)) + pad; }
static int panel_full_h(void) { int rows = slots_n() / INV_COLS; return 2 * bt + pad + rows * psl + (rows - 1) * pad + pad + label_h(); }
static int panel_h(void) {                                  /* how tall it is right now: eased out, in whole steps of `pad` */
    float k = 1.0f - ui_p, e = 1.0f - k * k * k; int full = panel_full_h();
    if (ui_p >= 1.0f) return full;
    int h = (int)(full * e) / pad * pad; return h > full ? full : h;
}
static void bag_rect(int *x, int *y, int *w, int *h) { *x = ux + uw - bag; *y = uy; *w = bag; *h = bag; }
static void panel_rect(int *x, int *y, int *w, int *h) { *x = ux; *y = uy + bag + pad; *w = uw; *h = panel_h(); }
static int in_rect(int x, int y, int rx, int ry, int rw, int rh) { return x >= rx && y >= ry && x < rx + rw && y < ry + rh; }
static int hit_bag(int x, int y) { int a, b, c, d; bag_rect(&a, &b, &c, &d); return in_rect(x, y, a, b, c, d); }
static int hit_panel(int x, int y) { if (ui_p <= 0.0f) return 0; int a, b, c, d; panel_rect(&a, &b, &c, &d); return in_rect(x, y, a, b, c, d); }
static int slot_x(int i) { return ux + bt + pad + (i % INV_COLS) * (psl + pad); }
static int slot_y(int i) { return uy + bag + pad + bt + pad + (i / INV_COLS) * (psl + pad); }
static int kind_at_slot(int i) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (have[k]) { if (n == i) return k; n++; } return -1; }
static int slot_of_kind(int kind) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (have[k]) { if (k == kind) return n; n++; } return -1; }

int item_ui_hit(int x, int y) { return owned() > 0 && (hit_bag(x, y) || hit_panel(x, y)); }
int item_ui_open(void) { return ui_open && owned() > 0; }
float item_ui_progress(void) { return ui_p; }
int item_ui_selected(void) { return ui_sel; }
int item_ui_flying(void) { return fly_t < INV_FLY_T; }
int item_ui_shown(void) { return shown(); }
int item_ui_slot_x(int kind) { int i = slot_of_kind(kind); return i < 0 ? -1 : slot_x(i) + psl / 2; }
int item_ui_slot_y(int kind) { int i = slot_of_kind(kind); return i < 0 ? -1 : slot_y(i) + psl / 2; }
int item_ui_bag_x(void) { return ux + uw - bag / 2; }
int item_ui_bag_y(void) { return uy + bag / 2; }

static void set_open(int o) {
    if (o && !ui_open) { ui_idle = 0; ui_sel = newest >= 0 && have[newest] ? newest : kind_at_slot(0); }
    ui_open = o;
}
int item_ui_touch(int a, int x, int y) {
    if (owned() == 0) { ui_open = ui_down = 0; ui_p = 0; return 0; }
    if (a == 0) {
        if (hit_bag(x, y)) { ui_down = 1; return 1; }
        if (ui_open && hit_panel(x, y)) { ui_down = 2; ui_idle = 0; return 1; }
        if (ui_open) set_open(0);                            /* a tap outside folds it (and goes on to whatever is there) */
        return 0;
    }
    if (!ui_down) return 0;
    if (a == 1) {
        if (ui_down == 1 && hit_bag(x, y)) set_open(!ui_open);
        else if (ui_down == 2 && hit_panel(x, y)) {
            ui_idle = 0;
            for (int i = 0; i < slots_n(); i++) {
                int sx = slot_x(i), sy = slot_y(i);
                if (in_rect(x, y, sx - pad / 2, sy - pad / 2, psl + pad, psl + pad) && kind_at_slot(i) >= 0) ui_sel = kind_at_slot(i);
            }
        }
        ui_down = 0; return 1;
    }
    if (a == 3) ui_down = 0;                                 /* cancelled */
    return 1;                                                /* a drag that began on the inventory stays its own */
}

static void box(SDL_Renderer *r, int x, int y, int w, int h, int br, int bg, int bb, int fr, int fg, int fb, int fa) {
    SDL_Rect o = { x, y, w, h }, in = { x + bt, y + bt, w - 2 * bt, h - 2 * bt };
    SDL_SetRenderDrawColor(r, br, bg, bb, 255); SDL_RenderFillRect(r, &o);
    SDL_SetRenderDrawColor(r, fr, fg, fb, fa); SDL_RenderFillRect(r, &in);
}
static void draw_flying(SDL_Renderer *r) {
    float k = fly_t / INV_FLY_T; if (k > 1.0f) k = 1.0f;
    int tx_ = item_ui_bag_x(), ty_ = item_ui_bag_y();
    for (int j = 3; j >= 0; j--) {                           /* a little gold trail behind it, then the icon itself (j = 0) */
        float kj = k - j * 0.07f; if (kj < 0) continue;
        float e = kj * kj * (3.0f - 2.0f * kj);
        int x = fly_x + (int)((tx_ - fly_x) * e), y = fly_y + (int)((ty_ - fly_y) * e) - (int)(4.0f * bag * kj * (1.0f - kj));
        if (j > 0) {
            SDL_SetRenderDrawColor(r, 255, 230, 140, 200 - 50 * j);
            SDL_Rect q = { x - s / 2, y - s / 2, s, s }; SDL_RenderFillRect(r, &q);
        } else {
            int cell = s + (int)((icell - s) * e + 0.5f); if (cell < 1) cell = 1;
            if (e < 0.5f) draw_rows_half(r, ICONS[fly_kind], x - 3 * cell, y - 3 * cell, cell, 1);      /* it still lies on its side, then it turns upright as it goes */
            else          draw_rows_half(r, ICONS[fly_kind], x - 3 * cell, y - 3 * cell, cell, 0);
        }
    }
}

void item_ui_draw(SDL_Renderer *r, float t) {
    int n = owned();
    if (n == 0) return;
    (void)t;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int flash = new_t >= 0.0f && new_t < 1.5f;
    int blink = flash && ((int)(new_t * 8.0f) & 1) == 0;

    /* the panel first (it comes out from under the bag) */
    if (ui_p > 0.0f) {
        int px_, py_, pw_, ph_; panel_rect(&px_, &py_, &pw_, &ph_);
        int full = panel_full_h(), rows = slots_n() / INV_COLS;
        SDL_Rect clip = { px_, py_, pw_, ph_ };
        SDL_RenderSetClipRect(r, &clip);
        box(r, px_, py_, pw_, full, 214, 170, 70, 6, 8, 20, 245);
        for (int i = 0; i < slots_n(); i++) {
            int sx = slot_x(i), sy = slot_y(i), k = kind_at_slot(i);
            if (k < 0) { box(r, sx, sy, psl, psl, 40, 46, 70, 10, 12, 26, 255); continue; }        /* an empty slot */
            int gold = k == newest, br = 122, bg = 130, bb = 156;
            if (gold) { br = 236; bg = 190; bb = 78; if (blink) { br = 255; bg = 250; bb = 220; } }
            else if (k == ui_sel) { br = 206; bg = 214; bb = 232; }
            box(r, sx, sy, psl, psl, br, bg, bb, 20, 24, 46, 255);
            if (gold && flash) { SDL_SetRenderDrawColor(r, 255, 238, 170, blink ? 70 : 0); SDL_Rect in = { sx + bt, sy + bt, psl - 2 * bt, psl - 2 * bt }; SDL_RenderFillRect(r, &in); }
            item_draw_icon(r, k, sx + (psl - ITEM_ICON_CELLS * pcell) / 2, sy + (psl - ITEM_ICON_CELLS * pcell) / 2, pcell);
        }
        if (ui_sel >= 0 && have[ui_sel]) {                    /* the name under the slot you touched: a little pointer, then the word */
            int i = slot_of_kind(ui_sel), cx = slot_x(i) + psl / 2;
            int gy = uy + bag + pad + bt + pad + rows * psl + (rows - 1) * pad;           /* the bottom of the grid */
            SDL_SetRenderDrawColor(r, 236, 190, 78, 255);
            SDL_Rect a0 = { cx - 2 * qs, gy + pad / 2, 4 * qs, qs }, a1 = { cx - qs, gy + pad / 2 + qs, 2 * qs, qs };
            SDL_RenderFillRect(r, &a0); SDL_RenderFillRect(r, &a1);
            const char *nm = item_name(ui_sel); int fc = fcell(nm, pw_ - 2 * bt - 2 * pad), tw = font_width(nm, fc);
            int x0 = cx - tw / 2, lo = px_ + bt + pad, hi = px_ + pw_ - bt - pad - tw; if (x0 > hi) x0 = hi; if (x0 < lo) x0 = lo;
            SDL_SetRenderDrawColor(r, 235, 240, 255, 255);
            font_draw(r, nm, x0, gy + pad / 2 + 2 * qs + qs / 2, fc);
        }
        SDL_RenderSetClipRect(r, NULL);
        if (ph_ < full) { SDL_SetRenderDrawColor(r, 214, 170, 70, 255); SDL_Rect e = { px_, py_ + ph_ - bt, pw_, bt }; SDL_RenderFillRect(r, &e); }   /* the edge while it is still growing */
    }

    /* the bag */
    {   int bx, by, bw, bh; bag_rect(&bx, &by, &bw, &bh);
        int up = ui_open || ui_p > 0.0f, bounce = (new_t >= 0.0f && new_t < 0.2f) ? qs : 0;
        box(r, bx, by, bw, bh, up ? 236 : 214, up ? 190 : 170, up ? 78 : 70, 6, 8, 20, 245);
        item_draw_bag_icon(r, bx + (bw - ITEM_ICON_CELLS * icell) / 2, by + (bh - ITEM_ICON_CELLS * icell) / 2 - bounce, icell);
        int cnt = shown();
        if (cnt > 0) {                                       /* the badge on the lower left corner */
            char num[16]; snprintf(num, sizeof num, "%d", cnt);
            int fc = fcell("00", bag / 2), tw = font_width(num, fc), th = font_height(fc);
            int bw2 = tw + 2 * pad, bh2 = th + pad, x0 = bx - bw2 / 2, y0 = by + bh - bh2 + bh2 / 3 - (new_t >= 0.0f && new_t < 0.2f ? qs : 0);
            int tick = new_t >= 0.0f && new_t < 0.3f;
            box(r, x0, y0, bw2, bh2, 236, 190, 78, tick ? 255 : 6, tick ? 238 : 8, tick ? 170 : 20, 255);
            SDL_SetRenderDrawColor(r, tick ? 6 : 255, tick ? 8 : 238, tick ? 20 : 170, 255);
            font_draw(r, num, x0 + (bw2 - tw) / 2, y0 + (bh2 - th) / 2, fc);
        }
    }
    if (fly_t < INV_FLY_T) draw_flying(r);
}
