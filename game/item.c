/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "item.h"
#include "font.h"
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* 12 x 12 icons. G outline, L light steel, D dark steel, W light wood, w dark wood, V violet, v dark violet, Y gold, y pale gold glow */
static const char *HAMMER[ITEM_ICON_CELLS] = {
    "..GGGGGGGG..",
    ".GLLLLLLLLG.",
    ".GLLLLLLLDG.",
    ".GDDDDDDDDG.",
    "..GGGWWGGG..",
    "....GWWG....",
    "....GWwG....",
    "....GWwG....",
    "....GWwG....",
    "....GWwG....",
    "....GWwG....",
    ".....GG.....",
};
/* the orb (chunk 39): a violet ball with a gold glow in the middle, like draw_orb_ball in brainwin.c */
static const char *ORB[ITEM_ICON_CELLS] = {
    "....GGGG....",
    "..GGvVVvGG..",
    ".GvVVVVVVvG.",
    ".GVVYyyYVVG.",
    "GvVYyyyyYVvG",
    "GVVYyyyyYVVG",
    "GVVVYyyYVVVG",
    "GvVVVYYVVVvG",
    ".GVVVVVVVVG.",
    ".GvvVVVVvvG.",
    "..GGvvvvGG..",
    "....GGGG....",
};
/* the note (story chunk 5): a sheet of paper with a few lines written on it in blood. P paper, R blood */
static const char *PAPER[ITEM_ICON_CELLS] = {
    "..GGGGGGGG..",
    ".GPPPPPPPPG.",
    ".GPRRPPRPPG.",
    ".GPPPPPPPPG.",
    ".GPRRRPRRPG.",
    ".GPPPPPPPPG.",
    ".GPRRPRRRPG.",
    ".GPPPPPPPPG.",
    ".GPRPRRPPPG.",
    ".GPPPPPPPPG.",
    ".GGGGGGGGGG.",
    "............",
};
/* the knife (story chunk 6): a steel blade with a wooden handle, the tip red */
static const char *KNIFE[ITEM_ICON_CELLS] = {
    "............",
    "..........GG",
    ".........GLG",
    "........GLLG",
    ".......GLLRG",
    "......GLDG..",
    ".....GLDG...",
    "..GGGLDG....",
    ".GwwGDG.....",
    ".GwwG.......",
    "..GG........",
    "............",
};
/* the magnifying glass (chunk 32). B glass, b glint on the glass. the rim is light steel (L), not the dark outline, so it reads on the dark button */
static const char *SEARCH[ITEM_ICON_CELLS] = {
    "...LLLLL....",
    "..LBBBBBL...",
    ".LBbbBBBBL..",
    ".LBbBBBBBL..",
    ".LBBBBBBBL..",
    ".LBBBBBBBL..",
    "..LBBBBBLL..",
    "...LLLLLWL..",
    ".......LLWL.",
    ".........LWL",
    "..........LL",
    "............",
};
static const char *HAND[ITEM_ICON_CELLS] = {
    "...G.G.G....",
    "..GSGSGSG.G.",
    "..GSGSGSGSG.",
    "..GSGSGSGSG.",
    ".GGSSSSSSSG.",
    "GSSSSSSSSSG.",
    "GSSSSSSSSSG.",
    ".GSSSSSSSSG.",
    "..GSSSSSSG..",
    "..GSSSSSsG..",
    "...GSSSssG..",
    "....GGGGG...",
};
/* chunk 70: the bag (the collapsed inventory button). an old leather pouch tied with a gold cord. not an item */
static const char *BAG[ITEM_ICON_CELLS] = {
    "....GYYG....",
    "....GyYG....",
    ".....GG.....",
    "...GWWWWG...",
    "..GWWWWWWG..",
    ".GWWWWWWWWG.",
    "GWWWWYYWWWwG",
    "GWWWWYyWWWwG",
    "GWWWWWWWWWwG",
    "GWWWWWWWWwwG",
    ".GwWWWWWwwG.",
    "..GGGGGGGG..",
};
/* the small rock (story chunk 8): a grey stone, lighter on the top left. L light, D dark */
static const char *ROCK[ITEM_ICON_CELLS] = {
    "............",
    "............",
    "....GGGG....",
    "..GGLLLLGG..",
    ".GLLLLLDDDG.",
    ".GLLLLDDDDG.",
    "GLLLDDDDDDDG",
    "GLLDDDDDDDDG",
    ".GDDDDDDDDG.",
    "..GGDDDDGG..",
    "....GGGG....",
    "............",
};

/* story chunk 9: the parts. S skin, s dark skin, R blood, N pink, n dark pink, M maroon, P bone, G outline */
static const char *ARM[ITEM_ICON_CELLS] = {
    "............", "........GGG.", ".......GSSsG", "......GSSsG.", ".....GSSsG..", "....GSSsG...",
    "...GSSsG....", "..GSSsG.....", ".GRRRG......", ".GRRRG......", "..GGG.......", "............" };
static const char *LEG[ITEM_ICON_CELLS] = {
    "...GGGG.....", "...GRRG.....", "...GSSsG....", "...GSSsG....", "...GSSsG....", "...GSSsG....",
    "...GSSsG....", "...GSSsG....", "...GSSsGGG..", "...GSSSSSsG.", "....GGGGGGG.", "............" };
static const char *HEAD[ITEM_ICON_CELLS] = {
    "....GGGG....", "..GGSSSSGG..", ".GSSSSSSSSG.", ".GSSSSSSSSG.", ".GSGGSSGGSG.", ".GSSSSSSSSG.",
    ".GSSSssSSSG.", ".GSSSSSSSSG.", "..GSRRRRSG..", "..GGRRRRGG..", "....GGGG....", "............" };
static const char *RIBS[ITEM_ICON_CELLS] = {
    "............", "..GPGGGGPG..", ".GPPGPPGPPG.", ".GPGPPPPGPG.", ".GPGPPPPGPG.", ".GPPGPPGPPG.",
    "..GPPGGPPG..", "..GPGPPGPG..", "...GPPPPG...", "....GGGG....", "............", "............" };
static const char *HEART[ITEM_ICON_CELLS] = {
    "............", "..GGG.GGG...", ".GRRRGRRRG..", "GRRNRRRRRRG.", "GRRRRRRRRRG.", ".GRRRRRRRG..",
    "..GRRRRRG...", "...GRRRG....", "....GRG.....", ".....G......", "............", "............" };
static const char *LIVER[ITEM_ICON_CELLS] = {
    "............", "..GGGGGG....", ".GMMMMMMGG..", "GMMMnnMMMMG.", "GMMMMMMMMMMG", "GMMMMMMMMMMG",
    ".GMMMMMMMMG.", "..GGMMMMMG..", "....GGGGG...", "............", "............", "............" };
static const char *INTEST[ITEM_ICON_CELLS] = {
    "............", "..GGGGGGGG..", ".GNNNNNNNNG.", ".GNGGGGGGNG.", ".GNG....GNG.", ".GNNNNNNNNG.",
    "..GGGGGGGNG.", ".GNNNNNNNNG.", ".GNGGGGGGG..", ".GNNNNNNNG..", "..GGGGGGGG..", "............" };
