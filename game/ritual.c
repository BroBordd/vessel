/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "ritual.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

#define GN   41                      /* the grid of cells: -20 .. 20 round the centre */
#define GO   20
#define BODY_X  (-26)                /* the dead vessel lies to the left of the circle, head to the left: his feet at (BODY_X, BODY_Y). the start room is 9 tiles wide at least (gen.c): his head is 32 cells out, the wall 36 */
#define BODY_Y  5
#define SRC_X   (-20)                /* the groove that carries his blood to the ring (2 cells high), its source under him */
#define SRC_Y   2

static uint8_t carve[GN][GN];
static int16_t dist[GN][GN];
static int     maxd, built, ncarve;
static int16_t carve_x[GN * GN], carve_y[GN * GN];

static int   on, active, chime, chimed;
static float st;
static float cx_t, cy_t;
static const Person *dead_who;

static uint32_t hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 374761393u + b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

/* ---------- the carving, built once ---------- */
static void put(int x, int y) { if (x >= -GO && x <= GO && y >= -GO && y <= GO) carve[y + GO][x + GO] = 1; }
static float ell(int x, int y) { return sqrtf((float)(x * x + 4 * y * y)); }               /* distance on the flat circle: y counts double */
static void ring(float R, float w) {
    for (int y = -GO; y <= GO; y++) for (int x = -GO; x <= GO; x++) if (fabsf(ell(x, y) - R) < w) put(x, y);
}
static void line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        put(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
static void build(void) {
    if (built) return;
    built = 1;
    memset(carve, 0, sizeof carve);
    ring(RITUAL_RADIUS, 0.85f);                                   /* the outer ring */
    ring(9.5f, 0.80f);                                            /* the inner ring */
    int vx[6], vy[6], ox[6], oy[6];
    for (int k = 0; k < 6; k++) {
        float a = (-90.0f + 60.0f * k) * 3.14159265f / 180.0f;
        vx[k] = (int)lroundf(9.5f * cosf(a)); vy[k] = (int)lroundf(4.75f * sinf(a));
        ox[k] = (int)lroundf(14.3f * cosf(a)); oy[k] = (int)lroundf(7.15f * sinf(a));
    }
    for (int k = 0; k < 6; k++) { line(vx[k], vy[k], vx[(k + 2) % 6], vy[(k + 2) % 6]); line(vx[k], vy[k], ox[k], oy[k]); }   /* the hexagram, and a spoke from every point out to the ring */
    for (int x = SRC_X; x <= -12; x++) { put(x, SRC_Y); put(x, SRC_Y + 1); }                                                     /* the channel from the body */

    /* the way the blood runs: distance along the grooves from the source (8 neighbours) */
    static int16_t qx[GN * GN], qy[GN * GN];
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) dist[y][x] = -1;
    int h = 0, t = 0; qx[t] = SRC_X; qy[t] = SRC_Y; t++; dist[SRC_Y + GO][SRC_X + GO] = 0; maxd = 0;
    while (h < t) {
        int x = qx[h], y = qy[h]; h++;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
            int nx = x + i, ny = y + j;
            if (nx < -GO || nx > GO || ny < -GO || ny > GO || !carve[ny + GO][nx + GO] || dist[ny + GO][nx + GO] >= 0) continue;
            dist[ny + GO][nx + GO] = (int16_t)(dist[y + GO][x + GO] + 1);
            if (dist[ny + GO][nx + GO] > maxd) maxd = dist[ny + GO][nx + GO];
            qx[t] = (int16_t)nx; qy[t] = (int16_t)ny; t++;
        }
    }
    ncarve = 0;
    for (int y = -GO; y <= GO; y++) for (int x = -GO; x <= GO; x++) if (carve[y + GO][x + GO]) { if (dist[y + GO][x + GO] < 0) dist[y + GO][x + GO] = 0; carve_x[ncarve] = (int16_t)x; carve_y[ncarve] = (int16_t)y; ncarve++; }
}

