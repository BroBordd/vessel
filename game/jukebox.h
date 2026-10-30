/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE JUKEBOX: decides what music plays, so the rest of the game never has to care.
 *
 * Two modes, picked by the player in the music window:
 *   MAP MUSIC     (default) every screen / map asks for its own track (jukebox_scene) and gets it: the menu
 *                 has its song, the clouds have theirs, the ground has another.
 *   CUSTOM MUSIC  the player picked a track from the .ogg files next to the executable. it plays (looping)
 *                 on every screen and every map and nothing the game asks for changes it.
 *
 * the game keeps calling jukebox_scene() / jukebox_scene_fade() as before. in custom mode those calls are only
 * remembered, so switching back to map music lands on the right song for wherever the player is by then.
 * (music_play / music_fade_out in audio.h are the raw layer underneath: only the jukebox should call them.) */
#ifndef JUKEBOX_H
#define JUKEBOX_H

typedef enum { JB_MAP = 0, JB_CUSTOM = 1 } JbMode;

void   jukebox_init(void);                       /* after audio_init(): scans the folder */

/* the current screen / map wants this track. plays it right away in map mode, just remembers it in custom mode */
void   jukebox_scene(const char *file, int loop);
/* the current screen's music should fade away (loading screen, falling to the ground). only touches the sound in
 * map mode, and always forgets the scene's track */
void   jukebox_scene_fade(float seconds);
/* the same, with the map's pitch in semitones (negative = slower and darker). jukebox_scene() is pitch 0. every map has its own song AND its own pitch */
void   jukebox_scene_p(const char *file, int loop, float semitones);
/* ... and its own volume (0..1; the others use MUSIC_DEFAULT_VOLUME) */
void   jukebox_scene_pv(const char *file, int loop, float semitones, float volume);

/* the pitch slider of the music window. map mode: the map decides (the slider shows it, locked). custom mode: the player's, -20..20 in half steps, 0 default */
float  jukebox_pitch(void);                      /* what is applied right now */
int    jukebox_pitch_locked(void);               /* 1 = map mode */
void   jukebox_set_pitch(float semitones);       /* custom mode only, ignored otherwise */
/* the volume slider: the same rule. map mode: the map's volume, locked. custom mode: the player's */
float  jukebox_volume(void);
int    jukebox_volume_locked(void);
void   jukebox_set_volume(float v);              /* custom mode only, ignored otherwise */

JbMode jukebox_mode(void);
void   jukebox_set_mode(JbMode m);               /* custom: keeps what is playing if it is a known track, else the first one */

/* the browsable list: every .ogg next to the executable */
void        jukebox_rescan(void);
int         jukebox_count(void);
const char *jukebox_file(int i);
int         jukebox_custom_index(void);          /* index of the chosen custom track, -1 if none / not in the list */
int         jukebox_playing_index(void);         /* index of what is audible right now, -1 if nothing / not in the list */
void        jukebox_pick(int i);                 /* play track i as the custom track (switches to custom mode) */

/* looping. map mode: the map decides (what it passed to jukebox_scene), the player can not change it. custom mode:
 * the player's checkbox, on by default */
int         jukebox_loop(void);                  /* 1 = the music loops, as it is right now */
int         jukebox_loop_locked(void);           /* 1 = map mode, the checkbox is greyed out */
void        jukebox_set_loop(int on);            /* custom mode only, ignored otherwise */

#endif
