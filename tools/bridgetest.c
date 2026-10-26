/* headless: push 8 bodies into the pit one after another and check the bridge (4 bodies = the near half is floor, 8 = the far ledge can be walked to)
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/bridgetest tools/bridgetest.c $(ls game/*.c | grep -v 'game/game.c\|game/world.c') $(sdl2-config --libs) -lm
 *   SDL_AUDIODRIVER=dummy build/bridgetest */
#define SDL_MAIN_HANDLED
#include "../game/world.c"
static int fell_cb; static void on_fell(unsigned v) { fell_cb++; world_pit_add(); (void)v; }
static int walk_to(float tx, float ty, float secs) {   /* walk (stick) toward a tile point; returns 1 when there */
    for (float t = 0; t < secs; t += 0.016f) {
        float dx = tx * tile - pxp, dy = ty * tile - pyp; float l = hypotf(dx, dy);
        if (l < 2.0f) { kx = ky = 0; return 1; }
        kx = dx / l; ky = dy / l; world_update(0.016f);
    }
    kx = ky = 0; return 0;
}
int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_setenv("VESSEL_AT", "edge", 1); if (!getenv("VESSEL_SEED")) SDL_setenv("VESSEL_SEED", "1", 1);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, 540, 960, 32, SDL_PIXELFORMAT_RGBA32); SDL_Renderer *rr = SDL_CreateSoftwareRenderer(surf);
    int shot_after = getenv("SHOT") ? atoi(getenv("SHOT_AFTER") ? getenv("SHOT_AFTER") : "1") : -1;
    world_init(540, 960);
    for (float t = 0; t < 16; t += 0.016f) world_update(0.016f);
    world_set_pit_cb(on_fell);
    int fx, fy, ex, ey; world_pit_spots(&fx, &fy, &ex, &ey);
    printf("hole rows %d..%d (%d), bridge column %d, left %d\n", pit_y0, pit_y1, pit_y1 - pit_y0 + 1, ex, world_pit_left());
    for (int i = 0; i < 8; i++) {
        float by, py;
        if (i < 4) { py = ey + 0.5f; by = ey + 0.5f - 0.8f; }                 /* from the south ledge */
        else       { py = pit_y1 - 1 + 0.95f; by = pit_y1 - 1 + 0.2f; }       /* from the end of the near platform (row pit_y1-2) */
        pxp = (ex + 0.5f) * tile; pyp = py * tile; kx = ky = 0;
        int id = 10 + i; bodies_add(id, ex + 0.5f, by, NULL);
        drag_pick(); if (drag_cand != id) { printf("body %d: cannot grab (cand %d)\n", i + 1, drag_cand); return 1; }
        drag_toggle();
        int before = fell_cb;
        kx = 0; ky = -1;
        for (int f = 0; f < 600 && fell_cb == before; f++) { world_update(0.016f); if (i == 0 && f == 20) printf("hands: grip behind (hidden) = %d, grip blend %.2f\n", dg_behind, dg_k); }
        kx = ky = 0; for (int f = 0; f < 90; f++) world_update(0.016f);        /* the fall plays, the body lands */
        int done = fell_cb > before;
        printf("body %d: %s, pile %d, bodies at the bottom %d, bridge tiles left %d\n", i + 1, done ? "fell" : "STUCK", pit_pile, landed_n, world_pit_left());
        if (!done) return 1;
        if (shot_after == i + 1) {
            pxp = (ex + 0.5f) * tile; pyp = (ey + 0.2f) * tile; kx = ky = 0; for (int f = 0; f < 30; f++) world_update(0.016f);
            world_draw(rr); SDL_SaveBMP(surf, getenv("SHOT")); printf("saved %s\n", getenv("SHOT"));
        }
        if (i == 3 || i == 7) {
            int open = 0; for (int y = pit_y0; y <= pit_y1; y++) open += map[y][ex] == T_RPIT;
            printf("  -> after %d bodies the bridge column has %d open tiles\n", i + 1, open);
        }
    }
    /* walk across: from the south ledge to the far ledge along the bridge column */
    pxp = (ex + 0.5f) * tile; pyp = (ey + 0.5f) * tile; drag_on = 0;
    int ok = walk_to(ex + 0.5f, pit_y1 + 0.6f, 6) && walk_to(ex + 0.5f, pit_y0 - 1.5f, 12);
    printf("walk across: %s (player y %.2f, far ledge is above row %d)\n", ok ? "REACHED" : "BLOCKED", pyp / tile, pit_y0);
    return 0;
}