/* ---------- the clock ---------- */
static float flow_end(void) { return RITUAL_FLOW_AT + (float)maxd / RITUAL_FLOW_V; }
static float charge_at(void) { return flow_end() + 0.45f; }
static float flash_at(void)  { return charge_at() + RITUAL_CHARGE_T; }
static float form_at(void)   { return flash_at() + 0.10f; }
static float end_at(void)    { return form_at() + RITUAL_FORM_T + RITUAL_DISS_T + RITUAL_HOLD_T; }

void  ritual_begin(float tile_x, float tile_y, const Person *dead) { build(); on = 1; active = 1; chime = chimed = 0; st = 0; cx_t = tile_x; cy_t = tile_y; dead_who = dead; }
void  ritual_cancel(void) { on = 0; active = 0; chime = 0; }
int   ritual_scene(void) { return on; }
int   ritual_active(void) { return active; }
float ritual_time(void) { return st; }
void  ritual_centre(float *tx, float *ty) { *tx = cx_t; *ty = cy_t; }
int   ritual_take_chime(void) { int c = chime; chime = 0; return c; }
int   ritual_update(float dt) {
    if (!on) return 0;
    st += dt;
    if (active && !chimed && st >= charge_at()) { chimed = 1; chime = 1; }
    if (active && st >= end_at()) { active = 0; return 1; }
    return 0;
}
int ritual_player_rows(void) {
    if (!active) return 12;
    float u = (st - form_at()) / RITUAL_FORM_T;
    if (u <= 0) return 0;
    int rows = (int)(u * 12.0f) + 1; return rows > 12 ? 12 : rows;
}

/* ---------- drawing ---------- */
static void cell_at(SDL_Renderer *r, int sx, int sy, int cell, int x, int y, int R, int G, int B, int A) {
    SDL_SetRenderDrawColor(r, (Uint8)(R < 0 ? 0 : R > 255 ? 255 : R), (Uint8)(G < 0 ? 0 : G > 255 ? 255 : G), (Uint8)(B < 0 ? 0 : B > 255 ? 255 : B), (Uint8)(A < 0 ? 0 : A > 255 ? 255 : A));
    SDL_Rect q = { sx + x * cell, sy + y * cell, cell, cell };
    SDL_RenderFillRect(r, &q);
}
static int is_carve(int x, int y) { return x >= -GO && x <= GO && y >= -GO && y <= GO && carve[y + GO][x + GO]; }
static int lerp(int a, int b, float k) { return (int)(a + (b - a) * clamp01(k)); }

static float brightness(void) {                                  /* 0 .. 1: how much the blood glows */
    float c = charge_at(), f = flash_at();
    if (st < c) return 0.0f;
    if (st < f) { float u = (st - c) / RITUAL_CHARGE_T; return 0.15f + 0.85f * u * u; }
    float after = st - f;
    return 0.22f + 0.78f * clamp01(1.0f - after / 1.4f) + 0.07f * sinf(st * 2.4f);
}

static void glow(SDL_Renderer *r, int sx, int sy, int cell, float k) {      /* the red light on the floor: three stepped discs */
    static const int rad[3] = { 24, 19, 14 }, al[3] = { 12, 14, 16 };
    for (int l = 0; l < 3; l++) {
        int a = (int)(al[l] * k); if (a <= 0) continue;
        SDL_SetRenderDrawColor(r, 200, 24, 30, (Uint8)a);
        int ry = rad[l] / 2;
        for (int j = -ry; j < ry; j++) {
            float yc = (j + 0.5f) / (float)ry, d = 1.0f - yc * yc; if (d <= 0) continue;
            int hw = (int)(rad[l] * sqrtf(d) + 0.5f);
            SDL_Rect q = { sx - hw * cell, sy + j * cell, 2 * hw * cell, cell };
            SDL_RenderFillRect(r, &q);
        }
    }
}

/* his hurts, laid over the lying body (cells from the circle's centre; his head is at x -32 .. -25, his body at -24 .. -21, his legs at -20):
 * the stab in his side with blood running out of it, cuts across the head, shoulder and leg, bruises, and blood from the mouth */
