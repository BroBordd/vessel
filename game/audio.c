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
static volatile int wrapped;      /* the track has looped since it started / was last seeked (heard position wraps with it) */
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

static void rs_reset(int rate) {
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
    if (mus_rate == OUT_RATE) {
        while (done < frames) {
            int got = stb_vorbis_get_samples_short_interleaved(mus, 2, out + done * 2, (frames - done) * 2);
            if (got > 0) { done += got; play_pos += got; reseeked = 0; continue; }
            if (!mus_loop || reseeked) break;       /* end, or broken stream */
            stb_vorbis_seek_start(mus);
            play_pos = 0; wrapped = 1;
            reseeked = 1;
        }
        return done;
    }
    float step = (float)mus_rate / (float)OUT_RATE;
    while (done < frames) {
        while (rs_pos >= 1.0f) {
            Sint16 f[2];
            if (!rs_next(f)) {
                if (!mus_loop || reseeked) return done;
                stb_vorbis_seek_start(mus); rs_len = rs_idx = 0; play_pos = 0; wrapped = 1; reseeked = 1;
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
#define COIN_NOTE1_T 0.085f                    /* the short first note, seconds */
#define COIN_END_T   0.75f                     /* total length */

/* the classic two-note coin: a quick B5, then E6 ringing out */
void sfx_coin(void) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    sfx_on = 1; sfx_pos = 0; sfx_phase = 0.0f;
    SDL_UnlockAudioDevice(dev);
}

/* ---------- the dialog "deek": one short, clean tick per letter, like a modern text-box blip ----------
 * a pure sine at a steady pitch (no square wave, no pitch slide: nothing to make it buzzy or noisy),
 * with a quick attack and a smooth fade so it never clicks.
 * its own voice, so it never cuts off the coin ding (and the coin never cuts off a deek). */
static int   blip_on, blip_pos;
static float blip_phase, blip_f;
#define BLIP_T    0.060f                       /* seconds per deek */
#define BLIP_HZ   256.5f                       /* base pitch at pitch 1.0 (was 190 Hz; +35% = 256.5 Hz) */
#define BLIP_GAIN 0.50f                        /* peak level, fraction of full scale (was 0.20; the coin is 0.30) */

void sfx_blip(float pitch) {
    if (!dev) return;
    if (pitch < 0.4f) pitch = 0.4f;
    if (pitch > 3.0f) pitch = 3.0f;
    SDL_LockAudioDevice(dev);
    blip_on = 1; blip_pos = 0; blip_phase = 0.0f; blip_f = BLIP_HZ * pitch;
    SDL_UnlockAudioDevice(dev);
}

static void blip_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)blip_pos / (float)OUT_RATE;
        if (tt >= BLIP_T) { blip_on = 0; return; }
        float env = expf(-tt * 22.0f);                               /* crisp decay: a tick, not a ring */
        if (tt < 0.003f) env *= tt / 0.003f;                         /* no click on the way in */
        if (tt > BLIP_T - 0.010f) env *= (BLIP_T - tt) / 0.010f;     /* no click on the way out */
        blip_phase += blip_f / (float)OUT_RATE;                      /* steady pitch, no sag */
        if (blip_phase >= 1.0f) blip_phase -= 1.0f;
        int v = (int)(sinf(6.2831853f * blip_phase) * env * BLIP_GAIN * 32767.0f);   /* pure sine */
        for (int ch = 0; ch < 2; ch++) {
            int o = out[i * 2 + ch] + v;
            out[i * 2 + ch] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
        }
        blip_pos++;
    }
}

static void sfx_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)sfx_pos / (float)OUT_RATE;
        if (tt >= COIN_END_T) { sfx_on = 0; return; }
        float f, env;
        if (tt < COIN_NOTE1_T) { f = 987.77f;  env = 1.0f; }
        else                   { f = 1318.51f; env = expf(-(tt - COIN_NOTE1_T) * 6.5f); }
        if (tt < 0.003f) env *= tt / 0.003f;                       /* no click on the way in */
        sfx_phase += f / (float)OUT_RATE;
        if (sfx_phase >= 1.0f) sfx_phase -= 1.0f;
        float sq = sfx_phase < 0.5f ? 1.0f : -1.0f;                /* chiptune pulse + a little sine for the shine */
        float sn = sinf(6.2831853f * sfx_phase);
        int v = (int)((0.55f * sq + 0.45f * sn) * env * 0.30f * 32767.0f);
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

