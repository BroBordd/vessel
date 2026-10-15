/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the recall harness of the language engine (docs/convo_plan.md, chunk 3). no SDL:
 *   cc -O1 -Igame -o build/langcorpus tools/langcorpus.c game/lang*.c game/lines_*.c
 *   build/langcorpus tools/corpus/tuning_*.tsv                     per-intent recall / precision, unknown rate, 30 misses
 *   build/langcorpus --min-recall 90 tools/corpus/tuning_*.tsv     exit 1 if overall recall or any intent below 90 %
 *   build/langcorpus --only greet,bye FILE...                      score only lines that have one of these intents (a group gate)
 *   build/langcorpus --misses 100 FILE...                          show more misses
 *   build/langcorpus --fuzz 5 FILE...                              also mutate every line 5 times per kind (typos, caps, elongation, leet,
 *                                                                  spaced letters, punctuation, emoji, fillers, glued words, junk, combos)
 *                                                                  and print how much recall is RETAINED: fuzzed recall / clean recall
 *   build/langcorpus --fuzz 5 --min-retained 93 FILE...            exit 1 if the overall retained recall is below 93 %
 *   build/langcorpus --fuzz 5 --fuzz-misses 40 FILE...              show more lost labels (default 15). the seed is fixed (--seed N to change)
 * corpus file: one line per player text:  text <TAB> intent[,intent...]   (# = comment, none = nothing should be understood)
 * recall of intent X = lines labelled X where lang_parse heard X. precision of X = lines where X was heard and labelled.
 * exact = the heard set equals the labelled set (the sentiment words pos/neg are ignored unless labelled). */
#include "lang.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IN_(i) lang_intent_name(i)
/* a set of intents (chunk 12: 380 intents do not fit a uint64_t any more) */
typedef struct { uint64_t w[HEARD_MAX / 64]; } Set;
static int  sh(const Set *s, int i) { return (int)((s->w[i >> 6] >> (i & 63)) & 1u); }
static void sa(Set *s, int i) { s->w[i >> 6] |= 1ull << (i & 63); }
static int  s_any(const Set *s) { for (int k = 0; k < HEARD_MAX / 64; k++) if (s->w[k]) return 1; return 0; }
static int  s_meet(const Set *a, const Set *b) { for (int k = 0; k < HEARD_MAX / 64; k++) if (a->w[k] & b->w[k]) return 1; return 0; }
static int  s_eq(const Set *a, const Set *b) { return !memcmp(a->w, b->w, sizeof a->w); }
static Set  s_of(const Heard *h) { Set s; memcpy(s.w, h->m, sizeof s.w); return s; }
#define MAXL 20000
typedef struct { char text[512]; Set exp; int none; } Row;
static Row rows[MAXL]; static int nrows;

static int intent_by_name(const char *n) { return lang_intent_by_name(n); }

static int load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 1; }
    char line[512]; int ln = 0, bad = 0;
    while (fgets(line, sizeof line, f)) {
        ln++;
        size_t n = strlen(line); while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
        if (!n || line[0] == '#') continue;
        char *tab = strchr(line, '\t');
        if (!tab) { fprintf(stderr, "%s:%d: no TAB\n", path, ln); bad++; continue; }
        *tab = 0;
        if (nrows >= MAXL) { fprintf(stderr, "too many rows\n"); fclose(f); return 1; }
        Row *r = &rows[nrows];
        snprintf(r->text, sizeof r->text, "%s", line); memset(&r->exp, 0, sizeof r->exp); r->none = 0;
        for (char *tok = strtok(tab + 1, ","); tok; tok = strtok(NULL, ",")) {
            while (*tok == ' ') tok++;
            if (!strcmp(tok, "none")) { r->none = 1; continue; }
            int k = intent_by_name(tok);
            if (k < 0) { fprintf(stderr, "%s:%d: unknown intent '%s'\n", path, ln, tok); bad++; continue; }
            sa(&r->exp, k);
        }
        nrows++;
    }
    fclose(f);
    return bad;
}

