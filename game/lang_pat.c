/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * PATTERN SYNTAX V2: the macro table, the compiler and the matcher (docs/convo_plan.md, chunk 26). No SDL. See lang_pat.h. */
#include "lang_pat.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---------- the arena ---------- */
#define ARENA_BLOCK 16384
typedef struct Block { struct Block *next; size_t used, cap; char mem[]; } Block;
static Block *arena;
static void *ar_alloc(size_t n) {
    n = (n + 7u) & ~(size_t)7u;
    if (!arena || arena->used + n > arena->cap) {
        size_t cap = n > ARENA_BLOCK ? n : ARENA_BLOCK;
        Block *b = malloc(sizeof *b + cap);
        if (!b) return NULL;
        b->next = arena; b->used = 0; b->cap = cap; arena = b;
    }
    void *p = arena->mem + arena->used; arena->used += n;
    return p;
}
static char *ar_str(const char *s, int n) { char *d = ar_alloc((size_t)n + 1); if (d) { memcpy(d, s, (size_t)n); d[n] = 0; } return d; }

/* ---------- nodes ---------- */
enum { LN_LIT, LN_STAR, LN_Q, LN_END, LN_ALT, LN_OPT, LN_ANY };
enum { LNF_DIGITS = 1, LNF_TIME = 2, LNF_NPC = 4 };
typedef struct LNode LNode;
typedef struct { const LNode *n; int cnt; } LSeq;
struct LNode { uint8_t kind, cross, flags, min, max; int len; const char *s; const LSeq *alts; int nalts; const char *name; };
struct LPat { LSeq top; int nstars; };

#define MAX_NODES 160            /* per sequence */
#define MAX_DEPTH 6
#define MAX_STEPS 40000          /* backtracking budget of one match */

/* ---------- macros ---------- */
#define MAX_MACROS 512
typedef struct { char name[24]; LSeq *alts; int nalts; const char **text; uint8_t flags; } Macro;
static Macro macros[MAX_MACROS]; static int nmacros;
static char npc_name[40];

void lpat_reset(void) {
    while (arena) { Block *n = arena->next; free(arena); arena = n; }
    nmacros = 0; memset(macros, 0, sizeof macros);
}
void lpat_set_npc(const char *name) {
    int i = 0;
    if (name) for (; name[i] && i < (int)sizeof npc_name - 1; i++) npc_name[i] = (char)tolower((unsigned char)name[i]);
    npc_name[i] = 0;
}

static int macro_find(const char *name, int n) {
    for (int i = 0; i < nmacros; i++) if ((int)strlen(macros[i].name) == n && !memcmp(macros[i].name, name, (size_t)n)) return i;
    return -1;
}
static uint8_t macro_flags(const char *name) { return !strcmp(name, "number") ? LNF_DIGITS : !strcmp(name, "time") ? LNF_TIME : !strcmp(name, "npc") ? LNF_NPC : 0; }

int lpat_macro_add(const char *row) {
    if (!row) return -1;
    const char *p = row; if (*p == '<') p++;
    int n = 0; while (p[n] && p[n] != '|' && p[n] != '>' && n < 23) n++;
    if (n < 1) return -1;
    for (int i = 0; i < n; i++) if (!(islower((unsigned char)p[i]) || isdigit((unsigned char)p[i]) || p[i] == '_')) return -1;
    char name[24]; memcpy(name, p, (size_t)n); name[n] = 0;
    p += n; if (*p == '>') p++;
    if (*p == '|') p++; else if (*p) return -1;
    int na = 0;                                           /* count the alternatives */
    for (const char *q = p; *q || q == p; ) { na++; while (*q && *q != '|') q++; if (!*q) break; q++; }
    if (!*p) na = 0;
    int slot = macro_find(name, n);
    if (slot < 0) { if (nmacros >= MAX_MACROS) return -1; slot = nmacros++; }
    Macro *m = &macros[slot];
    memset(m, 0, sizeof *m);
    memcpy(m->name, name, (size_t)n + 1); m->flags = macro_flags(name);
    m->nalts = na;
    if (na) {
        m->alts = ar_alloc((size_t)na * sizeof *m->alts); m->text = ar_alloc((size_t)na * sizeof *m->text);
        LNode *nodes = ar_alloc((size_t)na * sizeof *nodes);
        if (!m->alts || !m->text || !nodes) { m->nalts = 0; return -1; }
        const char *q = p;
        for (int i = 0; i < na; i++) {
            int len = 0; while (q[len] && q[len] != '|') len++;
            memset(&nodes[i], 0, sizeof nodes[i]);
            nodes[i].kind = LN_LIT; nodes[i].len = len; nodes[i].s = ar_str(q, len);
            m->alts[i].n = &nodes[i]; m->alts[i].cnt = len ? 1 : 0;
            m->text[i] = nodes[i].s;
            q += len; if (*q == '|') q++;
        }
    }
    return 0;
}
int lpat_macro_count(void) { return nmacros; }
const char *lpat_macro_name(int i) { return i >= 0 && i < nmacros ? macros[i].name : NULL; }
int lpat_macro_alts(int i) { return i >= 0 && i < nmacros ? macros[i].nalts : 0; }
const char *lpat_macro_alt(int i, int j) { return i >= 0 && i < nmacros && j >= 0 && j < macros[i].nalts ? macros[i].text[j] : NULL; }

