/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "item.h"
#include "font.h"
#include "audio.h"
#include <math.h>
#include <string.h>

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
static const char *const *ICONS[ITEM_COUNT] = { HAMMER, ORB };
static const char *NAMES[ITEM_COUNT] = { "HAMMER", "ORB" };

const char *item_name(int kind) { return kind >= 0 && kind < ITEM_COUNT ? NAMES[kind] : ""; }

static int s = 1, tile = 16;                /* pixel scale and tile size of the current map */
static int on, on_kind, tx, ty;             /* what lies on the map */
static int have[ITEM_COUNT];
static float new_t = 99.0f;                 /* seconds since the last pickup (the slot's frame pulses for a moment) */
static float drop_t = 99.0f;                /* seconds since the item on the map was dropped (chunk 32); 99 = it was simply placed */
#define DROP_BOUNCE_T 0.2f
#define DROP_HEIGHT   14                    /* units it falls from */

static int locked[ITEM_COUNT], n_blocked;
static float block_cd;                      /* seconds until the next turned-away try may speak */
static void (*blocked_cb)(void);
void item_set_locked(int kind, int lk) { if (kind >= 0 && kind < ITEM_COUNT) locked[kind] = lk ? 1 : 0; }
int  item_locked(int kind) { return kind >= 0 && kind < ITEM_COUNT && locked[kind]; }
void item_set_blocked_cb(void (*cb)(void)) { blocked_cb = cb; }
int  item_blocked_count(void) { return n_blocked; }
void item_reset(int pixel_scale, int tile_size) { s = pixel_scale; tile = tile_size; on = 0; drop_t = 99.0f; memset(locked, 0, sizeof locked); blocked_cb = NULL; block_cd = 0; }
void item_place(int kind, int tile_x, int tile_y) { if (kind < 0 || kind >= ITEM_COUNT) return; on = 1; on_kind = kind; tx = tile_x; ty = tile_y; drop_t = 99.0f; }
void item_drop(int kind, int tile_x, int tile_y) { item_place(kind, tile_x, tile_y); if (on) drop_t = 0; }
int  item_dropping(void) { return on && drop_t < ITEM_DROP_T; }
int  item_exists(void) { return on; }
void item_tile(float *x, float *y) { *x = tx + 0.5f; *y = ty + 0.5f; }
int  item_has(int kind) { return kind >= 0 && kind < ITEM_COUNT && have[kind]; }
void item_give(int kind) { if (kind < 0 || kind >= ITEM_COUNT) return; have[kind] = 1; new_t = 0; }
void item_clear(void) { memset(have, 0, sizeof have); new_t = 99.0f; }

int item_update(float dt, float pxw, float pyw, int allow) {
    if (new_t < 99.0f) new_t += dt;
    if (block_cd > 0) block_cd -= dt;
    if (on && drop_t < 99.0f) {                                 /* chunk 32: a dropped item falls first, and thunks when it lands */
        float before = drop_t; drop_t += dt;
        if (before < ITEM_DROP_T && drop_t >= ITEM_DROP_T) sfx_blip(0.42f);
        if (drop_t > ITEM_DROP_T + DROP_BOUNCE_T + 1.0f) drop_t = 99.0f;
    }
    if (!on || !allow || item_dropping()) return -1;
    float dx = pxw - (tx + 0.5f) * tile, dy = pyw - (ty + 0.5f) * tile;
    if (sqrtf(dx * dx + dy * dy) / tile >= ITEM_PICK_R) return -1;
    int k = on_kind;
    if (locked[k]) {                                            /* chunk 33: it stays where it is */
        if (block_cd <= 0) { block_cd = ITEM_BLOCK_GAP; n_blocked++; if (blocked_cb) blocked_cb(); }
        return -1;
    }
    on = 0; item_give(k);
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
static void draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell, int lying) { draw_rows(r, ICONS[kind], x, y, cell, lying); }
void item_draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell) {
    if (kind < 0 || kind >= ITEM_COUNT || cell < 1) return;
    draw_icon(r, kind, x, y, cell, 0);
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
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 70);
    SDL_Rect sh = { x - 7 * s, y + 3 * s, 14 * s, 2 * s }; SDL_RenderFillRect(r, &sh);      /* ground shadow */
    draw_icon(r, on_kind, x - 6 * s, y - 6 * s + bob * s - lift * s, s, 1);
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

