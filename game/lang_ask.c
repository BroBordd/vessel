/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * ASK TAGS (docs/convo_plan.md, chunk 36): see lang_ask.h. */
#include "lang_ask.h"
#include "lang_data.h"
#include "lang_clean.h"
#include "lang_frame.h"
#include "lang_vocab.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *const TAGS[] = { "yn", "name", "why", "any", "choice", "more", "ready", "agree", NULL };
int lang_ask_tag_valid(const char *t) {
    if (!t || !*t) return 0;
    for (int i = 0; TAGS[i]; i++) if (!strcmp(t, TAGS[i])) return 1;
    return 0;
}
static const char *tag_const(const char *t) { for (int i = 0; TAGS[i]; i++) if (!strcmp(t, TAGS[i])) return TAGS[i]; return ""; }

/* the last question sentence of a text: from after the last . ! ? before the final question mark, through that mark */
void lang_ask_sentence(const char *text, char *out, int cap) {
    out[0] = 0;
    if (!text || cap < 2) return;
    int e = -1;
    for (int i = 0; text[i]; i++) if (text[i] == '?') e = i;
    if (e < 0) return;
    int s = e;
    while (s > 0 && text[s - 1] != '.' && text[s - 1] != '!' && text[s - 1] != '?') s--;
    while (text[s] == ' ') s++;
    int n = e - s + 1;
    if (n < 1) return;
    if (n > cap - 1) n = cap - 1;
    memcpy(out, text + s, (size_t)n); out[n] = 0;
}

/* a lower case copy of a sentence without apostrophes and {slots}, words separated by one space, the question mark and commas kept as separators */
static void norm(const char *in, char *out, int cap) {
    int o = 0, brace = 0, sp = 1;
    for (; *in && o < cap - 2; in++) {
        unsigned char c = (unsigned char)*in;
        if (c == '{') { brace = 1; continue; }
        if (c == '}') { brace = 0; if (!sp) { out[o++] = ' '; sp = 1; } continue; }
        if (brace || c == '\'') continue;
        if (isalpha(c) || isdigit(c)) { out[o++] = (char)tolower(c); sp = 0; }
        else if (c == ',') { if (o && out[o - 1] == ' ') o--; out[o++] = ','; out[o++] = ' '; sp = 1; }
        else if (!sp) { out[o++] = ' '; sp = 1; }
    }
    while (o && out[o - 1] == ' ') o--;
    out[o] = 0;
}
static int first_word_is(const char *s, const char *const *list) {
    char w[16]; int n = 0;
    while (s[n] && s[n] != ' ' && s[n] != ',' && n < 15) { w[n] = s[n]; n++; }
    w[n] = 0;
    for (int i = 0; list[i]; i++) if (!strcmp(w, list[i])) return 1;
    return 0;
}
static const char *const AUX[] = { "do", "did", "does", "is", "are", "am", "was", "were", "will", "would", "can", "could", "should", "shall", "have", "has", "had", "may", "must", NULL };
static const char *const ANY[] = { "any", "anything", NULL };
static const char *const WHYW[] = { "why", NULL };

const char *lang_ask_tag_text(const char *text) {
    char sent[160], s[200];
    lang_ask_sentence(text, sent, (int)sizeof sent);
    if (!sent[0]) return "";
    norm(sent, s, (int)sizeof s);
    if (strstr(s, "your name") || strstr(s, "call you") || !strncmp(s, "who are you", 11)) return tag_const("name");
    if (first_word_is(s, WHYW) || !strncmp(s, "how come", 8)) return tag_const("why");
    const char *or_ = strstr(s, " or ");
    if (or_ && strncmp(or_, " or not", 7) && strncmp(or_, " or what", 8) && strncmp(or_, " or something", 13) && strncmp(or_, " or else", 8) && strncmp(or_, " or no", 6)) return tag_const("choice");
    if (first_word_is(s, AUX)) return tag_const("yn");
    for (const char *c = strstr(s, ", "); c; c = strstr(c + 2, ", ")) if (first_word_is(c + 2, AUX)) return tag_const("yn");      /* Before you go, do you have any questions? */
    if (first_word_is(s, ANY) || strstr(s, "any questions")) return tag_const("any");
    return "";
}
const char *lang_line_ask(const Line *l) {
    if (!l) return "";
    if (l->ask && lang_ask_tag_valid(l->ask)) return tag_const(l->ask);
    return lang_ask_tag_text(l->text);
}

