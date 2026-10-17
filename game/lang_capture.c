/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * Capturing facts: see lang_capture.h. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lang_capture.h"

#define MAXW 48
typedef struct { char w[24]; int a, b; int brk; int cap; } Tok;     /* w = lower case, apostrophes dropped; brk = a clause break (. ! ? ; , :) follows; cap = typed with a capital */

static int is_wc(unsigned char c) { return isalnum(c) || c == '\'' || c == '-' || c == '*' || c == '#'; }

static int tokenize(const char *lc, const char *lcase, int len, Tok *t) {
    int n = 0, i = 0;
    while (i < len && n < MAXW) {
        while (i < len && !is_wc((unsigned char)lc[i])) i++;
        if (i >= len) break;
        Tok *k = &t[n]; int a = i, o = 0;
        while (i < len && is_wc((unsigned char)lc[i])) { if (lc[i] != '\'' && o < 23) k->w[o++] = lc[i]; i++; }
        k->w[o] = 0; k->a = a; k->b = i; k->brk = 0;
        k->cap = lcase && isupper((unsigned char)lcase[a]);
        int j = i; while (j < len && !is_wc((unsigned char)lc[j])) { if (strchr(".!?;,:", lc[j])) k->brk = 1; j++; }
        if (o) n++;
    }
    return n;
}
static int is(const Tok *t, int n, int i, const char *w) { return i >= 0 && i < n && !strcmp(t[i].w, w); }
static int in_list(const char *w, const char *list) {           /* list = "a|b|c" */
    size_t n = strlen(w); if (!n) return 0;
    for (const char *p = list; *p; ) {
        const char *e = strchr(p, '|'); size_t l = e ? (size_t)(e - p) : strlen(p);
        if (l == n && !strncmp(p, w, n)) return 1;
        if (!e) break;
        p = e + 1;
    }
    return 0;
}

/* ---- blocked words: swear, sexual, self-harm. FIRST CUT: it has no slurs on purpose; the owner's safety review (chunk 158) and
 * lang_blocked_test extend it. a word of 4+ letters marked S is blocked inside a longer word too ---- */
static const char *const BLOCK_WHOLE = "ass|arse|dick|cock|piss|sex|cum|die|dies|kill|porn|rape|raped|penis|tits|tit|anal|damn|hell|crap|boob|boobs|nude|nudes|dildo|horny|sexy|incest|murder|suicide|end|ends|ending|disappear|vanish|hurt|harm|cutting|overdose";
static const char *const BLOCK_SUB = "fuck|shit|bitch|cunt|pussy|whore|slut|bastard|nazi|hitler|wank|twat|asshole|dickhead|motherf|suicid|rapist|pedo";
static char g_extra[16][24]; static int g_nextra;
int lang_blocked_test(const char *w) { if (g_nextra >= 16 || strlen(w) > 23) return 0; strcpy(g_extra[g_nextra++], w); return 1; }
void lang_blocked_clear_test(void) { g_nextra = 0; }

int lang_name_blocked(const char *text) {
    char w[24]; int o = 0;
    for (const char *p = text; ; p++) {
        unsigned char c = (unsigned char)*p;
        if (isalpha(c)) { if (o < 23) w[o++] = (char)tolower(c); continue; }
        if (c == '\'' || c == '-') continue;
        if (o) {
            w[o] = 0; o = 0;
            if (in_list(w, BLOCK_WHOLE)) return 1;
            for (int k = 0; k < g_nextra; k++) if (!strcmp(w, g_extra[k])) return 1;
            for (const char *s = BLOCK_SUB; *s; ) {
                const char *e = strchr(s, '|'); size_t l = e ? (size_t)(e - s) : strlen(s);
                char b[24]; if (l < sizeof b) { memcpy(b, s, l); b[l] = 0; if (strstr(w, b)) return 1; }
                if (!e) break;
                s = e + 1;
            }
        }
        if (!c) break;
    }
    return 0;
}

