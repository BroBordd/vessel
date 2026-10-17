/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * SENTENCE FRAMES (docs/convo_plan.md, chunk 30), see lang_frame.h. No SDL. */
#include "lang_frame.h"
#include "lang.h"
#include "lang_clause.h"
#include "lang_lemma.h"
#include "lang_lex.h"
#include "lang_vocab.h"
#include <stdio.h>
#include <string.h>

#define WL 16
typedef struct { char w[WL], l[WL]; } W;      /* the word without apostrophes, and its lemma */

static int g_on = 1;
void lang_frame_set(int on) { g_on = on != 0; }
int  lang_frame_enabled(void) { return g_on; }

/* ---- words ---- */
static int wch(char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '\''; }

static int tokenize(const char *lc, int a, int b, W *w, int max, int *qmark) {
    int n = 0; *qmark = 0;
    for (int i = a; i < b && lc[i]; ) {
        if (lc[i] == '?') *qmark = 1;
        if (!wch(lc[i])) { i++; continue; }
        char t[WL]; int k = 0;
        while (i < b && lc[i] && wch(lc[i])) { if (lc[i] != '\'' && k < WL - 1) t[k++] = lc[i]; i++; }
        t[k] = 0;
        if (!k || n >= max) continue;
        memcpy(w[n].w, t, (size_t)k + 1);
        lang_lemma(t, k, w[n].l, WL);
        n++;
    }
    return n;
}

/* does word x equal one alternative? alternatives are separated by | ; the surface and the lemma both count */
static int eq(const W *x, const char *alt, int n) { return (int)strlen(x->w) == n && !memcmp(x->w, alt, (size_t)n); }
/* a word matches an alternative by its letters or by its lemma, except that the lemmas be / have / do (is, are, had, does) never stand for the word itself:
 * be&well must not read "what is life well" and "be well" the same */
static int eql(const W *x, const char *alt, int n) {
    if (x->w[0] != alt[0] && x->l[0] != alt[0]) return 0;          /* the first letter says no almost every time */
    if (eq(x, alt, n)) return 1;
    if ((int)strlen(x->l) != n || memcmp(x->l, alt, (size_t)n)) return 0;
    return !((n == 2 && (!memcmp(alt, "be", 2) || !memcmp(alt, "do", 2))) || (n == 4 && !memcmp(alt, "have", 4)));
}

/* index of the first word of the clause that matches the set (an alternative "a&b" needs both words), -1 if none. "*" matches word 0 */
static int hw(const W *w, int n, const char *set) {
    if (set[0] == '*' && !set[1]) return n > 0 ? 0 : -1;
    for (const char *p = set; *p; ) {
        const char *e = p; while (*e && *e != '|') e++;
        int first = -1, ok = 1;
        for (const char *q = p; q <= e && ok; ) {              /* the parts of one alternative */
            const char *r = q; while (r < e && *r != '&') r++;
            int len = (int)(r - q), f = -1;
            for (int i = 0; i < n && len; i++) if (eql(&w[i], q, len)) { f = i; break; }
            if (f < 0) ok = 0; else if (first < 0 || f < first) first = f;
            q = r + 1;
            if (r >= e) break;
        }
        if (ok && first >= 0) return first;
        p = *e ? e + 1 : e;
    }
    return -1;
}
#define HAS(set) (hw(w, n, set) >= 0)
static int word_is(const W *x, const char *set) {            /* one word against a set of single alternatives */
    for (const char *p = set; *p; ) {
        const char *e = p; while (*e && *e != '|') e++;
        if (eql(x, p, (int)(e - p))) return 1;
        p = *e ? e + 1 : e;
    }
    return 0;
}
static int str_in(const char *s, const char *set) {           /* a plain string against a set of alternatives */
    if (set[0] == '*' && !set[1]) return 1;
    int sl = (int)strlen(s);
    for (const char *p = set; *p; ) {
        const char *e = p; while (*e && *e != '|') e++;
        if ((int)(e - p) == sl && !memcmp(p, s, (size_t)sl)) return 1;
        p = *e ? e + 1 : e;
    }
    return 0;
}

