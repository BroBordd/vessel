/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "item.h"
#include "font.h"
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
/* the small rock (story chunk 8, redrawn in chunk 15): a rough grey-brown stone, lit from the top left. J light, K body, k shade, G outline */
static const char *ROCK[ITEM_ICON_CELLS] = {
    "............",
    "............",
    ".....GGG....",
    "...GGJJJGG..",
    "..GJJJJKKKG.",
    ".GJJJKKKKKKG",
    ".GJJKKkKKKKG",
    "GKKKKKkkKKkG",
    "GKKKKkkkkkkG",
    ".GkkkkkGkkG.",
    "..GGkkkGGG..",
    "...GGGGG....",
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
    case ITEM_KNIFE: out[n++] = ACT_STAB; out[n++] = ACT_STAB_SELF; out[n++] = ACT_THROW; break;
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
#define GROUND_MAX 16
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
static int   held = -1;                     /* story chunk 16: the kind set as the one in hand */
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
void item_clear(void) { memset(have, 0, sizeof have); memset(worn_, 0, sizeof worn_); craft_a = -1; new_t = 99.0f; newest = -1; fly_t = 99.0f; ui_open = ui_down = 0; ui_p = 0; ui_sel = -1; held = -1; }

static int owned(void);
void item_update(float dt) {
    if (new_t < 99.0f) new_t += dt;
    if (fly_t < INV_FLY_T) fly_t += dt;
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
        case 'J': *R = 186; *G = 178; *B = 166; break;      /* stone: light */
        case 'K': *R = 138; *G = 131; *B = 124; break;      /* stone: body */
        case 'k': *R = 92;  *G = 87;  *B = 86;  break;      /* stone: shade */
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
    int rock = g->kind == ITEM_ROCK;                                                         /* chunk 15: a rock does not bob; now and then it sparkles */
    int bob = rock ? 0 : (int)floorf(sinf(t * 3.0f + n) * 0.9f + 0.5f);
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
    float win = rock ? 0.10f : 0.35f, ph = fmodf(t * (rock ? 0.22f : 0.8f) + n * 0.37f, 1.0f);   /* a glint, so it can be spotted on the grass (a rock: one short sparkle every few seconds) */
    if (ph < win) {
        int a = (int)(255 * sinf(ph / win * 3.14159f));
        int gx = x - 4 * s, gy = y - 7 * s + bob * s;
        SDL_SetRenderDrawColor(r, 255, 255, 255, a);
        SDL_Rect v = { gx, gy - s, s, 3 * s }, h = { gx - s, gy, 3 * s, s };
        SDL_RenderFillRect(r, &v); SDL_RenderFillRect(r, &h);
    }
    }
}

/* ---------- the inventory (story chunk 16) ----------
 * closed: two small buttons under the minimap, always there. the left one is a PREVIEW of the one thing in hand (its 12 x 12 icon in a frame, an empty frame
 * when nothing is in hand; a medium thing carried in the hands shows there too), the right one is the BAG: a tap on either opens the inventory window.
 * open: a real panel over the game (the world is paused, world.c): a grid of slots, the picture, name, description and buttons of the selected item, the craft
 * pair while COMBINE waits for its second item, and IN HAND to set the selected item as the one in hand. X, or a tap outside, or the back button closes it.
 * a tap on an action button closes it and calls the action callback (the story decides what happens), as it did with the old folding panel. */
#define INV_COLS 4
#define DESC_LINES 7
static int W, H, ux, uy, uw, bt, bag, icell, pad, qs;       /* the two buttons under the minimap */
static float uu = 1.0f;                                    /* one unit: a 360th of the short side of the screen */
/* story chunk 17: the same window can show a BODY's pockets instead of ours (searching it): the slots are its things, a tap on TAKE moves the stack to our pockets (the
 * story does it, body_cb), TAKE ALL moves everything. an empty body still opens the window, with empty slots */
static int bw_on; static unsigned bw_vessel; static char bw_name[32]; static unsigned char bw_inv[ITEM_COUNT];
static void (*body_cb)(unsigned vessel, int kind);
void item_set_body_cb(void (*cb)(unsigned vessel, int kind)) { body_cb = cb; }
static int hv(int k) { return bw_on ? bw_inv[k] : have[k]; }     /* what the window shows: the body's things or ours */
static int wx, wy, ww, wh, tb, cbx, cby;                    /* the window, the title bar height, the close button */
static int gx, gy, gw, gh, rowsn, sl, pcell;                /* the slot grid: its area, rows, slot size, icon cell */
static int dx, dy, dw, dh;                                  /* the detail area (the selected item) */

