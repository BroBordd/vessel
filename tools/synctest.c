/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * PC test for game/sync.c (the music <-> screen clock and timeline). no SDL, no device:
 *   cc -O2 -pthread -o /tmp/synctest tools/synctest.c game/sync.c -lm && /tmp/synctest
 * exits 0 when everything passes. */
#define _GNU_SOURCE
#include "../game/sync.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static double rnd(void) { return (double)rand() / RAND_MAX; }

/* one plain chunk of a track playing at normal speed */
static SyncChunk plain(double f0, double p0, unsigned track, double dur) {
    SyncChunk c; memset(&c, 0, sizeof c);
    c.f0 = f0; c.n = 1024; c.nm = 1024; c.p0 = p0; c.adv = 1024.0 / SYNC_RATE; c.dur = dur; c.track = track;
    return c;
}

static void test_clock(void) {
    printf("-- clock\n");
    sync_reset();
    CHECK(!sync_ready(), "ready before any stamp");
    /* the truth: stream frame 0 is heard at tau = 5000.0 s. stamps jitter +-3 ms */
    const double tau = 5000.0;
    srand(7);
    for (int i = 0; i < 400; i++) {
        double F = 44100.0 * (1.0 + i * 0.05);
        double T = tau + F / SYNC_RATE + (rnd() - 0.5) * 0.006;
        sync_stamp(1, F, T);
    }
    CHECK(sync_ready() && sync_hw(), "hw mode not on");
    double f = sync_frame_at(tau + 30.0);
    CHECK(fabs(f - 30.0 * SYNC_RATE) < 0.0006 * SYNC_RATE, "hw clock off by %.2f ms", (f - 30.0 * SYNC_RATE) / SYNC_RATE * 1000);

    /* estimates are ignored while the hardware speaks */
    for (int i = 0; i < 50; i++) sync_stamp(0, 44100.0 * i, tau + 0.5 + i);
    f = sync_frame_at(tau + 30.0);
    CHECK(fabs(f - 30.0 * SYNC_RATE) < 0.0006 * SYNC_RATE, "estimate leaked into the hw clock");

    /* a real jump of 80 ms (audio paused and came back): one or two odd stamps are ignored, the third snaps */
    double F = 44100.0 * 40.0, T = tau + 40.0 + 0.080;
    sync_stamp(1, F, T);
    double f1 = sync_frame_at(tau + 41.0);
    CHECK(fabs(f1 - 41.0 * SYNC_RATE) < 0.001 * SYNC_RATE, "one odd stamp moved the clock");
    sync_stamp(1, F + 2205, T + 0.05); sync_stamp(1, F + 4410, T + 0.10);
    double f3 = sync_frame_at(tau + 41.0);
    CHECK(fabs(f3 - (41.0 - 0.080) * SYNC_RATE) < 0.001 * SYNC_RATE, "clock did not snap after 3 stamps: %.1f ms", (f3 - 41.0 * SYNC_RATE) / SYNC_RATE * 1000);

    /* a lone outlier in between good stamps never counts */
    sync_reset();
    for (int i = 0; i < 20; i++) sync_stamp(1, 44100.0 * i, tau + i);
    for (int i = 0; i < 6; i++) { sync_stamp(1, 44100.0 * (20 + 2 * i), tau + 20 + 2 * i + 0.09); sync_stamp(1, 44100.0 * (21 + 2 * i), tau + 21 + 2 * i); }
    f = sync_frame_at(tau + 30.0);
    CHECK(fabs(f - 30.0 * SYNC_RATE) < 0.001 * SYNC_RATE, "scattered outliers moved the clock");

    /* estimates work alone, and the hw takes over at once */
    sync_reset();
    for (int i = 0; i < 100; i++) sync_stamp(0, 44100.0 * i * 0.02, tau + 0.2 + i * 0.02 + (rnd() - 0.5) * 0.01);
    CHECK(!sync_hw() && sync_ready(), "estimate mode wrong");
    f = sync_frame_at(tau + 0.2 + 5.0);
    CHECK(fabs(f - 5.0 * SYNC_RATE) < 0.004 * SYNC_RATE, "estimate clock off by %.2f ms", (f - 5.0 * SYNC_RATE) / SYNC_RATE * 1000);
    sync_stamp(1, 0.0, tau + 0.35);
    f = sync_frame_at(tau + 0.35 + 5.0);
    CHECK(fabs(f - 5.0 * SYNC_RATE) < 1.0, "hw did not take over at once");

    /* the app stops sending: after 3 s back to the estimate */
    printf("   (waiting 3.2 s for the hw watchdog)\n");
    usleep(3200000);
    CHECK(!sync_hw(), "hw watchdog never fired");
}

