/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * DIALOGUE MEMORY (docs/convo_plan.md, chunk 35): see lang_ctx.h. */
#include "lang_ctx.h"
#include "lang_clean.h"
#include "lang_lex.h"
#include "lang_frame.h"
#include "lang_persona.h"      /* Q_ASKS_BACK (chunk 41) */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

static LangCtx C;
static CtxFacts F;      /* survives lang_ctx_reset */
static int I_LIED = -2;

static void sat(uint8_t *v) { if (*v < 255) (*v)++; }
static void put_unique(char (*list)[24], int *n, int cap, const char *s) {
    for (int i = 0; i < *n; i++) if (!strcmp(list[i], s)) return;
    if (*n == cap) { memmove(list[0], list[1], (size_t)(cap - 1) * 24); (*n)--; }       /* the oldest goes */
    snprintf(list[(*n)++], 24, "%s", s);
}

void lang_ctx_reset(void) { memset(&C, 0, sizeof C); C.prior = F; }      /* chunk 39: what earlier chats left stays readable as `prior` */
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

/* ---------- chunk 39: callbacks (the npc remembers) ---------- */
static const char *const CB_KEY[CB_COUNT] = { "cb.insulted", "cb.thanked", "cb.complimented", "cb.name", "cb.scared", "cb.lied", "cb.asked_before",
    "cb.apology_taken", "cb.same_topic", "cb.insulted_past", "cb.name_past" };
const char *lang_cb_key(int kind) { return kind >= 0 && kind < CB_COUNT ? CB_KEY[kind] : ""; }
int lang_cb_by_key(const char *key) { for (int i = 0; i < CB_COUNT; i++) if (key && !strcmp(key, CB_KEY[i])) return i; return -1; }
void lang_ctx_set_counts(int insulted, int thanked, int complimented, int lied, int scared) {
    C.insulted_count = (uint8_t)insulted; C.thanked_count = (uint8_t)thanked; C.complimented_count = (uint8_t)complimented; C.lied_count = (uint8_t)lied; C.said_scared = (uint8_t)(scared != 0);
}
static int prev_has(int in) { const CtxTurn *t = lang_ctx_turn(0); if (!t) return 0; for (int i = 0; i < t->n; i++) if (t->intent[i] == in) return 1; return 0; }
static int cur_has_name(const Heard *h) { return heard_has(h, I_INTRO) || heard_has(h, I_NAME_SWEAR_NAME) || lang_heard_capture(h, CAP_NAME, 0) != NULL; }
/* the word of the topic just asked about: what the player typed when it is safe to echo, else the topic's own name (a curated one) */
static void topic_fact(const Heard *h, char *out, int cap) {
    const char *w = h->thing[0] && lang_safe_echo(h->thing) ? h->thing : lang_topic_name(h->topic[0]);
    snprintf(out, (size_t)cap, "%s", w ? w : "that");
    for (char *p = out; *p; p++) if (*p == '_' || *p == '.') *p = ' ';
}
int lang_ctx_callbacks(const Heard *h, int forgetful, CbCand *out) {
    if (!h || C.turns < 2) return 0;                                   /* nothing to remember yet */
    if (heard_has(h, I_SELFHARM) || heard_has(h, I_SAD) || heard_has(h, I_SCARED) || heard_has(h, I_HELP) || heard_has(h, I_CONFUSED)) return 0;
    if (C.cb_n && C.reps - C.cb_at < CB_GAP - 1) return 0;             /* one in 4 replies at most */
    int ok[CB_COUNT] = { 0 }; char fact[CB_COUNT][24]; memset(fact, 0, sizeof fact);
    int cur_ins = heard_has(h, I_INSULT), cur_sorry = heard_has(h, I_SORRY);
    int cur_thx = heard_has(h, I_THANKS) || heard_has(h, I_THANKS_SARCASTIC), cur_praise = heard_has(h, I_PRAISE) || heard_has(h, I_LOVE);
    /* the topic just asked about, and how often before */
    if (h->is_question && h->ntopic > 0) {
        int n = lang_ctx_topic_count(h->topic[0]);
        if (n == 1) { ok[CB_ASKED_BEFORE] = 1; topic_fact(h, fact[CB_ASKED_BEFORE], 24); }
        else if (n >= 2) { ok[CB_SAME_TOPIC] = 1; topic_fact(h, fact[CB_SAME_TOPIC], 24); }
    }
    ok[CB_APOLOGY_TAKEN] = C.apologized && !cur_sorry && !cur_ins && !prev_has(I_SORRY);
    ok[CB_INSULTED] = C.insulted_count >= 1 && !cur_ins && !cur_sorry && !prev_has(I_INSULT);
    ok[CB_SCARED] = C.said_scared && !prev_has(I_SCARED);
    ok[CB_THANKED] = C.thanked_count >= 1 && !cur_thx && !prev_has(I_THANKS);
    ok[CB_COMPLIMENTED] = C.complimented_count >= 1 && !cur_praise && !prev_has(I_PRAISE);
    ok[CB_LIED] = C.lied_count >= 1;
    if (!forgetful && C.name[0] && C.turns >= 3 && !cur_has_name(h)) { ok[CB_NAME] = 1; snprintf(fact[CB_NAME], 24, "%s", C.name); }
    if (!forgetful && !C.name[0] && C.prior.name[0] && !cur_has_name(h)) { ok[CB_NAME_PAST] = 1; snprintf(fact[CB_NAME_PAST], 24, "%s", C.prior.name); }
    ok[CB_INSULTED_PAST] = C.prior.insulted > 3 && C.insulted_count == 0 && !cur_ins;
    /* order: the priority above, a kind used less often first, a spent kind never */
    static const int PRIO[CB_COUNT] = { CB_ASKED_BEFORE, CB_SAME_TOPIC, CB_APOLOGY_TAKEN, CB_INSULTED, CB_SCARED, CB_THANKED, CB_COMPLIMENTED, CB_NAME, CB_LIED, CB_INSULTED_PAST, CB_NAME_PAST };
    int n = 0;
    for (int used = 0; used < CB_MAX_PER_KIND; used++)
        for (int i = 0; i < CB_COUNT; i++) {
            int k = PRIO[i], max = (k == CB_INSULTED_PAST || k == CB_NAME_PAST) ? 1 : CB_MAX_PER_KIND;
            if (ok[k] && C.cb_cnt[k] == used && used < max) { out[n].kind = k; snprintf(out[n].fact, sizeof out[n].fact, "%s", fact[k]); n++; }
        }
    return n;
}