void item_ui_resize(int w, int h, int right_edge, int top, int width) {
    W = w; H = h;
    float u = (w < h ? w : h) / 360.0f; uu = u;
    bt = (int)(1.5f * u); if (bt < 1) bt = 1;
    pad = (int)(4 * u); if (pad < 2) pad = 2;
    qs = (int)(2 * u); if (qs < 1) qs = 1;                  /* one step of the little ornaments */
    uw = width; ux = right_edge - width; uy = top;
    bag = width / 3; if (bag < 24) bag = 24;
    icell = (bag - 2 * bt - 2 * pad) / ITEM_ICON_CELLS; if (icell < 1) icell = 1;
}

void item_ui_init(int w, int h, int right_edge, int top, int width) {
    item_ui_resize(w, h, right_edge, top, width);
    ui_open = ui_down = 0; ui_p = ui_idle = 0; ui_sel = -1;
}

static int owned(void) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) n += have[k] > 0 ? 1 : 0; return n; }
static int owned_w(void) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) n += hv(k) > 0 ? 1 : 0; return n; }
static int shown(void) { return owned() - (fly_t < INV_FLY_T ? 1 : 0); }                 /* the badge does not count what is still in the air */
static int fcell(const char *name, int room) {              /* the biggest letter size (up to 3) that still fits `room` */
    int c = (int)(2.0f * (W < H ? W : H) / 360.0f); if (c < 1) c = 1; if (c > 3) c = 3;
    while (c > 1 && font_width(name, c) > room) c--;
    return c;
}
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
static int sel_ok(void) { return ui_sel >= 0 && ui_sel < ITEM_COUNT && hv(ui_sel) > 0; }
static int kind_at_slot(int i) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (hv(k) > 0) { if (n == i) return k; n++; } return -1; }
static int slot_of_kind(int kind) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (hv(k) > 0) { if (k == kind) return n; n++; } return -1; }
static int in_rect(int x, int y, int rx, int ry, int rw, int rh) { return x >= rx && y >= ry && x < rx + rw && y < ry + rh; }

/* the thing in hand: a medium thing carried in the hands first, else the one set as in hand (item_equip) */
int item_equipped(void) { return held >= 0 && held < ITEM_COUNT && have[held] > 0 ? held : -1; }
void item_equip(int kind) { held = kind >= 0 && kind < ITEM_COUNT && have[kind] > 0 ? kind : -1; }
static int hand_kind(void) {
    for (int k = 0; k < ITEM_COUNT; k++) if (have[k] > 0 && item_size(k) == SIZE_MEDIUM) return k;
    return item_equipped();
}

/* story chunk 18: THE LOOT WINDOW, chest style. a body's window shows two grids, the body's pockets and ours, and a strip under them with the name and the text of the
 * selected thing and the buttons. a tap selects a stack; a second tap on a selected stack of the body's takes it (or the TAKE button); TAKE ALL moves everything.
 * the story moves the things (body_cb) and shows the window again, so both grids are always read from bw_inv and have. (the bag window below is untouched) */