static void test_timeline(void) {
    printf("-- timeline\n");
    double pos, spd;
    unsigned long long dummy = 0; (void)dummy;
    /* track 1 plays 200 chunks at normal speed from position 3.0 s, length 100 s */
    double f0 = 1e6;     /* the stream does not have to start at 0 */
    for (int i = 0; i < 200; i++) { SyncChunk c = plain(f0 + i * 1024.0, 3.0 + i * 1024.0 / SYNC_RATE, 1, 100.0); c.spec[0] = (float)i; sync_push(&c); }
    for (int i = 0; i < 2000; i++) {
        double f = f0 + 10.0 + rnd() * (200 * 1024 - 20);
        int st = sync_position_at(f, 1, &pos, &spd);
        double want = 3.0 + (f - f0) / SYNC_RATE;
        CHECK(st == SYNC_OK && fabs(pos - want) < 1e-9 && fabs(spd - 1.0) < 1e-9, "plain: frame %.1f -> %.6f want %.6f (st %d)", f, pos, want, st);
        if (fails > 5) return;
    }
    CHECK(sync_position_at(f0 - 5000, 1, &pos, &spd) == SYNC_NONE, "a frame before everything should be NONE");
    CHECK(sync_position_at(f0 + 50 * 1024.0, 2, &pos, &spd) == SYNC_NOT_ARRIVED, "an older track should be NOT_ARRIVED");
    /* past the newest chunk: clamps to its end */
    CHECK(sync_position_at(f0 + 1e7, 1, &pos, &spd) == SYNC_OK && fabs(pos - (3.0 + 200 * 1024.0 / SYNC_RATE)) < 1e-6, "past the end: %.6f", pos);

    /* spectrum: chunk i describes the instant f0 + 1024 * (i + 1) - 256, values rise by 1 per chunk: linear in between */
    float sp[SYNC_BANDS];
    sync_spectrum_at(f0 + 1024.0 * 50 + 768 + 512, sp);          /* half way between chunk 50 and 51 */
    CHECK(fabs(sp[0] - 50.5f) < 1e-3, "spectrum interpolation %.4f", sp[0]);
    sync_spectrum_at(f0 + 1024.0 * 80 + 768, sp);
    CHECK(fabs(sp[0] - 80.0f) < 1e-3, "spectrum exact %.4f", sp[0]);
    sync_spectrum_at(f0 - 10, sp);
    CHECK(sp[0] == 0.0f, "spectrum before everything should be zeros");

    /* pause: the next chunks do not move, speed 0, position holds */
    double fp = f0 + 200 * 1024.0;
    for (int i = 0; i < 10; i++) { SyncChunk c = plain(fp + i * 1024.0, 9.0, 1, 100.0); c.adv = 0; sync_push(&c); }
    CHECK(sync_position_at(fp + 5 * 1024.0 + 100, 1, &pos, &spd) == SYNC_OK && fabs(pos - 9.0) < 1e-9 && spd == 0.0, "pause: %.6f speed %.3f", pos, spd);
    /* ... and a seek: the position jumps at the chunk boundary, not before */
    double fs = fp + 10 * 1024.0;
    for (int i = 0; i < 10; i++) sync_push(&(SyncChunk){0}), (void)0;      /* (silence chunks: track 0) */
    CHECK(sync_position_at(fs + 3 * 1024.0, 1, &pos, &spd) == SYNC_NONE, "silence should be NONE");
    double fq = fs + 10 * 1024.0;
    for (int i = 0; i < 5; i++) { SyncChunk c = plain(fq + i * 1024.0, 60.0 + i * 1024.0 / SYNC_RATE, 1, 100.0); sync_push(&c); }
    CHECK(sync_position_at(fq - 1, 1, &pos, &spd) == SYNC_NONE, "just before the seek");
    CHECK(sync_position_at(fq, 1, &pos, &spd) == SYNC_OK && fabs(pos - 60.0) < 1e-9, "seek landed at %.6f", pos);

    /* loop: 2.0 s track, chunk straddles the end. position goes 1.99, wraps to 0.0x and carries on */
    double fl = fq + 5 * 1024.0;
    double dur = 2.0, p0 = dur - 600.0 / SYNC_RATE;                          /* 600 frames before the end */
    SyncChunk w = plain(fl, p0, 3, dur); w.wrapped = 1; w.adv = (dur - p0) + (1024.0 - 600.0) / SYNC_RATE; sync_push(&w);
    CHECK(sync_position_at(fl + 300, 3, &pos, &spd) == SYNC_OK && fabs(pos - (p0 + 300.0 / SYNC_RATE)) < 1e-9, "before the wrap %.6f", pos);
    CHECK(sync_position_at(fl + 600 + 100, 3, &pos, &spd) == SYNC_OK && fabs(pos - 100.0 / SYNC_RATE) < 1e-6, "after the wrap %.6f (want %.6f)", pos, 100.0 / SYNC_RATE);
    CHECK(sync_position_at(fl + 600 + 100, 3, &pos, &spd) == SYNC_OK && fabs(spd - 1.0) < 1e-6, "speed over the wrap %.6f", spd);

    /* tape scrub backwards: adv negative */
    double fb = fl + 1024.0;
    for (int i = 0; i < 4; i++) { SyncChunk c = plain(fb + i * 1024.0, 50.0 - i * 2.0 * 1024.0 / SYNC_RATE, 3, 100.0); c.adv = -2.0 * 1024.0 / SYNC_RATE; sync_push(&c); }
    CHECK(sync_position_at(fb + 1024.0 + 512, 3, &pos, &spd) == SYNC_OK && fabs(spd + 2.0) < 1e-9 && fabs(pos - (50.0 - 2.0 * 1536.0 / SYNC_RATE)) < 1e-9, "tape back: %.6f speed %.3f", pos, spd);

    /* track change: old chunks heard under the new id = NOT_ARRIVED; the new track's chunks = OK */
    double ft = fb + 4 * 1024.0;
    for (int i = 0; i < 3; i++) { SyncChunk c = plain(ft + i * 1024.0, i * 1024.0 / SYNC_RATE, 4, 90.0); sync_push(&c); }
    CHECK(sync_position_at(fb + 100, 4, &pos, &spd) == SYNC_NOT_ARRIVED, "old track under the new id");
    CHECK(sync_position_at(ft + 1500, 4, &pos, &spd) == SYNC_OK && fabs(pos - 1500.0 / SYNC_RATE) < 1e-9, "new track %.6f", pos);
}

