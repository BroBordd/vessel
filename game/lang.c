/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * see lang.h for the big picture. the tables live in lang_pats.c and lines_*.c */
#include "lang.h"
#include "lang_data.h"
#include "lang_tables.h"
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
    if (nsf) { heard_set(h, I_SELFHARM); h->order[h->n++] = I_SELFHARM; }
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
        if (!heard_has(h, in) && h->n < HEARD_ORDER) { heard_set(h, in); h->order[h->n++] = in; }
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

static const Line *pick_exact(const Persona *ps, const char *key, int tone, int any_style) {
    int n = 0, sum = 0;
    const Line *const *rows; int nkey = lang_key_rows(key, &rows);     /* chunk 6: only this key's rows, in table order */
    for (int r = 0; r < nkey; r++) {
        const Line *l = rows[r];
        if (!(l->tones & (1u << tone))) continue;
        if (!any_style && !(l->styles & ps->style)) continue;
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
        if (!any_style && l->styles != ALLS && (l->styles & ps->style) == l->styles) w = 3;   /* made just for this style */
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

/* the mood asked for, else the next one of its order that has something */
static const Line *pick(const Persona *ps, const char *key, int tone) {
    int ord[TONE_COUNT], n = lang_mood_chain(tone, ord, TONE_COUNT);
    for (int any = 0; any < 2; any++)
        for (int k = 0; k < n; k++) {
            const Line *l = pick_exact(ps, key, ord[k], any);
            if (l) return l;
        }
    return NULL;
}

int lang_mood_resolve(const char *key, int t, int style) {
    int ord[TONE_COUNT], n = lang_mood_chain(t, ord, TONE_COUNT);
    const Line *const *rows; int nkey = lang_key_rows(key, &rows);
    for (int any = 0; any < 2; any++) {
        if (any && !style) break;                              /* style 0 = any style: one pass is enough */
        for (int k = 0; k < n; k++)
            for (int r = 0; r < nkey; r++)
                if ((rows[r]->tones & (1u << ord[k])) && (!style || any || (rows[r]->styles & style))) return ord[k];
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
            if ((RANK[in] == 99) == (pass == 1)) cand[nc++] = in;
        }
        for (int a = 0; a < nc; a++)                                   /* tiny sort by rank */
            for (int b = a + 1; b < nc; b++) if (RANK[cand[b]] < RANK[cand[a]]) { int x = cand[a]; cand[a] = cand[b]; cand[b] = x; }
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
        if (gibberish_flip(ps, h)) tone = lang_mood_now(ps);
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