static int chest_side;                                          /* which grid holds the selection: 0 = the body's, 1 = ours */
static int cg_x[2], cg_y[2], cg_w[2], cg_h[2], cg_rows[2], cg_hh, csl, cpc;      /* the two grids: their areas, rows, the header height, one slot size, icon cell */
static int st_x, st_y, st_w, st_nameh, st_bh;                   /* the strip under them: its area, the name line, the button height */
static int cnt(int side, int k) { return side ? have[k] : bw_inv[k]; }
static int kind_on(int side, int i) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) if (cnt(side, k) > 0) { if (n == i) return k; n++; } return -1; }
static int owned_on(int side) { int n = 0; for (int k = 0; k < ITEM_COUNT; k++) n += cnt(side, k) > 0 ? 1 : 0; return n; }
static int chest_sel_ok(void) { return ui_sel >= 0 && ui_sel < ITEM_COUNT && cnt(chest_side, ui_sel) > 0; }
static int cs_off(int s) { int used = INV_COLS * csl + (INV_COLS + 1) * pad, o = (cg_w[s] - used) / 2; return o > 0 ? o : 0; }
static int cs_x(int s, int i) { return cg_x[s] + cs_off(s) + pad + (i % INV_COLS) * (csl + pad); }
static int cs_y(int s, int i) { return cg_y[s] + cg_hh + pad + (i / INV_COLS) * (csl + pad); }
static void chest_btn(int i, int *x, int *y, int *w, int *h) { *w = (st_w - pad) / 2; *h = st_bh; *x = st_x + i * (*w + pad); *y = st_y + st_nameh + 2 * (font_height(dcell()) + qs) + pad; }
static void chest_layout(int ix, int iy, int iw, int ih) {
    cg_hh = font_height(dcell()) + pad;
    st_bh = font_height(fcell("TAKE ALL", iw / 2)) + 2 * pad;
    st_nameh = font_height(fcell("MMMMMMMMMMMM", iw)) + qs;
    int sh = st_nameh + 2 * (font_height(dcell()) + qs) + pad + st_bh;
    st_x = ix; st_w = iw; st_y = iy + ih - sh;
    int ga = st_y - pad - iy;                                   /* the room for the grids */
    int wide = iw > ih * 13 / 10;                               /* a wide, short screen: the grids side by side */
    for (int s = 0; s < 2; s++) {
        int n = owned_on(s); cg_rows[s] = (n + INV_COLS - 1) / INV_COLS; if (cg_rows[s] < 2) cg_rows[s] = 2;
        if (wide) { cg_w[s] = (iw - pad) / 2; cg_h[s] = ga; cg_x[s] = ix + s * (cg_w[s] + pad); cg_y[s] = iy; }
        else      { cg_w[s] = iw; cg_h[s] = (ga - pad) / 2; cg_x[s] = ix; cg_y[s] = iy + s * (cg_h[s] + pad); }
    }
    int rm = cg_rows[0] > cg_rows[1] ? cg_rows[0] : cg_rows[1];
    csl = (cg_w[0] - (INV_COLS + 1) * pad) / INV_COLS;
    int sv = (cg_h[0] - cg_hh - (rm + 1) * pad) / rm; if (sv < csl) csl = sv;
    if (csl < 14) csl = 14;
    cpc = (csl - 2 * bt - pad) / ITEM_ICON_CELLS; if (cpc < 1) cpc = 1;
}
static void chest_tap(int x, int y) {
    for (int s = 0; s < 2; s++)
        for (int i = 0; i < cg_rows[s] * INV_COLS; i++) {
            int kk = kind_on(s, i), sx = cs_x(s, i), sy = cs_y(s, i);
            if (kk < 0 || !in_rect(x, y, sx - pad / 2, sy - pad / 2, csl + pad, csl + pad)) continue;
            if (s == 0 && chest_side == 0 && ui_sel == kk) { if (body_cb) body_cb(bw_vessel, kk); return; }     /* a second tap on the body's stack takes it */
            chest_side = s; ui_sel = kk; return;
        }
    for (int i = 0; i < 2; i++) {
        int rx, ry, rw, rh; chest_btn(i, &rx, &ry, &rw, &rh);
        if (!in_rect(x, y, rx, ry, rw, rh)) continue;
        if (i == 0 && chest_side == 0 && chest_sel_ok()) { if (body_cb) body_cb(bw_vessel, ui_sel); }
        else if (i == 1 && owned_on(0)) { if (body_cb) body_cb(bw_vessel, -1); }
        return;
    }
}

/* chunk 18b: DRAG. a finger down on a stack and moved a little picks it up (a floating icon follows it); lifted over the other grid it moves there (body to us: the take,
 * body_cb). a finger that does not move is still a tap (chest_tap). 0 = nothing, 1 = pressed on a stack, 2 = dragging it */
static int drag_on, drag_side, drag_kind, drag_x0, drag_y0, drag_x, drag_y;
static int chest_slot_at(int x, int y, int *side, int *kind) {
    for (int s = 0; s < 2; s++)
        for (int i = 0; i < cg_rows[s] * INV_COLS; i++) {
            int kk = kind_on(s, i);
            if (kk < 0 || !in_rect(x, y, cs_x(s, i) - pad / 2, cs_y(s, i) - pad / 2, csl + pad, csl + pad)) continue;
            *side = s; *kind = kk; return 1;
        }
    return 0;
}
static void drag_press(int x, int y) { drag_on = 0; if (bw_on && chest_slot_at(x, y, &drag_side, &drag_kind)) { drag_on = 1; drag_x0 = drag_x = x; drag_y0 = drag_y = y; } }
static void drag_move(int x, int y) {
    if (!drag_on) return;
    drag_x = x; drag_y = y;
    int th = (int)(8 * uu); if (th < 6) th = 6;
    if (drag_on == 1 && (x - drag_x0) * (x - drag_x0) + (y - drag_y0) * (y - drag_y0) > th * th) { drag_on = 2; chest_side = drag_side; ui_sel = drag_kind; }
}
static int drag_end(int x, int y) {                             /* 1 = it was a drag (the touch is used up), 0 = it was a tap */
    int was = drag_on; drag_on = 0;
    if (was != 2) return 0;
    int tgt = in_rect(x, y, cg_x[0], cg_y[0], cg_w[0], cg_h[0]) ? 0 : in_rect(x, y, cg_x[1], cg_y[1], cg_w[1], cg_h[1]) ? 1 : -1;
    if (drag_side == 0 && tgt == 1 && body_cb) body_cb(bw_vessel, drag_kind);       /* the body's stack dropped on our grid: taken */
    return 1;
}