/* ---------- chunk 40: topic threads ---------- */
static int th_spent(int topic) { for (int i = 0; i < C.th_nspent; i++) if (C.th_spent[i] == topic) return 1; return 0; }
static void th_spend(int topic) {
    if (th_spent(topic)) return;
    if (C.th_nspent == 4) { memmove(C.th_spent, C.th_spent + 1, 3 * sizeof C.th_spent[0]); C.th_nspent--; }
    C.th_spent[C.th_nspent++] = (int16_t)topic;
}
int lang_thread_active(void) { return C.th_on; }
/* a short answer to the npc's question (yes, no, ok, because ..., the second one): the thread waits, it does not end */
static int th_answer(const Heard *h) {
    return heard_has(h, I_AGREE_TO_ASK) || heard_has(h, I_DECLINE_TO_ASK) || heard_has(h, I_BECAUSE) || heard_has(h, I_ANSWER_CHOICE)
        || heard_has(h, I_YES) || heard_has(h, I_NO) || heard_has(h, I_OK) || heard_has(h, I_AGREE) || heard_has(h, I_DISAGREE);
}
int lang_thread_ask(const char *lc) {
    static const char *const P[] = { "tell me more about ", "tell me about ", "more about ", "talk about ", "talk more about ", "talk of ", "speak of ", "speak about ",
        "lets talk about ", "let us talk about ", "can we talk about ", "i want to talk about ", "i wanna talk about ", "what about ", "how about ", "what do you think about ", NULL };
    if (!lc) return 0;
    for (int i = 0; P[i]; i++) { const char *f = strstr(lc, P[i]); if (f && (f == lc || f[-1] == ' ')) return 1; }
    return 0;
}
int lang_ctx_thread(const Heard *h, int tone, ThreadPlan *out) {
    out->kind = TH_NONE; out->topic = -1; out->step = 0;
    if (!h) return TH_NONE;
    if (heard_has(h, I_SELFHARM) || heard_has(h, I_SAD) || heard_has(h, I_SCARED) || heard_has(h, I_HELP) || heard_has(h, I_CONFUSED)) return TH_NONE;
    int more = heard_has(h, I_FOLLOW_MORE), top = h->ntopic ? h->topic[0] : -1;
    int dull = tone == TONE_BORED || tone == TONE_COLD;
    if (C.th_on) {
        int same = top == C.th_topic;
        if (!same && !more) return TH_NONE;      /* the player moved on, or only answered the npc's question (the thread waits, see th_commit) */                      /* the player moved on: the thread just ends (commit does it) */
        out->topic = C.th_topic;
        if (C.th_step >= 4 || dull) { out->kind = TH_CHANGE; out->step = 0; return TH_CHANGE; }      /* the goodbye is said, or the npc has no patience left for it */
        out->kind = TH_STEP; out->step = C.th_step + 1; return TH_STEP;
    }
    if (top < 0 || th_spent(top)) return TH_NONE;
    int run = (C.run_topic == top ? C.run_n : 0) + 1;
    if (run >= 3 || h->topic_ask) { out->kind = TH_STEP; out->topic = top; out->step = 1; return TH_STEP; }
    return TH_NONE;
}
/* commit what the reply said: key is the thread key lang_reply spoke (th.<topic>.<n> or ch.generic), "" if none */
static void th_commit(const Heard *h, const char *key) {
    int top = h->ntopic ? h->topic[0] : -1;
    if (!heard_has(h, I_SELFHARM)) { if (top >= 0 && top == C.run_topic) { if (C.run_n < 255) C.run_n++; } else { C.run_topic = (int16_t)top; C.run_n = top >= 0 ? 1 : 0; } }
    else { C.run_topic = -1; C.run_n = 0; }
    if (!C.th_on && key[0] == 0) return;
    if (!strncmp(key, "th.", 3)) { const char *d = strrchr(key, '.'); int n = d ? atoi(d + 1) : 0; if (!C.th_on) { C.th_on = 1; C.th_topic = (int16_t)top; } C.th_step = (uint8_t)n; if (n >= 4) th_spend(C.th_topic); return; }
    if (!strncmp(key, "ch.", 3)) { if (C.th_on) th_spend(C.th_topic); C.th_on = 0; C.th_step = 0; return; }
    if (C.th_on && !(top == C.th_topic || heard_has(h, I_FOLLOW_MORE) || th_answer(h))) { C.th_on = 0; C.th_step = 0; }      /* the player left the topic (an answer to the npc's question does not count as leaving) */
}

