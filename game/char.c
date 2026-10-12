/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "char.h"
#include <math.h>
#include <stdint.h>

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

/* the "flash" (the spawn blink after the fall): everything the character is drawn with is mixed toward white by this much, 0..1 */
static float g_lighten;
static int   g_headless, g_shut;                    /* chunk 38: no head; eyes forced shut */
static int   g_worried;                             /* chunk 52: everything drawn is worried */
static int   g_aim, n_aims;                         /* chunk 56: the aiming pose is being drawn (eyes look up, one shut) / how many times it was drawn */
void char_set_headless(int on) { g_headless = on != 0; }
void char_set_worried(int on) { g_worried = on != 0; }
void char_set_lighten(float k) { g_lighten = k < 0 ? 0 : k > 1 ? 1 : k; }
static void setc(SDL_Renderer *r, int R, int G, int B) {
    if (g_lighten > 0) { R += (int)((255 - R) * g_lighten); G += (int)((255 - G) * g_lighten); B += (int)((255 - B) * g_lighten); }
    SDL_SetRenderDrawColor(r, (Uint8)R, (Uint8)G, (Uint8)B, 255);
}
static void use(SDL_Renderer *r, Rgb c) { setc(r, c.r, c.g, c.b); }

/* BLINKING. everybody blinks on their own clock, so a crowd never blinks in step. the average person blinks about 15-20 times a minute
 * (every ~4 s, anywhere from 1 to 8 s apart), a blink lasts 0.1-0.25 s, and one in six is a double blink. no state is kept: time is cut into
 * 4 s slots and each person's hash picks, for every slot, when the blink starts, how long it lasts, and whether it is a double.
 * the same person blinks the same way in the world and in a face popup, since both are drawn through draw_sprite. */
static unsigned hash_u(unsigned a, unsigned b) { unsigned h = a * 374761393u + b * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static float rnd01(unsigned h) { return (float)(h & 0xffffu) / 65536.0f; }
static int eyes_shut(const Person *p) {
    const double SLOT = 4.0;
    double t = SDL_GetTicks() / 1000.0;
    unsigned seed = (unsigned)((uintptr_t)p >> 3) * 2654435761u;
    long n = (long)(t / SLOT);
    for (long m = n - 1; m <= n; m++) {                                  /* a blink can run over into the next slot */
        unsigned h = hash_u(seed, (unsigned)m);
        double start = m * SLOT + rnd01(hash_u(h, 1)) * (SLOT - 0.5);
        double dur = 0.10 + 0.15 * rnd01(hash_u(h, 2));
        if (t >= start && t < start + dur) return 1;
        if (rnd01(hash_u(h, 3)) < 0.17 && t >= start + dur + 0.12 && t < start + 2 * dur + 0.12) return 1;   /* the double blink */
    }
    return 0;
}

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
            if (g_headless && y < 8) continue;                            /* chunk 38: the head is gone (and the long hair with it, see cell_at) */
            int dy = y;
            if (y == 11 && lift >= 0 && ((x < 5) == (lift == 0))) dy = 10;   /* raised foot */
            switch (c) {
            case 'h': use(r, p->hair);  break;
            case 's': use(r, p->skin);  break;
            case 'c': if (g_headless && y == 8) setc(r, 150, 16, 28); else use(r, p->shirt); break;     /* (the cut across the neck) */
            case 'p': use(r, p->pants); break;
            case 'b': use(r, p->boots); break;
            }
            int dx = flip ? SPR_W - 1 - x : x;
            fill(r, ox + dx * s, oy + dy * s, s, s);
        }

    /* big eyes: 2x3 dark blocks with a 1px highlight, rows 4-6 of the head */
    if (facing != FACE_UP && !g_headless) {
        int ex[2], n = 0;
        if (facing == FACE_DOWN) { ex[n++] = 2; ex[n++] = 6; }
        else                     { ex[n++] = flip ? 2 : 6; }
        int shut = g_shut || eyes_shut(p);
        int worry = g_worried || (p->acc & ACC_WORRIED);
        for (int i = 0; i < n; i++) {
            if (g_aim) {                                                                    /* chunk 56: aiming at the sky. the left eye squints shut, the right one is open, the pupil right at its top */
                if (i == 0) { setc(r, 28, 22, 40); fill(r, ox + ex[i] * s, oy + 5 * s, 2 * s, s); }
                else {
                    setc(r, 236, 238, 246); fill(r, ox + ex[i] * s, oy + 4 * s, 2 * s, 3 * s);          /* the white of the eye ... */
                    setc(r, 28, 22, 40);    fill(r, ox + ex[i] * s, oy + 4 * s, 2 * s, s);               /* ... and the pupil up at the top */
                }
                continue;
            }
            setc(r, 28, 22, 40);
            if (shut) { fill(r, ox + ex[i] * s, oy + 5 * s, 2 * s, s); continue; }       /* the lid: a thin line where the eye was */
            fill(r, ox + ex[i] * s, oy + 4 * s, 2 * s, 3 * s);
            setc(r, 255, 255, 255);
            int hx = (facing == FACE_DOWN) ? ex[i] : (flip ? ex[i] + 1 : ex[i]);
            if (worry) {                                                                    /* chunk 52: the lid covers the upper half (row 4 and the top of row 5) of the eye, the glint drops to the lower half */
                int lid = s + s / 2;
                use(r, p->skin); fill(r, ox + ex[i] * s, oy + 4 * s, 2 * s, lid);
                setc(r, 255, 255, 255); fill(r, ox + hx * s, oy + 5 * s + s / 2, s, s / 2 > 0 ? s / 2 : 1);
            } else
                fill(r, ox + hx * s, oy + 4 * s, s, s);
        }
    }

    /* white shades over the eyes (rows 4-5): one wide bar from the front, one lens + temple from the side */
    if ((p->acc & ACC_SHADES) && facing != FACE_UP && !g_headless) {
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
    if ((p->acc & ACC_HALO) && !g_headless) draw_halo(r, ox, oy, s);
}