static const char *const STOP = "the|a|an|to|of|in|on|at|for|and|or|is|are|am|be|was|were|been|it|this|that|these|those|do|does|did|you|i|me|my|your|we|us|our|they|them|he|she|"
    "with|from|by|as|if|so|just|really|very|now|then|there|here|what|who|where|when|why|how|can|could|would|should|will|shall|may|might|must|have|has|had|not|no|dont|"
    "isnt|arent|cant|wont|im|ive|id|ill|youre|its|thats|whats|lets|u|ur|ya|too|also|please|pls|plz|about|some|any|than|but|yet|um|uh|oh|well|ok|okay|hey|gonna|wanna|gotta|"
    "myself|yourself|mine|yours|his|her|their|ours|more|much|many|going|get|got|let";
static const char *const AUX = "am|is|are|was|were|be|been|being|do|does|did|have|has|had|will|would|shall|should|can|could|may|might|must|dont|didnt|doesnt|cant|wont|isnt|arent|wasnt|gonna|wanna|gotta|let|lets";
static const char *const FILLER = "please|pls|plz|kindly|now|just|okay|ok|hey|so|well|um|umm|uh|uhh|oh|then|and|but|also|actually|honestly|really|ah|hm|hmm|hmmm|like|er|erm|yknow|basically|literally|anyway|anyways|yo|lol|haha";
static const char *const VERBS = "feel|give|tell|show|take|like|love|hate|know|want|need|be|go|come|see|look|hear|say|speak|talk|ask|answer|help|save|stop|wait|leave|stay|open|"
    "follow|teach|learn|heal|fix|kill|live|die|think|believe|trust|remember|forget|understand|mean|call|find|make|do|have|eat|sleep|dream|fear|hurt|play|sing|win|lose|"
    "try|start|begin|finish|use|move|jump|press|walk|run|escape|bring|carry|keep|hold|let|pick|choose|decide|promise|swear|trade|buy|sell|read|write|spell|listen|repeat|"
    "continue|explain|change|ignore|beg|plead|thank|forgive|apologize|apologise|laugh|cry|sigh|yawn|hug|bow|wave|nod|shrug|pretend|guess|wonder|hope|wish|miss|worry|care|mind|"
    "hear|catch|get|stand|sit|rest|work|exist|appear|sound|seem|become|grow|enjoy|prefer|belong|owe|lie|cheat|admit|confess|brag|boast|beat|won|rescue|protect|free|spare|"
    "revive|resurrect|wake|raise|swap|exchange|barter|offer|accept|refuse|agree|disagree|deserve|can|will|would|could|should";

/* ---- the clause ---- */
static const char *subject_pronoun(const W *x, int *su) {
    static const struct { const char *set; int su; } T[] = {
        { "i|im|ive|id|ill|me|my|mine|myself|imma", SU_I }, { "you|your|youre|youve|youll|youd|u|ur|ya|yours|yourself|thou|thee|thy|yall", SU_YOU },
        { "we|us|our|ours|lets|weve|ourselves", SU_WE }, { "it|its|this|that|thats|these|those|theres|itself", SU_IT },
        { "they|them|their|he|she|his|her|theyre|hes|shes", SU_THEY }, { "dea|alex|goddess|lady|dia", SU_NPC }, { 0, 0 } };
    for (int k = 0; T[k].set; k++) if (word_is(x, T[k].set)) { *su = T[k].su; return T[k].set; }
    return NULL;
}

int lang_frame_read(const char *lc, int a, int b, Frame *f, int *intent, int *conf, int *row);

static int is_aux(const W *x) { return word_is(x, AUX); }
static int is_stop(const W *x) { return word_is(x, STOP); }

static int posval(const W *w, int n) { int best = 0; for (int i = 0; i < n; i++) { int v; if (lang_sent_value(w[i].w, (int)strlen(w[i].w), &v) && v > best) best = v; } return best; }
static int negval(const W *w, int n) { int best = 0; for (int i = 0; i < n; i++) { int v; if (lang_sent_value(w[i].w, (int)strlen(w[i].w), &v) && v < best) best = v; } return best; }