/* ---------- chunk 41: npc initiative ---------- */
int lang_ctx_nudge(const Heard *h) { return h->silent ? (C.nudge_n >= 4 ? 5 : C.nudge_n + 1) : 0; }

static int ab_veto_intent(int in) {
    return in == I_SELFHARM || in == I_INSULT || in == I_SORRY || in == I_BYE || in == I_SAD || in == I_SCARED || in == I_CONFUSED || in == I_HELP
        || (in >= I_REPEAT_PLAYER && in <= I_SPAM) || in == I_FOLLOW_WHY || in == I_FOLLOW_MORE || in == I_FOLLOW_REALLY || in == I_FOLLOW_AGAIN
        || in == I_AGREE_TO_ASK || in == I_DECLINE_TO_ASK || in == I_BECAUSE || in == I_ANSWER_CHOICE;
}
int lang_ctx_askback(const Heard *h, int tone, int quirk, AskBackPlan *out) {
    out->n = 0;
    if (h->silent) return 0;
    if (!((quirk & Q_ASKS_BACK) || tone == TONE_CURIOUS || tone == TONE_SUSPICIOUS || tone == TONE_PLAYFUL)) return 0;
    if (tone == TONE_HOSTILE) return 0;
    const CtxReply *lr = lang_ctx_reply(0);
    if (lr && lr->mood <= -55) return 0;                         /* right after a hostile stretch */
    if (C.turns < (C.ab_n ? C.ab_next : 2)) return 0;           /* the third player turn at the earliest, then every 3 to 5 */
    for (int i = 0; i < h->n; i++) if (ab_veto_intent(h->order[i])) return 0;
    int known = C.name[0] || F.name[0] || C.prior.name[0] || lang_heard_capture(h, CAP_NAME, 0) != NULL;
    int feeling = h->valence != 0 || lang_heard_capture(h, CAP_LIKE, 0) || lang_heard_capture(h, CAP_DISLIKE, 0) || lang_heard_capture(h, CAP_FEELING, 0);
    static const char *const SUSP[] = { "why", "yn", "more", "choice", "name" }, *const PLAY[] = { "choice", "yn", "more", "name", "why" },
                             *const CURI[] = { "name", "more", "why", "yn", "choice" }, *const ANY[] = { "name", "more", "yn", "choice", "why" };
    const char *const *ord = tone == TONE_SUSPICIOUS ? SUSP : tone == TONE_PLAYFUL ? PLAY : tone == TONE_CURIOUS ? CURI : ANY;
    int rot = ord == ANY ? (C.turns + C.ab_n) % 5 : 0;           /* a quirk with no mood to lead takes the tags in turn */
    for (int i = 0; i < 5; i++) {
        const char *t = ord[(i + rot) % 5];
        if (!strcmp(t, "name") && known) continue;
        if (!strcmp(t, "why") && !feeling) continue;
        if (!strcmp(t, C.ab_tag)) continue;                      /* never the same kind twice in a row */
        snprintf(out->tag[out->n++], sizeof out->tag[0], "%s", t);
    }
    return out->n > 0;
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
    { int k = lang_cb_by_key(lang_reply_callback());      /* chunk 39: the reply carried a callback */
      if (k >= 0) { if (C.cb_cnt[k] < 255) C.cb_cnt[k]++; C.cb_n++; C.cb_at = C.reps; } }
    th_commit(h, lang_reply_thread());      /* chunk 40 */
    C.nudge_n = h->silent ? (uint8_t)(C.nudge_n < 255 ? C.nudge_n + 1 : 255) : 0;      /* chunk 41: dots in a row; anything else breaks the run */
    if (lang_reply_askback()[0]) {      /* chunk 41: the reply asked something back: the next one is 3 to 5 turns away, and not of the same kind */
        const char *k = lang_reply_askback();
        C.ab_n++; C.ab_next = C.turns + 2 + (int)(((unsigned)C.turns * 5u + (unsigned)C.ab_n * 3u) % 3u);
        snprintf(C.ab_tag, sizeof C.ab_tag, "%s", strlen(k) > 9 ? k + 9 : "");
    }
    if (lang_reply_hook()[0]) C.hook_n++;      /* chunk 41: a change of subject took a hook: the next takes the next one */
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
    if (heard_has(h, I_SORRY) && C.insulted_count) C.apologized = 1;      /* chunk 39 */
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
