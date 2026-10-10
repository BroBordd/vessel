/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the "New task added" toast over a fake sky, writes build/toast_*.bmp (no audio device needed):
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/toasttest tools/toasttest.c game/toast.c game/missions.c game/hud.c game/nowplaying.c game/musicwin.c game/char.c game/audio.c game/analyze.c game/jukebox.c game/font.c $(sdl2-config --libs) -lm
 *   ./build/toasttest 540 1170 */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "hud.h"
#include "missions.h"
#include "toast.h"

static const Person VAS = { "Vas", { 96, 58, 36 }, { 248, 208, 170 }, { 214, 60, 60 }, { 52, 70, 140 }, { 40, 30, 30 }, 0, 0 };

static const Person AON = { "Aonia", { 96, 58, 36 }, { 248, 208, 170 }, { 214, 60, 60 }, { 52, 70, 140 }, { 40, 30, 30 }, 0, 0 };

int main(int argc, char **argv) {
    int W = argc > 1 ? atoi(argv[1]) : 540, H = argc > 2 ? atoi(argv[2]) : 1170;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    float u = (W < H ? W : H) / 360.0f;
    missions_init(W, H); toast_init(W, H); hud_init(W, H, &VAS);
    task_toast("Find the orb");                     /* shows now */
    task_toast("Ask Alex");                         /* waits its turn */
    hud_set_person(&AON);                           /* \"YOU ARE NOW AONIA\": the same panel, third in the queue */
    static const float shots[] = { 0.10f, 1.0f, 3.4f, 4.3f, 7.0f, 8.0f };
    int si = 0, shown = 0;
    for (float t = 0; si < 6; t += 0.01f) {
        missions_update(0.01f); toast_update(0.01f); hud_update(0.01f);
        if (t >= shots[si]) {
            SDL_SetRenderDrawColor(r, 126, 188, 246, 255); SDL_RenderClear(r);
            missions_draw(r);
            toast_draw(r, (int)(10 * u), missions_bottom() + (int)(4 * u));
            hud_draw(r);
            char n[64]; snprintf(n, sizeof n, "build/toast_%d.bmp", si); SDL_SaveBMP(surf, n);
            printf("%s t=%.2f bottom=%d\n", n, t, missions_bottom()); si++; shown++;
        }
    }
    return shown == 6 ? 0 : 1;
}