/* ---------- reading an answer ---------- */
#define MAXW 10
typedef struct { char w[MAXW][24]; int n; } Toks;
static int tokenize2(const char *lc, Toks *t, int allowq) {
    t->n = 0;
    const char *p = lc;
    while (*p) {
        while (*p && !isalnum((unsigned char)*p)) {
            if (*p == '?' && !allowq) return 0;       /* a question back is not a bare answer */
            p++;
        }
        if (!*p) break;
        if (t->n >= MAXW) return 0;
        int o = 0;
        while (*p && (isalnum((unsigned char)*p) || *p == '\'')) { if (*p != '\'' && o < 23) t->w[t->n][o++] = (char)tolower((unsigned char)*p); p++; }
        t->w[t->n][o] = 0; t->n++;
    }
    return t->n > 0;
}
static int unit_at(const Toks *t, int at, const char *unit) {         /* does the word run at `at` spell `unit` (words separated by spaces)? returns its word count, 0 if not */
    int k = 0; const char *u = unit;
    while (*u) {
        const char *e = strchr(u, ' '); size_t n = e ? (size_t)(e - u) : strlen(u);
        if (at + k >= t->n || strlen(t->w[at + k]) != n || strncmp(t->w[at + k], u, n)) return 0;
        k++; u += n; if (*u == ' ') u++;
    }
    return k;
}
static int tokenize(const char *lc, Toks *t) { return tokenize2(lc, t, 0); }
/* consume the whole token run with units of `good` (at least one needed) and `neutral` (fillers, always allowed); 1 if it all parses */
static int consume(const Toks *t, const char *const *good, const char *const *neutral) {
    int at = 0, hit = 0;
    while (at < t->n) {
        int best = 0, isgood = 0;
        for (int i = 0; good[i]; i++) { int k = unit_at(t, at, good[i]); if (k > best) { best = k; isgood = 1; } }
        for (int i = 0; neutral[i]; i++) { int k = unit_at(t, at, neutral[i]); if (k > best) { best = k; isgood = 0; } }
        if (!best) return 0;
        if (isgood) hit = 1;
        at += best;
    }
    return hit;
}
static const char *const NEUTRAL[] = { "well", "um", "uh", "hmm", "oh", "ah", "so", "then", "please", "pls", "sir", "thanks", "thank you", "mm", "mhm", "hm", "lol", "haha", NULL };
static const char *const YES_UNITS[] = { "yes", "yeah", "yep", "yup", "ya", "yea", "ys", "sure", "ok", "okay", "k", "kk", "fine", "alright", "all right", "aye", "definitely", "absolutely",
    "certainly", "of course", "indeed", "gladly", "sure thing", "i do", "i am", "i will", "i can", "i would", "i have", "i did", "i guess", "i guess so", "i think so", "i suppose", "i suppose so",
    "why not", "go on", "go ahead", "do it", "lets go", "lets do it", "im ready", "i am ready", "ready", "thats right", "right", "correct", "exactly", "tell me", "yes sir", NULL };
static const char *const NO_UNITS[] = { "no", "nope", "nah", "naw", "nay", "never", "not really", "not now", "not yet", "not at all", "no way", "no thanks", "no thank you", "i do not", "i dont", "i am not",
    "im not", "i will not", "i wont", "i would not", "i cannot", "i cant", "nothing", "none", "i have none", "i have nothing", "no questions", "no more", "i have no questions", "not today", "not i", NULL };
static const char *const REASON_FIRST[] = { "because", "cause", "cuz", "coz", "bcuz", "becuz", "becos", "bc", "cos", "since", NULL };
static const char *const REASON_UNITS[] = { "no reason", "for no reason", "no particular reason", "for reasons", "reasons", NULL };

static void title(char *d, const char *s) { int i = 0; for (; s[i] && i < 23; i++) d[i] = (char)(i ? tolower((unsigned char)s[i]) : toupper((unsigned char)s[i])); d[i] = 0; }

