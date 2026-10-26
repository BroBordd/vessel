/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#define _GNU_SOURCE
#include <SDL2/SDL.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <dirent.h>

#define STB_VORBIS_NO_PUSHDATA_API
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
#include "third_party/stb_vorbis.c"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "audio.h"
#include "analyze.h"
#include "sync.h"

#define OUT_RATE 44100
#ifndef F_SETPIPE_SZ
#define F_SETPIPE_SZ 1031
#endif

/* ---------- how far behind the speakers are ----------
 * the audio goes game -> FIFO -> Java AudioTrack, and every stage fills up completely (the game
 * writes as fast as they accept), so what we mix now is heard a good while later. we measure it:
 * frames handed out so far minus what real time could have played = frames still in the pipeline.
 * the dialog uses this to fire each letter's blip a little BEFORE the letter shows, so they land together. */
static volatile Uint64 produced_frames;
static Uint64 t_first;
static float  lat_smooth = -1.0f;

float audio_latency(void) {
    if (!t_first) return 0.0f;
    double el = (double)(SDL_GetPerformanceCounter() - t_first) / (double)SDL_GetPerformanceFrequency();
    float l = (float)((double)produced_frames / OUT_RATE - el);
    if (el < 2.0 || l < 0) return lat_smooth < 0 ? 0.0f : lat_smooth;       /* not saturated yet: no trustworthy number */
    if (l > 1.5f) l = 1.5f;
    lat_smooth = lat_smooth < 0 ? l : lat_smooth + (l - lat_smooth) * 0.03f;   /* the chunks make it jitter: smooth it */
    return lat_smooth;
}

static SDL_AudioDeviceID dev;
static stb_vorbis *mus;       /* guarded by SDL_LockAudioDevice */
static unsigned char *mus_buf; /* encoded file, must outlive mus */
static int mus_loop;
/* fade: gain = fade_vol^2 (perceptual), ramps to 0 over fade_frames; guarded by device lock */
static int   fading;
static float fade_vol = 1.0f, fade_step;
static float master = MUSIC_DEFAULT_VOLUME;   /* instant master gain, set via music_set_volume */
/* pause: pg is the music's gain ramp (1 playing .. 0 paused). once it reaches 0 the stream is not read at all. */
static volatile int mus_paused;
static float pg = 1.0f;
static char  cur_file[128];
/* where the decoder is in the track, in SOURCE frames (what mus_read has consumed), and the track length */
static double play_pos;
static int    mus_len, mus_enc_len;
static volatile int wrapped;      /* the track has looped since it started / was last seeked */
static unsigned wrap_n;           /* how many times the loop restarted: the sync timeline sees a wrap inside one chunk by this changing */
static volatile unsigned track_id; /* which track is in the decoder: bumped by music_play under the device lock (the timeline tags every chunk with it) */
static volatile int scrub_on;     /* the tape scrub is running: the audio thread reads the decoded copy instead of the stream */

/* ---------- reading the stream, resampled when the file is not 44100 Hz ----------
 * a custom ogg can be 48000 / 32000 / 22050 Hz. rather than playing it at the wrong speed we read it through a
 * tiny linear resampler. 44100 Hz files take the straight (fast) path. all of this belongs to the audio thread
 * and is only reset under the device lock in music_play. */
static int    mus_rate = OUT_RATE;
static float  rs_pos;                        /* fractional position between rs_a and rs_b */
static Sint16 rs_a[2], rs_b[2];
static Sint16 rs_buf[1024 * 2];
static int    rs_len, rs_idx;

/* PITCH: the music is read through the resampler at a different speed (2^(semitones/12)): higher = faster and higher, lower = slower and darker, like a record
 * on a turntable. 0 = untouched (the fast path). set by the jukebox: every map has its own, a custom track has the player's */
static volatile float pitch_mul = 1.0f;
static float pitch_st;
static int   last_fast = -1;                 /* which path the last read took (the resampler has to be primed again when it takes over from the fast path) */
void  music_set_pitch(float st) { if (st < -12.0f) st = -12.0f; if (st > 12.0f) st = 12.0f; pitch_st = st; pitch_mul = exp2f(st / 12.0f); }
float music_pitch(void) { return pitch_st; }

static void rs_reset(int rate) {
    last_fast = -1;
    mus_rate = (rate >= 8000 && rate <= 192000) ? rate : OUT_RATE;
    rs_pos = 2.0f; rs_len = rs_idx = 0;
    rs_a[0] = rs_a[1] = rs_b[0] = rs_b[1] = 0;
}

static int rs_next(Sint16 *f) {              /* next input frame, 0 at the end of the stream */
    if (rs_idx >= rs_len) {
        rs_len = stb_vorbis_get_samples_short_interleaved(mus, 2, rs_buf, 1024 * 2);
        rs_idx = 0;
        if (rs_len <= 0) { rs_len = 0; return 0; }
    }
    f[0] = rs_buf[rs_idx * 2]; f[1] = rs_buf[rs_idx * 2 + 1]; rs_idx++;
    play_pos += 1.0;
    return 1;
}

/* fills up to `frames` stereo frames at OUT_RATE, looping if asked. returns how many it produced (less = the end) */
static int mus_read(Sint16 *out, int frames) {
    int done = 0, reseeked = 0;
    float pm = pitch_mul;
    int fast = mus_rate == OUT_RATE && pm == 1.0f;
    if (fast != last_fast) { if (!fast) { rs_pos = 2.0f; rs_len = rs_idx = 0; } last_fast = fast; }      /* (the fast path read straight from the stream: nothing is buffered) */
    if (fast) {
        while (done < frames) {
            int got = stb_vorbis_get_samples_short_interleaved(mus, 2, out + done * 2, (frames - done) * 2);
            if (got > 0) { done += got; play_pos += got; reseeked = 0; continue; }
            if (!mus_loop || reseeked) break;       /* end, or broken stream */
            stb_vorbis_seek_start(mus);
            play_pos = 0; wrapped = 1; wrap_n++;
            reseeked = 1;
        }
        return done;
    }
    float step = (float)mus_rate / (float)OUT_RATE * pm;
    while (done < frames) {
        while (rs_pos >= 1.0f) {
            Sint16 f[2];
            if (!rs_next(f)) {
                if (!mus_loop || reseeked) return done;
                stb_vorbis_seek_start(mus); rs_len = rs_idx = 0; play_pos = 0; wrapped = 1; wrap_n++; reseeked = 1;
                if (!rs_next(f)) return done;
            } else reseeked = 0;
            rs_a[0] = rs_b[0]; rs_a[1] = rs_b[1]; rs_b[0] = f[0]; rs_b[1] = f[1];
            rs_pos -= 1.0f;
        }
        out[done * 2]     = (Sint16)(rs_a[0] + (rs_b[0] - rs_a[0]) * rs_pos);
        out[done * 2 + 1] = (Sint16)(rs_a[1] + (rs_b[1] - rs_a[1]) * rs_pos);
        rs_pos += step;
        done++;
    }
    return done;
}

/* ---------- one-shot sound effects ----------
 * synthesised on the fly (no extra asset files) and mixed on top of the music after the master
 * volume, so a ding is always heard at the same loudness whatever the music is set to. */
static int   sfx_on, sfx_pos;                  /* guarded by the device lock (the callback holds it) */
static float sfx_phase;
static int   sfx_mode;                         /* 0 = the coin ding, 1 = the calm thought chime, 2 = the discovery chime (same voice) */
static int   n_found;
#define COIN_NOTE1_T 0.085f                    /* the short first note, seconds */
#define COIN_END_T   0.75f                     /* total length */

/* the classic two-note coin: a quick B5, then E6 ringing out */
static int n_coins;
int sfx_debug_coins(void) { return n_coins; }
void sfx_coin(void) {
    n_coins++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    sfx_on = 1; sfx_pos = 0; sfx_phase = 0.0f; sfx_mode = 0;
    SDL_UnlockAudioDevice(dev);
}

/* chunk 58: "You found ...": a bright two-note bell, a rising fifth (C6 then G6) with a little shine on top and a long ring. a pure bell, no square pulse, so it
 * is nothing like the coin's chiptune ding */
void sfx_found(void) {
    n_found++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    sfx_on = 1; sfx_pos = 0; sfx_phase = 0.0f; sfx_mode = 2;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_found(void) { return n_found; }

/* a thought arriving: calm, soft and low: two gentle sines (G4 then D5) with a slow attack and a long fade, nothing like the coin */
void sfx_thought(void) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    sfx_on = 1; sfx_pos = 0; sfx_phase = 0.0f; sfx_mode = 1;
    SDL_UnlockAudioDevice(dev);
}

/* ---------- the dialog "deek": one short, clean tick per letter, like a modern text-box blip ----------
 * a pure sine at a steady pitch (no square wave, no pitch slide: nothing to make it buzzy or noisy),
 * with a quick attack and a smooth fade so it never clicks.
 * its own voice, so it never cuts off the coin ding (and the coin never cuts off a deek). */