/* a writer hammering the ring while a reader asks: no crash and every answer is consistent with what was written */
static volatile int stop;
static void *writer(void *arg) {
    (void)arg;
    double f = 5e6;
    for (long i = 0; i < 400000 && !stop; i++) {
        SyncChunk c = plain(f, f / SYNC_RATE, 9, 1e9); c.spec[0] = (float)(f / 1024.0); c.spec[1] = c.spec[0];
        sync_push(&c); f += 1024.0;
        if ((i & 1023) == 0) usleep(100);
    }
    stop = 1;
    return NULL;
}
static void test_threads(void) {
    printf("-- writer thread vs reader\n");
    pthread_t th; pthread_create(&th, NULL, writer, NULL);
    long asked = 0, bad = 0;
    while (!stop) {
        double f = sync_newest() - 1024.0 * (3 + rand() % 200);
        if (f < 5e6) { usleep(50); continue; }
        double pos, spd;
        int st = sync_position_at(f, 9, &pos, &spd);
        asked++;
        if (st == SYNC_OK && fabs(pos - f / SYNC_RATE) > 1e-6) bad++;
        float sp[SYNC_BANDS]; sync_spectrum_at(f, sp);
        if (sp[0] != sp[1]) bad++;             /* torn copy */
    }
    pthread_join(th, NULL);
    CHECK(asked > 1000, "reader asked only %ld times", asked);
    CHECK(bad == 0, "%ld inconsistent answers out of %ld", bad, asked);
    printf("   %ld queries while the ring was being rewritten\n", asked);
}

int main(void) {
    test_clock();
    test_timeline();
    test_threads();
    printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
