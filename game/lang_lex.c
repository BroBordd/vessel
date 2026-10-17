/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE LEXICON LAYER. see lang_lex.h */
#include "lang_lex.h"
#include "lang.h"
#include "lang_tables.h"
#include "lang_clause.h"
#include "lang_lemma.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int threshold = LEX_THRESHOLD;
void lang_lex_set_threshold(int t) { threshold = t > 0 ? t : LEX_THRESHOLD; }
int  lang_lex_threshold(void) { return threshold; }

static int is_word(int c) { return isalnum(c) || c == '\''; }

/* the intent and weight of one lexicon row "word|intent|weight"; -1 if the row is not an intent row */
static int row_intent(const char *row, int *weight) {
    const char *a = strchr(row, '|'); if (!a) return -1;
    const char *b = strchr(a + 1, '|');
    char name[48]; int n = b ? (int)(b - a - 1) : (int)strlen(a + 1);
    if (n < 1 || n > 40) return -1;
    memcpy(name, a + 1, (size_t)n); name[n] = 0;
    int in = lang_intent_by_name(name);
    if (in < 0 || in == I_POS || in == I_NEG || in == I_SELFHARM) return -1;      /* sentiment is chunk 28; the safety words stay phrases */
    int w = b ? atoi(b + 1) : 5;
    if (w < 1) w = 1;
    if (w > 9) w = 9;
    *weight = w;
    return in;
}

typedef struct { int intent, neg, score10, start, end, n, maxw, maxdisc; } Acc;

int lang_lex_scan(const char *lc, int len, const unsigned char *covered, LexHit *out, int max) {
    Clause cl[CLAUSE_MAX]; int ncl = lang_clauses(lc, len, cl, CLAUSE_MAX), nout = 0;
    if (ncl <= 0) return 0;
    for (int c = 0; c < ncl && nout < max; c++) {
        /* the words of the clause */
        int ws[64], we[64], nw = 0;
        for (int i = cl[c].a; i < cl[c].b && nw < 64;) {
            if (!is_word((unsigned char)lc[i])) { i++; continue; }
            int j = i; while (j < cl[c].b && is_word((unsigned char)lc[j])) j++;
            ws[nw] = i; we[nw] = j; nw++; i = j;
        }
        Acc acc[LEX_MAXHITS]; int na = 0;
        for (int k = 0; k < nw; k++) {
            int n = we[k] - ws[k];
            if (n < 1 || n > 30) continue;
            int cov = 0; if (covered) for (int q = ws[k]; q < we[k]; q++) if (covered[q]) { cov = 1; break; }
            if (cov) continue;                                           /* a phrase hit blocks the words it covers */
            char w[32]; memcpy(w, lc + ws[k], (size_t)n); w[n] = 0;
            const char *rows[12]; int nr = lang_lex_find(w, rows, 12);
            if (!nr) {                                                   /* the lemma: loved -> love */
                char lem[32]; if (lang_lemma(w, n, lem, sizeof lem) && strcmp(lem, w)) nr = lang_lex_find(lem, rows, 12);
            }
            int neg = lang_neg_count(lc, ws[k], cl[c].a) & 1;
            int late = (nw - k) <= 3;                                    /* the last three words count in full, the others at 80 % */
            for (int r = 0; r < nr; r++) {
                int wt, in = row_intent(rows[r], &wt); if (in < 0) continue;
                int s10 = wt * (late ? 10 : 8), a;
                for (a = 0; a < na; a++) if (acc[a].intent == in && acc[a].neg == neg) break;
                if (a == na) { if (na >= LEX_MAXHITS) continue; acc[na].intent = in; acc[na].neg = neg; acc[na].score10 = 0; acc[na].start = ws[k]; acc[na].end = we[k]; acc[na].n = 0; acc[na].maxw = 0; acc[na].maxdisc = 0; na++; }
                acc[a].score10 += s10; acc[a].end = we[k]; acc[a].n++;
                if (wt > acc[a].maxw) { acc[a].maxw = wt; acc[a].maxdisc = s10; }
            }
        }
        for (int a = 0; a < na && nout < max; a++) {
            int sc = (acc[a].score10 - acc[a].maxdisc + acc[a].maxw * 10 + 5) / 10;     /* the strongest word of an intent counts in full wherever it stands, the others by position */
            if (cl[c].boost > 0) sc += cl[c].boost;                      /* very, so, really ... */
            else if (cl[c].boost < 0 && sc > 0) sc += cl[c].boost;       /* a bit, kinda */
            if (sc < threshold) continue;
            out[nout].intent = acc[a].intent; out[nout].score = sc; out[nout].neg = acc[a].neg;
            out[nout].clause = c; out[nout].start = acc[a].start; out[nout].end = acc[a].end; nout++;
        }
    }
    return nout;
}

