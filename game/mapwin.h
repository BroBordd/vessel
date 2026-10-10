/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE MAP WINDOW: opens when the player taps the minimap. a window like the music and brain ones (it takes every touch while it is open,
 * game.c routes it first and draws it last, over the top buttons): the whole map, DRAG to move it, the + and - buttons to zoom (the
 * touch stream has one finger, so no pinch), the round dot button to centre on the player, X to close. one line of help (drag) sits under the controls.
 * every mark has its name written next to it (MiniMark.label).
 * it starts zoomed out so the whole map shows. it shows the same marks as the minimap (npcs, hole, shrine, graves, the hammer).
 * the window owns no map: it asks world.c for a MapView every frame (mapwin_set_source), so it always shows what is there now. */
#ifndef MAPWIN_H
#define MAPWIN_H
#include <SDL2/SDL.h>
#include <stdint.h>
#include "char.h"
#include "minimap.h"

typedef struct {
    const uint8_t *tiles; int stride, mw, mh;       /* the tile grid, as for minimap_draw */
    const Rgb *pal; int npal;
    float ptx, pty; int facing;                     /* the player, in tiles */
    const MiniMark *marks; int nmarks;
} MapView;

void mapwin_init(int w, int h);
void mapwin_set_source(void (*fill)(MapView *v));
void mapwin_open(void);                             /* zoomed out, the whole map */
void mapwin_close(void);
int  mapwin_active(void);
int  mapwin_touch(int a, int x, int y);             /* Android MotionEvent action. 1 = used (always, while open) */
void mapwin_update(float dt);
void mapwin_draw(SDL_Renderer *r);

/* test hooks */
float mapwin_debug_zoom(void);                      /* screen px per tile */
void  mapwin_debug_center(float *tx, float *ty);
void  mapwin_debug_rects(SDL_Rect *win, SDL_Rect *view, SDL_Rect *zin, SDL_Rect *zout, SDL_Rect *you, SDL_Rect *close_btn);

#endif
