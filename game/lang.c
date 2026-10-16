/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * see lang.h for the big picture. the tables live in lang_pats.c and lines_*.c */
#include "lang.h"
#include "lang_data.h"
#include "lang_tables.h"
#include "lang_voice.h"
#include "lang_clean.h"
#include "lang_spell.h"
#include "lang_lemma.h"
#include "lang_clause.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- names ---------- */
/* the 46 original intents, then the 334 of lang_intents.h (chunk 12) */
static const char *const INTENT_NAME[I_COUNT] = {
    "greet", "bye", "yes", "no", "ok", "thanks", "sorry", "howru", "whoru", "where",
    "whoami", "mission", "why", "how", "what", "help", "insult", "praise", "love", "worship",
    "joke", "scared", "confused", "angry", "sad", "intro", "repeat", "sky", "music", "hole",
    "secret", "life", "wait", "please", "agree", "disagree", "dontcare", "demand", "callout", "age",
    "real", "address", "selfharm", "alex", "pos", "neg",
#define X(id, name, label) [I_##id] = name,
    INTENT_LIST(X)
#undef X
};
static const char *const INTENT_LABEL[I_COUNT] = {
    "HELLO", "BYE", "YES", "NO", "OK", "THANKS", "SORRY", "HOW ARE YOU", "WHO ARE YOU", "WHERE",
    "WHO AM I", "MISSION", "WHY", "HOW", "WHAT", "HELP", "INSULT", "PRAISE", "LOVE", "WORSHIP",
    "JOKE", "FEAR", "CONFUSION", "ANGER", "SADNESS", "INTRO", "REPEAT", "SKY", "MUSIC", "HOLE",
    "SECRET", "LIFE", "WAIT", "PLEASE", "AGREE", "DISAGREE", "DONT CARE", "DEMAND", "CALL OUT", "AGE",
    "REALITY", "NAME", "CONCERN", "ALEX", "GOOD", "BAD",
#define X(id, name, label) [I_##id] = label,
    INTENT_LIST(X)
#undef X
};

const char *lang_intent_name(int in) { return in >= 0 && in < I_COUNT ? INTENT_NAME[in] : NULL; }
const char *lang_intent_label(int in) { return in >= 0 && in < I_COUNT ? INTENT_LABEL[in] : NULL; }
int lang_intent_by_name(const char *name) { for (int i = 0; i < I_COUNT; i++) if (!strcmp(INTENT_NAME[i], name)) return i; return -1; }

/* INTENT -> REPLY KEY. by default an intent is answered by r.<name>. a row here sends a NAME to another key, so merged near-twins (several names, one
 * answer) need only one set of lines: { "greet_slang", "r.greet" }. the group chunks fill it (the first chunk of each group settles its merges).
 * the lint (langcheck --intents) reports a name with neither lines nor a mapping, and a mapping to a key that has no lines. the last row is the end mark. */
typedef struct { const char *name, *key; } KeyMap;
static const KeyMap KEYMAP[] = {
    { NULL, NULL }
};
int lang_keymap_count(void) { int n = 0; while (KEYMAP[n].name) n++; return n; }
int lang_keymap_row(int i, const char **name, const char **key) {
    if (i < 0 || i >= lang_keymap_count()) return 0;
    *name = KEYMAP[i].name; *key = KEYMAP[i].key; return 1;
}
static const KeyMap *test_map[8]; static KeyMap test_rows[8]; static int ntest;
int lang_keymap_test(const char *name, const char *key) {
    if (ntest >= 8 || lang_intent_by_name(name) < 0) return -1;
    test_rows[ntest].name = name; test_rows[ntest].key = key; test_map[ntest] = &test_rows[ntest]; ntest++;
    return 0;
}
void lang_keymap_clear_test(void) { ntest = 0; }
const char *lang_intent_key(int in, char *buf, int cap) {
    const char *n = lang_intent_name(in);
    if (cap <= 0) return "";
    if (!n) { buf[0] = 0; return buf; }
    for (int i = ntest - 1; i >= 0; i--) if (!strcmp(test_map[i]->name, n)) { snprintf(buf, (size_t)cap, "%s", test_map[i]->key); return buf; }
    for (const KeyMap *k = KEYMAP; k->name; k++) if (!strcmp(k->name, n)) { snprintf(buf, (size_t)cap, "%s", k->key); return buf; }
    snprintf(buf, (size_t)cap, "r.%s", n);
    return buf;
}

/* lower number = answered first. 99 = only ever answered when nothing better was said */
static const unsigned char RANK[I_COUNT] = {     /* chunk 12: an intent after I_NEG with a 0 here has the default rank 20 (rank_of); the group chunks fill their rows */
    /* greet */ 8, /* bye */ 9, /* yes */ 99, /* no */ 99, /* ok */ 99, /* thanks */ 7, /* sorry */ 3, /* howru */ 12, /* whoru */ 11, /* where */ 13,
    /* whoami */ 14, /* mission */ 15, /* why */ 99, /* how */ 99, /* what */ 99, /* help */ 25, /* insult */ 1, /* praise */ 6, /* love */ 4, /* worship */ 5,
    /* joke */ 24, /* scared */ 26, /* confused */ 29, /* angry */ 28, /* sad */ 27, /* intro */ 10, /* repeat */ 34, /* sky */ 21, /* music */ 22, /* hole */ 16,
    /* secret */ 17, /* life */ 18, /* wait */ 99, /* please */ 99, /* agree */ 33, /* disagree */ 32, /* dontcare */ 31, /* demand */ 30, /* callout */ 2, /* age */ 19,
    /* real */ 20, /* address */ 99, /* selfharm */ 0, /* alex */ 23, /* pos */ 99, /* neg */ 99
};

#define RANK_DEFAULT 20
static int rank_of(int in) { return in >= I_OLD_COUNT && RANK[in] == 0 ? RANK_DEFAULT : RANK[in]; }

/* WHAT MOVES THE MOOD (chunk 9): one row per intent, easy to tune. a row is what ONE hearing of the intent does:
 *   mood, cha  : the warm..hostile mood and the player's charisma (as before)
 *   trust fear energy amuse grief : the mood vector (-100..100). a value of 40 or more picks a mood (see vector_mood)
 *   flip       : a HARD FLIP, a mood that is set at once whatever the numbers say (see lang_apply)
 * positive mood, trust and amuse shrink with the npc's toxic (toxic npcs warm up slowly). a part of the vector that nothing pushed this
 * turn drifts back towards 0 by 1-2 points. */
