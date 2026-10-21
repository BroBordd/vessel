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
enum { TK_NONE, TK_POS, TK_NEG, TK_LOVE, TK_SHOCK, TK_CRY, TK_ANGRY, TK_LAUGH, TK_MASK, TK_COUNT };   /* TK_MASK (chunk 17): a censored word like f*ck, b***h (never rebuilt, the lexicon matches it) */
typedef struct { int kind, a, b, len; } LangTok;     /* original bytes [a, b); len = letters of a laugh, or how many of the same emoji in a row (1 for an emoticon) */
#define LANG_MAX_TOK 24
/* shape flags */
#define LF_SHOUT  1u    /* >= 60 % capitals over >= 4 letters */
#define LF_NOISE  2u    /* keyboard mash or gibberish: noise score >= 50 over >= 4 letters */
#define LF_LONG   4u    /* more than 120 characters (cleaned) */
#define LF_LAUGH  8u    /* a laugh token */
#define LF_EMOTE  16u   /* an emoticon or emoji token */
#define LF_STRETCH 32u  /* chunk 17: an elongated word (heeello, sooooo, pleeease) was shortened */
#define LF_LEET   64u   /* chunk 17: a leet word (h3ll0, l0ve, @wesome) was read as letters */
#define LF_SPACED 128u  /* chunk 17: spaced letters (h e l l o, h.e.l.l.o) were joined */
#define LF_MASKED 256u  /* chunk 17: a censored word (f*ck) was seen, see TK_MASK */
#define LF_PUNCT  512u  /* chunk 17: a run of !!! ??? .... was shortened */
#define LF_NUMBER 2048u    /* chunk 19: a number, an age, a time or an ordinal was captured (see LangNum) */
#define LF_SPELL  4096u    /* chunk 21: a misspelled word was repaired in the spelled variant (see lang_spell.h) */
#define LF_LEMMA  8192u     /* chunk 24: an inflected word (loved, children, went) has a lemma variant (see lang_lemma.h) */
#define LF_TEXTSPEAK 1024u  /* chunk 18: text-speak or a contraction (u, idk, dont, im, gonna) has an expanded variant */
/* chunk 19: what a number in the text is. value: NK_NUMBER the number, NK_AGE the age, NK_TIME minutes after midnight, NK_ORDINAL the position */
enum { NK_NONE, NK_NUMBER, NK_AGE, NK_TIME, NK_ORDINAL };
typedef struct { int kind, value, a, b; } LangNum;     /* original bytes [a, b) */
#define LANG_MAX_NUM 8
typedef struct {
    unsigned flags;
    int noise;                        /* 0-100: share of the letters that sit in mash / gibberish words */
    int excl, quest;                  /* number of ! and ? (cleaned text), at most 255 */
    int letters, caps;                /* letters seen and how many were capitals */
    int laugh;                        /* the longest laugh (letters), 0 = none */
    int stretch, nstretch, nleet, nspaced, npunct, nexpand;   /* chunk 17: longest elongated run, and how many words / runs each rule changed */
    int nnum; LangNum num[LANG_MAX_NUM];   /* chunk 19: set by lang_clean_numbers */
    int ntok, count[TK_COUNT];        /* tokens kept (the first LANG_MAX_TOK) and how many of each kind were seen */
    LangTok tok[LANG_MAX_TOK];
} LangInfo;

/* lower case; returns the length of out (at most cap - 1) */
int lang_clean(const char *text, char *out, int cap, int *map);
/* the same, and the tokens and the shape of the line (info may be NULL); info covers the part that was cleaned (out is cut at cap - 1; with no map and an info, cap is at most 1024) */
int lang_clean_info(const char *text, char *out, int cap, int *map, LangInfo *info);
const char *lang_token_name(int kind);
/* ---- PART 3 (chunk 17): elongation, leet, spaced letters, censor masks, repeated punctuation ----
 * lang_clean_words(in, inmap, out, cap, outmap, src, info) reads the text that lang_clean_info made (lower case, with its map) and
 * writes the text the matcher sees. out[i] came from in[src[i]] and from the original byte outmap[i] (= inmap[src[i]]; src and
 * outmap may be NULL; outmap[n] = inmap[strlen(in)]). info (may be NULL) gets the flags, counts and TK_MASK tokens added.
 *   - elongation: a run of 3+ of the same letter (heeello, sooooo, pleeease, helllo) is cut to 1 or 2: the variant that is a KNOWN
 *     word wins (hook below); with no known word a vowel run goes to 1 and a consonant run to 2. A run at the end of a word is
 *     mapped to its LAST letter so a highlight covers the whole stretch. Runs of exactly 2 are never touched (spell repair, chunk 21).
 *   - leet: 0 o, 1 i or l, 3 e, 4 a, 5 s, 7 t, @ a, $ s, ! i (only inside a word) read as letters in a word that mixes letters and
 *     them (h3ll0, l0ve, y0u, @wesome, sh!t, a$$, pl3453). A known word wins; with none, only safe shapes are read (never 3pm, 1st,
 *     b4, gr8, 2day, 10k, user1, a number, a word with a digit that is not leet). 1 is l next to another 1 (he11o) and i alone.
 *   - spaced letters: a chain of single letters joined by single spaces or one kind of . - _ , (h e l l o, h.e.l.l.o, h-e-l-l-o) is
 *     joined when it has 4+ letters, or 3 and the result is a known word (so "u r a bot" stays).
 *   - censor masks: * # % ^ ~ inside a word (f*ck, s**t, b***h) make a TK_MASK token; the text is left as it is, never rebuilt.
 *   - punctuation runs: !!!! -> !, ???? -> ?, ?!?! -> ?!, !?!? -> !?, 4+ dots -> ..., ,,, -> , (excl and quest keep the counts of part 2).
 * Plain text with none of these comes back unchanged (same text, same map). */
