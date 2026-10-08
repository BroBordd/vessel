#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

static volatile int a_on = 0;
static volatile float a_freq = 440.0f;

static void audio_cb(void *ud, Uint8 *stream, int len) {
    static float phase = 0;
    Sint16 *o = (Sint16 *)stream;
    int n = len / 4;                      /* stereo S16 */
    for (int i = 0; i < n; i++) {
        Sint16 s = 0;
        if (a_on) {
            s = (Sint16)(sinf(phase) * 8000);
            phase += 6.2831853f * a_freq / 44100.0f;
            if (phase > 6.2831853f) phase -= 6.2831853f;
        }
        o[i * 2] = s; o[i * 2 + 1] = s;
    }
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: game w h\n"); return 1; }
    int W = atoi(argv[1]), H = atoi(argv[2]);

    int fd = open("fb", O_RDWR);
    if (fd < 0) { perror("open fb"); return 1; }
    void *px = mmap(NULL, (size_t)W * H * 4, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);
    if (px == MAP_FAILED) { perror("mmap"); return 1; }

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    SDL_setenv("SDL_DISKAUDIOFILE", "audio.pcm", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init: %s\n", SDL_GetError()); fflush(stdout); return 1;
    }

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(
        px, W, H, 32, W * 4, SDL_PIXELFORMAT_RGBA32);
    if (!surf) { printf("surface: %s\n", SDL_GetError()); fflush(stdout); return 1; }
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    if (!r) { printf("renderer: %s\n", SDL_GetError()); fflush(stdout); return 1; }

    SDL_version v; SDL_GetVersion(&v);
    printf("SDL %d.%d.%d, %dx%d\n", v.major, v.minor, v.patch, W, H);
    fflush(stdout);

    /* blocks until the app opens the read end of the FIFO */
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100; want.format = AUDIO_S16SYS; want.channels = 2;
    want.samples = 1024; want.callback = audio_cb;
    SDL_AudioDeviceID ad = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!ad) { printf("audio: %s\n", SDL_GetError()); fflush(stdout); }
    else {
        printf("audio %dHz ch%d fmt%x\n", have.freq, have.channels, have.format);
        fflush(stdout);
        SDL_PauseAudioDevice(ad, 0);
    }

    fcntl(0, F_SETFL, O_NONBLOCK);
    char acc[1024]; int alen = 0;
    int tx = W / 2, ty = H / 2, down = 0;
    float bx = 50, by = 50, vx = 3, vy = 4;
    Uint32 last = SDL_GetTicks(); int frames = 0;

    for (;;) {
        char buf[256];
        ssize_t n = read(0, buf, sizeof buf);
        if (n == 0) break;
        if (n > 0) {
            if (alen + n >= (int)sizeof acc) alen = 0;
            memcpy(acc + alen, buf, n); alen += n;
            char *s = acc, *nl;
            while ((nl = memchr(s, '\n', acc + alen - s))) {
                *nl = 0;
                int a, x, y;
                if (sscanf(s, "t %d %d %d", &a, &x, &y) == 3) {
                    tx = x; ty = y; down = (a != 1 && a != 3);
                    a_freq = 220.0f + 660.0f * (1.0f - (float)y / H);
                    a_on = down;
                }
                s = nl + 1;
            }
            alen -= s - acc; memmove(acc, s, alen);
        }

        bx += vx; by += vy;
        if (bx < 0 || bx > W - 40) vx = -vx;
        if (by < 0 || by > H - 40) vy = -vy;

        SDL_SetRenderDrawColor(r, 20, 20, 40, 255);
        SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 255, 80, 80, 255);
        SDL_Rect strip = {0, 0, W, 12};
        SDL_RenderFillRect(r, &strip);
        SDL_SetRenderDrawColor(r, 80, 255, 120, 255);
        SDL_Rect ball = {(int)bx, (int)by, 40, 40};
        SDL_RenderFillRect(r, &ball);
        int s2 = down ? 30 : 20;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_Rect t = {tx - s2, ty - s2, s2 * 2, s2 * 2};
        SDL_RenderFillRect(r, &t);
        SDL_RenderPresent(r);

        if (++frames == 60) {
            Uint32 now = SDL_GetTicks();
            printf("fps %.1f\n", 60000.0 / (now - last)); fflush(stdout);
            last = now; frames = 0;
        }
        SDL_Delay(16);
    }

    if (ad) SDL_CloseAudioDevice(ad);
    SDL_DestroyRenderer(r);
    SDL_FreeSurface(surf);
    SDL_Quit();
    return 0;
}
