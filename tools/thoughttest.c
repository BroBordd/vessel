/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the thinking bar over a fake sky next to the ID card, writes build/thought_*.bmp (no audio device needed).
 * shots: sliding in, resting, a long text wrapping, the music card open (no room: it waits), sliding out.
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/thoughttest tools/thoughttest.c game/thought.c game/hud.c game/char.c game/audio.c game/analyze.c game/jukebox.c game/font.c game/nowplaying.c game/musicwin.c $(sdl2-config --libs) -lm
 *   ./build/thoughttest 540 1170
 * (if the link complains about a missing game/*.c, add it: the list is whatever nowplaying.c and hud.c pull in) */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "hud.h"
#include "thought.h"

static const Person VESSEL = { "Aonia", { 96, 58, 36 }, { 248, 208, 170 }, { 214, 60, 60 }, { 52, 70, 140 }, { 40, 30, 30 }, 0, 0 };

static SDL_Surface *surf;
static SDL_Renderer *rr;
static int W, H;

static void shot(const char *name, int x0, int x1, int y) {
    SDL_SetRenderDrawColor(rr, 126, 188, 246, 255); SDL_RenderClear(rr);
    hud_draw(rr);
    thought_draw(rr, x0, x1, y);
    SDL_SaveBMP(surf, name);
    printf("%s\n", name);
}

static void run(float secs) { for (float t = 0; t < secs; t += 0.01f) { thought_update(0.01f); hud_update(0.01f); } }

int main(int argc, char **argv) {
    W = argc > 1 ? atoi(argv[1]) : 540; H = argc > 2 ? atoi(argv[2]) : 1170;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    rr = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);
    float u = (W < H ? W : H) / 360.0f;
    hud_init(W, H, &VESSEL); thought_init(W, H);
    int cx, cy, cw, ch; hud_card_rect(&cx, &cy, &cw, &ch);
    int gap = (int)(5 * u);
    int x0 = (int)(10 * u) + 2 * (int)(26 * u) + 3 * gap;        /* right of the music + pause buttons */
    int x1 = cx - gap, y = (int)(8 * u);

    int fails = 0;
    thought_say("I need to find that orb.", 0);
    if (!thought_active()) { printf("FAIL: not active after thought_say\n"); fails++; }
    run(0.12f);  shot("build/thought_0.bmp", x0, x1, y);         /* sliding in */
    run(0.60f);  shot("build/thought_1.bmp", x0, x1, y);         /* resting */
    thought_say("She could at least have warned me about the fall.", 0);
    run(3.5f);   shot("build/thought_2.bmp", x0, x1, y);         /* the queued long one wraps over lines */
    /* too narrow (the music card is open): nothing is drawn and the timer holds */
    shot("build/thought_3.bmp", x0, x0 + (int)(40 * u), y);
    run(30.0f);
    if (!thought_active()) { printf("FAIL: it ran its timer while there was no room\n"); fails++; }
    thought_draw(rr, x0, x1, y);                                 /* the card folded away: there is room again */
    run(30.0f);
    if (thought_active()) { printf("FAIL: still active after its time ran out\n"); fails++; }
    return fails ? 1 : 0;
}
