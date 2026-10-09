/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * top buttons over time (silent dummy audio), writes build/ui_*.bmp:
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/uishot tools/uishot.c game/audio.c game/analyze.c game/musicwin.c game/nowplaying.c game/pausebtn.c game/font.c $(sdl2-config --libs) -lm
 *   cd music && ../build/uishot 540 1170 */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "musicwin.h"
#include "nowplaying.h"
#include "pausebtn.h"

int main(int argc, char **argv) {
    int W = argc > 1 ? atoi(argv[1]) : 540, H = argc > 2 ? atoi(argv[2]) : 1170;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    if (audio_init() != 0 || music_play("third_life.ogg", 1) != 0) return 1;
    nowplaying_init(W, H); musicwin_init(W, H); pausebtn_init(W, H); pausebtn_set_enabled(1);
    static const float shots[] = { 0.25f, 3.0f, 5.2f, 5.5f, 8.0f, 9.5f };
    int si = 0, paused = 0; float t = 0;
    for (; si < 6; t += 0.016f) {
        SDL_Delay(16);
        if (t > 8.2f && !paused) { int px, py, pw, ph; nowplaying_rect(&px, &py, &pw, &ph); int bx = px + pw + nowplaying_gap() + 6; pausebtn_touch(0, bx, py + 6); pausebtn_touch(1, bx, py + 6); paused = 1; }
        nowplaying_update(0.016f); pausebtn_update(0.016f); musicwin_update(0.016f);
        SDL_SetRenderDrawColor(r, 126, 188, 246, 255); SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 90, 150, 90, 255); SDL_Rect fl = { 0, H / 2, W, H / 2 }; SDL_RenderFillRect(r, &fl);
        pausebtn_draw_overlay(r); nowplaying_draw(r); pausebtn_draw(r);
        if (t >= shots[si]) { char n[64]; snprintf(n, sizeof n, "../build/ui_%d.bmp", si); SDL_SaveBMP(surf, n); printf("%s t=%.2f offset=%d\n", n, t, nowplaying_offset()); si++; }
    }
    audio_quit();
    return 0;
}
