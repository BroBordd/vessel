/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * see lang.h for the big picture. the tables live in lang_pats.c and lines_*.c */
#include "lang.h"
#include "lang_data.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- names ---------- */
static const char *const INTENT_NAME[I_COUNT] = {
    "greet", "bye", "yes", "no", "ok", "thanks", "sorry", "howru", "whoru", "where",
    "whoami", "mission", "why", "how", "what", "help", "insult", "praise", "love", "worship",
    "joke", "scared", "confused", "angry", "sad", "intro", "repeat", "sky", "music", "hole",
    "secret", "life", "wait", "please", "agree", "disagree", "dontcare", "demand", "callout", "age",
    "real", "address", "selfharm", "alex", "pos", "neg"
};
static const char *const INTENT_LABEL[I_COUNT] = {
    "HELLO", "BYE", "YES", "NO", "OK", "THANKS", "SORRY", "HOW ARE YOU", "WHO ARE YOU", "WHERE",
    "WHO AM I", "MISSION", "WHY", "HOW", "WHAT", "HELP", "INSULT", "PRAISE", "LOVE", "WORSHIP",
    "JOKE", "FEAR", "CONFUSION", "ANGER", "SADNESS", "INTRO", "REPEAT", "SKY", "MUSIC", "HOLE",
    "SECRET", "LIFE", "WAIT", "PLEASE", "AGREE", "DISAGREE", "DONT CARE", "DEMAND", "CALL OUT", "AGE",
    "REALITY", "NAME", "CONCERN", "ALEX", "GOOD", "BAD"
};

/* lower number = answered first. 99 = only ever answered when nothing better was said */
static const unsigned char RANK[I_COUNT] = {
    /* greet */ 8, /* bye */ 9, /* yes */ 99, /* no */ 99, /* ok */ 99, /* thanks */ 7, /* sorry */ 3, /* howru */ 12, /* whoru */ 11, /* where */ 13,
    /* whoami */ 14, /* mission */ 15, /* why */ 99, /* how */ 99, /* what */ 99, /* help */ 25, /* insult */ 1, /* praise */ 6, /* love */ 4, /* worship */ 5,
    /* joke */ 24, /* scared */ 26, /* confused */ 29, /* angry */ 28, /* sad */ 27, /* intro */ 10, /* repeat */ 34, /* sky */ 21, /* music */ 22, /* hole */ 16,
    /* secret */ 17, /* life */ 18, /* wait */ 99, /* please */ 99, /* agree */ 33, /* disagree */ 32, /* dontcare */ 31, /* demand */ 30, /* callout */ 2, /* age */ 19,
    /* real */ 20, /* address */ 99, /* selfharm */ 0, /* alex */ 23, /* pos */ 99, /* neg */ 99
};

/* mood change, charisma change. positive mood is softened for toxic npcs (see lang_apply) */
static const signed char MOOD[I_COUNT][2] = {
    [I_GREET] = {2, 0}, [I_THANKS] = {4, 1}, [I_SORRY] = {6, 0}, [I_PRAISE] = {8, 2}, [I_LOVE] = {6, 1},
    [I_WORSHIP] = {10, 2}, [I_JOKE] = {3, 1}, [I_INSULT] = {-14, -2}, [I_CALLOUT] = {-3, 0}, [I_DEMAND] = {-5, -1},
    [I_DONTCARE] = {-8, -1}, [I_DISAGREE] = {-4, 0}, [I_AGREE] = {4, 1}, [I_SCARED] = {0, -1}, [I_SAD] = {0, -1},
    [I_ANGRY] = {-4, -1}, [I_PLEASE] = {3, 1}, [I_INTRO] = {2, 0}, [I_HOWRU] = {2, 1}, [I_ADDRESS] = {1, 0},
};

/* ---------- random ---------- */
static uint32_t rs = 0x9E3779B9u;
void lang_seed(unsigned s) { rs = s ? s : 0x9E3779B9u; }
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
#define MAX_TEXT 200
#define MAX_SPANS 40

static int scan(const char *lc, int len, Span *sp, int maxsp) {
    int n = 0, i = 0;
    while (i < len && n < maxsp) {
        if (!is_word((unsigned char)lc[i]) || (i > 0 && is_word((unsigned char)lc[i - 1]))) { i++; continue; }
        const char *best = lc + i; int bk = -1, bstars = 99;
        for (int k = 0; k < PATS_N; k++) {
            const char *p = PATS[k].pat;
            if (*p != '*' && *p != '?' && *p != lc[i]) continue;
            const char *e = lc + i;
            mt(p, lc + i, lc, &e, p);
            if (e == lc + i) continue;
            int st = stars(p);
            if (e > best || (e == best && bk >= 0 && st < bstars)) { best = e; bk = k; bstars = st; }
        }
        if (bk >= 0) {
            sp[n].start = i; sp[n].end = (int)(best - lc); sp[n].intent = PATS[bk].intent; sp[n].prefix = prefix_len(PATS[bk].pat);
            n++; i = (int)(best - lc);
        } else i++;
    }
    return n;
}


