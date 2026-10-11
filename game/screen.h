/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * WHAT THE APP TELLS US ABOUT THE SCREEN. the game keeps the pixel size it started with (everything is laid out for it once), and the app
 * letterboxes that picture on whatever the screen turns into (rotation, split screen, a resized window). so a rotation never changes our
 * layout - but the app does tell us, and this is where a screen that wants to react to it (a landscape layout, say) can ask. */
#ifndef SCREEN_H
#define SCREEN_H

void screen_set(int landscape, int w, int h);     /* the app's "r" message: the real screen size in pixels, now */
int  screen_landscape(void);                      /* 1 when the real screen is wider than tall */
int  screen_width(void);
int  screen_height(void);
int  screen_changes(void);                        /* how many times it has changed since the start: poll it to notice a rotation */

#endif
