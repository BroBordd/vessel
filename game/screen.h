/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * WHAT THE APP TELLS US ABOUT THE SCREEN. the app sends the real screen size (the "r" message) on start, rotation, split screen and window
 * resizes. when the app has announced that it follows the game's size (gfx.h, header [9]), the picture follows the screen: the game takes
 * W = screen width / SCALE, H = screen height / SCALE and the app draws it 1:1, no bars. only in the FALLBACK (old app, the CPU path, the
 * frames before the new size arrives) does the old rule hold: the game keeps the size it started with and the app letterboxes that picture
 * on the turned screen, so a rotation changes nothing in our layout.
 *
 * ROTATION WORK (docs/rotation_plan.md): TESTS WASTE TIME HERE. do not write or run test binaries for it, just write the code and push; the owner
 * pulls and tries it himself on his phone, as the user. a compile check is the most a chunk does on its own. */
#ifndef SCREEN_H
#define SCREEN_H

void screen_set(int landscape, int w, int h);     /* the app's "r" message: the real screen size in pixels, now */
int  screen_landscape(void);                      /* 1 when the real screen is wider than tall */
int  screen_width(void);
int  screen_height(void);
int  screen_changes(void);                        /* how many times it has changed since the start: poll it to notice a rotation */

#endif
