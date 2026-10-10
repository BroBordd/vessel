/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "thought.h"
#include "nowplaying.h"
#include "char.h"
#include "font.h"
#include "hud.h"
#include <math.h>
#include <string.h>

#define LERP_SPEED   7.0f           /* the same easing as the music card: quick at first, eases to a stop */
#define MAX_TEXT     120
#define MAX_QUEUE    4
#define MAX_LINES    5
#define MIN_LETTERS  8              /* room for fewer letters per line than this and the thought waits */

typedef struct { char text[MAX_TEXT]; float secs; } Thought;
typedef struct { int start, len; } Line;

static int   W, H, enabled, active, closing, nq;
static float u, vis, c, hold_t;     /* vis: button fade-in. c: 0 = open card, 1 = collapsed button. hold_t: seconds on show */
static Thought cur, queue[MAX_QUEUE];

/* layout, rebuilt every frame (the music card next to us can open and close) */
static int q, pad, port, tgap, lgap, bs, bgap;
static int left, top, pw, ph, room;           /* pw, ph: the size the card opens to */
static Line ln[MAX_LINES];
static int  nl;

/* the pixel brain, 10 x 10 cells: two halves with a fissure between them */
static const char *BRAIN[10] = {
    ".PPP..PPP.",
    "PPPPffPPPP",
    "PdddffdddP",
    "PPPdffdPPP",
    "PdPPffPPdP",
    "PPddffddPP",
    "PdPPffPPdP",
    "PPdPffPdPP",
    ".PPPffPPP.",
    "..PP..PP..",
};

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void thought_init(int w, int h) {
    W = w; H = h; u = (w < h ? w : h) / 360.0f;
    q    = (int)(1.7f * u); if (q < 2)   q = 2;          /* the same pixel size as the ID card and the music card's text */
    pad  = (int)(6 * u);    if (pad < 3) pad = 3;        /* the music card's padding */
    port = 12 * q;                                       /* portrait window: 12 cells, the face is 10 inside it */
    tgap = 3 * q;                                        /* portrait <-> text */
    lgap = 2 * q;                                        /* between text lines */
    nq = 0; active = closing = 0; hold_t = 0;
    c = 1.0f;
    vis = enabled ? 1.0f : 0.0f;
    nl = 0;
    thought_update(0);                                   /* fills in the layout, so thought_rect is right from the start */
}

void thought_set_enabled(int on) { enabled = on; }

static void start(const Thought *t) { cur = *t; hold_t = 0; closing = 0; active = 1; }

void thought_say(const char *text, float seconds) {
    if (!text || !*text) return;
    Thought t;
    strncpy(t.text, text, MAX_TEXT - 1); t.text[MAX_TEXT - 1] = 0;
    if (seconds <= 0) {
        seconds = 1.8f + 0.07f * (float)strlen(t.text);      /* time to read it, with a little breathing room */
        if (seconds < 3.0f) seconds = 3.0f;
    }
    t.secs = seconds;
    if (!active || closing) start(&t);                       /* idle, or already on its way out: show it right away */
    else if (nq < MAX_QUEUE) queue[nq++] = t;
}

int thought_active(void) { return active; }

/* ---------- text wrapping ---------- */
/* breaks `s` into at most maxl lines of at most maxc letters, on spaces where it can. returns the line count */
static int wrap(const char *s, int maxc, int maxl, Line *out) {
    int n = 0, i = 0, len = (int)strlen(s);
    while (i < len && n < maxl) {
        while (i < len && s[i] == ' ') i++;                 /* no leading spaces */
        if (i >= len) break;
        int end = i + maxc;
        if (end >= len) end = len;
        else {
            int k = end;                                    /* look back for a space to break on */
            while (k > i && s[k] != ' ') k--;
            if (k > i) end = k;                             /* else: one long word, break it hard */
        }
        int e = end;
        while (e > i && s[e - 1] == ' ') e--;
        out[n].start = i; out[n].len = e - i; n++;
        i = end;
    }
    return n;
}

