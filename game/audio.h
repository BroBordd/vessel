/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef AUDIO_H
#define AUDIO_H

/* opens the output device (S16 stereo 44100). returns 0 on success */
int  audio_init(void);
/* streams an .ogg looked up next to the executable (then cwd). returns 0 on success */
int  music_play(const char *file, int loop);
void music_stop(void);
/* ramps the current music to silence over `seconds` (smooth curve), then drops it */
void music_fade_out(float seconds);
/* master music volume 0..1, applied instantly (no smoothing). persists across tracks.
 * starts at MUSIC_DEFAULT_VOLUME (65%); nothing in the game changes it behind the player's back */
#define MUSIC_DEFAULT_VOLUME 0.65f
void  music_set_volume(float v);
float music_volume(void);
/* pause / resume the music (the stream keeps its place; a ~30 ms ramp avoids a click). stays paused across
 * track changes until resumed, so a new map's music does not blast over someone who paused it */
void music_pause(int paused);
int  music_paused(void);
/* file name of what is playing right now (\"third_life.ogg\"), \"\" when nothing is */
const char *music_current_file(void);
/* \"divine_tale.ogg\" -> \"Divine Tale\" */
void music_title_of(const char *file, char *out, int cap);
/* every .ogg next to the executable (cwd if there are none there), sorted by name. rescans each call */
#define MUSIC_MAX_TRACKS 64
int  music_scan(void);                     /* returns how many were found */
const char *music_scan_file(int i);        /* file name of track i (valid until the next scan) */
void audio_quit(void);

/* sound effects, synthesised (no files). they play on top of the music and ignore music volume */
void sfx_coin(void);                    /* the little coin ding */
void sfx_blip(float pitch);             /* the clean "deek" for dialog letters. pitch ~0.7..1.4 (1.0 = 256.5 Hz) */
/* piano notes for the music window: the note rings while the key is held (a short release after).
 * midi 60 = middle C. several can sound at once. not part of the music analysis. */
void sfx_note_on(int midi);
void sfx_note_off(int midi);
float audio_latency(void);              /* seconds between mixing a sound and hearing it (measured; 0 until known) */

/* live analysis of what is actually being played (the real decoded stream, not a fake animation).
 * MUSIC_BANDS log-spaced frequency bands, low to high, each 0..1. all zero when silent. */
#define MUSIC_BANDS 24
void music_spectrum(float *bands);
/* name of the current track ("Third Life") and a counter that goes up every time music_play succeeds */
const char *music_title(void);
unsigned    music_serial(void);

#endif
