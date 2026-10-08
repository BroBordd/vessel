/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    if (done < frames) memset(out + done * 2, 0, (size_t)(frames - done) * 4);
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
    return 0;
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