enum { FL_NONE, FL_JOKE, FL_SORRY, FL_DEATH, FL_THREAT, FL_FLATTER, FL_INSULT };
typedef struct { signed char mood, cha, trust, fear, energy, amuse, grief; unsigned char flip; } Effect;
static const Effect EFFECT[I_COUNT] = {
    /*                      mood cha trust fear energy amuse grief flip */
    [I_GREET]    = {          2,  0,   1,   0,   0,    0,   0, FL_NONE },
    [I_THANKS]   = {          4,  1,   3,   0,   0,    1,   0, FL_NONE },
    [I_SORRY]    = {          6,  0,   3,  -3,   0,    0,   0, FL_SORRY },
    [I_PRAISE]   = {          8,  2,   2,   0,   2,    2,   0, FL_FLATTER },
    [I_LOVE]     = {          6,  1,   3,   0,   2,    2,   0, FL_FLATTER },
    [I_WORSHIP]  = {         10,  2,   2,   0,   3,    0,   0, FL_FLATTER },
    [I_JOKE]     = {          3,  1,   0,   0,   2,   15,   0, FL_JOKE },
    [I_INSULT]   = {        -14, -2,  -8,   0,   3,   -6,   0, FL_INSULT },
    [I_CALLOUT]  = {         -3,  0,  -3,   0,   2,    0,   0, FL_NONE },
    [I_DEMAND]   = {         -5, -1,  -4,   0,   2,    0,   0, FL_NONE },
    [I_DONTCARE] = {         -8, -1,  -4,   0,  -4,   -3,   0, FL_NONE },
    [I_DISAGREE] = {         -4,  0,  -1,   0,   0,    0,   0, FL_NONE },
    [I_AGREE]    = {          4,  1,   2,   0,   0,    0,   0, FL_NONE },
    [I_SCARED]   = {          0, -1,   1,   4,   0,    0,   0, FL_NONE },
    [I_SAD]      = {          0, -1,   0,   0,  -2,    0,   8, FL_NONE },
    [I_ANGRY]    = {         -4, -1,  -2,   3,   4,    0,   0, FL_NONE },
    [I_PLEASE]   = {          3,  1,   1,   0,   0,    0,   0, FL_NONE },
    [I_INTRO]    = {          2,  0,   1,   0,   0,    0,   0, FL_NONE },
    [I_HOWRU]    = {          2,  1,   1,   0,   0,    1,   0, FL_NONE },
    [I_ADDRESS]  = {          1,  0,   1,   0,   0,    0,   0, FL_NONE },
    [I_LIFE]     = {          0,  0,   0,   0,   0,    0,   6, FL_DEATH },   /* death is most of what I_LIFE hears: a flip to SOLEMN */
    [I_CONFUSED] = {          0,  0,   0,   0,  -2,    0,   0, FL_NONE },
    [I_REPEAT]   = {          0,  0,   0,   0,  -2,   -1,   0, FL_NONE },
    [I_HELP]     = {          0,  0,   2,   0,   0,    0,   0, FL_NONE },
};

/* ---------- random ---------- */
static uint32_t rs = 0x9E3779B9u;
void lang_seed(unsigned s) { rs = s ? s : 0x9E3779B9u; voice_seed(s); }
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static int rn(int n) { return n > 0 ? (int)(rnd() % (uint32_t)n) : 0; }

/* ---------- player state ---------- */
static char player_name[24] = "Vas";
static int  charisma = 50;
void lang_set_player(const char *n) { if (n) snprintf(player_name, sizeof player_name, "%s", n); }
int  lang_charisma(void) { return charisma; }
void lang_add_charisma(int d) { charisma += d; if (charisma < 0) charisma = 0; if (charisma > 100) charisma = 100; }

/* ---------- wildcard matching ---------- */
static int is_word(int c) { return isalnum(c) || c == '\''; }
static int is_stop(int c) { return c == '.' || c == '!' || c == '?' || c == ',' || c == 0; }

static int at_boundary(const char *base, const char *s) {
    if (s == base || !*s) return 1;
    return !(is_word((unsigned char)s[-1]) && is_word((unsigned char)*s));
}

/* longest end of text that pattern p can reach from s (written to *best, which starts at s).
 *  *  after a space (or on its own): runs across words until the sentence ends ("my name is *")
 *  *  glued to a letter: only finishes the current word ("thank*" -> thanks)
 *  $  matches only at the end of the sentence ("i do$" is yes, "i do not love you" is not)  */
static void mt(const char *p, const char *s, const char *base, const char **best, const char *pat0) {
    for (;;) {
        if (!*p) { if (s > *best && at_boundary(base, s)) *best = s; return; }
        if (*p == '*') {
            int cross = p == pat0 || p[-1] == ' ';
            p++;
            for (const char *q = s;; q++) {
                mt(p, q, base, best, pat0);
                if (is_stop((unsigned char)*q) || (!cross && !is_word((unsigned char)*q))) break;
            }
            return;
        }
        if (*p == '$') { if (is_stop((unsigned char)*s)) { if (s > *best) *best = s; } return; }
        if (*p == '?') { if (is_stop((unsigned char)*s)) return; p++; s++; continue; }
        if (*s != *p) return;
        p++; s++;
    }
}

static int stars(const char *p) { int n = 0; for (; *p; p++) if (*p == '*') n++; return n; }
static int prefix_len(const char *p);
static int prefix_len(const char *p) { int n = 0; while (p[n] && p[n] != '*' && p[n] != '$') n++; return n; }

typedef struct { int start, end, intent, prefix; } Span;
#define MAX_TEXT 400        /* chunk 5: was 200 */
#define MAX_SPANS 80        /* chunk 5: was 40 */

static int scan(const char *lc, int len, Span *sp, int maxsp) {
    int n = 0, i = 0;
    while (i < len && n < maxsp) {
        if (!is_word((unsigned char)lc[i]) || (i > 0 && is_word((unsigned char)lc[i - 1]))) { i++; continue; }
        const char *best = lc + i; int bk = -1, bstars = 99;
        PatCur cur; lang_pat_cursor(&cur, lc + i);               /* chunk 6: only the patterns that can start here, in table order */
        for (int k; (k = lang_pat_next(&cur)) >= 0;) {
            const char *p = lang_pat(k)->pat;
            if (*p != '*' && *p != '?' && *p != lc[i]) continue;
            const char *e = lc + i;
            mt(p, lc + i, lc, &e, p);
            if (e == lc + i) continue;
            int st = stars(p);
            if (e > best || (e == best && bk >= 0 && st < bstars)) { best = e; bk = k; bstars = st; }
        }
        if (bk >= 0) {
            sp[n].start = i; sp[n].end = (int)(best - lc); sp[n].intent = lang_pat(bk)->intent; sp[n].prefix = prefix_len(lang_pat(bk)->pat);
            n++; i = (int)(best - lc);
        } else i++;
    }
    return n;
}


/* safety words are checked everywhere in the text, even inside a longer phrase ("i want to kill myself") */
static int safety_scan(const char *lc, int len, Span *out, int maxo) {
    int n = 0;
    const int *ids; int nids = lang_pat_safety(&ids);          /* chunk 6: only the I_SELFHARM patterns */
    for (int i = 0; i < len && n < maxo; i++) {
        if (!is_word((unsigned char)lc[i]) || (i > 0 && is_word((unsigned char)lc[i - 1]))) continue;
        for (int j = 0; j < nids; j++) {
            const char *p = lang_pat(ids[j])->pat; const char *e = lc + i;
            if (*p != lc[i]) continue;
            mt(p, lc + i, lc, &e, p);
            if (e > lc + i) { out[n].start = i; out[n].end = (int)(e - lc); out[n].intent = I_SELFHARM; out[n].prefix = 0; n++; break; }
        }
    }
    return n;
}

/* chunk 25: negation is scoped by clause (lang_clause.c): lang_neg_count counts the negators in the 4 words before a match, inside its clause. an odd
 * number flips the meaning. (the old negated(): 3 words before, no clause, is gone; a one clause text takes the same path) */
static int flip(int intent) {
    switch (intent) {
    case I_PRAISE: case I_LOVE: case I_WORSHIP: return I_INSULT;
    case I_INSULT: return I_PRAISE;
    case I_AGREE: return I_DISAGREE;
    case I_DISAGREE: return I_AGREE;
    case I_POS: return I_NEG;
    case I_NEG: return I_POS;
    case I_SCARED: case I_SAD: case I_ANGRY: return I_OK;
    default: return intent;
    }
}

static int pol_of(int in) {
    switch (in) {
    case I_POS: case I_PRAISE: case I_LOVE: case I_WORSHIP: case I_THANKS: return 1;
    case I_NEG: case I_INSULT: case I_ANGRY: case I_SAD: case I_SCARED: return -1;
    default: return 0;
    }
}

