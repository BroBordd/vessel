/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * checks the {highlight} markup of dialog.c: when the callback fires, that green + underlined pixels
 * appear only once the span is typed, and that spans wrap over lines. writes build/dialog_*.bmp
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/dialogtest tools/dialogtest.c $(ls game/*.c | grep -v game/game.c) $(sdl2-config --libs) -lm && build/dialogtest */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "dialog.h"
#include "char.h"

#define W 540
#define H 1170
static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("  !! "); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static SDL_Surface *surf; static SDL_Renderer *rr;
static float now; static int fired, fired_page[8], fired_span[8]; static float fired_at[8];
static int chained;

static void hook(int page, int span) { if (fired < 8) { fired_page[fired] = page; fired_span[fired] = span; fired_at[fired] = now; } fired++; }

static void step(float dt) { now += dt; dialog_update(dt); }

/* how many pixels of the popup are the highlight green (120,235,170) */
static int green_pixels(void) {
    SDL_SetRenderDrawColor(rr, 0, 0, 0, 255); SDL_RenderClear(rr);
    dialog_draw(rr);
    int n = 0; Uint32 *px = surf->pixels;
    for (int i = 0; i < W * H; i++) { Uint8 r, g, b; SDL_GetRGB(px[i], surf->format, &r, &g, &b); if (r == 120 && g == 235 && b == 170) n++; }
    return n;
}
static void shot(const char *name) { SDL_SaveBMP(surf, name); }

static void reset(void) { now = 0; fired = 0; chained = 0; }

/* a highlight hook that starts another dialog (like chunk 5 might) */
static const DialogLine NEXT[] = { { NULL, "Next page." } };
static void hook_chain(int page, int span) { (void)page; (void)span; chained++; dialog_play(NEXT, 1, NULL); }

int main(void) {
    extern const Person DEA;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    dialog_init(W, H);

    printf("one span: fires when its last letter is typed\n");
    static const DialogLine ONE[] = { { &DEA, "I dropped my {orb} in the Grasslands. Bring it back." } };
    reset(); dialog_on_highlight(hook); dialog_play(ONE, 1, NULL);
    int before = -1;
    for (; now < 0.45f; ) step(0.016f);
    before = green_pixels();
    CHECK(before == 0, "green pixels before the span is typed: %d", before);
    for (; now < 1.5f; ) step(0.016f);
    int after = green_pixels();
    printf("  fired %d time(s) at %.3fs, green pixels after %d\n", fired, fired_at[0], after);
    CHECK(fired == 1, "expected 1 callback, got %d", fired);
    CHECK(fired_at[0] > 0.55f && fired_at[0] < 0.62f, "fired at %.3fs, expected ~0.57s (16 letters at 28/s)", fired_at[0]);
    CHECK(fired_page[0] == 0 && fired_span[0] == 0, "page/span = %d/%d", fired_page[0], fired_span[0]);
    CHECK(after > 100, "no green text drawn");
    shot("build/dialog_one.bmp");
    for (; now < 6.0f; ) step(0.016f);
    CHECK(fired == 1, "callback fired again (%d)", fired);

    printf("two spans on page 1, wrapped over lines\n");
    static const DialogLine TWO[] = {
        { &DEA, "Plain page, no highlight." },
        { &DEA, "Go and find the {old glass orb that I dropped in the Grasslands} before {nightfall}, or else." },
    };
    reset(); dialog_on_page(NULL); dialog_on_highlight(hook); dialog_play(TWO, 2, NULL);
    for (; now < 4.0f; ) step(0.016f);
    CHECK(fired == 0, "callback fired on a page without markup");
    CHECK(green_pixels() == 0, "green on a plain page");
    SDL_Event dummy; (void)dummy;
    dialog_touch(0, W / 2, H - 40); dialog_touch(1, W / 2, H - 40);   /* next page */
    for (; now < 12.0f; ) step(0.016f);
    printf("  fired %d: ", fired); for (int i = 0; i < fired && i < 8; i++) printf("[page %d span %d @%.2f] ", fired_page[i], fired_span[i], fired_at[i]); printf("\n");
    CHECK(fired == 2, "expected 2 callbacks, got %d", fired);
    CHECK(fired == 2 && fired_span[0] == 0 && fired_span[1] == 1, "spans out of order");
    CHECK(fired == 2 && fired_page[0] == 1, "wrong page index");
    CHECK(fired == 2 && fired_at[1] > fired_at[0], "second span fired before first");
    CHECK(green_pixels() > 300, "green text missing");
    shot("build/dialog_two.bmp");

    printf("hook is dropped when the dialog ends\n");
    dialog_touch(0, W / 2, H - 40); dialog_touch(1, W / 2, H - 40);
    CHECK(!dialog_active(), "dialog should have ended");
    int was = fired;
    reset(); dialog_play(ONE, 1, NULL);
    for (; now < 3.0f; ) step(0.016f);
    CHECK(fired == 0, "stale hook fired (%d, was %d)", fired, was);

    printf("hook may start another dialog\n");
    dialog_touch(0, W / 2, H - 40); dialog_touch(1, W / 2, H - 40);
    reset(); dialog_on_highlight(hook_chain); dialog_play(ONE, 1, NULL);
    for (; now < 3.0f; ) step(0.016f);
    CHECK(chained == 1, "chained %d times", chained);
    CHECK(dialog_active(), "chained dialog not playing");

    printf("odd markup (unclosed, stray close, empty) does not crash\n");
    dialog_touch(0, W / 2, H - 40); dialog_touch(1, W / 2, H - 40);
    static const DialogLine ODD[] = { { NULL, "unclosed {span runs on" }, { NULL, "stray } close {} and {a} {b} {c}" } };
    reset(); dialog_on_highlight(hook); dialog_play(ODD, 2, NULL);
    for (; now < 4.0f; ) step(0.016f);
    printf("  page 0 fired %d\n", fired);
    CHECK(fired == 1, "unclosed span should fire once, got %d", fired);
    dialog_touch(0, W / 2, H / 3); dialog_touch(1, W / 2, H / 3);

    printf(fails ? "FAILED: %d\n" : "all ok\n", fails);
    return fails != 0;
}
