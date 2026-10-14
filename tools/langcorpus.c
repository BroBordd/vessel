/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the recall harness of the language engine (docs/convo_plan.md, chunk 3). no SDL:
 *   cc -O1 -Igame -o build/langcorpus tools/langcorpus.c game/lang*.c game/lines_*.c
 *   build/langcorpus tools/corpus/tuning_*.tsv                     per-intent recall / precision, unknown rate, 30 misses
 *   build/langcorpus --min-recall 90 tools/corpus/tuning_*.tsv     exit 1 if overall recall or any intent below 90 %
 *   build/langcorpus --only greet,bye FILE...                      score only lines that have one of these intents (a group gate)
 *   build/langcorpus --misses 100 FILE...                          show more misses
 * corpus file: one line per player text:  text <TAB> intent[,intent...]   (# = comment, none = nothing should be understood)
 * recall of intent X = lines labelled X where lang_parse heard X. precision of X = lines where X was heard and labelled.
 * exact = the heard set equals the labelled set (the sentiment words pos/neg are ignored unless labelled). */
#include "lang.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* same order as INTENT_NAME in lang.c (the assert catches a drift in length) */
static const char *const IN[] = {
    "greet", "bye", "yes", "no", "ok", "thanks", "sorry", "howru", "whoru", "where",
    "whoami", "mission", "why", "how", "what", "help", "insult", "praise", "love", "worship",
    "joke", "scared", "confused", "angry", "sad", "intro", "repeat", "sky", "music", "hole",
    "secret", "life", "wait", "please", "agree", "disagree", "dontcare", "demand", "callout", "age",
    "real", "address", "selfharm", "alex", "pos", "neg"
};
_Static_assert(sizeof IN / sizeof *IN == I_COUNT, "IN in langcorpus.c is out of date: see lang.c");

#define M(i) (1ull << (i))
#define MAXL 20000
typedef struct { char text[512]; uint64_t exp; int none; } Row;
static Row rows[MAXL]; static int nrows;

static int intent_by_name(const char *n) { for (int i = 0; i < I_COUNT; i++) if (!strcmp(IN[i], n)) return i; return -1; }

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
        snprintf(r->text, sizeof r->text, "%s", line); r->exp = 0; r->none = 0;
        for (char *tok = strtok(tab + 1, ","); tok; tok = strtok(NULL, ",")) {
            while (*tok == ' ') tok++;
            if (!strcmp(tok, "none")) { r->none = 1; continue; }
            int k = intent_by_name(tok);
            if (k < 0) { fprintf(stderr, "%s:%d: unknown intent '%s'\n", path, ln, tok); bad++; continue; }
            r->exp |= M(k);
        }
        nrows++;
    }
    fclose(f);
    return bad;
}

static void names(uint64_t m, char *out, int cap) {
    out[0] = 0; int o = 0;
    for (int i = 0; i < I_COUNT; i++) if (m & M(i)) o += snprintf(out + o, (size_t)(cap - o > 0 ? cap - o : 0), "%s%s", o ? "," : "", IN[i]);
    if (!o) snprintf(out, (size_t)cap, "-");
}

int main(int argc, char **argv) {
    int minrec = -1, nmiss = 30, bad = 0; uint64_t only = 0; const char *files[64]; int nf = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--min-recall") && i + 1 < argc) minrec = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--misses") && i + 1 < argc) nmiss = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--only") && i + 1 < argc) {
            char buf[400]; snprintf(buf, sizeof buf, "%s", argv[++i]);
            for (char *t = strtok(buf, ","); t; t = strtok(NULL, ",")) { int k = intent_by_name(t); if (k < 0) { fprintf(stderr, "unknown intent %s\n", t); return 2; } only |= M(k); }
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
        if (only && !(w->exp & only)) continue;
        Heard h; lang_parse(w->text, &h);
        uint64_t got = h.mask, cmp_got = got & ~(M(I_POS) | M(I_NEG)), cmp_exp = w->exp & ~(M(I_POS) | M(I_NEG));
        if (w->exp & M(I_POS)) cmp_got |= got & M(I_POS);
        if (w->exp & M(I_NEG)) cmp_got |= got & M(I_NEG);
        scored++;
        if (!h.n) unknown++;
        int miss = 0;
        if (w->none) { none_n++; if (!h.n) none_ok++; else miss = 1; }
        else {
            if (cmp_got == cmp_exp) exact++;
            for (int i = 0; i < I_COUNT; i++) {
                if (w->exp & M(i)) { exp_n[i]++; if (got & M(i)) hit[i]++; else miss = 1; }
                if (got & M(i)) { pred[i]++; if (w->exp & M(i)) ok_pred[i]++; }
            }
        }
        if (w->none) for (int i = 0; i < I_COUNT; i++) if (got & M(i)) pred[i]++;
        if (miss) {
            missed_rows++;
            if (shown < nmiss) { char e[120], g[120]; names(w->none ? 0 : w->exp, e, sizeof e); names(got, g, sizeof g); printf("  %-44s want %-22s got %s\n", w->text, w->none ? "none" : e, g); shown++; }
        }
    }
    int te = 0, th = 0, worst_bad = 0;
    printf("\n=== per intent: recall (heard / labelled), precision (right / heard) ===\n");
    for (int i = 0; i < I_COUNT; i++) {
        if (!exp_n[i] && !pred[i]) continue;
        double rc = exp_n[i] ? 100.0 * hit[i] / exp_n[i] : -1, pr = pred[i] ? 100.0 * ok_pred[i] / pred[i] : -1;
        int gated = !only || (only & M(i));          /* with --only, just those intents are held to the minimum */
        printf("  %-9s recall %5.1f%% (%d/%d)   precision %5.1f%% (%d/%d)%s\n", IN[i], rc, hit[i], exp_n[i], pr, ok_pred[i], pred[i],
               (minrec >= 0 && gated && exp_n[i] && rc < minrec) ? "   <-- below min" : "");
        if (gated) { te += exp_n[i]; th += hit[i]; }
        if (minrec >= 0 && gated && exp_n[i] && rc < minrec) worst_bad++;
    }
    double overall = te ? 100.0 * th / te : 0;
    printf("\nlines scored %d | overall recall %.1f%% (%d/%d labels) | exact-set %.1f%% | unknown rate %.1f%% (%d lines heard nothing)\n",
           scored, overall, th, te, scored - none_n ? 100.0 * exact / (scored - none_n) : 0, scored ? 100.0 * unknown / scored : 0, unknown);
    if (none_n) printf("none-lines (should be heard as nothing): %d/%d correct\n", none_ok, none_n);
    printf("lines with at least one miss or false hit on a none line: %d\n", missed_rows);
    if (minrec >= 0) { int fail = overall < minrec || worst_bad; printf("min-recall %d%%: %s\n", minrec, fail ? "FAIL" : "PASS"); return fail ? 1 : 0; }
    return 0;
}