/* chunk 15: the text goes through lang_clean first (accents folded, odd spaces and invisible characters gone, lower case);
 * chunk 17: then through lang_clean_words (elongation, leet, spaced letters, repeated punctuation). map[i] = the byte of the ORIGINAL
 * text that produced lc[i] (map may be NULL); src[i] = the index in the part-1 text (for the case twin; may be NULL); info may be NULL */
static int prepare3(const char *text, char *lc, int *map, int *src, char *t1, LangInfo *info) {
    lang_init();                                         /* plugs the known-word hook into the clean-up layer (cheap when nothing changed) */
    char tmp[MAX_TEXT]; int m1[MAX_TEXT];
    char *c1 = t1 ? t1 : tmp;
    LangInfo own; LangInfo *f = info ? info : &own;
    lang_clean_info(text, c1, MAX_TEXT, m1, f);
    return lang_clean_words(c1, m1, lc, MAX_TEXT, map, src, f);
}
static int prepare(const char *text, char *lc, int *map) { return prepare3(text, lc, map, NULL, NULL, NULL); }

/* one text's matches into the Heard. primary = 1 the text as typed (counts positive / negative and takes the name capture); 0 a variant: the expanded
 * variant (chunk 18) adds only what the text as typed did not already say; 2 the lemma variant (chunk 24): like 0, but it never captures a name
 * (a lemma is not what the player typed). srcmap[i] = the index in the part 1 text t1 of lc[i], for the case twin */
typedef struct { const char *text, *t1; char cs[MAX_TEXT]; int have; } Case;
static void absorb(Heard *h, const char *lc, const Span *sp, int ns, const int *srcmap, Case *cs, int primary) {
    Clause cl[CLAUSE_MAX]; int ncl = lang_clauses(lc, (int)strlen(lc), cl, CLAUSE_MAX);
    int cpos[CLAUSE_MAX] = {0}, cneg[CLAUSE_MAX] = {0}, cpol[CLAUSE_MAX] = {0}, cflip[CLAUSE_MAX] = {0}, cn[CLAUSE_MAX] = {0};
    int cint[CLAUSE_MAX][4];
    for (int k = 0; k < ns; k++) {
        int in = sp[k].intent, ci = 0;
        while (ci + 1 < ncl && sp[k].start >= cl[ci].b) ci++;
        if (ncl > 0 && (lang_neg_count(lc, sp[k].start, cl[ci].a) & 1)) { int f = flip(in); if (f != in) cflip[ci] = 1; in = f; }
        if (primary == 1 || !heard_has(h, in)) { if (in == I_POS) { h->positive++; cpos[ci]++; } else if (in == I_NEG) { h->negative++; cneg[ci]++; } }
        if (primary == 1 && ncl > 0) { cpol[ci] += pol_of(in); if (cn[ci] < 4) cint[ci][cn[ci]++] = in; }
        if (in == I_INTRO && !h->capture[0] && primary != 2) {
            int a = sp[k].start + sp[k].prefix, b = sp[k].end;
            while (a < b && lc[a] == ' ') a++;
            int n = b - a; if (n > 22) n = 22;
            if (n > 0) {                                  /* the case kept: lc[i] came from t1[srcmap[i]], and lang_clean_case has the same places as t1 */
                if (!cs->have) { lang_clean_case(cs->text, cs->cs, MAX_TEXT, NULL); cs->have = 1; }
                for (int q = 0; q < n; q++) { int j = srcmap[a + q]; h->capture[q] = (lc[a + q] == cs->t1[j]) ? cs->cs[j] : lc[a + q]; }
                h->capture[n] = 0;
            }
        }
        if (!heard_has(h, in) && h->n < HEARD_ORDER) { heard_set(h, in); h->order[h->n++] = in; }
    }
    if (primary != 1 || ncl <= 0) return;
    /* chunk 25: the clauses of the text as typed. contrast: what follows a but wins, so the feeling counts of the clauses before it are dropped */
    h->nclauses = (uint8_t)ncl;
    int last = -1, strongest = 0;
    for (int k = 0; k < ncl; k++) {
        HeardClause *c = &h->clauses[k];
        c->a = (int16_t)cl[k].a; c->b = (int16_t)cl[k].b; c->contrast = (uint8_t)cl[k].contrast; c->n = (uint8_t)cn[k];
        for (int q = 0; q < cn[k]; q++) c->intent[q] = (int16_t)cint[k][q];
        c->polarity = (int8_t)(cpol[k] > 2 ? 2 : cpol[k] < -2 ? -2 : cpol[k]);
        int in = cn[k] ? 1 + cl[k].boost : 0;
        if (cn[k] && cflip[k] && c->polarity > 0 && in > 1) in = 1;                 /* not bad is a mild good */
        c->intensity = (uint8_t)(in < 0 ? 0 : in > 3 ? 3 : in);
        if (c->polarity) last = k;
        if (c->intensity > strongest) strongest = c->intensity;
    }
    h->intensity = (uint8_t)(last >= 0 ? h->clauses[last].intensity : strongest);
    int from = -1;
    for (int k = 0; k < ncl; k++) if (cl[k].contrast && cpos[k] + cneg[k] > 0) from = k;
    if (from > 0) {
        int p = 0, n = 0;
        for (int k = from; k < ncl; k++) { p += cpos[k]; n += cneg[k]; }
        h->positive = p; h->negative = n;
    }
}

/* chunk 24: the lemma variant of one text (every inflected word replaced by its lemma) */
typedef struct { char t[MAX_TEXT]; int src[MAX_TEXT]; uint8_t rep[MAX_TEXT], cov[MAX_TEXT]; int len, n; } LemVar;
static int lemma_variant(LemVar *v, const char *text) {
    LemmaInfo li;
    v->len = lang_lemma_text(text, v->t, MAX_TEXT, v->src, v->rep, &li);
    v->n = li.nlemma;
    if (v->n > 0) {                                           /* cov: the bytes of the base text that a phrase of the base text already covers */
        Span sp[MAX_SPANS]; int ns = scan(text, (int)strlen(text), sp, MAX_SPANS);
        memset(v->cov, 0, sizeof v->cov);
        for (int k = 0; k < ns; k++) for (int i = sp[k].start; i < sp[k].end && i < MAX_TEXT; i++) v->cov[i] = 1;
    }
    return v->n > 0;
}
/* only the spans that cover a replaced word the base text did not already cover are the lemma variant's own: the others are what the base text
 * said already, and a lemma can break a longer phrase of the base text ("who are you" -> "who be you" would hear the lone word who,
 * "no thanks" -> "no thank" would hear a thanks) */
static int keep_replaced(const LemVar *v, Span *sp, int n) {
    int m = 0;
    for (int k = 0; k < n; k++) for (int i = sp[k].start; i < sp[k].end; i++) if (v->rep[i] && !v->cov[v->src[i]]) { sp[m++] = sp[k]; break; }
    return m;
}

