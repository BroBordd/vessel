/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "grave.h"
#include "gfx.h"
#include <math.h>

#define GREY_PHOTO 232              /* gfx_set_filter grey, 0..256: nearly colourless, like an old photo */

static int s, tile, count;
static struct { int tx, ty; const Person *who; } g[GRAVE_MAX];

void grave_reset(int pixel_scale, int tile_size) { s = pixel_scale; tile = tile_size; count = 0; }

int grave_place(int tile_x, int tile_y, const Person *dead) {
    if (count >= GRAVE_MAX || !dead) return -1;
    g[count].tx = tile_x; g[count].ty = tile_y; g[count].who = dead;
    return count++;
}

int   grave_count(void) { return count; }
void  grave_tile(int i, float *x, float *y) { *x = g[i].tx + 0.5f; *y = g[i].ty + 0.9f; }
const Person *grave_person(int i) { return g[i].who; }
float grave_foot_y(int i) { return (g[i].ty + 0.9f) * tile; }

int grave_collides(float fx, float fy, float half_w, float h) {
    for (int i = 0; i < count; i++)
        if (fabsf(fx - (g[i].tx + 0.5f) * tile) < half_w + 7.0f * s && fabsf(fy - grave_foot_y(i)) < h + 1.0f * s) return 1;
    return 0;
}

/* the old photo: the portrait is a 10 x 10 sprite, drawn at half the world's pixel size so it is a small picture on the stone */
static int photo_scale(void) { int ps = s / 2; return ps < 1 ? 1 : ps; }

void grave_portrait_rect(int i, int cam_x, int cam_y, int *x, int *y, int *w, int *h) {
    int fx = (int)((g[i].tx + 0.5f) * tile) - cam_x, fy = (int)((g[i].ty + 0.9f) * tile) - cam_y;
    int p = 10 * photo_scale();
    *w = p; *h = p;
    *x = fx - p / 2;
    *y = fy - 17 * s + s;                                   /* the frame is one unit thick and starts 17 units above the feet */
}

/* ---------- drawing: pixel art, 18 units wide, 19 tall. (x, y) = feet centre. ---------- */
static void box(SDL_Renderer *r, int x, int y, int ux, int uy, int uw, int uh) {      /* units, relative to the feet centre */
    SDL_Rect q = { x + ux * s, y + uy * s, uw * s, uh * s };
    SDL_RenderFillRect(r, &q);
}
static void col(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }

void grave_draw(SDL_Renderer *r, int i, int cam_x, int cam_y) {
    if (i < 0 || i >= count) return;
    int x = (int)((g[i].tx + 0.5f) * tile) - cam_x, y = (int)((g[i].ty + 0.9f) * tile) - cam_y;

    col(r, 0, 0, 0, 70);        box(r, x, y, -10, -1, 20, 2);                          /* ground shadow */

    /* the stone: a slab with a rounded top and a plinth, lit from the left */
    col(r, 98, 102, 112, 255);  box(r, x, y, -7, -5, 14, 2);                           /* plinth */
    col(r, 150, 154, 164, 255); box(r, x, y, -7, -5, 14, 1);
    col(r, 140, 144, 154, 255); box(r, x, y, -4, -19, 8, 1);                           /* slab: top, shoulders, body */
    box(r, x, y, -5, -18, 10, 1); box(r, x, y, -6, -17, 12, 12);
    col(r, 178, 182, 192, 255); box(r, x, y, -4, -19, 8, 1); box(r, x, y, -5, -18, 1, 1); box(r, x, y, -6, -17, 1, 12);   /* light edge */
    col(r, 104, 108, 118, 255); box(r, x, y, 5, -17, 1, 12); box(r, x, y, 4, -18, 1, 1); /* shadow edge */
    col(r, 112, 140, 100, 255); box(r, x, y, -5, -7, 2, 1); box(r, x, y, 3, -9, 1, 2);   /* a little moss */

    /* the framed photo: a dark frame one unit thick, a pale card behind the picture, the picture itself greyed */
    int px_, py_, pw, ph; grave_portrait_rect(i, cam_x, cam_y, &px_, &py_, &pw, &ph);
    SDL_Rect frame = { px_ - s, py_ - s, pw + 2 * s, ph + 2 * s };
    col(r, 46, 36, 30, 255);    SDL_RenderFillRect(r, &frame);
    gfx_set_filter(GREY_PHOTO, 0);
    col(r, 214, 208, 192, 255); { SDL_Rect card = { px_, py_, pw, ph }; SDL_RenderFillRect(r, &card); }
    char_draw_portrait(r, g[i].who, px_, py_, photo_scale(), 0);
    gfx_set_filter(0, 0);                                                              /* (world.c puts its own filter back) */
    col(r, 92, 96, 106, 255);   box(r, x, y, -3, -8, 6, 1); box(r, x, y, -2, -6, 4, 1); /* two engraved lines: a name nobody can read */

    /* the plot: a mound of earth in front of the stone, a few tufts of grass, one flower */
    col(r, 92, 66, 46, 255);    box(r, x, y, -9, -3, 18, 3);
    col(r, 122, 90, 62, 255);   box(r, x, y, -9, -3, 18, 1);
    col(r, 70, 50, 36, 255);    box(r, x, y, -9, -1, 18, 1);
    col(r, 76, 150, 66, 255);   box(r, x, y, -8, -4, 1, 1); box(r, x, y, -7, -5, 1, 2); box(r, x, y, -5, -4, 1, 1); box(r, x, y, 2, -4, 1, 1);
    col(r, 70, 140, 60, 255);   box(r, x, y, 7, -7, 1, 4); box(r, x, y, 8, -5, 1, 1);  /* the stem and a leaf */
    col(r, 240, 120, 170, 255); box(r, x, y, 7, -10, 1, 1); box(r, x, y, 6, -9, 1, 1); box(r, x, y, 8, -9, 1, 1); box(r, x, y, 7, -8, 1, 1);   /* the petals */
    col(r, 255, 214, 110, 255); box(r, x, y, 7, -9, 1, 1);                             /* its heart */
}