/* ================= chunk 28: sentiment with boosters ================= */

/* the valence of one word: the LAST "word|sent|N" row wins (a later file refines an earlier one); no row: the lemma (loved -> love) */
static int sent_row(const char *w, int *val) {
    const char *rows[16]; int nr = lang_lex_find(w, rows, 16), found = 0;
    for (int r = 0; r < nr; r++) {
        const char *a = strchr(rows[r], '|');
        if (!a || strncmp(a + 1, "sent|", 5)) continue;
        *val = atoi(a + 6); found = 1;
    }
    return found;
}
int lang_sent_value(const char *word, int n, int *val) {
    char w[32]; if (n < 1 || n > 30) return 0;
    memcpy(w, word, (size_t)n); w[n] = 0;
    if (sent_row(w, val)) return 1;
    char lem[32];
    if (lang_lemma(w, n, lem, sizeof lem) && strcmp(lem, w) && sent_row(lem, val)) return 1;
    return 0;
}

/* sarcasm cues. C_ALWAYS: the phrase alone is mocking. C_START: only at the start of a clause (as if). C_CTX: mocking only when the text also says
 * something negative (great job + idiot); on its own it is sincere */
enum { C_ALWAYS, C_START, C_CTX };
typedef struct { const char *p; int kind; int to; } Cue;
static const Cue CUES[] = {
    { "oh great", C_ALWAYS, -1 }, { "oh joy", C_ALWAYS, -1 }, { "oh goody", C_ALWAYS, -1 }, { "oh wonderful", C_ALWAYS, -1 },
    { "oh brilliant", C_ALWAYS, -1 }, { "oh fantastic", C_ALWAYS, -1 }, { "oh perfect", C_ALWAYS, -1 }, { "oh lovely", C_ALWAYS, -1 },
    { "just great", C_ALWAYS, -1 }, { "just perfect", C_ALWAYS, -1 }, { "just wonderful", C_ALWAYS, -1 }, { "how original", C_ALWAYS, -1 },
    { "real smooth", C_ALWAYS, -1 }, { "what a surprise", C_ALWAYS, -1 }, { "how nice", C_ALWAYS, -1 }, { "how lovely", C_ALWAYS, -1 },
    { "how wonderful", C_ALWAYS, -1 }, { "love that for me", C_ALWAYS, -1 },
    { "yeah right", C_ALWAYS, I_NO_WAY },       /* yeah sure and sure sure are NOT cues: players mean them (the tuning corpus has yeah sure as a yes) */
    { "as if", C_START, I_NO_WAY },
    { "wow thanks", C_ALWAYS, I_THANKS_SARCASTIC }, { "gee thanks", C_ALWAYS, I_THANKS_SARCASTIC }, { "thanks for nothing", C_ALWAYS, I_THANKS_SARCASTIC },
    { "thanks a lot", C_CTX, I_THANKS_SARCASTIC }, { "thanks a bunch", C_CTX, I_THANKS_SARCASTIC }, { "thanks so much", C_CTX, I_THANKS_SARCASTIC },
    { "thank you so much", C_CTX, I_THANKS_SARCASTIC },
    { "great job", C_CTX, -1 }, { "good job", C_CTX, -1 }, { "nice job", C_CTX, -1 }, { "great work", C_CTX, -1 }, { "good work", C_CTX, -1 },
    { "nice work", C_CTX, -1 }, { "nice going", C_CTX, -1 }, { "well done", C_CTX, -1 }, { "way to go", C_CTX, -1 }, { "nice one", C_CTX, -1 },
    { "good one", C_CTX, -1 }, { "bravo", C_CTX, -1 },
    /* the shapes that are not a phrase: filled in by name so the cue index says which fired */
    { "/s", C_ALWAYS, -1 }, { "sure.", C_ALWAYS, I_FINE_WHATEVER }, { "great.", C_ALWAYS, -1 }, { "positive word and ...", C_ALWAYS, -1 },
};
#define NCUES ((int)(sizeof CUES / sizeof CUES[0]))
#define CUE_SLASH_S (NCUES - 4)
#define CUE_PERIOD  (NCUES - 3)
#define CUE_PERIOD2 (NCUES - 2)
#define CUE_DOTS    (NCUES - 1)
int lang_sent_cue_count(void) { return NCUES; }
const char *lang_sent_cue_name(int i) { return i >= 0 && i < NCUES ? CUES[i].p : NULL; }