/* ---- common first names (lower case, mixed, short: the capital rule covers the rest) ---- */
static const char *const NAMES =
    "adam|alan|alex|alice|amy|andrew|andy|anna|anne|anthony|ben|beth|bill|bob|bobby|brad|brian|carl|carlos|carol|charlie|chris|claire|dan|daniel|dave|david|"
    "diana|diego|dylan|ed|eddie|elena|ella|emily|emma|eric|eva|evan|fred|gabriel|george|grace|greg|hannah|harry|helen|henry|ian|isaac|jack|jake|james|jane|"
    "jason|jay|jeff|jen|jenny|jess|jessica|jim|joe|john|jon|jose|josh|joy|juan|julia|julie|justin|karen|kate|katie|ken|kevin|kim|larry|laura|lee|leo|lily|"
    "lisa|liz|lucas|lucy|luke|maria|mark|martin|mary|matt|max|mia|mike|mina|nancy|nick|noah|olivia|omar|oscar|pam|pat|paul|pete|peter|phil|rachel|ray|"
    "rick|rob|robert|roy|ruby|ryan|sam|sara|sarah|scott|sean|sofia|sophie|stan|steve|sue|tara|ted|tim|tom|tony|tyler|victor|vince|will|zack|zoe|"
    "ahmed|ali|amir|aya|chen|dmitri|elif|fatima|hana|hiro|ivan|kai|lena|li|mei|nina|olga|pedro|raj|ravi|rosa|sasha|sven|tariq|wei|yuki|yusuf";
int lang_name_listed(const char *w) { return in_list(w, NAMES); }
int lang_namelist_count(void) { int n = 1; for (const char *p = NAMES; *p; p++) if (*p == '|') n++; return n; }

/* words that are never a name (and never a thing to like, a place ...): pronouns, small words, feelings, answers */
static const char *const STOP =
    "i|me|my|mine|you|your|yours|u|ur|we|us|our|he|him|his|she|her|it|its|they|them|their|this|that|these|those|the|a|an|and|but|or|so|if|then|than|because|when|while|"
    "what|who|whom|why|how|where|which|not|no|yes|yeah|nope|ok|okay|nothing|nobody|nowhere|none|anything|something|someone|everything|everyone|here|there|just|also|very|really|"
    "too|still|only|even|never|always|ever|sorry|please|thanks|thank|hello|hi|hey|bye|fine|good|bad|well|back|later|again|now|soon|today|tomorrow|maybe|perhaps|sure|"
    "is|am|are|was|were|be|been|being|do|does|did|dont|cant|wont|will|would|can|could|should|have|has|had|not|to|of|in|on|at|for|from|with|by|about|like|as|into|out|up|down|"
    "going|gonna|wanna|trying|called|named|human|person|guy|girl|man|woman|boy|kid|friend|stranger|vessel|god|goddess|idiot|stupid|crazy|dumb|lost|here|home";
static int stopw(const char *w) { return in_list(w, STOP); }
static const char *const FEEL =
    "tired|sad|happy|scared|angry|hungry|lost|bored|fine|ok|okay|good|bad|cold|sick|alone|lonely|confused|afraid|excited|nervous|sorry|ready|lucky|curious|worried|hurt|hot|"
    "sleepy|weak|strong|brave|stuck|dizzy|thirsty|cranky|anxious|upset|mad|glad|proud|ashamed|jealous|numb|calm|relaxed|stressed|overwhelmed|hopeful|annoyed|furious|shy|"
    "empty|dead|safe|free|great|terrible|awful|amazing|nice|better|worse|different|strange|small|tiny|old|new|young";
static const char *const BOOST = "very|so|really|extremely|totally|kinda|sorta|quite|pretty|too|bit|little|a|rather|somewhat|a|slightly|truly|super|just";
static const char *const PRONOUN = "anyone|anybody|everybody|somebody|nobody|you|u|it|this|that|them|him|her|me|us|ya|yourself|myself|everyone|everything|nothing|something|anything|these|those|they|he|she|we|how|what|when|where|if|who|why|which|there|here|out|in|up|off|on";
static const char *const DET = "a|an|the|some|my|your|his|her|our|their|more|all|any|to|lots|of|much|many|really|so|very|just|also|always|totally|truly|still|too|even|actually";

