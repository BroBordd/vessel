/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "char.h"
#include <math.h>

/* chibi sprite, 10 wide x 12 tall: 8 rows of head, 3 of body, 1 of legs.
 * legend: h hair, s skin, c shirt, p pants, b boots. Eyes are drawn on top.
 * The side sprite faces RIGHT; facing LEFT mirrors it, so the face/nose and the
 * hair-at-the-back always point the way the character walks. */
#define SPR_W 10
#define SPR_H 12

static const char *body[4] = {
    "..cccccc..",
    ".sccccccs.",
    "..pppppp..",
    "..bb..bb..",
};
static const char *head_front[8] = {
    "..hhhhhh..",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    ".hssssssh.",
    ".ssssssss.",
    ".ssssssss.",
    ".ssssssss.",
    "..ssssss..",
};
static const char *head_back[8] = {
    "..hhhhhh..",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    "..hhhhhh..",
};
static const char *head_side[8] = {
    "..hhhhhh..",
    ".hhhhhhhh.",
    ".hhhhhhhh.",
    ".hhhsssss.",
    ".hhssssss.",
    ".hhsssssss",       /* nose */
    ".hhssssss.",
    "..hsssss..",
};

static void fill(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect q = { x, y, w, h };
    SDL_RenderFillRect(r, &q);
}

static void use(SDL_Renderer *r, Rgb c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255); }

static char cell_at(const Person *p, int facing, int x, int y) {
    const char **head = facing == FACE_DOWN ? head_front : facing == FACE_UP ? head_back : head_side;
    char c = y < 8 ? head[y][x] : body[y - 8][x];
    if (p->long_hair) {
        if (facing == FACE_DOWN) {
            if (y >= 4 && y <= 8 && (x == 1 || x == 8)) c = 'h';
        } else if (facing == FACE_UP) {
            if (y >= 8 && y <= 9 && x >= 2 && x <= 7) c = 'h';
        } else {
            if (y >= 4 && y <= 8 && x == 1) c = 'h';
            if (y >= 6 && y <= 8 && x == 2) c = 'h';
        }
    }
    return c;
}

/* rows = how many sprite rows to draw; lift = which foot to raise (0 left, 1 right, -1 none) */
static void draw_sprite(SDL_Renderer *r, const Person *p, int facing, int ox, int oy,
                        int s, int rows, int lift) {
    int flip = facing == FACE_LEFT;
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < SPR_W; x++) {
            char c = cell_at(p, facing, x, y);
            if (c == '.') continue;
            int dy = y;
            if (y == 11 && lift >= 0 && ((x < 5) == (lift == 0))) dy = 10;   /* raised foot */
            switch (c) {
            case 'h': use(r, p->hair);  break;
            case 's': use(r, p->skin);  break;
            case 'c': use(r, p->shirt); break;
            case 'p': use(r, p->pants); break;
            case 'b': use(r, p->boots); break;
            }
            int dx = flip ? SPR_W - 1 - x : x;
            fill(r, ox + dx * s, oy + dy * s, s, s);
        }

    /* big eyes: 2x3 dark blocks with a 1px highlight, rows 4-6 of the head */
    if (facing != FACE_UP) {
        int ex[2], n = 0;
        if (facing == FACE_DOWN) { ex[n++] = 2; ex[n++] = 6; }
        else                     { ex[n++] = flip ? 2 : 6; }
        for (int i = 0; i < n; i++) {
            SDL_SetRenderDrawColor(r, 28, 22, 40, 255);
            fill(r, ox + ex[i] * s, oy + 4 * s, 2 * s, 3 * s);
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            int hx = (facing == FACE_DOWN) ? ex[i] : (flip ? ex[i] + 1 : ex[i]);
            fill(r, ox + hx * s, oy + 4 * s, s, s);
        }
    }

    /* white shades over the eyes (rows 4-5): one wide bar from the front, one lens + temple from the side */
    if ((p->acc & ACC_SHADES) && facing != FACE_UP) {
        SDL_SetRenderDrawColor(r, 250, 250, 255, 255);
        if (facing == FACE_DOWN) {
            fill(r, ox + 1 * s, oy + 4 * s, 3 * s, 2 * s);
            fill(r, ox + 6 * s, oy + 4 * s, 3 * s, 2 * s);
            fill(r, ox + 4 * s, oy + 4 * s, 2 * s, s);               /* bridge */
        } else {
            int lx = flip ? SPR_W - 1 - 8 : 5;                       /* lens: sprite columns 5..8 */
            fill(r, ox + lx * s, oy + 4 * s, 4 * s, 2 * s);
            int tx = flip ? SPR_W - 1 - 4 : 2;                       /* temple arm back over the hair */
            fill(r, ox + tx * s, oy + 4 * s, 3 * s, s);
        }
        SDL_SetRenderDrawColor(r, 170, 180, 205, 255);               /* a little shade under the lens */
        if (facing == FACE_DOWN) {
            fill(r, ox + 1 * s, oy + 5 * s, 3 * s, s / 2 > 0 ? s / 2 : 1);
            fill(r, ox + 6 * s, oy + 5 * s, 3 * s, s / 2 > 0 ? s / 2 : 1);
        }
    }
}