/* where everything is: worked out from the screen, the pockets and the selection every time it is needed (cheap) */
static void win_layout(void) {
    int m = (int)(6 * uu); if (m < 4) m = 4;
    int top = (int)(48 * uu); if (top < 24) top = 24;           /* clear of the top buttons */
    wx = m; wy = top; ww = W - 2 * m; wh = H - top - m; if (wh < 60) wh = 60;
    int in = bt + pad;
    tb = font_height(fcell("INVENTORY", ww / 2)) + 2 * pad;
    cbx = wx + ww - in - tb; cby = wy + in;
    int ix = wx + in, iy = wy + in + tb + pad, iw = ww - 2 * in, ih = wy + wh - in - iy;
    if (bw_on) { chest_layout(ix, iy, iw, ih); return; }        /* chunk 18: a body's window has its own layout */
    int wide = ww > wh * 13 / 10;                               /* a wide, short screen: the grid left, the item right */
    int n = owned_w();
    rowsn = (n + INV_COLS - 1) / INV_COLS; if (rowsn < 3) rowsn = 3;
    gx = ix; gy = iy;
    if (wide) { gw = iw * 45 / 100; gh = ih; dx = ix + gw + pad; dy = iy; dw = iw - gw - pad; dh = ih; }
    else      { gw = iw; gh = ih * 42 / 100; dx = ix; dy = iy + gh + pad; dw = iw; dh = ih - gh - pad; }
    sl = (gw - (INV_COLS + 1) * pad) / INV_COLS;
    int sh = (gh - (rowsn + 1) * pad) / rowsn; if (sh < sl) sl = sh;
    if (sl < 14) sl = 14;
    pcell = (sl - 2 * bt - pad) / ITEM_ICON_CELLS; if (pcell < 1) pcell = 1;
}
static int grid_off(void) { int used = INV_COLS * sl + (INV_COLS + 1) * pad, o = (gw - used) / 2; return o > 0 ? o : 0; }
static int slot_x(int i) { return gx + grid_off() + pad + (i % INV_COLS) * (sl + pad); }
static int slot_y(int i) { return gy + pad + (i / INV_COLS) * (sl + pad); }
static int grid_h(void) { int h = rowsn * (sl + pad) + pad; return h; }

static int bigc(void) {                                         /* the cell size of the big picture */
    int c = (int)(3 * uu); if (c < 2) c = 2;
    while (c > 1 && (ITEM_ICON_CELLS * c + 2 * bt + 2 * pad > dh / 3 || ITEM_ICON_CELLS * c + 2 * bt + 2 * pad > dw / 3)) c--;
    return c;
}
static int bigsz(void) { return ITEM_ICON_CELLS * bigc() + 2 * bt + 2 * pad; }
static int btn_cell(void) { return fcell("MMMMMMMM", dw - 2 * pad); }
static int act_h(void) { return font_height(btn_cell()) + 2 * pad; }
static int d_pitch(void) { return font_height(dcell()) + qs; }
static int desc_n(void) { char ln[DESC_LINES][64]; return sel_ok() && craft_a < 0 ? wrap(item_desc(ui_sel), dcell(), dw, ln) : 0; }
static int act_n(int out[4]) { return sel_ok() && craft_a < 0 && !bw_on ? item_actions(ui_sel, out) : 0; }
static int desc_top(void) { return dy + bigsz() + pad; }
static int btn_top(void) {
    if (craft_a >= 0) return desc_top() + font_height(btn_cell()) + pad;      /* under the hint WITH WHAT? */
    int nl = desc_n(); return desc_top() + (nl ? nl * d_pitch() + pad : 0);
}
static void btn_rect(int i, int *x, int *y, int *w, int *h) { *x = dx; *w = dw; *h = act_h(); *y = btn_top() + i * (*h + pad); }
static const char *equip_label(void) { return item_equipped() == ui_sel ? "PUT AWAY" : "IN HAND"; }

/* the two small buttons */
static void bag_rect(int *x, int *y, int *w, int *h) { *x = ux; *y = uy; *w = uw; *h = bag; }        /* B1: ONE button under the map: as wide as the map, as high as the buttons were */
static int hit_btn(int x, int y) {
    int a, b, c, d; bag_rect(&a, &b, &c, &d); return in_rect(x, y, a, b, c, d);
}

int item_ui_hit(int x, int y) { return ui_open || hit_btn(x, y); }
int item_ui_open(void) { return ui_open; }
float item_ui_progress(void) { return ui_open ? 1.0f : 0.0f; }
int item_ui_selected(void) { return ui_sel; }
int item_ui_flying(void) { return fly_t < INV_FLY_T; }
int item_ui_shown(void) { return shown(); }
int item_ui_slot_x(int kind) { int i = slot_of_kind(kind); if (i < 0) return -1; win_layout(); return slot_x(i) + sl / 2; }
int item_ui_slot_y(int kind) { int i = slot_of_kind(kind); if (i < 0) return -1; win_layout(); return slot_y(i) + sl / 2; }
int item_ui_action_x(int i) { win_layout(); int a[4], n = act_n(a); if (i < 0 || i >= n) return -1; int x, y, w, h; btn_rect(i, &x, &y, &w, &h); return x + w / 2; }
int item_ui_action_y(int i) { win_layout(); int a[4], n = act_n(a); if (i < 0 || i >= n) return -1; int x, y, w, h; btn_rect(i, &x, &y, &w, &h); return y + h / 2; }
int item_ui_bag_x(void) { return ux + uw / 2; }
int item_ui_bag_y(void) { return uy + bag / 2; }