/* where the button is, and (when a thought is up) how big the card opens and whether it has room */
static void relayout(void) {
    int mx, my, mw, mh; nowplaying_rect(&mx, &my, &mw, &mh);
    bs = nowplaying_button_size(); bgap = nowplaying_gap();
    left = mx + mw + bgap; top = my;                        /* right of the music button / card */
    pw = ph = bs; room = 1; nl = 0;
    if (!active) return;

    int cx, cy, cw, ch; hud_card_rect(&cx, &cy, &cw, &ch);
    int avail = (cx - bgap) - (bs + bgap) - left;           /* up to the ID card, keeping the pause button's place free */
    int text_w = avail - 2 * pad - port - tgap;
    int maxc = (text_w + q) / (6 * q);                      /* letters per line (a letter is 6 cells wide, minus the last gap) */
    if (maxc < MIN_LETTERS) { room = 0; return; }

    int avail_h = (cy + ch) - top - 2 * pad;                /* never taller than the ID card: the toast under it stays clear */
    int maxl = clampi((avail_h + lgap) / (7 * q + lgap), 1, MAX_LINES);

    /* the music card can leave us narrow, and narrow means more lines than fit under the ID card's height.
     * a thought is never cut for that: it waits until the music card folds (then it fits). only a thought
     * that is too long even with the music card folded is cut, because waiting would never help. */
    Line tmp[64];
    int best_avail = (cx - bgap) - (bs + bgap) - (mx + bs + bgap);                 /* the music card folded */
    int best_maxc = (best_avail - 2 * pad - port - tgap + q) / (6 * q);
    int need_now  = wrap(cur.text, maxc, 64, tmp);
    int need_best = best_maxc >= MIN_LETTERS ? wrap(cur.text, best_maxc, 64, tmp) : 0;
    if (need_now > maxl && need_best <= maxl) { room = 0; return; }

    nl = wrap(cur.text, maxc, maxl, ln);
    int widest = 0;
    for (int i = 0; i < nl; i++) if (ln[i].len > widest) widest = ln[i].len;
    int th = nl * 7 * q + (nl > 0 ? (nl - 1) * lgap : 0);
    pw = 2 * pad + port + tgap + (widest > 0 ? widest * 6 * q - q : 0);
    ph = 2 * pad + (th > port ? th : port);
    if (pw < bs) pw = bs;
    if (ph < bs) ph = bs;
}

void thought_update(float dt) {
    relayout();

    float tv = enabled ? 1.0f : 0.0f;
    vis += (tv - vis) * (1.0f - expf(-dt * 9.0f));
    if (fabsf(tv - vis) < 0.004f) vis = tv;

    int want_open = 0;
    if (active && room && !closing) {                       /* the clock only runs while the card has room to be read */
        hold_t += dt;
        if (hold_t >= cur.secs) {
            if (nq > 0) {                                   /* next in line: swap the words, no need to fold away first */
                Thought next = queue[0];
                for (int i = 1; i < nq; i++) queue[i - 1] = queue[i];
                nq--;
                start(&next);
            } else closing = 1;
        }
    }
    if (active && room && !closing) want_open = 1;

    float target = want_open ? 0.0f : 1.0f;
    c += (target - c) * (1.0f - expf(-dt * LERP_SPEED));
    if (fabsf(target - c) < 0.002f) c = target;
    if (active && closing && c >= 0.98f) { active = 0; closing = 0; c = 1.0f; }
}

/* ---------- geometry for the neighbours ---------- */
static int cur_w(void) { return bs + (int)((pw - bs) * (1.0f - c) + 0.5f); }
static int cur_h(void) { return bs + (int)((ph - bs) * (1.0f - c) + 0.5f); }