/* ---------- text helpers (the same rules as lang.c) ---------- */
static int is_word(int c) { return isalnum(c) || c == '\''; }
static int is_stop(int c) { return c == '.' || c == '!' || c == '?' || c == ',' || c == 0; }
static int at_boundary(const char *base, const char *s) {
    if (s == base || !*s) return 1;
    return !(is_word((unsigned char)s[-1]) && is_word((unsigned char)*s));
}

int lpat_is_v2(const char *src) { for (; *src; src++) if (*src == '[' || *src == '(' || *src == '<') return 1; return 0; }

/* ---------- the compiler ---------- */
typedef struct { const char *src; int pos, nodes; const char *err; } Parse;
typedef struct { LNode *a; int n, cap; } NodeBuf;

static LNode *nb_push(Parse *P, NodeBuf *b) {
    if (b->n >= MAX_NODES) { P->err = "pattern too big"; return NULL; }
    if (b->n == b->cap) { int nc = b->cap ? b->cap * 2 : 8; LNode *na = realloc(b->a, (size_t)nc * sizeof *na); if (!na) { P->err = "out of memory"; return NULL; } b->a = na; b->cap = nc; }
    LNode *n = &b->a[b->n++]; memset(n, 0, sizeof *n); P->nodes++;
    return n;
}
static LSeq seq_from(Parse *P, NodeBuf *b) {
    LSeq s = { NULL, 0 };
    if (P->err || !b->n) { free(b->a); return s; }
    LNode *d = ar_alloc((size_t)b->n * sizeof *d);
    if (!d) { P->err = "out of memory"; free(b->a); return s; }
    memcpy(d, b->a, (size_t)b->n * sizeof *d); s.n = d; s.cnt = b->n; free(b->a);
    return s;
}
static void lit_flush(Parse *P, NodeBuf *b, char *lit, int *ll) {
    if (!*ll) return;
    LNode *n = nb_push(P, b);
    if (n) { n->kind = LN_LIT; n->len = *ll; n->s = ar_str(lit, *ll); }
    *ll = 0;
}
/* mode 0 top level, 1 inside [ ] (stops at | or ]), 2 inside ( ) (stops at ) ) */
static LSeq parse_seq(Parse *P, int mode, int depth, int *nstars) {
    NodeBuf b = { NULL, 0, 0 };
    char lit[256]; int ll = 0;
    if (depth > MAX_DEPTH) { P->err = "too deep"; return seq_from(P, &b); }
    for (;;) {
        char c = P->src[P->pos];
        if (P->err) break;
        if (!c) { if (mode) P->err = mode == 1 ? "unclosed [" : "unclosed ("; break; }
        if ((mode == 1 && (c == '|' || c == ']')) || (mode == 2 && c == ')')) break;
        if (c == ']' || c == ')' || c == '>') { P->err = "unbalanced bracket"; break; }
        if (c == '|') { P->err = "| outside [ ]"; break; }
        if (c == '*' || c == '?' || c == '$') {
            lit_flush(P, &b, lit, &ll);
            LNode *n = nb_push(P, &b); if (!n) break;
            n->kind = c == '*' ? LN_STAR : c == '?' ? LN_Q : LN_END;
            if (c == '*') { n->cross = (P->pos == 0 || P->src[P->pos - 1] == ' ' || P->src[P->pos - 1] == '(' || P->src[P->pos - 1] == '[' || P->src[P->pos - 1] == '|'); (*nstars)++; }
            P->pos++; continue;
        }
        if (c == '[') {
            lit_flush(P, &b, lit, &ll);
            P->pos++;
            LSeq alts[24]; int na = 0;
            for (;;) {
                if (na >= 24) { P->err = "too many alternatives"; break; }
                alts[na++] = parse_seq(P, 1, depth + 1, nstars);
                if (P->err) break;
                if (P->src[P->pos] == '|') { P->pos++; continue; }
                if (P->src[P->pos] == ']') { P->pos++; break; }
                P->err = "unclosed ["; break;
            }
            if (P->err) break;
            int any = 0; for (int i = 0; i < na; i++) if (alts[i].cnt) any = 1;
            if (!any) { P->err = "empty [ ]"; break; }
            LSeq *arr = ar_alloc((size_t)na * sizeof *arr);
            LNode *n = nb_push(P, &b);
            if (!arr || !n) { if (!P->err) P->err = "out of memory"; break; }
            memcpy(arr, alts, (size_t)na * sizeof *arr);
            n->kind = LN_ALT; n->alts = arr; n->nalts = na;
            continue;
        }
        if (c == '(') {
            int gstart = P->pos;
            P->pos++;
            LSeq in = parse_seq(P, 2, depth + 1, nstars);
            if (P->err) break;
            if (P->src[P->pos] != ')') { P->err = "unclosed ("; break; }
            P->pos++;
            if (!in.cnt) { P->err = "empty ( )"; break; }
            int tail_space = 0, lead_space = 0;
            /* the group swallows one neighbouring space: the one after it when the group starts a word ("a (b) c", "(a) b"), else the one before it ("a (b)").
             * a group glued to the word before it ("thank(s) you") swallows nothing: the space after it is the separator */
            char before = gstart > 0 ? P->src[gstart - 1] : ' ';
            if (P->src[P->pos] == ' ' && (before == ' ' || before == '[' || before == '|' || before == '(')) { tail_space = 1; P->pos++; }
            else if (ll > 0 && lit[ll - 1] == ' ' && P->src[P->pos] != ' ') { ll--; lead_space = 1; }
            lit_flush(P, &b, lit, &ll);
            if (tail_space || lead_space) {
                int cnt = in.cnt + (tail_space ? 1 : 0) + (lead_space ? 1 : 0);
                LNode *d = ar_alloc((size_t)cnt * sizeof *d); if (!d) { P->err = "out of memory"; break; }
                int o = 0;
                if (lead_space) { memset(&d[o], 0, sizeof d[o]); d[o].kind = LN_LIT; d[o].len = 1; d[o].s = " "; o++; }
                memcpy(d + o, in.n, (size_t)in.cnt * sizeof *d); o += in.cnt;
                if (tail_space) { memset(&d[o], 0, sizeof d[o]); d[o].kind = LN_LIT; d[o].len = 1; d[o].s = " "; o++; }
                in.n = d; in.cnt = o;
            }
            LSeq *arr = ar_alloc(sizeof *arr); LNode *n = nb_push(P, &b);
            if (!arr || !n) { if (!P->err) P->err = "out of memory"; break; }
            arr[0] = in; n->kind = LN_OPT; n->alts = arr; n->nalts = 1;
            continue;
        }
        if (c == '<') {
            lit_flush(P, &b, lit, &ll);
            int j = P->pos + 1; while (P->src[j] && P->src[j] != '>' && j - P->pos < 26) j++;
            if (P->src[j] != '>') { P->err = "unclosed <"; break; }
            int n = j - P->pos - 1;
            const char *name = P->src + P->pos + 1;
            P->pos = j + 1;
            LNode *nd = nb_push(P, &b); if (!nd) break;
            if (n == 1 && name[0] == 'x') { nd->kind = LN_ANY; nd->min = 1; nd->max = 3; nd->name = "x"; continue; }
            int mi = macro_find(name, n);
            char nm[24]; int fl = 0;
            if (n > 0 && n < 24) { memcpy(nm, name, (size_t)n); nm[n] = 0; fl = macro_flags(nm); }
            if (mi < 0 && !fl) { P->err = "unknown macro"; break; }
            nd->kind = LN_ALT; nd->flags = (uint8_t)fl;
            if (mi >= 0) { nd->alts = macros[mi].alts; nd->nalts = macros[mi].nalts; nd->name = macros[mi].name; }
            else nd->name = ar_str(name, n);
            continue;
        }
        if (ll >= (int)sizeof lit - 1) { P->err = "literal too long"; break; }
        lit[ll++] = c; P->pos++;
    }
    lit_flush(P, &b, lit, &ll);
    return seq_from(P, &b);
}