/* floating halo above the head, drawn separately because it sits outside the sprite box */
static void draw_halo(SDL_Renderer *r, int ox, int oy, int s) {
    int bob = ((SDL_GetTicks() / 450) & 1) ? s / 2 : 0;
    int y = oy - 4 * s - bob;
    SDL_SetRenderDrawColor(r, 255, 236, 140, 70);                    /* soft glow */
    fill(r, ox + 1 * s, y - s, 8 * s, 5 * s);
    SDL_SetRenderDrawColor(r, 255, 214, 70, 255);
    fill(r, ox + 3 * s, y, 4 * s, s);                                /* top arc */
    fill(r, ox + 2 * s, y + s, s, s);
    fill(r, ox + 7 * s, y + s, s, s);
    fill(r, ox + 1 * s, y + 2 * s, s, s);                            /* sides */
    fill(r, ox + 8 * s, y + 2 * s, s, s);
    fill(r, ox + 2 * s, y + 3 * s, s, s);
    fill(r, ox + 7 * s, y + 3 * s, s, s);
    fill(r, ox + 3 * s, y + 4 * s - s / 2, 4 * s, s);                /* bottom arc */
    SDL_SetRenderDrawColor(r, 255, 250, 205, 255);                   /* highlight */
    fill(r, ox + 3 * s, y, 2 * s, s);
}

void char_draw_air(SDL_Renderer *r, const Person *p, int x, int y,
                   int facing, int moving, float walk, int s) {
    int frame = moving ? ((int)walk & 1) : 0;
    int bob = (moving && frame) ? 1 : 0;
    int ox = x - (SPR_W / 2) * s, oy = y - SPR_H * s - bob * s;
    draw_sprite(r, p, facing, ox, oy, s, SPR_H, moving ? frame : -1);
    if (p->acc & ACC_HALO) draw_halo(r, ox, oy, s);
}

void char_draw(SDL_Renderer *r, const Person *p, int x, int y,
               int facing, int moving, float walk, int s) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);                    /* shadow */
    fill(r, x - 4 * s, y - s, 8 * s, 2 * s);
    char_draw_air(r, p, x, y, facing, moving, walk, s);
}

/* falling over backwards, pivoting on the feet: a = 0 standing .. pi/2 lying on the back, head to the right. the eyes
 * close on the way down. the body slides so it ends centred on x, and the shadow grows to a long strip */