void lang_parse(const char *text, Heard *h) {
    memset(h, 0, sizeof *h);
    char lc[MAX_TEXT], t1[MAX_TEXT]; int src[MAX_TEXT], map[MAX_TEXT]; LangInfo info;
    int len = prepare3(text, lc, map, src, t1, &info);
    lang_clean_numbers(text, lc, map, &info);                  /* chunk 19: numbers, ages, times, ordinals */
    char lx[MAX_TEXT]; int xs[MAX_TEXT], xsrc[MAX_TEXT];       /* chunk 18: the expanded variant (u -> you, idk -> i do not know, dont -> do not) */
    int lenx = lang_clean_expand(lc, NULL, lx, MAX_TEXT, NULL, xs, &info);
    int has_x = info.nexpand > 0;
    h->flags = info.flags; h->noise = (uint8_t)info.noise; h->excl = (uint8_t)info.excl; h->quest = (uint8_t)info.quest; h->laugh = (uint8_t)(info.laugh > 255 ? 255 : info.laugh);
    h->stretch = (uint8_t)(info.stretch > 255 ? 255 : info.stretch);
    h->nnum = (uint8_t)(info.nnum > HEARD_NUM ? HEARD_NUM : info.nnum);
    for (int k = 0; k < info.nnum; k++) { if (k < HEARD_NUM) h->num[k] = info.num[k]; if (info.num[k].kind == NK_AGE && !h->age) h->age = (int16_t)info.num[k].value; }
    for (int k = 0; k < TK_COUNT; k++) h->tok[k] = (uint8_t)(info.count[k] > 255 ? 255 : info.count[k]);
    Case cs; cs.text = text; cs.t1 = t1; cs.have = 0;
    Span sp[MAX_SPANS];
    int ns = scan(lc, len, sp, MAX_SPANS);
    /* chunk 21: the spelled variant (hwllo -> hello) and its expansion; the text as typed is never lost */
    char ls[MAX_TEXT], lsx[MAX_TEXT]; int ss[MAX_TEXT], ssrc[MAX_TEXT], sxs[MAX_TEXT], sxsrc[MAX_TEXT]; SpellInfo spi;
    int lens = lang_spell_text(text, lc, map, (info.flags & LF_SHOUT) != 0, ls, MAX_TEXT, NULL, ss, &spi);
    int has_s = spi.nfix > 0, lensx = 0, has_sx = 0;
    if (has_s) {
        info.flags |= LF_SPELL;
        h->nspell = (uint8_t)(spi.nfix > 255 ? 255 : spi.nfix); h->spell_conf = (uint8_t)spi.min_conf;
        LangInfo se; memset(&se, 0, sizeof se);
        lensx = lang_clean_expand(ls, NULL, lsx, MAX_TEXT, NULL, sxs, &se);
        has_sx = se.nexpand > 0;
        h->flags = info.flags;
    }
    if (has_x) for (int i = 0; i < lenx; i++) xsrc[i] = src[xs[i]];
    if (has_s) { for (int i = 0; i < lens; i++) ssrc[i] = src[ss[i]]; if (has_sx) for (int i = 0; i < lensx; i++) sxsrc[i] = ssrc[sxs[i]]; }
    LemVar L[4]; const int *Ls[4]; int nl = 0;                 /* chunk 24: the lemma variants (loved -> love) of the typed text and of each other variant */
    if (lemma_variant(&L[nl], lc)) { h->nlemma = (uint8_t)(L[nl].n > 255 ? 255 : L[nl].n); h->flags |= LF_LEMMA; Ls[nl++] = src; }
    if (has_x && lemma_variant(&L[nl], lx)) Ls[nl++] = xsrc;
    if (has_s && lemma_variant(&L[nl], ls)) Ls[nl++] = ssrc;
    if (has_sx && lemma_variant(&L[nl], lsx)) Ls[nl++] = sxsrc;
    Span sf[4]; int nsf = safety_scan(lc, len, sf, 4);
    if (has_x) {                                               /* the safety words are looked for in every variant */
        Span sx[4]; int nsx = safety_scan(lx, lenx, sx, 4);
        if (nsx) nsf = nsf ? nsf : nsx;
    }
    if (has_s) {
        Span sx[4]; int nsx = safety_scan(ls, lens, sx, 4);
        if (nsx) nsf = nsf ? nsf : nsx;
        if (has_sx) { nsx = safety_scan(lsx, lensx, sx, 4); if (nsx) nsf = nsf ? nsf : nsx; }
    }
    for (int k = 0; k < nl; k++) { Span sx[4]; int nsx = safety_scan(L[k].t, L[k].len, sx, 4); if (nsx) nsf = nsf ? nsf : nsx; }
    if (nsf) { heard_set(h, I_SELFHARM); h->order[h->n++] = I_SELFHARM; }
    absorb(h, lc, sp, ns, src, &cs, 1);
    if (has_x) {
        Span spx[MAX_SPANS];
        int nx = scan(lx, lenx, spx, MAX_SPANS);
        absorb(h, lx, spx, nx, xsrc, &cs, 0);
    }
    if (has_s) {
        Span sps[MAX_SPANS];
        int nq = scan(ls, lens, sps, MAX_SPANS);
        absorb(h, ls, sps, nq, ssrc, &cs, 0);
        if (has_sx) {
            Span spq[MAX_SPANS];
            int nr = scan(lsx, lensx, spq, MAX_SPANS);
            absorb(h, lsx, spq, nr, sxsrc, &cs, 0);
        }
    }
    for (int k = 0; k < nl; k++) {                             /* chunk 24: what only a lemma variant heard (never a name, never a count) */
        Span spl[MAX_SPANS]; int lsrc[MAX_TEXT];
        int nn = keep_replaced(&L[k], spl, scan(L[k].t, L[k].len, spl, MAX_SPANS));
        for (int i = 0; i < L[k].len; i++) lsrc[i] = Ls[k][L[k].src[i]];
        absorb(h, L[k].t, spl, nn, lsrc, &cs, 2);
    }
    int w = 0, inw = 0;
    for (int i = 0; i < len; i++) { int c = is_word((unsigned char)lc[i]); if (c && !inw) w++; inw = c; }
    h->words = w;
    static const char *const QW[] = { "what", "why", "how", "who", "where", "when", "can", "could", "do", "does", "is", "are",
        "will", "would", "should", "may", "which", "did", "have", "has", NULL };
    int q = 0;
    for (int i = 0; i < len; i++) if (lc[i] == '?') q = 1;
    if (!q) {
        char fw[12]; int n = 0;
        while (n < 11 && lc[n] && is_word((unsigned char)lc[n])) { fw[n] = lc[n]; n++; }
        fw[n] = 0;
        for (int k = 0; QW[k]; k++) if (!strcmp(fw, QW[k])) { q = 1; break; }
    }
    h->is_question = q;
}

/* marks the ORIGINAL bytes of every match (chunk 15: through the char map, so folded accents, collapsed spaces and so on still
 * light up the right characters); a phrase marks the whole stretch from its first to its last original byte */
static void mark(const char *text, const int *map, int total, int start, int end, uint8_t *mask) {
    if (end <= start) return;
    int a = map[start], b = map[end - 1] + lang_clean_orig_len(text, map[end - 1]);
    if (b > total) b = total;
    for (int i = a; i < b; i++) mask[i] = 1;
}