static const char *SPINE[ITEM_ICON_CELLS] = {
    "....GGGG....", "...GPPPPG...", "....GPPG....", "...GPPPPG...", "....GPPG....", "...GPPPPG...",
    "....GPPG....", "...GPPPPG...", "....GPPG....", "...GPPPPG...", "....GGGG....", "............" };
static const char *FINGERS[ITEM_ICON_CELLS] = {
    "............", "..G.G.G.G...", ".GSGSGSGSG..", ".GSGSGSGSG..", ".GSGSGSGSG..", ".GSSSSSSSSG.",
    ".GSSSSSSSSG.", "..GRRRRRG...", "..GGGGGGG...", "............", "............", "............" };
static const char *EYES[ITEM_ICON_CELLS] = {
    "............", "............", ".GGGG..GGGG.", "GbbbbGGbbbbG", "GbBBbGGbBBbG", "GbBGbGGbBGbG",
    "GbbbbGGbbbbG", ".GGGG..GGGG.", "............", "............", "............", "............" };
static const char *TEETH[ITEM_ICON_CELLS] = {
    "............", "............", "GGGGGGGGGGGG", "GPPGPPGPPGPG", "GPPGPPGPPGPG", "GPPGPPGPPGPG",
    ".GGGGGGGGGG.", "............", "............", "............", "............", "............" };
static const char *SKULL[ITEM_ICON_CELLS] = {
    "....GGGG....", "..GGPPPPGG..", ".GPPPPPPPPG.", ".GPPPPPPPPG.", ".GPGGPPGGPG.", ".GPGGPPGGPG.",
    ".GPPPPGPPPG.", "..GPPPPPPG..", "..GPGPGPGPG.", "...GGGGGGG..", "............", "............" };
static const char *BONE[ITEM_ICON_CELLS] = {
    "............", "........GG.G", ".......GPGPG", "......GPPPG.", ".....GPPG...", "....GPPG....",
    "...GPPG.....", "..GPPG......", ".GPGPG......", ".GP.GG......", "..GG........", "............" };

/* story chunk 10: what the recipes make */
static const char *TIED_ROCK[ITEM_ICON_CELLS] = {
    "....NNNN....", "...N....N...", "...N....N...", "....GGGG....", "..GGLLLLGG..", ".GLLLLLDDDG.",
    ".GLLLLDDDDG.", "GLLLDDDDDDDG", "GLLDDDDDDDDG", ".GDDDDDDDDG.", "..GGDDDDGG..", "....GGGG...." };
static const char *LONG_ROPE[ITEM_ICON_CELLS] = {
    "............", "..GGGGGGGG..", ".GNNNNNNNNG.", "GNnnnnnnnnNG", "GNnGGGGGGnNG", "GNnG....GnNG",
    "GNnG....GnNG", "GNnGGGGGGnNG", "GNnnnnnnnnNG", ".GNNNNNNNNG.", "..GGGGGGGG..", "............" };
static const char *SPINE_HOOK[ITEM_ICON_CELLS] = {
    "....GGGG....", "...GPPPPG...", "....GPPG....", "...GPPPPG...", "....GPPG....", "...GPPPPG...",
    "....GPPG....", "...GPPPPGG..", "....GPPG.GG.", "....GGPPGPG.", ".....GGGPG..", ".......GG..." };
static const char *CLUB[ITEM_ICON_CELLS] = {
    "......GGGG..", ".....GDDDDG.", "....GDDLDDG.", "....GDDDDG..", "...GGPPGG...", "..GPPG......",
    ".GPPG.......", "GPPG........", "GPG.........", ".G..........", "............", "............" };
static const char *LONG_REACH[ITEM_ICON_CELLS] = {
    "..........GG", ".........GPG", "........GPG.", "......GSSG..", ".....GSSsG..", "....GSSsG...",
    "...GSSsG....", "..GSSsG.....", ".GRRRG......", ".GRRRG......", "..GGG.......", "............" };
static const char *SKULL_STRING[ITEM_ICON_CELLS] = {
    "....GNNG....", "...GN..NG...", "..GGPPPPGG..", ".GPPPPPPPPG.", ".GPGGPPGGPG.", ".GPGGPPGGPG.",
    ".GPPPPGPPPG.", "..GPPPPPPG..", "..GPGPGPGPG.", "...GGGGGGG..", "............", "............" };
static const char *FINGER_NECKLACE[ITEM_ICON_CELLS] = {
    "..GNG..GNG..", ".GN.GGGG.NG.", ".GN......NG.", ".GN......NG.", "..GN....NG..", "...GN..NG...",
    "....GNNG....", "...GSGSGS...", "...GSGSGS...", "....G.G.G...", "............", "............" };
static const char *TOOTH_NECKLACE[ITEM_ICON_CELLS] = {
    "..GNG..GNG..", ".GN.GGGG.NG.", ".GN......NG.", ".GN......NG.", "..GN....NG..", "...GN..NG...",
    "....GNNG....", "...GPGPGP...", "...GPGPGP...", "....G.G.G...", "............", "............" };
static const char *WATCHING_ROCK[ITEM_ICON_CELLS] = {
    "....GGGG....", "..GGLLLLGG..", ".GLLbbbbLLG.", ".GLbBBBBbLG.", "GLLbBGGBbLLG", "GLLbBGGBbLDG",
    "GLLLbBBbLDDG", ".GLDDbbDDDG.", ".GDDDDDDDDG.", "..GGDDDDGG..", "....GGGG....", "............" };
static const char *RIB_CAGE[ITEM_ICON_CELLS] = {
    "....GGGG....", "..GGPPPPGG..", ".GPPGPPGPPG.", "GPPGGPPGGPPG", "GPGG.PP.GGPG", "GPG..PP..GPG",
    "GPG..PP..GPG", "GPG..PP..GPG", "GPG..PP..GPG", "GGG..GG..GGG", "............", "............" };
static const char *HEART_STRING[ITEM_ICON_CELLS] = {
    "....GNNG....", "...GN..NG...", "..GG.GG.GG..", ".GRRRGRRRG..", "GRRNRRRRRRG.", "GRRRRRRRRRG.",
    ".GRRRRRRRG..", "..GRRRRRG...", "...GRRRG....", "....GRG.....", ".....G......", "............" };
static const char *DULL_KNIFE[ITEM_ICON_CELLS] = {
    "............", "..........GG", ".........GDG", "........GDDG", ".......GDDDG", "......GDDG..",
    ".....GDDG...", "..GGGDDG....", ".GwwGDG.....", ".GwwG.......", "..GG........", "............" };
