/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "talk.h"
#include "font.h"
#include "lang.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_KEYS   48
#define MAX_CHIPS  8
#define ID_BACK    1000
#define ID_SEND    1001
#define ID_CHIP    2000                 /* + chip index */
#define ID_NONE    -1

typedef struct { SDL_Rect r; char ch; int kind; } Key;      /* kind: 0 character, 1 backspace, 2 space */

static int   W, H;
static float u;
static int   is_open, required;
static float anim, t, hold;
static void (*cb)(const char *);

static char  npc_name[24], npc_line[320];
static const char *chips[MAX_CHIPS]; static int nchips;
static SDL_Rect chip_r[MAX_CHIPS];

static char  text[TALK_MAX + 1]; static int tlen;
static uint8_t hl[TALK_MAX + 2];
static Heard heard; static char heard_label[120]; static int nhl;

static Key   keys[MAX_KEYS]; static int nkeys;
static SDL_Rect panel, back_r, send_r, input_r, heard_r, title_r;
static int   c1, c2, tc;                // font cells: c1 big (keys), c2 small (labels), tc text in the box
static int   pressed = ID_NONE;
static int   yoff;                       // slide-in offset

/* ---------- small drawing helpers ---------- */
static void fillr(SDL_Renderer *r, SDL_Rect q, int dy) { q.y += dy; SDL_RenderFillRect(r, &q); }
static void col(SDL_Renderer *r, int R, int G, int B, int A) { SDL_SetRenderDrawColor(r, R, G, B, A); }
static int  inside(SDL_Rect q, int x, int y) { return x >= q.x && x < q.x + q.w && y >= q.y + yoff && y < q.y + q.h + yoff; }

static void box(SDL_Renderer *r, SDL_Rect q, int bt, int fr, int fg, int fb, int fa, int lr, int lg, int lb, int la) {
    col(r, lr, lg, lb, la); fillr(r, q, yoff);
    SDL_Rect in = { q.x + bt, q.y + bt, q.w - 2 * bt, q.h - 2 * bt };
    col(r, fr, fg, fb, fa); fillr(r, in, yoff);
}

static void label_center(SDL_Renderer *r, const char *s, SDL_Rect q, int cell) {
    int w = font_width(s, cell), h = font_height(cell);
    font_draw(r, s, q.x + (q.w - w) / 2, q.y + yoff + (q.h - h) / 2, cell);
}

/* ---------- text state ---------- */
static void reparse(void) {
    lang_parse(text, &heard);
    nhl = lang_highlight(text, hl);
    lang_heard_label(&heard, heard_label, (int)sizeof heard_label);
}
static int allowed(int c) { return isalnum(c) || c == ' ' || c == ',' || c == '.' || c == '!' || c == '?' || c == '\'' || c == '-'; }
static void add_char(int c) {
    if (!allowed(c) || tlen >= TALK_MAX) return;
    if (c == ' ' && (tlen == 0 || text[tlen - 1] == ' ')) return;
    text[tlen++] = (char)c; text[tlen] = 0; reparse();
}
static void backspace(void) { if (tlen > 0) { text[--tlen] = 0; reparse(); } }
void talk_debug_type(const char *s) { for (; *s; s++) add_char((unsigned char)*s); }
const char *talk_debug_text(void) { return text; }
static void fire(int id);
void talk_debug_send(void) { fire(ID_SEND); }