int lang_highlight(const char *text, uint8_t *mask) {
    char lc[MAX_TEXT]; int map[MAX_TEXT];
    int len = prepare(text, lc, map);
    int total = (int)strlen(text);
    for (int i = 0; i <= total; i++) mask[i] = 0;
    Span sp[MAX_SPANS];
    int ns = scan(lc, len, sp, MAX_SPANS);
    for (int k = 0; k < ns; k++) mark(text, map, total, sp[k].start, sp[k].end, mask);
    Span sf[4]; int nsf = safety_scan(lc, len, sf, 4);
    for (int k = 0; k < nsf; k++) mark(text, map, total, sf[k].start, sf[k].end, mask);
    char lx[MAX_TEXT]; int mx[MAX_TEXT]; LangInfo ex;           /* chunk 18: what only the expanded variant heard lights up too (the whole typed token) */
    memset(&ex, 0, sizeof ex);
    int lenx = lang_clean_expand(lc, map, lx, MAX_TEXT, mx, NULL, &ex);
    if (ex.nexpand) {
        Span spx[MAX_SPANS];
        int nx = scan(lx, lenx, spx, MAX_SPANS);
        for (int k = 0; k < nx; k++) mark(text, mx, total, spx[k].start, spx[k].end, mask);
        Span sx[4]; int nsx = safety_scan(lx, lenx, sx, 4);
        for (int k = 0; k < nsx; k++) mark(text, mx, total, sx[k].start, sx[k].end, mask);
        ns += nx; nsf += nsx;
    }
    char ls[MAX_TEXT], lsx[MAX_TEXT]; int ms[MAX_TEXT], msx[MAX_TEXT]; SpellInfo spi;      /* chunk 21: what only the spelled variant heard lights up the whole typed word */
    int lens = lang_spell_text(text, lc, map, 0, ls, MAX_TEXT, ms, NULL, &spi), lensx = 0, has_sx = 0;
    if (spi.nfix) {
        Span sps[MAX_SPANS];
        int nq = scan(ls, lens, sps, MAX_SPANS);
        for (int k = 0; k < nq; k++) mark(text, ms, total, sps[k].start, sps[k].end, mask);
        Span sq[4]; int nsq = safety_scan(ls, lens, sq, 4);
        for (int k = 0; k < nsq; k++) mark(text, ms, total, sq[k].start, sq[k].end, mask);
        ns += nq; nsf += nsq;
        LangInfo se; memset(&se, 0, sizeof se);
        lensx = lang_clean_expand(ls, ms, lsx, MAX_TEXT, msx, NULL, &se);
        has_sx = se.nexpand > 0;
        if (se.nexpand) {
            Span spq[MAX_SPANS];
            int nr = scan(lsx, lensx, spq, MAX_SPANS);
            for (int k = 0; k < nr; k++) mark(text, msx, total, spq[k].start, spq[k].end, mask);
            Span sr[4]; int nsr = safety_scan(lsx, lensx, sr, 4);
            for (int k = 0; k < nsr; k++) mark(text, msx, total, sr[k].start, sr[k].end, mask);
            ns += nr; nsf += nsr;
        }
    }
    {   /* chunk 24: what only a lemma variant heard lights up the whole typed word too */
        const char *base[4] = { lc, lx, ls, lsx }; const int *bmap[4] = { map, mx, ms, msx };
        int use[4] = { 1, ex.nexpand > 0, spi.nfix > 0, spi.nfix > 0 && has_sx };
        for (int b = 0; b < 4; b++) {
            LemVar v; int lm[MAX_TEXT];
            if (!use[b] || !lemma_variant(&v, base[b])) continue;
            for (int i = 0; i < v.len; i++) lm[i] = bmap[b][v.src[i]];
            Span spl[MAX_SPANS];
            int nn = keep_replaced(&v, spl, scan(v.t, v.len, spl, MAX_SPANS));
            for (int k = 0; k < nn; k++) mark(text, lm, total, spl[k].start, spl[k].end, mask);
            Span sl[4]; int nsl = safety_scan(v.t, v.len, sl, 4);
            for (int k = 0; k < nsl; k++) mark(text, lm, total, sl[k].start, sl[k].end, mask);
            ns += nn; nsf += nsl;
        }
    }
    return ns + nsf;
}

void lang_heard_label(const Heard *h, char *out, int cap) {
    int shown = 0, o = snprintf(out, (size_t)cap, "HEARD -");
    for (int i = 0; i < h->n && shown < 3; i++) {
        o += snprintf(out + o, (size_t)(cap - o > 0 ? cap - o : 0), "%s %s", shown ? "," : "", INTENT_LABEL[h->order[i]]);
        shown++;
        if (o >= cap) break;
    }
    if (!shown) snprintf(out, (size_t)cap, "NO KNOWN WORDS - SHE WILL IMPROVISE");
}

/* ---------- tone ---------- */
static int tone_score(const Persona *ps) {
    return ps->mood + (int)((charisma - 50) * 0.6f) + (int)((ps->warmth - 50) * 0.5f) - (int)(ps->toxic * 0.5f);
}
static Tone tone_from(int score) {
    if (score >= 20) return TONE_WARM;
    if (score >= -25) return TONE_NEUTRAL;
    if (score >= -62) return TONE_COLD;
    return TONE_HOSTILE;
}
/* the plain mood: what the old four moods gave, unchanged (the same random draw) */
static Tone plain_mood(const Persona *ps) { return tone_from(tone_score(ps) + rn(15) - 7); }

Tone lang_mood_now(const Persona *ps) { return ps->hold ? (Tone)ps->held : plain_mood(ps); }

/* ---------- the mood state (chunk 8) ---------- */
#define OVERRIDE_AT 40          /* a vector value this strong picks a mood. `lean` for that mood makes it 10 sooner */
#define LEAN_DRIFT 20           /* % per turn that a character with a lean drifts to one of its moods when nothing else moves it */
#define SULK_DROP 10            /* mood lost in one turn that can make a character with a lean for SULKY sulk */

/* a mood that sticks 2-4 turns, whatever picked it */
static void stick(Persona *ps, Tone m) { ps->held = (int8_t)m; ps->hold = (uint8_t)(2 + rn(3)); }
static int lean_has(const Persona *ps, int t) { return (int)((ps->lean >> t) & 1u); }

/* the strongest push on the mood right now (a Tone), or -1. base = the plain mood without the random part; drop = mood lost this turn */
static int vector_mood(const Persona *ps, Tone base, int drop) {
    int best = -1, bests = 0;
    #define PUSH(mood, excess) do { int m_ = (mood), e_ = (excess) - (OVERRIDE_AT - (lean_has(ps, m_) ? 10 : 0)) + 1; if (e_ > bests) { best = m_; bests = e_; } } while (0)
    int am = ps->mood < 0 ? -ps->mood : ps->mood;
    if (ps->fear > 0)   PUSH(ps->fear >= 70 ? TONE_AFRAID : TONE_WORRIED, ps->fear);
    if (ps->amuse > 0)  PUSH(TONE_PLAYFUL, ps->amuse);
    if (ps->energy <= -1) PUSH(am < 20 ? TONE_BORED : TONE_WEARY, -ps->energy);
    if (ps->energy >= 1)  PUSH(ps->energy >= 70 && (ps->fear >= 20 || ps->mood < -25) ? TONE_MANIC : TONE_EXCITED, ps->energy);
    if (ps->grief > 0)  PUSH(ps->grief >= 70 ? TONE_HOLLOW : TONE_SOLEMN, ps->grief);
    if (ps->trust < 0 && base != TONE_HOSTILE) PUSH(TONE_SUSPICIOUS, -ps->trust);
    if (ps->trust > 0 && base == TONE_WARM)    PUSH(TONE_TENDER, ps->trust);
    if (drop >= SULK_DROP && lean_has(ps, TONE_SULKY)) PUSH(TONE_SULKY, 30 + (drop - SULK_DROP + 1) * 4);     /* the lean makes the bar 30: a bigger drop is a stronger push */
    #undef PUSH
    return best;
}

/* one turn of the mood state: count the sticking mood down, and when none is held pick the next one (the vector, else a drift to a
 * lean). draws nothing random unless something is picked, so a character with no vector and no lean plays as before */
static void mood_turn(Persona *ps, int drop) {
    if (ps->hold && --ps->hold) return;                 /* still sticking */
    Tone base = tone_from(tone_score(ps));
    int m = vector_mood(ps, base, drop);
    if (m < 0 && ps->lean && rn(100) < LEAN_DRIFT) {
        int ok[TONE_COUNT], n = 0;
        for (int t = 0; t < TONE_COUNT; t++) {
            if (!lean_has(ps, t) || t == (int)base) continue;
            if (base == TONE_WARM && ((1u << t) & RUDE)) continue;        /* a warm character does not drift to a rude mood, nor a hostile one to a nice one */
            if (base == TONE_HOSTILE && ((1u << t) & NICE)) continue;
            ok[n++] = t;
        }
        if (n) m = ok[rn(n)];
    }
    if (m < 0) return;
    stick(ps, (Tone)m);                                 /* 2 to 4 turns */
}