static const char *const *ICONS[ITEM_COUNT] = { HAMMER, ORB, PAPER, KNIFE, ROCK, ARM, LEG, HEAD, RIBS, HEART, LIVER, INTEST, SPINE, FINGERS, EYES, TEETH, SKULL, BONE,
    TIED_ROCK, LONG_ROPE, SPINE_HOOK, CLUB, LONG_REACH, SKULL_STRING, FINGER_NECKLACE, TOOTH_NECKLACE, WATCHING_ROCK, RIB_CAGE, HEART_STRING, DULL_KNIFE };
static const char *NAMES[ITEM_COUNT] = { "HAMMER", "ORB", "NOTE", "KNIFE", "ROCK",
    "ARM", "LEG", "HEAD", "RIBS", "HEART", "LIVER", "INTESTINE", "SPINE", "FINGERS", "EYES", "TEETH", "SKULL", "BONE",
    "TIED ROCK", "LONG ROPE", "SPINE HOOK", "CLUB", "LONG REACH", "SKULL STRING", "FINGER NECKLACE", "TOOTH NECKLACE", "WATCHING ROCK", "RIB CAGE", "HEART STRING", "DULL KNIFE" };
static const char *DESCS[ITEM_COUNT] = {
    "A heavy hammer.", "A violet orb. It glows.", "A note written in blood.",
    "A sharp knife. It cuts bodies, and the one who holds it.", "A small rock. Fits a hand. Good for throwing.",
    "An arm. Heavy in the hands. A knife can take the fingers or the bone.", "A leg. Heavy in the hands. A knife can take the bone.",
    "A head. Don't look at the face. A knife takes eyes and teeth, or cleans it.", "A ribcage. Small enough to pocket.",
    "A heart. Still warm.", "A liver. Dark and heavy.", "Long. Surprisingly long.", "A spine. It could hold a hook.",
    "Fingers. They still curl.", "Two eyes. They don't look at anything.", "Teeth. A handful.", "A clean skull.", "A bone. Long and strong.",
    "A rock on a gut. Swing it over the pit.", "Gut, tied end to end. Plenty of length.", "A spine with a hook. It grabs by itself.",
    "A bone with a rock on it. Smashes bodies.", "A long arm. It reaches what lies far away.", "A skull on a string. Vas dislikes it.",
    "Fingers on a string. Wear it.", "Teeth on a string. It rattles when you walk.", "A rock with an eye. It looks at you.",
    "A cage of ribs. A small shelter.", "A heart on a string. It still beats, for a moment.", "A dull knife. Worse than the first.",
};
static void (*action_cb)(int kind, int act);
const char *item_desc(int kind) { return kind >= 0 && kind < ITEM_COUNT ? DESCS[kind] : ""; }
static int have[ITEM_COUNT];                /* (declared again below, with its comment) */
/* section 7. one result per pair, either order */
static const struct { int a, b, r; } RECIPES[] = {
    { ITEM_ROCK, ITEM_INTESTINE, ITEM_TIED_ROCK }, { ITEM_INTESTINE, ITEM_INTESTINE, ITEM_LONG_ROPE }, { ITEM_SPINE, ITEM_INTESTINE, ITEM_SPINE_HOOK },
    { ITEM_BONE, ITEM_ROCK, ITEM_CLUB }, { ITEM_ARM, ITEM_BONE, ITEM_LONG_REACH }, { ITEM_SKULL, ITEM_INTESTINE, ITEM_SKULL_STRING },
    { ITEM_FINGERS, ITEM_INTESTINE, ITEM_FINGER_NECKLACE }, { ITEM_TEETH, ITEM_INTESTINE, ITEM_TOOTH_NECKLACE }, { ITEM_EYES, ITEM_ROCK, ITEM_WATCHING_ROCK },
    { ITEM_RIBS, ITEM_RIBS, ITEM_RIB_CAGE }, { ITEM_HEART, ITEM_INTESTINE, ITEM_HEART_STRING }, { ITEM_LIVER, ITEM_KNIFE, ITEM_DULL_KNIFE },
};
int item_recipe(int a, int b) {
    for (unsigned i = 0; i < sizeof RECIPES / sizeof RECIPES[0]; i++)
        if ((RECIPES[i].a == a && RECIPES[i].b == b) || (RECIPES[i].a == b && RECIPES[i].b == a)) return RECIPES[i].r;
    return -1;
}
static int in_recipe(int kind) {
    for (unsigned i = 0; i < sizeof RECIPES / sizeof RECIPES[0]; i++) if (RECIPES[i].a == kind || RECIPES[i].b == kind) return 1;
    return 0;
}
static int craft_a = -1;                                    /* the first half of a craft is chosen: the next slot you touch is the second */
static void (*craft_cb)(int a, int b);
void item_set_craft_cb(void (*cb)(int a, int b)) { craft_cb = cb; }
static int worn_[ITEM_COUNT];
int item_wear_toggle(int kind) { if (kind < 0 || kind >= ITEM_COUNT) return 0; worn_[kind] = !worn_[kind]; return worn_[kind]; }
int item_worn(int kind) { return kind >= 0 && kind < ITEM_COUNT && worn_[kind] && have[kind] > 0; }
int item_actions(int kind, int out[4]) {
    int n = 0;
    switch (kind) {
    case ITEM_KNIFE: out[n++] = ACT_STAB; out[n++] = ACT_STAB_SELF; break;
    case ITEM_ROCK:  out[n++] = ACT_THROW; break;
    case ITEM_ARM:   out[n++] = ACT_CUT_FINGERS; out[n++] = ACT_CUT_BONE; out[n++] = ACT_DROP; break;
    case ITEM_LEG:   out[n++] = ACT_CUT_BONE; out[n++] = ACT_DROP; break;
    case ITEM_HEAD:  out[n++] = ACT_CUT_EYES; out[n++] = ACT_CUT_TEETH; out[n++] = ACT_CLEAN; out[n++] = ACT_DROP; break;
    case ITEM_SPINE: case ITEM_RIB_CAGE: case ITEM_LONG_REACH: out[n++] = ACT_DROP; break;
    case ITEM_SPINE_HOOK: out[n++] = ACT_SWING; out[n++] = ACT_DROP; break;
    case ITEM_TIED_ROCK:  out[n++] = ACT_SWING; break;
    case ITEM_CLUB:       out[n++] = ACT_SMASH; out[n++] = ACT_DROP; break;
    case ITEM_SKULL_STRING: case ITEM_FINGER_NECKLACE: case ITEM_TOOTH_NECKLACE: out[n++] = ACT_WEAR; break;
    case ITEM_WATCHING_ROCK: out[n++] = ACT_DROP; break;
    default: break;
    }
    if (in_recipe(kind) && n < 4) out[n++] = ACT_COMBINE;
    return n;
}
const char *item_action_name(int act) { return act == ACT_STAB ? "STAB" : act == ACT_STAB_SELF ? "STAB SELF" : act == ACT_THROW ? "THROW" : act == ACT_CUT_FINGERS ? "CUT FINGERS" : act == ACT_CUT_BONE ? "CUT BONE" : act == ACT_CUT_EYES ? "CUT EYES" : act == ACT_CUT_TEETH ? "CUT TEETH" : act == ACT_CLEAN ? "CLEAN SKULL" : act == ACT_DROP ? "PUT DOWN" : act == ACT_COMBINE ? "COMBINE" : act == ACT_SWING ? "SWING" : act == ACT_SMASH ? "SMASH" : act == ACT_WEAR ? "WEAR" : ""; }
void item_set_action_cb(void (*cb)(int kind, int act)) { action_cb = cb; }

