/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the brain window: opens from the brain button, loops through the thoughts, numbers pick one, AUTO loops again.
 * writes ../build/brain_*.bmp (silent dummy audio) and checks the logic, exit code != 0 if a check fails.
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/brainwintest tools/brainwintest.c game/brainwin.c game/thought.c game/hud.c game/toast.c game/missions.c game/char.c game/audio.c game/analyze.c game/jukebox.c game/font.c game/nowplaying.c game/musicwin.c game/pausebtn.c $(sdl2-config --libs) -lm
 *   cd music && ../build/brainwintest 540 1170      (also try 1170 540 and 360 640) */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "brainwin.h"
#include "hud.h"
#include "nowplaying.h"
#include "pausebtn.h"
#include "thought.h"

static const Person VESSEL = { "Aonia", { 96, 58, 36 }, { 248, 208, 170 }, { 214, 60, 60 }, { 52, 70, 140 }, { 40, 30, 30 }, 0, 0 };

static SDL_Surface *surf;
static SDL_Renderer *rr;
static int W, H, fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void run(float secs) {
    for (float t = 0; t < secs; t += 0.01f) {
        hud_update(0.01f); pausebtn_update(0.01f); nowplaying_update(0.01f);
        thought_update(0.01f); brainwin_update(0.01f);
    }
}
static void shot(const char *name) {
    SDL_SetRenderDrawColor(rr, 126, 188, 246, 255); SDL_RenderClear(rr);
    hud_draw(rr);
    nowplaying_draw(rr); thought_draw(rr); pausebtn_draw(rr);
    brainwin_draw(rr);
    if (SDL_SaveBMP(surf, name) != 0) { printf("FAIL: could not write %s (run from the music folder)\n", name); fails++; }
    else printf("wrote %s\n", name);
}
static void tap(int x, int y) { brainwin_touch(0, x, y); brainwin_touch(1, x, y); }
static int  in_screen(SDL_Rect r) { return r.x >= 0 && r.y >= 0 && r.x + r.w <= W && r.y + r.h <= H; }

