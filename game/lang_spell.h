/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * SPELL REPAIR (docs/convo_plan.md, chunk 21). No SDL.
 *
 * A token the engine does not know (not in the vocabulary, not a name, not in a pattern or a lexicon row, not text-speak, not a number or a
 * word with odd characters) is looked up for the word the player MEANT. The result is a SECOND text, the spelled variant (like the expanded
 * variant of chunk 18): the engine scans it too, so "hwllo there" is heard as "hello there", while the text as typed is never lost and the
 * highlight still marks the ORIGINAL characters.
 *
 *   dictionary   every word of the vocabulary, the patterns and the lexicon (3-24 letters, letters only). rank = the vocabulary rank; a word that
 *                is only in a pattern or lexicon row gets a rank after the vocabulary. "value" = it is in a pattern or a lexicon row.
 *   index        the symmetric delete trick: for every dictionary word the word itself and all its 1-deletes (and, for words of 6+ letters, its
 *                2-deletes) are hashed into one sorted array; a token meets its candidates by its own deletes (1 for 3-7 letters, 2 for 8+),
 *                so the cost does not depend on the dictionary size. Built at first use, again when the tables changed (lang_tables_gen).
 *   cost         a weighted Damerau-Levenshtein distance (lower = more likely): a transposition 0.55, a keyboard neighbour 0.70, a vowel for a vowel
 *                0.85, any other substitution 1.0, a missing or extra letter 1.0 (0.7 when it is a doubled letter), plus 0.6 when the first letter
 *                differs (0.2 for a swap of the first two). Unit distance <= 1 for 3-7 letters, <= 2 for 8+ letters.
 *   score        cost + 0.06 * log2(rank): a frequent word wins a tie.
 *   confidence   0-100 from the kind of edit, the length (a short word is unsafe), the first letter, the margin to the second best candidate and
 *                the value flag. A fix is taken at confidence >= SPELL_MIN_CONF; a candidate with no value must be distance 1 and rank < 3000.
 *   never repaired: a word that is protected (vocabulary, name, pattern, lexicon, text-speak), 1-2 letters, over 24 letters, anything with a digit or
 *                an odd character, an ALL CAPS word of up to 4 letters (an acronym), a Capitalised word that is not the first of its sentence (a name),
 *                and the words that part 3 of the clean-up layer already changed are read as they came out of it.
 */
#ifndef LANG_SPELL_H
#define LANG_SPELL_H

#define SPELL_MIN_CONF  55          /* a correction below this is dropped */
#define SPELL_MAX_FIX   12          /* corrections reported per text */
#define SPELL_MIN_LEN   3
#define SPELL_MAX_LEN   24

typedef struct {
    int a, b;                       /* original bytes [a, b) of the token as typed */
    char from[SPELL_MAX_LEN + 1], to[SPELL_MAX_LEN + 1];
    int conf;                       /* 0-100 */
    int dist;                       /* unit edit distance 1 or 2 */
} SpellFix;
typedef struct { int nfix; int min_conf; SpellFix fix[SPELL_MAX_FIX]; } SpellInfo;

/* the best word for the token w[0..n) (lower case letters), or 0. Writes it to out (SPELL_MAX_LEN + 1 bytes), its confidence to *conf and the
 * unit distance to *dist (both may be NULL). The token is not checked for being protected here: lang_spell_text does that. */
int  lang_spell_word(const char *w, int n, char *out, int *conf, int *dist);
/* the question lang_spell_text asks first: a word that is never repaired because the engine knows it */
int  lang_spell_protected(const char *w, int n);

/* writes the SPELLED variant of in (the text the matcher sees; orig = the text as typed, inmap = in -> orig bytes, both may be NULL; shout = 1 when
 * the whole line is capitals, which turns the capital-letter name rule off). out[i] came from in[src[i]] and from the original byte outmap[i] =
 * inmap[src[i]] (a correction maps its first letters to the token's first characters and its last letter to the token's LAST character, so a
 * highlight covers the whole token); src, outmap and si may be NULL. Returns the length of out; nfix 0 means out == in. A correction that would not
 * fit in cap is left as typed. */
int  lang_spell_text(const char *orig, const char *in, const int *inmap, int shout, char *out, int cap, int *outmap, int *src, SpellInfo *si);

/* tools */
int  lang_spell_dict_size(void);
int  lang_spell_index_size(void);
double lang_spell_build_ms(void);      /* forces a rebuild, returns the milliseconds */
void lang_spell_set(int on);           /* 0 = the repair is off (tests: the superset rule); 1 = on (default) */

#endif