LPat *lpat_compile(const char *src, const char **why) {
    static const char *none = "";
    if (why) *why = none;
    Parse P = { src, 0, 0, NULL };
    int stars = 0;
    LSeq top = parse_seq(&P, 0, 0, &stars);
    if (!P.err && P.src[P.pos]) P.err = "unbalanced bracket";
    if (!P.err && !top.cnt) P.err = "empty pattern";
    if (P.err) { if (why) *why = P.err; return NULL; }
    LPat *lp = ar_alloc(sizeof *lp);
    if (!lp) { if (why) *why = "out of memory"; return NULL; }
    lp->top = top; lp->nstars = stars;
    return lp;
}
int lpat_stars(const LPat *p) { return p->nstars; }

/* ---------- the matcher ---------- */
typedef struct Cont { const LSeq *seq; int i; const struct Cont *next; } Cont;
typedef struct { const char *base; const char **best; const char *start; const char *star; const char *bstar; long steps; } Ctx;

static void run(Ctx *c, const LSeq *q, int i, const char *s, const Cont *k);

static void accept(Ctx *c, const char *s, int end_mark) {
    if (s > *c->best && (end_mark || at_boundary(c->base, s))) { *c->best = s; c->bstar = c->star ? c->star : s; }
}
/* how far a digit run reaches: ends of "12" and of "3.5" */
static int digit_ends(const char *s, const char **e) {
    if (!isdigit((unsigned char)*s)) return 0;
    const char *p = s; while (isdigit((unsigned char)*p)) p++;
    int n = 0; e[n++] = p;
    if (*p == '.' && isdigit((unsigned char)p[1])) { const char *r = p + 1; while (isdigit((unsigned char)*r)) r++; e[n++] = r; }
    return n;
}
/* 3  3pm  3 pm  10:30  10:30 pm  3 oclock */
static int time_ends(const char *s, const char **e) {
    if (!isdigit((unsigned char)*s)) return 0;
    const char *p = s; int d = 0; while (isdigit((unsigned char)*p) && d < 2) { p++; d++; }
    if (isdigit((unsigned char)*p)) return 0;
    const char *bases[2]; int nb = 0, n = 0;
    bases[nb++] = p;
    if (*p == ':' && isdigit((unsigned char)p[1]) && isdigit((unsigned char)p[2])) bases[nb++] = p + 3;
    for (int b = 0; b < nb; b++) {
        e[n++] = bases[b];
        static const char *const suf[] = { "am", "pm", "oclock", "o'clock" };
        for (int sp = 0; sp < 2; sp++) {
            const char *r = bases[b] + sp;
            if (sp && *bases[b] != ' ') break;
            for (int u = 0; u < 4; u++) { size_t l = strlen(suf[u]); if (!strncmp(r, suf[u], l) && !is_word((unsigned char)r[l]) && n < 12) e[n++] = r + l; }
        }
    }
    return n;
}