static void wounds(SDL_Renderer *r, int sx, int sy, int cell, float t) {
    static const int8_t cuts[][2] = { {-30,-4},{-29,-3},{-28,-2},{-27,-1},       /* across the back of the head */
                                      {-25,-3},{-25,-2},{-24,-1},{-24,0},        /* over the shoulder */
                                      {-21,-2},{-21,-1},{-20,0},{-20,1},         /* down the leg */
                                      {-31,3},{-30,4},{-29,4},                   /* a scratch on the cheek */
                                      {-26,3},{-25,3},{-24,4} };                 /* the neck */
    static const int8_t bruise[][2] = { {-29,-2},{-28,-1},{-28,-2},{-31,2},{-31,1},{-32,2},{-23,-3},{-22,-3},{-23,-4},{-26,0} };
    static const int8_t smear[][2] = { {-27,-3},{-31,-1},{-26,1},{-23,3},{-29,1},{-22,-2},{-28,3},{-32,0} };
    for (size_t i = 0; i < sizeof bruise / sizeof *bruise; i++) cell_at(r, sx, sy, cell, bruise[i][0], bruise[i][1], 96, 44, 78, 150);
    for (size_t i = 0; i < sizeof cuts / sizeof *cuts; i++) {
        int x = cuts[i][0], y = cuts[i][1];
        cell_at(r, sx, sy, cell, x, y + 1, 168, 28, 34, 150);                      /* the blood along the lip of the cut */
        cell_at(r, sx, sy, cell, x, y, 84, 6, 12, 255);                            /* the cut */
    }
    for (size_t i = 0; i < sizeof smear / sizeof *smear; i++) cell_at(r, sx, sy, cell, smear[i][0], smear[i][1], 130, 12, 20, 230);
    /* the stab: a dark slit, wet blood round it, and a trickle that runs down to the pool, a bright drop in it every moment */
    cell_at(r, sx, sy, cell, -24, 0, 150, 14, 22, 235); cell_at(r, sx, sy, cell, -21, 1, 150, 14, 22, 235);
    cell_at(r, sx, sy, cell, -23, -1, 150, 14, 22, 235); cell_at(r, sx, sy, cell, -22, 2, 150, 14, 22, 235);
    cell_at(r, sx, sy, cell, -23, 0, 70, 4, 10, 255); cell_at(r, sx, sy, cell, -22, 0, 70, 4, 10, 255);
    cell_at(r, sx, sy, cell, -23, 1, 70, 4, 10, 255); cell_at(r, sx, sy, cell, -22, 1, 200, 30, 36, 255);
    for (int y = 2; y <= 5; y++) cell_at(r, sx, sy, cell, -22, y, 120, 8, 16, 255);
    { int d = 2 + (int)fmodf(t * 3.0f, 4.0f); cell_at(r, sx, sy, cell, -22, d, 214, 44, 46, 255); }
    /* blood from his mouth, running down his face into the pool */
    cell_at(r, sx, sy, cell, -32, 4, 120, 8, 16, 255); cell_at(r, sx, sy, cell, -32, 5, 120, 8, 16, 255); cell_at(r, sx, sy, cell, -33, 5, 104, 9, 16, 255);
}