typedef int (*LangKnownFn)(const char *w, int n);        /* is w[0..n) a known word? */
void lang_clean_set_known(LangKnownFn fn);               /* set by lang_init (patterns, lexicon, vocabulary); NULL = no dictionary */
int  lang_clean_words(const char *in, const int *inmap, char *out, int cap, int *outmap, int *src, LangInfo *info);

/* ---- PART 4 (chunk 18): contractions and text-speak ----
 * lang_clean_expand(in, inmap, out, cap, outmap, src, info) writes the EXPANDED variant of the text the matcher sees: every token that the
 * text-speak table knows (u idk dont im gonna ...) is replaced by its standard form (you, i do not know, do not, i am, going to). It is a
 * second text, not a replacement: the engine scans both, so a pattern written either way (dont / don't / do not) matches, and the text as
 * typed is never lost. out[i] came from in[src[i]] (an expansion maps to the token it replaced: its first letters to the token's first
 * character, its last letter to the token's LAST character, so a highlight covers the whole token); outmap[i] = inmap[src[i]] (inmap, outmap,
 * src may be NULL). Returns the length. info (may be NULL) gets nexpand (tokens expanded) and LF_TEXTSPEAK; nexpand 0 means out == in.
 * A token whose expansion would not fit in cap is left as typed. Lookup is by the whole token (letters, digits, inner apostrophes). */
typedef const char *(*LangExpandFn)(const char *w, int n);   /* the standard form of w[0..n), NUL-ended, or NULL */
void lang_clean_set_expand(LangExpandFn fn);                 /* set by lang_init (the text-speak table); NULL = no expansion */
int  lang_clean_expand(const char *in, const int *inmap, char *out, int cap, int *outmap, int *src, LangInfo *info);

/* ---- PART 5 (chunk 19): numbers ----
 *   - lang_clean_numbers(orig, t, map, info) reads the text the matcher sees (t, with its map to the original text orig, both may be NULL) and
 *     fills info->num / nnum: plain numbers (7, 3.5 -> 3, -5, 10k, twenty one, twenty-one, one hundred and five), AGES (im 12, i am twelve, 12 years old,
 *     12yo, my age is 12, aged 30; not "i am 3 hours late"), TIMES (3pm, 3 pm, 3:30, 15:30, three oclock, midnight, noon; value = minutes after
 *     midnight, 3 o'clock = 180) and ORDINALS (1st, 22nd, third, "the first"). Words that only look like numbers (user1, mp3, 2day, 90s, a lone
 *     "one" as in "the one") are not. Each entry has the ORIGINAL byte range. Sets LF_NUMBER. Returns the count. The text is never changed.
 *   - lang_num_words(w, n, &v): the whole of w[0..n) as a number in words (spaces or hyphens, "and" after hundred / thousand); 0 up to 999,999,999.
 *   - lang_num_to_words(v, out, cap): the other way, "one hundred five", "minus twenty one" (0 if it does not fit). lang_num_ordinal: 21 -> "21st".
 *   - the expanded variant (part 4) also reads number-letter words: b4 gr8 l8r m8 w8 str8 2day 2morrow 2nite 4ever 4get 4u any1 no1 ..., and a lone
 *     2 or 4 in a word-like place (wait 4 me -> wait for me, want 2 go -> want to go, me 2 -> me too; never i am 4, 1 2 3, i have 2 dogs, 3pm).
 *   - lang_clean_set_numbers(0) turns the number rules off (tests: the superset rule). */
int  lang_clean_numbers(const char *orig, const char *t, const int *map, LangInfo *info);
int  lang_num_words(const char *w, int n, long *v);
int  lang_num_to_words(long v, char *out, int cap);
int  lang_num_ordinal(long v, char *out, int cap);
void lang_clean_set_numbers(int on);
/* chunk 52: the numbers table (lang_lex_numbers.c): the row of the token w[0..n) as its result text and kind (l u t y o), or NULL; lang_init plugs in
 * lang_numtab. NULL = only the built-in number-letter words */
typedef const char *(*LangNumFn)(const char *w, int n, int *kind);
void lang_clean_set_numtab(LangNumFn fn);

/* the same characters at the same places (same length, same map), but the case kept: for names (the capture of "im Alice") */
int lang_clean_case(const char *text, char *out, int cap, int *map);
/* how many bytes of the original text the character at byte index at occupies (a whole UTF-8 sequence; 1 for a broken byte) */
int lang_clean_orig_len(const char *text, int at);

#endif