void char_draw(SDL_Renderer *r, const Person *p, int x, int y,
               int facing, int moving, float walk, int s) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);                    /* shadow */
    fill(r, x - 4 * s, y - s, 8 * s, 2 * s);
    char_draw_air(r, p, x, y, facing, moving, walk, s);
}

/* chunk 56: the aiming pose. the normal front sprite with the eyes of the pose (g_aim, see draw_sprite), then the right arm raised up the side of the head
 * (sleeve, hand) and the hammer held over it: a brown handle going up past the top of the head and a grey head, its top end wobbling a cell left and right.
 * cells are in sprite units from the sprite's top-left; rows above 0 are over the head. */
int char_debug_aims(void) { return n_aims; }
static int g_aim_hammer = 1;                                                 /* chunk 57: 0 = the hand is up and empty (the hammer has been thrown) */
void char_set_aim_hammer(int on) { g_aim_hammer = on != 0; }
void char_draw_aim(SDL_Renderer *r, const Person *p, int x, int y, int s, float t) {
    int ox = x - (SPR_W / 2) * s, oy = y - SPR_H * s;
    n_aims++;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);                                  /* shadow */
    fill(r, x - 4 * s, y - s, 8 * s, 2 * s);
    g_aim = 1;
    draw_sprite(r, p, FACE_DOWN, ox, oy, s, SPR_H, -1);
    g_aim = 0;
    float ph = sinf(t * 13.0f);
    int wob = ph > 0.35f ? 1 : ph < -0.35f ? -1 : 0;                         /* a small wobble: the top of the hammer sways */
