/* headless: push a dragged body at the pit (world.c is included for its statics)
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/pushtest tools/pushtest.c $(ls game/*.c | grep -v 'game/game.c\|game/world.c') $(sdl2-config --libs) -lm
 *   SDL_AUDIODRIVER=dummy build/pushtest [dx dy] */
#define SDL_MAIN_HANDLED
#include "../game/world.c"
static int fell_cb; static void on_fell(unsigned v) { fell_cb++; (void)v; }
int main(int argc, char **argv) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_setenv("VESSEL_AT", "edge", 1); if (!getenv("VESSEL_SEED")) SDL_setenv("VESSEL_SEED", "1", 1);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, getenv("TW")?atoi(getenv("TW")):540, getenv("TH")?atoi(getenv("TH")):960, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    world_init(getenv("TW")?atoi(getenv("TW")):540, getenv("TH")?atoi(getenv("TH")):960);
    for (float t = 0; t < 16; t += 0.016f) { world_update(0.016f); }
    world_set_pit_cb(on_fell);
    int fx, fy, ex, ey; world_pit_spots(&fx, &fy, &ex, &ey);
    if (getenv("V")) printf("pit box t(%d,%d)-(%d,%d) edge(%d,%d) controls=%d\n", pit_x0, pit_y0, pit_x1, pit_y1, ex, ey, controls_visible);
    if (getenv("V")) for (int y = 8; y < 20; y++) { printf("%2d ", y); for (int x = 20; x < 31; x++) printf("%d", map[y][x]); printf("\n"); }
    float dy = argc > 2 ? atof(argv[2]) : 1.0f;
    float bty = argc > 1 ? atof(argv[1]) : 1.2f;
    float ox = argc > 3 ? atof(argv[3]) : 0.0f; float sx = getenv("SX") ? atof(getenv("SX")) : 0.0f; pxp = (ex + 0.5f + ox + sx) * tile; pyp = (ey + 0.5f + dy) * tile;
    int V1 = getenv("V1") != NULL;
    if (V1) {   /* push Aonia (vessel 1) instead: put him where the other body would lie */
        float a, b, c, d, hx, hy; if (!ritual_body_box(&a, &b, &c, &d)) { printf("RESULT skip(no vessel 1 box)\n"); return 0; }
        ritual_home(&hx, &hy);
        float bxc = (a + c) * 0.5f, byc = d;                            /* BODY_X, BODY_Y */
        float tx = (ex + 0.5f + ox) * tile, ty = (ey + 0.5f + bty) * tile;
        ritual_body_shift((tx - hx * tile) / px - bxc, (ty - hy * tile) / px - byc);
        printf("vessel1 in list: %d\n", bodies_find(1) != NULL);
    }
    int bi = V1 ? 0 : bodies_add(7, ex + 0.5f + ox, ey + 0.5f + bty, NULL);
    if (getenv("V")) printf("body idx %d, player tile y %.2f, body y %.2f\n", bi, pyp / tile, ey + 0.5f + bty);
    drag_pick(); if (drag_cand != (V1 ? 1 : 7)) { printf("RESULT skip(cand %d)\n", drag_cand); return 0; }
    drag_toggle();
    kx = getenv("KX")?atof(getenv("KX")):0; ky = -(getenv("MAG")?atof(getenv("MAG")):1.0f); float DT = getenv("DT")?atof(getenv("DT")):0.016f;
    float aim = getenv("AIM") ? atof(getenv("AIM")) : 0.0f;
    for (int i = 0; i < 600; i++) {
        if (aim > 0 && drag_on) { float a0, b0, a1, b1; if (drag_box_px(&a0, &b0, &a1, &b1)) { float e = ((a0 + a1) * 0.5f - pxp) / (float)tile; kx = e * aim; if (kx > 1) kx = 1; if (kx < -1) kx = -1; } }
        world_update(DT);
        float x0, y0, x1, y1; int have = drag_on && drag_box_px(&x0, &y0, &x1, &y1);
        if (i % 10 == 0 || !drag_on)
            if (getenv("V")) printf("f%03d player x=%.2f y=%.2f body %s mid_y=%.2f (pit top y=%d) drag_on=%d fell_cb=%d\n", i, pxp / tile, pyp / tile, have ? "" : "-", have ? (y0 + y1) / 2 / tile : -1.0f, pit_y0, drag_on, fell_cb);
        if (!drag_on && i > 5) { for (int k = 0; k < 150; k++) world_update(DT); break; }
    }
    if (getenv("RELOAD") && fell_cb) {   /* the death after a jump: the ruins again, the ritual draws vessel 1 lying at the circle (story_die) */
        world_reload_ruins(); world_ritual(story_vessel_look(1), NULL);
        for (int k = 0; k < 600; k++) world_update(DT);
        float a, b, c, d; printf("after reload: vessel 1 box present = %d, in list = %d\n", ritual_body_box(&a, &b, &c, &d), bodies_find(1) != NULL);
    }
    printf("RESULT %s\n", fell_cb ? "FELL" : "STUCK");
    return 0;
}