void ritual_draw_floor(SDL_Renderer *r, int sx, int sy, int cell, float darken) {
    if (!on) return;
    float t = st, front = (t - RITUAL_FLOW_AT) * RITUAL_FLOW_V, bright = brightness();

    /* the light on the floor under everything */
    if (bright > 0.02f) glow(r, sx, sy, cell, bright);

    /* the carving: a dark groove, a lit lip on its lower and right edges, a shadow on its upper and left ones */
    for (int y = -GO; y <= GO; y++) for (int x = -GO; x <= GO; x++) {
        if (carve[y + GO][x + GO]) { cell_at(r, sx, sy, cell, x, y, 20, 13, 16, 240); continue; }
        if (is_carve(x, y - 1) || is_carve(x - 1, y)) cell_at(r, sx, sy, cell, x, y, 200, 190, 200, 38);
        else if (is_carve(x, y + 1) || is_carve(x + 1, y)) cell_at(r, sx, sy, cell, x, y, 0, 0, 0, 60);
    }

    /* the pool under the dead vessel: it spreads slowly */
    {
        float g = clamp01(t / 12.0f);
        float prx = 3.0f + 5.0f * g, pry = 1.6f + 3.4f * g, pcx = -26.0f, pcy = 2.0f;
        for (int y = -4; y <= 8; y++) for (int x = -36; x <= -17; x++) {
            float dx = (x - pcx) / prx, dy = (y - pcy) / pry, d = dx * dx + dy * dy;
            float edge = 1.0f + ((hash((uint32_t)(x + 40), (uint32_t)(y + 40)) & 7) - 3) * 0.045f;
            if (d > edge) continue;
            int shine = (hash((uint32_t)(x + 90), (uint32_t)(y + 90)) % 13 == 0) && sinf(t * 1.7f + (float)(x * 3 + y)) > 0.75f;
            if (shine) cell_at(r, sx, sy, cell, x, y, 168, 30, 36, 235);
            else       cell_at(r, sx, sy, cell, x, y, 104, 9, 16, 235);
        }
    }

    /* the blood in the grooves: it comes up the channel and runs round the whole circle */
    for (int i = 0; i < ncarve; i++) {
        int x = carve_x[i], y = carve_y[i], d = dist[y + GO][x + GO];
        if ((float)d > front) continue;
        float s = 0.5f + 0.5f * sinf((float)d * 0.55f - t * 6.0f);
        int R = lerp(112, 182, s), G = lerp(8, 22, s), B = lerp(16, 32, s);
        if (!is_carve(x, y - 1)) { R = R * 78 / 100; G = G * 78 / 100; B = B * 78 / 100; }          /* the near wall of the groove shades it */
        float head = clamp01(1.0f - (front - (float)d) / 3.0f);                                      /* the front of the flow is bright */
        R = lerp(R, 255, head * 0.9f); G = lerp(G, 96, head * 0.9f); B = lerp(B, 60, head * 0.9f);
        R = lerp(R, 255, bright * 0.5f); G = lerp(G, 50, bright * 0.5f); B = lerp(B, 36, bright * 0.5f);
        cell_at(r, sx, sy, cell, x, y, R, G, B, 255);
    }

    /* the charge: two bright heads run round the circle, faster and faster, and the whole thing flashes at the end */
    if (t >= charge_at() && t < flash_at() + 0.3f) {
        float u = t - charge_at(), a0 = u * u * 11.0f, fl = t >= flash_at() ? clamp01(1.0f - (t - flash_at()) / 0.3f) : 0.0f;
        for (int i = 0; i < ncarve; i++) {
            int x = carve_x[i], y = carve_y[i];
            if (x * x + y * y < 16 || (float)dist[y + GO][x + GO] > front) continue;
            float ang = atan2f((float)(2 * y), (float)x), best = 9.0f;
            for (int k = 0; k < 2; k++) { float dd = fabsf(remainderf(ang - a0 - 3.14159265f * k, 6.2831853f)); if (dd < best) best = dd; }
            float h = clamp01(1.0f - best / 0.55f) * clamp01(u / 0.35f);
            h = h > fl ? h : fl;
            if (h > 0.05f) cell_at(r, sx, sy, cell, x, y, 255, lerp(70, 190, h), lerp(40, 110, h), (int)(255 * h));
        }
    }

    /* the dead vessel and his knife */
    if (dead_who) {
        char_set_darken(darken);
        char_draw_lying(r, dead_who, sx + BODY_X * cell, sy + BODY_Y * cell, cell);
        char_set_darken(0);
        wounds(r, sx, sy, cell, t);
    }
    {                                                                                 /* the knife lies above him: a shadow under it, the handle, the blade, a red tip */
        const int kx = -31, ky = -9;
        for (int i = 0; i < 8; i++) for (int j = 0; j < 3; j++) {
            int x = kx + i, y = ky + j;
            if (j == 2) { cell_at(r, sx, sy, cell, x, y, 0, 0, 0, 70); continue; }
            if (i <= 2)                          cell_at(r, sx, sy, cell, x, y, 70, 46, 34, 255);
            else if (i <= 6 && !(j == 0 && i == 6)) cell_at(r, sx, sy, cell, x, y, j == 0 ? 214 : 160, j == 0 ? 218 : 166, j == 0 ? 226 : 176, 255);
            else if (i == 7 && j == 1)           cell_at(r, sx, sy, cell, x, y, 150, 14, 20, 255);
        }
        cell_at(r, sx, sy, cell, kx + 4, ky + 1, 150, 14, 20, 255); cell_at(r, sx, sy, cell, kx + 5, ky + 1, 150, 14, 20, 255);
    }
}

