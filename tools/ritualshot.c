/* renders the blood circle at a few moments to build/ritual_N.bmp (software renderer, no window):
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/ritualshot tools/ritualshot.c game/ritual.c game/char.c game/vessel_gen.c game/story_stub... see below */
#include <SDL2/SDL.h>
#include <stdio.h>
#include "ritual.h"
#include "char.h"
#include "vessel_gen.h"

int main(int argc, char **argv) {
    const int W = 440, H = 240, cell = 4;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(s);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    const Person *dead = vessel_gen_person(1), *me = vessel_gen_person(2);
    ritual_begin(7.5f, 5.0f, dead);
    float times[] = { 1.2f, 2.4f, 4.0f, 5.0f, 5.8f, 6.3f, 6.9f, 8.5f };
    int n = (int)(sizeof times / sizeof *times), k = 0; float t = 0;
    while (k < n) {
        ritual_update(1.0f / 60); t += 1.0f / 60;
        if (t < times[k]) continue;
        SDL_SetRenderDrawColor(r, 58, 54, 62, 255); SDL_RenderClear(r);
        for (int y = 0; y < H / 24 + 1; y++) for (int x = 0; x < W / 24 + 1; x++) { SDL_SetRenderDrawColor(r, 66 + ((x + y) & 1) * 6, 62 + ((x + y) & 1) * 6, 70, 255); SDL_Rect q = { x * 24, y * 24, 24, 24 }; SDL_RenderFillRect(r, &q); }
        int sx = 280, sy = 130;
        ritual_draw_floor(r, sx, sy, cell, 0.45f);
        if (ritual_active()) {
            int rows = ritual_player_rows();
            if (rows > 0) { SDL_Rect c = { 0, sy - rows * cell, W, rows * cell + 4 * cell }; SDL_RenderSetClipRect(r, &c); char_set_darken(0.45f); char_draw(r, me, sx, sy, FACE_DOWN, 0, 0, cell); char_set_darken(0); SDL_RenderSetClipRect(r, NULL); }
        } else { char_set_darken(0.45f); char_draw(r, me, sx, sy, FACE_DOWN, 0, 0, cell); char_set_darken(0); }
        ritual_draw_over(r, sx, sy, cell, me);
        char fn[64]; snprintf(fn, sizeof fn, "build/ritual_%d.bmp", k); SDL_SaveBMP(s, fn);
        k++;
    }
    return 0;
}
