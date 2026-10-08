/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef AUDIO_H
#define AUDIO_H

/* opens the output device (S16 stereo 44100). returns 0 on success */
int  audio_init(void);
/* streams an .ogg looked up next to the executable (then cwd). returns 0 on success */
int  music_play(const char *file, int loop);
void music_stop(void);
void audio_quit(void);

#endif