/* the ring that goes out when the circle flashes, the blood pixels on the new body, the embers */
void ritual_draw_over(SDL_Renderer *r, int sx, int sy, int cell, const Person *who) {
    if (!on) return;
    float t = st;

    /* idle: now and then a mote of blood lifts off the grooves and fades */
    if (t > charge_at()) {
        float fade = t < end_at() ? 0.5f : 1.0f;
        for (int i = 0; i < 9; i++) {
            float per = 2.6f, ph = t / per + (float)i * 0.37f; uint32_t ep = (uint32_t)floorf(ph), hh = hash((uint32_t)i, ep);
            float u = ph - floorf(ph); int c = (int)(hh % (uint32_t)ncarve);
            int x = carve_x[c], y = carve_y[c] - (int)(u * 9.0f);
            cell_at(r, sx, sy, cell, x, y, 214, 40, 44, (int)(170.0f * fade * (1.0f - u) * (1.0f - u)));
        }
    }
    if (!active) return;

    /* the charge: embers swirl in from the ring and rise */
    float c0 = charge_at(), span = RITUAL_CHARGE_T + 0.7f;
    for (int i = 0; i < 42; i++) {
        float tb = c0 + span * (float)i / 42.0f, age = t - tb;
        if (age < 0 || age > 1.3f) continue;
        float u = age / 1.3f, a = (float)(hash((uint32_t)i, 7u) & 1023) * 0.00614f + u * 5.2f, rad = 14.0f * (1.0f - u) + 2.0f;
        int x = (int)lroundf(rad * cosf(a)), y = (int)lroundf(rad * sinf(a) * 0.5f - u * 12.0f);
        int big = (i & 3) == 0, A = (int)(255.0f * (1.0f - u * u));
        cell_at(r, sx, sy, cell, x, y, 255, lerp(70, 190, 1.0f - u), 40, A);
        if (big) cell_at(r, sx, sy, cell, x + 1, y, 255, 120, 60, A / 2);
    }

    /* the flash: one thin ring goes out over the floor */
    float f0 = flash_at();
    if (t >= f0 && t < f0 + 0.6f) {
        float u = (t - f0) / 0.6f, rad = 4.0f + 24.0f * u;
        int A = (int)(230.0f * (1.0f - u));
        for (int y = -GO; y <= GO; y++) for (int x = -GO; x <= GO; x++) {
            if (fabsf(ell(x, y) - rad) < 0.9f) cell_at(r, sx, sy, cell, x, y, 255, 120, 70, A);
        }
    }

    /* the body forms out of the blood: pixels of blood sit on it, and go one by one */
    float k = (t - form_at()) / (RITUAL_FORM_T + RITUAL_DISS_T);
    if (who && k >= 0.0f && k < 1.0f) {
        int dens = (int)(255.0f * (1.0f - k)), rows = ritual_player_rows();
        for (int j = 0; j < 12; j++) {
            if (11 - j >= rows) continue;
            for (int i = 0; i < 10; i++) {
                if (!char_cell_opaque(who, FACE_DOWN, i, j)) continue;
                uint32_t h = hash((uint32_t)i + 11u, (uint32_t)j + 3u) & 255u;
                if ((int)h >= dens) continue;
                if (h & 1) cell_at(r, sx, sy, cell, i - 5, j - 12, 118, 10, 22, 240);
                else       cell_at(r, sx, sy, cell, i - 5, j - 12, 232, 56, 52, 240);
            }
        }
    }
}