/* the choice: tokens left after the lead-ins are stripped, compared with the two options of the question */
static const char *const LEAD[] = { "i pick", "i choose", "i prefer", "i want", "i like", "i would pick", "i would choose", "i would say", "i say", "id pick", "id say", "id choose", "ill take", "ill pick",
    "ill choose", "ill go with", "go with", "im going with", "i am going with", "i guess", "i think", "definitely", "probably", "maybe", "well", "um", "uh", "hmm", "the", "a", "an", "this", "that", "my", "pick", "take", NULL };
static const char *const CTAIL[] = { "please", "i guess", "then", "one", "too", "thanks", NULL };
static int join_rest(const Toks *t, int from, int to, char *out, int cap) { int o = 0; out[0] = 0; for (int i = from; i < to; i++) { int n = (int)strlen(t->w[i]); if (o + n + 2 > cap) return 0; if (o) out[o++] = ' '; memcpy(out + o, t->w[i], (size_t)n); o += n; out[o] = 0; } return o; }
static int strip_article(const char *s, char *out, int cap) { if (!strncmp(s, "the ", 4)) s += 4; else if (!strncmp(s, "a ", 2)) s += 2; else if (!strncmp(s, "an ", 3)) s += 3; snprintf(out, (size_t)cap, "%s", s); return (int)strlen(out); }
static int read_choice(const char *ask_text, const Toks *t, AskAnswer *out) {
    if (!ask_text || !*ask_text) return 0;
    char c[200]; int map[200];
    int n = lang_clean(ask_text, c, (int)sizeof c, map);
    if (n <= 0) return 0;
    Question q; memset(&q, 0, sizeof q);
    if (lang_question(c, 0, n, &q) != QT_CHOICE || !q.opt1[0] || !q.opt2[0]) return 0;
    int a = 0, b = t->n;
    for (int go = 1; go;) {                                 /* strip lead-ins and tails */
        go = 0;
        for (int i = 0; LEAD[i] && a < b; i++) { int k = unit_at(t, a, LEAD[i]); if (k && a + k <= b) { a += k; go = 1; break; } }
        for (int i = 0; CTAIL[i] && a < b; i++) { int k = 1; const char *u = CTAIL[i]; if (strchr(u, ' ')) k = 2; if (b - k >= a && unit_at(t, b - k, u) == k) { b -= k; go = 1; break; } }
    }
    if (a >= b) return 0;
    char rest[48], o1[48], o2[48], tmp[48];
    if (!join_rest(t, a, b, tmp, (int)sizeof tmp)) return 0;
    strip_article(tmp, rest, (int)sizeof rest);
    strip_article(q.opt1, o1, (int)sizeof o1); strip_article(q.opt2, o2, (int)sizeof o2);
    int pick = 0;
    if (!strcmp(rest, o1) || !strcmp(rest, "first") || !strcmp(rest, "1st") || !strcmp(rest, "1") || !strcmp(rest, "former") || !strcmp(rest, "first one")) pick = 1;
    else if (!strcmp(rest, o2) || !strcmp(rest, "second") || !strcmp(rest, "2nd") || !strcmp(rest, "2") || !strcmp(rest, "latter") || !strcmp(rest, "second one")) pick = 2;
    else if (!strcmp(rest, "both") || !strcmp(rest, "all") || !strcmp(rest, "either") || !strcmp(rest, "whichever") || !strcmp(rest, "both of them")) pick = 3;
    else if (!strcmp(rest, "neither") || !strcmp(rest, "none") || !strcmp(rest, "none of them") || !strcmp(rest, "neither one")) pick = 4;
    if (!pick) return 0;
    out->pick = pick;
    return 1;
}