/* TWO voices, taken in turns. the old code restarted its one voice on every letter: a new blip began (phase 0, envelope 0) while the last one was still
 * ringing, which is a hard jump in the waveform = a click, and typing is fast enough to do that on most letters. now a new blip takes the other voice and
 * the old one finishes its own fade-out. the sum is soft-limited, not hard-clipped (a hard clip against the music is the crackle a small phone speaker
 * makes so obvious) and the peak is lower. */
#define BLIP_VOICES 2
static int   blip_on, blip_next;                 /* blip_on: any voice alive (the mixer's cheap switch) */
static int   bv_pos[BLIP_VOICES];                /* -1 = free */
static float bv_phase[BLIP_VOICES], bv_f[BLIP_VOICES];
#define BLIP_T    0.070f                       /* seconds per deek */
#define BLIP_HZ   440.0f                       /* base pitch at pitch 1.0 (A4: a small phone speaker plays this cleanly, 256 Hz made it buzz and crunch) */
#define BLIP_GAIN 0.17f                        /* peak level, fraction of full scale (was 0.50: with the music under it that clipped) */

void sfx_blip(float pitch) {
    if (!dev) return;
    if (pitch < 0.4f) pitch = 0.4f;
    if (pitch > 3.0f) pitch = 3.0f;
    SDL_LockAudioDevice(dev);
    int v = blip_next;
    for (int k = 0; k < BLIP_VOICES; k++) { int c = (blip_next + k) % BLIP_VOICES; if (!blip_on || bv_pos[c] < 0) { v = c; break; } }
    if (!blip_on) for (int k = 0; k < BLIP_VOICES; k++) bv_pos[k] = -1;
    bv_pos[v] = 0; bv_phase[v] = 0.0f; bv_f[v] = BLIP_HZ * pitch;
    blip_next = (v + 1) % BLIP_VOICES;           /* the next blip, if both are busy, steals the older one (it is the one nearest its end) */
    blip_on = 1;
    SDL_UnlockAudioDevice(dev);
}

/* a smooth knee instead of a wall: below 24000 untouched, above it bends towards full scale and never passes it */
static inline Sint16 soft_limit(int o) {
    const float knee = 24000.0f, room = 32767.0f - knee;
    float a = (float)(o < 0 ? -o : o);
    if (a > knee) { float x = (a - knee) / room; a = knee + room * (x / (1.0f + x)); }   /* x/(1+x): smooth, tends to 1 */
    if (a > 32767.0f) a = 32767.0f;
    return (Sint16)(o < 0 ? -a : a);
}

static void blip_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float sum = 0.0f; int alive = 0;
        for (int k = 0; k < BLIP_VOICES; k++) {
            if (bv_pos[k] < 0) continue;
            float tt = (float)bv_pos[k] / (float)OUT_RATE;
            if (tt >= BLIP_T) { bv_pos[k] = -1; continue; }
            alive = 1;
            float env = expf(-tt * 26.0f);                               /* soft decay: a tick, not a ring */
            if (tt < 0.010f) { float a = tt / 0.010f; env *= 0.5f - 0.5f * cosf(3.1415927f * a); }   /* raised-cosine attack: no click, no pop on a small speaker */
            if (tt > BLIP_T - 0.025f) { float a = (BLIP_T - tt) / 0.025f; env *= a * a; }            /* smooth way out */
            bv_phase[k] += bv_f[k] / (float)OUT_RATE;
            if (bv_phase[k] >= 1.0f) bv_phase[k] -= 1.0f;
            sum += sinf(6.2831853f * bv_phase[k]) * env;
            bv_pos[k]++;
        }
        if (!alive) { blip_on = 0; return; }
        int v = (int)(sum * BLIP_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) out[i * 2 + ch] = soft_limit(out[i * 2 + ch] + v);
    }
}

static void sfx_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)sfx_pos / (float)OUT_RATE;
        if (sfx_mode == 2) {                                       /* the discovery chime */
            if (tt >= 1.0f) { sfx_on = 0; return; }
            int second = tt >= 0.11f;
            float f = second ? 1567.98f : 1046.50f, t0 = second ? 0.11f : 0.0f;
            float env = expf(-(tt - t0) * (second ? 4.2f : 9.0f));
            float a = (tt - t0) / 0.004f; if (a < 1.0f) env *= a;     /* no click on the way in */
            sfx_phase += f / (float)OUT_RATE;
            if (sfx_phase >= 1.0f) sfx_phase -= 1.0f;
            float sh = sinf(6.2831853f * sfx_phase) + 0.35f * sinf(2.0f * 6.2831853f * sfx_phase);   /* the note and a bit of its octave */
            int v = (int)(sh * env * 0.12f * 32767.0f);
            for (int c = 0; c < 2; c++) {
                int o = out[i * 2 + c] + v;
                out[i * 2 + c] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
            }
            sfx_pos++;
            continue;
        }
        if (sfx_mode == 1) {                                       /* the thought chime */
            if (tt >= 1.1f) { sfx_on = 0; return; }
            float f = tt < 0.16f ? 392.0f : 587.33f, t0 = tt < 0.16f ? 0.0f : 0.16f;
            float env = expf(-(tt - t0) * 4.0f);
            float a = (tt - t0) / 0.03f; if (a < 1.0f) env *= a;      /* slow attack: no tick */
            if (tt < 0.16f && tt > 0.13f) env *= (0.16f - tt) / 0.03f;   /* the first note lets go before the second */
            sfx_phase += f / (float)OUT_RATE;
            if (sfx_phase >= 1.0f) sfx_phase -= 1.0f;
            int v = (int)(sinf(6.2831853f * sfx_phase) * env * 0.13f * 32767.0f);
            for (int c = 0; c < 2; c++) {
                int o = out[i * 2 + c] + v;
                out[i * 2 + c] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
            }
            sfx_pos++;
            continue;
        }
        if (tt >= COIN_END_T) { sfx_on = 0; return; }
        float f, env;
        if (tt < COIN_NOTE1_T) { f = 987.77f;  env = 1.0f; }
        else                   { f = 1318.51f; env = expf(-(tt - COIN_NOTE1_T) * 6.5f); }
        if (tt < 0.003f) env *= tt / 0.003f;                       /* no click on the way in */
        sfx_phase += f / (float)OUT_RATE;
        if (sfx_phase >= 1.0f) sfx_phase -= 1.0f;
        float sq = sfx_phase < 0.5f ? 1.0f : -1.0f;                /* chiptune pulse + a little sine for the shine */
        float sn = sinf(6.2831853f * sfx_phase);
        int v = (int)((0.55f * sq + 0.45f * sn) * env * 0.15f * 32767.0f);   /* (0.30 before: half as loud) */
        for (int c = 0; c < 2; c++) {
            int o = out[i * 2 + c] + v;
            out[i * 2 + c] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        sfx_pos++;
    }
}

/* ---------- the heart monitor: the "peep" and the flatline ----------
 * one voice for both (a flatline is a peep that does not stop): a pure sine, like the deeks, at the pitch of
 * a hospital monitor. a peep is short with a quick decay; the flatline holds one steady level for MON_FLAT_T
 * and then lets go (sfx_flatline_stop() lets go sooner, in ~60 ms). its own voice: music, coin and deeks go on. */
static int   mon_on, mon_pos, mon_flat, mon_len, mon_rel;      /* mon_len: frames; mon_rel: frames of release left (0 = not releasing) */
static float mon_phase;
static int   n_beeps, n_flats;                                 /* how many times each was asked for (tests) */
#define MON_HZ      1000.0f
#define MON_BEEP_T  0.16f
#define MON_FLAT_T  3.0f
#define MON_GAIN    0.28f

void sfx_beep(void) {
    n_beeps++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    mon_on = 1; mon_pos = 0; mon_phase = 0.0f; mon_flat = 0; mon_len = (int)(MON_BEEP_T * OUT_RATE); mon_rel = 0;
    SDL_UnlockAudioDevice(dev);
}
void sfx_flatline(void) {
    n_flats++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    mon_on = 1; mon_pos = 0; mon_phase = 0.0f; mon_flat = 1; mon_len = (int)(MON_FLAT_T * OUT_RATE); mon_rel = 0;
    SDL_UnlockAudioDevice(dev);
}
void sfx_flatline_stop(void) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    if (mon_on && mon_flat && !mon_rel) mon_rel = (int)(0.060f * OUT_RATE);
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_beeps(void) { return n_beeps; }
int sfx_debug_flatlines(void) { return n_flats; }