int item_size(int kind) {
    switch (kind) {                         /* arms, legs, heads, spines and bones are carried in the hands (one at a time); the rest fit a pocket */
    case ITEM_ARM: case ITEM_LEG: case ITEM_HEAD: case ITEM_SPINE:
    case ITEM_SPINE_HOOK: case ITEM_CLUB: case ITEM_LONG_REACH: case ITEM_RIB_CAGE: return SIZE_MEDIUM;      /* (the bone is small: arm + bone needs both) */
    default: return SIZE_SMALL;
    }
}

const char *item_name(int kind) { return kind >= 0 && kind < ITEM_COUNT ? NAMES[kind] : ""; }

static int s = 1, tile = 16;                /* pixel scale and tile size of the current map */
/* story chunk 8: several things can lie on the map now (the paper, rocks, a thrown rock). the oldest is dropped if GROUND_MAX is passed */
#define GROUND_MAX 12
typedef struct { int kind, tx, ty, gs_x, gs_y, gs_seen; float drop_t; } Ground;       /* gs_*: where it was drawn last (screen px): its flight to the bag starts there. drop_t: seconds since it was dropped, 99 = simply placed */
static Ground gnd[GROUND_MAX]; static int ng;
static int have[ITEM_COUNT];
static float new_t = 99.0f;                 /* seconds since the last pickup LANDED in the bag (the slot flashes, the badge ticks); negative while the icon is still flying */
static int   newest = -1;                   /* chunk 70: the kind that came last (gold frame) */
static float fly_t = 99.0f; static int fly_kind, fly_x, fly_y;      /* chunk 70: the icon flying from the ground to the bag */
#define INV_FLY_T   0.4f
#define INV_EASE_T  0.25f
#define INV_IDLE_T  4.0f
static int   ui_open, ui_down;              /* the panel is asked open / which part of the inventory the finger went down on (1 bag, 2 panel) */
static float ui_p, ui_idle;                 /* how far the panel is out (0..1), seconds since the last touch on it */
static int   ui_sel = -1;                   /* the slot whose name shows */
#define DROP_BOUNCE_T 0.2f
#define DROP_HEIGHT   14                    /* units it falls from */

static int locked[ITEM_COUNT], n_blocked;
static void (*blocked_cb)(void);
void item_set_locked(int kind, int lk) { if (kind >= 0 && kind < ITEM_COUNT) locked[kind] = lk ? 1 : 0; }
int  item_locked(int kind) { return kind >= 0 && kind < ITEM_COUNT && locked[kind]; }
void item_set_blocked_cb(void (*cb)(void)) { blocked_cb = cb; }
int  item_blocked_count(void) { return n_blocked; }
void item_reset(int pixel_scale, int tile_size) { s = pixel_scale; tile = tile_size; ng = 0; memset(locked, 0, sizeof locked); blocked_cb = NULL; }
void item_place(int kind, int tile_x, int tile_y) {
    if (kind < 0 || kind >= ITEM_COUNT) return;
    if (ng == GROUND_MAX) { memmove(gnd, gnd + 1, sizeof(Ground) * (GROUND_MAX - 1)); ng--; }
    Ground g = { kind, tile_x, tile_y, 0, 0, 0, 99.0f }; gnd[ng++] = g;
}
void item_drop(int kind, int tile_x, int tile_y) { int before = ng; item_place(kind, tile_x, tile_y); if (ng > 0 && (ng > before || before == GROUND_MAX)) gnd[ng - 1].drop_t = 0; }
int  item_dropping(void) { for (int i = 0; i < ng; i++) if (gnd[i].drop_t < ITEM_DROP_T) return 1; return 0; }
int  item_exists(void) { return ng > 0; }
int  item_ground_count(void) { return ng; }
int  item_ground_get(int i, int *kind, int *tile_x, int *tile_y) { if (i < 0 || i >= ng) return 0; *kind = gnd[i].kind; *tile_x = gnd[i].tx; *tile_y = gnd[i].ty; return 1; }
void item_tile(float *x, float *y) { if (ng > 0) { *x = gnd[0].tx + 0.5f; *y = gnd[0].ty + 0.5f; } else { *x = *y = 0; } }
int  item_has(int kind) { return kind >= 0 && kind < ITEM_COUNT && have[kind] > 0; }
int  item_count(int kind) { return kind >= 0 && kind < ITEM_COUNT ? have[kind] : 0; }
void item_give(int kind) { if (kind < 0 || kind >= ITEM_COUNT) return; if (have[kind] < 99) have[kind]++; new_t = 0; newest = kind; }
void item_take(int kind) { if (kind >= 0 && kind < ITEM_COUNT && have[kind] > 0) have[kind]--; }
int  item_medium_held(void) { for (int k = 0; k < ITEM_COUNT; k++) if (have[k] > 0 && item_size(k) == SIZE_MEDIUM) return 1; return 0; }
static void (*full_cb)(void);
void item_set_full_cb(void (*cb)(void)) { full_cb = cb; }
void item_pack(unsigned char out[ITEM_COUNT]) { for (int k = 0; k < ITEM_COUNT; k++) out[k] = (unsigned char)(have[k] > 0 ? have[k] : 0); }
void item_unpack(const unsigned char in[ITEM_COUNT]) { for (int k = 0; k < ITEM_COUNT; k++) have[k] = in[k]; }
void item_clear(void) { memset(have, 0, sizeof have); memset(worn_, 0, sizeof worn_); craft_a = -1; new_t = 99.0f; newest = -1; fly_t = 99.0f; ui_open = ui_down = 0; ui_p = 0; ui_sel = -1; }

