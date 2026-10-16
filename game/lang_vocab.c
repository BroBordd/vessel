/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * The vocabulary queries (docs/convo_plan.md, chunk 20). See lang_vocab.h. The hashes live in lang_tables.c. */
#include "lang_vocab.h"
#include "lang_tables.h"
#include <string.h>

#define VW 40

/* lower case copy of w[0..n) into b (VW bytes); returns the length, 0 if it is empty or too long */
static int norm(const char *w, int n, char *b) {
    if (!w || n < 1 || n >= VW) return 0;
    for (int i = 0; i < n; i++) { int c = (unsigned char)w[i]; b[i] = (char)(c >= 'A' && c <= 'Z' ? c + 32 : c); }
    b[n] = 0;
    return n;
}

static const char *row_of(const char *w, int n) {
    char b[VW];
    if (!norm(w, n, b)) return NULL;
    return lang_vocab_find(b);
}

int vocab_has_n(const char *w, int n) { return row_of(w, n) != NULL; }
int vocab_has(const char *w) { return w ? vocab_has_n(w, (int)strlen(w)) : 0; }

int vocab_rank(const char *w) {
    const char *r = w ? row_of(w, (int)strlen(w)) : NULL, *bar;
    if (!r || !(bar = strchr(r, '|'))) return 0;
    int v = 0;
    for (bar++; *bar >= '0' && *bar <= '9'; bar++) v = v * 10 + (*bar - '0');
    return v;
}

char vocab_pos(const char *w) {
    const char *r = w ? row_of(w, (int)strlen(w)) : NULL, *bar;
    if (!r) return 0;
    bar = strchr(r, '|'); if (bar) bar = strchr(bar + 1, '|');
    return (bar && bar[1]) ? bar[1] : 'x';
}

int vocab_is_name_n(const char *w, int n) {
    char b[VW];
    return norm(w, n, b) && lang_name_find(b) != NULL;
}
int vocab_is_name(const char *w) { return w ? vocab_is_name_n(w, (int)strlen(w)) : 0; }

int vocab_protected(const char *w, int n) { return vocab_has_n(w, n) || vocab_is_name_n(w, n); }

int vocab_size(void) { return lang_vocab_count(); }
int vocab_name_count(void) { return lang_name_count(); }