/* ---------- the hammer hit (chunk 36): a thump (a sine falling from 130 to 50 Hz) with a short burst of noise at the start ---------- */
static int   hit_on, hit_pos, n_hits;
static float hit_phase;
static unsigned hit_seed = 12345u;
#define HIT_T     0.16f
#define HIT_GAIN  0.45f
void sfx_hit(void) {
    n_hits++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    hit_on = 1; hit_pos = 0; hit_phase = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_hits(void) { return n_hits; }
static void hit_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)hit_pos / (float)OUT_RATE;
        if (tt >= HIT_T) { hit_on = 0; return; }
        float k = tt / HIT_T;
        float env = (1.0f - k) * (1.0f - k);
        if (tt < 0.003f) env *= tt / 0.003f;                                  /* no click on the way in */
        hit_phase += (130.0f - 80.0f * k) / (float)OUT_RATE;
        if (hit_phase >= 1.0f) hit_phase -= 1.0f;
        hit_seed = hit_seed * 1664525u + 1013904223u;
        float noise = ((float)(hit_seed >> 16) / 32768.0f - 1.0f) * (tt < 0.025f ? 1.0f - tt / 0.025f : 0.0f);
        int v = (int)((0.8f * sinf(6.2831853f * hit_phase) + 0.35f * noise) * env * HIT_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        hit_pos++;
    }
}

/* ---------- the drag: a body sliding over stone. a calm, low rumble, not a hiss: noise with everything above ~400 Hz rolled off (two poles) and the lows below
 * ~150 Hz taken out (they do not carry on a phone speaker), so all that is left is a soft dull scrape. two very slow swells (about 0.7 and 1.3 Hz, never in step)
 * make it breathe unevenly. no pulses, no tone, nothing bright. its loudness follows how fast he slides ---------- */
static int   drag_on;
static float drag_tgt, drag_amp, drag_lp1, drag_lp1b, drag_lp2, drag_ph, drag_slow;
static unsigned drag_seed = 4242u;
#define DRAG_GAIN 0.17f
void sfx_drag(float level) {
    if (!dev) return;
    if (level < 0) level = 0;
    if (level > 1) level = 1;
    SDL_LockAudioDevice(dev);
    drag_tgt = level;
    if (level > 0) drag_on = 1;
    SDL_UnlockAudioDevice(dev);
}
static void drag_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        drag_amp += (drag_tgt - drag_amp) * (drag_tgt > drag_amp ? 0.00020f : 0.00035f);            /* eases in over ~110 ms, out over ~65 ms: no sudden edge */
        if (drag_tgt <= 0.0f && drag_amp < 0.0006f) { drag_on = 0; drag_amp = 0; return; }
        drag_seed = drag_seed * 1664525u + 1013904223u;
        float n = (float)(drag_seed >> 16) / 32768.0f - 1.0f;
        drag_lp1  += (n - drag_lp1) * 0.058f;                                                        /* top off above ~400 Hz ... */
        drag_lp1b += (drag_lp1 - drag_lp1b) * 0.058f;                                                /* ... twice, so there is no hiss left at all */
        drag_lp2  += (drag_lp1b - drag_lp2) * 0.021f;                                                /* the slow part (below ~150 Hz) ... */
        float band = drag_lp1b - drag_lp2;                                                           /* ... taken out: what is left is 150 to 400 Hz */
        drag_ph   += 0.7f / (float)OUT_RATE; if (drag_ph   >= 1.0f) drag_ph   -= 1.0f;
        drag_slow += 1.3f / (float)OUT_RATE; if (drag_slow >= 1.0f) drag_slow -= 1.0f;
        float swell = 0.72f + 0.18f * sinf(6.2831853f * drag_ph) + 0.10f * sinf(6.2831853f * drag_slow);
        float v = band * 7.0f * swell;
        if (v > 1.0f) v = 1.0f; else if (v < -1.0f) v = -1.0f;
        int o16 = (int)(v * drag_amp * DRAG_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + o16;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
    }
}

/* ---------- the hold hint (chunk 55): a tap on a hammer button. a short, soft, rising "bup" (a sine gliding from 330 to 440 Hz, quiet) ---------- */
static int   hint_on, hint_pos, n_hints;
static float hint_phase;
#define HINT_T     0.11f
#define HINT_GAIN  0.14f
void sfx_hold_hint(void) {
    n_hints++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    hint_on = 1; hint_pos = 0; hint_phase = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_hints(void) { return n_hints; }
static void hint_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)hint_pos / (float)OUT_RATE;
        if (tt >= HINT_T) { hint_on = 0; return; }
        float k = tt / HINT_T;
        float env = sinf(3.1415926f * k);                                     /* in and out: no click either side */
        hint_phase += (330.0f + 110.0f * k) / (float)OUT_RATE;
        if (hint_phase >= 1.0f) hint_phase -= 1.0f;
        int v = (int)(sinf(6.2831853f * hint_phase) * env * HINT_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        hint_pos++;
    }
}

/* ---------- the tap: a UI button was pressed. a CLICK, not a note: a few milliseconds of high-passed noise with a very fast fall (the plastic of a switch), and under it
 * a tiny, dull knock (a low sine, 2 ms attack, gone in 12 ms) so it is not a bare hiss. nothing pitched sweeps. ---------- */
static int   tap_on, tap_pos;
static float tap_lp;                                            /* the one-pole filter state: noise minus its low part = the bright "tick" */
static unsigned tap_seed = 4242u;
#define TAP_T     0.030f
#define TAP_GAIN  0.30f
void sfx_tap(void) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    tap_on = 1; tap_pos = 0; tap_lp = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
static void tap_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)tap_pos / (float)OUT_RATE;
        if (tt >= TAP_T) { tap_on = 0; return; }
        tap_seed = tap_seed * 1664525u + 1013904223u;
        float nz = (float)(tap_seed >> 16) / 32768.0f - 1.0f;
        tap_lp += (nz - tap_lp) * 0.35f;                                         /* the low part of the noise ... */
        float hp = nz - tap_lp;                                                  /* ... taken away: the bright tick */
        float env_t = expf(-tt / 0.0022f);                                       /* the tick: gone in ~8 ms */
        float env_k = expf(-tt / 0.0035f) * (tt < 0.0015f ? tt / 0.0015f : 1.0f); /* the knock under it, with a 1.5 ms attack */
        float knock = sinf(6.2831853f * 190.0f * tt) * env_k * 0.55f;
        int v = (int)((hp * env_t * 0.9f + knock) * TAP_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        tap_pos++;
    }
}

/* ---------- the pfft (chunk 38.1): noise through a low-pass whose cutoff falls (the pitch sinks), a slow wobble on it so it sounds wet ---------- */
static int   pfft_on, pfft_pos, n_pfft;
static float pfft_lp;
static unsigned pfft_seed = 777u;
#define PFFT_T     0.17f
#define PFFT_GAIN  0.5f
void sfx_pfft(void) {
    n_pfft++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    pfft_on = 1; pfft_pos = 0; pfft_lp = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_pfft(void) { return n_pfft; }
static void pfft_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)pfft_pos / (float)OUT_RATE;
        if (tt >= PFFT_T) { pfft_on = 0; return; }
        float k = tt / PFFT_T;
        float env = (1.0f - k) * (1.0f - k * 0.5f);
        if (tt < 0.006f) env *= tt / 0.006f;                                  /* no click on the way in */
        pfft_seed = pfft_seed * 1664525u + 1013904223u;
        float noise = (float)(pfft_seed >> 16) / 32768.0f - 1.0f;
        float a = 0.55f - 0.47f * k;                                          /* the cutoff falls: the pitch sinks */
        pfft_lp += a * (noise - pfft_lp);
        float wet = 0.65f + 0.35f * sinf(6.2831853f * 62.0f * tt);            /* the gurgle */
        int v = (int)(pfft_lp * 1.6f * wet * env * PFFT_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        pfft_pos++;
    }
}

/* ---------- the blood fountain (self-stab scene): a wet, rushing "woosh". looped noise through two low-passes (the band between ~100 Hz and ~800 Hz, so it carries on a
 * phone speaker), a touch of hiss on top, two slow swells and a quick wet gurgle on it. call sfx_blood_fountain(level) every frame while it spurts (0..1 = how hard);
 * it eases in and out by itself, and if the calls stop (the app went to the background, the world is paused) it dies away within ~0.25 s. its own voice ---------- */
