/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * VOICE TRANSFORMS (docs/convo_plan.md, chunk 11). No SDL.
 *
 * voice_apply rewrites a FINISHED line (slots already filled) into one of the seven new voices by code, so a voice never lacks lines:
 *   CHILD   short words, long clauses dropped, a blunt tag
 *   ELDER   an old person's opener and closer
 *   FORMAL  contractions expanded, I shall, a stiff opener
 *   BEAST   fragments, function words dropped, a doubled word, a growl
 *   MACHINE labels, upper case, exclamations flattened
 *   GHOST   one broken sentence, trails off with " -"
 *   POET    an image opener, one rhythm cut
 * voice_flavour is the light version (only the openers, closers and tags, no rewriting) used on lines that were handwritten for the voice.
 *
 * rules every output keeps: only the font's characters (letters, digits, space and , . ! ? ' -), never empty, at most 150 characters
 * (a line that is already longer than that, or that has no change to make, comes back unchanged and the call returns 0).
 * deterministic: the same (seed, voice, mood, text) always gives the same output, whatever was called before. */
#ifndef LANG_VOICE_H
#define LANG_VOICE_H

#define VOICE_MAX_LEN 150

void voice_seed(unsigned s);                                   /* the seed of the transforms (lang_seed sets it too) */
int  voice_first(int style);                                   /* the first voice (lowest bit) of a style mask, PLAIN if 0 */
int  voice_is_new(int style);                                  /* 1 if the first voice is one of the seven new voices (elder .. poet) */
const char *voice_name(int style);                             /* "plain" ... "poet" for the first voice */
/* both fill out (always a valid line, a copy of text when nothing changed) and return 1 if it differs from text */
int  voice_apply(int style, int tone, const char *text, char *out, int cap);      /* the full rewrite */
int  voice_flavour(int style, int tone, const char *text, char *out, int cap);    /* only the openers, closers and tags */
/* tests: voice_enable(0) turns OFF the wiring in lang.c (a persona with a new voice is then answered with the raw lines, as in chunk 10); voice_apply / voice_flavour
 * called directly always work. on by default */
void voice_enable(int on);
int  voice_enabled(void);
unsigned voice_applied(void);                                  /* how many times a transform changed a line since the last voice_seed (tests) */

#endif