/* ---------- layout ---------- */
static void layout(void) {
    int m = (int)(6 * u); if (m < 3) m = 3;
    int g = (int)(3 * u); if (g < 2) g = 2;
    int kh = (int)(25 * u); if (kh < 14) kh = 14;
    int bpad = (int)(8 * u); if (bpad < 3) bpad = 3;
    c1 = (int)(2.0f * u); if (c1 < 2) c1 = 2;
    c2 = (int)(1.5f * u); if (c2 < 2) c2 = 2;
    tc = (int)(1.6f * u); if (tc < 2) tc = 2;

    int kb_h = 5 * kh + 4 * g;
    int kb_y = H - bpad - kb_h;
    int kw = (W - 2 * m - 9 * g) / 10;
    nkeys = 0;
    static const char *ROWS[5] = { "1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm", NULL };
    for (int row = 0; row < 4; row++) {
        int n = (int)strlen(ROWS[row]);
        int extra = row == 3 ? 2 : 0;                       /* the backspace key is 2 slots wide (row 3 only) */
        int total = n * kw + (n - 1) * g + (extra ? g + kw * 3 / 2 : 0);
        int x = (W - total) / 2, y = kb_y + row * (kh + g);
        for (int i = 0; i < n; i++, x += kw + g) keys[nkeys++] = (Key){ { x, y, kw, kh }, ROWS[row][i], 0 };
        if (extra) keys[nkeys++] = (Key){ { x, y, kw * 3 / 2, kh }, 8, 1 };
    }
    /* bottom row: ' - , [ space ] . ? !  (weights 1 1 1 4 1 1 1 = 10 slots) */
    {
        static const char PUNCT[7] = { '\'', '-', ',', ' ', '.', '?', '!' };
        static const int  WT[7] = { 1, 1, 1, 4, 1, 1, 1 };
        int y = kb_y + 4 * (kh + g), x = m;
        int unit = (W - 2 * m - 6 * g) / 10;
        for (int i = 0; i < 7; i++) {
            int w = unit * WT[i] + (WT[i] - 1) * g / 1;
            if (i == 6) w = W - m - x;
            keys[nkeys++] = (Key){ { x, y, w, kh }, PUNCT[i], PUNCT[i] == ' ' ? 2 : 0 };
            x += w + g;
        }
    }

    /* chips: wrap rows */
    int chip_h = 11 * c2 + 2 * c2; int cx = m, rows = nchips ? 1 : 0;
    for (int i = 0; i < nchips; i++) {
        int w = font_width(chips[i], c2) + 6 * c2;
        if (cx + w > W - m) { cx = m; rows++; }
        chip_r[i] = (SDL_Rect){ cx, rows - 1, w, chip_h };      /* y is the row for now */
        cx += w + g;
    }
    int chips_h = rows ? rows * chip_h + (rows - 1) * g : 0;

    int title_h = 11 * c2 + 4 * c2;
    int ib_h = 3 * 10 * tc + 2 * tc * 3;
    int heard_h = 9 * c2;
    int pad = g;
    int total = pad + title_h + g + (chips_h ? chips_h + g : 0) + ib_h + g + heard_h + g;
    int py = kb_y - total;
    panel = (SDL_Rect){ 0, py - pad, W, H - (py - pad) };
    int y = py;
    title_r = (SDL_Rect){ m, y, W - 2 * m, title_h };
    back_r = (SDL_Rect){ m, y, font_width("BACK", c2) + 8 * c2, title_h };
    y += title_h + g;
    for (int i = 0; i < nchips; i++) { int row = chip_r[i].y; chip_r[i].y = y + row * (chip_h + g); }
    if (chips_h) y += chips_h + g;
    int sw = font_width("SEND", c2) + 10 * c2;
    input_r = (SDL_Rect){ m, y, W - 2 * m - sw - g, ib_h };
    send_r = (SDL_Rect){ W - m - sw, y, sw, ib_h };
    y += ib_h + g;
    heard_r = (SDL_Rect){ m, y, W - 2 * m, heard_h };
}

void talk_init(int w, int h) { W = w; H = h; u = (w < h ? w : h) / 360.0f; is_open = 0; layout(); }

void talk_open(const char *name, const char *line, const char *const *sugg, int ns, int req, void (*done)(const char *)) {
    snprintf(npc_name, sizeof npc_name, "%s", name ? name : "");
    snprintf(npc_line, sizeof npc_line, "%s", line ? line : "");
    nchips = 0;
    for (int i = 0; i < ns && i < MAX_CHIPS; i++) chips[nchips++] = sugg[i];
    required = req; cb = done;
    text[0] = 0; tlen = 0; reparse();
    anim = 0; hold = 0; pressed = ID_NONE; t = 0; is_open = 1;
    layout();
}
int talk_active(void) { return is_open; }

static void close_overlay(void) { is_open = 0; pressed = ID_NONE; }

/* ---------- touch ---------- */
static int hit_id(int x, int y) {
    if (inside(back_r, x, y)) return ID_BACK;
    if (inside(send_r, x, y)) return ID_SEND;
    for (int i = 0; i < nchips; i++) if (inside(chip_r[i], x, y)) return ID_CHIP + i;
    for (int i = 0; i < nkeys; i++) if (inside(keys[i].r, x, y)) return i;
    return ID_NONE;
}