static int   bf_on, bf_hold;
static float bf_tgt, bf_amp, bf_lpa, bf_lpb, bf_lph, bf_p1, bf_p2, bf_p3;
static unsigned bf_seed = 99991u;
#define BF_GAIN 0.34f
void sfx_blood_fountain(float level) {
    if (!dev) return;
    level = level < 0.0f ? 0.0f : level > 1.0f ? 1.0f : level;
    SDL_LockAudioDevice(dev);
    bf_tgt = level;
    if (level > 0.0f) { if (!bf_on) { bf_amp = 0.0f; bf_lpa = bf_lpb = bf_lph = 0.0f; } bf_on = 1; bf_hold = (int)(0.25f * OUT_RATE); }
    SDL_UnlockAudioDevice(dev);
}
static void bf_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        if (bf_hold > 0) bf_hold--; else bf_tgt = 0.0f;
        bf_amp += (bf_tgt - bf_amp) * (bf_tgt > bf_amp ? 0.0006f : 0.0018f);
        if (bf_tgt <= 0.0f && bf_amp < 0.0005f) { bf_on = 0; bf_amp = 0.0f; return; }
        bf_seed = bf_seed * 1664525u + 1013904223u;
        float n = (float)(bf_seed >> 16) / 32768.0f - 1.0f;
        bf_lpa += 0.11f * (n - bf_lpa);                                       /* ~770 Hz */
        bf_lpb += 0.015f * (n - bf_lpb);                                      /* ~105 Hz: taken out */
        bf_lph += 0.45f * (n - bf_lph);
        float band = (bf_lpa - bf_lpb) * 2.6f, hiss = (n - bf_lph) * 0.16f;
        bf_p1 += 5.3f / (float)OUT_RATE;  if (bf_p1 >= 1.0f) bf_p1 -= 1.0f;
        bf_p2 += 1.9f / (float)OUT_RATE;  if (bf_p2 >= 1.0f) bf_p2 -= 1.0f;
        bf_p3 += 37.0f / (float)OUT_RATE; if (bf_p3 >= 1.0f) bf_p3 -= 1.0f;
        float swell = 0.72f + 0.2f * sinf(6.2831853f * bf_p1) + 0.08f * sinf(6.2831853f * bf_p2);
        float wet = 0.8f + 0.2f * sinf(6.2831853f * bf_p3 + 2.0f * sinf(6.2831853f * bf_p2));
        int v = (int)((band + hiss) * swell * wet * bf_amp * BF_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
    }
}

/* ---------- the sputter (self-stab scene): the last squirt of an empty ketchup bottle: a short ragged "bzzt", a pulse wave that sags from ~150 to ~70 Hz with the phase
 * shaken by noise, low-passed, wobbling wet. strength 0..1 = how big (a big one is longer and louder). a new call restarts it. its own voice ---------- */
static int   sp_on, sp_pos, sp_len, n_sput;
static float sp_str, sp_ph, sp_lp;
static unsigned sp_seed = 31337u;
#define SP_GAIN 0.38f
void sfx_blood_sputter(float strength) {
    n_sput++;
    if (!dev) return;
    strength = strength < 0.05f ? 0.05f : strength > 1.0f ? 1.0f : strength;
    SDL_LockAudioDevice(dev);
    sp_str = strength; sp_len = (int)((0.07f + 0.10f * strength) * OUT_RATE); sp_pos = 0; sp_ph = 0.0f; sp_lp = 0.0f; sp_on = 1;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_sputters(void) { return n_sput; }
static void sp_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        if (sp_pos >= sp_len) { sp_on = 0; return; }
        float tt = (float)sp_pos / (float)OUT_RATE, k = (float)sp_pos / (float)sp_len;
        float env = (1.0f - k) * sqrtf(1.0f - k);
        if (tt < 0.003f) env *= tt / 0.003f;                                  /* no click on the way in */
        sp_seed = sp_seed * 1664525u + 1013904223u;
        float n = (float)(sp_seed >> 16) / 32768.0f - 1.0f;
        sp_ph += (150.0f - 80.0f * k) * (1.0f + 0.35f * n) / (float)OUT_RATE;
        if (sp_ph >= 1.0f) sp_ph -= 1.0f;
        if (sp_ph < 0.0f) sp_ph += 1.0f;
        float sq = sp_ph < 0.4f ? 1.0f : -1.0f;
        sp_lp += 0.22f * (0.8f * sq + 0.5f * n - sp_lp);
        float wet = 0.6f + 0.4f * sinf(6.2831853f * 70.0f * tt);
        int v = (int)(sp_lp * wet * env * sp_str * SP_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        sp_pos++;
    }
}

/* ---------- the punch (self-stab scene, the red cut): a game-style impact. a fast pitch drop (200 -> 42 Hz) for the body, a bright noise crack in the first ~40 ms, a short
 * mid "slap" (noise through a low-pass) and a slow dull tail, all soft-clipped so it hits hard on a phone speaker. ~0.38 s. its own voice (a new call restarts it) ---------- */
static int   pn_on, pn_pos, n_punch;
static float pn_ph, pn_lp;
static unsigned pn_seed = 8675309u;
#define PN_T    0.38f
#define PN_GAIN 0.62f
void sfx_punch(void) {
    n_punch++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    pn_on = 1; pn_pos = 0; pn_ph = 0.0f; pn_lp = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_punches(void) { return n_punch; }
static void pn_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)pn_pos / (float)OUT_RATE;
        if (tt >= PN_T) { pn_on = 0; return; }
        pn_seed = pn_seed * 1664525u + 1013904223u;
        float n = (float)(pn_seed >> 16) / 32768.0f - 1.0f;
        float f = 42.0f + 158.0f * expf(-tt * 38.0f);                          /* the body: the pitch falls fast */
        pn_ph += f / (float)OUT_RATE; if (pn_ph >= 1.0f) pn_ph -= 1.0f;
        float body = sinf(6.2831853f * pn_ph) * expf(-tt * 11.0f);
        float crack = n * expf(-tt * 90.0f);                                   /* the bright crack, gone in ~40 ms */
        pn_lp += 0.18f * (n - pn_lp);
        float slap = pn_lp * 1.8f * expf(-tt * 26.0f);                         /* the slap: dull noise, ~100 ms */
        float x = 1.25f * body + 0.55f * crack + 0.6f * slap;
        x = x / (1.0f + fabsf(x));                                             /* soft clip: dense and punchy */
        float env = tt < 0.002f ? tt / 0.002f : 1.0f;                          /* no click on the way in */
        int v = (int)(x * 1.6f * env * PN_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        pn_pos++;
    }
}

/* ---------- the error buzz (chunk 33): two short low buzzes, "no". a square wave at 110 Hz softened with a sine, its own voice ---------- */
static int   err_on, err_pos, n_errors;
static float err_phase;
#define ERR_HZ    110.0f
#define ERR_BUZZ  0.09f                         /* seconds per buzz */
#define ERR_GAP   0.06f
#define ERR_GAIN  0.22f

void sfx_error(void) {
    n_errors++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    err_on = 1; err_pos = 0; err_phase = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_errors(void) { return n_errors; }

static void err_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)err_pos / (float)OUT_RATE;
        if (tt >= ERR_BUZZ * 2 + ERR_GAP) { err_on = 0; return; }
        float local = tt < ERR_BUZZ ? tt : tt - ERR_BUZZ - ERR_GAP;
        float env = 0.0f;
        if (local >= 0.0f && local < ERR_BUZZ) {
            env = 1.0f;
            if (local < 0.004f) env = local / 0.004f;                        /* no click on the way in */
            if (local > ERR_BUZZ - 0.012f) env = (ERR_BUZZ - local) / 0.012f;   /* nor on the way out */
        }
        err_phase += ERR_HZ / (float)OUT_RATE;
        if (err_phase >= 1.0f) err_phase -= 1.0f;
        float sq = err_phase < 0.5f ? 1.0f : -1.0f, sn = sinf(6.2831853f * err_phase);
        int v = (int)((0.6f * sq + 0.4f * sn) * env * ERR_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        err_pos++;
    }
}

