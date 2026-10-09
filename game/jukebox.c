/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "jukebox.h"
#include "audio.h"
#include <stdio.h>
#include <string.h>

static JbMode mode = JB_MAP;
static char scene_file[128];          /* what the current screen / map wants ("" = nothing) */
static int  scene_loop;
static char custom_file[128];         /* what the player picked ("" = nothing yet) */
static int  count;
static int  custom_loop = 1;          /* the player's loop checkbox (custom mode) */

static int index_of(const char *file) {
    if (!file[0]) return -1;
    for (int i = 0; i < count; i++) if (strcmp(music_scan_file(i), file) == 0) return i;
    return -1;
}

static void play_custom(void) {
    if (!custom_file[0]) return;
    if (music_play(custom_file, custom_loop) == 0) music_pause(0);      /* the player chose this: they want to hear it */
}

void jukebox_init(void) { count = music_scan(); }
void jukebox_rescan(void) { count = music_scan(); }
int  jukebox_count(void) { return count; }
const char *jukebox_file(int i) { return music_scan_file(i); }
JbMode jukebox_mode(void) { return mode; }
int  jukebox_custom_index(void) { return index_of(custom_file); }
int  jukebox_playing_index(void) { return index_of(music_current_file()); }

void jukebox_scene(const char *file, int loop) {
    snprintf(scene_file, sizeof scene_file, "%s", file);
    scene_loop = loop;
    if (mode == JB_MAP) music_play(scene_file, scene_loop);
}

void jukebox_scene_fade(float seconds) {
    scene_file[0] = 0;                                         /* that screen's song is over, in either mode */
    if (mode == JB_MAP) music_fade_out(seconds);
}

void jukebox_set_mode(JbMode m) {
    if (m == mode) return;
    mode = m;
    if (m == JB_CUSTOM) {
        const char *now = music_current_file();
        if (!custom_file[0] && index_of(now) >= 0) {           /* nothing picked yet: keep the song that is on, now locked in */
            snprintf(custom_file, sizeof custom_file, "%s", now);
            music_set_loop(custom_loop);                       /* it keeps playing, but the loop flag is the player's now */
            return;
        }
        if (!custom_file[0] && count > 0) snprintf(custom_file, sizeof custom_file, "%s", music_scan_file(0));
        if (strcmp(now, custom_file) != 0) play_custom();
        else music_set_loop(custom_loop);                      /* same song already playing: leave it running */
    } else {
        if (scene_file[0]) {
            if (strcmp(music_current_file(), scene_file) != 0) { if (music_play(scene_file, scene_loop) == 0) music_pause(0); }
            else music_set_loop(scene_loop);                   /* the map's song is already on: give the loop flag back to the map */
        }
        else music_stop();                                     /* this screen has no music of its own */
    }
}

void jukebox_pick(int i) {
    if (i < 0 || i >= count) return;
    snprintf(custom_file, sizeof custom_file, "%s", music_scan_file(i));
    mode = JB_CUSTOM;
    if (strcmp(music_current_file(), custom_file) == 0 && !music_fading()) {      /* that song is already on: do not restart it */
        music_set_loop(custom_loop);
        music_pause(0);                                                           /* (a paused one just resumes where it was) */
        return;
    }
    play_custom();
}

int jukebox_loop(void) {
    if (mode == JB_MAP) return scene_file[0] ? scene_loop : music_loop();
    return custom_loop;
}
int jukebox_loop_locked(void) { return mode == JB_MAP; }
void jukebox_set_loop(int on) {
    if (mode != JB_CUSTOM) return;
    custom_loop = on ? 1 : 0;
    music_set_loop(custom_loop);
}