/* the phrase in lc at word boundaries (the phrase may hold spaces); the byte it starts at, or -1 */
static int phrase_find(const char *lc, int len, const char *ph) {
    int n = (int)strlen(ph);
    for (int i = 0; i + n <= len; i++) {
        if (memcmp(lc + i, ph, (size_t)n)) continue;
        if (i > 0 && is_word((unsigned char)lc[i - 1])) continue;
        if (i + n < len && is_word((unsigned char)lc[i + n])) continue;
        return i;
    }
    return -1;
}
/* oh great goddess, oh wonderful one: the game's way to address a god is no mockery. a cue followed by one of these words is skipped */
static int vocative_after(const char *lc, int len, int i) {
    static const char *const VOC[] = { "goddess", "one", "god", "lord", "lady", "dea", "dia", "alex", "mother", "father", "master", "spirit", "creator",
        "queen", "king", "holy", "mighty", "divine", "wise", "powerful", "almighty", "being", "deity", "ones", "dia", NULL };
    while (i < len && lc[i] == ' ') i++;
    int j = i; while (j < len && is_word((unsigned char)lc[j])) j++;
    for (int k = 0; VOC[k]; k++) if ((int)strlen(VOC[k]) == j - i && !memcmp(lc + i, VOC[k], (size_t)(j - i))) return 1;
    return 0;
}
static int at_clause_start(const char *lc, int i) {
    while (i > 0 && lc[i - 1] == ' ') i--;
    return i == 0 || strchr(".,!?;", lc[i - 1]) != NULL;
}

/* tenths of a point per emoticon / emoji / laugh / censored word token (at most two of a kind count). a lone smiley reaches the firing threshold of 30 */
static int token_value(int kind) {
    switch (kind) {
    case TK_POS: return 20; case TK_LOVE: return 30; case TK_LAUGH: return 10;
    case TK_NEG: return -20; case TK_ANGRY: return -25; case TK_CRY: return -20; case TK_MASK: return -30;
    default: return 0;
    }
}