static void fire(int id) {
    if (id == ID_BACK) { close_overlay(); if (cb) cb(NULL); return; }
    if (id == ID_SEND) {
        if (!tlen) return;
        char out[TALK_MAX + 1]; memcpy(out, text, (size_t)tlen + 1);
        void (*f)(const char *) = cb;
        close_overlay();
        if (f) f(out);
        return;
    }
    if (id >= ID_CHIP && id < ID_CHIP + nchips) {
        snprintf(text, sizeof text, "%s", chips[id - ID_CHIP]); tlen = (int)strlen(text); reparse(); return;
    }
    if (id >= 0 && id < nkeys) {
        if (keys[id].kind == 1) backspace(); else add_char(keys[id].ch);
    }
}

void talk_touch(int a, int x, int y) {
    if (!is_open) return;
    if (a == 0) { pressed = hit_id(x, y); hold = 0; return; }
    if (a == 2) { if (pressed != ID_NONE && hit_id(x, y) != pressed) pressed = ID_NONE; return; }
    if (a == 3) { pressed = ID_NONE; return; }
    if (a == 1) {
        int id = pressed; pressed = ID_NONE;
        if (id != ID_NONE && hit_id(x, y) == id) {
            /* backspace already repeated while held: a long press must not delete one more */
            if (id >= 0 && id < nkeys && keys[id].kind == 1 && hold > 0.45f) return;
            fire(id);
        }
    }
}

void talk_update(float dt) {
    if (!is_open) return;
    t += dt;
    if (anim < 1) { anim += dt / 0.18f; if (anim > 1) anim = 1; }
    float k = 1.0f - anim; yoff = (int)(k * k * (panel.h + 8));
    if (pressed >= 0 && pressed < nkeys && keys[pressed].kind == 1) {      /* hold backspace to repeat */
        float before = hold; hold += dt;
        if (hold > 0.45f) { int n0 = (int)((before - 0.45f) / 0.07f), n1 = (int)((hold - 0.45f) / 0.07f); if (before < 0.45f) n0 = -1; for (int i = n0; i < n1; i++) backspace(); }
    }
}

/* ---------- drawing ---------- */
/* char-wraps text into at most 3 lines: start index of each line */
static int wrap(int maxc, int *start, int *len) {
    int n = 0, p = 0;
    if (!tlen) { start[0] = 0; len[0] = 0; return 1; }
    while (p < tlen && n < 3) {
        int remain = tlen - p, l = remain <= maxc ? remain : maxc;
        if (remain > maxc) {
            int sp = -1;
            for (int i = p + maxc; i > p; i--) if (text[i] == ' ') { sp = i; break; }
            if (sp > 0) l = sp - p;
        }
        start[n] = p; len[n] = l; n++;
        p += l; if (p < tlen && text[p] == ' ') p++;
    }
    return n;
}

