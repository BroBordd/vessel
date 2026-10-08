/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>

#include "audio.h"
#include "menu.h"

/* The game is a child of the app process. Closing the activity needs Java,
 * so for now exit == take the parent app down with us. */
static void quit_app(void) {
    pid_t pp = getppid();
    if (pp > 1) kill(pp, SIGKILL);
    _exit(0);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: game w h\n"); return 1; }
    int W = atoi(argv[1]), H = atoi(argv[2]);

    int fd = open("fb", O_RDWR);
    if (fd < 0) { perror("open fb"); return 1; }
    void *px = mmap(NULL, (size_t)W * H * 4, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);
    if (px == MAP_FAILED) { perror("mmap"); return 1; }

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    SDL_setenv("SDL_DISKAUDIOFILE", "audio.pcm", 1);
    /* disk driver sleeps a fixed ~46ms per buffer by default, which paces slightly
     * under real time and drains the cushion. 0 = let the app's blocking FIFO read
     * pace us instead (pipe gives ~350ms of slack). override via env if needed. */
    SDL_setenv("SDL_DISKAUDIODELAY", "0", 0);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init: %s\n", SDL_GetError()); fflush(stdout); return 1;
    }

    /* render into a private back buffer, then copy to the shared fb in one go,
     * so the app never reads a half-drawn frame (title/buttons are drawn last) */
    size_t fbsz = (size_t)W * H * 4;
    void *back = calloc(1, fbsz);
    if (!back) { perror("alloc back buffer"); return 1; }
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(
        back, W, H, 32, W * 4, SDL_PIXELFORMAT_RGBA32);
    if (!surf) { printf("surface: %s\n", SDL_GetError()); fflush(stdout); return 1; }
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    if (!r) { printf("renderer: %s\n", SDL_GetError()); fflush(stdout); return 1; }

    SDL_version v; SDL_GetVersion(&v);
    printf("SDL %d.%d.%d, %dx%d\n", v.major, v.minor, v.patch, W, H);
    fflush(stdout);

    if (audio_init() == 0) music_play("third_life.ogg", 0);

    menu_init(W, H);

    fcntl(0, F_SETFL, O_NONBLOCK);
    char acc[1024]; int alen = 0;

    /* fixed 60fps pacing: sleep only the time left in each frame budget */
    const Uint64 pf = SDL_GetPerformanceFrequency();
    const Uint64 frame_ticks = pf / 60;
    Uint64 prev = SDL_GetPerformanceCounter();
    Uint64 next_frame = prev + frame_ticks;
    Uint32 last = SDL_GetTicks(); int frames = 0;
    double worst_ms = 0, render_ms = 0;

    for (;;) {
        char buf[256];
        ssize_t n = read(0, buf, sizeof buf);
        if (n == 0) break;                          /* app closed our stdin */
        if (n > 0) {
            if (alen + n >= (int)sizeof acc) alen = 0;
            memcpy(acc + alen, buf, n); alen += n;
            char *s = acc, *nl;
            while ((nl = memchr(s, '\n', acc + alen - s))) {
                *nl = 0;
                int a, x, y;
                if (sscanf(s, "t %d %d %d", &a, &x, &y) == 3) {
                    MenuAction act = menu_touch(a, x, y);
                    if (act == MENU_PLAY) { printf("menu: play\n"); fflush(stdout); }
                    if (act == MENU_EXIT) quit_app();
                }
                s = nl + 1;
            }
            alen -= s - acc; memmove(acc, s, alen);
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - prev) / (float)pf;
        prev = now;
        if (dt > 0.05f) dt = 0.05f;

        menu_update(dt);
        menu_draw(r);
        SDL_RenderPresent(r);
        memcpy(px, back, fbsz);

        double ft = dt * 1000.0, rt = (double)(SDL_GetPerformanceCounter() - now) * 1000.0 / (double)pf;
        if (ft > worst_ms) worst_ms = ft;
        if (rt > render_ms) render_ms = rt;
        if (++frames == 60) {
            Uint32 tk = SDL_GetTicks();
            printf("fps %.1f worst %.1fms render %.1fms\n", 60000.0 / (tk - last), worst_ms, render_ms);
            fflush(stdout);
            last = tk; frames = 0; worst_ms = render_ms = 0;
        }

        Uint64 now_pc = SDL_GetPerformanceCounter();
        if (now_pc < next_frame) {
            SDL_Delay((Uint32)((next_frame - now_pc) * 1000 / pf));
            next_frame += frame_ticks;
        } else if (now_pc - next_frame > frame_ticks) {
            next_frame = now_pc + frame_ticks;      /* fell behind, resync */
        } else {
            next_frame += frame_ticks;
        }
    }

    audio_quit();
    SDL_DestroyRenderer(r);
    SDL_FreeSurface(surf);
    free(back);
    SDL_Quit();
    return 0;
}
