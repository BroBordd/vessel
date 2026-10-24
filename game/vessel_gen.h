/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE VESSEL GENERATOR. an endless line of vessels, one per index: vessel_gen(n) always gives the same man for the same n, on every device
 * (integer maths only, no rand(), no floats, no global state), and a different one for every other n. nothing is stored: the index IS the
 * vessel. the body is always the Vessel sprite (char.c) pixel for pixel; what the index picks is
 *   colours  hair, skin, shirt, pants, boots (a palette each, then a small per-channel jitter, so two vessels never share a colour set)
 *   looks    one of cap / beanie / headband (or none), a scarf, a shirt stripe or chest pocket, round glasses, a beard, a nose plaster
 *   name     see vessel_gen_name
 * the vessels are men (short hair); long hair is left to the hand-made people in story.c.
 * this file knows nothing about SDL drawing or the story: story.c decides who index 1, 2, 3 are (story_vessel_look). */
#ifndef VESSEL_GEN_H
#define VESSEL_GEN_H
#include <stdint.h>
#include "char.h"

#define VESSEL_NAME_MAX 24

/* a generated vessel: the look, and the storage its name points at (p.name == name). do not copy it by value: use vessel_gen_person for a
 * pointer that stays valid, or call vessel_gen again into the destination. */
typedef struct { Person p; char name[VESSEL_NAME_MAX]; } VesselLook;

/* fills *out for vessel `index` (any value, 0 .. 4294967295) */
void vessel_gen(uint32_t index, VesselLook *out);

/* the same, kept: a pointer that stays valid (and stays the same pointer for the same index) until 16 other vessels have been asked for, which
 * is what the hands and the blinking in char.c want (they tell people apart by the pointer). */
const Person *vessel_gen_person(uint32_t index);

/* the name of vessel `index`, written to buf (at most n bytes, always terminated). a name is pronounceable and never repeats: it is the index run
 * through a bijection and spelt in syllables, so no two indexes (up to 2^32) share one. the first 1024 are two syllables, the next 65536 three, ... */
void vessel_gen_name(uint32_t index, char *buf, int n);

#endif
