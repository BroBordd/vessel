/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * The clean-up layer, part 1 (docs/convo_plan.md, chunk 15). See lang_clean.h for what it does and promises.
 */
#include "lang_clean.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
    case 0x1F3FB: case 0x1F3FC: case 0x1F3FD: case 0x1F3FE: case 0x1F3FF:     /* skin tones belong to their emoji */
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

/* ------------------------------------------------------------------------------------------------------------------------
 * chunk 16: tokens (emoticons, emoji, laughs) and the shape of the line. None of this changes the cleaned text.
 * ---------------------------------------------------------------------------------------------------------------------- */
static const char *const TOKEN_NAME[TK_COUNT] = { "none", "pos", "neg", "love", "shock", "cry", "angry", "laugh", "mask" };
const char *lang_token_name(int kind) { return kind >= 0 && kind < TK_COUNT ? TOKEN_NAME[kind] : "none"; }

/* a new token, or one more of the same kind right after the last one (a run of the same emoji is one token) */
static void tok_add(LangInfo *f, int kind, int a, int b, int len) {
    if (f->ntok && f->tok[f->ntok - 1].kind == kind && f->tok[f->ntok - 1].b == a) { f->tok[f->ntok - 1].b = b; f->tok[f->ntok - 1].len += len; return; }
    f->count[kind]++;
    if (f->ntok < LANG_MAX_TOK) { f->tok[f->ntok].kind = kind; f->tok[f->ntok].a = a; f->tok[f->ntok].b = b; f->tok[f->ntok].len = len; f->ntok++; }
}
/* an invisible character (a variation selector, a joiner) right after a token belongs to it */
static void tok_touch(LangInfo *f, int at, int len) { if (f->ntok && f->tok[f->ntok - 1].b == at) f->tok[f->ntok - 1].b = at + len; }

/* emoji by Unicode: which feeling (0 = not one we read) */
static int emoji_kind(int cp) {
    switch (cp) {
    case 0x1F602: case 0x1F923: case 0x1F639: case 0x1F606: return TK_LAUGH;
    case 0x2764: case 0x2665: case 0x1F60D: case 0x1F618: case 0x1F617: case 0x1F619: case 0x1F61A: case 0x1F970: case 0x1F63B: case 0x1F48B:
    case 0x1F90D: case 0x1F90E: case 0x1F9E1: case 0x1F5A4: return TK_LOVE;
    case 0x1F494: case 0x1F61E: case 0x1F612: case 0x1F614: case 0x1F615: case 0x1F641: case 0x2639: case 0x1F44E: case 0x1F4A9: case 0x1F480:
    case 0x2620: case 0x1F644: case 0x1F61F: case 0x1F623: case 0x1F616: case 0x1F62B: case 0x1F629: case 0x1F922: case 0x1F92E: return TK_NEG;
    case 0x1F622: case 0x1F62D: case 0x1F625: case 0x1F613: case 0x1F63F: case 0x1F972: return TK_CRY;
    case 0x1F620: case 0x1F621: case 0x1F624: case 0x1F47F: case 0x1F92C: case 0x1F595: case 0x1F4A2: return TK_ANGRY;
    case 0x1F62E: case 0x1F62F: case 0x1F632: case 0x1F631: case 0x1F628: case 0x1F630: case 0x1F633: case 0x1F440: case 0x1F92F: case 0x1F627:
    case 0x1F626: case 0x1F62C: return TK_SHOCK;
    case 0x1F600: case 0x1F601: case 0x1F603: case 0x1F604: case 0x1F605: case 0x1F607: case 0x1F609: case 0x1F60A: case 0x1F60B: case 0x1F60C:
    case 0x1F60E: case 0x1F642: case 0x1F643: case 0x263A: case 0x1F917: case 0x1F929: case 0x1F61C: case 0x1F61D: case 0x1F61B: case 0x1F44D:
    case 0x1F44F: case 0x1F44C: case 0x1F64C: case 0x1F389: case 0x1F38A: case 0x2728: case 0x1F31F: case 0x1F525: case 0x1F64F: case 0x1F4AA:
    case 0x2B50: case 0x1F4AF: return TK_POS;
    }
    if (cp >= 0x1F493 && cp <= 0x1F49F) return TK_LOVE;              /* the coloured and sparkling hearts (the broken heart is above) */
    return 0;
}

/* emoticons and kaomoji in ASCII, matched case-insensitively, the longest wins. a mouth may repeat (:))) :((( xDDD <333). */
static const struct { const char *s; int kind; } EMO[] = {
    { ">:(", TK_ANGRY }, { ">:-(", TK_ANGRY }, { ">:[", TK_ANGRY }, { ">=(", TK_ANGRY }, { ":@", TK_ANGRY },
    { ":'(", TK_CRY }, { ":'-(", TK_CRY }, { ";(", TK_CRY }, { ";_;", TK_CRY }, { ";-;", TK_CRY }, { ";^;", TK_CRY }, { ":,(", TK_CRY },
    { "t_t", TK_CRY }, { "t.t", TK_CRY }, { "t-t", TK_CRY }, { "twt", TK_CRY }, { "q_q", TK_CRY },
    { "xd", TK_LAUGH }, { "x-d", TK_LAUGH }, { "x'd", TK_LAUGH }, { ":'d", TK_LAUGH },
    { "</3", TK_NEG },
    { "<3", TK_LOVE }, { ":*", TK_LOVE }, { ":-*", TK_LOVE }, { ";*", TK_LOVE }, { "xoxo", TK_LOVE },
    { ":o", TK_SHOCK }, { ":-o", TK_SHOCK }, { "=o", TK_SHOCK }, { ";o", TK_SHOCK }, { "o_o", TK_SHOCK }, { "o.o", TK_SHOCK }, { "0_0", TK_SHOCK },
    { "0.0", TK_SHOCK }, { ":0", TK_SHOCK }, { ":-0", TK_SHOCK }, { "@_@", TK_SHOCK }, { "x_x", TK_SHOCK },
    { ":(", TK_NEG }, { ":-(", TK_NEG }, { "=(", TK_NEG }, { ":[", TK_NEG }, { "=[", TK_NEG }, { ":/", TK_NEG }, { ":-/", TK_NEG }, { ":\\", TK_NEG },
    { ":-\\", TK_NEG }, { ":<", TK_NEG }, { ":-<", TK_NEG }, { "-_-", TK_NEG }, { "-.-", TK_NEG }, { "-__-", TK_NEG }, { ">_<", TK_NEG }, { "._.", TK_NEG },
    { ":)", TK_POS }, { ":-)", TK_POS }, { "=)", TK_POS }, { "(:", TK_POS }, { "(-:", TK_POS }, { ":]", TK_POS }, { "=]", TK_POS }, { ":>", TK_POS },
    { ":d", TK_POS }, { ":-d", TK_POS }, { "=d", TK_POS }, { ";)", TK_POS }, { ";-)", TK_POS }, { ";d", TK_POS }, { ":p", TK_POS }, { ":-p", TK_POS },
    { ";p", TK_POS }, { "=p", TK_POS }, { ":3", TK_POS }, { "^_^", TK_POS }, { "^^", TK_POS }, { "^-^", TK_POS }, { "(^_^)", TK_POS }, { "(^-^)", TK_POS },
    { "(^^)", TK_POS }, { "^o^", TK_POS }, { "^.^", TK_POS }, { "\\o/", TK_POS }, { "uwu", TK_POS }, { "owo", TK_POS },
};
#define N_EMO ((int)(sizeof EMO / sizeof *EMO))
static const char EMO_FIRST[] = ">:;'=<xX^-(.\\oO0@_tTqQuU";      /* every first character of the table, both cases */

