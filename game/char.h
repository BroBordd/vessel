/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#ifndef CHAR_H
#define CHAR_H
#include <SDL2/SDL.h>

typedef struct { Uint8 r, g, b; } Rgb;

/* a character's look. define these in story.c, then reuse them anywhere:
 * as the player, as an NPC, or as the face in a dialog box. */
typedef struct {
    const char *name;
    Rgb hair, skin, shirt, pants, boots;
    int long_hair;                 /* 1 = hair falls past the shoulders */
    int acc;                       /* accessories: ACC_* flags OR-ed together (0 if omitted) */
} Person;

/* accessories. shades show on every view that has eyes, the halo floats above the head
 * (full body only; the face popups are too tight for it) */
enum { ACC_SHADES = 1, ACC_HALO = 2 };

/* facing values */
enum { FACE_DOWN = 0, FACE_UP = 1, FACE_LEFT = 2, FACE_RIGHT = 3 };

/* draws the full body with its feet centred on (x, y). s = screen px per sprite pixel.
 * moving/walk drive the step animation (walk is a phase that grows while walking). */
void char_draw(SDL_Renderer *r, const Person *p, int x, int y,
               int facing, int moving, float walk, int s);

/* same as char_draw but without the ground shadow (for falling through the air) */
void char_draw_air(SDL_Renderer *r, const Person *p, int x, int y,
                   int facing, int moving, float walk, int s);

/* lying face-down on the ground, head to the left, feet centred on (x, y).
 * lift 0..1 pushes the upper body up off the floor (used while getting up). */
/* falling over backwards, feet pivot: a = 0 standing .. pi/2 (1.5708) lying on its back, head to the right (the death) */
void char_draw_fall(SDL_Renderer *r, const Person *p, int x, int y, int s, float a);

void char_draw_prone(SDL_Renderer *r, const Person *p, int x, int y, int s, float lift);

/* draws head + shoulders, front view, top-left at (x, y). size is 10*s square.
 * talking = 1 flaps the mouth. */
void char_draw_portrait(SDL_Renderer *r, const Person *p, int x, int y, int s, int talking);

#endif