static void run(Ctx *c, const LSeq *q, int i, const char *s, const Cont *k) {
    for (;;) {
        if (++c->steps > MAX_STEPS) return;
        if (i >= q->cnt) {
            if (!k) { accept(c, s, 0); return; }
            q = k->seq; i = k->i; k = k->next; continue;
        }
        const LNode *n = &q->n[i];
        switch (n->kind) {
        case LN_LIT:
            if (strncmp(s, n->s, (size_t)n->len)) return;
            s += n->len; i++; continue;
        case LN_Q:
            if (is_stop((unsigned char)*s)) return;
            s++; i++; continue;
        case LN_END:
            if (is_stop((unsigned char)*s)) { if (s > *c->best) { *c->best = s; c->bstar = c->star ? c->star : s; } }
            return;
        case LN_STAR: {
            const char *sv = c->star; if (!c->star) c->star = s;
            for (const char *p = s;; p++) {
                run(c, q, i + 1, p, k);
                if (c->steps > MAX_STEPS || is_stop((unsigned char)*p) || (!n->cross && !is_word((unsigned char)*p))) break;
            }
            c->star = sv; return;
        }
        case LN_ALT: {
            Cont nk = { q, i + 1, k };
            for (int a = 0; a < n->nalts; a++) run(c, &n->alts[a], 0, s, &nk);
            const char *e[12]; int ne = 0;
            if (n->flags & LNF_DIGITS) ne = digit_ends(s, e);
            else if (n->flags & LNF_TIME) ne = time_ends(s, e);
            for (int u = 0; u < ne; u++) run(c, q, i + 1, e[u], k);
            if ((n->flags & LNF_NPC) && npc_name[0]) {
                size_t l = strlen(npc_name);
                if (!strncmp(s, npc_name, l) && !is_word((unsigned char)s[l])) run(c, q, i + 1, s + l, k);
            }
            return;
        }
        case LN_OPT: {
            run(c, q, i + 1, s, k);
            Cont nk = { q, i + 1, k };
            run(c, &n->alts[0], 0, s, &nk);
            return;
        }
        case LN_ANY: {
            const char *p = s;
            for (int w = 1; w <= n->max; w++) {
                if (!is_word((unsigned char)*p)) break;
                while (is_word((unsigned char)*p)) p++;
                if (w >= n->min) run(c, q, i + 1, p, k);
                if (*p == ' ' && is_word((unsigned char)p[1])) p++; else break;
            }
            return;
        }
        default: return;
        }
    }
}

