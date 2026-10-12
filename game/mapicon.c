/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "mapicon.h"
#include "item.h"

/* 12 x 12 pictures. K dark outline, R red roof, r dark roof, W wall (or white marble), w dark wall, B window, D door / shade, L light stone, C crack, V violet, v dark violet,
 * Y gold, y pale glow, E earth, S dull grey stone, s dark grey stone, T dull shard. '.' is empty. */
static const char *HOUSE[MAPICON_CELLS] = {
    "....KKKK....",
    "..KKRRRRKK..",
    ".KRRRRRRRRK.",
    "KRRRRrrRRRRK",
    "KKKKKKKKKKKK",
    ".KWWWWWWWWK.",
    ".KWBBWWWWwK.",
    ".KWBBWDDWwK.",
    ".KWWWWDDWwK.",
    ".KWWWWDDWwK.",
    ".KKKKKKKKKK.",
    "............",
};
/* the shrine, whole: a violet gem above a pale pillar on a step */
static const char *SHRINE[MAPICON_CELLS] = {
    ".....KK.....",
    "....KyyK....",
    "...KVyyVK...",
    "...KVVVvK...",
    "....KKKK....",
    "...KLLLLK...",
    "....KLLDK...",
    "....KLLDK...",
    "....KLLDK...",
    "..KKLLLLDKK.",
    ".KLLLLLLLLDK",
    ".KKKKKKKKKKK",
};
/* the shrine, broken: a grey jagged stump, a crack in it, dull shards and rubble, no gem and no violet */
static const char *SHRINE_BROKEN[MAPICON_CELLS] = {
    "............",
    "............",
    "............",
    "....K.K.....",
    "...KSKSK....",
    "...KSSsK....",
    "...KSCSsK...",
    "..KSSCSssK..",
    "KKSKSSSSsKSK",
    "KSSKKKKKKKSs",
    ".KKKTKKKTKK.",
    "............",
};
static const char *GRAVE[MAPICON_CELLS] = {
    "............",
    "....KKKK....",
    "...KLLLLK...",
    "..KLLLDLLK..",
    "..KLLDDDLK..",
    "..KLLLDLLK..",
    "..KLLLDLLK..",
    "..KLLLLLLK..",
    "..KLLLLLLK..",
    ".KEEEEEEEEK.",
    ".KKKKKKKKKK.",
    "............",
};
/* the goddess's temple: a gold roof, white columns, steps */
static const char *TEMPLE[MAPICON_CELLS] = {
    "....KKKK....",
    "..KKYYYYKK..",
    ".KYYYWWYYYK.",
    "KKKKKKKKKKKK",
    "..WD.WD.WD..",
    "..WD.WD.WD..",
    "..WD.WD.WD..",
    "..WD.WD.WD..",
    ".KKKKKKKKKK.",
    "KYYYYYYYYYYK",
    "KKKKKKKKKKKK",
    "............",
};
static const char *const *ICONS[MAPICON_COUNT] = { HOUSE, SHRINE, SHRINE_BROKEN, 0 /* the hammer is item.c's own icon */, GRAVE, TEMPLE };

static int pal(char c, Uint8 *r, Uint8 *g, Uint8 *b) {
    switch (c) {
        case 'K': *r = 20;  *g = 14;  *b = 24;  return 1;
        case 'R': *r = 226; *g = 104; *b = 80;  return 1;
        case 'r': *r = 168; *g = 66;  *b = 52;  return 1;
        case 'W': *r = 244; *g = 232; *b = 200; return 1;
        case 'w': *r = 196; *g = 180; *b = 148; return 1;
        case 'B': *r = 120; *g = 178; *b = 236; return 1;
        case 'D': *r = 120; *g = 92;  *b = 62;  return 1;
        case 'L': *r = 206; *g = 210; *b = 222; return 1;
        case 'C': *r = 40;  *g = 40;  *b = 56;  return 1;
        case 'V': *r = 176; *g = 120; *b = 255; return 1;
        case 'v': *r = 120; *g = 76;  *b = 200; return 1;
        case 'Y': *r = 238; *g = 196; *b = 84;  return 1;
        case 'y': *r = 244; *g = 232; *b = 255; return 1;
        case 'E': *r = 118; *g = 82;  *b = 52;  return 1;
        case 'S': *r = 148; *g = 154; *b = 180; return 1;
        case 's': *r = 102; *g = 108; *b = 134; return 1;
        case 'T': *r = 100; *g = 100; *b = 124; return 1;
        default: return 0;
    }
}

void mapicon_draw(SDL_Renderer *r, int kind, int cx, int cy, int cell) {
    if (kind < 0 || kind >= MAPICON_COUNT || cell < 1) return;
    int x0 = cx - MAPICON_CELLS * cell / 2, y0 = cy - MAPICON_CELLS * cell / 2;
    if (kind == MAPICON_HAMMER) { item_draw_icon(r, ITEM_HAMMER, x0, y0, cell); return; }
    /* the pillar's shade (D on the shrine and the temple) is a cooler tone than the door's brown */
    int cool = kind == MAPICON_SHRINE || kind == MAPICON_TEMPLE || kind == MAPICON_GRAVE;
    for (int j = 0; j < MAPICON_CELLS; j++)
        for (int i = 0; i < MAPICON_CELLS; i++) {
            Uint8 R, G, B; char c = ICONS[kind][j][i];
            if (!pal(c, &R, &G, &B)) continue;
            if (c == 'D' && cool) { R = 150; G = 154; B = 176; }
            SDL_SetRenderDrawColor(r, R, G, B, 255);
            SDL_Rect q = { x0 + i * cell, y0 + j * cell, cell, cell }; SDL_RenderFillRect(r, &q);
        }
}
