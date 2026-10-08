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
/* master music volume 0..1, applied instantly (no smoothing). persists across tracks */
void music_set_volume(float v);
void audio_quit(void);

#endif
