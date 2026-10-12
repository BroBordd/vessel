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
 * (full body only; the face popups are too tight for it). ACC_WORRIED = the worried eyes (chunk 52), always on for whoever wears it */
enum { ACC_SHADES = 1, ACC_HALO = 2, ACC_WORRIED = 4 };

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

/* lying face-down on the ground after the fall (seen from the side, head to the left, eyes shut). feet at (x, y). */
void char_draw_lying(SDL_Renderer *r, const Person *p, int x, int y, int s);

/* mixes everything drawn from now on with white: 0 = normal .. 1 = white. the spawn flash. remember to set it back to 0. */
void char_set_lighten(float k);

/* chunk 38: Alex's body without its head (a dark red cut across the neck): everything drawn while it is on has no head, no face, no halo.
 * char_draw_head: just the head (front view, eyes shut, red at the neck), centred on (x, y), 10 x 8 sprite pixels. */
void char_set_headless(int on);
void char_draw_head(SDL_Renderer *r, const Person *p, int x, int y, int s);

/* chunk 52: the worried face: both eyes half shut (a flat lid of skin over the upper half of each eye, the pupil and the glint stay in
 * the lower half, the blink still closes them). everything drawn with eyes while it is on is worried (every view: the body, the portrait,
 * the face popups, the brain card face); a Person wears it permanently with ACC_WORRIED. */
void char_set_worried(int on);

/* chunk 56: the aiming pose ("Die" at the mended shrine, while the finger holds the hammer button): full body, front view, feet centred on (x, y), with a
 * ground shadow. the face is turned up to the sky: the left eye is shut (a squint), the right eye is open with its pupil at the top; the right arm is up over
 * the head holding the hammer, which wobbles a little (t = seconds, drives the wobble). char_debug_aims() counts the draws (tests). */
void char_draw_aim(SDL_Renderer *r, const Person *p, int x, int y, int s, float t);
int  char_debug_aims(void);
void char_set_aim_hammer(int on);   /* chunk 57: 0 = the pose with an empty raised hand (the hammer was thrown), 1 = with the hammer (the default) */

/* draws head + shoulders, front view, top-left at (x, y). size is 10*s square.
 * talking = 1 flaps the mouth. (the eyes blink, each person on their own clock: see eyes_shut in char.c) */
void char_draw_portrait(SDL_Renderer *r, const Person *p, int x, int y, int s, int talking);

#endif
