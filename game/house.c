/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "house.h"
#include <math.h>

/* drawing in sprite pixels ("units"): 8 per tile. (OX, OY) is where unit (0, 0) lands on screen, P is screen px per unit */
static SDL_Renderer *R;
static int OX, OY, P;
static void rc(int x, int y, int w, int h, int r, int g, int b, int a) {
    SDL_SetRenderDrawColor(R, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
    SDL_Rect q = { OX + x * P, OY + y * P, w * P, h * P };
    SDL_RenderFillRect(R, &q);
}
static unsigned hv(int a, int b) { unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }

/* ================= THE HUT ================= */
static void window(int x, int y, int shutter_left) {                 /* glass is 10 x 10 at (x, y) */
    rc(x - 1, y - 1, 12, 12, 96, 60, 34, 255);                       /* frame */
    rc(x, y, 10, 10, 150, 204, 232, 255);                            /* glass */
    rc(x, y, 10, 3, 178, 226, 246, 255);
    rc(x, y + 7, 10, 3, 238, 218, 150, 255);                         /* the warm light of the room below */
    rc(x + 4, y, 2, 10, 96, 60, 34, 255); rc(x, y + 4, 10, 2, 96, 60, 34, 255);   /* muntins */
    rc(x + 1, y + 1, 2, 1, 244, 252, 255, 255); rc(x + 1, y + 2, 1, 1, 244, 252, 255, 255);   /* glint */
    int sx = shutter_left ? x - 4 : x + 11;                          /* one shutter, on the outer side */
    rc(sx, y - 1, 3, 12, 70, 116, 94, 255);
    for (int k = 2; k < 11; k += 3) rc(sx, y + k - 1, 3, 1, 48, 86, 68, 255);
    rc(sx, y - 1, 1, 12, 92, 142, 114, 255);
    rc(x - 2, y + 10, 14, 2, 124, 80, 46, 255);                      /* sill */
    rc(x - 1, y + 12, 12, 3, 112, 70, 40, 255);                      /* flower box */
    rc(x - 1, y + 11, 12, 2, 58, 128, 66, 255);
    static const int fl[6][3] = { {0,10,0},{2,9,1},{4,10,2},{6,9,0},{8,10,1},{10,9,2} };
    for (int i = 0; i < 6; i++) {
        int c = fl[i][2];
        rc(x - 1 + fl[i][0], y + fl[i][1], 2, 2, c == 0 ? 236 : c == 1 ? 250 : 246, c == 0 ? 96 : c == 1 ? 214 : 150, c == 0 ? 120 : c == 1 ? 90 : 190, 255);
    }
}
static void bush(int x, int y, int mirror) {
    rc(x, y + 2, 9, 6, 38, 104, 50, 255);
    rc(x + 1, y, 7, 4, 52, 128, 62, 255);
    rc(x + 2, y + 1, 3, 2, 78, 158, 84, 255);
    rc(x + (mirror ? 6 : 2), y + 4, 1, 1, 220, 60, 70, 255); rc(x + (mirror ? 3 : 6), y + 3, 1, 1, 220, 60, 70, 255);
    rc(x, y + 7, 9, 1, 28, 80, 40, 255);
}

void house_draw_outside(SDL_Renderer *r, int ox, int oy, int px, float t) {
    R = r; OX = ox; OY = oy; P = px;
    rc(-1, 44, 60, 5, 0, 0, 0, 70);                                  /* the ground shadow */
    for (int k = 0; k < 6; k++) {                                    /* log wall, six logs */
        int y = 24 + 3 * k, odd = k & 1;
        rc(3, y, 50, 3, odd ? 166 : 178, odd ? 112 : 124, odd ? 66 : 74, 255);
        rc(3, y, 50, 1, 200, 148, 94, 255);
        rc(3, y + 2, 50, 1, 118, 76, 44, 255);
        int lx = odd ? 0 : 1, rx = odd ? 52 : 51;                    /* the log ends stick out at the corners, alternating */
        rc(lx, y, 4, 3, 208, 158, 102, 255); rc(lx + 1, y + 1, 2, 1, 150, 100, 58, 255);
        rc(rx, y, 4, 3, 208, 158, 102, 255); rc(rx + 1, y + 1, 2, 1, 150, 100, 58, 255);
    }
    rc(2, 42, 52, 4, 128, 126, 130, 255);                            /* stone foundation */
    rc(2, 42, 52, 1, 166, 164, 168, 255); rc(2, 44, 52, 1, 98, 96, 102, 255);
    for (int x = 6; x < 54; x += 9) rc(x, 42, 1, 2, 98, 96, 102, 255);
    for (int x = 10; x < 54; x += 9) rc(x, 44, 1, 2, 98, 96, 102, 255);

    window(10, 27, 1);                                               /* the windows, one each side of the door */
    window(36, 27, 0);

    rc(23, 28, 10, 18, 96, 60, 34, 255);                             /* the door: frame, planks, hinges, knob, step */
    rc(24, 29, 8, 17, 142, 94, 54, 255);
    rc(24, 29, 8, 1, 112, 72, 42, 255);
    for (int x = 26; x < 32; x += 2) rc(x, 30, 1, 16, 112, 72, 42, 255);
    rc(24, 33, 4, 1, 58, 58, 64, 255); rc(24, 41, 4, 1, 58, 58, 64, 255);
    rc(30, 38, 2, 2, 238, 198, 86, 255);
    rc(22, 46, 12, 2, 156, 154, 156, 255); rc(22, 47, 12, 1, 112, 110, 114, 255);
    int fl = 40 + (int)(sinf(t * 7.0f) * 6.0f);                      /* a porch lamp above the door, flickering a little */
    rc(24, 24, 8, 7, 255, 210, 90, fl);
    rc(27, 26, 2, 3, 255, 226, 130, 255); rc(26, 25, 4, 1, 70, 52, 40, 255);

    for (int y = 2; y < 22; y++) {                                   /* the roof: a trapezoid of shingles, widening toward the eaves */
        int lx = 8 - (y - 2) / 2, rx = 48 + (y - 2) / 2, k = (y - 2) / 3, dk = (y - 2) % 3;
        rc(lx, y, rx - lx, 1, dk == 0 ? 178 : dk == 1 ? 154 : 120, dk == 0 ? 94 : dk == 1 ? 72 : 52, dk == 0 ? 68 : dk == 1 ? 54 : 42, 255);
        if (dk > 0) for (int x = lx + ((k & 1) ? 3 : 0); x < rx; x += 6) rc(x, y, 1, 1, 96, 40, 32, 255);
        rc(lx, y, 1, 1, 84, 34, 28, 255); rc(rx - 1, y, 1, 1, 84, 34, 28, 255);
    }
    rc(8, 0, 40, 3, 200, 110, 80, 255); rc(8, 0, 40, 1, 224, 142, 106, 255);   /* the ridge */
    rc(-2, 22, 60, 3, 92, 38, 32, 255); rc(-2, 22, 60, 1, 128, 56, 46, 255);   /* the eave */
    rc(3, 25, 50, 2, 0, 0, 0, 60);                                   /* its shadow on the wall */

    rc(37, -5, 7, 16, 132, 128, 134, 255);                           /* the chimney */
    for (int y = -3; y < 10; y += 3) rc(37, y, 7, 1, 102, 98, 106, 255);
    rc(40, -5, 1, 3, 102, 98, 106, 255); rc(41, -2, 1, 3, 102, 98, 106, 255); rc(39, 4, 1, 3, 102, 98, 106, 255);
    rc(43, -5, 1, 16, 100, 96, 104, 255);
    rc(36, -7, 9, 3, 92, 90, 98, 255); rc(36, -7, 9, 1, 128, 126, 136, 255);
    for (int i = 0; i < 4; i++) {                                    /* smoke */
        float ph = fmodf(t * 0.5f + i * 0.25f, 1.0f);
        int sz = 2 + (int)(ph * 4), xx = 39 + (int)(sinf(ph * 5.0f + i * 1.7f) * 3.0f), yy = -9 - (int)(ph * 16.0f);
        rc(xx, yy, sz, sz, 228, 228, 236, (int)(170.0f * (1.0f - ph)));
    }

    bush(-4, 39, 0);                                                 /* a bush at each corner */
    bush(52, 39, 1);
}

/* ================= THE ROOM ================= */
/* furniture blocks the tiles it stands on */
int house_inside_solid(int tx, int ty) {
    if (tx >= 1 && tx <= 2 && ty >= 2 && ty <= 4) return 1;         /* bed */
    if (tx == 3 && ty == 2) return 1;                                /* nightstand */
    if ((tx == 6 || tx == 7) && ty == 2) return 1;                   /* bookshelf */
    if ((tx == 9 || tx == 10) && ty == 2) return 1;                  /* fireplace */
    if (tx >= 8 && tx <= 9 && ty >= 5 && ty <= 6) return 1;          /* table */
    if (tx == 7 && ty == 5) return 1;                                /* chair */
    if (tx == 10 && (ty == 7 || ty == 8)) return 1;                  /* barrel, crates */
    if (tx == 1 && ty == 8) return 1;                                /* plant */
    return 0;
}

void house_draw_floor_tile(SDL_Renderer *r, int tx, int ty, int x, int y, int px) {
    R = r; OX = x; OY = y; P = px;
    unsigned h = hv(tx, ty);
    int v = (int)(h & 3) * 3;
    rc(0, 0, 8, 8, 192 + v, 144 + v, 92 + v, 255);
    rc(0, 3, 8, 1, 150, 102, 62, 255); rc(0, 7, 8, 1, 150, 102, 62, 255);   /* two rows of planks per tile */
    rc((int)((h >> 3) & 7), 0, 1, 3, 150, 102, 62, 255);                    /* butt joints, staggered */
    rc((int)((h >> 7) & 7), 4, 1, 3, 150, 102, 62, 255);
    rc(0, 0, 8, 1, 206, 160, 106, 90);
    if (ty == 2) rc(0, 0, 8, 3, 0, 0, 0, 56);                               /* the back wall's shadow on the floor */
    if (tx == 1) rc(0, 0, 2, 8, 0, 0, 0, 38);
    if (tx == 10) rc(6, 0, 2, 8, 0, 0, 0, 38);
}

void house_draw_wall_tile(SDL_Renderer *r, int tx, int ty, int mw, int x, int y, int px) {
    R = r; OX = x; OY = y; P = px;
    if (ty <= 1 && tx > 0 && tx < mw - 1) {                                 /* the back wall, seen from the front: vertical planks */
        int odd = tx & 1;
        rc(0, 0, 8, 8, odd ? 170 : 180, odd ? 120 : 130, odd ? 72 : 80, 255);
        rc(0, 0, 1, 8, 124, 82, 48, 255); rc(4, 0, 1, 8, 138, 94, 56, 255);
        if (ty == 0) { rc(0, 0, 8, 2, 108, 68, 40, 255); rc(0, 2, 8, 1, 214, 168, 112, 255); }   /* crown trim */
        if (ty == 1) { rc(0, 5, 8, 3, 112, 72, 42, 255); rc(0, 5, 8, 1, 214, 168, 112, 255); }   /* baseboard */
        return;
    }
    rc(0, 0, 8, 8, 88, 58, 34, 255);                                        /* the dark outer wall */
    if (tx == 0)           rc(6, 0, 2, 8, 132, 90, 54, 255);
    else if (tx == mw - 1) rc(0, 0, 2, 8, 132, 90, 54, 255);
    else                   rc(0, 0, 8, 2, 132, 90, 54, 255);
}

/* ---- the pieces. each draws with (OX, OY) = screen px of the room's tile (0, 0); coordinates are in units of 1/8 tile ---- */
static float TT;

static void p_rug(void) {                                                   /* tiles 3..6 x 5..7 */
    rc(24, 40, 32, 24, 140, 48, 44, 255);
    rc(26, 42, 28, 20, 198, 152, 98, 255);
    rc(26, 42, 28, 2, 232, 208, 162, 255); rc(26, 60, 28, 2, 232, 208, 162, 255);
    rc(26, 44, 2, 16, 232, 208, 162, 255); rc(52, 44, 2, 16, 232, 208, 162, 255);
    rc(34, 46, 12, 8, 140, 48, 44, 255); rc(36, 48, 8, 4, 232, 196, 136, 255); rc(38, 49, 4, 2, 140, 48, 44, 255);
    for (int y = 42; y < 62; y += 2) { rc(23, y, 1, 1, 226, 206, 170, 255); rc(56, y, 1, 1, 226, 206, 170, 255); }   /* fringe */
}
static void p_mat(void) {                                                   /* in front of the door */
    rc(48, 66, 8, 5, 150, 52, 44, 255); rc(49, 67, 6, 3, 208, 178, 122, 255); rc(50, 68, 4, 1, 150, 52, 44, 255);
}
static void p_door(void) {                                                  /* the doorway in the bottom wall, light outside */
    rc(47, 70, 10, 10, 96, 60, 36, 255);
    rc(48, 72, 8, 8, 196, 228, 176, 255);
    rc(48, 72, 8, 3, 216, 238, 248, 255); rc(48, 77, 8, 3, 122, 186, 102, 255);
    rc(48, 72, 2, 8, 142, 94, 54, 255);
    rc(48, 64, 8, 6, 255, 244, 190, 26);                                    /* daylight spilling in */
}
static void p_window(void) {                                                /* tiles 4..5, on the back wall */
    rc(32, 2, 16, 13, 120, 78, 46, 255);
    rc(34, 4, 12, 9, 150, 208, 240, 255); rc(34, 4, 12, 3, 182, 230, 248, 255);
    rc(39, 4, 2, 9, 120, 78, 46, 255); rc(34, 8, 12, 1, 120, 78, 46, 255);
    rc(35, 5, 2, 1, 246, 252, 255, 255);
    rc(30, 13, 20, 2, 156, 106, 62, 255);                                   /* sill */
    rc(32, 3, 4, 10, 224, 170, 150, 255); rc(44, 3, 4, 10, 224, 170, 150, 255);   /* curtains */
    for (int y = 4; y < 13; y += 3) { rc(33, y, 1, 2, 196, 140, 122, 255); rc(46, y, 1, 2, 196, 140, 122, 255); }
    rc(31, 2, 18, 1, 100, 64, 38, 255);                                     /* rod */
    rc(33, 17, 14, 11, 255, 240, 190, 24);                                  /* a patch of light on the floor */
}
static void p_painting(void) {                                              /* a framed landscape between shelf and fireplace */
    rc(65, 4, 7, 8, 112, 72, 42, 255);
    rc(66, 5, 5, 6, 150, 206, 238, 255); rc(66, 8, 5, 3, 96, 160, 90, 255); rc(68, 7, 2, 1, 70, 130, 70, 255);
    rc(66, 5, 1, 1, 255, 244, 190, 255);
}
static void p_bookshelf(void) {                                             /* tiles 6..7, row 2 */
    rc(48, 3, 16, 21, 108, 68, 40, 255);
    rc(49, 4, 14, 19, 72, 46, 30, 255);
    static const int cols[7][3] = { {196,72,64},{70,110,170},{222,186,84},{84,150,96},{150,86,160},{226,226,214},{196,120,64} };
    for (int s = 0; s < 3; s++) {
        int y = 4 + s * 6;
        rc(49, y + 5, 14, 1, 128, 84, 50, 255);
        int x = 50;
        for (int b = 0; b < 6 && x < 62; b++) {
            unsigned h = hv(b, s + 11); int bw = 2 + (int)(h & 1), bh = 3 + (int)((h >> 1) & 1);
            const int *c = cols[(h >> 3) % 7];
            rc(x, y + 5 - bh, bw, bh, c[0], c[1], c[2], 255); rc(x, y + 5 - bh, 1, bh, c[0] * 3 / 4, c[1] * 3 / 4, c[2] * 3 / 4, 255);
            x += bw;
        }
    }
    rc(52, 20, 3, 2, 226, 220, 200, 255); rc(53, 19, 1, 1, 255, 210, 90, 255);   /* a candle stub on the low shelf */
    rc(48, 1, 16, 2, 126, 82, 48, 255);
}
static void p_fireplace(void) {                                             /* tiles 9..10, row 2, with its chimney breast up the wall */
    rc(74, 2, 12, 12, 128, 124, 130, 255);                                  /* the breast */
    for (int y = 3; y < 14; y += 3) rc(74, y, 12, 1, 98, 94, 102, 255);
    rc(77, 2, 1, 3, 98, 94, 102, 255); rc(81, 5, 1, 3, 98, 94, 102, 255); rc(79, 8, 1, 3, 98, 94, 102, 255);
    rc(72, 13, 16, 3, 96, 62, 38, 255); rc(72, 13, 16, 1, 150, 104, 64, 255);   /* mantel */
    rc(74, 10, 2, 3, 226, 220, 200, 255); rc(84, 11, 2, 2, 226, 220, 200, 255);  /* candle and a jar */
    rc(75, 9, 1, 1, 255, 210, 90, 255);
    rc(72, 16, 16, 8, 132, 128, 134, 255);                                  /* the hearth */
    for (int y = 17; y < 24; y += 3) rc(72, y, 16, 1, 100, 96, 104, 255);
    rc(75, 17, 10, 7, 28, 22, 24, 255);                                     /* the opening */
    rc(76, 22, 8, 2, 104, 66, 38, 255); rc(77, 21, 6, 1, 130, 84, 48, 255); /* logs */
    for (int i = 0; i < 4; i++) {                                           /* flames */
        float f = sinf(TT * (9.0f + i * 1.7f) + i * 2.1f) * 0.5f + 0.5f;
        int x = 76 + i * 2, hgt = 3 + (int)(f * 3.0f);
        rc(x, 22 - hgt, 2, hgt, 255, 150, 40, 255); rc(x, 22 - hgt + 1, 2, hgt - 1, 255, 206, 80, 255);
        if (hgt > 4) rc(x, 22 - hgt, 1, 1, 255, 246, 170, 255);
    }
    int g = 22 + (int)(sinf(TT * 11.0f) * 6.0f);                            /* the firelight on the room */
    rc(64, 12, 32, 30, 255, 150, 60, g);
}
static void p_nightstand(void) {                                            /* tile (3, 2) with its lamp */
    rc(24, 16, 8, 8, 150, 100, 60, 255); rc(24, 16, 8, 1, 192, 140, 90, 255);
    rc(25, 19, 6, 1, 112, 72, 42, 255); rc(27, 20, 2, 1, 226, 196, 96, 255);
    rc(24, 23, 8, 1, 100, 64, 38, 255);
    rc(27, 13, 2, 3, 214, 180, 90, 255);
    rc(25, 9, 6, 5, 252, 226, 150, 255); rc(26, 8, 4, 1, 252, 226, 150, 255); rc(25, 13, 6, 1, 214, 180, 100, 255);
    rc(19, 6, 18, 18, 255, 220, 120, 34);                                   /* the lamp's glow */
}
static void p_bed(void) {                                                   /* tiles 1..2 x 2..4 */
    rc(8, 12, 16, 7, 112, 70, 40, 255); rc(8, 12, 16, 1, 170, 120, 76, 255);   /* headboard, taller than the bed */
    rc(9, 14, 14, 3, 132, 88, 52, 255);
    rc(8, 18, 16, 22, 120, 78, 46, 255);                                    /* the frame */
    rc(9, 19, 14, 19, 240, 232, 214, 255);                                  /* the mattress */
    rc(10, 20, 12, 5, 250, 246, 236, 255); rc(10, 24, 12, 1, 222, 214, 196, 255);   /* the pillow */
    rc(9, 27, 14, 12, 62, 130, 148, 255);                                   /* the quilt */
    for (int y = 28; y < 38; y += 2) for (int x = 9 + ((y >> 1) & 1) * 2; x < 23; x += 4) rc(x, y, 2, 2, 86, 156, 172, 255);
    rc(9, 26, 14, 2, 234, 226, 204, 255);                                   /* the fold */
    rc(8, 38, 16, 3, 112, 70, 40, 255); rc(8, 38, 16, 1, 170, 120, 76, 255); /* the footboard */
    rc(8, 40, 2, 1, 70, 44, 26, 255); rc(22, 40, 2, 1, 70, 44, 26, 255);
}
static void p_table(void) {                                                 /* tiles 8..9 x 5..6 */
    rc(64, 42, 16, 9, 190, 140, 86, 255); rc(64, 42, 16, 1, 222, 176, 118, 255);   /* top */
    rc(64, 50, 16, 3, 132, 88, 52, 255);                                    /* apron */
    rc(65, 53, 2, 3, 104, 66, 38, 255); rc(77, 53, 2, 3, 104, 66, 38, 255);  /* legs */
    rc(68, 42, 8, 9, 176, 72, 60, 255); rc(68, 42, 8, 1, 206, 104, 88, 255);   /* a runner */
    rc(66, 44, 4, 3, 226, 226, 232, 255); rc(70, 45, 2, 2, 226, 226, 232, 255); rc(65, 45, 1, 2, 226, 226, 232, 255);   /* teapot, cup, spout */
    rc(73, 44, 4, 3, 214, 168, 96, 255); rc(74, 43, 2, 1, 236, 200, 130, 255);  /* a loaf on a plate */
    rc(78, 41, 2, 3, 90, 150, 90, 255); rc(78, 40, 1, 1, 236, 96, 120, 255);    /* a flower in a jar */
}
static void p_chair(void) {                                                 /* tile (7, 5), turned toward the table */
    rc(57, 39, 2, 11, 112, 72, 42, 255); rc(57, 39, 2, 1, 170, 120, 76, 255);
    rc(57, 46, 7, 3, 160, 108, 64, 255); rc(57, 46, 7, 1, 196, 148, 96, 255);
    rc(58, 49, 1, 4, 100, 64, 38, 255); rc(62, 49, 1, 4, 100, 64, 38, 255);
}
static void p_barrel(void) {                                                /* tile (10, 7) */
    rc(81, 52, 7, 11, 138, 92, 54, 255); rc(82, 51, 5, 1, 138, 92, 54, 255); rc(82, 63, 5, 1, 100, 64, 38, 255);
    rc(81, 54, 7, 1, 80, 80, 88, 255); rc(81, 60, 7, 1, 80, 80, 88, 255);
    rc(84, 52, 1, 11, 108, 70, 42, 255); rc(82, 52, 1, 11, 168, 118, 72, 255);
}
static void p_crates(void) {                                                /* tile (10, 8) */
    rc(80, 65, 8, 7, 164, 114, 66, 255); rc(80, 65, 8, 1, 200, 152, 98, 255);
    rc(80, 65, 1, 7, 116, 76, 44, 255); rc(87, 65, 1, 7, 116, 76, 44, 255); rc(80, 71, 8, 1, 116, 76, 44, 255);
    for (int i = 1; i < 7; i++) { rc(80 + i, 65 + i, 1, 1, 116, 76, 44, 255); rc(87 - i, 65 + i, 1, 1, 116, 76, 44, 255); }
    rc(81, 62, 5, 3, 206, 188, 140, 255); rc(82, 61, 3, 1, 206, 188, 140, 255);   /* a sack on top */
}
static void p_plant(void) {                                                 /* tile (1, 8) */
    rc(9, 67, 7, 5, 178, 94, 66, 255); rc(9, 67, 7, 1, 214, 130, 98, 255); rc(10, 71, 5, 1, 120, 60, 44, 255);
    rc(11, 56, 3, 11, 54, 128, 62, 255);
    rc(8, 58, 4, 3, 74, 156, 82, 255); rc(13, 56, 4, 3, 74, 156, 82, 255); rc(9, 62, 3, 2, 54, 128, 62, 255); rc(13, 62, 4, 3, 74, 156, 82, 255);
    rc(10, 55, 2, 2, 98, 180, 104, 255);
}

typedef struct { float foot; void (*fn)(void); } Piece;
static const Piece PIECES[] = {
    { 0.0f, p_rug }, { 0.1f, p_mat }, { 0.2f, p_window }, { 0.3f, p_painting }, { 0.4f, p_door },
    { 3.0f, p_nightstand }, { 3.0f, p_bookshelf }, { 3.0f, p_fireplace },
    { 5.0f, p_bed }, { 6.5f, p_chair }, { 7.0f, p_table }, { 8.0f, p_barrel }, { 9.0f, p_crates }, { 9.0f, p_plant },
};
int   house_piece_count(void) { return (int)(sizeof PIECES / sizeof PIECES[0]); }
float house_piece_foot(int i) { return PIECES[i].foot; }
void  house_piece_draw(SDL_Renderer *r, int i, int ox, int oy, int px, float t) {
    R = r; OX = ox; OY = oy; P = px; TT = t;
    PIECES[i].fn();
}