/* Bob, BOB and bob are all Bob; Mary Ann keeps its two capitals; McDonald keeps its inner capital (only an ALL CAPS word is lowered) */
static void title_case(char *s) {
    for (char *w = s; *w; ) {
        char *e = w; int up = 1, n = 0;
        while (*e && *e != ' ') { if (islower((unsigned char)*e)) up = 0; n++; e++; }
        for (char *p = w; p < e; p++) *p = (p == w) ? (char)toupper((unsigned char)*p) : (up && n > 1 ? (char)tolower((unsigned char)*p) : *p);
        w = *e ? e + 1 : e;
    }
}
/* a name token: letters / - only, 2-22 long, not a stop word, not blocked */
static int name_tok_ok(const Tok *k) {
    size_t n = strlen(k->w); if (n < 2 || n > 22) return 0;
    for (size_t i = 0; i < n; i++) if (!isalpha((unsigned char)k->w[i]) && k->w[i] != '-') return 0;
    if (stopw(k->w) || in_list(k->w, FEEL)) return 0;
    return 1;
}
static void add(Capture *out, int *n, int max, int type, const char *text, int *seen) {
    if (*n >= max) return;
    for (int i = 0; i < *n; i++) if (out[i].type == type && !strcmp(out[i].text, text)) return;
    if (type != CAP_LIKE && type != CAP_DISLIKE) for (int i = 0; i < *n; i++) if (out[i].type == type) return;     /* one of each kind (but several likes) */
    Capture *c = &out[*n]; memset(c, 0, sizeof *c); c->type = (uint8_t)type; snprintf(c->text, CAP_TEXT, "%s", text); (*n)++; (void)seen;
}
/* joins the typed words of tokens i..i+k-1 (case as typed) */
static void join_tok(const char *src, const Tok *t, int i, int k, char *out, int cap) {
    int o = 0; out[0] = 0;
    for (int q = 0; q < k && o < cap - 2; q++) {
        if (q) out[o++] = ' ';
        for (int p = t[i + q].a; p < t[i + q].b && o < cap - 1; p++) if (src[p] != '\'') out[o++] = src[p];
    }
    out[o] = 0;
}
/* a name starting at token i: one word, and a second one only when it is typed with a capital and follows without a break */
static int take_name(const char *lcase, const Tok *t, int n, int i, int force, int shout, char *out) {
    out[0] = 0;
    if (i >= n || !name_tok_ok(&t[i])) return 0;
    int ok = force || lang_name_listed(t[i].w) || (t[i].cap && !shout);
    if (!ok) return 0;
    int k = 1;
    if (!t[i].brk && i + 1 < n && t[i + 1].cap && !shout && name_tok_ok(&t[i + 1])) k = 2;
    join_tok(lcase, t, i, k, out, CAP_TEXT);
    if (strlen(out) > 22) { join_tok(lcase, t, i, 1, out, CAP_TEXT); k = 1; }
    return k;
}
/* the words of an object (a thing, a place, something liked): skip determiners, stop at a stop word, a break or 2 words */
static int take_obj(const char *lcase, const Tok *t, int n, int i, int maxw, char *out) {
    out[0] = 0;
    while (i < n && in_list(t[i].w, DET) && !t[i - 1 < 0 ? 0 : i - 1].brk) i++;
    if (i >= n || in_list(t[i].w, PRONOUN) || stopw(t[i].w)) return 0;
    int k = 0;
    while (k < maxw && i + k < n) {
        const char *w = t[i + k].w; size_t l = strlen(w);
        if (l < 2 || (k && (stopw(w) || in_list(w, DET)))) break;
        int okl = 1; for (size_t q = 0; q < l; q++) if (!isalpha((unsigned char)w[q]) && w[q] != '-') okl = 0;
        if (!okl) break;
        k++;
        if (t[i + k - 1].brk) break;
    }
    if (!k) return 0;
    join_tok(lcase, t, i, k, out, CAP_TEXT);
    for (char *p = out; *p; p++) *p = (char)tolower((unsigned char)*p);       /* things are kept lower case; names keep their capitals */
    return k;
}

static const char *const V_LIKE = "like|likes|love|loves|enjoy|enjoys|adore|adores|cherish|prefer";
static const char *const V_HATE = "hate|hates|dislike|dislikes|despise|loathe|detest";
static const char *const V_WANT = "want|need|wanna|crave";
static const char *const ADVERB = "really|totally|absolutely|truly|just|also|still|always|definitely|honestly|actually|kinda|sorta|do|did|so|very|much|quite|pretty|even|simply|genuinely";