int lpat_match(const LPat *p, const char *s, const char *base, const char **best, int *pref) {
    Ctx c = { base, best, s, NULL, NULL, 0 };
    const char *before = *best;
    run(&c, &p->top, 0, s, NULL);
    if (pref && *best > before) *pref = (int)(c.bstar - s);
    return c.steps <= MAX_STEPS;
}

/* ---------- the first letters (the index) ---------- */
typedef struct { char (*out)[4]; int n, max, wild, huge; } Hd;
static void hd_emit(Hd *h, const char *pre, int plen) {
    if (h->wild) return;
    char t[4] = { 0 }; memcpy(t, pre, (size_t)plen);
    for (int i = 0; i < h->n; i++) if (!strcmp(h->out[i], t)) return;
    if (h->n >= h->max) { h->wild = 1; h->huge = 1; return; }
    strcpy(h->out[h->n++], t);
}
static void hd_run(Hd *h, const LSeq *q, int i, char *pre, int plen, const Cont *k, int *budget) {   /* pre is the caller's private copy */
    for (;;) {
        if (h->wild || --*budget < 0) { if (*budget < 0) { h->wild = 1; h->huge = 1; } return; }
        if (i >= q->cnt) {
            if (!k) { if (plen) hd_emit(h, pre, plen); return; }
            q = k->seq; i = k->i; k = k->next; continue;
        }
        const LNode *n = &q->n[i];
        switch (n->kind) {
        case LN_LIT: {
            char p2[4]; memcpy(p2, pre, 4); int pl = plen;
            for (int j = 0; j < n->len; j++) {
                char ch = n->s[j];
                if (ch == ' ') { if (pl) hd_emit(h, p2, pl); return; }       /* a leading space can never match at a word start */
                p2[pl++] = ch;
                if (pl == 3) { hd_emit(h, p2, pl); return; }
            }
            memcpy(pre, p2, 4); plen = pl; i++; continue;
        }
        case LN_STAR: case LN_Q: case LN_ANY:
            if (plen) hd_emit(h, pre, plen); else h->wild = 1;
            return;
        case LN_END:
            if (plen) hd_emit(h, pre, plen);
            return;
        case LN_ALT: {
            Cont nk = { q, i + 1, k };
            for (int a = 0; a < n->nalts; a++) { char p2[4]; memcpy(p2, pre, 4); hd_run(h, &n->alts[a], 0, p2, plen, &nk, budget); }
            if (n->flags & (LNF_DIGITS | LNF_TIME)) {
                if (plen) hd_emit(h, pre, plen);
                else for (char d = '0'; d <= '9'; d++) { char dd = d; hd_emit(h, &dd, 1); }
            }
            if (n->flags & LNF_NPC) { if (plen) hd_emit(h, pre, plen); else h->wild = 1; }
            if (!n->nalts && !n->flags) { /* an empty macro matches nothing */ }
            return;
        }
        case LN_OPT: {
            char p2[4]; memcpy(p2, pre, 4);
            hd_run(h, q, i + 1, p2, plen, k, budget);
            Cont nk = { q, i + 1, k }; memcpy(p2, pre, 4);
            hd_run(h, &n->alts[0], 0, p2, plen, &nk, budget);
            return;
        }
        default: return;
        }
    }
}
int lpat_heads(const LPat *p, char (*out)[4], int max, int *wild, int *huge) {
    Hd h = { out, 0, max, 0, 0 }; char pre[4] = { 0 }; int budget = 4000;
    hd_run(&h, &p->top, 0, pre, 0, NULL, &budget);
    if (wild) *wild = h.wild;
    if (huge) *huge = h.huge;
    return h.wild ? 0 : h.n;
}

