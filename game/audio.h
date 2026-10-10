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
int  music_fading(void);                   /* a fade-out is running (the track is about to be dropped) */
/* loop flag of the playing track (the jukebox decides who may change it, see jukebox.h) */
int  music_loop(void);
void music_set_loop(int on);

/* ---- where we are in the track, and moving around in it ----
 * music_duration / music_position are in seconds (0 when nothing is playing / the length is unknown).
 * music_position is what is HEARD (it subtracts the output latency); while a scrub is on it is the tape head. */
double music_duration(void);
double music_position(void);
/* same, but not clamped at 0 and the one the music window syncs to: < 0 means a new track's first sound has not
 * reached the speakers yet. MUSIC_HW_LATENCY is the delay we can not measure (Android's mixer / the hardware): if the
 * lit notes still lead the sound make it bigger, if they trail it make it smaller (seconds) */
#define MUSIC_HW_LATENCY 0.15f
double music_heard_position(void);
void   music_seek(double sec);              /* plain jump, no scrub sound (fade-in of ~30 ms) */

/* TAPE SCRUB, for dragging the progress bar. the head chases the finger like a tape deck: it plays backwards when
 * the finger goes back, faster than normal (higher pitch) when it goes forward, and stops when the finger rests.
 * going backwards needs the decoded track in memory, so the music window asks for that while it is open:
 *   music_scrub_prepare()  - start decoding the current track (and every track after it) in a background thread
 *   music_scrub_release()  - drop the decoded copy
 * music_scrub_begin returns 0 when the scrub started, -1 when it can not (no copy yet, track too long...):
 * then the caller should use music_seek when the finger lifts. */
void   music_scrub_prepare(void);
void   music_scrub_release(void);
int    music_scrub_begin(double sec);       /* finger down at `sec` */
void   music_scrub_to(double sec);          /* finger moved */
void   music_scrub_end(double sec);         /* finger up: plays on from `sec` (sec < 0: from where the head is) */
int    music_scrubbing(void);
float  music_scrub_rate(void);              /* tape speed right now: 1 = normal, negative = backwards */
float  music_scrub_ready(void);             /* 0..1: how much of the track is decoded (test hook) */

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
void sfx_thought(void);                 /* a thought arriving: soft, calm two-note chime (shares the coin's voice) */
void sfx_blip(float pitch);             /* the clean "deek" for dialog letters. pitch ~0.7..1.4 (1.0 = 256.5 Hz), up to 3.0 */
void sfx_beep(void);                    /* the heart monitor's "peep" (death cutscene) */
void sfx_flatline(void);                /* the long monitor tone, ~3 s, then it lets go (replaces a peep in progress) */
void sfx_flatline_stop(void);           /* cut the flatline short (~60 ms release); nothing if it is not sounding */
void sfx_summon(void);                  /* the summoning: a soft rising arpeggio and a shimmer, ~3.6 s, its own voice (a new call restarts it) */
int  sfx_debug_summons(void);           /* tests: how many times sfx_summon was called */
int  sfx_debug_beeps(void);             /* tests: how many times sfx_beep / sfx_flatline were called */
int  sfx_debug_flatlines(void);
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