static void set_open(int o) {
    if (o && !ui_open) {
        bw_on = 0;                                               /* opened from the bag: our own pockets */
        int k = item_equipped();
        ui_sel = k >= 0 ? k : (newest >= 0 && have[newest] > 0 ? newest : kind_at_slot(0));
        craft_a = -1;
    }
    if (!o) { craft_a = -1; bw_on = 0; }
    drag_on = 0;
    ui_open = o; ui_p = o ? 1.0f : 0.0f; ui_idle = 0;
}
void item_ui_close(void) { set_open(0); ui_down = 0; }
/* story chunk 17: open (or refresh, after a take) the window on a body's pockets */
void item_ui_body_open(unsigned vessel, const char *name, const unsigned char inv[ITEM_COUNT]) {
    int was = ui_open && bw_on;
    bw_on = 1; bw_vessel = vessel;
    snprintf(bw_name, sizeof bw_name, "%s", name ? name : "");
    for (char *c = bw_name; *c; c++) *c = (char)toupper((unsigned char)*c);
    memcpy(bw_inv, inv, ITEM_COUNT);
    drag_on = 0;
    if (!was) { craft_a = -1; ui_sel = -1; chest_side = 0; }
    ui_open = 1; ui_p = 1.0f; ui_idle = 0; ui_down = 0;
    if (!chest_sel_ok()) { chest_side = 0; ui_sel = kind_on(0, 0); }       /* the selection is gone (taken): the first thing of the body */
}

static void win_tap(int x, int y) {                              /* a tap that began and ended in the window */
    if (in_rect(x, y, cbx, cby, tb, tb)) { set_open(0); return; }
    if (bw_on) { chest_tap(x, y); return; }
    int acts[4], na = act_n(acts);
    int nb = craft_a >= 0 ? 1 : (bw_on ? (sel_ok() ? 2 : 0) : (sel_ok() ? na + 1 : 0));
    for (int i = 0; i < nb; i++) {
        int rx, ry, rw, rh; btn_rect(i, &rx, &ry, &rw, &rh);
        if (!in_rect(x, y, rx, ry, rw, rh)) continue;
        if (craft_a >= 0) { craft_a = -1; return; }              /* CANCEL */
        if (bw_on) { if (body_cb) body_cb(bw_vessel, i == 0 ? ui_sel : -1); return; }       /* TAKE / TAKE ALL: the story moves it, then shows the body again */
        if (i < na) {
            int k = ui_sel;
            if (acts[i] == ACT_COMBINE) { craft_a = k; return; }   /* chunk 10: now touch the second slot */
            set_open(0); if (action_cb) action_cb(k, acts[i]); return;
        }
        held = item_equipped() == ui_sel ? -1 : ui_sel; sfx_blip(0.5f); return;        /* IN HAND / PUT AWAY */
    }
    for (int i = 0; i < rowsn * INV_COLS; i++) {
        int sx = slot_x(i), sy = slot_y(i), kk = kind_at_slot(i);
        if (kk < 0 || !in_rect(x, y, sx - pad / 2, sy - pad / 2, sl + pad, sl + pad)) continue;
        if (craft_a >= 0) { int a0 = craft_a; craft_a = -1; set_open(0); if (craft_cb) craft_cb(a0, kk); }     /* the second half of a craft */
        else ui_sel = kk;
        return;
    }
}

int item_ui_touch(int a, int x, int y) {
    if (ui_open) {                                               /* the window takes every touch */
        win_layout();
        if (a == 0) { ui_down = in_rect(x, y, wx, wy, ww, wh) ? 2 : 3; if (ui_down == 2) drag_press(x, y); return 1; }
        if (a == 2) { drag_move(x, y); return 1; }
        if (a == 1) {
            int d = ui_down; ui_down = 0;
            if (drag_end(x, y)) return 1;                        /* a drag: dropped, not a tap */
            if (d == 3 && !in_rect(x, y, wx, wy, ww, wh)) set_open(0);       /* a tap outside closes it */
            else if (d == 2 && in_rect(x, y, wx, wy, ww, wh)) win_tap(x, y);
            return 1;
        }
        if (a == 3) { ui_down = 0; drag_on = 0; }
        return 1;
    }
    if (a == 0) { if (hit_btn(x, y)) { ui_down = 1; return 1; } return 0; }
    if (!ui_down) return 0;
    if (a == 1) { int d = ui_down; ui_down = 0; if (d == 1 && hit_btn(x, y)) set_open(1); return 1; }
    if (a == 3) ui_down = 0;
    return 1;                                                    /* a drag that began on a button stays its own */
}