/* ---------- surface forms (tools) ---------- */
typedef struct { char buf[400]; int max, count; void (*fn)(const char *, void *); void *u; } Gen;
static void gen_run(Gen *g, const LSeq *q, int i, int len, const Cont *k) {
    for (;;) {
        if (g->count >= g->max) return;
        if (i >= q->cnt) {
            if (!k) { g->buf[len] = 0; g->fn(g->buf, g->u); g->count++; return; }
            q = k->seq; i = k->i; k = k->next; continue;
        }
        const LNode *n = &q->n[i];
        switch (n->kind) {
        case LN_LIT: if (len + n->len >= (int)sizeof g->buf - 1) return; memcpy(g->buf + len, n->s, (size_t)n->len); len += n->len; i++; continue;
        case LN_STAR: case LN_Q: case LN_END: {
            if (len + 1 >= (int)sizeof g->buf - 1) return;
            g->buf[len++] = n->kind == LN_STAR ? '*' : n->kind == LN_Q ? '?' : '$';
            if (n->kind == LN_END) { g->buf[len] = 0; g->fn(g->buf, g->u); g->count++; return; }
            i++; continue;
        }
        case LN_ANY: if (len + 3 >= (int)sizeof g->buf - 1) return; memcpy(g->buf + len, "<x>", 3); len += 3; i++; continue;
        case LN_ALT: {
            Cont nk = { q, i + 1, k };
            for (int a = 0; a < n->nalts; a++) gen_run(g, &n->alts[a], 0, len, &nk);
            const char *ex[3]; int ne = 0;
            if (n->flags & LNF_DIGITS) ex[ne++] = "7";
            if (n->flags & LNF_TIME) ex[ne++] = "3pm";
            if ((n->flags & LNF_NPC) && npc_name[0]) ex[ne++] = npc_name;
            for (int u = 0; u < ne; u++) {
                int l = (int)strlen(ex[u]); if (len + l >= (int)sizeof g->buf - 1) continue;
                memcpy(g->buf + len, ex[u], (size_t)l);
                gen_run(g, q, i + 1, len + l, k);
            }
            return;
        }
        case LN_OPT: {
            gen_run(g, q, i + 1, len, k);
            Cont nk = { q, i + 1, k };
            gen_run(g, &n->alts[0], 0, len, &nk);
            return;
        }
        default: return;
        }
    }
}
int lpat_forms(const LPat *p, int maxforms, void (*fn)(const char *form, void *u), void *u) {
    Gen *g = malloc(sizeof *g); if (!g) return 0;
    g->max = maxforms; g->count = 0; g->fn = fn; g->u = u;
    gen_run(g, &p->top, 0, 0, NULL);
    int n = g->count; free(g);
    return n;
}
