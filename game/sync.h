/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * MUSIC <-> SCREEN SYNC. no SDL in here, so it can be tested on a PC (tools/synctest.c).
 *
 * the audio goes game -> FIFO -> Java AudioTrack -> mixer -> speaker, so the sound we mix now is heard a while later,
 * and a picture we draw now is on the glass a while later too. to line them up we need two things:
 *
 *   1. THE CLOCK: at what time (CLOCK_MONOTONIC) is stream frame N of the PCM we produce actually heard?
 *      the app reads AudioTrack's hardware timestamp (frame F was at the speaker at time T) and sends it ("a F T").
 *      without it (old app) we fall back to an estimate made from the measured pipeline depth.
 *   2. THE TIMELINE: what was IN stream frame N? one record per audio chunk: where the track was (position, speed,
 *      which track) and the spectrum of the chunk. the screen asks "what is heard at time t?": t -> stream frame
 *      (the clock) -> chunk (the timeline) -> position / bars.
 *
 * the audio thread writes (sync_push, sync_stamp est), the game thread reads. */
#ifndef SYNC_H
#define SYNC_H

#define SYNC_RATE   44100
#define SYNC_BANDS  24                 /* == MUSIC_BANDS */
#define SYNC_FFT    512                /* the spectrum of a chunk is made of its last SYNC_FFT frames */

/* ---------------------------------------------------------------- the clock */
double sync_now(void);                                  /* CLOCK_MONOTONIC in seconds (the same clock as Java's System.nanoTime) */
void   sync_reset(void);
/* a stamp: stream frame `frame` is heard at monotonic time `t`. hw = 1: from the AudioTrack (exact), 0: our own estimate.
 * hardware stamps take over for good (until they stop coming for a few seconds) */
void   sync_stamp(int hw, double frame, double t);
int    sync_hw(void);                                   /* hardware stamps are in use */
int    sync_ready(void);                                /* any stamp at all yet */
/* the stream frame that is at the speaker at monotonic time t (fractional). only meaningful when sync_ready() */
double sync_frame_at(double t);

/* ---------------------------------------------------------------- the timeline */
typedef struct {
    double   f0;                       /* stream index of the chunk's first frame */
    int      n;                        /* frames in the chunk */
    int      nm;                        /* of those, frames the track moved over (the rest is silence after the end) */
    double   p0, adv, dur;             /* track position (s) at the first frame, how far it moves over nm frames (s, wrap included, negative = tape back), length (s) */
    unsigned track;                    /* which track (0 = none: nothing was playing) */
    int      wrapped;                  /* the track looped inside this chunk */
    float    spec[SYNC_BANDS];
} SyncChunk;

void sync_push(const SyncChunk *c);                     /* audio thread: the chunk just produced */

enum { SYNC_OK = 0, SYNC_NOT_ARRIVED = 1, SYNC_NONE = 2 };
/* what is at stream frame `frame`: its track position (s) and speed (track seconds per second: 1 = playing, 0 = at rest,
 * negative = tape backwards). SYNC_NOT_ARRIVED: that frame belongs to another (earlier) track than `cur_track`;
 * SYNC_NONE: nothing playing there / no data */
int  sync_position_at(double frame, unsigned cur_track, double *pos, double *speed);
/* the spectrum at stream frame `frame`, interpolated between the two chunks round it. zeros when there is no data */
void sync_spectrum_at(double frame, float *out);
/* newest frame index produced so far (end of the last pushed chunk), 0 when none */
double sync_newest(void);

#endif
