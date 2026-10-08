/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

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

#define OUT_RATE 44100

static SDL_AudioDeviceID dev;
static stb_vorbis *mus;       /* guarded by SDL_LockAudioDevice */
static unsigned char *mus_buf; /* encoded file, must outlive mus */
static int mus_loop;
/* fade: gain = fade_vol^2 (perceptual), ramps to 0 over fade_frames; guarded by device lock */
static int   fading;
static float fade_vol = 1.0f, fade_step;
static float master = 1.0f;   /* instant master gain, set via music_set_volume */

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

/* ---------- the dialog blip: one short 25%-pulse square "dod" per letter, like every pixel-game text box ----------
 * its own voice, so it never cuts off the coin ding (and the coin never cuts off a blip). */
static int   blip_on, blip_pos;
static float blip_phase, blip_f;
#define BLIP_T 0.050f                          /* seconds per blip */

void sfx_blip(float pitch) {
    if (!dev) return;
    if (pitch < 0.4f) pitch = 0.4f;
    if (pitch > 2.5f) pitch = 2.5f;
    SDL_LockAudioDevice(dev);
    blip_on = 1; blip_pos = 0; blip_phase = 0.0f; blip_f = 520.0f * pitch;
    SDL_UnlockAudioDevice(dev);
}

static void blip_mix(Sint16 *out, int frames) {
    for (int i = 0; i < frames; i++) {
        float tt = (float)blip_pos / (float)OUT_RATE;
        if (tt >= BLIP_T) { blip_on = 0; return; }
        float env = 1.0f - tt / BLIP_T;                              /* snappy linear decay: the "d" then "od" */
        env *= env;
        if (tt < 0.002f) env *= tt / 0.002f;                         /* no click on the way in */
        float f = blip_f * (1.0f - 0.12f * (tt / BLIP_T));           /* a tiny downward chirp gives it the "dod" */
        blip_phase += f / (float)OUT_RATE;
        if (blip_phase >= 1.0f) blip_phase -= 1.0f;
        float sq = blip_phase < 0.25f ? 1.0f : -1.0f;
        int v = (int)(sq * env * 0.13f * 32767.0f);
        for (int c = 0; c < 2; c++) {
            int o = out[i * 2 + c] + v;
            out[i * 2 + c] = (Sint16)(o > 32767 ? 32767 : o < -32768 ? -32768 : o);
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

/* ---------- spectrum analysis (runs in the audio thread on the samples about to be played) ---------- */
#define FFT_N 512
static volatile float spec[MUSIC_BANDS];
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
    if (frames < FFT_N) { for (int i = 0; i < MUSIC_BANDS; i++) spec[i] = 0; return; }
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
        float lvl = (db + 62.0f + 0.7f * b) / 46.0f;                                /* tilt: highs are naturally quieter */
        spec[b] = lvl < 0 ? 0 : lvl > 1 ? 1 : lvl;
    }
}

void music_spectrum(float *bands) { for (int i = 0; i < MUSIC_BANDS; i++) bands[i] = spec[i]; }
const char *music_title(void) { return title; }
unsigned music_serial(void) { return serial; }

static void audio_cb(void *ud, Uint8 *stream, int len) {
    (void)ud;
    Sint16 *out = (Sint16 *)stream;
    int frames = len / 4;     /* stereo S16 */
    int done = 0;

    if (mus) {
        int seeked = 0;
        while (done < frames) {
            int got = stb_vorbis_get_samples_short_interleaved(
                mus, 2, out + done * 2, (frames - done) * 2);
            if (got > 0) { done += got; seeked = 0; continue; }
            if (!mus_loop || seeked) break;       /* end, or broken stream */
            stb_vorbis_seek_start(mus);
            seeked = 1;
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
                    fading = 0; fade_vol = 1.0f;
                    break;
                }
            }
        }
        if (mus && done < frames && !mus_loop) { stb_vorbis_close(mus); mus = NULL; }
    }
    if (done >= FFT_N) spectrum_feed(out, done);      /* before master volume, so the bars ignore the volume knob */
    else               for (int i = 0; i < MUSIC_BANDS; i++) spec[i] = 0;
    if (master != 1.0f)
        for (int i = 0; i < done * 2; i++) out[i] = (Sint16)(out[i] * master);
    if (done < frames) memset(out + done * 2, 0, (size_t)(frames - done) * 4);
    if (sfx_on) sfx_mix(out, frames);
    if (blip_on) blip_mix(out, frames);
}

int audio_init(void) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = OUT_RATE; want.format = AUDIO_S16SYS; want.channels = 2;
    want.samples = 2048; want.callback = audio_cb;
    /* blocks until the app opens the read end of the FIFO */
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev) { printf("audio: %s\n", SDL_GetError()); fflush(stdout); return -1; }
    printf("audio %dHz ch%d fmt%x\n", have.freq, have.channels, have.format);
    fflush(stdout);
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
        printf("music: WARNING expected %d Hz, will play at wrong speed\n", OUT_RATE);
    fflush(stdout);

    SDL_LockAudioDevice(dev);
    stb_vorbis *old = mus; unsigned char *oldbuf = mus_buf;
    mus = v; mus_buf = buf; mus_loop = loop;
    fading = 0; fade_vol = 1.0f;
    SDL_UnlockAudioDevice(dev);
    if (old) stb_vorbis_close(old);
    free(oldbuf);

    /* title from the file name: "divine_tale.ogg" -> "Divine Tale" */
    const char *base = strrchr(file, '/'); base = base ? base + 1 : file;
    size_t k = 0; int cap = 1;
    for (; base[k] && base[k] != '.' && k < sizeof title - 1; k++) {
        char c = base[k] == '_' ? ' ' : base[k];
        if (cap && c >= 'a' && c <= 'z') c = (char)(c - 32);
        cap = c == ' ';
        title[k] = c;
    }
    title[k] = 0;
    serial++;
    return 0;
}

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
    mus = NULL; mus_buf = NULL; fading = 0; fade_vol = 1.0f;
    SDL_UnlockAudioDevice(dev);
    if (old) stb_vorbis_close(old);
    free(oldbuf);
}

void audio_quit(void) {
    music_stop();
    if (dev) { SDL_CloseAudioDevice(dev); dev = 0; }
}
