/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * drives the tape scrub (audio.c) on a PC and records what it plays, to check it really runs backwards / faster:
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/scrubtest tools/scrubtest.c game/audio.c game/analyze.c $(sdl2-config --libs) -lm
 *   cd music && SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=/tmp/scrub.raw ../build/scrubtest [track.ogg]
 * then /tmp/scrub.raw is S16 stereo 44100 Hz: cross-correlate short windows of it with the decoded .ogg (forward and
 * reversed) to see the head really ran backwards while the finger dragged back */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include "audio.h"

static void wait_s(double s) {
    Uint64 t0 = SDL_GetPerformanceCounter(), f = SDL_GetPerformanceFrequency();
    while ((double)(SDL_GetPerformanceCounter() - t0) / (double)f < s) SDL_Delay(5);
}

static void drag(double from, double speed, double secs, const char *tag) {   /* finger moves `speed` music-seconds per second */
    Uint64 t0 = SDL_GetPerformanceCounter(), f = SDL_GetPerformanceFrequency();
    double next = 0;
    for (;;) {
        double t = (double)(SDL_GetPerformanceCounter() - t0) / (double)f;
        if (t >= secs) break;
        music_scrub_to(from + speed * t);
        if (t >= next) { printf("%-8s t=%.1f finger=%.2f head=%.2f rate=%+.2f\n", tag, t, from + speed * t, music_position(), music_scrub_rate()); next += 0.5; }
        SDL_Delay(8);
    }
    music_scrub_to(from + speed * secs);
}

int main(int argc, char **argv) {
    const char *track = argc > 1 ? argv[1] : "third_life.ogg";
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_AUDIO);
    if (audio_init() != 0 || music_play(track, 1) != 0) { printf("audio/music failed\n"); return 1; }
    music_set_volume(1.0f);
    printf("duration %.2f s\n", music_duration());
    music_scrub_prepare();
    Uint32 t0 = SDL_GetTicks();
    while (music_scrub_ready() < 1.0f && SDL_GetTicks() - t0 < 20000) SDL_Delay(5);
    printf("decoded %.0f%% after %u ms\n", music_scrub_ready() * 100, SDL_GetTicks() - t0);

    wait_s(1.0);
    printf("normal play: pos %.2f (loop %d)\n", music_position(), music_loop());
    double start = 20.0;
    int ok = music_scrub_begin(start);
    printf("scrub_begin(%.1f) = %d, scrubbing %d, head %.2f\n", start, ok, music_scrubbing(), music_position());
    wait_s(0.5);
    printf("held still: head %.2f rate %+.2f (tape stopped)\n", music_position(), music_scrub_rate());
    drag(start, -1.0, 3.0, "BACK 1x");
    drag(start - 3.0, +3.0, 1.5, "FWD 3x");
    double fin = start - 3.0 + 4.5;
    wait_s(1.5);
    printf("finger rests at %.2f: head %.2f rate %+.2f\n", fin, music_position(), music_scrub_rate());
    music_scrub_end(fin);
    printf("released: scrubbing %d, pos %.2f\n", music_scrubbing(), music_position());
    wait_s(1.0);
    printf("1 s later: pos %.2f (expect ~%.2f, normal speed again)\n", music_position(), fin + 1.0);
    music_set_loop(0);
    printf("loop now %d\n", music_loop());
    music_scrub_release();
    audio_quit();
    return 0;
}
