/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * headless check of the two multitouch windows: a pinch on the map window zooms (and a lifted finger leaves the other one panning), and
 * two fingers on the music window's piano hold two keys at once (a chord) and let go one by one:
 *   cc -O1 -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -o build/multitouchtest tools/multitouchtest.c game/audio.c game/analyze.c game/jukebox.c game/musicwin.c game/mapwin.c game/mapicon.c game/font.c game/gfx.c game/sync.c $(sdl2-config --libs) -lm
 *   cd music && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ../build/multitouchtest */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include "audio.h"
#include "mapwin.h"
#include "musicwin.h"

static int fails;
void item_draw_icon(SDL_Renderer *r, int kind, int cx, int cy, int cell) { (void)r; (void)kind; (void)cx; (void)cy; (void)cell; }   /* mapicon.c draws the items through this; the test needs no pictures */
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static uint8_t tiles[64 * 64];
static const Rgb pal[2] = { { 20, 80, 20 }, { 90, 90, 90 } };
static void fill(MapView *v) {
    v->tiles = tiles; v->stride = 64; v->mw = 64; v->mh = 64; v->pal = pal; v->npal = 2; v->ptx = 32; v->pty = 32; v->facing = 0;
}

int main(void) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1); SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    int W = 540, H = 1170;
    audio_init();

    /* ---- the map: pinch ---- */
    mapwin_init(W, H); mapwin_set_source(fill); mapwin_open();
    CHECK(mapwin_active(), "the map did not open");
    SDL_Rect wn, vw, zi, zo, yo, cl; mapwin_debug_rects(&wn, &vw, &zi, &zo, &yo, &cl);
    int cx = vw.x + vw.w / 2, cy = vw.y + vw.h / 2;
    mapwin_touch(0, zi.x + 3, zi.y + 3); mapwin_touch(1, zi.x + 3, zi.y + 3);             /* zoom in once so there is room to zoom both ways */
    float z0 = mapwin_debug_zoom(), x0, y0; mapwin_debug_center(&x0, &y0);

    mapwin_touch(0, cx - 40, cy); mapwin_touch_p(4, 0, cx + 40, cy);                      /* two fingers down on the map */
    for (int i = 1; i <= 10; i++) { mapwin_touch(2, cx - 40 - i * 6, cy); mapwin_touch_p(4, 2, cx + 40 + i * 6, cy); }   /* spread: 80 -> 200 px apart, same midpoint */
    float z1 = mapwin_debug_zoom(), x1, y1; mapwin_debug_center(&x1, &y1);
    printf("spread: zoom %.2f -> %.2f, centre (%.2f,%.2f) -> (%.2f,%.2f)\n", z0, z1, x0, y0, x1, y1);
    CHECK(z1 > z0 * 1.2f, "pinching apart did not zoom in");
    CHECK(fabsf(x1 - x0) < 0.3f && fabsf(y1 - y0) < 0.3f, "a pinch around the middle moved the view");

    for (int i = 1; i <= 10; i++) { mapwin_touch(2, cx - 100 + i * 6, cy); mapwin_touch_p(4, 2, cx + 100 - i * 6, cy); }  /* and together again */
    float z2 = mapwin_debug_zoom();
    printf("together: zoom %.2f\n", z2);
    CHECK(z2 < z1 * 0.8f, "pinching together did not zoom out");

    /* the midpoint pans: both fingers move right by 60 px at the same distance = the view moves left (the map follows the fingers) */
    float px0, py0; mapwin_debug_center(&px0, &py0); float zp = mapwin_debug_zoom();
    mapwin_touch(2, cx - 40 + 60, cy); mapwin_touch_p(4, 2, cx + 40 + 60, cy);
    float px1, py1; mapwin_debug_center(&px1, &py1);
    CHECK(fabsf(mapwin_debug_zoom() - zp) < 0.01f, "a two-finger pan changed the zoom");
    CHECK(px1 < px0 - 0.1f, "a two-finger pan did not move the map (%.2f -> %.2f)", px0, px1);

    mapwin_touch_p(4, 1, cx + 100, cy);                                                   /* the second finger lifts: the first carries on dragging */
    float qa, qb; mapwin_debug_center(&qa, &qb);
    mapwin_touch(2, cx + 20 - 40, cy);
    float qc, qd; mapwin_debug_center(&qc, &qd);
    CHECK(fabsf(qc - qa) > 0.05f, "after the second finger lifted the first one no longer drags");
    mapwin_touch(1, cx - 20, cy);

    /* the first finger lifts first: the second one pans, and a new first finger pinches again */
    mapwin_touch(0, cx - 40, cy); mapwin_touch_p(5, 0, cx + 40, cy);
    mapwin_touch(1, cx - 40, cy);
    float ra, rb; mapwin_debug_center(&ra, &rb);
    mapwin_touch_p(5, 2, cx + 40 - 30, cy);
    float rc, rd; mapwin_debug_center(&rc, &rd);
    CHECK(fabsf(rc - ra) > 0.05f, "after the first finger lifted the second one does not pan");
    float zr0 = mapwin_debug_zoom();
    mapwin_touch(0, cx - 80, cy);                                                         /* the first finger is back: a pinch again */
    mapwin_touch(2, cx - 140, cy);
    CHECK(mapwin_debug_zoom() > zr0, "a returning first finger did not resume the pinch");
    mapwin_touch(1, cx - 140, cy); mapwin_touch_p(5, 1, cx + 10, cy);
    CHECK(mapwin_active(), "a pinch closed the map");

    /* a second finger on a button does nothing; a cancel lets everything go */
    mapwin_touch(0, zi.x + 3, zi.y + 3); mapwin_touch_p(6, 0, cx, cy);
    float zb = mapwin_debug_zoom(); mapwin_touch(2, zi.x + 8, zi.y + 8);
    CHECK(fabsf(mapwin_debug_zoom() - zb) < 0.001f, "a finger pressed on a button pinched");
    mapwin_touch(3, 0, 0); mapwin_touch_p(6, 3, 0, 0);
    mapwin_close();

    /* ---- the piano: a chord ---- */
    musicwin_init(W, H); musicwin_open();
    for (int i = 0; i < 90; i++) musicwin_update(0.016f);                                 /* let it slide in */
    int ky = -1, k1 = -1, k2 = -1, xa = 0, xb = 0;
    for (int y = H / 2; y < H && ky < 0; y += 4) for (int x = 10; x < W - 10; x += 3) { int k = musicwin_debug_key_at(x, y); if (k >= 0) { ky = y; break; } }
    CHECK(ky > 0, "no piano found");
    for (int x = 10; x < W - 10; x += 2) { int k = musicwin_debug_key_at(x, ky); if (k >= 0 && k != k1) { if (k1 < 0) { k1 = k; xa = x; } else if (k != k1 && xb == 0 && x > xa + 30) { k2 = k; xb = x; } } }
    CHECK(k1 >= 0 && k2 >= 0 && k1 != k2, "could not find two keys (%d %d)", k1, k2);
    printf("piano: key %d at x=%d, key %d at x=%d, y=%d\n", k1, xa, k2, xb, ky);
    musicwin_touch(0, xa, ky);                                                            /* finger 1 plays key 1 */
    musicwin_touch_p(3, 0, xb, ky);                                                       /* finger 2 plays key 2 AT THE SAME TIME */
    CHECK(musicwin_touch_p(3, 2, xb + 1, ky) == 1, "the piano did not take the second finger");
    CHECK(musicwin_debug_held() == 2, "a chord holds %d keys, not 2", musicwin_debug_held());
    musicwin_touch_p(3, 1, xb, ky);                                                       /* finger 2 lets go: finger 1 still holds its key */
    CHECK(musicwin_debug_held() == 1, "after one finger lifted %d keys are held, not 1", musicwin_debug_held());
    musicwin_touch_p(3, 0, xb, ky); musicwin_touch_p(3, 2, xb + 40, ky);                  /* finger 2 again, slides to the next key: still ONE note for it */
    CHECK(musicwin_debug_held() == 2, "the sliding finger holds %d keys in all, not 2", musicwin_debug_held());
    musicwin_touch_p(3, 1, xb + 40, ky);
    musicwin_touch(1, xa, ky);
    CHECK(musicwin_debug_held() == 0, "keys still held after every finger lifted (%d)", musicwin_debug_held());
    musicwin_touch_p(7, 0, xa, ky); CHECK(musicwin_debug_held() == 1, "a third finger did not play");
    musicwin_touch_p(7, 3, xa, ky);                                                       /* a cancel lets go */
    CHECK(musicwin_debug_held() == 0, "a cancel left a key held");
    musicwin_touch_p(9, 0, 5, 5);                                                         /* a finger outside the keys plays nothing and takes nothing */
    musicwin_touch_p(9, 1, 5, 5);
    CHECK(musicwin_active(), "the piano touches closed the window");

    printf(fails ? "multitouchtest: %d FAILED\n" : "multitouchtest: ok\n", fails);
    audio_quit();
    return fails != 0;
}
