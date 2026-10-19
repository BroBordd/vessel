/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * DIALOGUE MEMORY (docs/convo_plan.md, chunk 35): see lang_ctx.h. */
#include "lang_ctx.h"
#include "lang_clean.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static LangCtx C;
static CtxFacts F;      /* survives lang_ctx_reset */
static int I_LIED = -2;

static void sat(uint8_t *v) { if (*v < 255) (*v)++; }
static void put_unique(char (*list)[24], int *n, int cap, const char *s) {
    for (int i = 0; i < *n; i++) if (!strcmp(list[i], s)) return;
    if (*n == cap) { memmove(list[0], list[1], (size_t)(cap - 1) * 24); (*n)--; }       /* the oldest goes */
    snprintf(list[(*n)++], 24, "%s", s);
}

void lang_ctx_reset(void) { memset(&C, 0, sizeof C); }
void lang_ctx_set_ask(const char *tag, const char *text) {
    C.ask[0] = 0; C.ask_text[0] = 0;
    if (!tag || !lang_ask_tag_valid(tag)) return;
    snprintf(C.ask, sizeof C.ask, "%s", tag);
    if (text) snprintf(C.ask_text, sizeof C.ask_text, "%.95s", text);
}
void lang_ctx_set_last(const char *key, const char *text) {
    CtxReply *r = &C.rep[C.reps % CTX_TURNS];
    snprintf(r->key, sizeof r->key, "%s", key ? key : ""); r->mood = 0;
    C.reps++; if (C.nrep < CTX_TURNS) C.nrep++;
    snprintf(C.last_text, sizeof C.last_text, "%.159s", text ? text : "");
}
void lang_ctx_set_ask_line(const char *text) {
    char q[96]; lang_ask_sentence(text, q, (int)sizeof q);
    lang_ctx_set_ask(lang_ask_tag_text(text), q);
}
const LangCtx *lang_ctx(void) { return &C; }
const CtxTurn *lang_ctx_turn(int back) { return back >= 0 && back < C.nturn ? &C.turn[(C.turns - 1 - back) % CTX_TURNS] : NULL; }
const CtxReply *lang_ctx_reply(int back) { return back >= 0 && back < C.nrep ? &C.rep[(C.reps - 1 - back) % CTX_TURNS] : NULL; }
int lang_ctx_topic_count(int topic) { for (int i = 0; i < C.nasked; i++) if (C.asked_about[i] == topic) return C.asked_n[i]; return 0; }
const CtxFacts *lang_ctx_facts(void) { return &F; }
void lang_ctx_facts_save(CtxFacts *out) { if (out) *out = F; }
void lang_ctx_facts_load(const CtxFacts *in) {
    if (!in) { memset(&F, 0, sizeof F); return; }
    F = *in; F.name[sizeof F.name - 1] = 0;
}

/* ---------- chunk 38: repetition, spam, copying ---------- */
/* lower case letters and digits, one space between words, apostrophes dropped (don't = dont) */
static void squash(const char *in, char *out, int cap) {
    int n = 0, sp = 0;
    for (; *in; in++) {
        unsigned char c = (unsigned char)*in;
        if (c == '\'') continue;
        if (isalnum(c)) { if (sp && n && n < cap - 2) out[n++] = ' '; sp = 0; if (n < cap - 1) out[n++] = (char)tolower(c); }
        else sp = 1;
    }
    out[n] = 0;
}
static int toks(char *s, char **w, int max) {      /* splits in place, returns the word count */
    int n = 0;
    for (char *p = s; *p && n < max; ) { while (*p == ' ') *p++ = 0; if (!*p) break; w[n++] = p; while (*p && *p != ' ') p++; }
    return n;
}
static int jaccard(const char *a, const char *b, int *na_out) {      /* word set overlap 0-100 */
    char x[240], y[240]; char *wa[24], *wb[24];
    snprintf(x, sizeof x, "%s", a); snprintf(y, sizeof y, "%s", b);
    int na = toks(x, wa, 24), nb = toks(y, wb, 24), ua = 0, ub = 0, common = 0;
    char *A[24], *B[24];
    for (int i = 0; i < na; i++) { int d = 0; for (int k = 0; k < ua; k++) if (!strcmp(A[k], wa[i])) d = 1; if (!d) A[ua++] = wa[i]; }
    for (int i = 0; i < nb; i++) { int d = 0; for (int k = 0; k < ub; k++) if (!strcmp(B[k], wb[i])) d = 1; if (!d) B[ub++] = wb[i]; }
    for (int i = 0; i < ua; i++) for (int k = 0; k < ub; k++) if (!strcmp(A[i], B[k])) { common++; break; }
    if (na_out) *na_out = na;
    return ua + ub - common > 0 ? common * 100 / (ua + ub - common) : 0;
}
static int nwords(const char *s) { int n = 0, in = 0; for (; *s; s++) { if (*s == ' ') in = 0; else if (!in) { in = 1; n++; } } return n; }
static int same_set(const Heard *h, const CtxTurn *t) {      /* the same non-empty set of intents (the weak feeling intents pos / neg do not count: one extra nice word is no new topic) */
    int a[8], na = 0, b[4], nb = 0;
    for (int i = 0; i < h->n && i < 4; i++) if (h->order[i] != I_POS && h->order[i] != I_NEG) a[na++] = h->order[i];
    for (int i = 0; i < t->n; i++) if (t->intent[i] != I_POS && t->intent[i] != I_NEG) b[nb++] = t->intent[i];
    if (!na || na != nb) return 0;
    for (int i = 0; i < na; i++) { int f = 0; for (int k = 0; k < nb; k++) if (b[k] == a[i]) f = 1; if (!f) return 0; }
    return 1;
}

