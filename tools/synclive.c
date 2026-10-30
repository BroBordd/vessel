/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LIVE PC test of the music <-> screen sync: the real audio.c runs on SDL's disk driver into a FIFO, and this program plays the app:
 * it pretends the speaker is a fixed delay behind, sends the speaker timestamps (or not), and
 * compares music_heard_position() with the true position at the moment the picture would reach the eye.
 *   cc -O1 $(sdl2-config --cflags) -Igame -o /tmp/synclive tools/synclive.c game/sync.c game/analyze.c $(sdl2-config --libs) -lm -lpthread
 *   cd music && /tmp/synclive third_life.ogg [hardware_delay_ms]
 * prints the error per phase (estimate only, then hardware stamps), exits 1 if the hardware phase is off by more than 4 ms. */
#define SDL_MAIN_HANDLED
#include "../game/audio.c"
#include <pthread.h>
#include <time.h>

static volatile int send_stamps;
static double t_play0;                          /* when stream frame 0 is heard */
static volatile int run = 1;

/* the pretend pipeline. SDL's own pacing is too crude for ms accuracy (its dummy driver sleeps whole milliseconds), so this thread calls the
 * real audio callback itself, exactly 1024 frames per 23.2 ms after a first burst of K0 chunks (what fills the FIFO and the AudioTrack
 * buffer); the speaker plays stream frame f at t0 + f / 44100 + HW0. so the pipeline is K0 chunks + HW0 deep, like a saturated one. */
static const int K0 = 6;
static double HW0 = 0.08;
#define SPKD (K0 * 1024.0 / SYNC_RATE + HW0)     /* the true delay between mixing a frame and hearing it */

static void sleep_until(double t) { double d; while ((d = t - sync_now()) > 0) usleep(d > 0.002 ? (useconds_t)((d - 0.001) * 1e6) : 100); }

static void *producer(void *a) {
    (void)a;
    static Sint16 buf[4096];
    const double dt = 1024.0 / SYNC_RATE;
    double t0 = sync_now(); t_play0 = t0 + HW0;
    for (long k = 0; run; k++) {
        if (k >= K0) sleep_until(t0 + (double)(k - K0 + 1) * dt);
        SDL_LockAudioDevice(dev); audio_cb(NULL, (Uint8 *)buf, 8192); SDL_UnlockAudioDevice(dev);
    }
    return NULL;
}

/* the app: the speaker's timestamps, 25 per second, +-1 ms quantised, when asked to */
static void *app(void *a) {
    (void)a;
    while (run) {
        usleep(40000);
        if (!send_stamps || !t_play0) continue;
        double t = sync_now() + ((rand() % 2001) - 1000) * 1e-6;
        if (t > t_play0) audio_hw_stamp((long long)((t - t_play0) * SYNC_RATE), (long long)(t * 1e9));
    }
    return NULL;
}

static double truth(double f0m, double t_eye) { return ((t_eye - t_play0) * SYNC_RATE - f0m) / SYNC_RATE; }

int main(int argc, char **argv) {
    const char *ogg = argc > 1 ? argv[1] : "third_life.ogg";
    if (argc > 2) HW0 = atof(argv[2]) / 1000.0;
    setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (audio_init() != 0) return 2;
    SDL_PauseAudioDevice(dev, 1);                                       /* SDL's own callback thread stays out: the producer below drives audio_cb */
    pthread_t th, th2; pthread_create(&th, NULL, app, NULL); pthread_create(&th2, NULL, producer, NULL);
    audio_set_refresh(60.0f);
    usleep(2600000);                                                    /* the estimate wants 2 s */
    SDL_LockAudioDevice(dev);                                           /* (recursive) no chunk is produced between reading the index and the track starting */
    double f0m = (double)produced_frames;
    music_play(ogg, 1);
    SDL_UnlockAudioDevice(dev);
    int fail = 0;
    for (int phase = 0; phase < 2; phase++) {
        send_stamps = phase;
        if (phase) usleep(1500000);                                     /* the stamps need a moment to settle */
        double sum = 0, mx = 0; int n = 0;
        for (int i = 0; i < 60; i++) {
            usleep(50000 + (rand() % 7000));
            double t_eye = sync_now() + (double)disp_delay;
            double hp = music_heard_position();
            double err = hp - truth(f0m, t_eye);
            if (hp > 0.3) { sum += err; if (fabs(err) > mx) mx = fabs(err); n++; }
        }
        printf("%s: speaker really %.0f ms behind; position error mean %+.1f ms, worst %.1f ms (%d samples)\n",
               phase ? "hardware stamps" : "estimate only  ", SPKD * 1000, n ? sum / n * 1000 : 0, mx * 1000, n);
        if (phase && (n < 30 || mx > 0.004)) fail = 1;     /* (the estimate phase only informs: SDL's dummy driver paces ~1% fast, so the pipeline it fakes keeps growing) */
    }
    /* a pause is HEARD when the speaker gets there, not when it is asked for */
    music_pause(1);
    double t_ask = sync_now(); int saw = 0; double t_saw = 0;
    while (sync_now() - t_ask < 1.0) { if (!music_heard_playing()) { saw = 1; t_saw = sync_now(); break; } usleep(2000); }
    printf("pause asked; heard as paused after %.0f ms (pipeline ~%.0f ms)\n", saw ? (t_saw - t_ask) * 1000 : -1.0, (SPKD - (double)disp_delay) * 1000);
    if (!saw || t_saw - t_ask < SPKD * 0.5 || t_saw - t_ask > SPKD + 0.3) fail = 1;     /* the paused chunk reaches the speaker ~SPKD later, minus the display delay we look ahead */
    run = 0; audio_quit();
    printf(fail ? "FAIL\n" : "ok\n");
    return fail;
}