static void names(const Set *m, char *out, int cap) {
    out[0] = 0; int o = 0;
    for (int i = 0; i < I_COUNT; i++) if (sh(m, i)) o += snprintf(out + o, (size_t)(cap - o > 0 ? cap - o : 0), "%s%s", o ? "," : "", IN_(i));
    if (!o) snprintf(out, (size_t)cap, "-");
}

/* ---------- the fuzzer (chunk 4) ---------- */
static uint32_t fz = 12345u;
static uint32_t frnd(void) { fz ^= fz << 13; fz ^= fz >> 17; fz ^= fz << 5; return fz; }
static int fn(int n) { return n > 0 ? (int)(frnd() % (uint32_t)n) : 0; }

enum { F_SWAP, F_DROP, F_DOUBLE, F_NEIGH, F_CAPS, F_STRETCH, F_LEET, F_SPACED, F_PUNCT, F_EMOJI, F_FILLER, F_GLUED, F_JUNK, F_COMBO, F_COUNT };
static const char *const FNAME[F_COUNT] = { "typo-swap", "typo-drop", "typo-double", "typo-neighbour", "caps", "stretch", "leet", "spaced",
                                            "punct", "emoji", "filler", "glued", "junk", "combo(2-3)" };

/* a random word of at least minlen letters: its start and length (returns 0 if none) */
static int pick_word(const char *t, int minlen, int *st, int *ln) {
    int cs[64], cl[64], n = 0, i = 0, len = (int)strlen(t);
    while (i < len && n < 64) {
        while (i < len && !isalpha((unsigned char)t[i])) i++;
        int a = i; while (i < len && isalpha((unsigned char)t[i])) i++;
        if (i - a >= minlen) { cs[n] = a; cl[n] = i - a; n++; }
    }
    if (!n) return 0;
    int k = fn(n); *st = cs[k]; *ln = cl[k]; return 1;
}

