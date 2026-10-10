/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the brain button over a fake sky, right of the music and pause buttons, next to the ID card. writes
 * ../build/thought_*.bmp (silent dummy audio) and checks the logic, exit code != 0 if a check fails.
 * run it from the music folder: it plays a real track to open the music card next to the brain.
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/thoughttest tools/thoughttest.c game/thought.c game/hud.c game/char.c game/audio.c game/analyze.c game/jukebox.c game/font.c game/nowplaying.c game/musicwin.c game/pausebtn.c $(sdl2-config --libs) -lm
 *   cd music && ../build/thoughttest 540 1170 */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "hud.h"
#include "nowplaying.h"
#include "pausebtn.h"
#include "thought.h"

static const Person VESSEL = { "Aonia", { 96, 58, 36 }, { 248, 208, 170 }, { 214, 60, 60 }, { 52, 70, 140 }, { 40, 30, 30 }, 0, 0 };

static SDL_Surface *surf;
static SDL_Renderer *rr;
static int W, H, fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

/* the same order game.c uses */
static void run(float secs) {
    for (float t = 0; t < secs; t += 0.01f) {
        hud_update(0.01f); pausebtn_update(0.01f);
        nowplaying_update(0.01f); thought_update(0.01f);
    }
}
static void shot(const char *name) {
    SDL_SetRenderDrawColor(rr, 126, 188, 246, 255); SDL_RenderClear(rr);
    hud_draw(rr);
    nowplaying_draw(rr); thought_draw(rr); pausebtn_draw(rr);
    if (SDL_SaveBMP(surf, name) != 0) { printf("FAIL: could not write %s (run from the music folder)\n", name); fails++; }
    else printf("wrote %s\n", name);
}
static int bw(void) { int x, y, w, h; thought_rect(&x, &y, &w, &h); return w; }
static int bh(void) { int x, y, w, h; thought_rect(&x, &y, &w, &h); return h; }

int main(int argc, char **argv) {
    W = argc > 1 ? atoi(argv[1]) : 540; H = argc > 2 ? atoi(argv[2]) : 1170;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    int have_music = audio_init() == 0 && music_play("third_life.ogg", 1) == 0;
    nowplaying_init(W, H); pausebtn_init(W, H); pausebtn_set_enabled(1);
    hud_init(W, H, &VESSEL); thought_init(W, H); thought_set_enabled(1);
    int bs = nowplaying_button_size(), gap = nowplaying_gap();
    int cx, cy, cw, ch; hud_card_rect(&cx, &cy, &cw, &ch);
    int mx, my, mw, mh;

    /* let the music card from the track start fold away first, so we begin from the plain buttons */
    run(8.0f);
    nowplaying_rect(&mx, &my, &mw, &mh);
    CHECK(mw == bs, "the music card is still open at the start (%d vs %d)", mw, bs);

    /* 1. idle: a plain button right of the music AND pause buttons (music, pause, brain) */
    {   int x, y, w, h; thought_rect(&x, &y, &w, &h);
        CHECK(x == mx + mw + gap + bs + gap && y == my, "the brain button is not right of the pause button");
        CHECK(w == bs && h == bs && !thought_active(), "idle brain is not a plain button"); }
    shot("../build/thought_0_idle.bmp");

    /* 2. a thought opens the card like the music card: grows, fits before the ID card */
    thought_say("I need to find that orb.", 0);
    CHECK(thought_active(), "not active after thought_say");
    run(0.12f); shot("../build/thought_1_opening.bmp");
    run(0.9f);  shot("../build/thought_2_open.bmp");
    {   int x, y, w, h; thought_rect(&x, &y, &w, &h);
        CHECK(w > bs * 2, "the card did not open (w %d)", w);
        CHECK(x + w <= cx - gap + 1, "the card runs into the ID card (%d > %d)", x + w, cx - gap);
        CHECK(y + h <= cy + ch, "the card is taller than the ID card (%d > %d)", y + h, cy + ch);
        CHECK(thought_offset() > nowplaying_offset(), "the mission list is not pushed down by the open card");
        int bottom_gap = thought_offset() - (h + y + gap - (int)(14 * (W < H ? W : H) / 360.0f));
        CHECK(bottom_gap == 0, "thought_offset does not follow the card height"); }

    /* 3. it folds back into the button by itself, and the list comes back up */
    run(2.6f);  shot("../build/thought_3_closing.bmp");
    run(2.5f);
    CHECK(!thought_active(), "the thought never ended");
    CHECK(bw() == bs && bh() == bs, "the card did not fold back into the button (%dx%d)", bw(), bh());
    CHECK(thought_offset() == nowplaying_offset(), "the mission list did not come back up");

    /* 4. a queue: the second thought swaps in without folding away first */
    thought_say("Maybe she has seen the orb.", 0);
    thought_say("She could at least have warned me.", 0);
    run(1.2f);
    int w_first = bw();
    run(2.3f);                                                  /* first one's time is up: the second takes over */
    CHECK(thought_active() && bw() > bs * 2, "the queue folded away between two thoughts");
    shot("../build/thought_4_queue.bmp");
    run(12.0f);
    CHECK(!thought_active(), "queued thoughts never ended");
    (void)w_first;

    /* 5. too long: cut to what fits, never taller than the ID card, never past it */
    thought_say("This one is far too long to ever fit in the little card up here.", 0);
    run(1.2f);
    {   int x, y, w, h; thought_rect(&x, &y, &w, &h);
        CHECK(y + h <= cy + ch && x + w <= cx - gap + 1, "an overlong thought broke the layout"); }
    shot("../build/thought_5_long.bmp");
    run(12.0f);

    /* 6. the music card is open: no room, so the thought waits (and keeps its time) until it folds away */
    if (have_music) {
        music_play("best_me.ogg", 1);                           /* a new track: the music card opens */
        run(0.6f);
        nowplaying_rect(&mx, &my, &mw, &mh);
        CHECK(mw > bs * 2, "the music card did not open");
        thought_say("I need to find that orb.", 0);
        run(0.8f);
        CHECK(bw() == bs, "the thought opened with no room (%d)", bw());
        shot("../build/thought_6_waiting.bmp");
        run(5.0f);                                              /* longer than the thought's own 3 s, while the music card was in the way */
        shot("../build/thought_7_after_music.bmp");
        CHECK(thought_active(), "the thought used up its time while it had no room");
        CHECK(bw() > bs * 2, "the thought never opened after the music card folded");
        run(8.0f);
        CHECK(!thought_active(), "the waiting thought never ended");
    } else printf("note: no audio, skipped the music-card check\n");

    /* 7. paused: dt = 0, the card holds still and the thought keeps its time */
    thought_say("I need to find that orb.", 0);
    run(1.0f);
    int w_open = bw();
    for (int i = 0; i < 600; i++) thought_update(0.0f);
    CHECK(thought_active() && bw() == w_open, "the card moved while paused");

    printf(fails ? "%d check(s) FAILED\n" : "all checks passed\n", fails);
    return fails ? 1 : 0;
}