static void mon_mix(Sint16 *out, int frames) {
    const int rel_total = (int)(0.060f * OUT_RATE), tail = (int)(0.400f * OUT_RATE);
    for (int i = 0; i < frames; i++) {
        if (mon_pos >= mon_len || (mon_rel && --mon_rel <= 0)) { mon_on = 0; mon_rel = 0; return; }
        float tt = (float)mon_pos / (float)OUT_RATE, env;
        if (mon_flat) {
            env = 1.0f;                                            /* one steady level ... */
            if (mon_pos < (int)(0.010f * OUT_RATE)) env = tt / 0.010f;                 /* no click on the way in */
            if (mon_len - mon_pos < tail) env *= (float)(mon_len - mon_pos) / (float)tail;   /* ... then it lets go */
            if (mon_rel) env *= (float)mon_rel / (float)rel_total;                     /* or is told to stop */
        } else {
            env = expf(-tt * 9.0f);                                /* a peep: a clean tick of tone, not a ring */
            if (tt < 0.004f) env *= tt / 0.004f;
            if (mon_len - mon_pos < (int)(0.012f * OUT_RATE)) env *= (float)(mon_len - mon_pos) / (0.012f * OUT_RATE);
        }
        mon_phase += MON_HZ / (float)OUT_RATE;
        if (mon_phase >= 1.0f) mon_phase -= 1.0f;
        int v = (int)(sinf(6.2831853f * mon_phase) * env * MON_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        mon_pos++;
    }
}

/* ---------- the summoning (vessel 2, chunk 9): a soft rising arpeggio and a shimmer ----------
 * ONE voice, like the monitor: a slow swell underneath, a pentatonic arpeggio climbing from C5 in pure sines that ring and
 * decay, and over it a shimmer (two sines a few Hz apart, so they beat, with a rising then falling level). starts with
 * sfx_summon(), ends by itself after SUM_T seconds (the length of the effect, summon.h), a new call restarts it. */
static int   sum_on, sum_pos;
static float sum_ph[8], sum_sw, sum_sh1, sum_sh2;
static int   n_summons;                                        /* how many times it was asked for (tests) */
#define SUM_T      3.6f
#define SUM_GAIN   0.20f
#define SUM_NOTES  8
static const float SUM_HZ[SUM_NOTES] = { 523.25f, 659.25f, 783.99f, 1046.50f, 1318.51f, 1567.98f, 2093.00f, 2637.02f };   /* C5 E5 G5 C6 E6 G6 C7 E7 */
#define SUM_NOTE_T0  0.55f                                     /* the first note, seconds in */
#define SUM_NOTE_GAP 0.22f

void sfx_summon(void) {
    n_summons++;
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    sum_on = 1; sum_pos = 0; sum_sw = sum_sh1 = sum_sh2 = 0.0f;
    for (int k = 0; k < SUM_NOTES; k++) sum_ph[k] = 0.0f;
    SDL_UnlockAudioDevice(dev);
}
int sfx_debug_summons(void) { return n_summons; }

static void sum_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)sum_pos / (float)OUT_RATE;
        if (tt >= SUM_T) { sum_on = 0; return; }
        float master_env = 1.0f;
        if (tt < 0.05f) master_env = tt / 0.05f;                          /* no click on the way in */
        if (tt > SUM_T - 0.5f) master_env = (SUM_T - tt) / 0.5f;          /* a soft end */
        sum_sw += 261.63f / (float)OUT_RATE; if (sum_sw >= 1.0f) sum_sw -= 1.0f;
        float sw = sinf(6.2831853f * sum_sw) * (0.5f * (1.0f - cosf(3.1415927f * fminf(tt / 2.4f, 1.0f)))) * 0.55f;   /* the swell: C4 rising slowly */
        float arp = 0.0f;
        for (int k = 0; k < SUM_NOTES; k++) {
            float nt = tt - (SUM_NOTE_T0 + k * SUM_NOTE_GAP);
            if (nt < 0) continue;
            sum_ph[k] += SUM_HZ[k] / (float)OUT_RATE; if (sum_ph[k] >= 1.0f) sum_ph[k] -= 1.0f;
            float e = expf(-nt * 4.2f);
            if (nt < 0.004f) e *= nt / 0.004f;
            arp += sinf(6.2831853f * sum_ph[k]) * e * 0.45f;
        }
        sum_sh1 += 3135.96f / (float)OUT_RATE; if (sum_sh1 >= 1.0f) sum_sh1 -= 1.0f;
        sum_sh2 += 3141.50f / (float)OUT_RATE; if (sum_sh2 >= 1.0f) sum_sh2 -= 1.0f;
        float lv = tt < 1.0f ? 0.0f : tt < 2.4f ? (tt - 1.0f) / 1.4f : 1.0f - (tt - 2.4f) / (SUM_T - 2.4f);     /* swells in, then fades */
        float sh = (sinf(6.2831853f * sum_sh1) + sinf(6.2831853f * sum_sh2)) * 0.5f * lv * 0.30f;
        int v = (int)((sw + arp + sh) * master_env * SUM_GAIN * 32767.0f);
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        sum_pos++;
    }
}

/* ---------- the piano in the music window ----------
 * a few overlapping voices (a chord or a slide of the finger works), each a soft piano-ish tone:
 * a few harmonics with a quick attack, a slow fade while the key is held, a short release after.
 * mixed after the master volume like the other effects, and NOT fed to the analyzer, so the keys
 * you play never light up as if they were part of the song. */
#define VOICES 8
typedef struct { int on, midi, held; float ph, t, rel, f; } Voice;
static Voice voice[VOICES];

void sfx_note_on(int midi) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    int pick = -1;
    for (int i = 0; i < VOICES && pick < 0; i++) if (!voice[i].on) pick = i;
    if (pick < 0) { float oldest = -1; for (int i = 0; i < VOICES; i++) if (voice[i].t > oldest) { oldest = voice[i].t; pick = i; } }
    voice[pick] = (Voice){ 1, midi, 1, 0.0f, 0.0f, 1.0f, 440.0f * powf(2.0f, (float)(midi - 69) / 12.0f) };
    SDL_UnlockAudioDevice(dev);
}

void sfx_note_off(int midi) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    for (int i = 0; i < VOICES; i++) if (voice[i].on && voice[i].held && voice[i].midi == midi) voice[i].held = 0;
    SDL_UnlockAudioDevice(dev);
}

static void voices_mix(Sint16 *out, int frames) {
    for (int vi = 0; vi < VOICES; vi++) {
        Voice *v = &voice[vi];
        if (!v->on) continue;
        float step = v->f / (float)OUT_RATE;
        float decay = 1.4f + v->f / 900.0f;                         /* higher notes die away faster */
        for (int i = 0; i < frames; i++) {
            float tt = v->t;
            float env = expf(-tt * decay);
            if (tt < 0.004f) env *= tt / 0.004f;                    /* no click on the way in */
            if (!v->held) { v->rel *= 0.99935f; if (v->rel < 0.002f) { v->on = 0; break; } }   /* ~50 ms release */
            else if (env < 0.003f && tt > 0.05f) { v->on = 0; break; }
            v->ph += step; if (v->ph >= 1.0f) v->ph -= 1.0f;
            float a = 6.2831853f * v->ph;
            float w = sinf(a) + 0.45f * sinf(2 * a) + 0.2f * sinf(3 * a) + 0.08f * sinf(4 * a);
            int val = (int)(w * env * v->rel * 0.17f * 32767.0f);
            for (int c = 0; c < 2; c++) {
                int o = out[i * 2 + c] + val;
                out[i * 2 + c] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
            }
            v->t += 1.0f / (float)OUT_RATE;
        }
    }
}

/* ---------- spectrum analysis (runs in the audio thread on the samples about to be played) ----------
 * the spectrum of each chunk is stored in that chunk's record of the sync timeline (sync.h), so the bars can show what is HEARD at
 * any moment, not what is being mixed now */
#define FFT_N SYNC_FFT
_Static_assert(MUSIC_BANDS == SYNC_BANDS, "the timeline keeps MUSIC_BANDS bands");
static SyncChunk cur_chunk;                             /* the chunk the audio thread is filling in right now */
static char  title[48];
static unsigned serial;

