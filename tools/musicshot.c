/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * plays a track for real (silent dummy audio driver), opens the music window and saves screenshots:
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/musicshot tools/musicshot.c game/audio.c game/analyze.c game/musicwin.c game/nowplaying.c game/font.c $(sdl2-config --libs) -lm
 *   cd music && ../build/musicshot 540 1170 out.bmp [track.ogg] [seconds] */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "musicwin.h"
#include "nowplaying.h"

int main(int argc, char **argv) {
    int W = argc > 1 ? atoi(argv[1]) : 540, H = argc > 2 ? atoi(argv[2]) : 1170;
    const char *out = argc > 3 ? argv[3] : "shot_music.bmp", *track = argc > 4 ? argv[4] : "third_life.ogg";
    float secs = argc > 5 ? (float)atof(argv[5]) : 6.0f;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    if (audio_init() != 0 || music_play(track, 1) != 0) { printf("audio/music failed\n"); return 1; }
    nowplaying_init(W, H); musicwin_init(W, H);
    int tapped = 0;
    for (float t = 0; t < secs; t += 0.016f) {
        SDL_Delay(16);
        nowplaying_update(0.016f);
        if (t > 0.5f && !tapped) {                        /* tap the card */
            nowplaying_touch(0, 30, 30); nowplaying_touch(1, 30, 30); tapped = 1;
            printf("window active after tap: %d\n", musicwin_active());
        }
        musicwin_update(0.016f);
        SDL_SetRenderDrawColor(r, 126, 188, 246, 255); SDL_RenderClear(r);
        nowplaying_draw(r); musicwin_draw(r);
    }
    int k = musicwin_debug_key_at(W / 2, H / 2 + (H < W ? 120 : 270));
    printf("key near the middle of the piano: %d\n", k);
    SDL_SaveBMP(surf, out); printf("wrote %s\n", out);
    audio_quit();
    return 0;
}