int lang_ctx_repeat_kind(const Heard *h, const char *raw, const char *lc, int *level) {
    *level = 0;
    if (!h || !lc) return REP_NONE;
    if (heard_has(h, I_SELFHARM) || heard_has(h, I_SORRY) || heard_has(h, I_SAD) || heard_has(h, I_SCARED) || heard_has(h, I_HELP) || heard_has(h, I_CONFUSED)) return REP_NONE;
    if (heard_has(h, I_AGREE_TO_ASK) || heard_has(h, I_DECLINE_TO_ASK) || heard_has(h, I_BECAUSE) || heard_has(h, I_ANSWER_CHOICE)) return REP_NONE;
    for (int i = I_FOLLOW_WHY; i <= I_FOLLOW_AGAIN; i++) if (heard_has(h, i)) return REP_NONE;
    /* spam: a character over and over (the raw text: cleaning folds the runs) */
    if (raw) { int run = 1; for (const char *p = raw + 1; *raw && *p; p++) { run = *p == p[-1] ? run + 1 : 1; if (run >= 12 && !isspace((unsigned char)*p)) { *level = C.rep_n + 1 < 2 ? 2 : C.rep_n + 1 > 5 ? 5 : C.rep_n + 1; return REP_SPAM; } } }
    char cur[400]; squash(lc, cur, sizeof cur);
    if (!cur[0]) return REP_NONE;
    int L = (int)strlen(cur), nw = nwords(cur);
    int spam = 0;
    { char t[400]; snprintf(t, sizeof t, "%s", cur); char *w[64]; int n = toks(t, w, 64), best = 0;     /* one word 4+ times, most of the words */
      for (int i = 0; i < n; i++) { int c = 0; for (int k = 0; k < n; k++) if (!strcmp(w[i], w[k])) c++; if (c > best) best = c; }
      if (best >= 4 && best * 100 >= n * 60) spam = 1; }
    if (!spam && L >= 24) {                                           /* the same text pasted twice */
        if (L % 2 == 1 && cur[L / 2] == ' ' && !memcmp(cur, cur + L / 2 + 1, (size_t)(L / 2))) spam = 1;
        else if (L % 2 == 0 && !memcmp(cur, cur + L / 2, (size_t)(L / 2))) spam = 1;
    }
    if (spam) { *level = C.rep_n + 1 < 2 ? 2 : C.rep_n + 1 > 5 ? 5 : C.rep_n + 1; return REP_SPAM; }
    /* the npc's own words typed back */
    if (C.last_text[0] && C.nrep && nw >= 2) {
        char last[400]; squash(C.last_text, last, sizeof last);
        int copy = !strcmp(cur, last);
        if (!copy && nw >= 4) { const char *f = strstr(last, cur); if (f && (f == last || f[-1] == ' ') && (f[L] == 0 || f[L] == ' ')) copy = 1; }
        if (!copy && nw >= 4 && nwords(last) >= 4 && jaccard(cur, last, NULL) >= 80) copy = 1;
        if (copy) { *level = C.copy_n + 1 > 3 ? 3 : C.copy_n + 1; return REP_COPYCAT; }
    }
    /* the player says it again */
    const CtxTurn *p = lang_ctx_turn(0);
    if (p) {
        char c95[96], prv[200], pc[96]; snprintf(c95, sizeof c95, "%.95s", lc); squash(c95, pc, sizeof pc); squash(p->text, prv, sizeof prv);
        int asked = C.ask[0] || (C.last_text[0] && C.last_text[strlen(C.last_text) - 1] == '?');
        int answer_again = nw <= 3 && asked && (heard_has(h, I_YES) || heard_has(h, I_NO) || heard_has(h, I_OK) || heard_has(h, I_AGREE) || heard_has(h, I_DISAGREE));       /* answering the same question again (yes, no, ok) is not a repeat */
        int rep = 0, n1;
        if (pc[0] && !strcmp(pc, prv)) rep = 1;
        else if (nw >= 3 && nwords(prv) >= 3 && jaccard(pc, prv, &n1) >= 80) rep = 1;
        else { const CtxTurn *p2 = lang_ctx_turn(1); if (p2 && same_set(h, p) && same_set(h, p2) && jaccard(pc, prv, &n1) >= 50) rep = 1; }
        if (rep && !answer_again) { *level = C.rep_n + 1 > 5 ? 5 : C.rep_n + 1; return REP_REPEAT; }
    }
    return REP_NONE;
}

