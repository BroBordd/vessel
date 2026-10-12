/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "npc.h"
#include "dialog.h"
#include "gfx.h"
#include <math.h>

#define MAX_NPC     16
#define TALK_RANGE  1.7f            /* tiles: this close and the interact button appears */
#define LEAVE_RANGE 2.2f            /* tiles: this far and it goes away again */
#define NPC_FLASH_T   0.18f         /* the white flash of a hit */
#define NPC_SHOVE_T   0.22f         /* the recoil eases back over this long */
#define NPC_SHOVE_UNITS 2           /* ... starting this many pixel units back */
#define NPC_FALL_T    0.9f          /* the topple of a dead npc */

typedef struct {
    const Person *who;
    float tx, ty;                   /* feet position in tiles */
    int   facing, rest_facing, in_range;   /* rest_facing: default stance, restored on leaving */
    void (*on_talk)(int);
    int   hostile;                  /* chunk 36: the hammer can hit her */
    float flash, shove;             /* seconds left of the white flash / of the recoil */
    int   shove_x, shove_y;         /* the recoil's direction (-1, 0, 1) */
    int   lying, headless;          /* chunk 38 */
    float lying_t;                  /* seconds since she began to fall */
    int   shake_x, shake_y;         /* chunk 38.1: drawn offset of the lying body, px */
} Npc;

static Npc n[MAX_NPC];
static int count, s, tile;
static int nearby = -1;

void npc_reset(int pixel_scale, int tile_size) { count = 0; nearby = -1; s = pixel_scale; tile = tile_size; }

int npc_add(const Person *who, int tile_x, int tile_y, void (*on_talk)(int)) {
    if (count >= MAX_NPC) return -1;
    n[count] = (Npc){ who, tile_x + 0.5f, tile_y + 0.9f, FACE_DOWN, FACE_DOWN, 0, on_talk, 0, 0, 0, 0, 0, 0, 0, 0 };
    return count++;
}

/* works out who is in talking range (with a little hysteresis so the button doesn't flicker)
 * and turns them to face the player. nothing is triggered here: talking is up to the button. */
void npc_update(float px, float py) {
    nearby = -1;
    float best = 1e9f;
    for (int i = 0; i < count; i++) {
        float dx = px - n[i].tx * tile, dy = py - n[i].ty * tile;
        float d = sqrtf(dx * dx + dy * dy) / tile;
        if (!n[i].in_range && d < TALK_RANGE)  n[i].in_range = 1;
        else if (n[i].in_range && d > LEAVE_RANGE) {
            n[i].in_range = 0;
            n[i].facing = n[i].rest_facing;                 /* player walked away: back to the default stance */
        }
        if (!n[i].in_range) continue;
        if (n[i].lying) continue;                           /* chunk 38: nobody to talk to */
        if (fabsf(dx) > fabsf(dy)) n[i].facing = dx < 0 ? FACE_LEFT : FACE_RIGHT;
        else                       n[i].facing = dy < 0 ? FACE_UP : FACE_DOWN;
        if (d < best) { best = d; nearby = i; }
    }
}

void npc_set_facing(int id, int facing) {
    if (id < 0 || id >= count) return;
    n[id].rest_facing = facing;
    if (!n[id].in_range) n[id].facing = facing;
}

void npc_set_hostile(int id, int on) { if (id >= 0 && id < count) n[id].hostile = on != 0; }
int  npc_hostile(int id)             { return id >= 0 && id < count && n[id].hostile; }
int  npc_flashing(int id)            { return id >= 0 && id < count && n[id].flash > 0; }
void npc_hit(int id, float from_x, float from_y) {
    if (id < 0 || id >= count) return;
    float dx = n[id].tx * tile - from_x, dy = n[id].ty * tile - from_y;
    n[id].shove_x = n[id].shove_y = 0;
    if (fabsf(dx) > fabsf(dy)) n[id].shove_x = dx < 0 ? -1 : 1;
    else                       n[id].shove_y = dy < 0 ? -1 : 1;
    n[id].flash = NPC_FLASH_T; n[id].shove = NPC_SHOVE_T;
}
void npc_set_lying_now(int id)       { if (id >= 0 && id < count) { n[id].lying = 1; n[id].lying_t = NPC_FALL_T; } }   /* chunk 41: already down (a map made again) */
void npc_set_lying(int id)           { if (id >= 0 && id < count && !n[id].lying) { n[id].lying = 1; n[id].lying_t = 0; } }
int  npc_lying(int id)               { return id >= 0 && id < count && n[id].lying; }
void npc_set_headless(int id, int on) { if (id >= 0 && id < count) n[id].headless = on != 0; }
void npc_set_shake(int id, int ox, int oy) { if (id >= 0 && id < count) { n[id].shake_x = ox; n[id].shake_y = oy; } }
int  npc_headless(int id)            { return id >= 0 && id < count && n[id].headless; }
void npc_tick(float dt) {
    for (int i = 0; i < count; i++) {
        if (n[i].lying) n[i].lying_t += dt;
        if (n[i].flash > 0) { n[i].flash -= dt; if (n[i].flash < 0) n[i].flash = 0; }
        if (n[i].shove > 0) { n[i].shove -= dt; if (n[i].shove < 0) n[i].shove = 0; }
    }
}

int npc_nearby(void) { return nearby; }
const Person *npc_person(int id) { return n[id].who; }
void npc_interact(int id) {
    if (id >= 0 && id < count && n[id].on_talk) n[id].on_talk(id);
}

int   npc_count(void)          { return count; }
void  npc_tile(int id, float *tx, float *ty) { *tx = n[id].tx; *ty = n[id].ty; }
float npc_foot_y(int id)       { return n[id].ty * tile; }

void npc_draw(SDL_Renderer *r, int id, int cam_x, int cam_y) {
    float k = n[id].shove / NPC_SHOVE_T;                   /* the recoil: all the way back at the hit, eased out */
    int off = (int)(NPC_SHOVE_UNITS * s * k * k + 0.5f);
    if (n[id].flash > 0) gfx_set_white(256);
    if (n[id].headless) char_set_headless(1);
    if (n[id].lying) {                                      /* chunk 38: down on her back, head to the right */
        float f = n[id].lying_t / NPC_FALL_T; if (f > 1) f = 1;
        char_draw_fall(r, n[id].who, (int)(n[id].tx * tile) - cam_x + n[id].shake_x, (int)(n[id].ty * tile) - cam_y + n[id].shake_y, s, 1.5708f * f * f);
        char_set_headless(0);
        if (n[id].flash > 0) gfx_set_white(0);
        return;
    }
    char_draw(r, n[id].who, (int)(n[id].tx * tile) - cam_x + off * n[id].shove_x, (int)(n[id].ty * tile) - cam_y + off * n[id].shove_y,
              n[id].facing, 0, 0.0f, s);
    char_set_headless(0);
    if (n[id].flash > 0) gfx_set_white(0);
}

int npc_collides(float fx, float fy, float half_w, float h) {
    for (int i = 0; i < count; i++)
        if (fabsf(fx - n[i].tx * tile) < half_w + 3.0f * s && fabsf(fy - n[i].ty * tile) < h) return 1;
    return 0;
}