static float vis_delay(void);

/* ---------- spectrum analysis (runs in the audio thread on the samples about to be played) ---------- */
#define FFT_N 512
static volatile float spec[MUSIC_BANDS];
#define SPEC_HIST 128                                   /* ~3 s of 1024-frame chunks, the delay line for the bars */
static float spec_hist[SPEC_HIST][MUSIC_BANDS];
static volatile unsigned spec_n;                        /* chunks pushed so far (one per audio callback) */
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
static void spec_push(void) {
    unsigned n = spec_n;
    for (int i = 0; i < MUSIC_BANDS; i++) spec_hist[n % SPEC_HIST][i] = spec[i];
    spec_n = n + 1;
}

static void spectrum_feed(const Sint16 *pcm, int frames) {
    if (frames < FFT_N) { for (int i = 0; i < MUSIC_BANDS; i++) spec[i] = 0; spec_push(); return; }
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
        spec[b] = lvl < 0 ? 0 : lvl > 1 ? 1 : lvl;
    }
    spec_push();
}

/* the bars show what is HEARD: the chunk that was mixed vis_delay() seconds ago, not the one being mixed now */
void music_spectrum(float *bands) {
    unsigned n = spec_n;
    unsigned back = (unsigned)(vis_delay() * (float)OUT_RATE / 1024.0f + 0.5f);
    if (back > SPEC_HIST - 2) back = SPEC_HIST - 2;
    if (n == 0 || back >= n) { for (int i = 0; i < MUSIC_BANDS; i++) bands[i] = 0; return; }
    const float *s = spec_hist[(n - 1 - back) % SPEC_HIST];
    for (int i = 0; i < MUSIC_BANDS; i++) bands[i] = s[i];
}
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
    if (!t_first) t_first = SDL_GetPerformanceCounter();
    produced_frames += (Uint64)frames;

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
    an_feed(out, done);                               /* drums / notes for the music window: the song only, before master volume */
    if (master != 1.0f)
        for (int i = 0; i < done * 2; i++) out[i] = (Sint16)(out[i] * master);
    if (done < frames) memset(out + done * 2, 0, (size_t)(frames - done) * 4);
    voices_mix(out, frames);                          /* the piano keys the player plays */
    spectrum_feed(out, frames);                       /* the equalizer: song after the volume knob + the piano keys (not the ding / blips) */
    if (sfx_on) sfx_mix(out, frames);
    if (blip_on) blip_mix(out, frames);
    if (mon_on) mon_mix(out, frames);
    if (sum_on) sum_mix(out, frames);
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

/* seconds between what we mix and what the ear gets: the measured pipeline (game -> FIFO -> AudioTrack) plus the
 * part nobody can measure from here (Android's mixer / the hardware), MUSIC_HW_LATENCY */
static float vis_delay(void) { return audio_latency() + MUSIC_HW_LATENCY; }

/* where the track is in what the ear hears right now. NOT clamped at 0: negative = the start of a new track is
 * still on its way to the speakers (the music window stays dark until it arrives) */
double music_heard_position(void) {
    double d = music_duration();
    if (d <= 0.0) return 0.0;
    if (scrub_on) return sc_pos / (double)mus_rate > d ? d : sc_pos / (double)mus_rate;
    double p = play_pos / (double)mus_rate - (double)vis_delay();
    if (p < 0.0 && wrapped) p += d;              /* the loop restarted in the decoder, the ear is still at the end */
    return p > d ? d : p;
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

void music_scrub_prepare(void) { want_cache = 1; if (!cache && !dec_thread) cache_start(); }
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