/* ---------- the inventory widget ---------- */
static int W, H, ux, uy, uw, bt, slot, icell, pad;
static int ui_open, ui_down;

void item_ui_init(int w, int h, int right_edge, int top, int width) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    pad = (int)(4 * u); if (pad < 2) pad = 2;
    uw = width; ux = right_edge - width; uy = top;
    slot = width / 3; if (slot < 24) slot = 24;
    icell = (slot - 2 * bt - 2 * pad) / ITEM_ICON_CELLS; if (icell < 1) icell = 1;
    ui_open = ui_down = 0;
}

static int owned(void) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) n += have[k]; return n; }
static int fcell(const char *name, int room) {              /* the biggest letter size (up to 3) that still fits `room` */
    int c = (int)(2.0f * (W < H ? W : H) / 360.0f); if (c < 1) c = 1; if (c > 3) c = 3;
    while (c > 1 && font_width(name, c) > room) c--;
    return c;
}
/* the panel: each item stacked, a big icon over its name */
static int big_cell(void) { int c = uw / 24; return c < 2 ? 2 : c; }
static int block_h(void) { return pad + ITEM_ICON_CELLS * big_cell() + pad + font_height(fcell("MMMMMM", uw - 2 * bt - 2 * pad)) + pad; }

static void rect_of(int *x, int *y, int *w, int *h) {
    int n = owned();
    if (ui_open) { *x = ux; *y = uy; *w = uw; *h = 2 * bt + n * block_h(); }
    else      { *w = n * slot + (n > 1 ? (n - 1) * pad : 0); *h = slot; *x = ux + uw - *w; *y = uy; }
}
int item_ui_hit(int x, int y) {
    if (owned() == 0) return 0;
    int rx, ry, rw, rh; rect_of(&rx, &ry, &rw, &rh);
    return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}
int item_ui_open(void) { return ui_open && owned() > 0; }

int item_ui_touch(int a, int x, int y) {
    if (owned() == 0) { ui_open = ui_down = 0; return 0; }
    if (a == 0) { if (item_ui_hit(x, y)) { ui_down = 1; return 1; } return 0; }
    if (!ui_down) return 0;
    if (a == 1) { if (item_ui_hit(x, y)) ui_open = !ui_open; ui_down = 0; return 1; }
    if (a == 3) ui_down = 0;                                 /* cancelled */
    return 1;                                                /* a drag that began on the inventory stays its own */
}

void item_ui_draw(SDL_Renderer *r, float t) {
    int n = owned();
    if (n == 0) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int rx, ry, rw, rh; rect_of(&rx, &ry, &rw, &rh);
    int pulse = new_t < 2.5f ? 150 + (int)(105 * sinf(new_t * 9.0f)) : 235;                  /* the frame blinks for a moment after a pickup */
    SDL_Rect fr = { rx, ry, rw, rh };
    SDL_SetRenderDrawColor(r, 255, 255, 255, pulse); SDL_RenderFillRect(r, &fr);
    if (ui_open) {
        SDL_Rect in = { rx + bt, ry + bt, rw - 2 * bt, rh - 2 * bt };
        SDL_SetRenderDrawColor(r, 6, 8, 20, 245); SDL_RenderFillRect(r, &in);
        int by = ry + bt, bc = big_cell();
        for (int k = 0; k < ITEM_COUNT; k++) {
            if (!have[k]) continue;
            item_draw_icon(r, k, rx + (rw - ITEM_ICON_CELLS * bc) / 2, by + pad, bc);
            const char *nm = item_name(k);
            int fc = fcell(nm, rw - 2 * bt - 2 * pad);
            SDL_SetRenderDrawColor(r, 235, 240, 255, 255);
            font_draw(r, nm, rx + (rw - font_width(nm, fc)) / 2, by + pad + ITEM_ICON_CELLS * bc + pad, fc);
            by += block_h();
        }
    } else {
        int sx = rx;
        for (int k = 0; k < ITEM_COUNT; k++) {
            if (!have[k]) continue;
            SDL_Rect in = { sx + bt, ry + bt, slot - 2 * bt, slot - 2 * bt };
            SDL_SetRenderDrawColor(r, 6, 8, 20, 245); SDL_RenderFillRect(r, &in);
            item_draw_icon(r, k, sx + (slot - ITEM_ICON_CELLS * icell) / 2, ry + (slot - ITEM_ICON_CELLS * icell) / 2, icell);
            sx += slot + pad;
        }
    }
    (void)t;
}
