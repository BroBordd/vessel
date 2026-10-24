/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>

#include "audio.h"
#include "jukebox.h"
#include "menu.h"
#include "world.h"
#include "dialog.h"
#include "space.h"
#include "story.h"
#include "nowplaying.h"
#include "thought.h"
#include "musicwin.h"
#include "pausebtn.h"
#include "brainwin.h"
#include "mapwin.h"
#include "lang.h"
#include "screen.h"
#include "loading.h"
#include "gfx.h"

/* ST_LIMBO (vessel 2): the space screen, the stars and the top buttons only, no map and no HUD. the soul waits here
 * at the start of the game and after every death. the brain button works (thoughts), the pause button does not. */
typedef enum { ST_MENU, ST_WORLD, ST_LIMBO } State;
/* PLAY -> the menu fades out (menu.c) -> ST_LIMBO. no loading screen. the story drives limbo (story_limbo_begin: "Where am I?", a hold,
 * limbo_end), then the world fades in on the cloud island and the summoning runs (chunk 10) */
/* The game is a child of the app process. Closing the activity needs Java,
 * so for now exit == take the parent app down with us. */
static void quit_app(void) {
    pid_t pp = getppid();
    if (pp > 1) kill(pp, SIGKILL);
    _exit(0);
}

/* the Android back button (the app sends "k 4"). it never leaves the game: it closes the foreground-most window (the one that opened last, in
 * case several are stacked), else pauses / resumes the world. when there is nothing for it to do (the menu, limbo) it says so and the app
 * sends itself to the background instead. */
enum { WIN_MAP, WIN_MUSIC, WIN_BRAIN, WIN_N };
static int win_order[WIN_N], win_n;                         /* the open windows, oldest first: the last one is in front */

static int win_active(int w) {
    return w == WIN_MAP ? mapwin_active() : w == WIN_MUSIC ? musicwin_active() : brainwin_active();
}
static void win_close(int w) {
    if (w == WIN_MAP) mapwin_close(); else if (w == WIN_MUSIC) musicwin_close(); else brainwin_close();
}
/* called every frame: windows that just opened go on top of the stack, windows that are gone leave it */
static void win_track(void) {
    int k = 0;
    for (int i = 0; i < win_n; i++) if (win_active(win_order[i])) win_order[k++] = win_order[i];
    win_n = k;
    for (int w = 0; w < WIN_N; w++) {
        if (!win_active(w)) continue;
        int in = 0;
        for (int i = 0; i < win_n; i++) if (win_order[i] == w) in = 1;
        if (!in) win_order[win_n++] = w;
    }
}

static int handle_back(State st) {
    win_track();                                            /* (also catches a window that opened since the last frame) */
    if (win_n > 0) { win_close(win_order[win_n - 1]); return 1; }   /* the front window; one still sliding away just swallows the press */
    if (st == ST_WORLD && !world_dawning()) {
        int was = pausebtn_paused();
        pausebtn_set_paused(!was);
        if (!was) world_touch(3, 0, 0);                     /* just paused: let go of the stick */
        return 1;
    }
    return 0;
}

/* the game's logical size changes here and nowhere else (rotation plan, design 5): called at a frame boundary only, with the latest size. cancels the
 * held touches, tells the recorder, then every module re-lays itself out and keeps its state. which ones depend on the state: the menu is on its own
 * (menu_resize also moves the stars); limbo and the world have run world_init, so world_resize does the HUD, the dialog, the controls; modules
 * that do not exist yet (the world before the menu fades) simply read the new W/H when they are created, because W/H are main's locals. */