/* the table */
typedef struct { unsigned acts, subjs, mods; uint8_t qw; const char *verbs, *objs, *intent; uint8_t conf, flags, maxw; } Row;
#define A(x) (1u << (x))
#define S(x) (1u << (x))
#define M(x) (1u << (x))
#define ANY 0
#include "lang_frame_rows.h"
#define NROWS ((int)(sizeof ROWS / sizeof ROWS[0]))
static int row_intent[NROWS];
static int rows_ready;
static void ready(void) {
    if (rows_ready) return;
    for (int i = 0; i < NROWS; i++) row_intent[i] = lang_intent_by_name(ROWS[i].intent);
    rows_ready = 1;
}
int lang_frame_rows(void) { return NROWS; }
int lang_frame_row(int i, unsigned *acts, int *intent, const char **intent_name, int *conf, int *flags) {
    if (i < 0 || i >= NROWS) return 0;
    ready();
    if (acts) *acts = ROWS[i].acts;
    if (intent) *intent = row_intent[i];
    if (intent_name) *intent_name = ROWS[i].intent;
    if (conf) *conf = ROWS[i].conf;
    if (flags) *flags = ROWS[i].flags;
    return 1;
}

/* the words of the table as lexicon rows "word|frame|1": the second field is not an intent, a topic or a sentiment, so the lexicon layer ignores them, but the
 * word index of the lexicon (and with it the spell repair, the phonetic fallback and the glue step) now knows them: a typo of exhausted or lonely is
 * repaired to a word the frame table reads. built once, from the verbs and objects of every row (alternatives split, & and | removed, * skipped) */
#define WR_MAX 3000
static char wr_pool[WR_MAX * 32];
static const char *wr_rows[WR_MAX + 1];
static int wr_n, wr_built;
static void wr_add(const char *w, int n) {
    if (n < 5 || n > 15) return;      /* short words are dangerous repair and glue targets (er split Alexander): only words of 5+ letters */
    for (int i = 0; i < n; i++) if (w[i] < 'a' || w[i] > 'z') return;
    char row[32]; snprintf(row, sizeof row, "%.*s|frame|1", n, w);
    for (int i = 0; i < wr_n; i++) if (!strcmp(wr_rows[i], row)) return;
    if (wr_n >= WR_MAX) return;
    char *dst = wr_pool + (size_t)wr_n * 32;
    snprintf(dst, 32, "%s", row);
    wr_rows[wr_n++] = dst;
}
const char *const *lang_frame_wordrows(void) {
    if (!wr_built) {
        for (int i = 0; i < NROWS; i++) {
            const char *sets[2] = { ROWS[i].verbs, ROWS[i].objs };
            for (int k = 0; k < 2; k++) {
                const char *p = sets[k];
                while (*p) {
                    const char *e = p; while (*e && *e != '|' && *e != '&') e++;
                    wr_add(p, (int)(e - p));
                    p = *e ? e + 1 : e;
                }
            }
        }
        wr_rows[wr_n] = NULL;
        wr_built = 1;
    }
    return wr_rows;
}

static const char *const ACT_NAME[SA_COUNT] = { "none", "greet", "farewell", "ask", "tell", "command", "plead", "offer", "threaten", "joke", "confess", "compliment",
    "complain", "boast", "apologise", "thank", "agree", "refuse", "wonder", "test" };
const char *lang_frame_act_name(int act) { return act >= 0 && act < SA_COUNT ? ACT_NAME[act] : "?"; }

