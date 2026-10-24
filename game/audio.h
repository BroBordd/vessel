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
void  music_set_pitch(float semitones);   /* -12..12, 0 = as recorded. speed and pitch together. only the jukebox sets it */
float music_pitch(void);
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
 *
 * MUSIC <-> SCREEN SYNC (game/sync.h has the whole story). everything the music window shows is looked up for the moment its
 * picture reaches the player's eye: now + the display delay, mapped through the speaker's clock onto what was in the audio then.
 * the speaker's clock is the AudioTrack's own timestamps, which the app sends (audio_hw_stamp); an app that does not falls back
 * to an estimate (the measured pipeline depth + MUSIC_HW_LATENCY, the part nobody can measure from here). */
#define MUSIC_HW_LATENCY 0.15f         /* only the estimate uses it: if the lit notes lead the sound make it bigger, if they trail it make it smaller */
double music_duration(void);
double music_position(void);           /* what the player hears, for the progress bar (>= 0) */
/* same, but not clamped at 0 and the one the music window syncs to: < 0 means a new track's first sound has not
 * reached the speakers yet */
double music_heard_position(void);
int    music_heard_playing(void);      /* the sound at the speaker is moving through the track (not paused, not a tape at rest) */
/* the app's speaker timestamp: stream frame `frame` (counted from the first frame we ever produced) was at the speaker at
 * CLOCK_MONOTONIC time `nano` (ns, the same clock as Java's System.nanoTime) */
void   audio_hw_stamp(long long frame, long long nano);
/* the app's display refresh rate in Hz (a frame we draw is on the glass ~2.5 refreshes later) */
void   audio_set_refresh(float hz);
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
void sfx_found(void);                   /* chunk 58: "You found ...": a bright two-note bell, rising (not the coin) */
int  sfx_debug_coins(void);             /* tests: how many times sfx_coin was called */
int  sfx_debug_found(void);             /* tests: how many times sfx_found was called */
void sfx_thought(void);                 /* a thought arriving: soft, calm two-note chime (shares the coin's voice) */
void sfx_blip(float pitch);             /* the clean "deek" for dialog letters. pitch ~0.7..1.4 (1.0 = 256.5 Hz), up to 3.0 */
void sfx_beep(void);                    /* the heart monitor's "peep" (death cutscene) */
void sfx_flatline(void);                /* the long monitor tone, ~3 s, then it lets go (replaces a peep in progress) */
void sfx_flatline_stop(void);           /* cut the flatline short (~60 ms release); nothing if it is not sounding */
void sfx_summon(void);                  /* the summoning: a soft rising arpeggio and a shimmer, ~3.6 s, its own voice (a new call restarts it) */
int  sfx_debug_summons(void);           /* tests: how many times sfx_summon was called */
int  sfx_debug_beeps(void);             /* tests: how many times sfx_beep / sfx_flatline were called */
int  sfx_debug_flatlines(void);
void sfx_error(void);                   /* chunk 33: a short low double buzz ("not allowed"), its own voice */
int  sfx_debug_errors(void);
void sfx_pfft(void);                    /* chunk 38.1: a watery pfft: a short burst of low-passed noise, the pitch falling, a little gurgle on it */
int  sfx_debug_pfft(void);              /* tests: how many times sfx_pfft was called */
void sfx_hit(void);                     /* chunk 36: a hammer hit: a dull low thump with a short crack on top */
void sfx_drag(float level);             /* a body dragged over the stone: a low rough growl, "grrrr". call it every frame with how hard we pull 0..1 (0 = quiet; it fades in and out by itself) */
void sfx_hold_hint(void);               /* chunk 55: a tap on a hammer button (a hold is needed): a short soft rising "bup" */
int  sfx_debug_hints(void);             /* tests: how many times sfx_hold_hint was called */
int  sfx_debug_hits(void);              /* tests: how many times sfx_hit was called */            /* tests: how many times sfx_error was called */
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