static void fft(float *re, float *im) {
    for (int i = 1, j = 0; i < FFT_N; i++) {              /* bit-reversal permutation */
        int bit = FFT_N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { float t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (int len = 2; len <= FFT_N; len <<= 1) {
        float ang = -2.0f * (float)M_PI / len, wr = cosf(ang), wi = sinf(ang);
        for (int i = 0; i < FFT_N; i += len) {
            float cr = 1, ci = 0;
            for (int k = 0; k < len / 2; k++) {
                int a = i + k, b = i + k + len / 2;
                float xr = re[b] * cr - im[b] * ci, xi = re[b] * ci + im[b] * cr;
                re[b] = re[a] - xr; im[b] = im[a] - xi;
                re[a] += xr;        im[a] += xi;
                float nr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = nr;
            }
        }
    }
}

/* pcm: stereo S16, `frames` long. uses the last FFT_N frames (about 12 ms). */
static void spectrum_feed(const Sint16 *pcm, int frames) {
    if (frames < FFT_N) { for (int i = 0; i < MUSIC_BANDS; i++) cur_chunk.spec[i] = 0; return; }
    float re[FFT_N], im[FFT_N];
    const Sint16 *src = pcm + (frames - FFT_N) * 2;
    for (int i = 0; i < FFT_N; i++) {
        float m = (src[i * 2] + src[i * 2 + 1]) * (0.5f / 32768.0f);
        re[i] = m * (0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (FFT_N - 1)));   /* Hann window */
        im[i] = 0;
    }
    fft(re, im);
    for (int b = 0; b < MUSIC_BANDS; b++) {
        int lo = (int)powf(150.0f, (float)b / MUSIC_BANDS);                         /* ~86 Hz .. ~13 kHz, log spaced */
        int hi = (int)powf(150.0f, (float)(b + 1) / MUSIC_BANDS);
        if (hi <= lo) hi = lo + 1;
        float peak = 0;
        for (int k = lo; k < hi && k < FFT_N / 2; k++) {
            float mag = sqrtf(re[k] * re[k] + im[k] * im[k]) / (FFT_N / 4.0f);     /* full-scale sine = 1.0 */
            if (mag > peak) peak = mag;
        }
        float db = 20.0f * log10f(peak + 1e-9f);
        float lvl = (db + 58.0f + 0.7f * b) / 58.0f;                                /* tilt: highs are naturally quieter. top = 0 dBFS, so the bars have headroom and follow the volume knob */
        cur_chunk.spec[b] = lvl < 0 ? 0 : lvl > 1 ? 1 : lvl;
    }
}

static double view_frame(void);
static void trim_load(void);

/* the bars show what is on the speaker at the moment this picture reaches the eye (see view_frame) */
void music_spectrum(float *bands) { sync_spectrum_at(view_frame(), bands); }
const char *music_title(void) { return title; }
unsigned music_serial(void) { return serial; }

/* ---------- the tape scrub ----------
 * stb_vorbis can only decode forwards, and a tape plays backwards, so while the music window is open a background
 * thread decodes the whole track into plain PCM (cache). the scrub then reads that copy at any speed in any direction
 * with linear interpolation (that is also where the tape-ish squeal at high speed comes from).
 *
 * the head chases the finger: it closes 1/SC_TAU of the gap per second, never faster than SC_MAXR times normal speed,
 * and the speed itself is smoothed (~30 ms) so it has weight. finger rests -> head arrives -> the tape stops (silence).
 * everything below sc_* is touched by the audio thread, and by the game thread only under SDL_LockAudioDevice. */
#define SC_TAU           0.20f                 /* seconds: a 1 s gap makes 5x speed */
#define SC_MAXR          10.0f                 /* fastest the tape goes, either way */
#define SC_LEAD          (SC_TAU * SC_MAXR)    /* the head never aims further than this (seconds) from itself */
#define SC_MAX_BYTES     (96.0 * 1024 * 1024)  /* longer tracks do not get a copy: they only get plain seeking */

static Sint16 *cache;                          /* decoded track, stereo S16, at the file's own rate */
static int     cache_cap;                      /* frames the copy has room for */
static volatile int cache_ready;               /* frames decoded so far (written by the decoder thread, release / acquire) */
static SDL_Thread *dec_thread;
static volatile int dec_stop;
static int     want_cache;                     /* the music window is open: keep a copy of whatever plays */
static double  sc_pos, sc_finger;              /* head and finger, in source frames */
static float   sc_rate;                        /* smoothed tape speed (1 = normal) */

typedef struct { unsigned char *enc; int enc_len; Sint16 *pcm; int cap; } DecJob;

static int dec_main(void *p) {
    DecJob *j = (DecJob *)p;
    int err = 0;
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
    stb_vorbis *d = stb_vorbis_open_memory(j->enc, j->enc_len, &err, NULL);   /* its own decoder over its own copy of the file */
    if (d) {
        int done = 0;
        while (!dec_stop && done < j->cap) {
            int want = j->cap - done; if (want > 4096) want = 4096;
            int got = stb_vorbis_get_samples_short_interleaved(d, 2, j->pcm + (size_t)done * 2, want * 2);
            if (got <= 0) break;
            done += got;
            __atomic_store_n(&cache_ready, done, __ATOMIC_RELEASE);
        }
        stb_vorbis_close(d);
    }
    free(j->enc); free(j);
    return 0;
}

static void cache_free(void) {                 /* game thread */
    if (dec_thread) { dec_stop = 1; SDL_WaitThread(dec_thread, NULL); dec_thread = NULL; }
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    Sint16 *old = cache;
    cache = NULL; cache_cap = 0; cache_ready = 0; scrub_on = 0;
    SDL_UnlockAudioDevice(dev);
    free(old);
}

static void cache_start(void) {                /* game thread: a copy of the track that plays right now */
    cache_free();
    if (!dev || mus_len <= 0 || mus_enc_len <= 0 || (double)mus_len * 4.0 > SC_MAX_BYTES) return;
    Sint16 *pcm = (Sint16 *)malloc((size_t)mus_len * 4);
    unsigned char *enc = pcm ? (unsigned char *)malloc((size_t)mus_enc_len) : NULL;
    DecJob *j = enc ? (DecJob *)malloc(sizeof *j) : NULL;
    if (!j) { free(enc); free(pcm); return; }
    SDL_LockAudioDevice(dev);                  /* the audio thread frees mus_buf when a fade ends: copy it while holding the lock */
    if (!mus || !mus_buf) { SDL_UnlockAudioDevice(dev); free(j); free(enc); free(pcm); return; }
    memcpy(enc, mus_buf, (size_t)mus_enc_len);
    cache = pcm; cache_cap = mus_len; cache_ready = 0;
    SDL_UnlockAudioDevice(dev);
    *j = (DecJob){ enc, mus_enc_len, pcm, mus_len };
    dec_stop = 0;
    dec_thread = SDL_CreateThread(dec_main, "oggdec", j);
    if (!dec_thread) { free(enc); free(j); cache_free(); }
}

/* head -> output, `frames` stereo frames. always fills the whole buffer (silence while the tape is stopped) */
static int scrub_read(Sint16 *out, int frames) {
    int ready = __atomic_load_n(&cache_ready, __ATOMIC_ACQUIRE);
    int hi = ready - 2; if (hi < 0) hi = 0;
    const double lead = (double)SC_LEAD * mus_rate;
    const float  ratio = (float)mus_rate / (float)OUT_RATE;
    const float  k = 1.0f - expf(-1.0f / (0.030f * OUT_RATE));
    for (int i = 0; i < frames; i++) {
        double tgt = sc_finger;
        if (tgt > sc_pos + lead) tgt = sc_pos + lead; else if (tgt < sc_pos - lead) tgt = sc_pos - lead;
        float want = (float)((tgt - sc_pos) / (double)mus_rate / (double)SC_TAU);
        sc_rate += (want - sc_rate) * k;
        sc_pos += (double)sc_rate * ratio;
        if (sc_pos < 0)  { sc_pos = 0;  sc_rate = 0; }
        if (sc_pos > hi) { sc_pos = hi; sc_rate = 0; }
        int ip = (int)sc_pos; float fr = (float)(sc_pos - ip);
        float gain = fabsf(sc_rate) * 6.0f; if (gain > 1.0f) gain = 1.0f;       /* a (nearly) stopped tape is silent, not a DC buzz */
        for (int c = 0; c < 2; c++) {
            float a = ready > 1 ? cache[ip * 2 + c] : 0.0f, b = ready > 1 ? cache[(ip + 1) * 2 + c] : 0.0f;
            out[i * 2 + c] = (Sint16)((a + (b - a) * fr) * gain);
        }
    }
    return frames;
}

/* jump the decoder to `fr` source frames. device lock held. fades in over ~30 ms (pg ramps up from 0) */
static void seek_locked(double fr) {
    if (!mus) return;
    if (mus_len > 0 && fr > mus_len - 1) fr = mus_len - 1;
    if (fr < 0) fr = 0;
    if (!stb_vorbis_seek(mus, (unsigned)fr)) { stb_vorbis_seek_start(mus); fr = 0; }
    rs_reset(mus_rate);
    play_pos = fr; wrapped = 0;
    pg = 0.0f;
}

static void audio_cb(void *ud, Uint8 *stream, int len) {
    (void)ud;
    Sint16 *out = (Sint16 *)stream;
    int frames = len / 4;     /* stereo S16 */
    int done = 0;
    double t_cb = sync_now();
    if (!t_first) t_first = SDL_GetPerformanceCounter();
    cur_chunk.f0 = (double)produced_frames;                 /* this chunk's first frame, as an index into everything we ever produce */
    produced_frames += (Uint64)frames;
    /* where the track is as this chunk starts, for the sync timeline (the music window asks what was in the chunk that is heard NOW) */
    const int    sc0 = scrub_on, rate0 = mus_rate, had_mus = mus != NULL;
    const double a0 = sc0 ? sc_pos : play_pos;
    const double dur0 = had_mus && mus_len > 0 ? (double)mus_len / (double)rate0 : 0.0;
    const unsigned wrap0 = wrap_n;

    int idle = mus_paused && pg <= 0.0f && !scrub_on;     /* paused and fully ramped down: leave the stream alone (a scrub still sounds) */
    if (mus && !idle) {
        if (scrub_on) done = scrub_read(out, frames);     /* dragging the progress bar: the tape head, not the stream */
        else          done = mus_read(out, frames);
        if (!scrub_on && (mus_paused || pg < 1.0f)) {                    /* the pause / resume ramp (~30 ms), so it never clicks */
            float tgt = mus_paused ? 0.0f : 1.0f, st = 1.0f / (0.030f * OUT_RATE);
            for (int i = 0; i < done; i++) {
                if (pg < tgt) { pg += st; if (pg > tgt) pg = tgt; }
                else if (pg > tgt) { pg -= st; if (pg < tgt) pg = tgt; }
                out[i * 2]     = (Sint16)(out[i * 2]     * pg);
                out[i * 2 + 1] = (Sint16)(out[i * 2 + 1] * pg);
            }
        }
        if (fading) {
            for (int i = 0; i < done; i++) {
                float g = fade_vol * fade_vol;
                out[i * 2]     = (Sint16)(out[i * 2]     * g);
                out[i * 2 + 1] = (Sint16)(out[i * 2 + 1] * g);
                fade_vol -= fade_step;
                if (fade_vol <= 0.0f) {             /* silent: drop the stream */
                    done = i + 1;                   /* rest of the buffer is zeroed below */
                    stb_vorbis_close(mus); mus = NULL;
                    free(mus_buf); mus_buf = NULL;
                    fading = 0; fade_vol = 1.0f; scrub_on = 0;
                    break;
                }
            }
        }
        if (mus && done < frames && !mus_loop) { stb_vorbis_close(mus); mus = NULL; }
    }
    {   /* the timeline record of this chunk: which track, where in it, how fast it moved (0 = paused / tape at rest, <0 = tape back) */
        SyncChunk *c = &cur_chunk;
        c->n = frames; c->track = had_mus ? track_id : 0; c->dur = dur0;
        c->wrapped = 0; c->adv = 0.0; c->p0 = 0.0; c->nm = frames;
        if (had_mus) {
            double a1 = sc0 ? sc_pos : play_pos;
            double p0s = a0 / (double)rate0, p1s = a1 / (double)rate0;
            c->p0 = p0s;
            if (!sc0 && wrap_n != wrap0 && dur0 > 0.0) { c->wrapped = 1; c->adv = (dur0 - p0s) + p1s; }
            else c->adv = p1s - p0s;
            c->nm = sc0 || idle ? frames : (done > 0 ? done : frames);
        }
    }
    an_feed(out, done);                               /* drums / notes for the music window: the song only, before master volume */
    if (master != 1.0f)
        for (int i = 0; i < done * 2; i++) out[i] = (Sint16)(out[i] * master);
    if (done < frames) memset(out + done * 2, 0, (size_t)(frames - done) * 4);
    voices_mix(out, frames);                          /* the piano keys the player plays */
    spectrum_feed(out, frames);                       /* the equalizer: song after the volume knob + the piano keys (not the ding / blips) */
    if (sfx_on) sfx_mix(out, frames);
    if (blip_on) blip_mix(out, frames);
    if (mon_on) mon_mix(out, frames);
    if (err_on) err_mix(out, frames);
    if (hit_on) hit_mix(out, frames);
    if (hint_on) hint_mix(out, frames);
    if (tap_on) tap_mix(out, frames);
    if (drag_on) drag_mix(out, frames);
    if (pfft_on) pfft_mix(out, frames);
    if (bf_on) bf_mix(out, frames);
    if (sp_on) sp_mix(out, frames);
    if (pn_on) pn_mix(out, frames);
    if (sum_on) sum_mix(out, frames);
    sync_push(&cur_chunk);                            /* the chunk is complete: what the screen can look up once it is heard */
    /* our own estimate of the clock, for apps that do not send the speaker's timestamps: at this moment the speaker is at the frame
     * (everything produced - the pipeline depth we measure), plus what we can not measure (Android's mixer, the hardware) */
    {
        double el = (double)(SDL_GetPerformanceCounter() - t_first) / (double)SDL_GetPerformanceFrequency();
        double l = (double)produced_frames / OUT_RATE - el;
        static double est_l = -1.0;
        if (el >= 2.0 && l >= 0.0 && l < 1.5) est_l = est_l < 0.0 ? l : est_l + (l - est_l) * 0.02;   /* the pipeline has filled up: its depth. (not saturated yet / paused for a long time: keep the last) */
        sync_stamp(0, (double)produced_frames - (est_l > 0.0 ? est_l : 0.0) * OUT_RATE, t_cb + (double)MUSIC_HW_LATENCY);
    }
}

int audio_init(void) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = OUT_RATE; want.format = AUDIO_S16SYS; want.channels = 2;
    want.samples = 1024; want.callback = audio_cb;      /* 23 ms chunks (was 46) */
    /* blocks until the app opens the read end of the FIFO */
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev) { printf("audio: %s\n", SDL_GetError()); fflush(stdout); return -1; }
    printf("audio %dHz ch%d fmt%x\n", have.freq, have.channels, have.format);
    fflush(stdout);
    {   /* shrink the FIFO from 64 KB (~370 ms of sound) to one page (~23 ms): less sound waiting in line = less lag */
        int fd = open("audio.pcm", O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            int got = fcntl(fd, F_SETPIPE_SZ, 4096);
            printf("audio: fifo now %d bytes\n", got); fflush(stdout);
            close(fd);
        }
    }
    sync_reset();
    trim_load();
    SDL_PauseAudioDevice(dev, 0);
    return 0;
}