static int ci(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int alnum_c(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

/* the emoticon that starts at byte i of s: its length (0 = none) and its kind */
static int emoticon_at(const unsigned char *s, int i, int *kind) {
    int best = 0, bk = 0, prev = i ? s[i - 1] : 0;
    for (int k = 0; k < N_EMO; k++) {
        const char *e = EMO[k].s; int n = 0;
        while (e[n] && ci(s[i + n]) == e[n]) n++;
        if (e[n] || n <= best) continue;
        if (alnum_c(e[0]) ? alnum_c(prev) : (prev >= '0' && prev <= '9')) continue;    /* "axd", "10:30", "5:3" are not faces */
        best = n; bk = EMO[k].kind;
    }
    if (!best) return 0;
    int last = ci(s[i + best - 1]), L = best, next;
    if (last == '/' || last == '\\') { if (s[i + L] == last) return 0; }                /* "://" in an address */
    else while (ci(s[i + L]) == last && (!alnum_c(last) || last == 'd' || last == 'p' || last == 'o' || last == '3')) L++;   /* :))) xDDD <333 */
    next = s[i + L];
    if (alnum_c(last) && alnum_c(next) && !(s[i] == '<' && last == '3' && !(next >= '0' && next <= '9'))) return 0;   /* "<3u" is love, ":dog" is not a face */
    *kind = bk;
    return L;
}

/* laughter as whole lower case words */
static int is_haha(const char *w, int n) {                   /* haha hehe hihi ahahah muahaha hahahaa */
    int i = 0, k = 0;
    if (n > 3 && (!strncmp(w, "mwa", 3) || !strncmp(w, "mua", 3))) i = 3; else if (n > 2 && w[0] == 'a' && w[1] == 'h') i = 1;
    while (i + 1 < n && w[i] == 'h' && strchr("aeiou", w[i + 1])) { k++; i += 2; }
    if (k < 2) return 0;
    if (i < n && w[i] == 'h') i++;
    while (i < n && w[i] == w[i - 1] && strchr("aeiou", w[i])) i++;
    return i == n;
}
static int is_laugh(const char *w, int n) {
    if (n < 2 || n > 30) return 0;
    if (is_haha(w, n)) return 1;
    if (n >= 3 && w[0] == 'l') {                             /* lol lool lolol */
        int i = 1, ok = 0;
        while (i < n && w[i] == 'o') { while (i < n && w[i] == 'o') i++; if (i < n && w[i] == 'l') { i++; ok = 1; } else { ok = 0; break; } }
        if (ok && i == n) return 1;
    }
    if (!strncmp(w, "lm", 2)) { int i = 2; if (i < n && w[i] == 'f') i++; if (i + 1 < n && w[i] == 'a' && w[i + 1] == 'o') { i += 2; while (i < n && w[i] == 'o') i++; if (i == n) return 1; } }
    if (!strncmp(w, "rofl", 4)) { if (n == 4) return 1; if (!strncmp(w + 4, "mao", 3)) { int i = 7; while (i < n && w[i] == 'o') i++; return i == n; } }
    if (n >= 3) { int all = 1; for (int i = 0; i < n; i++) if (w[i] != 'k') all = 0; if (all) return 1; }                  /* kkkk */
    if (n >= 3 && w[0] == 'x') { int i = 0; while (i + 1 < n && w[i] == 'x' && w[i + 1] == 'd') i += 2; while (i < n && w[i] == 'd') i++; if (i == n) return 1; }   /* xd xdd xdxd */
    if (n >= 4 && w[0] == 'j' && strchr("aei", w[1])) { int i = 0, k = 0; while (i + 1 < n && w[i] == 'j' && strchr("aei", w[i + 1])) { k++; i += 2; } if (i < n && w[i] == 'j') i++; if (k >= 2 && i == n) return 1; }   /* jaja jeje */
    if (n >= 4 && !strncmp(w, "rs", 2)) { int i = 0, k = 0; while (i + 1 < n && w[i] == 'r' && w[i + 1] == 's') { k++; i += 2; } if (k >= 2 && i == n) return 1; }                    /* rsrs */
    if (n >= 4 && !strncmp(w, "wk", 2)) { int i = 0, k = 0; while (i + 1 < n && w[i] == 'w' && w[i + 1] == 'k') { k++; i += 2; } if (i < n && w[i] == 'w') i++; if (k >= 2 && i == n) return 1; }   /* wkwk */
    return !strcmp(w, "kek") || !strcmp(w, "kekw") || !strcmp(w, "lul") || !strcmp(w, "xd");
}

/* keyboard mash and gibberish, per whole lower case word */
static int vowel_c(int c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y'; }
static int is_noise(const char *w, int n) {
    static const char *const ROWS[] = { "qwertyuiop", "asdfghjkl", "zxcvbnm", "poiuytrewq", "lkjhgfdsa", "mnbvcxz" };
    static const char *const SHORT[] = { "asd", "sdf", "dfg", "fgh", "ghj", "hjk", "jkl", "qwe", "wer", "ert", "rty", "tyu", "yui", "uio", "iop", "zxc", "xcv", "cvb", "vbn", "bnm",
                                         "asdf", "qwer", "zxcv", "uiop", "hjkl", "fdsa", "poiu", "lkjh", NULL };
    if (n < 3 || is_laugh(w, n)) return 0;
    { int snd = 1, seen[26] = { 0 }, distinct = 0;                 /* shhhh hmmmm grrrr psssst pfffft are sounds, not mash */
      for (int i = 0; i < n; i++) { if (!strchr("shmgrbpfzwt", w[i])) snd = 0; else if (!seen[w[i] - 'a']++) distinct++; }
      if (snd && distinct <= 3) return 0; }
    for (int k = 0; SHORT[k]; k++) if (!strcmp(w, SHORT[k])) return 1;
    if (n < 4) return 0;
    for (int r = 0; r < 6; r++) { int rl = (int)strlen(ROWS[r]); for (int o = 0; o + 5 <= rl; o++) for (int i = 0; i + 5 <= n; i++) if (!memcmp(ROWS[r] + o, w + i, 5)) return 1; }   /* five keys in a row */
    int run = 0; for (int i = 0; i < n; i++) { run = vowel_c(w[i]) ? 0 : run + 1; if (run >= 6) return 1; }                     /* no vowel for six letters */
    if (n >= 6) { int same = 1; for (int i = 1; i < n; i++) if (w[i] != w[0]) same = 0; if (same && !strchr("mhszrw", w[0])) return 1; }   /* aaaaaa (hmmmm, shhhh, zzzz are sounds) */
    for (int u = 2; u <= 3; u++) {                                                                                              /* sdsdsd asdasdasd */
        if (n < 3 * u) continue;
        int rep = 1; for (int i = u; i < n; i++) if (w[i] != w[i - u]) rep = 0;
        if (rep && !(u == 2 && !vowel_c(w[0]) && vowel_c(w[1]))) return 1;                                                      /* lalala, papapa are singing, not mash */
    }
    return 0;
}

/* words of the cleaned text: laughs become tokens, mash counts toward the noise score */
static void read_words(const char *out, int n, const int *map, const char *text, LangInfo *f) {
    int letters = 0, noisy = 0;
    for (int i = 0; i < n;) {
        if (!(out[i] >= 'a' && out[i] <= 'z')) { i++; continue; }
        int j = i; while (j < n && out[j] >= 'a' && out[j] <= 'z') j++;
        int len = j - i; letters += len;
        if (is_laugh(out + i, len)) {
            int a = map[i], b = map[j - 1] + lang_clean_orig_len(text, map[j - 1]), seen = 0;
            for (int k = 0; k < f->ntok; k++) if (f->tok[k].kind == TK_LAUGH && f->tok[k].a <= a && f->tok[k].b >= b) { if (f->tok[k].len < len) f->tok[k].len = len; seen = 1; }   /* "xD" is both */
            if (!seen) tok_add(f, TK_LAUGH, a, b, len);
            if (len > f->laugh) f->laugh = len;
        } else if (is_noise(out + i, len)) noisy += len;
        i = j;
    }
    f->noise = letters ? 100 * noisy / letters : 0;
    if (f->noise >= 50 && noisy >= 3) f->flags |= LF_NOISE;
}

static int clean(const char *text, char *out, int cap, int *map, int lower, LangInfo *info) {
    const unsigned char *s = (const unsigned char *)text;
    int n = 0, i = 0, skip = 0, own[1024];
    if (cap < 1) return 0;
    if (info) { memset(info, 0, sizeof *info); if (!map) { map = own; if (cap > 1024) cap = 1024; } }
    while (s[i]) {
        int cp, len = decode(s + i, &cp);
        if (info) {
            if (cp >= 0x80) { int k = emoji_kind(cp); if (k) tok_add(info, k, i, i + len, 1); }
            else if (i >= skip && strchr(EMO_FIRST, cp)) { int k, L = emoticon_at(s, i, &k); if (L) { tok_add(info, k, i, i + L, 1); skip = i + L; } }
        }
        char buf[4]; int bn = fold(cp, 0, buf);                  /* the case is kept here, lowered below: the capitals are counted */
        if (bn < 0) {
            if (n > 0 && out[n - 1] != ' ') { if (n >= cap - 1) break; out[n] = ' '; if (map) map[n] = i; n++; }
        } else if (bn > 0) {
            if (n + bn > cap - 1) break;                          /* an expansion is never cut in half */
            for (int k = 0; k < bn; k++) {
                char c = buf[k];
                if (info && isalpha((unsigned char)c)) { info->letters++; if (isupper((unsigned char)c)) info->caps++; }
                if (lower && c >= 'A' && c <= 'Z') c = (char)(c + 32);
                out[n] = c; if (map) map[n] = i; n++;
            }
        } else if (info && cp >= 0) tok_touch(info, i, len);
        i += len;
    }
    if (n > 0 && out[n - 1] == ' ') n--;                          /* no trailing space */
    out[n] = 0;
    if (map) map[n] = (int)strlen(text);
    if (info) {
        for (int k = 0; k < n; k++) { if (out[k] == '!' && info->excl < 255) info->excl++; else if (out[k] == '?' && info->quest < 255) info->quest++; }
        char low[1024]; int m = n < 1023 ? n : 1023;
        for (int k = 0; k < m; k++) low[k] = (char)ci((unsigned char)out[k]);
        low[m] = 0;
        read_words(low, m, map, text, info);
        if (info->letters >= 4 && info->caps * 100 >= 60 * info->letters) info->flags |= LF_SHOUT;
        if (n > 120) info->flags |= LF_LONG;
        if (info->count[TK_LAUGH]) info->flags |= LF_LAUGH;
        if (info->count[TK_POS] + info->count[TK_NEG] + info->count[TK_LOVE] + info->count[TK_SHOCK] + info->count[TK_CRY] + info->count[TK_ANGRY]) info->flags |= LF_EMOTE;
        for (int a = 1; a < info->ntok; a++) { LangTok t = info->tok[a]; int b = a - 1; while (b >= 0 && info->tok[b].a > t.a) { info->tok[b + 1] = info->tok[b]; b--; } info->tok[b + 1] = t; }   /* in text order */
    }
    return n;
}

/* ------------------------------------------------------------------------------------------------------------------------
 * chunk 17: part 3 - elongation, leet, spaced letters, censor masks, repeated punctuation (see lang_clean.h).
 * Every stage reads (text, global index per char) and writes the same, where the index points into the text lang_clean_info made,
 * so the composition of the stages needs no extra work. Output is never longer than input.
 * ---------------------------------------------------------------------------------------------------------------------- */
static LangKnownFn KNOWN;
void lang_clean_set_known(LangKnownFn fn) { KNOWN = fn; }
static int known(const char *w, int n) { return KNOWN && n > 0 && KNOWN(w, n); }

#define W3 1024
static int is_a(int c) { return c >= 'a' && c <= 'z'; }
static int is_d(int c) { return c >= '0' && c <= '9'; }
static int is_aq(int c) { return is_a(c) || is_d(c) || c == '\''; }      /* part of a word */
static int is_mask(int c) { return c == '*' || c == '#' || c == '%' || c == '^' || c == '~'; }
static int is_trim(int c) { return c && strchr("\"'()[]{}<>.,;:?!*#%^~-_+=/\\|&", c) != NULL; }
static int is_vow(int c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y'; }

typedef struct { char *b; int *ib; int o; } Out;
static void emit(Out *u, int c, int idx) { u->b[u->o] = (char)c; u->ib[u->o] = idx; u->o++; }

/* stage 0: !!!! -> !   ???? -> ?   ?!?! -> ?!   !?!? -> !?   ..... -> ...   ,,, -> , */
static int st_punct(const char *a, const int *ia, int n, char *b, int *ib, LangInfo *f) {
    Out u = { b, ib, 0 };
    for (int i = 0; i < n;) {
        int c = a[i], j = i;
        if (c == '!' || c == '?') {
            while (j < n && (a[j] == '!' || a[j] == '?')) j++;
            int run = j - i, k = i + 1, keep = 1;
            while (k < j && a[k] == c) k++;
            if (k < j) keep = 2;                                   /* a mixed run keeps its first two different marks */
            if (run > keep) { emit(&u, c, ia[i]); if (keep == 2) emit(&u, a[k], ia[k]); f->npunct++; }
            else for (int q = i; q < j; q++) emit(&u, a[q], ia[q]);
        } else if (c == '.' || c == ',') {
            while (j < n && a[j] == c) j++;
            int run = j - i, keep = c == '.' ? (run >= 4 ? 3 : run) : 1;
            if (c == '.' && run >= 4) f->npunct++; else if (c == ',' && run >= 2) f->npunct++;
            for (int q = 0; q < keep; q++) emit(&u, c, ia[i + q]);
        } else { emit(&u, c, ia[i]); j = i + 1; }
        i = j;
    }
    b[u.o] = 0;
    return u.o;
}

/* stage 1: h e l l o   h.e.l.l.o   h-e-l-l-o   (single letters, one kind of separator) */
static int st_spaced(const char *a, const int *ia, int n, char *b, int *ib, LangInfo *f) {
    Out u = { b, ib, 0 };
    for (int i = 0; i < n;) {
        if (is_a(a[i]) && (i == 0 || !is_aq(a[i - 1])) && i + 1 < n) {
            int sep = a[i + 1], cnt = 1, k = i;
            if (sep == ' ' || sep == '.' || sep == '-' || sep == '_' || sep == ',') {
                while (k + 2 < n && a[k + 1] == sep && is_a(a[k + 2]) && !is_aq(a[k + 3])) { k += 2; cnt++; }
                if (cnt >= 3) {
                    char w[W3]; int m = 0;
                    for (int q = i; q <= k; q += 2) w[m++] = a[q];
                    if (cnt >= 4 || known(w, m)) {
                        for (int q = i; q <= k; q += 2) emit(&u, a[q], ia[q]);
                        f->nspaced++;
                    } else for (int q = i; q <= k; q++) emit(&u, a[q], ia[q]);
                    i = k + 1;
                    continue;
                }
            }
        }
        emit(&u, a[i], ia[i]); i++;
    }
    b[u.o] = 0;
    return u.o;
}

/* a censored word: * # % ^ ~ between letters (f*ck, s**t, b***h); a*b and i*m are not */
static int is_masked_word(const char *c, int n) {
    int letters = 0;
    for (int i = 0; i < n; i++) if (is_a(c[i])) letters++;
    if (letters < 2) return 0;
    for (int i = 1; i < n - 1;) {
        if (!is_mask(c[i])) { i++; continue; }
        int e = i; while (e < n && is_mask(c[e])) e++;
        if (is_a(c[i - 1]) && e < n && is_a(c[e]) && (e - i >= 2 || letters >= 3)) return 1;
        i = e;
    }
    return 0;
}

/* leet: which letter a character stands for (1 is i here; he11o and l1k3 are settled below) */
static int leet_of(int c) {
    switch (c) { case '0': return 'o'; case '1': return 'i'; case '3': return 'e'; case '4': return 'a'; case '5': return 's'; case '7': return 't';
                 case '@': return 'a'; case '$': return 's'; case '!': return 'i'; }
    return 0;
}
/* the shapes that are safe to read as leet when no dictionary knows the result: never 3pm 1st b4 gr8 2day 10k user1 mp3 ps5 a number */
static int leet_safe(const char *c, int n) {
    static const char *const SUF[] = { "st", "nd", "rd", "th", "am", "pm", "s", "k", "m", "x", "d", "yo", "y", NULL };
    int letters = 0, vowel = 0;                                      /* vowel: in the reading, so the 0 of h3ll0 counts */
    for (int i = 0; i < n; i++) {
        if (is_a(c[i])) letters++;
        if (is_vow(is_a(c[i]) ? c[i] : leet_of(c[i]))) vowel = 1;
        if (c[i] == '!' || c[i] == '@') return 0;                  /* ambiguous: only a known word may decide */
    }
    for (int i = 0; i < n;) {                                       /* digit runs */
        if (!is_d(c[i])) { i++; continue; }
        int e = i; while (e < n && is_d(c[e])) e++;
        if (e - i >= 3 || (e - i == 2 && c[i] != c[i + 1])) return 0;
        i = e;
    }
    if (c[0] == '$') {                                              /* $hut $ucks, not $5 */
        if (n < 4 || is_d(c[1]) || letters < 3) return 0;
    }
    if (is_d(c[0])) {                                               /* a leading digit: 1ove 0kay 3vil yes, 3pm 1st 4ever 0k no */
        if (c[0] == '4') return 0;
        int e = 0; while (e < n && is_d(c[e])) e++;
        if (n - e < 2) return 0;
        for (int k = 0; SUF[k]; k++) if (!strcmp(c + e, SUF[k])) return 0;
    }
    if (n == 2) {                                                   /* h1 n0 m3 i5 i7 : a letter and one trailing digit */
        if (!(is_a(c[0]) && strchr("01357", c[1]))) return 0;
    }
    for (int i = 1; i + 1 < n; i++) if (c[i] == '4' && (is_d(c[i - 1]) || is_d(c[i + 1]))) return 0;
    if (c[n - 1] == '4') return 0;                                  /* b4 mp4 */
    if (is_d(c[n - 1]) && n >= 3) {                                 /* hell0 yes: needs three letters before it and a vowel in the reading (not mp3 ps5) */
        if (!strchr("035", c[n - 1])) return 0;
        if (letters < 3 || !vowel) return 0;
    }
    return vowel;
}
/* read leet in place (1 to 1, c is at most W3 long); returns 1 if it changed */
static int leet_fix(char *c, int n) {
    int nl = 0, np = 0, pos[8], letters = 0;
    for (int i = 0; i < n; i++) {
        if (is_a(c[i])) letters++;
        else if (c[i] != '\'' && !leet_of(c[i])) return 0;           /* a digit that is not leet (2 6 8 9) or any other mark: not a leet word */
        if (leet_of(c[i])) nl++;
    }
    if (!nl || !letters) return 0;
    char v[W3];
    for (int i = 0; i < n; i++) {
        int L = leet_of(c[i]);
        v[i] = (char)(L ? L : c[i]);
        if (c[i] == '1') {
            if ((i > 0 && c[i - 1] == '1') || (i + 1 < n && c[i + 1] == '1')) v[i] = 'l';      /* he11o */
            else if (np < 8) pos[np++] = i;                          /* a lone 1 may be i or l */
        }
    }
    v[n] = 0;
    int np2 = np > 4 ? 4 : np;
    for (int combo = 0; combo < (1 << np2); combo++) {               /* the default reading first, then the other i / l choices */
        char w[W3]; memcpy(w, v, (size_t)n + 1);
        for (int q = 0; q < np2; q++) if (combo >> q & 1) w[pos[q]] = 'l';
        if (known(w, n)) { memcpy(c, w, (size_t)n); return 1; }
    }
    if (!leet_safe(c, n)) return 0;
    memcpy(c, v, (size_t)n);
    return 1;
}

/* elongation: runs of 3+ of one letter go to 1 or 2. A known word wins (two first: goood is good, not god; heeello has no heello, so it is
 * hello); with no known word see the rule below */
static int elong_fix(char *c, int *x, int *n, LangInfo *f) {
    int rs[6], rl[6], nr = 0, N = *n;
    for (int i = 0; i < N;) {
        int e = i; while (e < N && c[e] == c[i]) e++;
        if (e - i >= 3 && is_a(c[i])) { if (nr < 6) { rs[nr] = i; rl[nr] = e - i; } nr++; }
        i = e;
    }
    if (!nr) return 0;
    int rr = nr > 6 ? 6 : nr, pick = -1;                              /* pick: bit q set = run q keeps 1 letter, clear = 2 */
    if (rr <= 4 && KNOWN) {
        for (int combo = 0; combo < (1 << rr) && pick < 0; combo++) {
            char w[W3]; int m = 0, r = 0;
            for (int i = 0; i < N;) {
                if (r < rr && i == rs[r]) { for (int q = 0; q < ((combo >> r & 1) ? 1 : 2); q++) w[m++] = c[i]; i += rl[r]; r++; }
                else w[m++] = c[i++];
            }
            if (known(w, m)) pick = combo;
        }
    }
    if (pick < 0) {                                                   /* nothing known: a, i, u, y runs and long e, o runs go to 1; a short e or o run (soon, good, see) and every consonant run go to 2 */
        pick = 0;
        for (int r = 0; r < rr; r++) { int ch = c[rs[r]]; if (is_vow(ch) && !((ch == 'e' || ch == 'o') && rl[r] <= 4)) pick |= 1 << r; }
    }
    char nc[W3]; int nx[W3], m = 0, r = 0, longest = 0;
    for (int i = 0; i < N;) {
        if (r < rr && i == rs[r]) {
            int keep = (pick >> r & 1) ? 1 : 2;
            int last = i + rl[r] == N;                                 /* a run that ends the word maps to its LAST letter, so a highlight covers it all */
            for (int q = 0; q < keep; q++) { nc[m] = c[i]; nx[m] = (last && q == keep - 1) ? x[i + rl[r] - 1] : x[i + q]; m++; }
            if (rl[r] > longest) longest = rl[r];
            i += rl[r]; r++;
        } else { nc[m] = c[i]; nx[m] = x[i]; m++; i++; }
    }
    memcpy(c, nc, (size_t)m); c[m] = 0; memcpy(x, nx, (size_t)m * sizeof(int)); *n = m;
    f->nstretch++; if (longest > f->stretch) f->stretch = longest;
    return 1;
}

/* one token (a run without spaces) */
static void do_token(const char *t, const int *it, int m, const int *inmap, Out *u, LangInfo *f) {
    for (int k = 0; k < m; k++)                                      /* addresses, paths and numbers like 3.5 are left alone */
        if (t[k] == '/' || (t[k] == '.' && k > 0 && k + 1 < m && is_aq(t[k - 1]) && is_aq(t[k + 1]))) { for (int q = 0; q < m; q++) emit(u, t[q], it[q]); return; }
    int lo = 0, hi = m;
    while (lo < hi && is_trim(t[lo])) lo++;
    while (hi > lo && is_trim(t[hi - 1])) hi--;
    for (int q = 0; q < lo; q++) emit(u, t[q], it[q]);
    if (lo < hi) {
        char c[W3]; int x[W3], n = hi - lo, ok = 1;
        for (int q = 0; q < n; q++) { c[q] = t[lo + q]; x[q] = it[lo + q]; if (!(is_aq(c[q]) || c[q] == '@' || c[q] == '$' || c[q] == '!' || is_mask(c[q]))) ok = 0; }
        c[n] = 0;                                                  /* leet_safe compares tails as strings */
        if (ok && is_masked_word(c, n)) {
            tok_add(f, TK_MASK, inmap[x[0]], inmap[x[n - 1] + 1], n);
        } else {
            if (ok && leet_fix(c, n)) f->nleet++;
            elong_fix(c, x, &n, f);
        }
        for (int q = 0; q < n; q++) emit(u, c[q], x[q]);
    }
    for (int q = hi > lo ? hi : lo; q < m; q++) emit(u, t[q], it[q]);
}

static int st_tokens(const char *a, const int *ia, int n, char *b, int *ib, const int *inmap, LangInfo *f) {
    Out u = { b, ib, 0 };
    for (int i = 0; i < n;) {
        if (a[i] == ' ') { emit(&u, ' ', ia[i]); i++; continue; }
        int j = i; while (j < n && a[j] != ' ') j++;
        do_token(a + i, ia + i, j - i, inmap, &u, f);
        i = j;
    }
    b[u.o] = 0;
    return u.o;
}

int lang_clean_words(const char *in, const int *inmap, char *out, int cap, int *outmap, int *src, LangInfo *info) {
    LangInfo own;
    char b1[W3], b2[W3], b3[W3]; int i0[W3], i1[W3], i2[W3], i3[W3];
    int n = (int)strlen(in);
    if (cap < 1) return 0;
    if (n > W3 - 2) n = W3 - 2;
    if (!info) { memset(&own, 0, sizeof own); info = &own; }
    for (int i = 0; i < n; i++) { b1[i] = in[i]; i0[i] = i; }
    b1[n] = 0;
    n = st_punct(b1, i0, n, b2, i1, info);
    n = st_spaced(b2, i1, n, b3, i2, info);
    n = st_tokens(b3, i2, n, b1, i3, inmap, info);
    if (n > cap - 1) n = cap - 1;
    memcpy(out, b1, (size_t)n); out[n] = 0;
    for (int i = 0; i < n; i++) { if (src) src[i] = i3[i]; if (outmap) outmap[i] = inmap[i3[i]]; }
    if (outmap) outmap[n] = inmap[(int)strlen(in)];
    if (src) src[n] = (int)strlen(in);
    if (info->nstretch) info->flags |= LF_STRETCH;
    if (info->nleet) info->flags |= LF_LEET;
    if (info->nspaced) info->flags |= LF_SPACED;
    if (info->count[TK_MASK]) info->flags |= LF_MASKED;
    if (info->npunct) info->flags |= LF_PUNCT;
    for (int a = 1; a < info->ntok; a++) { LangTok t = info->tok[a]; int b = a - 1; while (b >= 0 && info->tok[b].a > t.a) { info->tok[b + 1] = info->tok[b]; b--; } info->tok[b + 1] = t; }
    return n;
}

static const char *numexpand(const char *in, int n, int i, int a, int b, int j);

/* ---- chunk 18: contractions and text-speak (see lang_clean.h) ---- */
static LangExpandFn EXPAND;
void lang_clean_set_expand(LangExpandFn fn) { EXPAND = fn; }

int lang_clean_expand(const char *in, const int *inmap, char *out, int cap, int *outmap, int *src, LangInfo *info) {
    int n = (int)strlen(in), o = 0, nx = 0, own[W3];
    if (cap < 1) return 0;
    if (outmap && !src) { src = own; if (cap > W3) cap = W3; }       /* the map needs the sources */
    for (int i = 0; i < n && o < cap - 1;) {
        if (!is_aq(in[i])) { out[o] = in[i]; if (src) src[o] = i; o++; i++; continue; }
        int j = i; while (j < n && is_aq(in[j])) j++;
        int a = i, b = j;                                          /* the word without edge apostrophes (a quote, not a letter) */
        while (a < b && in[a] == '\'') a++;
        while (b > a && in[b - 1] == '\'') b--;
        const char *ex = b > a ? numexpand(in, n, i, a, b, j) : NULL;          /* chunk 19: b4 gr8 2morrow 4ever, a lone 2 / 4 */
        if (!ex && EXPAND && b > a) ex = EXPAND(in + a, b - a);
        int xl = ex ? (int)strlen(ex) : 0;
        if (ex && xl > 0 && o + (a - i) + xl + (j - b) <= cap - 1) {
            for (int q = i; q < a; q++) { out[o] = in[q]; if (src) src[o] = q; o++; }
            for (int q = 0; q < xl; q++) { out[o] = ex[q]; if (src) src[o] = (q == xl - 1) ? b - 1 : a; o++; }
            for (int q = b; q < j; q++) { out[o] = in[q]; if (src) src[o] = q; o++; }
            nx++;
        } else for (int q = i; q < j && o < cap - 1; q++) { out[o] = in[q]; if (src) src[o] = q; o++; }
        i = j;
    }
    out[o] = 0;
    if (src) src[o] = n;
    if (outmap && inmap) { for (int k = 0; k < o; k++) outmap[k] = inmap[src[k]]; outmap[o] = inmap[n]; }
    if (info) { info->nexpand = nx; if (nx) info->flags |= LF_TEXTSPEAK; }
    return o;
}

/* ---- chunk 19: numbers (see lang_clean.h) ---- */
static int NUMBERS_ON = 1;
void lang_clean_set_numbers(int on) { NUMBERS_ON = on; }

static const char *const NW_UNIT[] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", NULL };
static const char *const NW_TEEN[] = { "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", NULL };
static const char *const NW_TENS[] = { "", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety", NULL };
enum { NWK_NONE, NWK_UNIT, NWK_TEEN, NWK_TENS, NWK_HUND, NWK_SCALE, NWK_AND };
static int weq(const char *w, int n, const char *s) { return (int)strlen(s) == n && !strncmp(w, s, (size_t)n); }
static int nw_kind(const char *w, int n, long *v) {
    for (int i = 0; NW_UNIT[i]; i++) if (weq(w, n, NW_UNIT[i])) { *v = i; return NWK_UNIT; }
    for (int i = 0; NW_TEEN[i]; i++) if (weq(w, n, NW_TEEN[i])) { *v = 10 + i; return NWK_TEEN; }
    for (int i = 2; i < 10; i++) if (weq(w, n, NW_TENS[i]) || (i == 4 && weq(w, n, "fourty"))) { *v = i * 10; return NWK_TENS; }
    if (weq(w, n, "hundred")) { *v = 100; return NWK_HUND; }
    if (weq(w, n, "thousand")) { *v = 1000; return NWK_SCALE; }
    if (weq(w, n, "million")) { *v = 1000000; return NWK_SCALE; }
    if (weq(w, n, "and")) { *v = 0; return NWK_AND; }
    return NWK_NONE;
}
/* the state of a number in words being read: last = kind of the last word (7 = closed by a lone zero) */
typedef struct { long total, cur, scale; int last; } NW;
static void nw_init(NW *s) { s->total = 0; s->cur = 0; s->scale = 1000000000L; s->last = 0; }
static int nw_step(NW *s, const char *w, int n) {
    long v; int k = nw_kind(w, n, &v), L = s->last;
    switch (k) {
    case NWK_UNIT:
        if (v == 0) { if (L != 0) return 0; s->last = 7; return 1; }
        if (L != 0 && L != 3 && L != 4 && L != 5 && L != 6) return 0;
        s->cur += v; s->last = 1; return 1;
    case NWK_TEEN: case NWK_TENS:
        if (L != 0 && L != 4 && L != 5 && L != 6) return 0;
        s->cur += v; s->last = k == NWK_TEEN ? 2 : 3; return 1;
    case NWK_HUND:
        if (L != 1 || s->cur < 1 || s->cur > 9) return 0;
        s->cur *= 100; s->last = 4; return 1;
    case NWK_SCALE:
        if (s->cur <= 0 || L == 6 || L == 5 || v >= s->scale) return 0;
        s->total += s->cur * v; s->cur = 0; s->scale = v; s->last = 5; return 1;
    case NWK_AND:
        if (L != 4 && L != 5) return 0;
        s->last = 6; return 1;
    }
    return 0;
}
static int nw_valid(const NW *s) { return s->last != 0 && s->last != 6; }
/* a token (letters, maybe with hyphens) as a number word; the state is only changed when every piece is accepted */
static int nw_token(NW *s, const char *w, int n) {
    NW c = *s; int i = 0;
    while (i < n) {
        int j = i; while (j < n && w[j] != '-' && w[j] != ' ') j++;
        if (j == i || !nw_step(&c, w + i, j - i)) return 0;
        i = j + 1;
    }
    *s = c; return 1;
}
int lang_num_words(const char *w, int n, long *v) {
    NW s; nw_init(&s);
    if (n <= 0 || !nw_token(&s, w, n) || !nw_valid(&s)) return 0;
    if (v) *v = s.total + s.cur;
    return 1;
}
static int put(char *o, int cap, int *n, const char *s) {
    int l = (int)strlen(s), sp = *n > 0;
    if (*n + sp + l > cap - 1) return 0;
    if (sp) o[(*n)++] = ' ';
    memcpy(o + *n, s, (size_t)l); *n += l; o[*n] = 0; return 1;
}
static int words_below_1000(long v, char *o, int cap, int *n) {
    if (v >= 100) { if (!put(o, cap, n, NW_UNIT[v / 100]) || !put(o, cap, n, "hundred")) return 0; v %= 100; }
    if (v >= 20) { if (!put(o, cap, n, NW_TENS[v / 10])) return 0; v %= 10; if (v && !put(o, cap, n, NW_UNIT[v])) return 0; }
    else if (v >= 10) { if (!put(o, cap, n, NW_TEEN[v - 10])) return 0; }
    else if (v > 0) { if (!put(o, cap, n, NW_UNIT[v])) return 0; }
    return 1;
}
int lang_num_to_words(long v, char *o, int cap) {
    int n = 0;
    if (cap < 1) return 0;
    o[0] = 0;
    if (v <= -1000000000L || v >= 1000000000L) return 0;
    if (v < 0) { if (!put(o, cap, &n, "minus")) { o[0] = 0; return 0; } v = -v; }
    if (v == 0) { if (!put(o, cap, &n, "zero")) { o[0] = 0; return 0; } return n; }
    if (v >= 1000000) { if (!words_below_1000(v / 1000000, o, cap, &n) || !put(o, cap, &n, "million")) { o[0] = 0; return 0; } v %= 1000000; }
    if (v >= 1000) { if (!words_below_1000(v / 1000, o, cap, &n) || !put(o, cap, &n, "thousand")) { o[0] = 0; return 0; } v %= 1000; }
    if (!words_below_1000(v, o, cap, &n)) { o[0] = 0; return 0; }
    return n;
}
int lang_num_ordinal(long v, char *o, int cap) {
    char b[24]; long a = v < 0 ? -v : v, m = a % 100;
    const char *s = (m >= 11 && m <= 13) ? "th" : a % 10 == 1 ? "st" : a % 10 == 2 ? "nd" : a % 10 == 3 ? "rd" : "th";
    int n = snprintf(b, sizeof b, "%ld%s", v, s);
    if (cap < 1) return 0;
    if (n >= cap) { o[0] = 0; return 0; }
    memcpy(o, b, (size_t)n + 1); return n;
}

typedef struct { int a, b; } Tk;
#define NTK 160
static int tk_tokens(const char *t, int n, Tk *tk, int max) {
    int c = 0;
    for (int i = 0; i < n && c < max;) {
        int ch = (unsigned char)t[i];
        int neg = ch == '-' && i + 1 < n && is_d(t[i + 1]) && (i == 0 || t[i - 1] == ' ');
        if (!(is_aq(ch) || neg)) { i++; continue; }
        int j = i + 1;
        while (j < n) {
            int d = (unsigned char)t[j];
            if (is_aq(d)) j++;
            else if ((d == ':' || d == '.') && j + 1 < n && is_d(t[j - 1]) && is_d(t[j + 1])) j++;
            else if (d == '-' && j + 1 < n && is_a(t[j - 1]) && is_a(t[j + 1])) j++;
            else break;
        }
        int a = i, b = j;
        while (a < b && t[a] == '\'') a++;
        while (b > a && t[b - 1] == '\'') b--;
        if (a < b) { tk[c].a = a; tk[c].b = b; c++; }
        i = j;
    }
    return c;
}
static int teq(const char *t, Tk k, const char *s) { return weq(t + k.a, k.b - k.a, s); }
static int tin(const char *t, Tk k, const char *const *l) { for (; *l; l++) if (teq(t, k, *l)) return 1; return 0; }
static int tgap_ok(const char *t, Tk x, Tk y) { for (int i = x.b; i < y.a; i++) if (t[i] != ' ') return 0; return 1; }

static const char *const UNITS_W[] = { "hours", "hour", "minutes", "minute", "mins", "min", "seconds", "second", "secs", "days", "day", "weeks", "week",
    "months", "month", "times", "time", "feet", "foot", "miles", "mile", "percent", "km", "kg", "lbs", "inches", "inch", "dollars", "bucks", "points", "steps", NULL };
static const char *const YEARS_W[] = { "years", "year", "yrs", "yr", NULL };
static const char *const OPS_W[] = { "plus", "minus", "times", "and", "by", "over", "divided", "multiplied", "x", "or", NULL };
static const char *const ORD_W[] = { "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth",
    "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth", "twentieth", "thirtieth", "fortieth", "fiftieth", "hundredth", "thousandth", NULL };
static const char *const ORD_OF[] = { "the", "my", "your", "his", "her", "our", "their", NULL };

static int is_numword_tok(const char *t, Tk k) { NW s; nw_init(&s); return nw_token(&s, t + k.a, k.b - k.a); }

static int age_ctx(const char *t, const Tk *tk, int nt, int i, int j, long v) {
    if (v < 1 || v > 120) return 0;
    if (j < nt && tgap_ok(t, tk[j - 1], tk[j])) {
        if (teq(t, tk[j], "yo")) return 1;
        if (tin(t, tk[j], YEARS_W) && j + 1 < nt && teq(t, tk[j + 1], "old")) return 1;
        if (tin(t, tk[j], YEARS_W) && j + 2 < nt && teq(t, tk[j + 1], "of") && teq(t, tk[j + 2], "age")) return 1;
    }
    int route = 0;
    if (i >= 1 && tgap_ok(t, tk[i - 1], tk[i])) {
        Tk p = tk[i - 1];
        if (teq(t, p, "im") || teq(t, p, "i'm")) route = 1;
        else if (teq(t, p, "am") && (i == 1 || (i >= 2 && teq(t, tk[i - 2], "i")))) route = 1;
        else if (teq(t, p, "aged") || teq(t, p, "age") || teq(t, p, "turned") || teq(t, p, "turning")) route = 1;
        else if (teq(t, p, "is") && i >= 2 && teq(t, tk[i - 2], "age")) route = 1;
    }
    if (route && j < nt && tgap_ok(t, tk[j - 1], tk[j]) && tin(t, tk[j], UNITS_W)) route = 0;
    return route;
}

static void num_add(LangInfo *f, const char *orig, const int *map, int ta, int tb, int kind, long v) {   /* [ta, tb) in the cleaned text */
    if (f->nnum >= LANG_MAX_NUM) return;
    LangNum *n = &f->num[f->nnum++];
    n->kind = kind; n->value = (int)v;
    if (map) { n->a = map[ta]; int at = map[tb - 1]; n->b = at + (orig ? lang_clean_orig_len(orig, at) : 1); }
    else { n->a = ta; n->b = tb; }
}

int lang_clean_numbers(const char *orig, const char *t, const int *map, LangInfo *info) {
    Tk tk[NTK]; LangInfo own;
    int n = (int)strlen(t), nt = tk_tokens(t, n, tk, NTK);
    if (!info) { memset(&own, 0, sizeof own); info = &own; }
    info->nnum = 0;
    if (!NUMBERS_ON) return 0;
    for (int i = 0; i < nt;) {
        const char *w = t + tk[i].a; int wl = tk[i].b - tk[i].a, j = i + 1, kind = 0; long val = 0;
        if (is_d(w[0]) || (w[0] == '-' && wl > 1)) {
            int p = w[0] == '-', neg = p; long d1 = 0; int nd = 0;
            while (p < wl && is_d(w[p])) { if (d1 < 100000000L) d1 = d1 * 10 + (w[p] - '0'); p++; nd++; }
            if (!nd) { i++; continue; }
            long d2 = -1; int colon = 0, dec = 0;
            if (p < wl && w[p] == ':') { colon = 1; p++; int c2 = 0; d2 = 0; while (p < wl && is_d(w[p])) { d2 = d2 * 10 + (w[p] - '0'); p++; c2++; } if (c2 != 2) { i++; continue; } }
            else if (p < wl && w[p] == '.') { dec = 1; p++; while (p < wl && is_d(w[p])) p++; }
            const char *s = w + p; int sl = wl - p;
            if (neg) d1 = -d1;
            if (sl == 0) {
                if (colon) {
                    if (neg || d1 > 23 || d2 > 59) { i++; continue; }
                    kind = NK_TIME; val = d1 * 60 + d2;
                    if (j < nt && tgap_ok(t, tk[i], tk[j]) && d1 >= 1 && d1 <= 12 && (teq(t, tk[j], "am") || teq(t, tk[j], "pm"))) { val = (d1 % 12 + (teq(t, tk[j], "pm") ? 12 : 0)) * 60 + d2; j++; }   /* 11:59 pm */
                }
                else { if (wl == 1 && numexpand(t, n, tk[i].a, tk[i].a, tk[i].b, tk[i].b)) { i++; continue; } kind = NK_NUMBER; val = d1; }   /* a 2 / 4 that reads as to / too / for is not a number */
            } else if (weq(s, sl, "am") || weq(s, sl, "pm")) {
                if (neg || dec || d1 < 1 || d1 > 12 || (colon && d2 > 59)) { i++; continue; }
                kind = NK_TIME; val = (d1 % 12 + (s[0] == 'p' ? 12 : 0)) * 60 + (colon ? d2 : 0);
            } else if (!colon && !dec && (weq(s, sl, "st") || weq(s, sl, "nd") || weq(s, sl, "rd") || weq(s, sl, "th")) && !neg) { kind = NK_ORDINAL; val = d1; }
            else if (!colon && !dec && weq(s, sl, "k") && !neg && d1 < 1000000) { kind = NK_NUMBER; val = d1 * 1000; }
            else if (!colon && !dec && weq(s, sl, "yo") && d1 >= 1 && d1 <= 120) { kind = NK_AGE; val = d1; }
            else { i++; continue; }                                  /* user1 mp3 3d 2day 90s: not numbers */
            if (kind == NK_NUMBER) {                                 /* "3 pm", "3 o'clock", "im 12" */
                if (j < nt && tgap_ok(t, tk[i], tk[j]) && val >= 1 && val <= 12 && (teq(t, tk[j], "am") || teq(t, tk[j], "pm"))) { kind = NK_TIME; val = (val % 12 + (teq(t, tk[j], "pm") ? 12 : 0)) * 60; j++; }
                else if (j < nt && tgap_ok(t, tk[i], tk[j]) && val >= 1 && val <= 12 && (teq(t, tk[j], "oclock") || teq(t, tk[j], "o'clock"))) { kind = NK_TIME; val = val * 60; j++; }
                else if (age_ctx(t, tk, nt, i, j, val)) kind = NK_AGE;
            }
        } else if (weq(w, wl, "midnight")) { kind = NK_TIME; val = 0; }
        else if (weq(w, wl, "noon") || weq(w, wl, "midday")) { kind = NK_TIME; val = 720; }
        else if ((weq(w, wl, "first") || weq(w, wl, "second")) && i >= 1 && tgap_ok(t, tk[i - 1], tk[i]) && tin(t, tk[i - 1], ORD_OF)) { kind = NK_ORDINAL; val = weq(w, wl, "first") ? 1 : 2; }
        else if (tin(t, tk[i], ORD_W)) {
            kind = NK_ORDINAL;
            for (int q = 0; ORD_W[q]; q++) if (weq(w, wl, ORD_W[q])) { static const int OV[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 30, 40, 50, 100, 1000 }; val = OV[q]; }
        } else {                                                     /* number words: the longest valid run */
            NW s; nw_init(&s); int best = -1; long bv = 0, v2;
            for (int k = i; k < nt; k++) {
                if (k > i && !tgap_ok(t, tk[k - 1], tk[k])) break;
                if (!nw_token(&s, t + tk[k].a, tk[k].b - tk[k].a)) break;
                if (nw_valid(&s)) { best = k + 1; bv = s.total + s.cur; }
            }
            if (best < 0) { i++; continue; }
            j = best; val = bv; kind = NK_NUMBER; (void)v2;
            if (j == i + 1 && weq(w, wl, "one")) {                  /* a lone "one" is a pronoun unless the numbers around it say otherwise */
                int ok = nt == 1 || (nt == 2 && i == 0);
                if (i > 0 && tgap_ok(t, tk[i - 1], tk[i]) && (tin(t, tk[i - 1], OPS_W) || is_d(t[tk[i - 1].a]) || is_numword_tok(t, tk[i - 1]))) ok = 1;
                if (j < nt && tgap_ok(t, tk[i], tk[j]) && (tin(t, tk[j], OPS_W) || is_d(t[tk[j].a]) || is_numword_tok(t, tk[j]))) ok = 1;
                if (!ok) { i++; continue; }
            }
            if (j < nt && tgap_ok(t, tk[j - 1], tk[j]) && val >= 1 && val <= 12 && (teq(t, tk[j], "am") || teq(t, tk[j], "pm"))) { kind = NK_TIME; val = (val % 12 + (teq(t, tk[j], "pm") ? 12 : 0)) * 60; j++; }
            else if (j < nt && tgap_ok(t, tk[j - 1], tk[j]) && val >= 1 && val <= 12 && (teq(t, tk[j], "oclock") || teq(t, tk[j], "o'clock"))) { kind = NK_TIME; val = val * 60; j++; }
            else if (age_ctx(t, tk, nt, i, j, val)) kind = NK_AGE;
        }
        num_add(info, orig, map, tk[i].a, tk[j - 1].b, kind, val);
        i = j;
    }
    if (info->nnum) info->flags |= LF_NUMBER;
    return info->nnum;
}

/* ---- chunk 19: number-letter words in the expanded variant (b4 gr8 l8r 2morrow 4ever any1) and a lone 2 / 4 in a word-like place ---- */
static const char *const NLW[][2] = {
    { "b4", "before" }, { "b4n", "bye for now" }, { "gr8", "great" }, { "l8r", "later" }, { "l8", "late" }, { "m8", "mate" }, { "w8", "wait" }, { "w84", "wait for" },
    { "str8", "straight" }, { "h8", "hate" }, { "d8", "date" }, { "g8", "gate" }, { "f8", "fate" }, { "sk8", "skate" }, { "cr8", "create" },
    { "2day", "today" }, { "2moro", "tomorrow" }, { "2morro", "tomorrow" }, { "2morow", "tomorrow" }, { "2mrw", "tomorrow" }, { "2mro", "tomorrow" }, { "2morrow", "tomorrow" },
    { "2nite", "tonight" }, { "2night", "tonight" }, { "2gether", "together" }, { "2b", "to be" }, { "2u", "to you" }, { "2late", "too late" }, { "2much", "too much" },
    { "4ever", "forever" }, { "4eva", "forever" }, { "4get", "forget" }, { "4got", "forgot" }, { "4give", "forgive" }, { "4u", "for you" }, { "4real", "for real" },
    { "any1", "anyone" }, { "ne1", "anyone" }, { "every1", "everyone" }, { "some1", "someone" }, { "no1", "no one" }, { NULL, NULL } };
static const char *const TO_NEXT[] = { "be", "go", "see", "get", "do", "have", "know", "say", "tell", "find", "make", "take", "give", "come", "leave", "play", "help", "ask", "talk",
    "speak", "meet", "die", "live", "stay", "run", "kill", "hurt", "save", "stop", "try", "look", "hear", "feel", "think", "want", "learn", "read", "write", "sleep", "eat", "fight",
    "win", "lose", "escape", "fly", "swim", "sing", "dance", "hide", "kiss", "love", "like", "you", "u", "me", "him", "her", "us", "them", "it", "the", "a", "an", "my", "your", "ur",
    "our", "his", "their", "this", "that", "those", "these", NULL };
static const char *const FOR_NEXT[] = { "me", "you", "u", "ya", "him", "her", "us", "them", "it", "the", "a", "an", "my", "your", "ur", "our", "his", "their", "this", "that",
    "now", "once", "real", "sure", "ever", "good", "free", "fun", "life", NULL };
static const char *const TOO_PREV[] = { "me", "u", "you", "ya", "him", "her", "us", "them", "it", "i", "we", "they", "he", "she", "same", "so", NULL };
static int wlist(const char *w, int n, const char *const *l) { for (; *l; l++) if (weq(w, n, *l)) return 1; return 0; }
static const char *numexpand(const char *in, int n, int i, int a, int b, int j) {
    int wl = b - a; const char *w = in + a;
    if (!NUMBERS_ON) return NULL;
    for (int k = 0; NLW[k][0]; k++) if (weq(w, wl, NLW[k][0])) return NLW[k][1];
    if (!(wl == 1 && (w[0] == '2' || w[0] == '4'))) return NULL;
    int p = i - 1; while (p >= 0 && in[p] == ' ') p--;                 /* the word before */
    int pe = (p >= 0 && is_aq(in[p])) ? p + 1 : 0, ps = pe; while (ps > 0 && is_aq(in[ps - 1])) ps--;
    int q = j; while (q < n && in[q] == ' ') q++;                      /* the word after */
    int qs = (q < n && is_aq(in[q])) ? q : -1, qe = qs; while (qe >= 0 && qe < n && is_aq(in[qe])) qe++;
    long nv;
    if (pe > ps && (is_d(in[ps]) || nw_kind(in + ps, pe - ps, &nv) != NWK_NONE || weq(in + ps, pe - ps, "im") || weq(in + ps, pe - ps, "i'm") || weq(in + ps, pe - ps, "am") ||
                    weq(in + ps, pe - ps, "age") || weq(in + ps, pe - ps, "aged") || weq(in + ps, pe - ps, "turned") || weq(in + ps, pe - ps, "turning") || weq(in + ps, pe - ps, "number"))) return NULL;
    if (qs >= 0 && is_d(in[qs])) return NULL;
    if (w[0] == '2') {
        if (qs >= 0 && wlist(in + qs, qe - qs, TO_NEXT)) return "to";
        if (qs < 0 && pe > ps && wlist(in + ps, pe - ps, TOO_PREV)) return "too";
    } else if (qs >= 0 && wlist(in + qs, qe - qs, FOR_NEXT)) return "for";
    return NULL;
}

int lang_clean(const char *text, char *out, int cap, int *map)      { return clean(text, out, cap, map, 1, NULL); }
int lang_clean_case(const char *text, char *out, int cap, int *map) { return clean(text, out, cap, map, 0, NULL); }
int lang_clean_info(const char *text, char *out, int cap, int *map, LangInfo *info) { return clean(text, out, cap, map, 1, info); }

int lang_clean_orig_len(const char *text, int at) {
    int cp; return decode((const unsigned char *)text + at, &cp);
}
