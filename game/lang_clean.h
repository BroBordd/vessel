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
 * PART 2 (chunk 16): the same pass also SEES what the text only carries as feeling, without changing a letter of the cleaned text
 * (so every old pattern, like "lol" and "haha*", keeps matching): emoticons and kaomoji (:) :( :D xD <3 o_o T_T ^_^ :/ >:( ...),
 * common emoji (smileys, hearts, skull, fire, eyes, crying, pray, thumbs; a skin tone or a variation selector belongs to its emoji),
 * and laughter (haha, hehe, lol, lmao, rofl, kkkk, jajaja, xd, wkwk ...) become TOKENS in a list (LangInfo.tok: a kind TK_*, the
 * ORIGINAL byte range, and a length: letters of a laugh, how many times an emoji or a mouth was repeated). A run of the same emoji
 * is one token. The lexicon (chunk 27/28) reads them as <pos> <neg> <love> <shock> <cry> <angry> <laugh>. It also measures the
 * shape of the line: SHOUT (>= 60 % capitals over >= 4 letters), NOISE (keyboard mash or gibberish, with a 0-100 score), LONG
 * (over 120 characters), the number of ! and ? marks. Tokens are never put into the text itself: nothing the player types can
 * spoof one, and a text without any of them cleans exactly as in chunk 15.
 *
 * map[i] = byte index in the ORIGINAL text of the character that produced out[i]; map[len] = strlen(text). So lang_highlight
 * can mark the original bytes even after folding. map may be NULL; if not, it holds cap entries (out needs cap bytes).
 */
#ifndef LANG_CLEAN_H
#define LANG_CLEAN_H

/* sentiment token kinds (chunk 16); the printed names are lang_token_name: pos neg love shock cry angry laugh */
enum { TK_NONE, TK_POS, TK_NEG, TK_LOVE, TK_SHOCK, TK_CRY, TK_ANGRY, TK_LAUGH, TK_COUNT };
typedef struct { int kind, a, b, len; } LangTok;     /* original bytes [a, b); len = letters of a laugh, or how many of the same emoji in a row (1 for an emoticon) */
#define LANG_MAX_TOK 24
/* shape flags */
#define LF_SHOUT  1u    /* >= 60 % capitals over >= 4 letters */
#define LF_NOISE  2u    /* keyboard mash or gibberish: noise score >= 50 over >= 4 letters */
#define LF_LONG   4u    /* more than 120 characters (cleaned) */
#define LF_LAUGH  8u    /* a laugh token */
#define LF_EMOTE  16u   /* an emoticon or emoji token */
typedef struct {
    unsigned flags;
    int noise;                        /* 0-100: share of the letters that sit in mash / gibberish words */
    int excl, quest;                  /* number of ! and ? (cleaned text), at most 255 */
    int letters, caps;                /* letters seen and how many were capitals */
    int laugh;                        /* the longest laugh (letters), 0 = none */
    int ntok, count[TK_COUNT];        /* tokens kept (the first LANG_MAX_TOK) and how many of each kind were seen */
    LangTok tok[LANG_MAX_TOK];
} LangInfo;

/* lower case; returns the length of out (at most cap - 1) */
int lang_clean(const char *text, char *out, int cap, int *map);
/* the same, and the tokens and the shape of the line (info may be NULL); info covers the part that was cleaned (out is cut at cap - 1; with no map and an info, cap is at most 1024) */
int lang_clean_info(const char *text, char *out, int cap, int *map, LangInfo *info);
const char *lang_token_name(int kind);
/* the same characters at the same places (same length, same map), but the case kept: for names (the capture of "im Alice") */
int lang_clean_case(const char *text, char *out, int cap, int *map);
/* how many bytes of the original text the character at byte index at occupies (a whole UTF-8 sequence; 1 for a broken byte) */
int lang_clean_orig_len(const char *text, int at);

#endif