static unsigned char *slurp(const char *path, int *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *b = n > 0 ? malloc((size_t)n) : NULL;
    if (b && fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); b = NULL; }
    fclose(f);
    if (b) *len = (int)n;
    return b;
}

int music_play(const char *file, int loop) {
    if (!dev) return -1;

    char path[1024], dir[900];
    unsigned char *buf = NULL;
    int len = 0;

    ssize_t n = readlink("/proc/self/exe", dir, sizeof dir - 1);
    if (n > 0) {
        dir[n] = 0;
        char *slash = strrchr(dir, '/');
        if (slash) {
            *slash = 0;
            snprintf(path, sizeof path, "%s/%s", dir, file);
            buf = slurp(path, &len);
            if (buf) { printf("music: %s\n", path); fflush(stdout); }
        }
    }
    if (!buf) {                                  /* cwd */
        buf = slurp(file, &len);
        if (buf) { printf("music: ./%s\n", file); fflush(stdout); }
    }
    if (!buf) { printf("music: %s not found\n", file); fflush(stdout); return -1; }

    int err = 0;
    stb_vorbis *v = stb_vorbis_open_memory(buf, len, &err, NULL);
    if (!v) { printf("music: bad ogg (err %d)\n", err); fflush(stdout); free(buf); return -1; }

    stb_vorbis_info vi = stb_vorbis_get_info(v);
    printf("music: %u Hz, %d ch\n", vi.sample_rate, vi.channels);
    if (vi.sample_rate != OUT_RATE)
        printf("music: resampling %u Hz -> %d Hz\n", vi.sample_rate, OUT_RATE);
    fflush(stdout);

    cache_free();                                /* the old track's decoded copy is useless now (also ends a scrub) */
    int total = (int)stb_vorbis_stream_length_in_samples(v);      /* before the stream is shared with the audio thread */
    SDL_LockAudioDevice(dev);
    stb_vorbis *old = mus; unsigned char *oldbuf = mus_buf;
    mus = v; mus_buf = buf; mus_loop = loop;
    mus_len = total > 0 ? total : 0; mus_enc_len = len; play_pos = 0; wrapped = 0; scrub_on = 0;
    track_id++;
    rs_reset((int)vi.sample_rate);
    fading = 0; fade_vol = 1.0f;
    SDL_UnlockAudioDevice(dev);
    if (old) stb_vorbis_close(old);
    free(oldbuf);
    if (want_cache) cache_start();

    snprintf(cur_file, sizeof cur_file, "%s", file);
    music_title_of(file, title, sizeof title);
    serial++;
    return 0;
}

/* "divine_tale.ogg" -> "Divine Tale" (also strips any folder in front) */
void music_title_of(const char *file, char *out, int cap) {
    const char *base = strrchr(file, '/'); base = base ? base + 1 : file;
    int k = 0, cp = 1;
    for (; base[k] && base[k] != '.' && k < cap - 1; k++) {
        char c = base[k] == '_' ? ' ' : base[k];
        if (cp && c >= 'a' && c <= 'z') c = (char)(c - 32);
        cp = c == ' ';
        out[k] = c;
    }
    out[k] = 0;
}

const char *music_current_file(void) { return mus ? cur_file : ""; }
float music_volume(void) { return master; }
int   music_paused(void) { return mus_paused; }
int   music_fading(void) { return fading; }
void  music_pause(int p) { mus_paused = p ? 1 : 0; }     /* the audio thread ramps pg towards the new state */

int  music_loop(void) { return mus_loop; }
void music_set_loop(int on) {
    if (!dev) { mus_loop = on ? 1 : 0; return; }
    SDL_LockAudioDevice(dev);
    mus_loop = on ? 1 : 0;
    SDL_UnlockAudioDevice(dev);
}

/* ---------- position, seeking, scrub (the music window's progress bar) ---------- */
double music_duration(void) { return mus && mus_len > 0 ? (double)mus_len / (double)mus_rate : 0.0; }