static int owned(void);
void item_update(float dt) {
    if (new_t < 99.0f) new_t += dt;
    if (fly_t < INV_FLY_T) fly_t += dt;
    if (owned() == 0) { ui_open = ui_down = 0; ui_p = 0; }
    if (ui_open) { ui_idle += dt; if (ui_idle >= INV_IDLE_T) ui_open = 0; }        /* chunk 70: it never stays open by itself */
    if (ui_open && ui_p < 1.0f) { ui_p += dt / INV_EASE_T; if (ui_p > 1.0f) ui_p = 1.0f; }
    if (!ui_open && ui_p > 0.0f) { ui_p -= dt / INV_EASE_T; if (ui_p < 0.0f) ui_p = 0.0f; }
    for (int i = 0; i < ng; i++) {                              /* chunk 32: a dropped item falls first, and thunks when it lands */
        if (gnd[i].drop_t >= 99.0f) continue;
        float before = gnd[i].drop_t; gnd[i].drop_t += dt;
        if (before < ITEM_DROP_T && gnd[i].drop_t >= ITEM_DROP_T) sfx_blip(0.42f);
        if (gnd[i].drop_t > ITEM_DROP_T + DROP_BOUNCE_T + 1.0f) gnd[i].drop_t = 99.0f;
    }
}
static int nearest_in_reach(float pxw, float pyw) {              /* the nearest landed item within ITEM_REACH_R tiles of the feet, or -1 */
    int best = -1; float bd = ITEM_REACH_R;
    for (int i = 0; i < ng; i++) {
        if (gnd[i].drop_t < ITEM_DROP_T) continue;
        float dx = pxw - (gnd[i].tx + 0.5f) * tile, dy = pyw - (gnd[i].ty + 0.5f) * tile, d = sqrtf(dx * dx + dy * dy) / tile;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
int item_in_reach(float pxw, float pyw) { return nearest_in_reach(pxw, pyw) >= 0; }
int item_grab(float pxw, float pyw) {                           /* chunk 68: the tap on the hand */
    int i = nearest_in_reach(pxw, pyw);
    if (i < 0) return -1;
    int k = gnd[i].kind;
    if (locked[k]) {                                            /* chunk 33: it stays where it is, every press says so */
        n_blocked++; if (blocked_cb) blocked_cb();
        return -1;
    }
    if (item_size(k) == SIZE_MEDIUM && item_medium_held()) { sfx_error(); if (full_cb) full_cb(); return -1; }       /* chunk 9: the hands are full */
    int seen = gnd[i].gs_seen, gx = gnd[i].gs_x, gy = gnd[i].gs_y;
    memmove(gnd + i, gnd + i + 1, sizeof(Ground) * (size_t)(ng - i - 1)); ng--;
    item_give(k);
    if (seen) { fly_kind = k; fly_x = gx; fly_y = gy; fly_t = 0; new_t = -INV_FLY_T; }      /* chunk 70: it flies to the bag, the flash and the badge wait for it */
    return k;
}

/* ---------- drawing ---------- */
static void rgb_of(char c, int *R, int *G, int *B) {
    switch (c) {
        case 'G': *R = 36;  *G = 40;  *B = 58;  break;
        case 'L': *R = 206; *G = 214; *B = 232; break;
        case 'D': *R = 122; *G = 130; *B = 156; break;
        case 'W': *R = 196; *G = 136; *B = 74;  break;
        case 'B': *R = 150; *G = 200; *B = 236; break;
        case 'b': *R = 244; *G = 250; *B = 255; break;
        case 'V': *R = 150; *G = 96;  *B = 220; break;
        case 'v': *R = 88;  *G = 52;  *B = 150; break;
        case 'S': *R = 236; *G = 190; *B = 150; break;
        case 's': *R = 200; *G = 140; *B = 104; break;
        case 'Y': *R = 236; *G = 190; *B = 78;  break;
        case 'y': *R = 255; *G = 238; *B = 170; break;
        case 'P': *R = 232; *G = 222; *B = 196; break;
        case 'R': *R = 150; *G = 28;  *B = 34;  break;
        case 'N': *R = 222; *G = 126; *B = 136; break;      /* pink (gut) */
        case 'n': *R = 164; *G = 74;  *B = 92;  break;      /* dark pink */
        case 'M': *R = 104; *G = 30;  *B = 44;  break;      /* maroon (liver) */
        default:  *R = 132; *G = 84;  *B = 42;  break;      /* w */
    }
}

/* the picture, optionally turned on its side (the handle points right): used for the one lying on the grass */
static void draw_rows(SDL_Renderer *r, const char *const *rows, int x, int y, int cell, int lying) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int j = 0; j < ITEM_ICON_CELLS; j++)
        for (int i = 0; i < ITEM_ICON_CELLS; i++) {
            char c = rows[j][i];
            if (c == '.') continue;
            int R, G, B; rgb_of(c, &R, &G, &B);
            SDL_SetRenderDrawColor(r, R, G, B, 255);
            SDL_Rect q = { x + (lying ? j : i) * cell, y + (lying ? i : j) * cell, cell, cell };
            SDL_RenderFillRect(r, &q);
        }
}
/* chunk 69: the same picture at HALF the size with square cells: every 2 x 2 block of the 12 x 12 icon becomes one cell (the commonest colour in the block; a
 * block with nothing in it stays empty), so it is 6 x 6 cells at the same cell size, centred where the big one was */
static void draw_rows_half(SDL_Renderer *r, const char *const *rows, int x, int y, int cell, int lying) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int j = 0; j < ITEM_ICON_CELLS / 2; j++)
        for (int i = 0; i < ITEM_ICON_CELLS / 2; i++) {
            char c4[4] = { rows[2 * j][2 * i], rows[2 * j][2 * i + 1], rows[2 * j + 1][2 * i], rows[2 * j + 1][2 * i + 1] };
            char best = '.'; int bestn = 0;
            for (int a = 0; a < 4; a++) {
                if (c4[a] == '.') continue;
                int n = 0; for (int b = 0; b < 4; b++) if (c4[b] == c4[a]) n++;
                if (n > bestn) { bestn = n; best = c4[a]; }
            }
            if (best == '.') continue;
            int R, G, B; rgb_of(best, &R, &G, &B);
            SDL_SetRenderDrawColor(r, R, G, B, 255);
            SDL_Rect q = { x + (lying ? j : i) * cell, y + (lying ? i : j) * cell, cell, cell };
            SDL_RenderFillRect(r, &q);
        }
}
static void draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell, int lying) { draw_rows(r, ICONS[kind], x, y, cell, lying); }
void item_draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell) {
    if (kind < 0 || kind >= ITEM_COUNT || cell < 1) return;
    draw_icon(r, kind, x, y, cell, 0);
}
void item_draw_bag_icon(SDL_Renderer *r, int x, int y, int cell) {
    if (cell < 1) return;
    draw_rows(r, BAG, x, y, cell, 0);
}
void item_draw_hand_icon(SDL_Renderer *r, int x, int y, int cell) {
    if (cell < 1) return;
    draw_rows(r, HAND, x, y, cell, 0);
}
void item_draw_search_icon(SDL_Renderer *r, int x, int y, int cell) {
    if (cell < 1) return;
    draw_rows(r, SEARCH, x, y, cell, 0);
}