static char neighbour(char c) {
    static const char *ROW[3] = { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
    for (int r = 0; r < 3; r++) {
        const char *p = strchr(ROW[r], c);
        if (!p) continue;
        int i = (int)(p - ROW[r]), len = (int)strlen(ROW[r]);
        char cand[6]; int n = 0;
        if (i > 0) cand[n++] = ROW[r][i - 1];
        if (i < len - 1) cand[n++] = ROW[r][i + 1];
        if (r > 0 && i < (int)strlen(ROW[r - 1])) cand[n++] = ROW[r - 1][i];
        if (r < 2 && i < (int)strlen(ROW[r + 1])) cand[n++] = ROW[r + 1][i];
        return cand[fn(n)];
    }
    return c;
}

static void ins(char *t, int cap, int at, const char *s) {
    int len = (int)strlen(t), sl = (int)strlen(s);
    if (len + sl >= cap) return;
    memmove(t + at + sl, t + at, (size_t)(len - at + 1)); memcpy(t + at, s, (size_t)sl);
}

/* apply one mutation in place. returns 1 if the text changed */
static int mutate(char *t, int cap, int kind) {
    char before[512]; snprintf(before, sizeof before, "%s", t);
    int st, ln, len = (int)strlen(t);
    switch (kind) {
    case F_SWAP:   if (pick_word(t, 3, &st, &ln)) { int k = st + fn(ln - 1); char c = t[k]; t[k] = t[k + 1]; t[k + 1] = c; } break;
    case F_DROP:   if (pick_word(t, 4, &st, &ln)) { int k = st + fn(ln); memmove(t + k, t + k + 1, (size_t)(len - k)); } break;
    case F_DOUBLE: if (pick_word(t, 3, &st, &ln)) { int k = st + fn(ln); char s2[2] = { t[k], 0 }; ins(t, cap, k, s2); } break;
    case F_NEIGH:  if (pick_word(t, 3, &st, &ln)) { int k = st + fn(ln); t[k] = neighbour((char)tolower((unsigned char)t[k])); } break;
    case F_CAPS:   for (int i = 0; i < len; i++) t[i] = (char)toupper((unsigned char)t[i]); break;
    case F_STRETCH: if (pick_word(t, 2, &st, &ln)) {          /* repeat the last vowel (else the last letter) of a word 3 more times */
            int k = st + ln - 1; for (int i = st + ln - 1; i >= st; i--) if (strchr("aeiou", tolower((unsigned char)t[i]))) { k = i; break; }
            char s3[4] = { t[k], t[k], t[k], 0 }; ins(t, cap, k, s3); } break;
    case F_LEET:   for (int i = 0; i < len; i++) { char c = (char)tolower((unsigned char)t[i]);
            if (fn(10) < 7) { if (c == 'o') t[i] = '0'; else if (c == 'e') t[i] = '3'; else if (c == 'i') t[i] = '1'; else if (c == 'a') t[i] = '4'; } } break;
    case F_SPACED: if (pick_word(t, 3, &st, &ln) && ln <= 8) { for (int i = st + ln - 1; i > st; i--) ins(t, cap, i, " "); } break;
    case F_PUNCT:  { static const char *const P[] = { "!!!", "???", "!?", "...", "!!!!!!", "??" }; ins(t, cap, len, P[fn(6)]); } break;
    case F_EMOJI:  { static const char *const E[] = { " :)", " :(", " <3", " xD", " \xF0\x9F\x98\x8A", " \xF0\x9F\x98\xAD", " \xF0\x9F\x92\x80" }; ins(t, cap, len, E[fn(7)]); } break;
    case F_FILLER: { static const char *const Fl[] = { "um ", "uh ", "so ", "well ", "umm ok so ", "like ", "hmm " }; ins(t, cap, 0, Fl[fn(7)]); } break;
    case F_GLUED:  { int o = 0; for (int i = 0; i < len; i++) if (t[i] != ' ') t[o++] = t[i]; t[o] = 0; } break;
    case F_JUNK:   { static const char *const L[] = { "  ", " - ", "... ", ">> ", "\" " }, *const R[] = { "  ", " -", " ..", " <<", " \"" };
                     ins(t, cap, 0, L[fn(5)]); ins(t, cap, (int)strlen(t), R[fn(5)]); } break;
    }
    return strcmp(before, t) != 0;
}

static Set only;
static int run_fuzz(int per, int nshow, int minret) {
    long lab[F_COUNT] = {0}, cl[F_COUNT] = {0}, fu[F_COUNT] = {0}, kept[F_COUNT] = {0}; int nvar[F_COUNT] = {0};
    long ilab[I_COUNT] = {0}, icl[I_COUNT] = {0}, ifu[I_COUNT] = {0};
    long tl = 0, tc = 0, tf = 0, tk = 0; int shown = 0;
    printf("\n=== fuzz: %d variants per line per kind, seed fixed ===\n", per);
    for (int r = 0; r < nrows; r++) {
        Row *w = &rows[r];
        if (w->none || !s_any(&w->exp) || (s_any(&only) && !s_meet(&w->exp, &only))) continue;
        Heard h0; lang_parse(w->text, &h0);
        for (int k = 0; k < F_COUNT; k++) for (int v = 0; v < per; v++) {
            char t[512]; strncpy(t, w->text, sizeof t - 1); t[sizeof t - 1] = 0;
            int changed;
            if (k == F_COMBO) { changed = 0; int n = 2 + fn(2); for (int j = 0; j < n; j++) { int kk = fn(F_COMBO); if (kk == F_GLUED) kk = F_NEIGH; changed |= mutate(t, 199, kk); } }
            else changed = mutate(t, 199, k);
            if (!changed) continue;
            Heard h; lang_parse(t, &h); nvar[k]++;
            for (int i = 0; i < I_COUNT; i++) if (sh(&w->exp, i)) {
                int c = heard_has(&h0, i), f = heard_has(&h, i);
                lab[k]++; cl[k] += c; fu[k] += f; kept[k] += c && f; tk += c && f; tl++; tc += c; tf += f;
                ilab[i]++; icl[i] += c; ifu[i] += f;
                if (c && !f && shown < nshow) { printf("  lost %-8s %-40s <- %-30s [%s]\n", IN_(i), t, w->text, FNAME[k]); shown++; }
            }
        }
    }
    printf("\n%-16s %8s %8s %8s %9s %8s\n", "kind", "labels", "clean", "fuzzed", "retained", "kept");
    for (int k = 0; k < F_COUNT; k++) {
        if (!lab[k]) continue;
        printf("%-16s %8ld %7.1f%% %7.1f%% %8.1f%% %7.1f%%\n", FNAME[k], lab[k], 100.0 * cl[k] / lab[k], 100.0 * fu[k] / lab[k],
               cl[k] ? 100.0 * fu[k] / cl[k] : 0, cl[k] ? 100.0 * kept[k] / cl[k] : 0);
    }
    double ret = tc ? 100.0 * tf / tc : 0;
    printf("%-16s %8ld %7.1f%% %7.1f%% %8.1f%% %7.1f%%   (retained = fuzzed recall / clean recall; kept = labels heard clean that are still heard)\n",
           "ALL", tl, tl ? 100.0 * tc / tl : 0, tl ? 100.0 * tf / tl : 0, ret, tc ? 100.0 * tk / tc : 0.0);
    printf("\nper intent (retained = fuzzed hits / clean hits over all mutations):\n");
    for (int i = 0; i < I_COUNT; i++) if (ilab[i]) printf("  %-9s clean %5.1f%%  fuzzed %5.1f%%  retained %5.1f%%\n", IN_(i), 100.0 * icl[i] / ilab[i], 100.0 * ifu[i] / ilab[i], icl[i] ? 100.0 * ifu[i] / icl[i] : 0);
    if (minret >= 0) { printf("min-retained %d%%: %s\n", minret, ret < minret ? "FAIL" : "PASS"); return ret < minret; }
    return 0;
}

int main(int argc, char **argv) {
    int minrec = -1, nmiss = 30, bad = 0, fuzz = 0, fuzzshow = 15, minret = -1; memset(&only, 0, sizeof only); const char *files[64]; int nf = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--min-recall") && i + 1 < argc) minrec = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--misses") && i + 1 < argc) nmiss = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fuzz") && i + 1 < argc) fuzz = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fuzz-misses") && i + 1 < argc) fuzzshow = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--min-retained") && i + 1 < argc) minret = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--seed") && i + 1 < argc) fz = (uint32_t)atoi(argv[++i]) | 1u;
        else if (!strcmp(argv[i], "--only") && i + 1 < argc) {
            char buf[400]; snprintf(buf, sizeof buf, "%s", argv[++i]);
            for (char *t = strtok(buf, ","); t; t = strtok(NULL, ",")) { int k = intent_by_name(t); if (k < 0) { fprintf(stderr, "unknown intent %s\n", t); return 2; } sa(&only, k); }
        } else if (nf < 64) files[nf++] = argv[i];
    }
    if (!nf) { fprintf(stderr, "usage: langcorpus [--min-recall N] [--only a,b] [--misses N] FILE...\n"); return 2; }
    for (int i = 0; i < nf; i++) bad += load(files[i]);
    if (bad) return 2;

    lang_seed(1);
    int exp_n[I_COUNT] = {0}, hit[I_COUNT] = {0}, pred[I_COUNT] = {0}, ok_pred[I_COUNT] = {0};
    int scored = 0, unknown = 0, exact = 0, none_n = 0, none_ok = 0, missed_rows = 0;
    int shown = 0;
    printf("=== misses (first %d) ===\n", nmiss);
    for (int r = 0; r < nrows; r++) {
        Row *w = &rows[r];
        if (s_any(&only) && !s_meet(&w->exp, &only)) continue;
        Heard h; lang_parse(w->text, &h);
        Set got = s_of(&h), cmp_got = got, cmp_exp = w->exp;
        cmp_got.w[I_POS >> 6] &= ~(1ull << (I_POS & 63)); cmp_got.w[I_NEG >> 6] &= ~(1ull << (I_NEG & 63));      /* the sentiment words only count when expected */
        cmp_exp.w[I_POS >> 6] &= ~(1ull << (I_POS & 63)); cmp_exp.w[I_NEG >> 6] &= ~(1ull << (I_NEG & 63));
        if (sh(&w->exp, I_POS) && sh(&got, I_POS)) sa(&cmp_got, I_POS);
        if (sh(&w->exp, I_NEG) && sh(&got, I_NEG)) sa(&cmp_got, I_NEG);
        scored++;
        if (!h.n) unknown++;
        int miss = 0;
        if (w->none) { none_n++; if (!h.n) none_ok++; else miss = 1; }
        else {
            if (s_eq(&cmp_got, &cmp_exp)) exact++;
            for (int i = 0; i < I_COUNT; i++) {
                if (sh(&w->exp, i)) { exp_n[i]++; if (sh(&got, i)) hit[i]++; else miss = 1; }
                if (sh(&got, i)) { pred[i]++; if (sh(&w->exp, i)) ok_pred[i]++; }
            }
        }
        if (w->none) for (int i = 0; i < I_COUNT; i++) if (sh(&got, i)) pred[i]++;
        if (miss) {
            missed_rows++;
            if (shown < nmiss) { char e[120], g[120]; Set nil; memset(&nil, 0, sizeof nil); names(w->none ? &nil : &w->exp, e, sizeof e); names(&got, g, sizeof g); printf("  %-44s want %-22s got %s\n", w->text, w->none ? "none" : e, g); shown++; }
        }
    }
    int te = 0, th = 0, worst_bad = 0;
    printf("\n=== per intent: recall (heard / labelled), precision (right / heard) ===\n");
    for (int i = 0; i < I_COUNT; i++) {
        if (!exp_n[i] && !pred[i]) continue;
        double rc = exp_n[i] ? 100.0 * hit[i] / exp_n[i] : -1, pr = pred[i] ? 100.0 * ok_pred[i] / pred[i] : -1;
        int gated = !s_any(&only) || sh(&only, i);          /* with --only, just those intents are held to the minimum */
        printf("  %-9s recall %5.1f%% (%d/%d)   precision %5.1f%% (%d/%d)%s\n", IN_(i), rc, hit[i], exp_n[i], pr, ok_pred[i], pred[i],
               (minrec >= 0 && gated && exp_n[i] && rc < minrec) ? "   <-- below min" : "");
        if (gated) { te += exp_n[i]; th += hit[i]; }
        if (minrec >= 0 && gated && exp_n[i] && rc < minrec) worst_bad++;
    }
    double overall = te ? 100.0 * th / te : 0;
    printf("\nlines scored %d | overall recall %.1f%% (%d/%d labels) | exact-set %.1f%% | unknown rate %.1f%% (%d lines heard nothing)\n",
           scored, overall, th, te, scored - none_n ? 100.0 * exact / (scored - none_n) : 0, scored ? 100.0 * unknown / scored : 0, unknown);
    if (none_n) printf("none-lines (should be heard as nothing): %d/%d correct\n", none_ok, none_n);
    printf("lines with at least one miss or false hit on a none line: %d\n", missed_rows);
    int rc = 0;
    if (minrec >= 0) { int fail = overall < minrec || worst_bad; printf("min-recall %d%%: %s\n", minrec, fail ? "FAIL" : "PASS"); rc |= fail; }
    if (fuzz > 0) rc |= run_fuzz(fuzz, fuzzshow, minret);
    return rc;
}