/* ---------- what is on the speaker, at the moment the picture reaches the eye ----------
 * the music window (bars, lit notes, drums, the progress bar) must show what is HEARD at the instant its frame is on the glass:
 *   t_eye   = now + display delay (the game's frame -> the app's next vsync -> the compositor -> the panel) + trim
 *   frame   = the stream frame at the speaker at t_eye        (sync clock: the AudioTrack's own timestamps, or an estimate)
 *   what    = the timeline's record of that frame             (position, speed, spectrum)
 * the display delay is a number of vsyncs, so the app tells us the refresh rate ("v"); the trim is the player's last word. */
#define MUSIC_DISPLAY_VSYNCS 2.5f      /* the game's frame waits for the app's next vsync (~0.5), is drawn, composited (1), scanned out (1) */
static volatile float disp_delay = MUSIC_DISPLAY_VSYNCS / 60.0f;
static volatile float trim_s;

void audio_set_refresh(float hz) {
    if (hz >= 24.0f && hz <= 240.0f) disp_delay = MUSIC_DISPLAY_VSYNCS / hz;
}

/* "sync_trim_ms" next to the game (a number of milliseconds): + shows the music window EARLIER (use it when the lit notes trail the sound),
 * - later. read at start and every time the music window opens. */
static void trim_load(void) {
    float ms = 0.0f;
    FILE *f = fopen("sync_trim_ms", "r");
    if (f) { if (fscanf(f, "%f", &ms) != 1) ms = 0.0f; fclose(f); }
    if (!(ms > -500.0f)) ms = -500.0f;
    if (ms > 500.0f) ms = 500.0f;
    if (ms != trim_s * 1000.0f) { printf("sync: trim %.0f ms\n", ms); fflush(stdout); }
    trim_s = ms / 1000.0f;
}

/* the stream frame the player is looking at right now. never steps back for jitter (a drum hit must not fire twice) */
static double view_frame(void) {
    static double last_f = -1.0;
    if (!sync_ready()) return -1.0;
    double f = sync_frame_at(sync_now() + (double)disp_delay + (double)trim_s);
    if (f < last_f && last_f - f < 0.05 * OUT_RATE) f = last_f;
    last_f = f;
    return f;
}

/* the speaker's own timestamp, from the app: stream frame `frame` was at the speaker at CLOCK_MONOTONIC `nano` ns */
void audio_hw_stamp(long long frame, long long nano) {
    static unsigned bad, good;
    double t = (double)nano * 1e-9, now = sync_now();
    if (frame <= 0 || t > now + 0.25 || t < now - 5.0) {          /* nonsense, or another clock: ignore (we stay on the estimate) */
        if (++bad == 1 || bad % 200 == 0) { printf("sync: hw stamp ignored (frame %lld, %.3f s from now)\n", frame, t - now); fflush(stdout); }
        return;
    }
    sync_stamp(1, (double)frame, t);
    if (++good == 1 || good % 250 == 0) {
        double heard = sync_frame_at(now);
        printf("sync: hw clock, pipeline %.0f ms (estimate said %.0f), display %.0f ms, trim %.0f ms\n",
               ((double)produced_frames - heard) * 1000.0 / OUT_RATE, (double)(audio_latency() + MUSIC_HW_LATENCY) * 1000.0,
               (double)disp_delay * 1000.0, (double)trim_s * 1000.0);
        fflush(stdout);
    }
}

/* where the track is in what the player sees now. NOT clamped at 0: negative = the start of a new track is still on its way to the
 * speakers (the music window stays dark until it arrives). 0 when nothing is playing */
static int heard_at(double *pos, double *spd) { return sync_position_at(view_frame(), track_id, pos, spd); }

double music_heard_position(void) {
    double d = music_duration();
    if (d <= 0.0) return 0.0;
    double p, v;
    int st = heard_at(&p, &v);
    if (st == SYNC_NOT_ARRIVED) return -1.0;
    if (st != SYNC_OK) return 0.0;
    return p > d ? d : p;
}

/* the sound at the speaker is moving through the track (playing, or a tape that is turning), not paused / at rest */
int music_heard_playing(void) {
    double p, v;
    return music_duration() > 0.0 && heard_at(&p, &v) == SYNC_OK && fabs(v) > 0.05;
}

double music_position(void) {
    double p = music_heard_position();
    return p < 0.0 ? 0.0 : p;
}

void music_seek(double sec) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    if (!scrub_on) seek_locked(sec * (double)mus_rate);
    SDL_UnlockAudioDevice(dev);
}

void music_scrub_prepare(void) { trim_load(); want_cache = 1; if (!cache && !dec_thread) cache_start(); }
void music_scrub_release(void) { want_cache = 0; cache_free(); }

int music_scrub_begin(double sec) {
    if (!dev) return -1;
    int ret = -1;
    SDL_LockAudioDevice(dev);
    int ready = cache ? __atomic_load_n(&cache_ready, __ATOMIC_ACQUIRE) : 0;
    if (mus && cache && mus_len > 0 && (ready >= 2 * mus_rate || ready >= mus_len)) {    /* wait until there is something to play with */
        double hi = (double)(ready - 2 > 0 ? ready - 2 : 0);
        double f = sec * (double)mus_rate;
        if (f < 0) f = 0;
        if (f > hi) f = hi;
        sc_finger = f;
        if (fabs(f - play_pos) < 2.0 * mus_rate) {           /* finger landed on the head: carry on at the current speed, no jump */
            sc_pos = play_pos < hi ? play_pos : hi;
            sc_rate = mus_paused ? 0.0f : 1.0f;
        } else { sc_pos = f; sc_rate = 0.0f; }               /* a tap somewhere else: the head jumps there and the tape rests */
        scrub_on = 1;
        ret = 0;
    }
    SDL_UnlockAudioDevice(dev);
    return ret;
}

void music_scrub_to(double sec) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    if (scrub_on) {
        double f = sec * (double)mus_rate;
        sc_finger = f < 0 ? 0 : f > mus_len - 1 ? mus_len - 1 : f;
    }
    SDL_UnlockAudioDevice(dev);
}

void music_scrub_end(double sec) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    if (scrub_on) {
        scrub_on = 0;
        seek_locked(sec < 0 ? sc_pos : sec * (double)mus_rate);
    }
    SDL_UnlockAudioDevice(dev);
}

int   music_scrubbing(void) { return scrub_on; }
float music_scrub_rate(void) { return scrub_on ? sc_rate : 0.0f; }
float music_scrub_ready(void) {
    int r = __atomic_load_n(&cache_ready, __ATOMIC_ACQUIRE);
    return cache && cache_cap > 0 ? (float)r / (float)cache_cap : 0.0f;
}

/* ---------- the list of tracks the player can pick from ---------- */
static char tracks[MUSIC_MAX_TRACKS][128];
static int  ntracks;

static int cmp_names(const void *a, const void *b) { return strcasecmp((const char *)a, (const char *)b); }

static int scan_dir(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) return 0;
    struct dirent *e;
    int n = 0;
    while ((e = readdir(d)) && n < MUSIC_MAX_TRACKS) {
        size_t nl = strlen(e->d_name);
        if (nl < 5 || nl >= sizeof tracks[0] || strcasecmp(e->d_name + nl - 4, ".ogg") != 0) continue;
        memcpy(tracks[n], e->d_name, nl + 1);
        n++;
    }
    closedir(d);
    if (n > 1) qsort(tracks, (size_t)n, sizeof tracks[0], cmp_names);
    return n;
}

int music_scan(void) {
    char dir[900];
    ssize_t n = readlink("/proc/self/exe", dir, sizeof dir - 1);
    ntracks = 0;
    if (n > 0) {                                  /* same lookup order as music_play: next to the executable ... */
        dir[n] = 0;
        char *slash = strrchr(dir, '/');
        if (slash) { *slash = 0; ntracks = scan_dir(dir); }
    }
    if (ntracks == 0) ntracks = scan_dir(".");    /* ... then the working directory */
    return ntracks;
}

const char *music_scan_file(int i) { return i >= 0 && i < ntracks ? tracks[i] : ""; }

void music_set_volume(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (!dev) { master = v; return; }
    SDL_LockAudioDevice(dev);
    master = v;                                  /* takes effect on the very next buffer, no ramp */
    SDL_UnlockAudioDevice(dev);
}

void music_fade_out(float seconds) {
    if (!dev) return;
    if (seconds < 0.05f) seconds = 0.05f;
    SDL_LockAudioDevice(dev);
    if (mus) { fade_step = 1.0f / (seconds * OUT_RATE); fading = 1; }
    SDL_UnlockAudioDevice(dev);
}

void music_stop(void) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    stb_vorbis *old = mus; unsigned char *oldbuf = mus_buf;
    mus = NULL; mus_buf = NULL; fading = 0; fade_vol = 1.0f; scrub_on = 0;
    SDL_UnlockAudioDevice(dev);
    if (old) stb_vorbis_close(old);
    free(oldbuf);
}

void audio_quit(void) {
    music_stop();
    want_cache = 0; cache_free();
    if (dev) { SDL_CloseAudioDevice(dev); dev = 0; }
}