static void box(SDL_Renderer *r, int x, int y, int w, int h, int br, int bg, int bb, int fr, int fg, int fb, int fa) {
    SDL_Rect o = { x, y, w, h }, in = { x + bt, y + bt, w - 2 * bt, h - 2 * bt };
    SDL_SetRenderDrawColor(r, br, bg, bb, 255); SDL_RenderFillRect(r, &o);
    SDL_SetRenderDrawColor(r, fr, fg, fb, fa); SDL_RenderFillRect(r, &in);
}
static void txt(SDL_Renderer *r, const char *s, int x, int y, int c, int R, int G, int B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255); font_draw(r, s, x, y, c);
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

/* the two buttons under the minimap */
void item_ui_draw(SDL_Renderer *r, float t) {
    (void)t;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int bounce = (new_t >= 0.0f && new_t < 0.2f) ? qs : 0;
    int hk = hand_kind();
    {   int bx, by, bw, bh; bag_rect(&bx, &by, &bw, &bh);                   /* the one button: the hand, or what is in it; it opens the inventory */
        int up = ui_open, ix_ = bx + (bw - ITEM_ICON_CELLS * icell) / 2, iy_ = by + (bh - ITEM_ICON_CELLS * icell) / 2 - bounce;
        if (up)          box(r, bx, by, bw, bh, 236, 190, 78, 6, 8, 20, 245);
        else if (hk >= 0) box(r, bx, by, bw, bh, 214, 170, 70, 6, 8, 20, 245);
        else             box(r, bx, by, bw, bh, 122, 130, 156, 6, 8, 20, 245);
        if (hk >= 0) item_draw_icon(r, hk, ix_, iy_, icell); else item_draw_hand_icon(r, ix_, iy_, icell);
        int cnt = shown();
        if (cnt > 0) {                                       /* the badge on the lower left corner */
            char num[16]; snprintf(num, sizeof num, "%d", cnt);
            int fc = fcell("00", bag / 2), tw = font_width(num, fc), th = font_height(fc);
            int bw2 = tw + 2 * pad, bh2 = th + pad, x0 = bx + qs, y0 = by + bh - bh2 + bh2 / 3 - bounce;
            int tick = new_t >= 0.0f && new_t < 0.3f;
            box(r, x0, y0, bw2, bh2, 236, 190, 78, tick ? 255 : 6, tick ? 238 : 8, tick ? 170 : 20, 255);
            SDL_SetRenderDrawColor(r, tick ? 6 : 255, tick ? 8 : 238, tick ? 20 : 170, 255);
            font_draw(r, num, x0 + (bw2 - tw) / 2, y0 + (bh2 - th) / 2, fc);
        }
    }
    if (fly_t < INV_FLY_T) draw_flying(r);
}

