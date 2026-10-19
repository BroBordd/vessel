/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the ten presets, see lang_persona.h. Persona is { name, style, mood, warmth, toxic, patience, trust fear energy amuse grief, lean, quirk }.
 * the numbers of the goddess and the street friend are Dea's and Alex's (story.c) on purpose. */
#include "lang_persona.h"
#include <string.h>

#define L(...) LEAN_ALL(__VA_ARGS__)
#define LEAN_ALL(...) lean_of((int[]){ __VA_ARGS__, -1 })
static uint32_t lean_of(const int *t) { uint32_t m = 0; for (; *t >= 0; t++) m |= LEAN(*t); return m; }

static const char *const PRESET_NAME[PERSONA_COUNT] = {
    "goddess", "street_friend", "old_keeper", "lost_child", "clerk_guard", "beast", "machine", "whisper_ghost", "poet", "trickster" };

/* built at first use (a lean is an or of shifts: not a constant expression through the helper above) */
static Persona P[PERSONA_COUNT];
static int built;
static void build(void) {
    if (built) return;
    built = 1;
    #define SET(id, nm, sty, mo, wa, to, pa, leanm, q) do { Persona *p_ = &P[id]; memset(p_, 0, sizeof *p_); p_->name = nm; p_->style = sty; p_->mood = mo; \
        p_->warmth = wa; p_->toxic = to; p_->patience = pa; p_->lean = leanm; p_->quirk = (uint8_t)(q); } while (0)
    SET(PERSONA_GODDESS,       "Goddess", STYLE_DIVINE,                 0,  30, 65,  8, L(TONE_SMUG, TONE_COLD, TONE_SARCASTIC, TONE_BORED),          Q_LONG);
    SET(PERSONA_STREET_FRIEND, "Friend",  STYLE_STREET,                10,  70,  5, 12, L(TONE_PLAYFUL, TONE_WARM, TONE_WORRIED, TONE_CURIOUS),        Q_ASKS_BACK);
    SET(PERSONA_OLD_KEEPER,    "Keeper",  STYLE_ELDER,                  0,  60, 15, 10, L(TONE_SOLEMN, TONE_WEARY, TONE_TENDER, TONE_WORRIED),         Q_LONG | Q_FORGETFUL);
    SET(PERSONA_LOST_CHILD,    "Child",   STYLE_CHILD,                  5,  65,  0,  6, L(TONE_AFRAID, TONE_CURIOUS, TONE_SHY, TONE_EXCITED),          Q_ASKS_BACK | Q_TERSE);
    SET(PERSONA_CLERK_GUARD,   "Guard",   STYLE_FORMAL,                -5,  40, 35,  5, L(TONE_SUSPICIOUS, TONE_BORED, TONE_COLD, TONE_WEARY),         Q_TERSE);
    SET(PERSONA_BEAST,         "Beast",   STYLE_BEAST,                -10,  25, 30,  4, L(TONE_SUSPICIOUS, TONE_AFRAID, TONE_MANIC, TONE_HOSTILE),     Q_TERSE);
    SET(PERSONA_MACHINE,       "Unit",    STYLE_MACHINE,                0,  50,  0, 20, L(TONE_NEUTRAL, TONE_CURIOUS, TONE_HOLLOW, TONE_BORED),        Q_ASKS_BACK);
    SET(PERSONA_WHISPER_GHOST, "Whisper", STYLE_GHOST,                 -5,  45, 10, 10, L(TONE_HOLLOW, TONE_SOLEMN, TONE_TENDER, TONE_SHY),            Q_FORGETFUL);
    SET(PERSONA_POET,          "Poet",    STYLE_POET,                  10,  60, 10, 14, L(TONE_TENDER, TONE_SOLEMN, TONE_CURIOUS, TONE_SMUG),          Q_RHYMES | Q_LONG);
    SET(PERSONA_TRICKSTER,     "Trickster", STYLE_STREET | STYLE_POET,   5,  50, 40,  9, L(TONE_PLAYFUL, TONE_SARCASTIC, TONE_MANIC, TONE_SMUG),       Q_LIES | Q_ASKS_BACK);
    #undef SET
    /* chunk 41: what each steers the talk back to when it changes the subject (ch.<hook> lines). only two presets so far: the others never steer */
    P[PERSONA_GODDESS].hooks[0] = "mission"; P[PERSONA_GODDESS].hooks[1] = "obey";
    P[PERSONA_STREET_FRIEND].hooks[0] = "orb"; P[PERSONA_STREET_FRIEND].hooks[1] = "grass";
}

const Persona *persona_preset(int id) { if (id < 0 || id >= PERSONA_COUNT) return NULL; build(); return &P[id]; }
const char *persona_preset_name(int id) { return id >= 0 && id < PERSONA_COUNT ? PRESET_NAME[id] : NULL; }
const char *persona_quirk_name(int bit) {
    switch (bit) {
    case Q_ASKS_BACK: return "asks_back"; case Q_SWEARS: return "swears"; case Q_RHYMES: return "rhymes"; case Q_FORGETFUL: return "forgetful";
    case Q_LIES: return "lies"; case Q_LONG: return "long"; case Q_TERSE: return "terse"; default: return "?";
    }
}
int persona_init(Persona *out, int id, const char *name) {
    const Persona *src = persona_preset(id);
    if (!src || !out) return 0;
    *out = *src;
    if (name && *name) out->name = name;
    out->hold = 0; out->held = 0; out->quiet = 0; out->flat = 0; out->sour = 0;      /* the engine's own state starts clean */
    return 1;
}
