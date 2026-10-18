/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * DIALOGUE MEMORY (docs/convo_plan.md, chunk 35): see lang_ctx.h. */
#include "lang_ctx.h"
#include "lang_clean.h"
#include <string.h>
#include <stdio.h>

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
