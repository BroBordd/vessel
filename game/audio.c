#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STB_VORBIS_NO_PUSHDATA_API
#include "third_party/stb_vorbis.c"

#include "audio.h"

#define OUT_RATE 44100

static SDL_AudioDeviceID dev;
static stb_vorbis *mus;       /* guarded by SDL_LockAudioDevice */
static int mus_loop;

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
        if (done < frames && !mus_loop) { stb_vorbis_close(mus); mus = NULL; }
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

static stb_vorbis *try_open(const char *path) {
    int err = 0;
    stb_vorbis *v = stb_vorbis_open_filename(path, &err, NULL);
    if (v) { printf("music: %s\n", path); fflush(stdout); }
    return v;
}

int music_play(const char *file, int loop) {
    if (!dev) return -1;

    char path[1024], dir[1024];
    stb_vorbis *v = NULL;

    ssize_t n = readlink("/proc/self/exe", dir, sizeof dir - 1);
    if (n > 0) {
        dir[n] = 0;
        char *slash = strrchr(dir, '/');
        if (slash) {
            *slash = 0;
            snprintf(path, sizeof path, "%s/%s", dir, file);
            v = try_open(path);
        }
    }
    if (!v) v = try_open(file);            /* cwd */
    if (!v) { printf("music: %s not found\n", file); fflush(stdout); return -1; }

    stb_vorbis_info vi = stb_vorbis_get_info(v);
    printf("music: %u Hz, %d ch\n", vi.sample_rate, vi.channels);
    if (vi.sample_rate != OUT_RATE)
        printf("music: WARNING expected %d Hz, will play at wrong speed\n", OUT_RATE);
    fflush(stdout);

    SDL_LockAudioDevice(dev);
    stb_vorbis *old = mus;
    mus = v; mus_loop = loop;
    SDL_UnlockAudioDevice(dev);
    if (old) stb_vorbis_close(old);
    return 0;
}

void music_stop(void) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    stb_vorbis *old = mus;
    mus = NULL;
    SDL_UnlockAudioDevice(dev);
    if (old) stb_vorbis_close(old);
}

void audio_quit(void) {
    music_stop();
    if (dev) { SDL_CloseAudioDevice(dev); dev = 0; }
}