static void chest_draw(SDL_Renderer *r) {
    int dcc = dcell();
    for (int s = 0; s < 2; s++) {
        box(r, cg_x[s], cg_y[s], cg_w[s], cg_h[s], 40, 46, 70, 10, 12, 26, 255);
        const char *hd = s == 0 ? bw_name : "YOU";
        txt(r, hd, cg_x[s] + bt + pad, cg_y[s] + bt + qs, dcc, 178, 190, 222);
        for (int i = 0; i < cg_rows[s] * INV_COLS; i++) {
            int sx = cs_x(s, i), sy = cs_y(s, i), k = kind_on(s, i);
            if (k < 0) { box(r, sx, sy, csl, csl, 40, 46, 70, 10, 12, 26, 255); continue; }
            int br = 122, bg = 130, bb = 156;
            if (s == 1 && k == newest) { br = 236; bg = 190; bb = 78; }
            if (s == chest_side && k == ui_sel) { br = 206; bg = 214; bb = 232; }
            box(r, sx, sy, csl, csl, br, bg, bb, 20, 24, 46, 255);
            item_draw_icon(r, k, sx + (csl - ITEM_ICON_CELLS * cpc) / 2, sy + (csl - ITEM_ICON_CELLS * cpc) / 2, cpc);
            if (s == 1 && k == item_equipped()) { SDL_SetRenderDrawColor(r, 120, 230, 150, 255); SDL_Rect g = { sx + bt + qs / 2, sy + bt + qs / 2, 3 * qs, 3 * qs }; SDL_RenderFillRect(r, &g); }
            if (cnt(s, k) > 1) {
                char num[8]; snprintf(num, sizeof num, "%d", cnt(s, k));
                txt(r, num, sx + csl - bt - font_width(num, dcc) - qs, sy + csl - bt - font_height(dcc) - qs, dcc, 255, 238, 170);
            }
            if (drag_on == 2 && s == drag_side && k == drag_kind) { SDL_SetRenderDrawColor(r, 6, 8, 20, 170); SDL_Rect dm = { sx, sy, csl, csl }; SDL_RenderFillRect(r, &dm); }   /* the one in the air leaves its slot dim */
        }
    }
    if (drag_on == 2 && drag_side == 0) {                       /* where it can be dropped: our grid gets a green frame */
        SDL_SetRenderDrawColor(r, 120, 230, 150, 255);
        SDL_Rect f[4] = { { cg_x[1], cg_y[1], cg_w[1], bt }, { cg_x[1], cg_y[1] + cg_h[1] - bt, cg_w[1], bt }, { cg_x[1], cg_y[1], bt, cg_h[1] }, { cg_x[1] + cg_w[1] - bt, cg_y[1], bt, cg_h[1] } };
        for (int q = 0; q < 4; q++) SDL_RenderFillRect(r, &f[q]);
    }
    if (chest_sel_ok()) {                                       /* under the items: the name and the text of the selected one */
        const char *nm = item_name(ui_sel); int nc = fcell(nm, st_w);
        txt(r, nm, st_x, st_y, nc, 235, 240, 255);
        char ln[DESC_LINES][64]; int nl = wrap(item_desc(ui_sel), dcc, st_w, ln); if (nl > 2) nl = 2;
        for (int j = 0; j < nl; j++) txt(r, ln[j], st_x, st_y + st_nameh + j * (font_height(dcc) + qs), dcc, 178, 190, 222);
    } else txt(r, "TAP A SLOT", st_x, st_y, fcell("TAP A SLOT", st_w), 122, 130, 156);
    for (int j = 0; j < 2; j++) {
        int rx, ry, rw, rh; chest_btn(j, &rx, &ry, &rw, &rh);
        const char *an = j == 0 ? "TAKE" : "TAKE ALL";
        int on = j == 0 ? (chest_side == 0 && chest_sel_ok()) : owned_on(0) > 0, ac = fcell("TAKE ALL", rw - 2 * pad), aw = font_width(an, ac);
        if (on) box(r, rx, ry, rw, rh, 236, 190, 78, 40, 34, 20, 255); else box(r, rx, ry, rw, rh, 60, 66, 90, 20, 24, 40, 255);
        if (on) txt(r, an, rx + (rw - aw) / 2, ry + (rh - font_height(ac)) / 2, ac, 255, 238, 170);
        else    txt(r, an, rx + (rw - aw) / 2, ry + (rh - font_height(ac)) / 2, ac, 122, 130, 156);
    }
    if (drag_on == 2) {                                         /* the stack under the finger, over everything */
        int fx = drag_x - csl / 2, fy = drag_y - csl / 2;
        box(r, fx, fy, csl, csl, 206, 214, 232, 20, 24, 46, 235);
        item_draw_icon(r, drag_kind, fx + (csl - ITEM_ICON_CELLS * cpc) / 2, fy + (csl - ITEM_ICON_CELLS * cpc) / 2, cpc);
    }
}
/* the window itself: world.c draws it over the HUD, under the dialogs */
void item_ui_draw_window(SDL_Renderer *r) {
    if (!ui_open) return;
    win_layout();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 150); SDL_Rect all = { 0, 0, W, H }; SDL_RenderFillRect(r, &all);      /* the game, dimmed */
    box(r, wx, wy, ww, wh, 214, 170, 70, 6, 8, 20, 250);
    const char *ttl = bw_on ? "LOOT" : "INVENTORY";
    int tc = fcell(ttl, ww - tb - 4 * pad);
    txt(r, ttl, wx + bt + pad, wy + bt + pad + (tb - font_height(tc)) / 2, tc, 235, 240, 255);
    box(r, cbx, cby, tb, tb, 236, 190, 78, 40, 34, 20, 255);                  /* the close button: an X of little squares */
    {   int n = 6, st = qs, ox = cbx + (tb - n * st) / 2, oy = cby + (tb - n * st) / 2;
        SDL_SetRenderDrawColor(r, 255, 238, 170, 255);
        for (int k = 0; k < n; k++) {
            SDL_Rect p = { ox + k * st, oy + k * st, st, st }, q = { ox + (n - 1 - k) * st, oy + k * st, st, st };
            SDL_RenderFillRect(r, &p); SDL_RenderFillRect(r, &q);
        }
    }

    if (bw_on) { chest_draw(r); return; }                       /* chunk 18: the loot window is its own picture */

    /* the grid */
    box(r, gx, gy, gw, grid_h() < gh ? grid_h() : gh, 40, 46, 70, 10, 12, 26, 255);
    for (int i = 0; i < rowsn * INV_COLS; i++) {
        int sx = slot_x(i), sy = slot_y(i), k = kind_at_slot(i);
        if (k < 0) { box(r, sx, sy, sl, sl, 40, 46, 70, 10, 12, 26, 255); continue; }              /* an empty slot */
        int br = 122, bg = 130, bb = 156;
        if (!bw_on && k == newest) { br = 236; bg = 190; bb = 78; }
        if (k == ui_sel) { br = 206; bg = 214; bb = 232; if (!bw_on && k == newest) { br = 255; bg = 238; bb = 170; } }
        if (k == craft_a) { br = 255; bg = 130; bb = 150; }
        box(r, sx, sy, sl, sl, br, bg, bb, 20, 24, 46, 255);
        item_draw_icon(r, k, sx + (sl - ITEM_ICON_CELLS * pcell) / 2, sy + (sl - ITEM_ICON_CELLS * pcell) / 2, pcell);
        if (!bw_on && k == item_equipped()) { SDL_SetRenderDrawColor(r, 120, 230, 150, 255); SDL_Rect g = { sx + bt + qs / 2, sy + bt + qs / 2, 3 * qs, 3 * qs }; SDL_RenderFillRect(r, &g); }   /* in hand */
        if (hv(k) > 1) {                                     /* more than one of a kind: the number in the corner */
            char num[8]; snprintf(num, sizeof num, "%d", hv(k)); int cc = dcell();
            txt(r, num, sx + sl - bt - font_width(num, cc) - qs, sy + sl - bt - font_height(cc) - qs, cc, 255, 238, 170);
        }
    }

    /* the selected item */
    int dcc = dcell(), bc2 = btn_cell();
    if (!sel_ok()) {
        const char *none = owned_w() ? "TAP A SLOT" : "NOTHING HERE";
        txt(r, none, dx + pad, dy + pad, fcell(none, dw - 2 * pad), 122, 130, 156);
        return;
    }
    int bc = bigc(), bs = bigsz();
    box(r, dx, dy, bs, bs, 122, 130, 156, 20, 24, 46, 255);
    item_draw_icon(r, ui_sel, dx + (bs - ITEM_ICON_CELLS * bc) / 2, dy + (bs - ITEM_ICON_CELLS * bc) / 2, bc);
    if (craft_a >= 0) {                                      /* the craft pair: this one, a plus, and a slot waiting for the second */
        int pw = font_width("+", bc2), x1 = dx + bs + pad;
        txt(r, "+", x1, dy + (bs - font_height(bc2)) / 2, bc2, 235, 240, 255);
        int x2 = x1 + pw + pad;
        box(r, x2, dy, bs, bs, 255, 130, 150, 10, 12, 26, 255);
        int qc = fcell("?", bs); txt(r, "?", x2 + (bs - font_width("?", qc)) / 2, dy + (bs - font_height(qc)) / 2, qc, 255, 130, 150);
        txt(r, "WITH WHAT?", dx, desc_top(), bc2, 255, 190, 200);
    } else {
        const char *nm = item_name(ui_sel); int nc = fcell(nm, dw - bs - pad);
        txt(r, nm, dx + bs + pad, dy, nc, 235, 240, 255);
        if (!bw_on && item_equipped() == ui_sel) txt(r, "IN HAND", dx + bs + pad, dy + font_height(nc) + qs, dcc, 120, 230, 150);
        char ln[DESC_LINES][64]; int nl = wrap(item_desc(ui_sel), dcc, dw, ln);
        for (int j = 0; j < nl; j++) txt(r, ln[j], dx, desc_top() + j * d_pitch(), dcc, 178, 190, 222);
    }
    int acts[4], na = act_n(acts), nb = craft_a >= 0 ? 1 : (bw_on ? 2 : na + 1);
    for (int j = 0; j < nb; j++) {                           /* one button per action, then IN HAND, or just CANCEL while crafting */
        int rx, ry, rw, rh; btn_rect(j, &rx, &ry, &rw, &rh);
        const char *an = craft_a >= 0 ? "CANCEL" : (bw_on ? (j == 0 ? "TAKE" : "TAKE ALL") : (j < na ? item_action_name(acts[j]) : equip_label()));
        int eq = craft_a < 0 && !bw_on && j >= na, ac = fcell(an, rw - 2 * pad), aw = font_width(an, ac);
        if (eq) box(r, rx, ry, rw, rh, 120, 230, 150, 14, 40, 30, 255);
        else    box(r, rx, ry, rw, rh, 236, 190, 78, 40, 34, 20, 255);
        if (eq) txt(r, an, rx + (rw - aw) / 2, ry + (rh - font_height(ac)) / 2, ac, 200, 255, 215);
        else    txt(r, an, rx + (rw - aw) / 2, ry + (rh - font_height(ac)) / 2, ac, 255, 238, 170);
    }
}
