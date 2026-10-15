/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE CLEAN-UP LAYER, part 1: bytes and characters (docs/convo_plan.md, chunk 15). No SDL.
 *
 *   raw typed text (UTF-8, anything) --lang_clean--> plain lower case ASCII + a map back to the original bytes
 *
 * What it does: UTF-8 is decoded and folded to ASCII (accents: e-acute -> e, sharp s -> ss, n-tilde -> n, ae ligature -> ae),
 * curly quotes and apostrophes -> ' and ", dashes -> -, the ellipsis character -> ..., full-width letters -> ASCII,
 * zero-width characters, soft hyphens, bidi marks, variation selectors and control characters are removed, no-break and
 * other unicode spaces, tabs and newlines become a space, runs of spaces collapse to one, and leading / trailing spaces go.
 * Punctuation is kept as typed ("..." and "?!" stay). Any other non-ASCII character (emoji, kaomoji, other scripts) becomes
 * ONE space for now (the same word break the raw bytes gave before); chunk 16 turns the emoji into sentiment tokens there.
 * Broken UTF-8 bytes are dropped. For plain ASCII text without control characters, repeated spaces or edge spaces the
 * output is exactly the old tolower() of the text (langcheck --clean proves it over every line, pattern and corpus file).
 *
 * map[i] = byte index in the ORIGINAL text of the character that produced out[i]; map[len] = strlen(text). So lang_highlight
 * can mark the original bytes even after folding. map may be NULL; if not, it holds cap entries (out needs cap bytes).
 */
#ifndef LANG_CLEAN_H
#define LANG_CLEAN_H

/* lower case; returns the length of out (at most cap - 1) */
int lang_clean(const char *text, char *out, int cap, int *map);
/* the same characters at the same places (same length, same map), but the case kept: for names (the capture of "im Alice") */
int lang_clean_case(const char *text, char *out, int cap, int *map);
/* how many bytes of the original text the character at byte index at occupies (a whole UTF-8 sequence; 1 for a broken byte) */
int lang_clean_orig_len(const char *text, int at);

#endif
