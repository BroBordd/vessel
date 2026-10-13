/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "bsearch.h"
#include "npc.h"
#include "grave.h"
#include "audio.h"
#include <math.h>
#include <string.h>

#define BSEARCH_RANGE 2.0f          /* tiles from the body: this close and the button shows the magnifying glass */
#define BSEARCH_LEAVE 2.6f

static struct { int npc, on, in_range, active, n; float prog, tick; void (*cb)(void); } b = { -1 };
static int tile = 48;

void bsearch_reset(int tile_size) { memset(&b, 0, sizeof b); b.npc = -1; tile = tile_size; }
void bsearch_enable(int npc_id, void (*on_done)(void)) {
    if (npc_id < 0 || npc_id >= npc_count()) return;
    b.npc = npc_id; b.on = 1; b.in_range = b.active = 0; b.prog = b.tick = 0; b.n = 0; b.cb = on_done;
}
int   bsearch_enabled(void)  { return b.on; }
int   bsearch_npc(void)      { return b.on ? b.npc : -1; }
int   bsearch_active(void)   { return b.on && b.active; }
float bsearch_progress(void) { return b.prog; }

int bsearch_near(float pxw, float pyw, int allow) {
    if (!b.on || !allow || b.npc < 0 || b.npc >= npc_count() || !npc_lying(b.npc)) { b.in_range = b.active = 0; b.prog = 0; return 0; }
    float bx, by; npc_tile(b.npc, &bx, &by);
    float dx = pxw - bx * tile, dy = pyw - by * tile, d = sqrtf(dx * dx + dy * dy) / tile;
    if (!b.in_range && d < BSEARCH_RANGE) b.in_range = 1;
    else if (b.in_range && d > BSEARCH_LEAVE) b.in_range = 0;
    if (!b.in_range) { b.active = 0; b.prog = 0; }                  /* walked away: the search is lost */
    return b.in_range;
}
int bsearch_update(float dt, int holding) {
    if (!b.on) return 0;
    if (holding && b.in_range) {                            /* chunk 64: a hold, like the grave search */
        b.active = 1;
        b.prog += dt / GRAVE_SEARCH_T;
        b.tick -= dt;
        if (b.tick <= 0) { b.tick = 0.17f; sfx_blip((b.n++ & 1) ? 1.5f : 1.25f); }      /* the same soft, quick rustle as the grave search */
        if (b.prog >= 1.0f) {
            b.prog = 0; b.on = 0; b.active = 0; b.in_range = 0;
            sfx_blip(1.9f);
            if (b.cb) { void (*cb)(void) = b.cb; b.cb = NULL; cb(); }
            return 1;
        }
    } else {
        b.active = 0; b.tick = 0;
        if (b.prog > 0) { b.prog -= dt / SEARCH_DECAY_T; if (b.prog < 0) b.prog = 0; }
    }
    return 0;
}
