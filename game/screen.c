/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "screen.h"

static int land, sw, sh, changes;

void screen_set(int landscape, int w, int h) {
    if (landscape != land || w != sw || h != sh) changes++;
    land = landscape ? 1 : 0; sw = w; sh = h;
}
int screen_landscape(void) { return land; }
int screen_width(void)     { return sw; }
int screen_height(void)    { return sh; }
int screen_changes(void)   { return changes; }