int lang_ask_read(const char *tag, const char *ask_text, const char *lc, const char *lcase, AskAnswer *out) {
    memset(out, 0, sizeof *out);
    if (!tag || !*tag || !lc) return ASK_NONE;
    Toks t;
    if (!tokenize(lc, &t)) return ASK_NONE;
    int yn = !strcmp(tag, "yn") || !strcmp(tag, "any") || !strcmp(tag, "more") || !strcmp(tag, "ready") || !strcmp(tag, "agree");
    if (yn) {
        if (consume(&t, NO_UNITS, NEUTRAL)) { out->kind = ASK_NO; return ASK_NO; }
        if (consume(&t, YES_UNITS, NEUTRAL)) { out->kind = ASK_YES; return ASK_YES; }
        return ASK_NONE;
    }
    if (!strcmp(tag, "why")) {
        int at = 0; if (unit_at(&t, 0, "just")) at = 1;
        for (int i = 0; REASON_FIRST[i]; i++) if (at < t.n && !strcmp(t.w[at], REASON_FIRST[i])) { out->kind = ASK_BECAUSE; return ASK_BECAUSE; }
        if (consume(&t, REASON_UNITS, NEUTRAL)) { out->kind = ASK_BECAUSE; return ASK_BECAUSE; }
        return ASK_NONE;
    }
    if (!strcmp(tag, "name")) {
        int at = 0;
        if (unit_at(&t, 0, "its") || unit_at(&t, 0, "it is")) at = unit_at(&t, 0, "its") ? 1 : 2;
        if (t.n - at != 1) return ASK_NONE;
        const char *w = t.w[at];
        int n = (int)strlen(w);
        if (n < 2 || n > 22) return ASK_NONE;
        for (int i = 0; i < n; i++) if (!isalpha((unsigned char)w[i])) return ASK_NONE;
        if (consume(&t, YES_UNITS, NEUTRAL) || consume(&t, NO_UNITS, NEUTRAL)) return ASK_NONE;       /* yes / no / ok are no names */
        int capital = 0;
        if (lcase) {                                         /* the word as typed: is its first letter a capital? */
            int wi = -1, inw = 0;
            for (const char *p = lcase; *p; p++) {
                int c = isalnum((unsigned char)*p) || *p == '\'';
                if (c && !inw) { wi++; if (wi == at) { capital = isupper((unsigned char)*p) != 0; break; } }
                inw = c;
            }
        }
        if (!capital && !vocab_is_name(w)) return ASK_NONE;
        title(out->name, w);
        out->kind = ASK_NAME; return ASK_NAME;
    }
    if (!strcmp(tag, "choice")) {
        if (read_choice(ask_text, &t, out)) { out->kind = ASK_CHOICE; return ASK_CHOICE; }
        return ASK_NONE;
    }
    return ASK_NONE;
}

/* ---------- chunk 37: follow-ups ---------- */
static const char *const FU_FILL[] = { "well", "um", "uh", "hmm", "oh", "ah", "but", "please", "pls", "mm", "hm", NULL };
static const char *const FW_WHY[] = { "why", "why not", "how come", "why is that", "why so", "why though", "why is it", "why do you say that", "why would you say that", NULL };
static const char *const FW_MORE[] = { "and", "and then", "and so", "so", "then what", "go on", "more", "continue", "tell me more", "explain", "elaborate", "like what", "for example", "for instance",
    "in what way", "what do you mean", "what does that mean", "what do you mean by that", "how so", "such as", "example", "keep going", "carry on", "go on then", "explain more", "say more", "and what else", "what else", "meaning", NULL };
static const char *const FW_REALLY[] = { "really", "seriously", "are you sure", "truly", "for real", "is that so", "is that true", "you sure", "honestly", "really really", "is it", "is that right", "no kidding", NULL };
static const char *const FW_AGAIN[] = { "what", "huh", "pardon", "sorry what", "say that again", "say again", "again", "repeat", "repeat that", "what did you say", "come again", "eh", "excuse me", "once more",
    "i did not hear", "i didnt hear", "i did not catch that", "i didnt catch that", "say it again", "what was that", "what was that again", "can you repeat that", "sorry", "wait what", "hm what", NULL };
int lang_follow_read(const char *lc) {
    Toks t;
    if (!lc || !tokenize2(lc, &t, 1) || t.n > 8) return FU_NONE;
    if (consume(&t, FW_WHY, FU_FILL)) return FU_WHY;
    if (consume(&t, FW_REALLY, FU_FILL)) return FU_REALLY;
    if (consume(&t, FW_AGAIN, FU_FILL)) return FU_AGAIN;
    if (consume(&t, FW_MORE, FU_FILL)) return FU_MORE;
    return FU_NONE;
}