void talk_draw(SDL_Renderer *r) {
    if (!is_open) return;
    int dy = yoff;
    int bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    col(r, 0, 0, 0, (int)(120 * anim)); SDL_Rect all = { 0, 0, W, H }; SDL_RenderFillRect(r, &all);

    SDL_Rect p = panel;
    col(r, 255, 255, 255, 255); p.y += dy; p.h = bt; SDL_RenderFillRect(r, &p);
    p = panel; p.y += dy + bt; p.h -= bt; col(r, 6, 8, 20, 246); SDL_RenderFillRect(r, &p);

    /* title: back button, the npc's name, and a hint about what the box does */
    SDL_Rect b = back_r; int bp = pressed == ID_BACK;
    box(r, b, bt, bp ? 70 : 24, bp ? 80 : 28, bp ? 130 : 48, 255, 255, 255, 255, 170);
    col(r, 255, 255, 255, 255); label_center(r, "BACK", b, c2);
    char title[64]; snprintf(title, sizeof title, "SAY SOMETHING TO %s", npc_name);
    col(r, 255, 214, 110, 255);
    int tw = font_width(title, c2);
    font_draw(r, title, title_r.x + title_r.w - tw, title_r.y + dy + (title_r.h - font_height(c2)) / 2, c2);

    /* chips */
    for (int i = 0; i < nchips; i++) {
        SDL_Rect q = chip_r[i]; int pr = pressed == ID_CHIP + i;
        int same = tlen && strlen(chips[i]) == (size_t)tlen;
        for (int k = 0; same && k < tlen; k++) if (tolower((unsigned char)chips[i][k]) != tolower((unsigned char)text[k])) same = 0;
        box(r, q, bt, pr ? 70 : 34, pr ? 80 : 40, pr ? 130 : 78, 255, same ? 255 : 255, same ? 214 : 255, same ? 110 : 255, same ? 255 : 150);
        col(r, 255, 255, 255, 255); label_center(r, chips[i], q, c2);
    }

    /* the input box */
    box(r, input_r, bt, 14, 18, 36, 255, 255, 255, 255, 200);
    int pad = tc * 3, maxc = (input_r.w - 2 * pad) / (6 * tc);
    int ls[3], ll[3], nl = wrap(maxc, ls, ll);
    int lh = 10 * tc;
    for (int li = 0; li < nl; li++) {
        int i = ls[li], end = ls[li] + ll[li], x = input_r.x + pad, y = input_r.y + dy + pad + li * lh;
        while (i < end) {                                         /* draw runs that share a highlight state */
            int on = hl[i], j = i; char run[TALK_MAX + 1]; int n = 0;
            while (j < end && hl[j] == on) run[n++] = text[j++];
            run[n] = 0;
            if (on) col(r, 120, 235, 170, 255); else col(r, 255, 255, 255, 255);
            font_draw(r, run, x + (i - ls[li]) * 6 * tc, y, tc);
            if (on) { SDL_Rect ul = { x + (i - ls[li]) * 6 * tc, y + 8 * tc, n * 6 * tc - tc, tc }; SDL_RenderFillRect(r, &ul); }
            i = j;
        }
    }
    if (!tlen) {                                                  /* the invitation to type */
        col(r, 130, 138, 170, 255);
        font_draw(r, "TYPE ANYTHING...", input_r.x + pad, input_r.y + dy + pad, tc);
    }
    if (((int)(t * 2.2f) & 1) == 0) {                             /* blinking cursor */
        int li = nl - 1, cxp = input_r.x + pad + ll[li] * 6 * tc;
        if (!tlen) cxp = input_r.x + pad - tc;
        col(r, 255, 214, 110, 255);
        SDL_Rect cur = { cxp, input_r.y + dy + pad + li * lh - tc, tc, 9 * tc }; SDL_RenderFillRect(r, &cur);
    }

    /* send */
    int sp = pressed == ID_SEND, ready = tlen > 0;
    box(r, send_r, bt, ready ? (sp ? 255 : 255) : 30, ready ? (sp ? 240 : 214) : 34, ready ? (sp ? 170 : 110) : 54, 255, 255, 255, 255, ready ? 255 : 120);
    if (ready) col(r, 30, 24, 10, 255); else col(r, 120, 126, 150, 255);
    label_center(r, "SEND", send_r, c2);

    /* what the npc understood so far */
    col(r, nhl ? 120 : 150, nhl ? 235 : 156, nhl ? 170 : 186, 255);
    font_draw(r, tlen ? heard_label : "TAP A WORD ABOVE OR TYPE YOUR OWN", heard_r.x, heard_r.y + dy, c2);

    /* keyboard */
    for (int i = 0; i < nkeys; i++) {
        const Key *k = &keys[i]; int pr = pressed == i;
        int special = k->kind != 0;
        box(r, k->r, bt, pr ? 90 : special ? 40 : 24, pr ? 100 : special ? 48 : 28, pr ? 160 : special ? 92 : 48, 255, 255, 255, 255, 150);
        col(r, 255, 255, 255, 255);
        if (k->kind == 1) {                                       /* a pixel arrow pointing left */
            int cx = k->r.x + k->r.w / 2, cy = k->r.y + dy + k->r.h / 2, a = c1;
            SDL_Rect sh = { cx - a, cy - a / 2, 4 * a, a }; SDL_RenderFillRect(r, &sh);
            for (int s2 = 0; s2 < 3; s2++) {                      /* arrow head: tall at the shaft, a point on the left */
                SDL_Rect hd = { cx - 4 * a + s2 * a, cy - (s2 * a + a / 2), a, 2 * s2 * a + a };
                SDL_RenderFillRect(r, &hd);
            }
        } else if (k->kind == 2) {
            label_center(r, "SPACE", k->r, c2);
        } else {
            char s[2] = { k->ch, 0 };
            label_center(r, s, k->r, c1);
        }
    }
}
