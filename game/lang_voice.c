/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * voice transforms, see lang_voice.h. The pools below are built in; chunk 128 may add database pools (p.elder_open, p.beast ...) later. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wformat-truncation"   /* the drafts are 700 characters and every snprintf here is bounded; finish() cuts to 150 */
#endif
#include "lang_voice.h"
#include "lang.h"
#include "lang_data.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

static unsigned g_seed = 1, g_applied;
void voice_seed(unsigned s) { g_seed = s ? s : 1; g_applied = 0; }
unsigned voice_applied(void) { return g_applied; }

int voice_first(int style) { return style ? (style & -style) : STYLE_PLAIN; }
int voice_is_new(int style) { int v = voice_first(style); return v >= STYLE_ELDER && v <= STYLE_POET; }
const char *voice_name(int style) {
    switch (voice_first(style)) {
    case STYLE_PLAIN: return "plain";   case STYLE_DIVINE: return "divine"; case STYLE_STREET: return "street";
    case STYLE_ELDER: return "elder";   case STYLE_CHILD: return "child";   case STYLE_FORMAL: return "formal";
    case STYLE_BEAST: return "beast";   case STYLE_MACHINE: return "machine"; case STYLE_GHOST: return "ghost";
    case STYLE_POET: return "poet";     default: return "?";
    }
}