#define CELL(cx, cy, w_, h_) fill(r, ox + (cx) * s, oy + (cy) * s, (w_) * s, (h_) * s)
    use(r, p->shirt);                                                        /* the sleeve: over the old hand, then up along the head */
    CELL(8, 9, 1, 1); CELL(9, 9, 1, 1); CELL(9, 8, 1, 1);
    use(r, p->skin);  CELL(9, 7, 1, 1);                                      /* the hand */
    if (!g_aim_hammer) return;                                               /* chunk 57: thrown: nothing in the hand */
    setc(r, 150, 100, 56); CELL(9, 0, 1, 7);                                 /* the handle, from the hand up over the head ... */
    CELL(9 + wob, -2, 1, 2);                                                 /* ... the top of it swaying */
    setc(r, 120, 78, 42);  CELL(9, 2, 1, 5);                                 /* a darker grip under the hand */
    setc(r, 204, 210, 228); CELL(7 + wob, -4, 5, 1);                         /* the hammer's head: a light top ... */
    setc(r, 142, 148, 174); CELL(7 + wob, -3, 5, 1);                         /* ... and a shaded face */
    setc(r, 98, 104, 130);  CELL(7 + wob, -3, 1, 1); CELL(11 + wob, -3, 1, 1);
#undef CELL
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
            if (g_headless && j < 8) continue;                                  /* chunk 38 */
            switch (ch) {
            case 'h': use(r, p->hair);  break;
            case 's': use(r, p->skin);  break;
            case 'c': if (g_headless && j == 8) setc(r, 150, 16, 28); else use(r, p->shirt); break;
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

/* lying face-down after the fall, seen from the side: head to the left, eyes shut, the normal sprite (arms at the sides). the whole
 * sprite is turned a quarter round about the feet. feet at (x, y). */
void char_draw_lying(SDL_Renderer *r, const Person *p, int x, int y, int s) {
    const float Q = 1.5708f;
    float ca = cosf(-Q), sa = sinf(-Q), shx = 6.0f * s, shy = -4.5f * s;      /* centred over (x, y), resting on the ground line */
    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);
    fill(r, x - (int)(6.5f * s), y - s, 13 * s, 4 * s);                         /* the shadow: a long strip */
    int sz = (int)(s * 1.3f + 0.5f); if (sz < s + 1) sz = s + 1;                /* a rotated grid of squares needs them a little big to leave no gaps */
#define LY_PT(cx, cy, ox, oy) do { float cx_ = (cx), cy_ = (cy); (ox) = shx / s + cx_ * ca - cy_ * sa; (oy) = shy / s + cx_ * sa + cy_ * ca; } while (0)
#define LY_SQ(px_, py_) fill(r, x + (int)((px_) * s) - sz / 2, y + (int)((py_) * s) - sz / 2, sz, sz)
    for (int j = 0; j < SPR_H; j++)
        for (int i = 0; i < SPR_W; i++) {
            char ch = cell_at(p, FACE_LEFT, i, j);
            if (ch == '.') continue;
            switch (ch) {
            case 'h': use(r, p->hair);  break;
            case 's': use(r, p->skin);  break;
            case 'c': use(r, p->shirt); break;
            case 'p': use(r, p->pants); break;
            case 'b': use(r, p->boots); break;
            }
            float qx, qy; LY_PT((SPR_W - 1 - i) - 5 + 0.5f, j - SPR_H + 0.5f, qx, qy); LY_SQ(qx, qy);
        }
    SDL_SetRenderDrawColor(r, 28, 22, 40, 255);                                 /* the eye, on the side facing the ground: shut */
    for (int ex = 2; ex <= 3; ex++) { float qx, qy; LY_PT(ex - 5 + 0.5f, 5 - SPR_H + 0.5f, qx, qy); LY_SQ(qx, qy); }
#undef LY_PT
#undef LY_SQ
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

/* chunk 38: just the head, front view, eyes shut, a dark red cut at the bottom; centred on (x, y) */
void char_draw_head(SDL_Renderer *r, const Person *p, int x, int y, int s) {
    int ox = x - (SPR_W / 2) * s, oy = y - 4 * s;
    g_shut = 1;
    draw_sprite(r, p, FACE_DOWN, ox, oy, s, 8, -1);
    g_shut = 0;
    setc(r, 150, 16, 28); fill(r, ox + 2 * s, oy + 7 * s, 6 * s, s);          /* the neck, cut */
}
