/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "npc.h"
#include "dialog.h"
#include <math.h>

#define MAX_NPC     16
#define TALK_RANGE  1.7f            /* tiles: walk this close to start talking */
#define LEAVE_RANGE 2.2f            /* tiles: walk this far away to be able to talk again */

typedef struct {
    const Person *who;
    float tx, ty;                   /* feet position in tiles */
    int   facing, in_range;
    void (*on_talk)(int);
} Npc;

static Npc n[MAX_NPC];
static int count, s, tile;

void npc_reset(int pixel_scale, int tile_size) { count = 0; s = pixel_scale; tile = tile_size; }

int npc_add(const Person *who, int tile_x, int tile_y, void (*on_talk)(int)) {
    if (count >= MAX_NPC) return -1;
    n[count] = (Npc){ who, tile_x + 0.5f, tile_y + 0.9f, FACE_DOWN, 0, on_talk };
    return count++;
}

void npc_update(float px, float py) {
    for (int i = 0; i < count; i++) {
        float dx = px - n[i].tx * tile, dy = py - n[i].ty * tile;
        float d = sqrtf(dx * dx + dy * dy) / tile;
        if (d < TALK_RANGE && !n[i].in_range && !dialog_active()) {
            n[i].in_range = 1;
            if (fabsf(dx) > fabsf(dy)) n[i].facing = dx < 0 ? FACE_LEFT : FACE_RIGHT;   /* turn to the player */
            else                       n[i].facing = dy < 0 ? FACE_UP : FACE_DOWN;
            if (n[i].on_talk) n[i].on_talk(i);
        } else if (d > LEAVE_RANGE) n[i].in_range = 0;
    }
}

int   npc_count(void)          { return count; }
float npc_foot_y(int id)       { return n[id].ty * tile; }

void npc_draw(SDL_Renderer *r, int id, int cam_x, int cam_y) {
    char_draw(r, n[id].who, (int)(n[id].tx * tile) - cam_x, (int)(n[id].ty * tile) - cam_y,
              n[id].facing, 0, 0.0f, s);
}

int npc_collides(float fx, float fy, float half_w, float h) {
    for (int i = 0; i < count; i++)
        if (fabsf(fx - n[i].tx * tile) < half_w + 3.0f * s && fabsf(fy - n[i].ty * tile) < h) return 1;
    return 0;
}