static int8_t *vec(Persona *ps, int i) { return i == 0 ? &ps->trust : i == 1 ? &ps->fear : i == 2 ? &ps->energy : i == 3 ? &ps->amuse : &ps->grief; }
static int clamp100(int v) { return v > 100 ? 100 : v < -100 ? -100 : v; }

/* every part of the vector that was not pushed this turn (d[i] == 0) drifts back towards 0 by 1-2 points. nothing is drawn if it is all 0 */
static void drift(Persona *ps, const int d[5]) {
    for (int i = 0; i < 5; i++) {
        int8_t *v = vec(ps, i);
        if (d[i] || !*v) continue;
        int step = 1 + rn(2);
        if (*v > 0) *v = (int8_t)(*v > step ? *v - step : 0); else *v = (int8_t)(*v < -step ? *v + step : 0);
    }
}

void lang_threat(Persona *ps) {
    if (ps->toxic < 40) { ps->fear = (int8_t)clamp100(ps->fear + 25); stick(ps, TONE_AFRAID); }
    else stick(ps, TONE_HOSTILE);
    ps->trust = (int8_t)clamp100(ps->trust - 15);
}

void lang_apply(Persona *ps, const Heard *h) {
    int before = ps->mood, was_sour = ps->sour, was_held = ps->hold ? ps->held : -1;
    int d[5] = {0}, joke = 0, sorry = 0, flatter = 0, insult = 0, death = 0, threat = 0;
    for (int i = 0; i < h->n; i++) {
        const Effect *e = &EFFECT[h->order[i]];
        int dm = e->mood, dt = e->trust, da = e->amuse;
        if (dm > 0) dm = dm * (100 - ps->toxic / 2) / 100;          /* toxic npcs warm up slowly */
        if (dt > 0) dt = dt * (100 - ps->toxic / 2) / 100;
        if (da > 0) da = da * (100 - ps->toxic / 2) / 100;
        ps->mood += dm;
        lang_add_charisma(e->cha);
        d[0] += dt; d[1] += e->fear; d[2] += e->energy; d[3] += da; d[4] += e->grief;
        switch (e->flip) {
            case FL_JOKE: joke = 1; break;      case FL_SORRY: sorry = 1; break;   case FL_FLATTER: flatter = 1; break;
            case FL_INSULT: insult = 1; break;  case FL_DEATH: death = 1; break;   case FL_THREAT: threat = 1; break;
            default: break;
        }
    }
    ps->mood = clamp100(ps->mood);
    for (int i = 0; i < 5; i++) { int8_t *v = vec(ps, i); *v = (int8_t)clamp100(*v + d[i]); }
    drift(ps, d);
    ps->quiet = 0;
    if (ps->sour) ps->sour--;
    if (insult) ps->sour = 3;                                       /* an apology within 3 turns of an insult softens it */
    mood_turn(ps, before - ps->mood);

    /* the hard flips: they replace whatever mood is held */
    ps->flat = flatter ? (uint8_t)(ps->flat + 1) : 0;
    if (ps->flat >= 3) { stick(ps, TONE_SUSPICIOUS); ps->trust = (int8_t)clamp100(ps->trust - 12); ps->flat = 0; }      /* flattery again and again: what do they want? */
    if (joke && (was_held == TONE_SOLEMN || was_held == TONE_HOLLOW)) {          /* a joke in a dark moment: it lands or it stings */
        if (lean_has(ps, TONE_SULKY)) stick(ps, TONE_SULKY);
        else if (lean_has(ps, TONE_PLAYFUL)) stick(ps, TONE_PLAYFUL);
    }
    if (sorry && was_sour) {                                        /* an apology after an insult softens one step */
        ps->mood = clamp100(ps->mood + 10);
        if (ps->hold && (ps->held == TONE_SULKY || ps->held == TONE_SUSPICIOUS || ps->held == TONE_SARCASTIC || ps->held == TONE_SMUG)) ps->hold = 0;
        ps->sour = 0;
    }
    if (death) stick(ps, TONE_SOLEMN);
    if (threat) lang_threat(ps);
}

void lang_idle_turn(Persona *ps) {
    int d[5] = {0};
    drift(ps, d);
    if (ps->quiet < 255) ps->quiet++;
    if (ps->sour) ps->sour--;
    mood_turn(ps, 0);
    if (ps->quiet >= 2 && !ps->hold) stick(ps, ps->warmth >= 50 ? TONE_WORRIED : TONE_BORED);      /* a long silence: they worry, or they are bored */
}

/* nothing understood and only a word or two: a character that leans to CURIOUS wonders, one that leans to BORED yawns. 1 if it stuck */
static int gibberish_flip(Persona *ps, const Heard *h) {
    if (ps->hold || h->words < 1 || h->words > 3 || h->is_question || h->positive || h->negative) return 0;
    if (lean_has(ps, TONE_CURIOUS)) { stick(ps, TONE_CURIOUS); return 1; }
    if (lean_has(ps, TONE_BORED)) { stick(ps, TONE_BORED); return 1; }
    return 0;
}

/* ---------- line picking ---------- */

/* the no-repeat memory (chunk 5: 24 -> 128 lines). how much of it counts depends on the key: a key with N rows
 * does not use its last min(N/2, 40) lines again, so a small key is never starved and a big one rarely repeats */
#define RECENT_MAX 128
#define WINDOW_MAX 40
static const Line *recent[RECENT_MAX];
static int nrecent;

static void remember(const Line *l) { recent[nrecent++ % RECENT_MAX] = l; }

/* was l one of the last `window` lines used from this key? */
static int was_recent(const Line *l, const char *key, int window) {
    int seen = 0, total = nrecent < RECENT_MAX ? nrecent : RECENT_MAX;
    for (int i = 1; i <= total && seen < window; i++) {
        const Line *e = recent[(nrecent - i) % RECENT_MAX];
        if (e == l) return 1;
        if (!strcmp(e->key, key)) seen++;
    }
    return 0;
}

/* candidate lists grow to the key's real row count (no cap: the lint fails a key over LANG_MAX_KEY_ROWS) */
static const Line **cand;
static int *wt, cand_cap;

/* pass 0: the persona's own voices. pass 1: PLAIN lines (a voice with nothing of its own). pass 2: any voice (chunk 10) */
static int voice_mask(const Persona *ps, int pass) { return pass == 0 ? (ps->style ? ps->style : PLAIN) : pass == 1 ? PLAIN : 0; }

static const Line *pick_exact(const Persona *ps, const char *key, int tone, int pass) {
    int n = 0, sum = 0;
    int mask = voice_mask(ps, pass), mine = ps->style ? ps->style : PLAIN, first = mine & -mine;
    const Line *const *rows; int nkey = lang_key_rows(key, &rows);     /* chunk 6: only this key's rows, in table order */
    for (int r = 0; r < nkey; r++) {
        const Line *l = rows[r];
        if (!(l->tones & (1u << tone))) continue;
        if (mask && !(l->styles & mask)) continue;
        if (n >= cand_cap) {
            int nc = cand_cap ? cand_cap * 2 : 128;
            const Line **c2 = realloc(cand, (size_t)nc * sizeof *cand);
            int *w2 = realloc(wt, (size_t)nc * sizeof *wt);
            if (c2) cand = c2;
            if (w2) wt = w2;
            if (!c2 || !w2) break;                 /* out of memory: pick from what we have */
            cand_cap = nc;
        }
        int w = 1;
        if (pass == 0 && l->styles != ALLS && (l->styles & first) && !(l->styles & ~mine)) w = 3;   /* made just for this persona's voices, first voice included */
        cand[n] = l; wt[n] = w; n++;
    }
    if (!n) return NULL;
    int window = nkey / 2 < WINDOW_MAX ? nkey / 2 : WINDOW_MAX;
    for (int i = 0; i < n; i++) { if (window > 0 && was_recent(cand[i], key, window)) wt[i] = 0; sum += wt[i]; }
    if (sum == 0) { const Line *l = cand[rn(n)]; return l; }   /* only repeats left: allow one */
    int r = rn(sum);
    for (int i = 0; i < n; i++) { r -= wt[i]; if (r < 0) return cand[i]; }
    return cand[0];
}

