/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * The clean-up layer, part 1 (docs/convo_plan.md, chunk 15). See lang_clean.h for what it does and promises.
 */
#include "lang_clean.h"
#include <string.h>

/* folded spellings of U+00C0..U+00FF and U+0100..U+017F (lower case; " " = a space). made from the Unicode decompositions plus the
 * letters that do not decompose (ae, d-bar, o-slash, thorn, sharp s, l-stroke, oe, ij ...) */
static const char *const L1[] = {
    "a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
    "d", "n", "o", "o", "o", "o", "o", "x", "o", "u", "u", "u", "u", "y", "th", "ss",
    "a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
    "d", "n", "o", "o", "o", "o", "o", " ", "o", "u", "u", "u", "u", "y", "th", "y",
};
static const char *const LA[] = {
    "a", "a", "a", "a", "a", "a", "c", "c", "c", "c", "c", "c", "c", "c", "d", "d",
    "d", "d", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "g", "g", "g", "g",
    "g", "g", "g", "g", "h", "h", "h", "h", "i", "i", "i", "i", "i", "i", "i", "i",
    "i", "i", "ij", "ij", "j", "j", "k", "k", "k", "l", "l", "l", "l", "l", "l", "l",
    "l", "l", "l", "n", "n", "n", "n", "n", "n", "n", "n", "n", "o", "o", "o", "o",
    "o", "o", "oe", "oe", "r", "r", "r", "r", "r", "r", "s", "s", "s", "s", "s", "s",
    "s", "s", "t", "t", "t", "t", "t", "t", "u", "u", "u", "u", "u", "u", "u", "u",
    "u", "u", "u", "u", "w", "w", "y", "y", "y", "z", "z", "z", "z", "z", "z", "s",
};

/* is this Latin letter a capital? (only used to keep the case of names) */
static int latin_upper(int cp) {
    if (cp >= 0xC0 && cp <= 0xDE) return cp != 0xD7;
    if (cp >= 0x100 && cp <= 0x137) return !(cp & 1);
    if (cp >= 0x139 && cp <= 0x148) return cp & 1;
    if (cp >= 0x14A && cp <= 0x177) return !(cp & 1);
    if (cp == 0x178) return 1;
    if (cp >= 0x179 && cp <= 0x17E) return cp & 1;
    return 0;
}

/* decode one UTF-8 sequence at s; returns its length (1..4) and the code point, or 1 and -1 when the bytes are broken */
static int decode(const unsigned char *s, int *cp) {
    unsigned c = s[0];
    if (c < 0x80) { *cp = (int)c; return 1; }
    int n = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC2 ? 2 : 0;
    if (!n || c > 0xF4) { *cp = -1; return 1; }
    int v = (int)(c & (n == 2 ? 0x1F : n == 3 ? 0x0F : 0x07));
    for (int i = 1; i < n; i++) {
        if ((s[i] & 0xC0) != 0x80) { *cp = -1; return 1; }       /* also catches the end of the string */
        v = (v << 6) | (int)(s[i] & 0x3F);
    }
    if ((n == 3 && v < 0x800) || (n == 4 && (v < 0x10000 || v > 0x10FFFF)) || (v >= 0xD800 && v <= 0xDFFF)) { *cp = -1; return 1; }
    *cp = v;
    return n;
}

/* what a character becomes: bytes into buf (<= 4); returns how many, -1 = a space, 0 = removed */
static int fold(int cp, int lower, char *buf) {
    if (cp < 0) return 0;                                         /* a broken byte */
    if (cp >= 0xFF01 && cp <= 0xFF5E) cp -= 0xFEE0;               /* full-width forms */
    if (cp < 0x80) {
        if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\v' || cp == '\f') return -1;
        if (cp < 0x20 || cp == 0x7F) return 0;
        buf[0] = (char)((lower && cp >= 'A' && cp <= 'Z') ? cp + 32 : cp);
        return 1;
    }
    const char *e = NULL;
    if (cp >= 0xC0 && cp <= 0xFF) e = L1[cp - 0xC0];
    else if (cp >= 0x100 && cp <= 0x17F) e = LA[cp - 0x100];
    if (e) {
        if (e[0] == ' ') return -1;
        int n = (int)strlen(e);
        memcpy(buf, e, (size_t)n);
        if (!lower && latin_upper(cp)) buf[0] = (char)(buf[0] - 32);
        return n;
    }
    switch (cp) {
    case 0xA0: case 0x1680: case 0x2028: case 0x2029: case 0x202F: case 0x205F: case 0x3000: return -1;   /* spaces */
    case 0xAD: case 0x2060: case 0x2061: case 0x2062: case 0x2063: case 0x2064: case 0xFEFF: return 0;    /* soft hyphen, word joiners, BOM */
    case 0xAA: buf[0] = 'a'; return 1;
    case 0xBA: buf[0] = 'o'; return 1;
    case 0xB9: buf[0] = '1'; return 1;
    case 0xB2: buf[0] = '2'; return 1;
    case 0xB3: buf[0] = '3'; return 1;
    case 0xB4: case 0x2B9: case 0x2BC: case 0x2018: case 0x2019: case 0x201A: case 0x201B: case 0x2032: buf[0] = '\''; return 1;
    case 0x201C: case 0x201D: case 0x201E: case 0x201F: case 0x2033: buf[0] = '"'; return 1;
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015: case 0x2043: case 0x2212: buf[0] = '-'; return 1;
    case 0x2026: memcpy(buf, "...", 3); return 3;
    }
    if ((cp >= 0x2000 && cp <= 0x200A)) return -1;                /* en / em / thin spaces ... */
    if ((cp >= 0x200B && cp <= 0x200F) || (cp >= 0x202A && cp <= 0x202E) || (cp >= 0x2066 && cp <= 0x2069) || (cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xE0100 && cp <= 0xE01EF)) return 0;
    /* everything else (emoji, kaomoji, other scripts): one word break. chunk 16 reads the emoji here before this happens */
    return -1;
}

static int clean(const char *text, char *out, int cap, int *map, int lower) {
    const unsigned char *s = (const unsigned char *)text;
    int n = 0, i = 0;
    if (cap < 1) return 0;
    while (s[i]) {
        int cp, len = decode(s + i, &cp);
        char buf[4]; int bn = fold(cp, lower, buf);
        if (bn < 0) {
            if (n > 0 && out[n - 1] != ' ') { if (n >= cap - 1) break; out[n] = ' '; if (map) map[n] = i; n++; }
        } else if (bn > 0) {
            if (n + bn > cap - 1) break;                          /* an expansion is never cut in half */
            for (int k = 0; k < bn; k++) { out[n] = buf[k]; if (map) map[n] = i; n++; }
        }
        i += len;
    }
    if (n > 0 && out[n - 1] == ' ') n--;                          /* no trailing space */
    out[n] = 0;
    if (map) map[n] = (int)strlen(text);
    return n;
}

int lang_clean(const char *text, char *out, int cap, int *map)      { return clean(text, out, cap, map, 1); }
int lang_clean_case(const char *text, char *out, int cap, int *map) { return clean(text, out, cap, map, 0); }

int lang_clean_orig_len(const char *text, int at) {
    int cp; return decode((const unsigned char *)text + at, &cp);
}