int lang_capture_read(const char *lc, const char *lcase, int len, int shout, Capture *out, int max, int *blocked) {
    Capture tmp[HEARD_CAPS * 2]; Capture *o = out ? out : tmp; if (!out || max > HEARD_CAPS * 2) max = out ? max : HEARD_CAPS * 2;
    if (!lcase) lcase = lc;
    int n = 0, nt, bl = 0, seen = 0; Tok t[MAXW];
    nt = tokenize(lc, lcase, len, t);
    char buf[48];
    for (int i = 0; i < nt && n < max; i++) {
        /* ---- NAME: the sure frames ---- */
        int at = -1, force = 1;
        if (is(t, nt, i, "my") && (is(t, nt, i + 1, "name") && (is(t, nt, i + 2, "is") || is(t, nt, i + 2, "s")))) at = i + 3;
        else if (is(t, nt, i, "my") && is(t, nt, i + 1, "names")) at = i + 2;
        else if (is(t, nt, i, "call") && is(t, nt, i + 1, "me") && !(i && (is(t, nt, i - 1, "dont") || is(t, nt, i - 1, "not") || is(t, nt, i - 1, "never")))) at = i + 2;
        else if (is(t, nt, i, "i") && ((is(t, nt, i + 1, "am") && is(t, nt, i + 2, "called")) || (is(t, nt, i + 1, "go") && is(t, nt, i + 2, "by")))) at = i + 3;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "am") && is(t, nt, i + 2, "named")) at = i + 3;
        else if (is(t, nt, i, "im") && (is(t, nt, i + 1, "called") || is(t, nt, i + 1, "named"))) at = i + 2;
        else if (is(t, nt, i, "name") && is(t, nt, i + 1, "is") && i == 0) at = i + 2;                       /* name is Bob: only as the first words */
        else if ((is(t, nt, i, "i") && is(t, nt, i + 1, "am")) || is(t, nt, i, "im")) { at = i + (is(t, nt, i, "im") ? 1 : 2); force = 0; }
        if (at >= 0 && at < nt) {
            char nm[CAP_TEXT];
            int k = take_name(lcase, t, nt, at, force, shout, nm);
            if (!k && force && at < nt) {                                                                    /* a word that is only a blocked name still counts as an offer */
                size_t l = strlen(t[at].w); int lett = 1; for (size_t q = 0; q < l; q++) if (!isalpha((unsigned char)t[at].w[q]) && t[at].w[q] != '-' && t[at].w[q] != '*' && t[at].w[q] != '#') lett = 0;
                if (lett && l >= 2 && l <= 22 && !stopw(t[at].w)) { join_tok(lcase, t, at, 1, nm, CAP_TEXT); k = 1; if (strpbrk(t[at].w, "*#")) bl = 1; }
            }
            if (k) {
                if (lang_name_blocked(nm)) bl = 1;
                else { title_case(nm); add(o, &n, max, CAP_NAME, nm, &seen); }
            }
        }
        /* ---- FEELING: i am (very) tired / im sad / i feel lost / i am feeling x ---- */
        int f = -1;
        if (is(t, nt, i, "i") && (is(t, nt, i + 1, "am") || is(t, nt, i + 1, "feel") || (is(t, nt, i + 1, "am") && is(t, nt, i + 2, "feeling")))) f = i + 2;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "am") && is(t, nt, i + 2, "feeling")) f = i + 3;
        else if (is(t, nt, i, "im")) f = i + 1;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "feel")) f = i + 2;
        if (f >= 0) {
            int g = f; while (g < nt && g < f + 3 && in_list(t[g].w, BOOST) && !t[g - 1].brk) g++;
            if (g < nt && in_list(t[g].w, FEEL) && !(is(t, nt, g - 1, "not"))) {
                int neg = 0; for (int q = i; q < f; q++) if (is(t, nt, q, "not")) neg = 1;
                if (!neg) add(o, &n, max, CAP_FEELING, t[g].w, &seen);
            }
        }
        /* ---- i ... verbs ---- */
        if (is(t, nt, i, "i")) {
            do {
            int j = i + 1, neg = 0, cant = 0;
            for (int g = 0; g < 3 && j < nt; g++) {
                if (is(t, nt, j, "dont") || is(t, nt, j, "never") || is(t, nt, j, "not")) { neg = 1; j++; }
                else if (is(t, nt, j, "do") && is(t, nt, j + 1, "not")) { neg = 1; j += 2; }
                else if (is(t, nt, j, "cant") || is(t, nt, j, "cannot")) { cant = 1; j++; }
                else if (is(t, nt, j, "can") && is(t, nt, j + 1, "not")) { cant = 1; j += 2; }
                else if (in_list(t[j].w, ADVERB) && !t[j - 1].brk) j++;
                else break;
            }
            if (j >= nt) break;
            const char *v = t[j].w; int kind = 0, vi = j;                 /* kind 1 like, 2 hate, 3 want, 4 cant stand */
            if (in_list(v, V_LIKE)) kind = 1;
            else if (in_list(v, V_HATE)) kind = 2;
            else if (in_list(v, V_WANT)) kind = 3;
            else if (cant && !strcmp(v, "stand")) kind = 4;
            else if (is(t, nt, j, "am") && !neg) {                         /* i am sick of X / i am tired of X / i am into X */
                if ((is(t, nt, j + 1, "sick") || is(t, nt, j + 1, "tired")) && is(t, nt, j + 2, "of")) { kind = 2; vi = j + 2; }
                else if (is(t, nt, j + 1, "into")) { kind = 1; vi = j + 1; }
            }
            else if (is(t, nt, j, "would") && is(t, nt, j + 1, "like") && !neg && is(t, nt, j + 2, "to") == 0) { kind = 3; vi = j + 1; }
            if (!kind) break;
            if (kind == 4 && !neg && !cant) break;
            int k = take_obj(lcase, t, nt, vi + 1, 2, buf);
            if (!k || (t[vi].brk && kind != 3)) break;
            if (kind == 1 && !strcmp(buf, "to") ) break;
            if (lang_name_blocked(buf)) break;
            if (kind == 1) add(o, &n, max, neg ? CAP_DISLIKE : CAP_LIKE, buf, &seen);
            else if (kind == 2) { if (!neg) add(o, &n, max, CAP_DISLIKE, buf, &seen); }
            else if (kind == 4) add(o, &n, max, CAP_DISLIKE, buf, &seen);
            else if (kind == 3 && !neg) add(o, &n, max, CAP_THING, buf, &seen);
            } while (0);
        }
        /* ---- my favorite Y is X ---- */
        if (is(t, nt, i, "my") && (is(t, nt, i + 1, "favorite") || is(t, nt, i + 1, "favourite")) && i + 3 < nt) {
            int j = i + 3; if (is(t, nt, i + 3, "is") || is(t, nt, i + 3, "are")) j = i + 4; else if (is(t, nt, i + 2, "is")) j = i + 3; else continue;
            if (take_obj(lcase, t, nt, j, 2, buf) && !lang_name_blocked(buf)) add(o, &n, max, CAP_LIKE, buf, &seen);
        }
        /* ---- PLACE ---- */
        int pl = -1;
        if (is(t, nt, i, "i") && is(t, nt, i + 1, "live") && (is(t, nt, i + 2, "in") || is(t, nt, i + 2, "at") || is(t, nt, i + 2, "near"))) pl = i + 3;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "am") && is(t, nt, i + 2, "from")) pl = i + 3;
        else if (is(t, nt, i, "im") && is(t, nt, i + 1, "from")) pl = i + 2;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "come") && is(t, nt, i + 2, "from")) pl = i + 3;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "was") && is(t, nt, i + 2, "born") && is(t, nt, i + 3, "in")) pl = i + 4;
        else if (is(t, nt, i, "i") && is(t, nt, i + 1, "grew") && is(t, nt, i + 2, "up") && is(t, nt, i + 3, "in")) pl = i + 4;
        else if (is(t, nt, i, "my") && (is(t, nt, i + 1, "home") || is(t, nt, i + 1, "hometown")) && is(t, nt, i + 2, "is")) pl = i + 3;
        if (pl >= 0 && take_obj(lcase, t, nt, pl, 3, buf)) {
            const char *first = buf; if (!strcmp(first, "nowhere") || !strcmp(first, "nothing")) continue;
            if (!lang_name_blocked(buf)) { for (char *p = buf; *p; p++) if (p == buf || p[-1] == ' ') *p = (char)toupper((unsigned char)*p); add(o, &n, max, CAP_PLACE, buf, &seen); }
        }
    }
    if (blocked) *blocked = bl;
    return n;
}

const char *lang_capture_type_name(int type) {
    static const char *const N[CAP_COUNT] = { "none", "name", "age", "place", "like", "dislike", "thing", "number", "time", "feeling" };
    return type >= 0 && type < CAP_COUNT ? N[type] : "?";
}

int lang_name_clean(char *name, int *blocked) {
    Tok t[MAXW]; int len = (int)strlen(name);
    if (blocked) *blocked = 0;
    if (len > 60) { name[0] = 0; return 0; }
    if (lang_name_blocked(name)) { if (blocked) *blocked = 1; name[0] = 0; return 0; }
    char low[64]; for (int i = 0; i <= len; i++) low[i] = (char)tolower((unsigned char)name[i]);
    int n = tokenize(low, name, len, t);
    if (n < 1 || !name_tok_ok(&t[0])) { name[0] = 0; return 0; }
    int k = (n > 1 && !t[0].brk && t[1].cap && name_tok_ok(&t[1])) ? 2 : 1;
    char out[CAP_TEXT]; join_tok(name, t, 0, k, out, CAP_TEXT);
    if (strlen(out) > 22) join_tok(name, t, 0, 1, out, CAP_TEXT);
    title_case(out);
    snprintf(name, 24, "%s", out);
    return 1;
}