int lang_sent_scan(const char *lc, int len, unsigned flags, int excl, int stretch, const unsigned char *tok, SentInfo *si) {
    memset(si, 0, sizeof *si); si->cue = -1; si->to = -1;
    Clause cl[CLAUSE_MAX]; int ncl = len > 0 ? lang_clauses(lc, len, cl, CLAUSE_MAX) : 0;
    int cp[CLAUSE_MAX] = {0}, cn[CLAUSE_MAX] = {0};
    for (int c = 0; c < ncl; c++) {
        for (int i = cl[c].a; i < cl[c].b;) {
            if (!is_word((unsigned char)lc[i])) { i++; continue; }
            int j = i; while (j < cl[c].b && is_word((unsigned char)lc[j])) j++;
            int v; if (lang_sent_value(lc + i, j - i, &v) && v) {
                int v10 = v * 10;
                if (lang_neg_count(lc, i, cl[c].a) & 1) v10 = -v10 * 3 / 4;        /* not good is a bit less bad than bad is, not bad is mildly good */
                if (v10 > 0) cp[c] += v10; else cn[c] += -v10;
                si->words++;
            }
            i = j;
        }
        int m = 100 + 30 * cl[c].boost;                                          /* very +30 %, very very +60 %, a bit -30 %, kinda kinda -60 % */
        cp[c] = cp[c] * m / 100; cn[c] = cn[c] * m / 100;
    }
    int from = 0;                                                                /* but: what follows the last contrast that says something wins */
    for (int c = 0; c < ncl; c++) if (cl[c].contrast && cp[c] + cn[c] > 0) from = c;
    int P = 0, N = 0;
    for (int c = from; c < ncl; c++) { P += cp[c]; N += cn[c]; }
    int bp = 100 + ((flags & LF_SHOUT) ? 25 : 0) + 10 * (excl > 3 ? 3 : excl) + (stretch >= 3 ? 20 : 0);     /* CAPS, !!! and sooo raise it */
    if (P + N > 0) { P = P * bp / 100; N = N * bp / 100; }
    int Nwords = N;                                                              /* the negative evidence of the WORDS: a :( or a skull after nice job is a sad face, not mockery */
    if (tok) for (int k = 1; k < TK_COUNT; k++) {
        int t = tok[k] > 2 ? 2 : tok[k], v = token_value(k) * t;
        if (v > 0) P += v; else N -= v;
    }
    si->evidence = P + N;
    /* sarcasm: the positive part is mocking, so it counts as negative. cues are looked for in the whole text */
    int cue = -1;
    int ctx_neg = Nwords >= 15;
    for (int i = 0; i < NCUES - 4 && cue < 0; i++) {
        int at = phrase_find(lc, len, CUES[i].p);
        if (at < 0) continue;
        if (CUES[i].kind == C_START && !at_clause_start(lc, at)) continue;
        if (vocative_after(lc, len, at + (int)strlen(CUES[i].p))) continue;
        if (CUES[i].kind == C_CTX && !ctx_neg) continue;
        cue = i;
    }
    int end = len; while (end > 0 && lc[end - 1] == ' ') end--;
    if (cue < 0 && end >= 2 && !strncmp(lc + end - 2, "/s", 2) && (end == 2 || lc[end - 3] == ' ' || is_word((unsigned char)lc[end - 3]))) cue = CUE_SLASH_S;
    if (cue < 0) {                                                               /* a lone positive word with a full stop: sure. great. */
        static const char *const ALONE[] = { "sure.", "yeah sure.", "great.", "wonderful.", "fantastic.", "brilliant.", "lovely.", NULL };
        for (int k = 0; ALONE[k]; k++) if (end == (int)strlen(ALONE[k]) && !memcmp(lc, ALONE[k], (size_t)end)) { cue = k < 2 ? CUE_PERIOD : CUE_PERIOD2; break; }
    }
    if (cue < 0 && end >= 4 && !strncmp(lc + end - 3, "...", 3) && P >= 20 && P > N && !(tok && (tok[TK_LOVE] || tok[TK_POS]))) {
        int w = 0, bad = 0;                                                      /* nice... great... wonderful...: ONE word, and not a trailing thanks... or a tender word */
        for (int i = 0; i < end;) {
            if (!is_word((unsigned char)lc[i])) { i++; continue; }
            int j = i; while (j < end && is_word((unsigned char)lc[j])) j++;
            if (j - i >= 5 && !strncmp(lc + i, "thank", 5)) bad = 1;
            w++; i = j;
        }
        if (w == 1 && !bad) cue = CUE_DOTS;
    }
    if (cue >= 0) {
        si->sarcasm = 1; si->cue = cue; si->to = CUES[cue].to; si->ctx = cue < NCUES - 4 && CUES[cue].kind == C_CTX;
        N += P; P = 0; if (N < 30) N = 30;
    }
    si->p10 = P; si->n10 = N;
    si->pos = (P + 5) / 10 > 9 ? 9 : (P + 5) / 10;
    si->neg = (N + 5) / 10 > 9 ? 9 : (N + 5) / 10;
    int v = (P - N) * 3 / 2;                                                    /* one word of 3 is 45, two strong words reach 100 */
    si->valence = v > 100 ? 100 : v < -100 ? -100 : v;
    return si->valence;
}

