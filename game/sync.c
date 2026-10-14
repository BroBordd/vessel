/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * music <-> screen sync, see sync.h */
#define _GNU_SOURCE
#include "sync.h"
#include <math.h>
#include <string.h>
#include <time.h>

double sync_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---------------------------------------------------------------- the clock
 * every stamp says "frame F is heard at time T", i.e. the stream's frame 0 is heard at tau = T - F / RATE. if the speaker's clock ran
 * exactly like ours tau would never change; it only jitters (the timestamps come in quantised) and drifts a few ppm. so tau is
 * kept as a running average that follows slow changes. a real jump (the audio paused and came back, the output device changed)
 * shows as a few stamps in a row that all disagree with the average: then it snaps. */
static volatile int lk;
static void lock(void)   { while (__atomic_exchange_n(&lk, 1, __ATOMIC_ACQUIRE)) { } }
static void unlock(void) { __atomic_store_n(&lk, 0, __ATOMIC_RELEASE); }

static double tau0, last_hw;
static int    ready, hw_mode, nst, nout, out_dir;

static void fold(double tau, double alpha, double jump) {
    double err = tau - tau0;
    if (fabs(err) > jump) {
        int dir = err > 0 ? 1 : -1;
        nout = dir == out_dir ? nout + 1 : 1;
        out_dir = dir;
        if (nout >= 3) { tau0 = tau; nout = 0; nst = 1; }       /* a real jump: snap, and average again from here */
        return;
    }
    nout = 0;
    double a = 1.0 / (double)(nst + 1);                          /* the first few stamps are a plain average, then the fixed pace */
    if (a < alpha) a = alpha;
    tau0 += err * a;
    if (nst < 10000) nst++;
}

void sync_reset(void) {
    lock();
    tau0 = 0; last_hw = 0; ready = hw_mode = 0; nst = nout = out_dir = 0;
    unlock();
}

void sync_stamp(int hw, double frame, double t) {
    double tau = t - frame / (double)SYNC_RATE;
    lock();
    if (hw) {
        last_hw = sync_now();
        if (!hw_mode) { hw_mode = 1; tau0 = tau; ready = 1; nst = 1; nout = 0; }     /* the exact clock replaces the estimate at once */
        else fold(tau, 0.15, 0.020);
    } else if (!hw_mode) {
        if (!ready) { tau0 = tau; ready = 1; nst = 1; nout = 0; }
        else fold(tau, 0.02, 0.060);
    }
    unlock();
}

int sync_hw(void) {
    lock();
    if (hw_mode && sync_now() - last_hw > 3.0) hw_mode = 0;     /* the app stopped sending: back to the estimate (tau0 stays, the estimate folds in) */
    int r = hw_mode;
    unlock();
    return r;
}

int sync_ready(void) { lock(); int r = ready; unlock(); return r; }

double sync_frame_at(double t) {
    lock();
    double tau = tau0;
    unlock();
    return (t - tau) * (double)SYNC_RATE;
}

/* ---------------------------------------------------------------- the timeline */
#define HIST 512                                         /* ~12 s of 1024-frame chunks: far more than the pipeline is deep */
static SyncChunk ring[HIST];
static volatile unsigned long long cnt;                  /* chunks pushed so far */

void sync_push(const SyncChunk *c) {
    unsigned long long n = cnt;
    ring[n % HIST] = *c;
    __atomic_store_n(&cnt, n + 1, __ATOMIC_RELEASE);
}

/* the newest chunk that starts at or before `frame`. 0 = none (older than all we keep, or nothing pushed yet).
 * *idx is its number (so the next one is idx + 1) and *newest the count at the time. the copy is checked afterwards:
 * if the writer lapped the ring meanwhile the answer is thrown away */
static int find_chunk(double frame, SyncChunk *out, unsigned long long *idx, unsigned long long *newest) {
    unsigned long long n = __atomic_load_n(&cnt, __ATOMIC_ACQUIRE);
    if (n == 0) return 0;
    unsigned long long lo = n > HIST - 4 ? n - (HIST - 4) : 0;
    for (unsigned long long i = n; i-- > lo; ) {
        if (ring[i % HIST].f0 <= frame) {
            *out = ring[i % HIST];
            if (__atomic_load_n(&cnt, __ATOMIC_ACQUIRE) - i >= HIST - 2) return 0;
            *idx = i; *newest = n;
            return 1;
        }
    }
    return 0;
}

int sync_position_at(double frame, unsigned cur_track, double *pos, double *speed) {
    SyncChunk c; unsigned long long i, n;
    if (!find_chunk(frame, &c, &i, &n)) return SYNC_NONE;
    if (c.track == 0) return SYNC_NONE;
    if (c.track != cur_track) return SYNC_NOT_ARRIVED;
    double k = frame - c.f0;
    if (k < 0) k = 0;
    if (k > c.nm) k = c.nm;
    double v = c.nm > 0 ? c.adv / ((double)c.nm / SYNC_RATE) : 0.0;
    double p = c.p0 + v * k / SYNC_RATE;
    if (c.dur > 0) {
        if (c.wrapped && p >= c.dur) p -= c.dur;                 /* the loop restarted inside this chunk */
        if (p > c.dur) p = c.dur;
    }
    if (p < 0) p = 0;
    if (pos) *pos = p;
    if (speed) *speed = v;
    return SYNC_OK;
}

void sync_spectrum_at(double frame, float *out) {
    for (int b = 0; b < SYNC_BANDS; b++) out[b] = 0;
    /* a chunk's spectrum describes its last SYNC_FFT frames: it belongs to the instant `centre` */
    unsigned long long n = __atomic_load_n(&cnt, __ATOMIC_ACQUIRE);
    if (n == 0) return;
    unsigned long long lo = n > HIST - 4 ? n - (HIST - 4) : 0;
    for (unsigned long long i = n; i-- > lo; ) {
        const SyncChunk *a = &ring[i % HIST];
        double ca = a->f0 + a->n - SYNC_FFT / 2.0;
        if (ca > frame) continue;
        float sa[SYNC_BANDS]; memcpy(sa, a->spec, sizeof sa);
        double w = 0.0; float sb[SYNC_BANDS];
        int have_b = 0;
        if (i + 1 < n) {
            const SyncChunk *b = &ring[(i + 1) % HIST];
            double cb = b->f0 + b->n - SYNC_FFT / 2.0;
            if (cb > ca) { w = (frame - ca) / (cb - ca); memcpy(sb, b->spec, sizeof sb); have_b = 1; }
        }
        if (__atomic_load_n(&cnt, __ATOMIC_ACQUIRE) - i >= HIST - 2) return;      /* lapped while copying: leave the zeros */
        if (w > 1.0) w = 1.0;
        for (int k = 0; k < SYNC_BANDS; k++) out[k] = have_b ? (float)(sa[k] + (sb[k] - sa[k]) * w) : sa[k];
        return;
    }
}

double sync_newest(void) {
    unsigned long long n = __atomic_load_n(&cnt, __ATOMIC_ACQUIRE);
    if (n == 0) return 0.0;
    const SyncChunk *c = &ring[(n - 1) % HIST];
    return c->f0 + c->n;
}
