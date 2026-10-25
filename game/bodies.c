/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the dead, see bodies.h. */
#include "bodies.h"
#include "char.h"
#include "story.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static Body list[BODIES_MAX];
static int  n;

void bodies_reset(void) { n = 0; memset(list, 0, sizeof list); }
int  bodies_add(unsigned vessel, float tx, float ty, const unsigned char *inv) {
    if (n >= BODIES_MAX) return -1;
    Body *b = &list[n];
    memset(b, 0, sizeof *b);
    b->vessel = vessel; b->tx = tx; b->ty = ty;
    if (inv) memcpy(b->inv, inv, ITEM_COUNT);
    return n++;
}
int bodies_count(void) { return n; }
const Body *bodies_get(int i) { return i >= 0 && i < n ? &list[i] : NULL; }
const Body *bodies_find(unsigned vessel) { for (int i = 0; i < n; i++) if (list[i].vessel == vessel) return &list[i]; return NULL; }

void bodies_inv_text(const Body *b, char *out, int cap) {
    if (!out || cap < 1) return;
    out[0] = 0;
    if (!b) return;
    int k = 0, have[ITEM_COUNT];
    for (int i = 0; i < ITEM_COUNT; i++) if (b->inv[i]) have[k++] = i;
    if (k == 0) { snprintf(out, (size_t)cap, "has nothing"); return; }
    int len = snprintf(out, (size_t)cap, "has");
    for (int i = 0; i < k && len < cap - 1; i++) {
        const char *sep = i == 0 ? " a " : i == k - 1 ? " and a " : ", a ";
        len += snprintf(out + len, (size_t)(cap - len), "%s", sep);
        for (const char *s = item_name(have[i]); *s && len < cap - 1; s++) out[len++] = (char)tolower((unsigned char)*s);
        out[len] = 0;
    }
}

static void box(const Body *b, int px, int tile, float *x0, float *y0, float *x1, float *y1) {
    float fx = b->tx * (float)tile, fy = b->ty * (float)tile;
    *x0 = fx - 6.0f * (float)px; *x1 = fx + 6.0f * (float)px; *y0 = fy - 9.0f * (float)px; *y1 = fy;
}

int bodies_hit(float fx, float fy, int px, int tile) {
    for (int i = 0; i < n; i++) {
        if (list[i].vessel == 1) continue;
        float x0, y0, x1, y1; box(&list[i], px, tile, &x0, &y0, &x1, &y1);
        if (fx + 3.0f * px > x0 && fx - 3.0f * px < x1 && fy > y0 && fy - 3.0f * px < y1) return 1;
    }
    return 0;
}

int bodies_nearest(float fx, float fy, int px, int tile, float reach, float *gap) {
    int best = -1; float bg = reach;
    for (int i = 0; i < n; i++) {
        if (list[i].vessel == 1) continue;
        float x0, y0, x1, y1; box(&list[i], px, tile, &x0, &y0, &x1, &y1);
        float dx = fmaxf(fmaxf(x0 - (fx + 3.0f * px), (fx - 3.0f * px) - x1), 0.0f), dy = fmaxf(fmaxf(y0 - fy, (fy - 3.0f * px) - y1), 0.0f);
        float g = sqrtf(dx * dx + dy * dy) / (float)px;
        if (g <= bg) { bg = g; best = i; }
    }
    if (best >= 0 && gap) *gap = bg;
    return best;
}

void bodies_draw(SDL_Renderer *r, int cam_x, int cam_y, int px, int tile, float darken) {
    float k = 1.0f - darken; if (k < 0) k = 0;
    for (int i = 0; i < n; i++) {
        const Body *b = &list[i];
        if (b->vessel == 1) continue;
        int fx = (int)lroundf(b->tx * (float)tile) - cam_x, fy = (int)lroundf(b->ty * (float)tile) - cam_y;
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);                         /* the pool: a flat ellipse of blood, a little bigger than he is */
        SDL_SetRenderDrawColor(r, (Uint8)(104 * k), (Uint8)(14 * k), (Uint8)(22 * k), 215);
        for (int j = -4; j <= 4; j++) {
            int w = (int)(sqrtf(1.0f - (float)(j * j) / 25.0f) * 11.0f);
            SDL_Rect q = { fx - w * px, fy - 4 * px + j * px, 2 * w * px, px };
            SDL_RenderFillRect(r, &q);
        }
        char_set_darken(darken);
        char_draw_lying(r, story_vessel_look(b->vessel), fx, fy, px);
        char_set_darken(0);
    }
}