/* ---------- a small private random stream: hash of (seed, voice, mood, text), so a call never depends on an earlier one ---------- */
typedef struct { uint32_t s; } Rng;
static void rng_init(Rng *r, const char *text, int style, int tone) {
    uint32_t h = 2166136261u;
    for (const char *p = text; *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
    h ^= g_seed * 2654435761u; h ^= (uint32_t)style * 40503u + (uint32_t)tone * 9973u;
    if (!h) h = 1;
    r->s = h;
}
static uint32_t rng_next(Rng *r) { uint32_t x = r->s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return r->s = x; }
static int rn_(Rng *r, int n) { return n > 0 ? (int)(rng_next(r) % (uint32_t)n) : 0; }
static int chance(Rng *r, int pct) { return rn_(r, 100) < pct; }

/* ---------- pools: a tag fits the mood groups of lang_data.h (1 nice, 2 rude, 4 dark) ---------- */
typedef struct { const char *t; unsigned g; } Tag;
#define GN 1u
#define GR 2u
#define GD 4u
#define GA 7u
static unsigned group_of(int tone) {
    if (tone < 0 || tone >= TONE_COUNT) return GN;
    unsigned b = 1u << tone;
    return (NICE & b) ? GN : (RUDE & b) ? GR : GD;
}
static const char *pool_pick(const Tag *p, int n, unsigned g, Rng *r) {
    int c = 0;
    for (int i = 0; i < n; i++) if (p[i].g & g) c++;
    if (!c) return NULL;
    int k = rn_(r, c);
    for (int i = 0; i < n; i++) if (p[i].g & g) { if (!k--) return p[i].t; }
    return NULL;
}
#define PICK(arr, g, r) pool_pick(arr, (int)(sizeof arr / sizeof *arr), g, r)

static const Tag CHILD_TAG[]   = { {"Why?", GA}, {"Okay?", GN}, {"Really.", GA}, {"So there.", GR}, {"Meanie.", GR}, {"I'm scared.", GD}, {"Hmm.", GA} };
static const Tag ELDER_OPEN[]  = { {"Ah, little one.", GA}, {"Hm. Sit down, child.", GN | GD}, {"Well now.", GA}, {"Listen, little one.", GA | GR}, {"In my day, things were simpler.", GN | GR} };
static const Tag ELDER_CLOSE[] = { {"That is how it was.", GA}, {"So it goes.", GD | GN}, {"Mark my words.", GN | GR}, {"Hmm, yes. Yes.", GA}, {"Old bones know.", GD} };
static const Tag FORMAL_OPEN[] = { {"Be advised.", GA}, {"Kindly note.", GN | GD}, {"Permit me to say.", GA}, {"Allow me to be plain.", GR} };
static const Tag BEAST_GROWL[] = { {"Grr.", GA}, {"Hrrr.", GA}, {"Hsss.", GR | GD}, {"Rrrf.", GN}, {"Hrm.", GA} };
static const Tag MACH_LABEL[]  = { {"RESPONSE.", GA}, {"QUERY RECEIVED.", GA}, {"ACKNOWLEDGED.", GA}, {"PROCESSING.", GA}, {"NOTICE.", GA}, {"WARNING.", GR | GD} };
static const Tag GHOST_OPEN[]  = { {"Shh.", GA}, {"Listen.", GA}, {"Hush.", GA}, {"Closer.", GD | GN} };
static const Tag POET_OPEN[]   = { {"Like moths at a window,", GA}, {"As the tide forgets the shore,", GA}, {"Under a tired moon,", GD | GN}, {"Where ash remembers fire,", GD | GR},
                                   {"Soft as dust on a bell,", GN}, {"Like a lamp in an empty house,", GA} };

/* ---------- text helpers ---------- */
static int starts_ci(const char *s, const char *w) { for (; *w; s++, w++) if (tolower((unsigned char)*s) != tolower((unsigned char)*w)) return 0; return 1; }
static int is_wc(int c) { return isalnum(c) || c == '\''; }

/* keep only the font's characters, collapse spaces, trim; cut to VOICE_MAX_LEN at a sentence or a word. returns the length (0 = nothing left) */
static int finish(const char *src, char *dst, int cap) {
    char t[700]; int n = 0, sp = 1;
    for (const char *p = src; *p && n < (int)sizeof t - 2; p++) {
        int c = (unsigned char)*p;
        if (isalnum(c) || c == ',' || c == '.' || c == '!' || c == '?' || c == '\'' || c == '-') { t[n++] = (char)c; sp = 0; }
        else if (!sp) { t[n++] = ' '; sp = 1; }
    }
    while (n && t[n - 1] == ' ') n--;
    t[n] = 0;
    if (n > VOICE_MAX_LEN) {
        int cut = -1;
        for (int i = 0; i < VOICE_MAX_LEN; i++) if (t[i] == '.' || t[i] == '!' || t[i] == '?') cut = i + 1;
        if (cut < 12) { cut = -1; for (int i = 0; i < VOICE_MAX_LEN; i++) if (t[i] == ' ') cut = i; }
        if (cut < 12) cut = VOICE_MAX_LEN;
        n = cut; t[n] = 0;
        while (n && (t[n - 1] == ' ' || t[n - 1] == ',')) t[--n] = 0;
    }
    if (n >= cap) n = cap - 1;
    memcpy(dst, t, (size_t)n); dst[n] = 0;
    return n;
}

/* replace every whole-word, case-insensitive `from` in src by `to` (the first letter of a replacement follows the original's case) */
static void swap_words(const char *src, char *out, int cap, const char *const (*tab)[2], int ntab) {
    int o = 0;
    for (const char *p = src; *p && o < cap - 1;) {
        if (is_wc((unsigned char)*p) && (p == src || !is_wc((unsigned char)p[-1]))) {
            int hit = 0;
            for (int k = 0; k < ntab; k++) {
                int L = (int)strlen(tab[k][0]);
                if (starts_ci(p, tab[k][0]) && !is_wc((unsigned char)p[L])) {
                    const char *to = tab[k][1];
                    for (int j = 0; to[j] && o < cap - 1; j++) {
                        char c = to[j];
                        if (j == 0 && isupper((unsigned char)*p) && islower((unsigned char)c)) c = (char)toupper((unsigned char)c);
                        out[o++] = c;
                    }
                    p += L; hit = 1; break;
                }
            }
            if (hit) continue;
        }
        out[o++] = *p++;
    }
    out[o] = 0;
}
/* a plain string replace (case sensitive, anywhere) */
static void rep_all(char *s, int cap, const char *from, const char *to) {
    char t[700]; int o = 0, fl = (int)strlen(from);
    for (const char *p = s; *p && o < (int)sizeof t - 2;) {
        if (!strncmp(p, from, (size_t)fl)) { for (const char *q = to; *q && o < (int)sizeof t - 2; q++) t[o++] = *q; p += fl; }
        else t[o++] = *p++;
    }
    t[o] = 0;
    strncpy(s, t, (size_t)cap - 1); s[cap - 1] = 0;
}
static void upper_first_word(char *s) { for (; *s && !isalpha((unsigned char)*s); s++) {} for (; *s && is_wc((unsigned char)*s); s++) *s = (char)toupper((unsigned char)*s); }
static int count_words(const char *s) { int n = 0, in = 0; for (; *s; s++) { int w = is_wc((unsigned char)*s); if (w && !in) n++; in = w; } return n; }

/* "don't" -> "do not" ... both formal and machine voices */
static const char *const CONTR[][2] = {
    {"don't","do not"},{"doesn't","does not"},{"didn't","did not"},{"can't","cannot"},{"won't","will not"},{"isn't","is not"},{"aren't","are not"},
    {"wasn't","was not"},{"weren't","were not"},{"haven't","have not"},{"hasn't","has not"},{"hadn't","had not"},{"shouldn't","should not"},
    {"wouldn't","would not"},{"couldn't","could not"},{"i'm","I am"},{"i've","I have"},{"i'd","I would"},{"i'll","I will"},{"you're","you are"},
    {"you've","you have"},{"you'll","you will"},{"you'd","you would"},{"we're","we are"},{"we've","we have"},{"we'll","we will"},{"they're","they are"},
    {"they've","they have"},{"they'll","they will"},{"he's","he is"},{"she's","she is"},{"it's","it is"},{"that's","that is"},{"what's","what is"},
    {"who's","who is"},{"there's","there is"},{"here's","here is"},{"let's","let us"},{"gonna","going to"},{"wanna","want to"},{"gotta","must"},
    {"yeah","yes"},{"yep","yes"},{"nope","no"},{"nah","no"},
};
#define NCONTR ((int)(sizeof CONTR / sizeof *CONTR))
static const char *const KIDWORDS[][2] = {
    {"however","but"},{"therefore","so"},{"understand","get"},{"certainly","sure"},{"perhaps","maybe"},{"unfortunately","sadly"},{"difficult","hard"},
    {"beautiful","pretty"},{"mortal","person"},{"ancient","old"},{"silence","quiet"},{"very","so"},{"extremely","super"},{"approach","come"},
    {"immediately","now"},{"currently","now"},{"regarding","about"},{"request","ask"},{"attempt","try"},{"terrified","scared"},{"frightened","scared"},
    {"magnificent","big"},{"obey","listen"},{"foolish","silly"},{"pathetic","sad"},{"wretched","sad"},{"wisdom","smart"},{"mission","job"},
    {"cannot","can't"},{"do not","don't"},{"will not","won't"},{"I am","I'm"},
};
#define NKID ((int)(sizeof KIDWORDS / sizeof *KIDWORDS))

/* ---------- the seven transforms. each reads `in` (<= 150 characters) and writes a draft to t (cap 700) ---------- */
static void t_child(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    char a[700]; snprintf(a, sizeof a, "%s", in);
    if (!light) {
        char b[700]; swap_words(a, b, sizeof b, KIDWORDS, NKID);
        /* a long clause is dropped: in a sentence of more than 10 words keep what is before the first comma */
        char c[700]; int o = 0; const char *p = b;
        while (*p) {
            const char *e = p; while (*e && *e != '.' && *e != '!' && *e != '?') e++;
            int len = (int)(e - p); char endc = *e ? *e : 0;
            char sent[400]; if (len > 390) len = 390; memcpy(sent, p, (size_t)len); sent[len] = 0;
            char *cm = strchr(sent, ',');
            if (cm && count_words(sent) > 10 && count_words(cm + 1) >= 2) {
                int head = (int)(cm - sent);
                if (count_words(sent) - count_words(cm + 1) >= 3) { sent[head] = 0; if (!endc) endc = '.'; }
            }
            for (char *q = sent; *q && o < (int)sizeof c - 3; q++) c[o++] = *q;
            if (endc && o < (int)sizeof c - 3) c[o++] = endc;
            p = *e ? e + 1 : e;
            if (*p == ' ' && o < (int)sizeof c - 3) c[o++] = ' ';
            while (*p == ' ') p++;
        }
        c[o] = 0; snprintf(a, sizeof a, "%s", c);
    }
    int n = (int)strlen(a);
    const char *tag = (!light && chance(r, 40)) || (light && chance(r, 60)) ? PICK(CHILD_TAG, g, r) : NULL;
    if (tag && n && a[n - 1] != '?' && n + 1 + (int)strlen(tag) <= VOICE_MAX_LEN) { strcat(a, " "); strcat(a, tag); }
    snprintf(t, (size_t)cap, "%s", a);
}

static void t_elder(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    (void)light;
    const char *open = chance(r, 55) ? PICK(ELDER_OPEN, g, r) : NULL;
    const char *close = chance(r, 35) ? PICK(ELDER_CLOSE, g, r) : NULL;
    int n = (int)strlen(in);
    if (close && n + 1 + (int)strlen(close) > VOICE_MAX_LEN) close = NULL;
    if (open && n + 1 + (int)strlen(open) + (close ? 1 + (int)strlen(close) : 0) > VOICE_MAX_LEN) open = NULL;
    snprintf(t, (size_t)cap, "%s%s%s%s%s", open ? open : "", open ? " " : "", in, close ? " " : "", close ? close : "");
}

static void t_formal(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    (void)light;
    char a[700]; swap_words(in, a, sizeof a, CONTR, NCONTR);
    rep_all(a, (int)sizeof a, "I will", "I shall"); rep_all(a, (int)sizeof a, "we will", "we shall");
    rep_all(a, (int)sizeof a, "We will", "We shall"); rep_all(a, (int)sizeof a, "Will you", "Shall you");
    const char *open = chance(r, 30) ? PICK(FORMAL_OPEN, g, r) : NULL;
    if (open && (int)strlen(a) + 1 + (int)strlen(open) > VOICE_MAX_LEN) open = NULL;
    snprintf(t, (size_t)cap, "%s%s%s", open ? open : "", open ? " " : "", a);
}

static int is_fn_word(const char *w, int L) {
    static const char *const FN[] = { "a","an","the","of","to","is","are","am","was","were","be","been","that","which","would","should","could",
        "will","shall","do","does","did","it","its","as","at","in","on","for","with","so","just","very","really","quite","rather","i", NULL };
    for (int i = 0; FN[i]; i++) if ((int)strlen(FN[i]) == L && starts_ci(w, FN[i])) return 1;
    return 0;
}
static void t_beast(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    char a[700]; a[0] = 0;
    if (!light) {
        int o = 0, kept_total = 0;
        const char *p = in;
        while (*p && kept_total < 5) {
            const char *e = p; while (*e && *e != ',' && *e != '.' && *e != '!' && *e != '?' && *e != ';') e++;
            char endc = *e == '?' ? '?' : '.';
            char clause[320]; int cl = 0, words = 0;
            for (const char *w = p; w < e;) {
                if (!is_wc((unsigned char)*w)) { w++; continue; }
                const char *we = w; while (we < e && is_wc((unsigned char)*we)) we++;
                int L = (int)(we - w);
                int conj = (L == 3 && starts_ci(w, "and")) || (L == 3 && starts_ci(w, "but")) || (L == 2 && starts_ci(w, "or")) || (L == 4 && starts_ci(w, "then"));
                if (conj && !cl) { w = we; continue; }                  /* a clause does not start with and / but / or */
                if (!is_fn_word(w, L) && words < 4 && cl + L + 2 < (int)sizeof clause) {
                    if (cl) clause[cl++] = ' ';
                    memcpy(clause + cl, w, (size_t)L); cl += L; words++;
                }
                w = we;
            }
            clause[cl] = 0;
            if (cl && o + cl + 3 < (int)sizeof a) {
                if (o) a[o++] = ' ';
                clause[0] = (char)toupper((unsigned char)clause[0]);
                memcpy(a + o, clause, (size_t)cl); o += cl; a[o++] = endc; a[o] = 0;
                kept_total++;
            }
            p = *e ? e + 1 : e;
        }
        if (!a[0]) {                                         /* nothing survived: the first three words, as they were */
            int w = 0, o2 = 0;
            for (const char *q = in; *q && w < 3 && o2 < 300; q++) { if (*q == ' ') w++; if (w < 3) a[o2++] = *q; }
            a[o2] = 0; strcat(a, ".");
        }
        /* the first emphasised word is said twice */
        static const char *const EMPH[] = { "no","mine","go","run","stay","hungry","cold","dark","come","now", NULL };
        for (const char *q = a; *q; q++) {
            if (!is_wc((unsigned char)*q) || (q > a && is_wc((unsigned char)q[-1]))) continue;
            int hit = -1;
            for (int i = 0; EMPH[i]; i++) { int L = (int)strlen(EMPH[i]); if (starts_ci(q, EMPH[i]) && !is_wc((unsigned char)q[L])) { hit = i; break; } }
            if (hit >= 0) {
                int L = (int)strlen(EMPH[hit]), pos = (int)(q - a);
                char b[700]; snprintf(b, sizeof b, "%.*s %.*s%s", pos + L, a, L, EMPH[hit], a + pos + L);
                b[pos + L + 1] = (char)tolower((unsigned char)b[pos + L + 1]);
                if ((int)strlen(b) <= VOICE_MAX_LEN) snprintf(a, sizeof a, "%s", b);
                break;
            }
        }
    } else snprintf(a, sizeof a, "%s", in);
    const char *gr = (light || chance(r, 50)) ? PICK(BEAST_GROWL, g, r) : NULL;
    int n = (int)strlen(a), front = chance(r, 60);
    if (gr && n + 1 + (int)strlen(gr) <= VOICE_MAX_LEN) { if (front) snprintf(t, (size_t)cap, "%s %s", gr, a); else snprintf(t, (size_t)cap, "%s %s", a, gr); }
    else snprintf(t, (size_t)cap, "%s", a);
}

static void t_machine(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    char a[700], b[700];
    swap_words(in, a, sizeof a, CONTR, NCONTR);
    if (!light) {
        rep_all(a, (int)sizeof a, "I am", "THIS UNIT IS"); rep_all(a, (int)sizeof a, "I will", "THIS UNIT WILL");
        for (char *q = a; *q; q++) if (*q == '!') *q = '.';
        upper_first_word(a);
    }
    const char *lab = (light || chance(r, 60)) ? PICK(MACH_LABEL, g, r) : NULL;
    int n = (int)strlen(a);
    if (lab && n + 1 + (int)strlen(lab) <= VOICE_MAX_LEN) snprintf(b, sizeof b, "%s %s", lab, a); else snprintf(b, sizeof b, "%s", a);
    n = (int)strlen(b);
    if (n && b[n - 1] == '?' && chance(r, 25) && n + 16 <= VOICE_MAX_LEN) strcat(b, " AWAITING INPUT.");
    snprintf(t, (size_t)cap, "%s", b);
}

static void t_ghost(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    char a[700]; snprintf(a, sizeof a, "%s", in);
    int n = (int)strlen(a);
    while (n && (a[n - 1] == '.' || a[n - 1] == '!' || a[n - 1] == '?' || a[n - 1] == ' ' || a[n - 1] == ',')) a[--n] = 0;
    if (!light) {
        int nw = count_words(a);
        if (nw >= 5) {                                       /* break the sentence once, at 40-70 % of its words */
            int cutw = nw * (4 + rn_(r, 4)) / 10, w = 0;
            char b[700]; int o = 0, in_w = 0, done = 0;
            for (const char *p = a; *p && o < (int)sizeof b - 8; p++) {
                int wc = is_wc((unsigned char)*p);
                if (wc && !in_w) { w++; }
                if (!wc && in_w && w == cutw && !done) { b[o++] = ' '; b[o++] = '-'; done = 1; }
                in_w = wc;
                b[o++] = *p;
            }
            b[o] = 0;
            if (done) snprintf(a, sizeof a, "%s", b);
        }
    }
    const char *op = (!light && chance(r, 40)) || (light && chance(r, 50)) ? PICK(GHOST_OPEN, g, r) : NULL;
    int len = (int)strlen(a) + 2;
    if (op && len + 1 + (int)strlen(op) <= VOICE_MAX_LEN) snprintf(t, (size_t)cap, "%s %s -", op, a);
    else snprintf(t, (size_t)cap, "%s -", a);
}

static int lowerable(const char *s) {
    static const char *const W[] = { "you","the","we","it","this","that","there","what","why","who","how","so","now","but","and","then","if","when","they","he","she",
        "your","my","our","these","those","here","no","not","yes","do","does","did","are","is","can","will","come","go","stay","look","listen", NULL };
    for (int i = 0; W[i]; i++) { int L = (int)strlen(W[i]); if (starts_ci(s, W[i]) && !is_wc((unsigned char)s[L])) return 1; }
    return 0;
}
static void t_poet(const char *in, Rng *r, unsigned g, int light, char *t, int cap) {
    char a[700]; snprintf(a, sizeof a, "%s", in);
    if (!light && !strchr(a, ',')) {                          /* one rhythm cut: a comma in a long single sentence */
        int nw = count_words(a);
        if (nw > 10 && !strpbrk(a, ".!?") ) {
            int cutw = nw * 45 / 100, w = 0, o = 0, in_w = 0; char b[700];
            for (const char *p = a; *p && o < (int)sizeof b - 4; p++) {
                int wc = is_wc((unsigned char)*p);
                if (wc && !in_w) w++;
                if (!wc && in_w && w == cutw) b[o++] = ',';
                in_w = wc; b[o++] = *p;
            }
            b[o] = 0; snprintf(a, sizeof a, "%s", b);
        }
    }
    const char *op = chance(r, light ? 50 : 65) ? PICK(POET_OPEN, g, r) : NULL;
    if (op && (int)strlen(a) + 1 + (int)strlen(op) <= VOICE_MAX_LEN) {
        char o2[80]; snprintf(o2, sizeof o2, "%s", op);
        if (lowerable(a)) a[0] = (char)tolower((unsigned char)a[0]);
        else o2[strlen(o2) - 1] = '.';                       /* a name or a proper word follows: end the image with a full stop */
        snprintf(t, (size_t)cap, "%s %s", o2, a);
    } else snprintf(t, (size_t)cap, "%s", a);
}

static int run(int style, int tone, const char *text, char *out, int cap, int light) {
    if (cap <= 0) return 0;
    snprintf(out, (size_t)cap, "%s", text ? text : "");
    int v = voice_first(style);
    if (!text || v < STYLE_ELDER || v > STYLE_POET) return 0;
    int len = (int)strlen(text);
    if (len == 0 || len > VOICE_MAX_LEN) return 0;           /* already too long (the self-harm lines): never touched */
    Rng r; rng_init(&r, text, v, tone);
    unsigned g = group_of(tone);
    char t[700], f[700];
    switch (v) {
    case STYLE_ELDER:   t_elder(text, &r, g, light, t, (int)sizeof t); break;
    case STYLE_CHILD:   t_child(text, &r, g, light, t, (int)sizeof t); break;
    case STYLE_FORMAL:  t_formal(text, &r, g, light, t, (int)sizeof t); break;
    case STYLE_BEAST:   t_beast(text, &r, g, light, t, (int)sizeof t); break;
    case STYLE_MACHINE: t_machine(text, &r, g, light, t, (int)sizeof t); break;
    case STYLE_GHOST:   t_ghost(text, &r, g, light, t, (int)sizeof t); break;
    default:            t_poet(text, &r, g, light, t, (int)sizeof t); break;
    }
    if (finish(t, f, (int)sizeof f) == 0) return 0;          /* never empty: keep the original */
    if (!strcmp(f, text)) return 0;
    snprintf(out, (size_t)cap, "%s", f);
    g_applied++;
    return 1;
}

int voice_apply(int style, int tone, const char *text, char *out, int cap) { return run(style, tone, text, out, cap, 0); }
int voice_flavour(int style, int tone, const char *text, char *out, int cap) { return run(style, tone, text, out, cap, 1); }
