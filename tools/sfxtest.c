/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the heart monitor sounds (chunk 13): includes audio.c so it can run the mixer by hand, with the audio device
 * locked so the dummy driver's own thread stays out of the way. checks the peep (short, ~1 kHz, decays), the flatline
 * (steady for ~3 s, then silent), the early stop, and that a peep replaces a flatline. exit != 0 on a failed check.
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/sfxtest tools/sfxtest.c game/analyze.c $(sdl2-config --libs) -lm && SDL_AUDIODRIVER=dummy build/sfxtest */
#define SDL_MAIN_HANDLED
#include "../game/audio.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("  !! " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static Sint16 buf[2048 * 2];
/* mixes `sec` seconds, returns the peak of the LAST 50 ms (or of everything with whole = 1); zc gets zero crossings of the left channel */
static int render(float sec, int whole, int *zc) {
    int total = (int)(sec * OUT_RATE), peak = 0, z = 0, prev = 0, from = whole ? 0 : total - OUT_RATE / 20;
    for (int done = 0; done < total; ) {
        int n = total - done < 2048 ? total - done : 2048;
        audio_cb(NULL, (Uint8 *)buf, n * 4);
        for (int i = 0; i < n; i++) {
            int v = buf[i * 2], a = v < 0 ? -v : v;
            if (done + i >= from && a > peak) peak = a;
            if (zc && ((v > 0) != (prev > 0))) z++;
            prev = v;
        }
        done += n;
    }
    if (zc) *zc = z;
    return peak;
}

int main(void) {
    SDL_Init(SDL_INIT_AUDIO);
    if (audio_init() != 0) { printf("no audio device (dummy driver?): skipped\n"); return 0; }
    SDL_LockAudioDevice(dev);
    music_stop();

    int zc, peak;
    printf("peep\n");
    sfx_beep();
    peak = render(0.10f, 1, &zc);
    printf("  peak %d, zero crossings in 100 ms: %d (1 kHz = 200)\n", peak, zc);
    CHECK(peak > 3000, "the peep is too quiet");
    CHECK(zc > 150 && zc < 250, "the peep is not around 1 kHz");
    CHECK(render(0.40f, 0, NULL) == 0, "the peep did not end");

    printf("flatline\n");
    sfx_flatline();
    int p1 = render(0.50f, 0, NULL), p2 = render(1.50f, 0, NULL);
    printf("  level at 0.5 s: %d, at 2.0 s: %d\n", p1, p2);
    CHECK(p1 > 3000 && p2 > 3000, "the flatline is too quiet");
    CHECK(abs(p1 - p2) < 400, "the flatline is not steady (%d vs %d)", p1, p2);
    peak = render(1.50f, 0, NULL);
    CHECK(peak == 0, "the flatline did not end after ~3 s (%d)", peak);

    printf("flatline cut short\n");
    sfx_flatline(); render(0.50f, 0, NULL);
    sfx_flatline_stop();
    CHECK(render(0.15f, 0, NULL) == 0, "the flatline did not stop");
    sfx_flatline_stop();                                         /* nothing sounding: must be harmless */
    sfx_beep(); CHECK(render(0.05f, 1, NULL) > 3000, "a peep after a stop is silent");

    printf("a peep replaces a flatline\n");
    sfx_flatline(); render(0.30f, 0, NULL); sfx_beep();
    CHECK(render(0.60f, 0, NULL) == 0, "the flatline kept going under the peep");

    CHECK(sfx_debug_beeps() == 3 && sfx_debug_flatlines() == 3, "call counters off (%d beeps, %d flatlines)", sfx_debug_beeps(), sfx_debug_flatlines());
    SDL_UnlockAudioDevice(dev);
    audio_quit();
    printf("%s\n", fails ? "FAILED" : "all checks passed");
    return fails != 0;
}