/* safety words are checked everywhere in the text, even inside a longer phrase ("i want to kill myself") */
static int safety_scan(const char *lc, int len, Span *out, int maxo) {
    int n = 0;
    for (int i = 0; i < len && n < maxo; i++) {
        if (!is_word((unsigned char)lc[i]) || (i > 0 && is_word((unsigned char)lc[i - 1]))) continue;
        for (int k = 0; k < PATS_N; k++) {
            if (PATS[k].intent != I_SELFHARM) continue;
            const char *p = PATS[k].pat; const char *e = lc + i;
            if (*p != lc[i]) continue;
            mt(p, lc + i, lc, &e, p);
            if (e > lc + i) { out[n].start = i; out[n].end = (int)(e - lc); out[n].intent = I_SELFHARM; out[n].prefix = 0; n++; break; }
        }
    }
    return n;
}

static const char *const NEGATORS[] = { "not", "no", "never", "dont", "don't", "cant", "can't", "wont", "won't", "isnt", "isn't",
    "arent", "aren't", "didnt", "didn't", "hardly", "without", "aint", "ain't", "doesnt", "doesn't", NULL };

/* is there a negator in the 3 words before this point (same sentence)? */
static int negated(const char *lc, int start) {
    int i = start - 1, words = 0;
    while (i >= 0 && words < 3) {
        while (i >= 0 && lc[i] == ' ') i--;
        if (i < 0 || !is_word((unsigned char)lc[i])) break;
        int e = i;
        while (i >= 0 && is_word((unsigned char)lc[i])) i--;
        char w[16]; int n = e - i; if (n > 15) n = 15;
        memcpy(w, lc + i + 1, (size_t)n); w[n] = 0;
        for (int k = 0; NEGATORS[k]; k++) if (!strcmp(w, NEGATORS[k])) return 1;
        words++;
    }
    return 0;
}

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

static int prepare(const char *text, char *lc) {
    int n = 0;
    for (; text[n] && n < MAX_TEXT - 1; n++) lc[n] = (char)tolower((unsigned char)text[n]);
    lc[n] = 0;
    return n;
}