void lang_ctx_note(const Heard *h, const char *text, const char *reply_key, int mood) {
    if (I_LIED == -2) I_LIED = lang_intent_by_name("lie");
    /* the player's turn */
    CtxTurn *t = &C.turn[C.turns % CTX_TURNS];
    memset(t, 0, sizeof *t);
    if (text) { char cl[400]; lang_clean(text, cl, (int)sizeof cl, NULL); snprintf(t->text, sizeof t->text, "%.95s", cl); }
    if (heard_has(h, I_SELFHARM)) t->intent[t->n++] = I_SELFHARM;
    for (int i = 0; i < h->n && t->n < 4; i++) if (h->order[i] != I_SELFHARM) t->intent[t->n++] = (int16_t)h->order[i];
    t->valence = h->valence;
    C.turns++; if (C.nturn < CTX_TURNS) C.nturn++;
    /* the npc's reply */
    CtxReply *r = &C.rep[C.reps % CTX_TURNS];
    snprintf(r->key, sizeof r->key, "%s", reply_key ? reply_key : "");
    r->mood = (int8_t)(mood < -100 ? -100 : mood > 100 ? 100 : mood);
    C.reps++; if (C.nrep < CTX_TURNS) C.nrep++;
    if (heard_has(h, I_SELFHARM)) C.rep_n = C.copy_n = 0;      /* chunk 38 */
    else { C.rep_n = (heard_has(h, I_REPEAT_PLAYER) || heard_has(h, I_SPAM)) ? h->rep_level : 0; C.copy_n = heard_has(h, I_COPYCAT) ? h->rep_level : 0; }
    if (heard_has(h, I_SELFHARM)) C.last_text[0] = 0; else snprintf(C.last_text, sizeof C.last_text, "%.159s", lang_reply_text());
    if (heard_has(h, I_SELFHARM)) lang_ctx_set_ask("", NULL); else lang_ctx_set_ask(lang_reply_ask(), lang_reply_ask_text());      /* chunk 36: what the npc just asked */
    /* facts. nothing is kept after self-harm: that turn is a person, not a data point */
    if (heard_has(h, I_SELFHARM)) return;
    const Capture *c;
    if ((c = lang_heard_capture(h, CAP_NAME, 0)) && c->text[0]) { snprintf(C.name, sizeof C.name, "%s", c->text); snprintf(F.name, sizeof F.name, "%s", c->text); }
    if ((c = lang_heard_capture(h, CAP_AGE, 0)) && c->value > 0) C.age = c->value;
    for (int k = 0; (c = lang_heard_capture(h, CAP_LIKE, k)); k++) put_unique(C.likes, &C.nlikes, CTX_LIKES, c->text);
    for (int k = 0; (c = lang_heard_capture(h, CAP_DISLIKE, k)); k++) put_unique(C.dislikes, &C.ndislikes, CTX_LIKES, c->text);
    if (heard_has(h, I_SCARED)) C.said_scared = 1;
    if (heard_has(h, I_INSULT)) { sat(&C.insulted_count); sat(&F.insulted); }
    if (heard_has(h, I_THANKS)) { sat(&C.thanked_count); sat(&F.thanked); }
    if (heard_has(h, I_PRAISE)) sat(&C.complimented_count);
    if (I_LIED >= 0 && heard_has(h, I_LIED)) sat(&C.lied_count);
    for (int i = 0; i < h->ntopic && i < 1; i++) {      /* the strongest topic of a question */
        if (!h->is_question) break;
        int k; for (k = 0; k < C.nasked; k++) if (C.asked_about[k] == h->topic[i]) break;
        if (k == C.nasked) { if (C.nasked == CTX_TOPICS) break; C.asked_about[C.nasked++] = h->topic[i]; C.asked_n[k] = 0; }
        sat(&C.asked_n[k]);
    }
}