/* ---------- the 20 moods and their fallback chains (chunk 7) ----------
 * a mood with no line for a key tries its chain, then the nearest of the old four, then (second pass) any style. the old four are
 * their own chain, so a play-through with only those answers exactly as before. */
static const char *const TONE_NAME[TONE_COUNT] = {
    "warm", "neutral", "cold", "hostile", "playful", "sarcastic", "weary", "worried", "curious", "solemn",
    "smug", "tender", "sulky", "suspicious", "afraid", "hollow", "manic", "shy", "bored", "excited" };
static const char TONE_CODE[TONE_COUNT + 1] = "WNCHPSEOQLMTKUAZXYBV";
#define CH_END 255
static const unsigned char CHAIN[TONE_COUNT][6] = {
    [TONE_WARM] = { TONE_WARM, CH_END }, [TONE_NEUTRAL] = { TONE_NEUTRAL, CH_END },
    [TONE_COLD] = { TONE_COLD, CH_END }, [TONE_HOSTILE] = { TONE_HOSTILE, CH_END },
    [TONE_PLAYFUL]    = { TONE_PLAYFUL, TONE_EXCITED, TONE_WARM, TONE_NEUTRAL, CH_END },
    [TONE_SARCASTIC]  = { TONE_SARCASTIC, TONE_COLD, TONE_NEUTRAL, TONE_HOSTILE, CH_END },
    [TONE_WEARY]      = { TONE_WEARY, TONE_NEUTRAL, TONE_HOLLOW, TONE_COLD, CH_END },
    [TONE_WORRIED]    = { TONE_WORRIED, TONE_TENDER, TONE_WARM, TONE_NEUTRAL, CH_END },
    [TONE_CURIOUS]    = { TONE_CURIOUS, TONE_NEUTRAL, TONE_PLAYFUL, TONE_WARM, CH_END },
    [TONE_SOLEMN]     = { TONE_SOLEMN, TONE_NEUTRAL, TONE_WEARY, TONE_COLD, CH_END },
    [TONE_SMUG]       = { TONE_SMUG, TONE_COLD, TONE_SARCASTIC, TONE_NEUTRAL, CH_END },
    [TONE_TENDER]     = { TONE_TENDER, TONE_WARM, TONE_WORRIED, TONE_NEUTRAL, CH_END },
    [TONE_SULKY]      = { TONE_SULKY, TONE_COLD, TONE_NEUTRAL, TONE_HOSTILE, CH_END },
    [TONE_SUSPICIOUS] = { TONE_SUSPICIOUS, TONE_COLD, TONE_CURIOUS, TONE_NEUTRAL, CH_END },
    [TONE_AFRAID]     = { TONE_AFRAID, TONE_WORRIED, TONE_SHY, TONE_NEUTRAL, CH_END },
    [TONE_HOLLOW]     = { TONE_HOLLOW, TONE_NEUTRAL, TONE_COLD, TONE_WEARY, CH_END },
    [TONE_MANIC]      = { TONE_MANIC, TONE_EXCITED, TONE_PLAYFUL, TONE_WARM, TONE_NEUTRAL, CH_END },
    [TONE_SHY]        = { TONE_SHY, TONE_NEUTRAL, TONE_WARM, TONE_WORRIED, CH_END },
    [TONE_BORED]      = { TONE_BORED, TONE_NEUTRAL, TONE_COLD, TONE_WEARY, CH_END },
    [TONE_EXCITED]    = { TONE_EXCITED, TONE_WARM, TONE_PLAYFUL, TONE_NEUTRAL, CH_END },
};

const char *lang_tone_name(int t) { return t >= 0 && t < TONE_COUNT ? TONE_NAME[t] : "?"; }
char lang_tone_code(int t) { return t >= 0 && t < TONE_COUNT ? TONE_CODE[t] : '?'; }

/* the full try order for mood t: its chain, then the old four by distance from the chain's first old mood (as the old picker did) */
int lang_mood_chain(int t, int *out, int cap) {
    int n = 0, base = TONE_NEUTRAL;
    if (t < 0 || t >= TONE_COUNT) t = TONE_NEUTRAL;
    #define ADD(x) do { int x_ = (x); int dup_ = 0; for (int q_ = 0; q_ < n; q_++) if (out[q_] == x_) dup_ = 1; if (!dup_ && n < cap) out[n++] = x_; } while (0)
    for (const unsigned char *c = CHAIN[t]; *c != CH_END; c++) ADD(*c);
    for (const unsigned char *c = CHAIN[t]; *c != CH_END; c++) if (*c < 4) { base = *c; break; }
    for (int d = 0; d < 4; d++) {
        int order[2] = { base - d, base + d };
        if (base < 2) { order[0] = base + d; order[1] = base - d; }
        for (int k = 0; k < (d ? 2 : 1); k++) if (order[k] >= 0 && order[k] <= 3) ADD(order[k]);
    }
    #undef ADD
    return n;
}

/* the mood asked for, else the next one of its order that has something. all moods are tried for the persona's voices first, then for PLAIN, then for any */
static int g_pass;      /* which pass of pick() found the last line: 0 the persona's own voices, 1 PLAIN, 2 any (chunk 11) */
static const Line *pick(const Persona *ps, const char *key, int tone) {
    int ord[TONE_COUNT], n = lang_mood_chain(tone, ord, TONE_COUNT);
    int mine = ps->style ? ps->style : PLAIN;
    for (int pass = 0; pass < 3; pass++) {
        if (pass == 1 && mine == PLAIN) continue;                  /* PLAIN already tried */
        for (int k = 0; k < n; k++) {
            const Line *l = pick_exact(ps, key, ord[k], pass);
            if (l) { g_pass = pass; return l; }
        }
    }
    return NULL;
}

/* chunk 11: a persona whose first voice is one of the seven new ones (elder .. poet) gets a PLAIN line (it had none of its own for this key and
 * mood) rewritten into its voice by code, and a little flavour (25 %) on the lines made for it. the old three voices are never touched, and the
 * out-of-character self-harm line never is. `pass` is g_pass read right after the pick (formatting picks pool lines and changes it) */
static void voice_line(const Persona *ps, const char *key, const Line *l, int pass, int tone, char *out, int cap) {
    if (!voice_enabled() || !voice_is_new(ps->style) || !strcmp(key, "r.selfharm")) return;
    char tmp[600]; int did = 0, v = voice_first(ps->style);
    if (pass == 1 && (l->styles & PLAIN)) did = voice_apply(v, tone, out, tmp, (int)sizeof tmp);
    else if (pass == 0 && rn(100) < 25) did = voice_flavour(v, tone, out, tmp, (int)sizeof tmp);
    if (did) snprintf(out, (size_t)cap, "%s", tmp);
}