void item_draw_ground(SDL_Renderer *r, int cam_x, int cam_y, float t) {
    for (int n = 0; n < ng; n++) {
    Ground *g = &gnd[n]; float drop_t = g->drop_t;
    int x = (int)((g->tx + 0.5f) * tile) - cam_x, y = (int)((g->ty + 0.5f) * tile) - cam_y;      /* centre of the spot */
    int bob = (int)floorf(sinf(t * 3.0f + n) * 0.9f + 0.5f);
    int lift = 0;                                                                           /* chunk 32: how high a dropped item still is (units) */
    if (drop_t < ITEM_DROP_T) { float k = drop_t / ITEM_DROP_T; lift = (int)(DROP_HEIGHT * (1.0f - k * k) + 0.5f); bob = 0; }
    else if (drop_t < ITEM_DROP_T + DROP_BOUNCE_T) { lift = (int)(3.0f * sinf((drop_t - ITEM_DROP_T) / DROP_BOUNCE_T * 3.14159f) + 0.5f); bob = 0; }
    g->gs_x = x; g->gs_y = y - 2 * s + bob * s - lift * s; g->gs_seen = 1;                  /* chunk 70: where the flight to the bag starts */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 70);
    SDL_Rect sh = { x - 7 * s / 2, y + 3 * s / 2, 7 * s, s }; SDL_RenderFillRect(r, &sh);   /* ground shadow (chunk 69: half) */
    draw_rows_half(r, ICONS[g->kind], x - 3 * s, y - 3 * s + bob * s - lift * s, s, 1);     /* chunk 69: half the size, centred on the tile */
    if (drop_t >= ITEM_DROP_T && drop_t < ITEM_DROP_T + 0.5f) {                             /* landed: a burst of glints */
        float k = (drop_t - ITEM_DROP_T) / 0.5f; int a = (int)(255 * (1.0f - k)), d = (int)(3 + 9 * k);
        SDL_SetRenderDrawColor(r, 255, 255, 255, a);
        for (int q = 0; q < 4; q++) {
            int ox = (q & 1 ? d : -d), oy = (q & 2 ? d : -d) / 2;
            SDL_Rect v = { x + ox * s, y - 4 * s + oy * s, s, s }; SDL_RenderFillRect(r, &v);
        }
    }
    float ph = fmodf(t * 0.8f + n * 0.37f, 1.0f);                                           /* a glint, so it can be spotted on the grass */
    if (ph < 0.35f) {
        int a = (int)(255 * sinf(ph / 0.35f * 3.14159f));
        int gx = x - 4 * s, gy = y - 7 * s + bob * s;
        SDL_SetRenderDrawColor(r, 255, 255, 255, a);
        SDL_Rect v = { gx, gy - s, s, 3 * s }, h = { gx - s, gy, 3 * s, s };
        SDL_RenderFillRect(r, &v); SDL_RenderFillRect(r, &h);
    }
    }
}

/* ---------- the inventory widget (chunk 70) ----------
 * closed: one small bag button under the minimap (nothing at all while the pockets are empty) with a badge that counts what is in them.
 * a tap on it, and the panel eases out under it (INV_EASE_T): framed square slots, the newest in gold, the name of the slot you touched last under the grid.
 * it folds back by itself after INV_IDLE_T without a touch, on a tap outside it, or on a tap on the bag. a new pick-up flies from the ground to the bag, then the
 * badge ticks and the slot flashes. pixel art only: square cells, no blend modes other than plain alpha. */
#define INV_COLS 3
static int W, H, ux, uy, uw, bt, bag, icell, pad, psl, pcell, qs;

void item_ui_resize(int w, int h, int right_edge, int top, int width) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f;
    bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    pad = (int)(4 * u); if (pad < 2) pad = 2;
    qs = (int)(2 * u); if (qs < 1) qs = 1;                                                  /* one step of the little ornaments */
    uw = width; ux = right_edge - width; uy = top;
    bag = width / 3; if (bag < 24) bag = 24;
    icell = (bag - 2 * bt - 2 * pad) / ITEM_ICON_CELLS; if (icell < 1) icell = 1;
    psl = (uw - 2 * bt - (INV_COLS + 1) * pad) / INV_COLS; if (psl < 24) psl = 24;
    pcell = (psl - 2 * bt - pad) / ITEM_ICON_CELLS; if (pcell < 1) pcell = 1;
}

void item_ui_init(int w, int h, int right_edge, int top, int width) {
    item_ui_resize(w, h, right_edge, top, width);
    ui_open = ui_down = 0; ui_p = ui_idle = 0; ui_sel = -1;
}

static int owned(void) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) n += have[k] > 0 ? 1 : 0; return n; }
static int shown(void) { return owned() - (fly_t < INV_FLY_T ? 1 : 0); }                 /* the badge does not count what is still in the air */
static int fcell(const char *name, int room) {              /* the biggest letter size (up to 3) that still fits `room` */
    int c = (int)(2.0f * (W < H ? W : H) / 360.0f); if (c < 1) c = 1; if (c > 3) c = 3;
    while (c > 1 && font_width(name, c) > room) c--;
    return c;
}
static int slots_n(void) { int n = owned(); int rows = (n + INV_COLS - 1) / INV_COLS; if (rows < 1) rows = 1; return rows * INV_COLS; }
static int label_h(void) { return 2 * qs + font_height(fcell("MMMMMM", uw - 2 * bt - 2 * pad)) + pad; }
/* story chunk 8: under the name, the description of the selected item (a few short lines) and one button per action */
#define DESC_LINES 7
static int sel_ok(void) { return ui_sel >= 0 && ui_sel < ITEM_COUNT && have[ui_sel] > 0; }
static int dcell(void) { int c = (int)((W < H ? W : H) / 360.0f); if (c < 1) c = 1; if (c > 2) c = 2; return c; }
static int wrap(const char *text, int cell, int room, char out[DESC_LINES][64]) {      /* greedy word wrap; returns the number of lines */
    int n = 0; out[0][0] = 0; char word[64]; const char *p = text;
    while (*p) {
        while (*p == ' ') p++;
        int wl = 0; while (*p && *p != ' ' && wl < 63) word[wl++] = *p++;
        word[wl] = 0; if (!wl) break;
        char tr[140];
        if (out[n][0]) snprintf(tr, sizeof tr, "%s %s", out[n], word); else snprintf(tr, sizeof tr, "%s", word);
        if (out[n][0] && font_width(tr, cell) > room) { if (n + 1 >= DESC_LINES) break; n++; snprintf(out[n], 64, "%s", word); }
        else snprintf(out[n], 64, "%s", tr);
    }
    return out[0][0] ? n + 1 : 0;
}
static int d_pitch(void) { return font_height(dcell()) + qs; }
static int act_h(void) { return font_height(fcell("MMMMMM", uw - 2 * bt - 2 * pad)) + 2 * pad; }
static int desc_n(void) { char ln[DESC_LINES][64]; return sel_ok() && craft_a < 0 ? wrap(item_desc(ui_sel), dcell(), uw - 2 * bt - 2 * pad, ln) : 0; }
static int act_n(int out[4]) { return sel_ok() && craft_a < 0 ? item_actions(ui_sel, out) : 0; }
static int extra_h(void) { int a[4], nl = desc_n(), na = act_n(a); return (nl ? nl * d_pitch() + pad : 0) + na * (act_h() + pad); }
static int panel_full_h(void) { int rows = slots_n() / INV_COLS; return 2 * bt + pad + rows * psl + (rows - 1) * pad + pad + label_h() + extra_h(); }
static int grid_bottom(void) { int rows = slots_n() / INV_COLS; return uy + bag + pad + bt + pad + rows * psl + (rows - 1) * pad; }
static void act_rect(int i, int *x, int *y, int *w, int *h) {
    int nl = desc_n();
    *x = ux + bt + pad; *w = uw - 2 * bt - 2 * pad; *h = act_h();
    *y = grid_bottom() + label_h() + (nl ? nl * d_pitch() + pad : 0) + i * (*h + pad);
}
static int panel_h(void) {                                  /* how tall it is right now: eased out, in whole steps of `pad` */
    float k = 1.0f - ui_p, e = 1.0f - k * k * k; int full = panel_full_h();
    if (ui_p >= 1.0f) return full;
    int h = (int)(full * e) / pad * pad; return h > full ? full : h;
}
static void bag_rect(int *x, int *y, int *w, int *h) { *x = ux + uw - bag; *y = uy; *w = bag; *h = bag; }
static void panel_rect(int *x, int *y, int *w, int *h) { *x = ux; *y = uy + bag + pad; *w = uw; *h = panel_h(); }
static int in_rect(int x, int y, int rx, int ry, int rw, int rh) { return x >= rx && y >= ry && x < rx + rw && y < ry + rh; }
static int hit_bag(int x, int y) { int a, b, c, d; bag_rect(&a, &b, &c, &d); return in_rect(x, y, a, b, c, d); }
static int hit_panel(int x, int y) { if (ui_p <= 0.0f) return 0; int a, b, c, d; panel_rect(&a, &b, &c, &d); return in_rect(x, y, a, b, c, d); }
static int slot_x(int i) { return ux + bt + pad + (i % INV_COLS) * (psl + pad); }
static int slot_y(int i) { return uy + bag + pad + bt + pad + (i / INV_COLS) * (psl + pad); }
static int kind_at_slot(int i) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (have[k] > 0) { if (n == i) return k; n++; } return -1; }
static int slot_of_kind(int kind) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (have[k] > 0) { if (k == kind) return n; n++; } return -1; }

