/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the Phase 3 gate (docs/convo_plan.md, chunk 44): scripted transcripts. no SDL:
 *   cc -O1 -Igame -o build/transcript tools/transcript.c game/lang*.c game/lines_*.c -lm && build/transcript
 *   build/transcript            run the gate, print two chats for a human to read
 *   build/transcript --all      print every chat      build/transcript --quiet   print nothing but the numbers and failures
 *
 * every chat is played twice: in game mode with the npc's own patience (Dea 8 turns, Alex 12: boredom ends most chats before the script's goodbye) and in long
 * mode (patience 60) so the whole script is played and the bye / dots / anger endings are reached.
 * 40 scripts (10 friendly, 10 hostile, 10 shy, 10 lost player; 16-26 player lines each: a greeting, a name, topic lines, typos, a CAPS line,
 * a line typed twice, follow-ups, then a goodbye, a long dots silence, or insults until the npc has had enough) are played against five
 * npcs (Dea, Alex as story.c defines them, and the old keeper, the lost child and the machine presets) = 200 chats, fixed seed.
 * a chat is driven the way convo.c's free chat drives the engine: lang_parse, lang_reply, lang_ctx_note, the patience counter, the
 * sorry stage after a hard insult, the end by bye, boredom (patience, rep.5, dots) or anger (three refused apologies).
 * checks:
 *   - no empty reply          - no reply over 150 characters (r.selfharm is exempt: it names 988; no script uses it)
 *   - only the font's characters (letters, digits, space and , . ! ? ' -), no open {slot}
 *   - no reply repeated within 40 replies of the same npc (the window runs across that npc's chats: the line memory does too). With today's
 *     ~1,800 lines this cannot hold yet (pools of 12 lines), so the default limit is the measured baseline (DEFAULT_MAX_REPEATS below);
 *     `--strict` (limit 0) is the real rule and chunk 150 runs it. `--max-repeats N` and `--max-repeats-in-chat N` set the limits.
 *   - every chat ends by bye, boredom or anger; none runs past its script
 *   - the mood stays in the npc's lean (or one of the 4 old moods) most of the time; both shares are printed */
#include "lang.h"
#include "lang_data.h"
#include "lang_persona.h"
#include "lang_ctx.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bad;
#define FAIL(...) do { printf("FAIL: " __VA_ARGS__); printf("\n"); bad++; } while (0)

enum { K_FRIENDLY, K_HOSTILE, K_SHY, K_LOST, K_COUNT };
static const char *const KIND[K_COUNT] = { "friendly", "hostile", "shy", "lost" };
#define NSCRIPT 10
#define DEFAULT_MAX_REPEATS 500             /* measured at chunk 44: 378 across chats (both modes); a broken no-repeat memory gives thousands */
#define DEFAULT_MAX_REPEATS_IN_CHAT 70      /* measured at chunk 44: 49 inside one chat */
#define MAXLINES 40
#define LINELEN 96
typedef struct { int n; char t[MAXLINES][LINELEN]; } Script;
static Script SCRIPTS[K_COUNT][NSCRIPT];

/* ---- the lines, by kind -------------------------------------------------------------------------------------------------------- */
typedef struct { const char *const *open, *const *name, *const *body, *const *close; int nopen, nname, nbody, nclose; } Pool;

static const char *const F_OPEN[]  = { "hello", "hi there", "hey", "good morning", "hello again", "greetings", "hiya", "yo", "howdy", "hello friend" };
static const char *const F_NAME[]  = { "my name is Bob", "call me Mia", "im Omar", "my name is zoe", "I am Lena", "my name is Karim", "call me Sam", "my nmae is Nora", "my name is Yusuf", "im Ada" };
static const char *const F_BODY[]  = { "how are you", "you seem nice", "thank you for talking to me", "i like the sky", "tell me about the moon", "what do you like", "do you like music",
    "tell me a joke", "you are very kind", "where are we", "who are you", "what is this place", "i love the grass", "can you help me", "what should i do", "that is interesting",
    "you are smart", "i like talking to you", "do you have a favorite color", "what do you think about stars", "I LOVE THIS PLACE", "hw r u doin", "wat is ur name",
    "tel me abuot the sun", "do you ever get lonely", "thanks a lot", "that was funny", "you are wise", "i am happy today", "what is the shrine" };
static const char *const F_CLOSE[] = { "bye", "goodbye", "see you later", "bye bye", "gotta go bye", "thanks bye", "farewell", "good night", "byeee", "see ya" };

static const char *const H_OPEN[]  = { "what do you want", "ugh hello", "hey you", "hi stupid", "hello idiot", "listen", "yo useless", "you again", "hey ugly thing", "oh great you" };
static const char *const H_NAME[]  = { "my name is Rex", "call me Boss", "im Dante", "my name is Vlad", "call me Killer", "im Max", "my name is Zed", "call me Chief", "im Brutus", "my name is Hawk" };
static const char *const H_BODY[]  = { "you are stupid", "shut up", "i hate you", "you are useless", "give me the orb", "stop talking", "you are boring", "idiot", "you are a liar", "nobody likes you",
    "do what i say", "this place sucks", "you are ugly", "LISTEN TO ME", "i dont care about you", "you know nothing", "wat a waste of time", "you r dumb", "go away", "you are pathetic" };
static const char *const H_CLOSE[] = { "whatever bye", "bye idiot", "im leaving", "bye", "forget you", "i am done", "bye you fool", "good riddance bye", "leave me alone", "bye bye" };

static const char *const S_OPEN[]  = { "hi", "um hi", "h-hello", "hello?", "hi...", "um", "hey", "hello sorry", "uh hi", "hi there" };
static const char *const S_NAME[]  = { "im ana", "my name is Ben", "um my name is Eli", "call me Kai", "i am Ivy", "um im Max", "my name is lou", "im Tess", "my name is Joe", "call me Ray" };
static const char *const S_BODY[]  = { "um", "i dont know", "maybe", "sorry", "ok", "i guess", "what do you mean", "is that ok", "i am not sure", "um who are you", "can i ask something",
    "i am nervous", "...", "thanks", "uh yes", "no", "im scared a bit", "is it safe here", "i dont want to bother you", "please dont be mad" };
static const char *const S_CLOSE[] = { "bye", "um bye", "i should go bye", "sorry bye", "ok bye", "bye then", "...", "thanks bye", "...", "um goodbye" };

static const char *const L_OPEN[]  = { "hello?", "is anyone there", "where am i", "hello", "help", "hi where am i", "um hello", "anyone", "hey where is this", "hello who are you" };
static const char *const L_NAME[]  = { "my name is Noor", "im Lee", "call me Jo", "i am Sky", "my name is Omar", "im Pat", "call me Dee", "my name is Ali", "i am Ren", "im Cam" };
static const char *const L_BODY[]  = { "where am i", "what is this place", "who are you", "what do i do", "i am confused", "help me", "how do i get out", "am i dead", "what is happening",
    "i dont understand", "where do i go", "what am i supposed to do", "how long have i been here", "i cant remember", "is this a dream", "who am i", "what is that hole",
    "what is the orb", "what is your mission", "wat shud i do" };
static const char *const L_CLOSE[] = { "bye", "ok thanks bye", "goodbye", "i will go bye", "...", "thank you bye", "...", "see you", "bye bye", "..." };

#define POOL(p) { p##_OPEN, p##_NAME, p##_BODY, p##_CLOSE, (int)(sizeof p##_OPEN / sizeof *p##_OPEN), (int)(sizeof p##_NAME / sizeof *p##_NAME), \
                  (int)(sizeof p##_BODY / sizeof *p##_BODY), (int)(sizeof p##_CLOSE / sizeof *p##_CLOSE) }
static const Pool POOLS[K_COUNT] = { POOL(F), POOL(H), POOL(S), POOL(L) };
static const char *const FOLLOW[] = { "why", "what do you mean", "really", "go on", "tell me more", "like what" };
static const char *const INSULT3[] = { "you are an idiot", "i hate you so much", "you are worthless" };

static void put(Script *s, const char *t) { if (s->n < MAXLINES) snprintf(s->t[s->n++], LINELEN, "%s", t); }

static void typo(const char *in, char *out, int cap) {          /* swap two letters inside the longest word (a fixed rule, no random numbers) */
    snprintf(out, cap, "%s", in);
    int best = -1, bl = 0, n = (int)strlen(out);
    for (int i = 0; i < n;) { int j = i; while (j < n && out[j] != ' ') j++; if (j - i > bl) { bl = j - i; best = i; } i = j + 1; }
    if (bl >= 4) { char c = out[best + 1]; out[best + 1] = out[best + 2]; out[best + 2] = c; }
}
static void shout(const char *in, char *out, int cap) { snprintf(out, cap, "%s", in); for (char *p = out; *p; p++) *p = (char)toupper((unsigned char)*p); }

static void build_scripts(void) {
    for (int k = 0; k < K_COUNT; k++)
        for (int i = 0; i < NSCRIPT; i++) {
            const Pool *P = &POOLS[k]; Script *s = &SCRIPTS[k][i]; s->n = 0;
            put(s, P->open[i % P->nopen]); put(s, P->name[(i * 3 + 1) % P->nname]);
            int nb = 8 + i % 5; char prev[LINELEN] = "", tmp[LINELEN];
            for (int j = 0; j < nb; j++) {
                const char *b = P->body[(i * 7 + j * 11) % P->nbody];
                if (j == 3) { typo(b, tmp, sizeof tmp); put(s, tmp); snprintf(prev, sizeof prev, "%s", tmp); }          /* a typo */
                else if (j == 4 && prev[0]) put(s, prev);                                                                /* the same line again */
                else if (j == 6 && prev[0]) { shout(prev, tmp, sizeof tmp); put(s, tmp); }                              /* CAPS */
                else { put(s, b); snprintf(prev, sizeof prev, "%s", b); }
                if (j % 3 == 2) put(s, FOLLOW[(i + j) % 6]);                                                             /* a follow-up */
            }
            if (k == K_HOSTILE) for (int j = 0; j < 3; j++) put(s, INSULT3[(i + j) % 3]);                               /* hard insults: the sorry stage, then anger */
            const char *c = P->close[i % P->nclose];
            if (!strcmp(c, "...")) for (int j = 0; j < 5; j++) put(s, "...");                                            /* dots: four nudges, then the npc gives up */
            else put(s, c);
        }
}

/* ---- the npcs ------------------------------------------------------------------------------------------------------------------ */
typedef struct { const char *name; Persona p; } Actor;
#define NACT 5
static Actor ACT[NACT];
static void build_actors(void) {
    ACT[0].name = "Dea";  ACT[0].p = (Persona){ "Dea",  STYLE_DIVINE,  0, 30, 65,  8, .lean = LEAN(TONE_SMUG) | LEAN(TONE_COLD) | LEAN(TONE_SARCASTIC) | LEAN(TONE_BORED), .hooks = { "mission", "obey" }, .loves = { "god", "power" } };
    ACT[1].name = "Alex"; ACT[1].p = (Persona){ "Alex", STYLE_STREET, 10, 70,  5, 12, .lean = LEAN(TONE_PLAYFUL) | LEAN(TONE_WARM) | LEAN(TONE_WORRIED) | LEAN(TONE_CURIOUS), .quirk = Q_ASKS_BACK, .hooks = { "grass" }, .loves = { "grass", "flower" } };
    ACT[2].name = "Orrin";  persona_init(&ACT[2].p, PERSONA_OLD_KEEPER, "Orrin");
    ACT[3].name = "Pip";    persona_init(&ACT[3].p, PERSONA_LOST_CHILD, "Pip");
    ACT[4].name = "Unit 7"; persona_init(&ACT[4].p, PERSONA_MACHINE, "Unit 7");
}

/* ---- one chat ------------------------------------------------------------------------------------------------------------------ */
enum { END_NONE, END_BYE, END_BORED, END_ANGRY, END_SILENCE, END_COUNT };
static const char *const END_NAME[END_COUNT] = { "none", "bye", "boredom", "anger", "silence" };

#define WIN 40
static char RING[NACT][WIN][200]; static int RING_CHAT[NACT][WIN]; static int RING_N[NACT];
static struct { char key[40]; int n; } RK[48]; static int RK_N;      /* which keys repeat (both modes) */
static int g_verbose, g_chat_id, g_long, g_notes_in, g_notes_cross;
typedef struct { int chats, turns, replies, ends[END_COUNT], in_lean, tones[TONE_COUNT], tone_n, longest, dup_cross, dup_in, flow_alone; } Tot;
static Tot TOT[NACT];
static int ends_by[NACT][K_COUNT][END_COUNT], turns_by[NACT][K_COUNT];

static int charset_ok(const char *s) {
    for (; *s; s++) if (!(isalnum((unsigned char)*s) || *s == ' ' || *s == ',' || *s == '.' || *s == '!' || *s == '?' || *s == '\'' || *s == '-')) return 0;
    return 1;
}

static void check_reply(int a, const char *who, int kind, int sc, int turn, const char *text, const char *key) {
    int n = (int)strlen(text);
    if (!n) FAIL("%s %s #%d turn %d: empty reply (key %s)", who, KIND[kind], sc + 1, turn, key);
    if (n > 150 && strcmp(key, "r.selfharm")) FAIL("%s %s #%d turn %d: %d characters: %s", who, KIND[kind], sc + 1, turn, n, text);
    if (!charset_ok(text)) FAIL("%s %s #%d turn %d: a character the font lacks: %s", who, KIND[kind], sc + 1, turn, text);
    if (strchr(text, '{') || strchr(text, '}')) FAIL("%s %s #%d turn %d: open slot: %s", who, KIND[kind], sc + 1, turn, text);
    if (n > TOT[a].longest) TOT[a].longest = n;
    int have = RING_N[a] < WIN ? RING_N[a] : WIN;
    for (int i = 0; i < have; i++)
        if (!strcmp(RING[a][i], text)) {
            int same = RING_CHAT[a][i] == g_chat_id;
            if (same) TOT[a].dup_in++;
            TOT[a].dup_cross++;
            { int r = 0; while (r < RK_N && strcmp(RK[r].key, key)) r++; if (r == RK_N && RK_N < 48) { snprintf(RK[RK_N].key, sizeof RK[0].key, "%s", key); RK[RK_N++].n = 0; } if (r < RK_N) RK[r].n++; }
            if (same ? g_notes_in < 12 : g_notes_cross < 4) { if (same) g_notes_in++; else g_notes_cross++; printf("  note: %s %s #%d turn %d: repeated within %d replies%s (key %s): %s\n", who, KIND[kind], sc + 1, turn, WIN, same ? " IN THE SAME CHAT" : "", key, text); }
            break;
        }
    snprintf(RING[a][RING_N[a] % WIN], sizeof RING[a][0], "%s", text); RING_CHAT[a][RING_N[a] % WIN] = g_chat_id; RING_N[a]++;
}

static void play(int a, int kind, int sc) {
    Persona ps = ACT[a].p; const char *who = ACT[a].name;
    const Script *S = &SCRIPTS[kind][sc];
    if (g_long) ps.patience = 60;                                              /* long mode: the npc is patient, so the whole script is played */
    lang_add_charisma(50 - lang_charisma()); lang_ctx_reset(); g_chat_id++;
    int stage_sorry = 0, demands = 0, turns = 0, end = END_NONE;
    if (g_verbose) printf("\n== %s / %s #%d ==\n", who, KIND[kind], sc + 1);
    char open[200]; if (lang_line(&ps, "open", open, sizeof open) && g_verbose) printf("  %s: %s\n", who, open);
    int i;
    for (i = 0; i < S->n && end == END_NONE; i++) {
        const char *text = S->t[i];
        Heard h; memset(&h, 0, sizeof h); lang_parse(text, &h);
        char reply[480] = ""; const char *key = "";
        if (stage_sorry) {                                                      /* the npc demanded an apology: no skipping here */
            lang_apply(&ps, &h);
            if (heard_has(&h, I_SORRY)) { ps.mood += 22; if (ps.mood > 100) ps.mood = 100; stage_sorry = 0; lang_line(&ps, "accept.sorry", reply, sizeof reply); key = "accept.sorry"; }
            else if (++demands >= 3) { lang_line(&ps, "end.angry", reply, sizeof reply); key = "end.angry"; end = END_ANGRY; }
            else { lang_line(&ps, "demand.again", reply, sizeof reply); key = "demand.again"; }
            if (g_verbose) printf("  you: %s\n  %s [sorry stage]: %s\n", text, who, reply);
            check_reply(a, who, kind, sc, i + 1, reply, key);
            TOT[a].replies++; continue;
        }
        lang_reply(&ps, &h, reply, sizeof reply);
        key = lang_reply_key();
        int tone = lang_reply_tone();
        lang_ctx_note(&h, text, key, ps.mood);
        turns++;
        int worn = heard_has(&h, I_REPEAT_PLAYER) || heard_has(&h, I_SPAM);
        if (worn) turns++;
        const char *tag = "";
        if (heard_has(&h, I_SELFHARM)) { tag = ""; }
        else if (!strcmp(key, "end.silence")) end = END_SILENCE;
        else if (heard_has(&h, I_BYE)) end = END_BYE;
        else if (heard_has(&h, I_INSULT) && ps.mood <= -55) {
            char dem[160]; lang_line(&ps, "demand.sorry", dem, sizeof dem);
            if (!lang_append_flow(reply, sizeof reply, dem, 1)) FAIL("demand.sorry not appended");
            if (!strncmp(reply, dem, strlen(dem)) && strlen(reply) == strlen(dem)) TOT[a].flow_alone++;
            stage_sorry = 1; demands = 0; tag = " + sorry stage";
        }
        if (end == END_NONE && (turns >= ps.patience || (worn && h.rep_level >= 5))) {
            char e[200]; lang_line(&ps, "end.bored", e, sizeof e);
            lang_append_flow(reply, sizeof reply, e, 1);
            if (strlen(reply) == strlen(e)) TOT[a].flow_alone++;
            end = END_BORED;
        }
        if (g_verbose) printf("  you: %s\n  %s [%s]%s: %s\n", text, who, tone >= 0 ? lang_tone_name(tone) : "-", tag, reply);
        check_reply(a, who, kind, sc, i + 1, reply, key);
        TOT[a].replies++;
        if (tone >= 0) { TOT[a].tone_n++; TOT[a].tones[tone]++; if ((ps.lean >> tone) & 1u) TOT[a].in_lean++; }
    }
    if (end == END_NONE) FAIL("%s %s #%d: the chat did not end by bye, boredom or anger (%d of %d lines played)", who, KIND[kind], sc + 1, i, S->n);
    if (g_verbose) printf("  [chat over: %s after %d lines]\n", END_NAME[end], i);
    TOT[a].chats++; TOT[a].turns += i; TOT[a].ends[end]++; ends_by[a][kind][end]++; turns_by[a][kind] += i;
}

/* one pass over the 200 chats. returns the number of repeats across chats (inside the 40 reply window) */
static void run_mode(int long_mode, int all, int quiet, int *cross, int *in_chat) {
    memset(TOT, 0, sizeof TOT); memset(ends_by, 0, sizeof ends_by); memset(turns_by, 0, sizeof turns_by); memset(RING_N, 0, sizeof RING_N);
    g_long = long_mode; g_notes_in = g_notes_cross = 0;
    printf("\n######## %s ########\n", long_mode ? "long mode: patience 60, every script is played to its end" : "game mode: the npcs' own patience (Dea 8, Alex 12 ...), boredom cuts most chats short");
    for (int a = 0; a < NACT; a++) {
        lang_seed(44 + (unsigned)a); lang_ctx_facts_load(NULL);
        for (int k = 0; k < K_COUNT; k++)
            for (int s = 0; s < NSCRIPT; s++) {
                /* two chats are printed for a human (long mode): Alex with a friendly player and Dea with a hostile one, the first script of each */
                g_verbose = !quiet && long_mode && (all || (a == 1 && k == K_FRIENDLY && s == 0) || (a == 0 && k == K_HOSTILE && s == 0));
                play(a, k, s);
            }
    }
    int chats = 0, replies = 0, in_lean = 0, in_base = 0, tone_n = 0, flow_alone = 0; *cross = *in_chat = 0;
    printf("\n%-8s %5s %7s %7s | %5s %7s %7s %7s %5s | %s\n", "npc", "chats", "replies", "lines", "bye", "boredom", "anger", "silence", "none", "mood in lean / in lean or old 4 / longest reply");
    for (int a = 0; a < NACT; a++) {
        int *e = TOT[a].ends, lean_or_base = 0;
        for (int t = 0; t < TONE_COUNT; t++) if (t < 4 || ((ACT[a].p.lean >> t) & 1u)) lean_or_base += TOT[a].tones[t];
        printf("%-8s %5d %7d %7d | %5d %7d %7d %7d %5d | %5.1f %% / %5.1f %% / %d\n", ACT[a].name, TOT[a].chats, TOT[a].replies, TOT[a].turns, e[END_BYE], e[END_BORED], e[END_ANGRY], e[END_SILENCE], e[END_NONE],
               TOT[a].tone_n ? 100.0 * TOT[a].in_lean / TOT[a].tone_n : 0.0, TOT[a].tone_n ? 100.0 * lean_or_base / TOT[a].tone_n : 0.0, TOT[a].longest);
        chats += TOT[a].chats; replies += TOT[a].replies; in_lean += TOT[a].in_lean; in_base += lean_or_base; tone_n += TOT[a].tone_n; flow_alone += TOT[a].flow_alone;
        *cross += TOT[a].dup_cross; *in_chat += TOT[a].dup_in;
        if (TOT[a].tone_n && 100.0 * lean_or_base / TOT[a].tone_n < 85.0) FAIL("%s: only %.1f %% of the replies are in the lean or the 4 old moods (want >= 85 %%)", ACT[a].name, 100.0 * lean_or_base / TOT[a].tone_n);
    }
    printf("\nends by kind of player (bye / boredom / anger / silence), average lines played:\n");
    for (int k = 0; k < K_COUNT; k++) {
        int e[END_COUNT] = {0}, t = 0;
        for (int a = 0; a < NACT; a++) { for (int j = 0; j < END_COUNT; j++) e[j] += ends_by[a][k][j]; t += turns_by[a][k]; }
        printf("  %-9s %3d / %3d / %3d / %3d   %.1f lines\n", KIND[k], e[END_BYE], e[END_BORED], e[END_ANGRY], e[END_SILENCE], (double)t / (NACT * NSCRIPT));
    }
    printf("\n%d chats, %d replies (%d with a mood), mood in the lean %.1f %%, in the lean or the 4 old moods %.1f %%, flow line alone %d times,\n"
           "repeats inside the %d reply window: %d across chats, %d of them in the same chat\n",
           chats, replies, tone_n, tone_n ? 100.0 * in_lean / tone_n : 0.0, tone_n ? 100.0 * in_base / tone_n : 0.0, flow_alone, WIN, *cross, *in_chat);
    if (chats != K_COUNT * NSCRIPT * NACT) FAIL("expected %d chats, played %d", K_COUNT * NSCRIPT * NACT, chats);
}

int main(int argc, char **argv) {
    int all = 0, quiet = 0, max_cross = DEFAULT_MAX_REPEATS, max_in = DEFAULT_MAX_REPEATS_IN_CHAT;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--all")) all = 1; else if (!strcmp(argv[i], "--quiet")) quiet = 1;
        else if (!strcmp(argv[i], "--strict")) max_cross = max_in = 0;
        else if (!strcmp(argv[i], "--max-repeats") && i + 1 < argc) max_cross = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--max-repeats-in-chat") && i + 1 < argc) max_in = atoi(argv[++i]);
    }
    build_scripts(); build_actors();
    for (int k = 0; k < K_COUNT; k++)
        for (int i = 0; i < NSCRIPT; i++) {
            int n = SCRIPTS[k][i].n;
            if (n < 12 || n > 26) FAIL("script %s #%d has %d lines (want 12-26)", KIND[k], i + 1, n);
        }
    int c0, i0, c1, i1;
    run_mode(0, all, quiet, &c0, &i0);
    run_mode(1, all, quiet, &c1, &i1);
    int cross = c0 + c1, in = i0 + i1;
    for (int i = 0; i < RK_N; i++) for (int j = i + 1; j < RK_N; j++) if (RK[j].n > RK[i].n) { char kt[40]; int nt = RK[i].n; memcpy(kt, RK[i].key, 40); memcpy(RK[i].key, RK[j].key, 40); RK[i].n = RK[j].n; memcpy(RK[j].key, kt, 40); RK[j].n = nt; }
    printf("\nkeys that repeat (both modes):"); for (int i = 0; i < RK_N && i < 8; i++) printf(" %s %d%s", RK[i].key, RK[i].n, i + 1 < RK_N && i < 7 ? "," : ""); printf("\n");
    printf("\nrepeat gate (both modes): %d repeats across chats (limit %d), %d in the same chat (limit %d)%s\n", cross, max_cross, in, max_in, max_cross == 0 && max_in == 0 ? "  [strict]" : "");
    if (cross > max_cross) FAIL("%d reply repeats across chats inside %d replies, the limit is %d (the line pools are still small: chunk 150 runs --strict)", cross, WIN, max_cross);
    if (in > max_in) FAIL("%d reply repeats inside ONE chat, the limit is %d", in, max_in);
    printf("%d failures\n", bad);
    return bad ? 1 : 0;
}