int lang_mood_resolve(const char *key, int t, int style) {
    int ord[TONE_COUNT], n = lang_mood_chain(t, ord, TONE_COUNT);
    const Line *const *rows; int nkey = lang_key_rows(key, &rows);
    int masks[3] = { style, PLAIN, 0 }, np = style ? 3 : 1;      /* the voices, then PLAIN, then any (style 0 = any: one pass is enough) */
    if (style) { if (style == PLAIN) np = 2; else masks[0] = style; }
    for (int pass = 0; pass < np; pass++) {
        int m = style == PLAIN && pass == 1 ? 0 : masks[pass];
        for (int k = 0; k < n; k++)
            for (int r = 0; r < nkey; r++)
                if ((rows[r]->tones & (1u << ord[k])) && (!m || (rows[r]->styles & m))) return ord[k];
    }
    return -1;
}

static int tone_rude(Tone t) { return (int)((RUDE >> t) & 1u); }   /* cold, hostile, sarcastic, smug, sulky, suspicious (was tone >= COLD with 4 tones) */

/* ---------- templates: {slot} gets replaced by a line from the pool "p.<slot>" ---------- */
static const char *g_topic = "";
static int g_tone;

static void fmt(const Persona *ps, const char *tpl, char *out, int cap, int depth) {
    int o = 0;
    out[0] = 0;
    for (const char *s = tpl; *s && o < cap - 1; s++) {
        if (*s != '{') { out[o++] = *s; out[o] = 0; continue; }
        const char *e = strchr(s, '}');
        if (!e) break;
        char name[16]; int n = (int)(e - s - 1); if (n > 15) n = 15;
        memcpy(name, s + 1, (size_t)n); name[n] = 0;
        const char *rep = NULL; char sub[160];
        if (!strcmp(name, "player")) rep = player_name;
        else if (!strcmp(name, "npc")) rep = ps->name;
        else if (!strcmp(name, "topic")) rep = g_topic[0] ? g_topic : "that";
        else if (!strcmp(name, "pet") && voice_first(ps->style) == STYLE_MACHINE) rep = "USER";      /* chunk 11 */
        else if (depth < 4) {
            char key[24]; snprintf(key, sizeof key, "p.%s", name);
            const Line *l = pick(ps, key, g_tone);
            if (l) { fmt(ps, l->text, sub, (int)sizeof sub, depth + 1); rep = sub; remember(l); }
        }
        if (rep) { int r = (int)strlen(rep); if (o + r >= cap) r = cap - 1 - o; memcpy(out + o, rep, (size_t)r); o += r; out[o] = 0; }
        s = e;
    }
}

int lang_line_tone(const Persona *ps, const char *key, Tone t, char *out, int cap) {
    g_tone = (int)t;
    const Line *l = pick(ps, key, (int)t);
    if (!l) { out[0] = 0; return 0; }
    int pass = g_pass;
    remember(l);
    fmt(ps, l->text, out, cap, 0);
    voice_line(ps, key, l, pass, (int)t, out, cap);
    return 1;
}
int lang_line(const Persona *ps, const char *key, char *out, int cap) { return lang_line_tone(ps, key, lang_mood_now(ps), out, cap); }

/* ---------- answering ---------- */
static void cat(char *dst, int cap, const char *src) {
    int d = (int)strlen(dst);
    if (d && d < cap - 1) dst[d++] = ' ';
    snprintf(dst + d, (size_t)(cap - d), "%s", src);
}

int lang_reply(Persona *ps, const Heard *h, char *out, int cap) {
    out[0] = 0;
    g_topic = h->capture;
    /* a real person might be behind these words: step out of character, whoever is talking */
    if (heard_has(h, I_SELFHARM)) { lang_line_tone(ps, "r.selfharm", TONE_WARM, out, cap); return 1; }

    lang_apply(ps, h);
    Tone tone = lang_mood_now(ps);

    /* strong intents first (by rank), weak ones only if nothing else was said */
    int pickd[HEARD_ORDER], np = 0;
    for (int pass = 0; pass < 2 && !np; pass++) {
        int cand[HEARD_ORDER], nc = 0;
        for (int i = 0; i < h->n; i++) {
            int in = h->order[i];
            if (in == I_POS || in == I_NEG) continue;
            if ((rank_of(in) == 99) == (pass == 1)) cand[nc++] = in;
        }
        for (int a = 0; a < nc; a++)                                   /* tiny sort by rank */
            for (int b = a + 1; b < nc; b++) if (rank_of(cand[b]) < rank_of(cand[a])) { int x = cand[a]; cand[a] = cand[b]; cand[b] = x; }
        for (int a = 0; a < nc && np < 2; a++) pickd[np++] = cand[a];
    }

    if (np == 1 && pickd[0] == I_WHAT && h->words >= 5) np = 0;      /* a long question we have no answer for */
    char body[600]; body[0] = 0;       /* chunk 5: was 400 (cut to a readable length in chunk 42) */
    int answered = 0;
    for (int a = 0; a < np; a++) {
        int in = pickd[a];
        Tone tt = tone;
        if (in == I_SAD || in == I_SCARED || in == I_CONFUSED || in == I_HELP) {      /* even toxic gods do not kick a crying player */
            if (tt == TONE_HOSTILE || tt == TONE_SARCASTIC || tt == TONE_SMUG || tt == TONE_SULKY) tt = TONE_COLD;
            else if (tt == TONE_BORED) tt = TONE_NEUTRAL;
        }
        g_tone = (int)tt;
        char key[40], one[200];
        lang_intent_key(in, key, (int)sizeof key);
        const Line *l = pick(ps, key, (int)tt);
        if (!l) continue;
        int pass = g_pass;
        remember(l);
        fmt(ps, l->text, one, (int)sizeof one, 0);
        voice_line(ps, key, l, pass, (int)tt, one, (int)sizeof one);
        if (a == 1 && (int)strlen(body) + (int)strlen(one) > 150) break;   /* keep it readable on a phone */
        cat(body, (int)sizeof body, one);
        answered++;
        if ((int)strlen(body) > 90) break;
    }

    if (!answered) {                                                   /* never leave the player hanging */
        if (gibberish_flip(ps, h)) tone = lang_mood_now(ps);
        const char *key = "r.unknown";
        if (h->is_question && h->words >= 2) key = "r.unknown_q";
        else if (h->negative > h->positive) key = "r.unknown_neg";
        else if (h->positive > h->negative) key = "r.unknown_pos";
        else if (h->words <= 1) key = "r.unknown_short";
        g_tone = (int)tone;
        const Line *l = pick(ps, key, (int)tone);
        if (l) { int pass = g_pass; remember(l); fmt(ps, l->text, body, (int)sizeof body, 0); voice_line(ps, key, l, pass, (int)tone, body, (int)sizeof body); }
    }

    /* a little colour around the edges: an opener and/or a closer */
    int len = (int)strlen(body);
    char pre[80] = "", post[100] = "", full[800];
    g_tone = (int)tone;
    if (len < 110 && rn(100) < (tone_rude(tone) ? 45 : 25)) {
        const Line *l = pick(ps, "p.pre", (int)tone);
        if (l) { remember(l); fmt(ps, l->text, pre, (int)sizeof pre, 1); }
    }
    if (len + (int)strlen(pre) < 100 && rn(100) < (tone_rude(tone) ? 50 : 30)) {
        const Line *l = pick(ps, "p.post", (int)tone);
        if (l) { remember(l); fmt(ps, l->text, post, (int)sizeof post, 1); }
    }
    full[0] = 0;
    if (pre[0]) cat(full, (int)sizeof full, pre);
    cat(full, (int)sizeof full, body);
    if (post[0]) cat(full, (int)sizeof full, post);
    snprintf(out, (size_t)cap, "%s", full);
    return answered;
}