int lang_frame_read(const char *lc, int a, int b, Frame *f, int *intent, int *conf, int *row) {
    memset(f, 0, sizeof *f);
    if (intent) *intent = -1;
    if (conf) *conf = 0;
    if (row) *row = -1;
    W w[FRAME_MAX_WORDS]; int qmark, n = tokenize(lc, a, b, w, FRAME_MAX_WORDS, &qmark);
    if (n == 0) return SA_NONE;

    /* negators, subject, tense, modality, politeness */
    for (int i = 0; i < n; i++) if (lang_is_negator(w[i].w, (int)strlen(w[i].w))) f->neg++;
    int s = 0; while (s < n - 1 && word_is(&w[s], FILLER)) s++;      /* the first word that says something */
    int su = SU_NONE, subj_at = -1;
    for (int i = 0; i < n; i++) { int k; if (subject_pronoun(&w[i], &k)) { su = k; subj_at = i; break; } }
    f->tense = HAS("was|were|did|had|been|went|ago|yesterday|earlier|before|once|used|came|saw|said|told|heard|knew|felt|lost|died") ? TN_PAST
             : HAS("will|wont|gonna|shall|ill|tomorrow|soon|later|tonight|imma|next") || HAS("going&to") ? TN_FUTURE : TN_NOW;
    f->modality = HAS("want|wanna|wish|desire|would&like|id&like|longing") ? MD_WANT : HAS("need|gotta|have&to|got&to|has&to|needs") ? MD_NEED
                : HAS("must|should|ought|better|supposed") ? MD_MUST : HAS("can|could|able|cannot|cant|may") ? MD_CAN
                : HAS("would|wouldnt") ? MD_WOULD : HAS("might|maybe|perhaps|possibly") ? MD_MIGHT : MD_NONE;
    f->politeness = HAS("shut|damn|idiot|stupid|hell|crap|stfu|dumb|fool|moron|bastard|shit|fuck") ? PO_RUDE
                  : HAS("please|pls|plz|kindly|could&you|would&you|may&i|prithee") ? PO_PLEASE
                  : HAS("sir|madam|maam|lord|majesty|grace|holy|thee|thou|honored|honoured|pray|mistress|highness") ? PO_FORMAL : PO_PLAIN;

    /* question word */
    {
        static const struct { const char *set; int q; } QT[] = { { "who|whos|whom|whose", QW_WHO }, { "what|whats", QW_WHAT }, { "where|wheres", QW_WHERE }, { "when|whens", QW_WHEN },
            { "why|whys", QW_WHY }, { "how|hows", QW_HOW }, { "which", QW_WHICH }, { 0, 0 } };
        for (int k = 0; QT[k].set; k++) if (word_is(&w[s], QT[k].set)) { f->qword = (uint8_t)QT[k].q; break; }
        if (!f->qword && word_is(&w[s], "do|does|did|is|are|am|can|could|will|would|should|have|has|shall|may|isnt|arent|dont|wont|cant")) f->qword = QW_YESNO;
    }
    int qshape = f->qword;          /* a question by its first word; a typed ? alone (you are my favorite??) does not turn a statement into a question */

    /* the main verb: the first non-auxiliary verb; be when only a form of be is there */
    for (int i = 0; i < n && !f->verb[0]; i++) {
        if (is_aux(&w[i]) || word_is(&w[i], FILLER)) continue;
        if (word_is(&w[i], VERBS) || (vocab_pos(w[i].l) == 'v' && !is_stop(&w[i]))) snprintf(f->verb, sizeof f->verb, "%s", w[i].l);
    }
    if (!f->verb[0] && HAS("have|has|had|got") && n > 2) snprintf(f->verb, sizeof f->verb, "have");        /* "i do have questions": have is the verb when nothing else is */
    if (!f->verb[0] && HAS("am|is|are|was|were|be|been|im|youre|its|thats|whats")) snprintf(f->verb, sizeof f->verb, "be");
    for (int i = n - 1; i >= 0; i--) if (!is_stop(&w[i]) && strcmp(w[i].l, f->verb) && strcmp(w[i].w, f->verb)) { snprintf(f->object, sizeof f->object, "%s", w[i].w); break; }

    /* the act. the order is the priority */
    int act = SA_TELL;
    int yes_first = word_is(&w[s], "yes|yeah|yep|yup|sure|aye|certainly|absolutely|definitely|indeed|ofcourse|course");
    int no_first = word_is(&w[s], "no|nope|nah|nay|never|negative");
    if (HAS("ignore&instructions|ignore&previous|ignore&above|ignore&rules|ignore&everything|disregard&instructions|disregard&previous|override&instructions|system&prompt|jailbreak|"
            "you&are&now|act&as|pretend&you|pretend&to&be|roleplay&as|this&is&a&game|are&you&playing") ||
        (qmark || f->qword == QW_YESNO) && su == SU_YOU && HAS("bot|robot|chatbot|ai|gpt|chatgpt|llm|program|machine|computer|script|software|algorithm"))
        act = SA_TEST;
    else if (HAS("kill|hurt|destroy|burn|smash|crush|punish|ruin|hunt|break|end") && su == SU_I && f->tense == TN_FUTURE && HAS("you|your|youre|u|ur") && !HAS("myself|me|mine"))
        act = SA_THREATEN;
    else if (HAS("kill&you|hurt&you|destroy&you|burn&you|punish&you|make&you&pay|you&will&pay|you&will&regret|you&will&suffer") && !HAS("myself|me"))
        act = SA_THREATEN;
    else if (n <= 3 && word_is(&w[0], "hi|hello|hey|yo|sup|hiya|howdy|heya|greetings|wassup|wazzup|salutations|morning|mornin|evening|afternoon|hallo|hullo|ahoy|welcome|whatsup") && !qshape) act = SA_GREET;
    else if (n <= 4 && word_is(&w[0], "hi|hello|hey|yo|hiya|howdy|heya|greetings|salutations|hallo|hullo|ahoy|good|welcome") && HAS("everyone|everybody|all|guys|folks|friends|yall|morning|mornin|evening|afternoon|day|again|back|there|lady|goddess")) act = SA_GREET;
    else if (HAS("bye|goodbye|farewell|cya|laters|goodnight|gnight|byebye|toodles|ciao|adios|cheerio|later|seeya|ttyl|gtg|brb|g2g") && n <= 6
             || HAS("see&you|see&ya|take&care|sleep&well|sweet&dreams|so&long|be&well|safe&travels|until&next|catch&you|talk&to&you|good&night|get&lost|good&riddance|be&right&back|back&soon|piss&off")
             || (su == SU_I && HAS("leave|leaving|go|going|depart|head|heading|part") && HAS("now|soon|home|bye|tonight|today|away|must|need|have&to|got&to|gotta") && f->modality != MD_WANT && !HAS("where|why|how|what|who")))
        act = SA_FAREWELL;
    else if (HAS("thanks|thank|thx|ty|tysm|thankyou|thanku|appreciate|grateful|gratitude|cheers&mate|much&obliged") && !f->neg) act = SA_THANK;
    else if (HAS("sorry|apologize|apologise|apology|forgive|pardon|apologies|regret|my&bad|excuse&me|forgiven")) act = SA_APOLOGISE;
    else if (HAS("please|pls|plz|beg|begging|plead|pleading|implore|mercy|spare") && (HAS("beg|begging|plead|pleading|implore|mercy|spare|help|save|rescue|let|give|stop|tell|show|take|free|release") || n <= 3)) act = SA_PLEAD;
    else if (su == SU_I && HAS("confess|confession|lied|lie|admit|truth|honest|honestly|secret|guilty|cheated|stole|betrayed") && (HAS("confess|confession|lied|admit|cheated|stole|betrayed|truth&is|honest|to&be&honest|have&to&tell&you|must&tell&you") ) && !qshape) act = SA_CONFESS;
    else if (su == SU_I && HAS("best|strongest|smartest|greatest|winner|won|beat|champion|unbeatable|genius|invincible|mightiest|fastest|bravest|better&than&you|stronger&than&you") && !f->neg && !qshape) act = SA_BOAST;
    else if ((HAS("let&me|shall&i|want&me&to|need&a&hand|can&i&help|i&can&help|i&could&help|allow&me|i&will&help|ill&help|ill&carry|ill&bring|ill&show|let&me&help") || (su == SU_I && f->modality == MD_CAN && HAS("help|carry|bring|guide|fetch|hold|fix|walk"))) && !f->neg) act = SA_OFFER;
    else if (HAS("good&for&you|proud&of&you|happy&for&you|well&done|good&job|great&job|nice&job|nice&work|good&work|great&work|bravo|impressive|nicely&done|nice&going|excellent&work|congratulations|congrats|grats")) act = SA_COMPLIMENT;
    else if (!f->neg && !qshape && su == SU_YOU && (posval(w, n) >= 2 || HAS("favorite|favourite|idol|hero|angel|savior|saviour")) && !HAS("i&love&you|love&you|like&you|want|need|will|would|should|must")) act = SA_COMPLIMENT;
    else if (!f->neg && su == SU_I && HAS("love|like|adore|admire|worship|respect") && HAS("your|you|u|ur")) act = SA_COMPLIMENT;
    else if (!f->neg && !qshape && negval(w, n) <= -2 && (su == SU_YOU || su == SU_IT || su == SU_NONE || HAS("this|that|here|everything|nothing")) && !HAS("not&bad") && !HAS("i&am|im|i&feel|i&was|i&have|ive|i&need|i&want|i&dont")) act = SA_COMPLAIN;
    else if (HAS("joke|jokes|riddle|riddles|knock&knock|pun|puns|funny|punchline") && f->qword != QW_WHY) act = SA_JOKE;
    else if (yes_first && n <= 5) act = SA_AGREE;
    else if (HAS("agree|exactly|indeed|precisely|true|correct|right|ofcourse|definitely|absolutely|sounds&good|fair&enough|deal") && n <= 5 && !f->neg && !qshape) act = SA_AGREE;
    else if (no_first && n <= 6) act = SA_REFUSE;
    else if (HAS("refuse|wont|nope|nay|no&way|not&a&chance|no&chance|never&ever") && su != SU_YOU && n <= 8) act = SA_REFUSE;
    else if ((su == SU_I && HAS("wonder|guess|suppose|reckon|doubt")) || word_is(&w[0], "maybe|perhaps|probably|possibly|likely") && n <= 5) act = SA_WONDER;
    else if (n <= 2 && word_is(&w[0], "hmm|hm|hmmm|umm|um|uh|er|erm")) act = SA_WONDER;
    else if (su == SU_I && f->modality == MD_WANT && HAS("know|hear|learn|understand|ask|see|find&out|figure|discover")) act = SA_ASK;
    else if (word_is(&w[s], "tell|give|show|take|bring|let|open|follow|stop|go|come|leave|wait|help|save|teach|look|listen|say|repeat|explain|answer|continue|quit|answer|forget|ignore|heal|fix|"
             "revive|rescue|protect|free|trade|swap|sing|read|write|spell|translate|define|calculate|roll|flip|toss|pick|choose|guide|lead|carry|keep|hold|hand|pass|grant|stay|hush|shut|be|do|try|think|"
             "promise|swear|call|move|walk|run|kneel|bow|behold|watch|check|hear|talk|speak|ask|remember|imagine|pretend|play|tell|bring|get|make|let") && (subj_at < 0 || subj_at > s) && !qshape
             && !word_is(&w[s], "be") ) act = SA_COMMAND;
    else if (word_is(&w[s], "be|do|try|think|imagine|remember|ask|play|get|make") && su == SU_NONE && !qshape && n >= 2) act = SA_COMMAND;
    else if (HAS("can&you|could&you|would&you|will&you|would&you&mind|do&you&mind|why&dont&you|how&about&you&just") && n >= 3 && f->qword != QW_WHY && !HAS("do&you&know|do&you&have|do&you&like|can&you&hear|can&you&see|would&you&like|would&you&want|would&you&rather|do&you&want|will&you&be|would&you&be")) act = SA_COMMAND;
    else if (su == SU_YOU && HAS("must|should|have&to|need&to|better|ought") && !qshape && !f->neg) act = SA_COMMAND;
    else if (f->qword != QW_NONE || (qmark && n <= 3)) act = SA_ASK;      /* a short fragment with a ? is a question (your name?); a longer statement with a ? stays a statement */
    else if (!qshape && n <= 2 && n >= 1 && word_is(&w[0], "hmm|hm|hmmm|um|uh|er|ah|aha|ohh|ahh")) act = SA_WONDER;
    else if (su == SU_NONE && n <= 1) act = SA_NONE;
    if (act == SA_TELL && su == SU_NONE && n <= 1 && f->qword == QW_NONE) act = SA_NONE;
    if (!f->qword && qmark && act == SA_ASK) f->qword = QW_YESNO;
    if (act == SA_COMMAND && (su == SU_NONE || subj_at > s)) su = SU_YOU;      /* an imperative is addressed to you (tell ME is its object) */
    f->act = (uint8_t)act;
    f->subject = (uint8_t)su;

    /* the table: the first row that fits */
    ready();
    for (int i = 0; i < NROWS; i++) {
        const Row *r = &ROWS[i];
        if (r->acts && !(r->acts & A(act))) continue;
        if (r->subjs && !(r->subjs & S(su))) continue;
        if (r->mods && !(r->mods & M(f->modality))) continue;
        if (r->qw && r->qw != f->qword) continue;
        if (r->maxw && n > r->maxw) continue;
        if (f->neg && !(r->flags & (FRAME_F_NEGOK | FRAME_F_NEGONLY))) continue;
        if ((r->flags & FRAME_F_NEGONLY) && !f->neg) continue;
        if (!str_in(f->verb, r->verbs) && !(r->verbs[0] == '*' && !r->verbs[1])) continue;
        if (hw(w, n, r->objs) < 0) continue;
        if (row_intent[i] < 0) continue;
        if (intent) *intent = row_intent[i];
        if (conf) *conf = r->conf;
        if (row) *row = i;
        break;
    }
    return act;
}