void char_draw_fall(SDL_Renderer *r, const Person *p, int x, int y, int s, float a) {
    float sa = sinf(a), ca = cosf(a);
    float shx = -6.0f * s * sa, shy = -3.0f * s * sa;            /* centre the lying body on x, rest it on the ground line */
    int sw = (int)((8.0f + 5.0f * sa) * s), sh = (int)((2.0f + 1.5f * sa) * s);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);
    fill(r, x + (int)shx - sw / 2 + (int)(sa * 6.0f * s), y - s + (int)(shy * 0.3f), sw, sh);
    int sz = (int)(s * 1.3f + 0.5f); if (sz < s + 1) sz = s + 1; /* a rotated grid of squares needs them a little big to leave no gaps */
    for (int j = 0; j < SPR_H; j++)
        for (int i = 0; i < SPR_W; i++) {
            char ch = cell_at(p, FACE_DOWN, i, j);
            if (ch == '.') continue;
            switch (ch) {
            case 'h': use(r, p->hair);  break;
            case 's': use(r, p->skin);  break;
            case 'c': use(r, p->shirt); break;
            case 'p': use(r, p->pants); break;
            case 'b': use(r, p->boots); break;
            }
            if (a > 0.35f && j == 5 && (i == 2 || i == 3 || i == 6 || i == 7)) SDL_SetRenderDrawColor(r, 28, 22, 40, 255);   /* closed eyes */
            else if (a <= 0.35f && j >= 4 && j <= 6 && (i == 2 || i == 3 || i == 6 || i == 7)) SDL_SetRenderDrawColor(r, 28, 22, 40, 255);
            float cx = (i - 5 + 0.5f) * s, cy = (j - SPR_H + 0.5f) * s;     /* from the feet middle */
            float rx = cx * ca - cy * sa, ry = cx * sa + cy * ca;
            fill(r, x + (int)(rx + shx) - sz / 2, y + (int)(ry + shy) - sz / 2, sz, sz);
        }
}

void char_draw_prone(SDL_Renderer *r, const Person *p, int x, int y, int s, float lift) {
    if (lift < 0) lift = 0;
    if (lift > 1) lift = 1;
    int up = (int)(lift * 3.0f + 0.5f) * s;                      /* upper body rises, legs stay */
    int ox = x - 6 * s, oy = y - 6 * s;                          /* body is 12 wide, rows -1..5 */

    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);                      /* shadow stays on the ground */
    fill(r, ox - s, oy + 1 * s, 14 * s, 6 * s);

    use(r, p->pants);                                            /* legs, feet to the right */
    fill(r, ox + 8 * s, oy + 0 * s, 2 * s, 2 * s);
    fill(r, ox + 8 * s, oy + 3 * s, 2 * s, 2 * s);
    use(r, p->boots);
    fill(r, ox + 10 * s, oy + 0 * s, 2 * s, 2 * s);
    fill(r, ox + 10 * s, oy + 3 * s, 2 * s, 2 * s);

    use(r, p->shirt);                                            /* torso and sleeves */
    fill(r, ox + 4 * s, oy + 0 * s - up, 4 * s, 5 * s);
    fill(r, ox + 4 * s, oy - 1 * s - up, 3 * s, s);
    fill(r, ox + 4 * s, oy + 5 * s - up, 3 * s, s);
    use(r, p->skin);                                             /* hands pushing on the ground */
    fill(r, ox + 2 * s, oy - 1 * s - up / 2, 2 * s, s);
    fill(r, ox + 2 * s, oy + 5 * s - up / 2, 2 * s, s);

    use(r, p->hair);                                             /* back of the head */
    fill(r, ox + 0 * s, oy + 0 * s - up, 4 * s, 5 * s);
    fill(r, ox + 0 * s + s, oy - s / 2 - up, 3 * s, s);
    if (p->long_hair) fill(r, ox + 3 * s, oy + 0 * s - up, 2 * s, 5 * s);
    if (lift > 0.35f) {                                          /* face turning up as they rise */
        use(r, p->skin);
        fill(r, ox + 0 * s, oy + 2 * s - up, s, 2 * s);
    }
}

void char_draw_portrait(SDL_Renderer *r, const Person *p, int x, int y, int s, int talking) {
    draw_sprite(r, p, FACE_DOWN, x, y, s, 10, -1);
    if (talking) {
        SDL_SetRenderDrawColor(r, 150, 70, 80, 255);
        fill(r, x + 4 * s, y + 7 * s, 2 * s, s);
    }
}