int item_ui_hit(int x, int y) { return owned() > 0 && (hit_bag(x, y) || hit_panel(x, y)); }
int item_ui_open(void) { return ui_open && owned() > 0; }
float item_ui_progress(void) { return ui_p; }
int item_ui_selected(void) { return ui_sel; }
int item_ui_flying(void) { return fly_t < INV_FLY_T; }
int item_ui_shown(void) { return shown(); }
int item_ui_slot_x(int kind) { int i = slot_of_kind(kind); return i < 0 ? -1 : slot_x(i) + psl / 2; }
int item_ui_slot_y(int kind) { int i = slot_of_kind(kind); return i < 0 ? -1 : slot_y(i) + psl / 2; }
int item_ui_action_x(int i) { int a[4], n = act_n(a); if (i < 0 || i >= n) return -1; int x, y, w, h; act_rect(i, &x, &y, &w, &h); return x + w / 2; }
int item_ui_action_y(int i) { int a[4], n = act_n(a); if (i < 0 || i >= n) return -1; int x, y, w, h; act_rect(i, &x, &y, &w, &h); return y + h / 2; }
int item_ui_bag_x(void) { return ux + uw - bag / 2; }
int item_ui_bag_y(void) { return uy + bag / 2; }

static void set_open(int o) {
    if (o && !ui_open) { ui_idle = 0; ui_sel = newest >= 0 && have[newest] > 0 ? newest : kind_at_slot(0); }
    if (!o) craft_a = -1;
    ui_open = o;
}
int item_ui_touch(int a, int x, int y) {
    if (owned() == 0) { ui_open = ui_down = 0; ui_p = 0; return 0; }
    if (a == 0) {
        if (hit_bag(x, y)) { ui_down = 1; return 1; }
        if (ui_open && hit_panel(x, y)) { ui_down = 2; ui_idle = 0; return 1; }
        if (ui_open) set_open(0);                            /* a tap outside folds it (and goes on to whatever is there) */
        return 0;
    }
    if (!ui_down) return 0;
    if (a == 1) {
        if (ui_down == 1 && hit_bag(x, y)) set_open(!ui_open);
        else if (ui_down == 2 && hit_panel(x, y)) {
            ui_idle = 0;
            int acts[4], na = ui_p >= 1.0f ? act_n(acts) : 0;
            for (int i = 0; i < na; i++) {                   /* story chunk 8: an action button: fold the panel, then the story decides what happens */
                int rx, ry, rw, rh; act_rect(i, &rx, &ry, &rw, &rh);
                if (in_rect(x, y, rx, ry, rw, rh)) {
                    int k = ui_sel; ui_down = 0;
                    if (acts[i] == ACT_COMBINE) { craft_a = k; ui_idle = 0; return 1; }       /* chunk 10: now touch the second slot */
                    ui_open = 0; if (action_cb) action_cb(k, acts[i]); return 1;
                }
            }
            for (int i = 0; i < slots_n(); i++) {
                int sx = slot_x(i), sy = slot_y(i);
                if (in_rect(x, y, sx - pad / 2, sy - pad / 2, psl + pad, psl + pad) && kind_at_slot(i) >= 0) {
                    int kk = kind_at_slot(i);
                    if (craft_a >= 0) { int a0 = craft_a; craft_a = -1; ui_open = 0; if (craft_cb) craft_cb(a0, kk); }     /* chunk 10: the second half of a craft */
                    else ui_sel = kk;
                }
            }
        }
        ui_down = 0; return 1;
    }
    if (a == 3) ui_down = 0;                                 /* cancelled */
    return 1;                                                /* a drag that began on the inventory stays its own */
}

static void box(SDL_Renderer *r, int x, int y, int w, int h, int br, int bg, int bb, int fr, int fg, int fb, int fa) {
    SDL_Rect o = { x, y, w, h }, in = { x + bt, y + bt, w - 2 * bt, h - 2 * bt };
    SDL_SetRenderDrawColor(r, br, bg, bb, 255); SDL_RenderFillRect(r, &o);
    SDL_SetRenderDrawColor(r, fr, fg, fb, fa); SDL_RenderFillRect(r, &in);
}
static void draw_flying(SDL_Renderer *r) {
    float k = fly_t / INV_FLY_T; if (k > 1.0f) k = 1.0f;
    int tx_ = item_ui_bag_x(), ty_ = item_ui_bag_y();
    for (int j = 3; j >= 0; j--) {                           /* a little gold trail behind it, then the icon itself (j = 0) */
        float kj = k - j * 0.07f; if (kj < 0) continue;
        float e = kj * kj * (3.0f - 2.0f * kj);
        int x = fly_x + (int)((tx_ - fly_x) * e), y = fly_y + (int)((ty_ - fly_y) * e) - (int)(4.0f * bag * kj * (1.0f - kj));
        if (j > 0) {
            SDL_SetRenderDrawColor(r, 255, 230, 140, 200 - 50 * j);
            SDL_Rect q = { x - s / 2, y - s / 2, s, s }; SDL_RenderFillRect(r, &q);
        } else {
            int cell = s + (int)((icell - s) * e + 0.5f); if (cell < 1) cell = 1;
            if (e < 0.5f) draw_rows_half(r, ICONS[fly_kind], x - 3 * cell, y - 3 * cell, cell, 1);      /* it still lies on its side, then it turns upright as it goes */
            else          draw_rows_half(r, ICONS[fly_kind], x - 3 * cell, y - 3 * cell, cell, 0);
        }
    }
}