static void apply_size(int *W, int *H, int nw, int nh, int state) {
    if (nw == *W && nh == *H) return;
    if (state == ST_WORLD) world_touch(3, 0, 0);            /* let go of the stick and the buttons, like pausing does */
    /* ...and of whatever a window holds: the map's pan / pinch, the piano's keys, the seek drag, the brain window's buttons, the cards, the menu's pressed
     * button. a = 3 only ever clears (never fires); the finger that is still down sends moves and a lift that now find nothing to hold */
    mapwin_touch(3, 0, 0);
    musicwin_cancel();
    brainwin_touch(3, 0, 0);
    nowplaying_touch(3, 0, 0);
    thought_touch(3, 0, 0);
    pausebtn_touch(3, 0, 0);
    if (state == ST_MENU) menu_touch(3, 0, 0);
    *W = nw; *H = nh;
    gfx_resize(nw, nh);
    nowplaying_resize(nw, nh);
    musicwin_resize(nw, nh);
    brainwin_resize(nw, nh);
    pausebtn_resize(nw, nh);
    loading_resize(nw, nh);
    if (state == ST_MENU) {
        menu_resize(nw, nh);                                /* (and the stars under it) */
    } else {
        space_resize(nw, nh);                               /* limbo, and the stars under the opening's dawn */
        world_resize(nw, nh);                               /* the HUD, the dialog (so limbo's popup too), the controls, the map window, the camera */
    }
    printf("resize: %dx%d\n", nw, nh); fflush(stdout);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: game w h\n"); return 1; }
    int W = atoi(argv[1]), H = atoi(argv[2]);
    /* third argument "gpu": the app replays our draw calls with OpenGL ES (see gfx.h).
     * anything else: draw on the CPU into the shared "fb" pixel file like before. */
    int gpu = argc > 3 && strcmp(argv[3], "gpu") == 0;

    void *px = NULL;
    if (!gpu) {
        int fd = open("fb", O_RDWR);
        if (fd < 0) { perror("open fb"); return 1; }
        px = mmap(NULL, (size_t)W * H * 4, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
        if (px == MAP_FAILED) { perror("mmap"); return 1; }
    }

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    SDL_setenv("SDL_DISKAUDIOFILE", "audio.pcm", 1);
    /* disk driver sleeps a fixed ~46ms per buffer by default, which paces slightly
     * under real time and drains the cushion. 0 = let the app's blocking FIFO read
     * pace us instead (pipe gives ~350ms of slack). override via env if needed. */
    SDL_setenv("SDL_DISKAUDIODELAY", "0", 0);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init: %s\n", SDL_GetError()); fflush(stdout); return 1;
    }

    /* CPU mode: render into a private back buffer, then copy to the shared fb in one go,
     * so the app never reads a half-drawn frame (title/buttons are drawn last).
     * GPU mode: no pixels at all, the renderer is a stand-in and every draw call is recorded. */
    size_t fbsz = (size_t)W * H * 4;
    void *back = NULL;
    SDL_Surface *surf = NULL;
    SDL_Renderer *r = NULL;
    if (gpu) {
        if (gfx_init_gpu("gfx", W, H) != 0) { printf("gfx init failed\n"); fflush(stdout); return 1; }
        r = gfx_dummy_renderer();
    } else {
        back = calloc(1, fbsz);
        if (!back) { perror("alloc back buffer"); return 1; }
        surf = SDL_CreateRGBSurfaceWithFormatFrom(
            back, W, H, 32, W * 4, SDL_PIXELFORMAT_RGBA32);
        if (!surf) { printf("surface: %s\n", SDL_GetError()); fflush(stdout); return 1; }
        r = SDL_CreateSoftwareRenderer(surf);
        if (!r) { printf("renderer: %s\n", SDL_GetError()); fflush(stdout); return 1; }
    }

    SDL_version v; SDL_GetVersion(&v);
    printf("SDL %d.%d.%d, %dx%d, %s\n", v.major, v.minor, v.patch, W, H, gpu ? "gpu" : "cpu");
    fflush(stdout);

    if (audio_init() == 0) { jukebox_init(); jukebox_scene("third_life.ogg", 0); }

    lang_seed((unsigned)SDL_GetTicks() * 2654435761u + 12345u);
    menu_init(W, H);
    menu_set_load_enabled(story_save_exists());     /* LOAD only works with a save (made by the game at every summoning after the first) */
    nowplaying_init(W, H);
    musicwin_init(W, H);
    brainwin_init(W, H);
    pausebtn_init(W, H);
    State state = ST_MENU;
    /* dev: VESSEL_LIMBO=1 in the environment starts in limbo (desktop only), says its thought and stays there, so the screen can be looked at */
    if (getenv("VESSEL_LIMBO")) {
        world_init(W, H);                                   /* the thought card and the HUD need their init */
        thought_set_voice(&SOUL);
        space_init(W, H);
        state = ST_LIMBO;
        story_limbo_begin(1);
    }

    fcntl(0, F_SETFL, O_NONBLOCK);
    char acc[4096]; int alen = 0;
    int want_load = 0;                              /* the menu's fade-out was started by LOAD, not PLAY */
    int seen_screen = 0;                            /* screen_changes() as of the last size we applied */
    int bg = 0;                                     /* the app is in the background: nothing runs, nothing is drawn, the world is paused for when it comes back */

    /* fixed 60fps pacing: sleep only the time left in each frame budget */
    const Uint64 pf = SDL_GetPerformanceFrequency();
    const Uint64 frame_ticks = pf / 60;
    Uint64 prev = SDL_GetPerformanceCounter();
    Uint64 next_frame = prev + frame_ticks;
    Uint32 last = SDL_GetTicks(); int frames = 0;
    double worst_ms = 0, render_ms = 0;

    for (;;) {
        char buf[1024];                             /* (two fingers + the speaker stamps are more lines per frame than one) */
        ssize_t n = read(0, buf, sizeof buf);
        if (n == 0) break;                          /* app closed our stdin */
        if (n > 0) {
            if (alen + n >= (int)sizeof acc) alen = 0;
            memcpy(acc + alen, buf, n); alen += n;
            char *s = acc, *nl;
            while ((nl = memchr(s, '\n', acc + alen - s))) {
                *nl = 0;
                int a, x, y, pr;
                long long hf, hn; float hz;
                if (s[0] == 'a' && sscanf(s, "a %lld %lld", &hf, &hn) == 2) {      /* the speaker's timestamp: stream frame hf was heard at monotonic ns hn (see audio.h) */
                    audio_hw_stamp(hf, hn);
                } else if (s[0] == 'v' && sscanf(s, "v %f", &hz) == 1) {           /* the display's refresh rate in Hz: how long a frame takes to reach the glass */
                    audio_set_refresh(hz);
                } else if (s[0] == 'm' && sscanf(s, "m %d %d %d %d", &pr, &a, &x, &y) == 4) {      /* another finger: id, action, x, y (the primary one is "t") */
                    /* the open window takes the other finger first: the map pinches, the music window's piano plays a chord. the world only gets it
                     * when no window is open (a lift always gets through to the world, so no control stays held) */
                    if (mapwin_active()) mapwin_touch_p(pr + 1, a, x, y);
                    else if (musicwin_active()) musicwin_touch_p(pr + 1, a, x, y);
                    if (state == ST_WORLD && (a != 0 || (!mapwin_active() && !musicwin_active() && !brainwin_active() && !pausebtn_paused())))
                        world_touch_p(pr + 1, a, x, y);
                } else if (sscanf(s, "k %d", &a) == 1) {                    /* a key: 4 = the back button */
                    if (a == 4 && !handle_back(state)) { printf("back: unhandled\n"); fflush(stdout); }
                } else if (sscanf(s, "p %d", &a) == 1) {             /* the app went to the background (1) or came back (0) */
                    bg = a != 0;
                    if (bg && state == ST_WORLD) { pausebtn_set_paused(1); world_touch(3, 0, 0); }       /* it waits for them, paused */
                } else if (sscanf(s, "r %d %d %d", &a, &x, &y) == 3) {   /* the screen turned or was resized: landscape?, width, height */
                    screen_set(a, x, y);
                    printf("screen: %dx%d%s\n", x, y, a ? " landscape" : " portrait"); fflush(stdout);
                } else if (sscanf(s, "t %d %d %d", &a, &x, &y) == 3) {
                    if (mapwin_touch(a, x, y)) {
                        /* the map window is open: it takes every touch */
                    } else if (musicwin_touch(a, x, y)) {
                        /* the music window is open: it takes every touch */
                    } else if (brainwin_touch(a, x, y)) {
                        /* the brain window is open: it takes every touch */
                    } else if (nowplaying_touch(a, x, y)) {
                        /* the music button / card was tapped: it opens the music window */
                    } else if (thought_touch(a, x, y)) {
                        /* the brain button / card was tapped: it opens the brain window */
                    } else if ((pr = pausebtn_touch(a, x, y)) != 0) {
                        if (pr == 2) world_touch(3, 0, 0);          /* just paused: let go of the stick */
                    } else if (state == ST_MENU) {
                        MenuAction act = menu_touch(a, x, y);
                        if (act == MENU_PLAY) {
                            printf("menu: play\n"); fflush(stdout);
                            want_load = 0;
                            menu_fade_out();                    /* the title and buttons fade out, then limbo (below) */
                        }
                        if (act == MENU_LOAD) {
                            printf("menu: load\n"); fflush(stdout);
                            want_load = 1;
                            menu_fade_out();                    /* same fade; then limbo goes to the saved life instead of the opening */
                        }
                        if (act == MENU_EXIT) quit_app();
                    } else if (state == ST_WORLD) {
                        world_touch(a, x, y);
                    } else if (state == ST_LIMBO) {
                        if (dialog_active()) dialog_touch(a, x, y);          /* the soul's popup at the start: a tap on the arrow goes on */
                    }
                }
                s = nl + 1;
            }
            alen -= s - acc; memmove(acc, s, alen);
        }

        /* the screen turned: at the top of the frame, after every pending line is read, only the LAST size counts (a burst of "r" lines = one resize).
         * only when the app said it follows our size (the old app letterboxes our picture, and the CPU path never resizes) */
        if (screen_resizable() && screen_changes() != seen_screen) {
            seen_screen = screen_changes();
            int sw = screen_width(), sh = screen_height();
            if (sw > 0 && sh > 0) {
                int nw = sw / SCREEN_SCALE, nh = sh / SCREEN_SCALE;
                if (nw < SCREEN_MIN_SIDE) nw = SCREEN_MIN_SIDE;
                if (nh < SCREEN_MIN_SIDE) nh = SCREEN_MIN_SIDE;
                if (nw > SCREEN_MAX_SIDE) nw = SCREEN_MAX_SIDE;
                if (nh > SCREEN_MAX_SIDE) nh = SCREEN_MAX_SIDE;
                apply_size(&W, &H, nw, nh, state);
            }
        }

        win_track();
        if (bg) {                                   /* in the background: just keep an ear on the app */
            SDL_Delay(50);
            prev = SDL_GetPerformanceCounter(); next_frame = prev + frame_ticks;
            continue;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - prev) / (float)pf;
        prev = now;
        if (dt > 0.05f) dt = 0.05f;

        if (state == ST_MENU) {
            menu_update(dt); menu_draw(r);
            if (menu_faded()) {                                  /* gone: go on from this very frame, the stars have not stopped */
                world_init(W, H);                                /* hidden: the thought card and the HUD are initialised here */
                state = ST_LIMBO;
                thought_set_voice(&SOUL);                        /* in limbo the thoughts wear the soul's face */
                space_draw(r);                                   /* world_init made no frame of its own: keep this one stars only */
                if (!(want_load && story_load_begin()))          /* LOAD: the saved life's summoning (story.c) ... */
                    story_limbo_begin(0);                        /* ... else a beat, "Where am I?", a hold, then the world */
            }
        } else if (state == ST_LIMBO) {
            space_update(dt); space_draw(r);
            dialog_update(dt);                                   /* the soul's popup (the world is not running, so we drive the dialog here) */
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            dialog_draw(r);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            story_limbo_update(dt);                              /* the soul's own clock: limbo_run (story.c) */
            if (story_limbo_take_end()) {                        /* limbo_end(): the HUD and the world come back */
                if (!world_dawning()) jukebox_scene_pv("second_life.ogg", 1, -8.0f, 0.40f);   /* after a death: the cloud music starts with the summoning. at the start the dawn starts it (world.c) */
                state = ST_WORLD;
                world_update(0); world_draw(r);
            }
        } else {
            if (!pausebtn_paused()) world_update(dt);
            if (world_dawning()) { space_update(dt); space_draw(r); }   /* the opening: the stars shine through the brightening sky */
            world_draw(r);
            if (story_limbo_take_enter()) {                      /* a death: the world has faded to black, the soul is alone with the stars */
                state = ST_LIMBO;
                space_init(W, H);
                space_draw(r);                                   /* this frame is stars only (the black world was drawn just above) */
            }
        }
        /* the pause and brain buttons come in together, once the game really runs (not during the opening's popup and dawn). a death's limbo
         * keeps the brain button: the soul thinks there */
        if (state == ST_WORLD && pausebtn_take_menu()) {         /* the pause panel's MAIN MENU: back to the title, the stars and the menu music. PLAY starts a fresh game (world_init) */
            musicwin_close(); mapwin_close(); brainwin_close();
            printf("menu: main menu\n"); fflush(stdout);
            jukebox_scene("third_life.ogg", 0);
            menu_init(W, H);
            menu_set_load_enabled(story_save_exists());
            want_load = 0;
            state = ST_MENU;
        }
        pausebtn_set_enabled(state == ST_WORLD && !world_dawning());
        thought_set_enabled(state == ST_WORLD && !world_dawning());
        pausebtn_update(dt); pausebtn_draw_overlay(r);   /* paused: dim the world, under the buttons */
        nowplaying_set_wide(state == ST_WORLD);          /* the task panels only exist in the world: before that the card ends at the equalizer */
        nowplaying_update(dt); nowplaying_draw(r);       /* the top buttons, on top of every screen */
        thought_update(pausebtn_paused() ? 0.0f : dt);   /* the brain button, right of the pause one (frozen while paused) */
        thought_draw(r);
        pausebtn_draw(r);
        musicwin_update(dt); musicwin_draw(r);           /* and the music window on top of that */
        brainwin_update(dt); brainwin_draw(r);           /* the brain window (never open together with the music one) */
        mapwin_update(dt); mapwin_draw(r);               /* the map window, over everything (a tap on the minimap opens it) */
        SDL_RenderPresent(r);
        if (!gpu) memcpy(px, back, fbsz);

        double ft = dt * 1000.0, rt = (double)(SDL_GetPerformanceCounter() - now) * 1000.0 / (double)pf;
        if (ft > worst_ms) worst_ms = ft;
        if (rt > render_ms) render_ms = rt;
        if (++frames == 60) {
            Uint32 tk = SDL_GetTicks();
            printf("fps %.1f worst %.1fms render %.1fms rects %u\n", 60000.0 / (tk - last), worst_ms, render_ms, gfx_rect_stat());
            fflush(stdout);
            last = tk; frames = 0; worst_ms = render_ms = 0;
        }

        Uint64 now_pc = SDL_GetPerformanceCounter();
        if (now_pc < next_frame) {
            SDL_Delay((Uint32)((next_frame - now_pc) * 1000 / pf));
            next_frame += frame_ticks;
        } else if (now_pc - next_frame > frame_ticks) {
            next_frame = now_pc + frame_ticks;      /* fell behind, resync */
        } else {
            next_frame += frame_ticks;
        }
    }

    audio_quit();
    if (!gpu) {
        SDL_DestroyRenderer(r);
        SDL_FreeSurface(surf);
        free(back);
    }
    SDL_Quit();
    return 0;
}