void lang_parse(const char *text, Heard *h) {
    memset(h, 0, sizeof *h);
    char lc[MAX_TEXT]; int len = prepare(text, lc);
    Span sp[MAX_SPANS];
    int ns = scan(lc, len, sp, MAX_SPANS);
    Span sf[4]; int nsf = safety_scan(lc, len, sf, 4);
    if (nsf) { h->mask |= 1ull << I_SELFHARM; h->order[h->n++] = I_SELFHARM; }
    for (int k = 0; k < ns; k++) {
        int in = sp[k].intent;
        if (negated(lc, sp[k].start)) in = flip(in);
        if (in == I_POS) h->positive++;
        else if (in == I_NEG) h->negative++;
        if (in == I_INTRO) {
            int a = sp[k].start + sp[k].prefix, b = sp[k].end;
            while (a < b && lc[a] == ' ') a++;
            int n = b - a; if (n > 22) n = 22;
            if (n > 0) { memcpy(h->capture, text + a, (size_t)n); h->capture[n] = 0; }
        }
        if (!(h->mask & (1ull << in)) && h->n < 12) { h->mask |= 1ull << in; h->order[h->n++] = in; }
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

int lang_highlight(const char *text, uint8_t *mask) {
    char lc[MAX_TEXT]; int len = prepare(text, lc);
    int total = (int)strlen(text);
    for (int i = 0; i <= total; i++) mask[i] = 0;
    Span sp[MAX_SPANS];
    int ns = scan(lc, len, sp, MAX_SPANS);
    for (int k = 0; k < ns; k++) for (int i = sp[k].start; i < sp[k].end; i++) mask[i] = 1;
    Span sf[4]; int nsf = safety_scan(lc, len, sf, 4);
    for (int k = 0; k < nsf; k++) for (int i = sf[k].start; i < sf[k].end; i++) mask[i] = 1;
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
Tone lang_tone(const Persona *ps) { return tone_from(tone_score(ps) + rn(15) - 7); }

void lang_apply(Persona *ps, const Heard *h) {
    for (int i = 0; i < h->n; i++) {
        int in = h->order[i];
        int dm = MOOD[in][0], dc = MOOD[in][1];
        if (dm > 0) dm = dm * (100 - ps->toxic / 2) / 100;          /* toxic npcs warm up slowly */
        ps->mood += dm;
        lang_add_charisma(dc);
    }
    if (ps->mood > 100) ps->mood = 100;
    if (ps->mood < -100) ps->mood = -100;
}

/* ---------- line picking ---------- */
static const Line *const TABLE[4] = { LINES_A, LINES_B, LINES_C, LINES_D };
static const int *const COUNT[4] = { &LINES_A_N, &LINES_B_N, &LINES_C_N, &LINES_D_N };
static const Line *recent[24];
static int nrecent;

static int was_recent(const Line *l) { for (int i = 0; i < 24; i++) if (recent[i] == l) return 1; return 0; }
static void remember(const Line *l) { recent[nrecent++ % 24] = l; }

static const Line *pick_exact(const Persona *ps, const char *key, int tone, int any_style) {
    const Line *cand[96]; int wt[96]; int n = 0, sum = 0;
    for (int t = 0; t < 4; t++)
        for (int i = 0; i < *COUNT[t]; i++) {
            const Line *l = &TABLE[t][i];
            if (!(l->tones & (1 << tone)) || strcmp(l->key, key)) continue;
            if (!any_style && !(l->styles & ps->style)) continue;
            if (n >= 96) break;
            int w = 1;
            if (!any_style && l->styles != ALLS && (l->styles & ps->style) == l->styles) w = 3;   /* made just for this style */
            if (was_recent(l)) w = 0;
            cand[n] = l; wt[n] = w; sum += w; n++;
        }
    if (!n) return NULL;
    if (sum == 0) { const Line *l = cand[rn(n)]; return l; }   /* only repeats left: allow one */
    int r = rn(sum);
    for (int i = 0; i < n; i++) { r -= wt[i]; if (r < 0) return cand[i]; }
    return cand[0];
}

/* the tone asked for, else the nearest tone that has something */
static const Line *pick(const Persona *ps, const char *key, int tone) {
    for (int any = 0; any < 2; any++)
        for (int d = 0; d < 4; d++) {
            int order[2] = { tone - d, tone + d };
            if (tone < 2) { order[0] = tone + d; order[1] = tone - d; }
            for (int k = 0; k < (d ? 2 : 1); k++) {
                int t = order[k];
                if (t < 0 || t > 3) continue;
                const Line *l = pick_exact(ps, key, t, any);
                if (l) return l;
            }
        }
    return NULL;
}

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
    remember(l);
    fmt(ps, l->text, out, cap, 0);
    return 1;
}
int lang_line(const Persona *ps, const char *key, char *out, int cap) { return lang_line_tone(ps, key, lang_tone(ps), out, cap); }

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
    if (h->mask & (1ull << I_SELFHARM)) { lang_line_tone(ps, "r.selfharm", TONE_WARM, out, cap); return 1; }

    lang_apply(ps, h);
    Tone tone = lang_tone(ps);

    /* strong intents first (by rank), weak ones only if nothing else was said */
    int pickd[12], np = 0;
    for (int pass = 0; pass < 2 && !np; pass++) {
        int cand[12], nc = 0;
        for (int i = 0; i < h->n; i++) {
            int in = h->order[i];
            if (in == I_POS || in == I_NEG) continue;
            if ((RANK[in] == 99) == (pass == 1)) cand[nc++] = in;
        }
        for (int a = 0; a < nc; a++)                                   /* tiny sort by rank */
            for (int b = a + 1; b < nc; b++) if (RANK[cand[b]] < RANK[cand[a]]) { int x = cand[a]; cand[a] = cand[b]; cand[b] = x; }
        for (int a = 0; a < nc && np < 2; a++) pickd[np++] = cand[a];
    }

    if (np == 1 && pickd[0] == I_WHAT && h->words >= 5) np = 0;      /* a long question we have no answer for */
    char body[400]; body[0] = 0;
    int answered = 0;
    for (int a = 0; a < np; a++) {
        int in = pickd[a];
        Tone tt = tone;
        if ((in == I_SAD || in == I_SCARED || in == I_CONFUSED || in == I_HELP) && tt == TONE_HOSTILE) tt = TONE_COLD;   /* even toxic gods do not kick a crying player */
        g_tone = (int)tt;
        char key[24], one[200];
        snprintf(key, sizeof key, "r.%s", INTENT_NAME[in]);
        const Line *l = pick(ps, key, (int)tt);
        if (!l) continue;
        remember(l);
        fmt(ps, l->text, one, (int)sizeof one, 0);
        if (a == 1 && (int)strlen(body) + (int)strlen(one) > 150) break;   /* keep it readable on a phone */
        cat(body, (int)sizeof body, one);
        answered++;
        if ((int)strlen(body) > 90) break;
    }

    if (!answered) {                                                   /* never leave the player hanging */
        const char *key = "r.unknown";
        if (h->is_question && h->words >= 2) key = "r.unknown_q";
        else if (h->negative > h->positive) key = "r.unknown_neg";
        else if (h->positive > h->negative) key = "r.unknown_pos";
        else if (h->words <= 1) key = "r.unknown_short";
        g_tone = (int)tone;
        const Line *l = pick(ps, key, (int)tone);
        if (l) { remember(l); fmt(ps, l->text, body, (int)sizeof body, 0); }
    }

    /* a little colour around the edges: an opener and/or a closer */
    int len = (int)strlen(body);
    char pre[80] = "", post[100] = "", full[480];
    g_tone = (int)tone;
    if (len < 110 && rn(100) < (tone >= TONE_COLD ? 45 : 25)) {
        const Line *l = pick(ps, "p.pre", (int)tone);
        if (l) { remember(l); fmt(ps, l->text, pre, (int)sizeof pre, 1); }
    }
    if (len + (int)strlen(pre) < 100 && rn(100) < (tone >= TONE_COLD ? 50 : 30)) {
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