void item_ui_draw(SDL_Renderer *r, float t) {
    int n = owned();
    if (n == 0) return;
    (void)t;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int flash = new_t >= 0.0f && new_t < 1.5f;
    int blink = flash && ((int)(new_t * 8.0f) & 1) == 0;

    /* the panel first (it comes out from under the bag) */
    if (ui_p > 0.0f) {
        int px_, py_, pw_, ph_; panel_rect(&px_, &py_, &pw_, &ph_);
        int full = panel_full_h(), rows = slots_n() / INV_COLS;
        SDL_Rect clip = { px_, py_, pw_, ph_ };
        SDL_RenderSetClipRect(r, &clip);
        box(r, px_, py_, pw_, full, 214, 170, 70, 6, 8, 20, 245);
        for (int i = 0; i < slots_n(); i++) {
            int sx = slot_x(i), sy = slot_y(i), k = kind_at_slot(i);
            if (k < 0) { box(r, sx, sy, psl, psl, 40, 46, 70, 10, 12, 26, 255); continue; }        /* an empty slot */
            int gold = k == newest, br = 122, bg = 130, bb = 156;
            if (gold) { br = 236; bg = 190; bb = 78; if (blink) { br = 255; bg = 250; bb = 220; } }
            else if (k == ui_sel) { br = 206; bg = 214; bb = 232; }
            if (k == craft_a) { br = 255; bg = 130; bb = 150; }
            box(r, sx, sy, psl, psl, br, bg, bb, 20, 24, 46, 255);
            if (gold && flash) { SDL_SetRenderDrawColor(r, 255, 238, 170, blink ? 70 : 0); SDL_Rect in = { sx + bt, sy + bt, psl - 2 * bt, psl - 2 * bt }; SDL_RenderFillRect(r, &in); }
            item_draw_icon(r, k, sx + (psl - ITEM_ICON_CELLS * pcell) / 2, sy + (psl - ITEM_ICON_CELLS * pcell) / 2, pcell);
            if (have[k] > 1) {                                   /* chunk 9: more than one of a kind: the number in the corner */
                char num[8]; snprintf(num, sizeof num, "%d", have[k]); int cc = dcell();
                SDL_SetRenderDrawColor(r, 255, 238, 170, 255);
                font_draw(r, num, sx + psl - bt - font_width(num, cc) - qs, sy + psl - bt - font_height(cc) - qs, cc);
            }
        }
        if (ui_sel >= 0 && have[ui_sel] > 0) {                    /* the name under the slot you touched: a little pointer, then the word */
            int i = slot_of_kind(ui_sel), cx = slot_x(i) + psl / 2;
            int gy = uy + bag + pad + bt + pad + rows * psl + (rows - 1) * pad;           /* the bottom of the grid */
            SDL_SetRenderDrawColor(r, 236, 190, 78, 255);
            SDL_Rect a0 = { cx - 2 * qs, gy + pad / 2, 4 * qs, qs }, a1 = { cx - qs, gy + pad / 2 + qs, 2 * qs, qs };
            SDL_RenderFillRect(r, &a0); SDL_RenderFillRect(r, &a1);
            const char *nm = craft_a >= 0 ? "WITH WHAT?" : item_name(ui_sel); int fc = fcell(nm, pw_ - 2 * bt - 2 * pad), tw = font_width(nm, fc);
            int x0 = cx - tw / 2, lo = px_ + bt + pad, hi = px_ + pw_ - bt - pad - tw; if (x0 > hi) x0 = hi; if (x0 < lo) x0 = lo;
            SDL_SetRenderDrawColor(r, 235, 240, 255, 255);
            font_draw(r, nm, x0, gy + pad / 2 + 2 * qs + qs / 2, fc);
            char ln[DESC_LINES][64]; int dc = dcell(), nl = wrap(item_desc(ui_sel), dc, pw_ - 2 * bt - 2 * pad, ln);      /* story chunk 8: what it is ... */
            SDL_SetRenderDrawColor(r, 178, 190, 222, 255);
            for (int j = 0; j < nl; j++) font_draw(r, ln[j], px_ + bt + pad, grid_bottom() + label_h() + j * d_pitch(), dc);
            int acts[4], na = act_n(acts);                        /* ... and what it can do: one button each */
            for (int j = 0; j < na; j++) {
                int rx, ry, rw, rh; act_rect(j, &rx, &ry, &rw, &rh);
                box(r, rx, ry, rw, rh, 236, 190, 78, 40, 34, 20, 255);
                const char *an = item_action_name(acts[j]); int ac = fcell(an, rw - 2 * pad), aw = font_width(an, ac);
                SDL_SetRenderDrawColor(r, 255, 238, 170, 255);
                font_draw(r, an, rx + (rw - aw) / 2, ry + (rh - font_height(ac)) / 2, ac);
            }
        }
        SDL_RenderSetClipRect(r, NULL);
        if (ph_ < full) { SDL_SetRenderDrawColor(r, 214, 170, 70, 255); SDL_Rect e = { px_, py_ + ph_ - bt, pw_, bt }; SDL_RenderFillRect(r, &e); }   /* the edge while it is still growing */
    }

    /* the bag */
    {   int bx, by, bw, bh; bag_rect(&bx, &by, &bw, &bh);
        int up = ui_open || ui_p > 0.0f, bounce = (new_t >= 0.0f && new_t < 0.2f) ? qs : 0;
        box(r, bx, by, bw, bh, up ? 236 : 214, up ? 190 : 170, up ? 78 : 70, 6, 8, 20, 245);
        item_draw_bag_icon(r, bx + (bw - ITEM_ICON_CELLS * icell) / 2, by + (bh - ITEM_ICON_CELLS * icell) / 2 - bounce, icell);
        int cnt = shown();
        if (cnt > 0) {                                       /* the badge on the lower left corner */
            char num[16]; snprintf(num, sizeof num, "%d", cnt);
            int fc = fcell("00", bag / 2), tw = font_width(num, fc), th = font_height(fc);
            int bw2 = tw + 2 * pad, bh2 = th + pad, x0 = bx - bw2 / 2, y0 = by + bh - bh2 + bh2 / 3 - (new_t >= 0.0f && new_t < 0.2f ? qs : 0);
            int tick = new_t >= 0.0f && new_t < 0.3f;
            box(r, x0, y0, bw2, bh2, 236, 190, 78, tick ? 255 : 6, tick ? 238 : 8, tick ? 170 : 20, 255);
            SDL_SetRenderDrawColor(r, tick ? 6 : 255, tick ? 8 : 238, tick ? 20 : 170, 255);
            font_draw(r, num, x0 + (bw2 - tw) / 2, y0 + (bh2 - th) / 2, fc);
        }
    }
    if (fly_t < INV_FLY_T) draw_flying(r);
}