int main(int argc, char **argv) {
    W = argc > 1 ? atoi(argv[1]) : 540; H = argc > 2 ? atoi(argv[2]) : 1170;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    audio_init();
    nowplaying_init(W, H); pausebtn_init(W, H); pausebtn_set_enabled(1);
    hud_init(W, H, &VESSEL); thought_init(W, H); thought_set_enabled(1);
    brainwin_init(W, H);
    char name[64];
    SDL_Rect win, mon, autob, closeb;

    /* 1. closed: it takes no touches, and the brain button opens it */
    CHECK(brainwin_count() == 0, "the window must start empty, got %d thoughts", brainwin_count());
    brainwin_open(); run(1.0f); shot("../build/brain_empty.bmp");          /* the empty state must draw and not crash */
    brainwin_touch(0, 2, 2); brainwin_touch(1, 2, 2); run(1.0f);
    CHECK(!brainwin_active(), "could not close the empty window");
    /* events: acquire adds, jumps to the new thought on AUTO, drop_tag forgets silently */
    CHECK(brainwin_acquire("Why am I standing on clouds?", BRAIN_SCENE_CLOUDS, 1) == 0, "first acquire");
    brainwin_open(); run(1.0f);
    CHECK(brainwin_debug_current() == 0 && brainwin_debug_auto(), "should show the only thought, on AUTO");
    CHECK(brainwin_acquire("I need to find that orb.", BRAIN_SCENE_ORB, 2) == 1 && brainwin_debug_current() == 1, "an open AUTO window did not jump to the new thought");
    CHECK(brainwin_drop_tag(1) == 1 && brainwin_count() == 1, "drop_tag(1) did not remove the cloud thought");
    CHECK(brainwin_drop_tag(0) == 0 && brainwin_drop_tag(99) == 0, "dropping tag 0 / an unknown tag removed something");
    run(1.0f);
    CHECK(brainwin_debug_current() == 0, "cur not fixed up after a drop (%d)", brainwin_debug_current());
    brainwin_touch(0, 2, 2); brainwin_touch(1, 2, 2); run(1.0f);
    brainwin_acquire("I happen to like grass.", BRAIN_SCENE_GRASS, 3);
    brainwin_acquire("Who is paying me for these tasks?", BRAIN_SCENE_COIN, 5);
    brainwin_acquire("Maybe she has seen the orb.", BRAIN_SCENE_ORB, 4);
    CHECK(brainwin_count() == 4, "expected 4 thoughts after the story events, got %d", brainwin_count());
    brainwin_clear();
    CHECK(brainwin_count() == 0, "clear did not empty the window");
    brainwin_acquire("I need to find that orb.", BRAIN_SCENE_ORB, 2);
    brainwin_acquire("I happen to like grass.", BRAIN_SCENE_GRASS, 3);
    brainwin_acquire("Who is paying me for these tasks?", BRAIN_SCENE_COIN, 5);
    brainwin_acquire("Why am I standing on clouds?", BRAIN_SCENE_CLOUDS, 1);
    CHECK(!brainwin_active() && brainwin_touch(0, 5, 5) == 0, "a closed window swallowed a touch");
    run(8.0f);
    {   int x, y, w, h; thought_rect(&x, &y, &w, &h);
        CHECK(thought_touch(0, x + w / 2, y + h / 2) == 1, "the brain button did not take the touch");
        CHECK(!brainwin_active(), "it opened on touch-down, not on release");
        CHECK(thought_touch(1, x + w / 2, y + h / 2) == 1 && brainwin_active(), "tapping the brain button did not open the window");
        CHECK(thought_touch(0, 3, H - 3) == 0, "the brain button took a touch that was not on it"); }
    run(1.0f);
    brainwin_debug_rects(&win, &mon, &autob, &closeb);
    CHECK(in_screen(win), "the window is off the screen (%d,%d %dx%d on %dx%d)", win.x, win.y, win.w, win.h, W, H);
    CHECK(brainwin_touch(0, 2, 2) == 1, "an open window must take every touch");
    brainwin_touch(3, 2, 2);
    CHECK(brainwin_active(), "a cancelled touch outside closed the window");
    CHECK(mon.w * 10 >= win.w * 8, "the monitor is tiny (%d of %d)", mon.w, win.w);

    /* 2. starts on thought 1, looks at every scene, and loops by itself */
    CHECK(brainwin_debug_current() == 0 && brainwin_debug_auto(), "did not start on the first thought, looping");
    run(2.0f); shot("../build/brain_0_orb.bmp");
    int seen[16] = { 0 }, order = 0, last = 0;
    for (float t = 0; t < 40.0f; t += 0.05f) {
        run(0.05f);
        int c = brainwin_debug_current(); seen[c] = 1;
        if (c != last) { CHECK(c == (last + 1) % 4, "the loop skipped (%d -> %d)", last, c); last = c; order++; }
    }
    CHECK(seen[0] && seen[1] && seen[2] && seen[3], "the loop did not visit every thought");
    CHECK(order >= 5, "the loop is too slow: only %d switches in 40 s", order);
    CHECK(order <= 8, "the loop is too fast: %d switches in 40 s", order);

    /* 2b. tapping the number of the thought already on the monitor does nothing (no replay, no pin) */
    {   int c = brainwin_debug_current(); SDL_Rect p = brainwin_debug_pick(c);
        run(1.0f);
        tap(p.x + p.w / 2, p.y + p.h / 2);
        CHECK(brainwin_debug_current() == c && brainwin_debug_auto(), "tapping the visible thought changed something (cur %d, auto %d)", brainwin_debug_current(), brainwin_debug_auto()); }
    /* 3. numbers pin a thought: it stays, whatever the time */
    for (int i = 0; i < 4; i++) {
        SDL_Rect p = brainwin_debug_pick(i);
        CHECK(p.w > 0 && p.x >= win.x && p.x + p.w <= win.x + win.w, "number box %d is not inside the window", i + 1);
        if (brainwin_debug_current() == i) { brainwin_debug_pick(i); tap(brainwin_debug_pick((i + 1) % 4).x + 2, p.y + 2); }   /* the visible one ignores taps: go via another */
        tap(p.x + p.w / 2, p.y + p.h / 2);
        CHECK(brainwin_debug_current() == i && !brainwin_debug_auto(), "tapping number %d did not pin thought %d (now %d, auto %d)", i + 1, i + 1, brainwin_debug_current(), brainwin_debug_auto());
        run(2.4f);
        snprintf(name, sizeof name, "../build/brain_%d_scene.bmp", i + 1);
        shot(name);
        run(20.0f);
        CHECK(brainwin_debug_current() == i, "a pinned thought moved on");
    }

    /* 4. AUTO starts the loop again */
    tap(autob.x + autob.w / 2, autob.y + autob.h / 2);
    CHECK(brainwin_debug_auto(), "AUTO did not turn the loop back on");
    int c0 = brainwin_debug_current(); run(10.0f);
    CHECK(brainwin_debug_current() != c0, "the loop did not move after AUTO");

    /* 5. more thoughts: the count follows, the window still fits, the array has a limit */
    int n0 = brainwin_count();
    CHECK(brainwin_add("A fifth thought, a bit longer than the others, to see how the window copes with it.", BRAIN_SCENE_GRASS) == n0, "brainwin_add did not return the new index");
    CHECK(brainwin_count() == n0 + 1, "count did not follow brainwin_add");
    while (brainwin_add("Filler.", BRAIN_SCENE_COIN) >= 0) { }
    CHECK(brainwin_count() == BRAIN_MAX_THOUGHTS, "the array limit is not BRAIN_MAX_THOUGHTS (%d)", brainwin_count());
    brainwin_debug_rects(&win, &mon, &autob, &closeb);
    CHECK(in_screen(win), "the window is off the screen with a full array");
    run(1.0f); shot("../build/brain_5_full.bmp");

    /* 6. closing: X, and a tap outside */
    tap(closeb.x + closeb.w / 2, closeb.y + closeb.h / 2);
    run(1.0f);
    CHECK(!brainwin_active(), "X did not close the window");
    brainwin_open(); run(1.0f);
    CHECK(brainwin_active(), "could not open again");
    tap(2, 2); run(1.0f);
    CHECK(!brainwin_active(), "tapping outside did not close the window");

    printf(fails ? "%d check(s) FAILED\n" : "all checks passed\n", fails);
    return fails != 0;
}