void thought_rect(int *x, int *y, int *w, int *h) { *x = left; *y = top; *w = cur_w(); *h = cur_h(); }

int thought_offset(void) {
    int off = cur_h() + top + bgap - (int)(14 * u);         /* same formula as nowplaying_offset(): the list starts at 14u on its own */
    return off > 0 ? off : 0;
}

/* ---------- drawing ---------- */
static void fillr(SDL_Renderer *r, int x, int y, int w, int h) {
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderFillRect(r, &rc);
}

static void draw_brain(SDL_Renderer *r, int x, int y, int size, int a) {
    int ic = size / 10; if (ic < 1) ic = 1;
    int ox = x + (size - 10 * ic) / 2, oy = y + (size - 10 * ic) / 2;
    for (int j = 0; j < 10; j++)
        for (int i = 0; i < 10; i++) {
            char ch = BRAIN[j][i];
            if (ch == '.') continue;
            if      (ch == 'P') SDL_SetRenderDrawColor(r, 255, 150, 170, a);
            else if (ch == 'd') SDL_SetRenderDrawColor(r, 196, 84, 116, a);
            else                SDL_SetRenderDrawColor(r, 150, 52, 88, a);
            fillr(r, ox + i * ic, oy + j * ic, ic, ic);
        }
}

/* the player's face in a little window of dusk sky, so it reads as "inside the head" */
static void draw_face(SDL_Renderer *r, int x, int y, int a) {
    SDL_SetRenderDrawColor(r, 90, 104, 170, a);
    fillr(r, x, y, port, port);
    int rows = port / q - 2;
    for (int row = 0; row < rows; row++) {
        float k = row / (float)(rows - 1);
        SDL_SetRenderDrawColor(r, (int)(52 + 40 * k), (int)(66 + 50 * k), (int)(120 + 60 * k), a);
        fillr(r, x + q, y + q + row * q, port - 2 * q, q);
    }
    const Person *who = hud_person();
    if (!who) return;
    char_draw_portrait(r, who, x + q, y + q, q, 0);
    if (a < 255) {                                          /* fade the face by dimming it toward the card */
        SDL_SetRenderDrawColor(r, 10, 12, 24, 255 - a);
        fillr(r, x + q, y + q, port - 2 * q, port - 2 * q);
    }
}

void thought_draw(SDL_Renderer *r) {
    if (vis <= 0.0f) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int A = clampi((int)(255 * vis), 0, 255);
    int bx = left + (int)((1.0f - vis) * -6 * u), by = top;     /* tucks in from the left as it appears, like the pause button */
    int w = cur_w(), h = cur_h();
    ui_button_frame(r, bx, by, w, h, 0, A);

    float open = 1.0f - c;
    int ca = clampi((int)((open - 0.45f) / 0.55f * 255.0f), 0, 255) * A / 255;   /* card contents fade in late, out early */
    int ia = clampi((int)((c - 0.45f) / 0.55f * 255.0f), 0, 255) * A / 255;      /* the brain fades in as it closes */
    SDL_Rect clip = { bx + 1, by + 1, w - 2, h - 2 };
    SDL_RenderSetClipRect(r, &clip);

    if (ca > 0 && nl > 0) {
        draw_face(r, bx + pad, by + (ph - port) / 2, ca);
        int th = nl * 7 * q + (nl - 1) * lgap;
        int tx = bx + pad + port + tgap, ty = by + (ph - th) / 2;
        char buf[MAX_TEXT];
        SDL_SetRenderDrawColor(r, 255, 255, 255, ca);
        for (int i = 0; i < nl; i++) {
            memcpy(buf, cur.text + ln[i].start, (size_t)ln[i].len); buf[ln[i].len] = 0;
            font_draw(r, buf, tx, ty + i * (7 * q + lgap), q);
        }
    }
    if (ia > 0) draw_brain(r, bx, by, bs, ia);
    SDL_RenderSetClipRect(r, NULL);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