/* ================= chunk 29: topic words ================= */
#define TOPIC_NAMES 400
static char tnames[TOPIC_NAMES][20]; static int ntn;
const char *lang_topic_name(int id) { return id >= 0 && id < ntn ? tnames[id] : NULL; }
int lang_topic_id(const char *name) { for (int i = 0; i < ntn; i++) if (!strcmp(tnames[i], name)) return i; return -1; }
static int topic_intern(const char *name, int n) {
    if (n < 1 || n > 19) return -1;
    char b[20]; memcpy(b, name, (size_t)n); b[n] = 0;
    int id = lang_topic_id(b); if (id >= 0) return id;
    if (ntn >= TOPIC_NAMES) return -1;
    memcpy(tnames[ntn], b, (size_t)n + 1); return ntn++;
}
/* the topic rows of a word: calls back with (topic id, weight); 0 rows = none */
static int topic_rows(const char *w, int *ids, int *wts, int max) {
    const char *rows[16]; int nr = lang_lex_find(w, rows, 16), k = 0;
    for (int r = 0; r < nr && k < max; r++) {
        const char *a = strchr(rows[r], '|');
        if (!a || strncmp(a + 1, "topic.", 6)) continue;
        const char *b = strchr(a + 7, '|'); if (!b) continue;
        int id = topic_intern(a + 7, (int)(b - a - 7)); if (id < 0) continue;
        ids[k] = id; wts[k] = atoi(b + 1); k++;
    }
    return k;
}
int lang_topic_scan(const char *lc, int len, TopicHit *out, int max) {
    int score[TOPIC_NAMES + 1] = {0}, best[TOPIC_NAMES + 1] = {0}, touched[64], nt = 0;
    static char word[TOPIC_NAMES + 1][24];
    for (int i = 0; i < len;) {
        if (!is_word((unsigned char)lc[i])) { i++; continue; }
        int j = i; while (j < len && is_word((unsigned char)lc[j])) j++;
        int n = j - i;
        if (n >= 2 && n <= 23) {
            char w[32]; memcpy(w, lc + i, (size_t)n); w[n] = 0;
            int ids[8], wts[8], k = topic_rows(w, ids, wts, 8);
            if (!k) { char lem[32]; if (lang_lemma(w, n, lem, sizeof lem) && strcmp(lem, w)) k = topic_rows(lem, ids, wts, 8); }
            for (int q = 0; q < k; q++) {
                int id = ids[q];
                if (!score[id] && nt < 64) touched[nt++] = id;
                score[id] += wts[q];
                if (wts[q] > best[id]) { best[id] = wts[q]; memcpy(word[id], w, (size_t)n + 1); }
            }
        }
        i = j;
    }
    int no = 0;                                                  /* the strongest max topics that reach the minimum (ties: the one met first) */
    for (int r = 0; r < max; r++) {
        int bi = -1;
        for (int t = 0; t < nt; t++) { int id = touched[t]; if (score[id] >= TOPIC_MIN && (bi < 0 || score[id] > score[bi])) bi = id; }
        if (bi < 0) break;
        out[no].id = bi; out[no].score = score[bi]; snprintf(out[no].word, sizeof out[no].word, "%s", word[bi]); no++;
        score[bi] = 0;
    }
    return no;
}
