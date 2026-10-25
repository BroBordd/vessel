/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * ITEMS AND THE INVENTORY: a thing that lies on the map until the player walks over it, and the pockets it goes into.
 *
 *  - story.c puts an item on the map (world_place_item). chunk 68: nothing is picked up by walking any more: within ITEM_REACH_R tiles the HAND
 *    button shows (world.c) and a TAP on it takes the item (item_grab): it leaves the map and goes into the inventory. one item lies on a map at a
 *    time (the hammer is the only one so far). not solid.
 *  - the inventory (chunk 70) is one small bag button right under the minimap with a badge counting what is in the pockets (nothing shows while they are
 *    empty). a tap and a panel eases out under it: framed square slots, the newest in gold, the name of the slot you touched last. it folds back by itself
 *    after a few seconds, on a tap outside or on the bag. a new pick-up flies from the ground to the bag, then the badge ticks and the slot flashes.
 *  - item_draw_icon draws the 12x12 picture of an item anywhere: the slot, the panel, the interact button.
 *  - everything here is engine: no story text. the inventory survives map changes; story.c empties it with a new life. */
#ifndef ITEM_H
#define ITEM_H
#include <SDL2/SDL.h>

enum { ITEM_HAMMER, ITEM_ORB, ITEM_PAPER, ITEM_KNIFE, ITEM_ROCK,
       ITEM_ARM, ITEM_LEG, ITEM_HEAD, ITEM_RIBS, ITEM_HEART, ITEM_LIVER, ITEM_INTESTINE, ITEM_SPINE, ITEM_FINGERS, ITEM_EYES, ITEM_TEETH, ITEM_SKULL, ITEM_BONE,   /* story chunk 9: what a body gives */
       ITEM_TIED_ROCK, ITEM_LONG_ROPE, ITEM_SPINE_HOOK, ITEM_CLUB, ITEM_LONG_REACH, ITEM_SKULL_STRING, ITEM_FINGER_NECKLACE, ITEM_TOOTH_NECKLACE,
       ITEM_WATCHING_ROCK, ITEM_RIB_CAGE, ITEM_HEART_STRING, ITEM_DULL_KNIFE,   /* story chunk 10: what the recipes make */
       ITEM_COUNT };      /* story chunk 8: the small rock (thrown) */      /* chunk 5 of the restarted story: the paper, Aonia's note (taking it reads it, story.c) */      /* chunk 39: the orb (it is never on the ground: it goes straight into the pockets) */
/* story chunk 7: size classes (docs/story/vessel.md section 4). SMALL goes in the pockets; MEDIUM is carried in the hands, one at a time, and slows the player a
 * little (CARRY_SPEED_MEDIUM, times the walking speed); BIG cannot be picked up at all, only dragged (a whole body, a torso, the big rock: props.h) and slows the
 * player a lot. nothing medium exists before chunk 9 (the parts that come off a body), so the carrying itself is built there; the table and the numbers are here */
typedef enum { SIZE_SMALL, SIZE_MEDIUM, SIZE_BIG } ItemSize;
#define CARRY_SPEED_MEDIUM 0.88f
int item_size(int kind);                    /* the size class of an item kind (SIZE_SMALL for all four so far) */
#define ITEM_ICON_CELLS 12                  /* an icon is 12 x 12 cells */
#define ITEM_REACH_R    1.3f                /* tiles: this close and the hand button shows (chunk 68; it was the walk-over pick-up radius) */

const char *item_name(int kind);            /* "HAMMER", "ORB" (capitals: the pixel font) */
/* story chunk 8: WHAT AN ITEM IS AND WHAT IT CAN DO. the open inventory shows, under the name of the slot you touched, a short description and one button per
 * action. a tap on a button folds the panel and calls the action callback (story.c decides what happens: the knife stabs a body or ourselves, the rock flies). */
typedef enum { ACT_NONE = 0, ACT_STAB, ACT_STAB_SELF, ACT_THROW, ACT_CUT_FINGERS, ACT_CUT_BONE, ACT_CUT_EYES, ACT_CUT_TEETH, ACT_CLEAN, ACT_DROP, ACT_COMBINE, ACT_SWING, ACT_SMASH, ACT_WEAR } ItemAction;
const char *item_desc(int kind);                        /* a short text (letters, digits, space and , . ! ? ' -) */
int         item_actions(int kind, int out[4]);         /* the actions of an item, 0 to 4 of them. returns how many */
const char *item_action_name(int act);                  /* the button's words (capitals) */
void        item_set_action_cb(void (*cb)(int kind, int act));
/* story chunk 10: CRAFTING. COMBINE on an item in a recipe, then a tap on a second slot (the same kind needs two): the craft callback runs with both kinds
 * (story.c asks item_recipe, takes both away and gives the result). the recipes of section 7 live in item.c */
int         item_recipe(int a, int b);                  /* what a + b makes (either order), -1 when nothing */
void        item_set_craft_cb(void (*cb)(int a, int b));
int         item_wear_toggle(int kind);                 /* wearable things: put on / take off. returns 1 when worn now */
int         item_worn(int kind);
int         item_ui_action_x(int i);                    /* (tests) the centre of the i-th action button of the selected item (-1 none) */
int         item_ui_action_y(int i);

/* world hooks */
void  item_reset(int pixel_scale, int tile_size);       /* a new map: whatever lay on the old one is gone (the inventory stays) */
void  item_place(int kind, int tile_x, int tile_y);     /* it lies there */
/* chunk 32: it FALLS there (out of someone's hand): drops from a little above, lands with a thunk and a bounce, a glint. it cannot be
 * picked up until it has landed. item_dropping() is 1 until then (tests). */
#define ITEM_DROP_T 0.45f
void  item_drop(int kind, int tile_x, int tile_y);
int   item_dropping(void);
/* chunk 33: a LOCKED kind cannot be taken. chunk 68: pressing the hand next to it does not take it: the callback set with item_set_blocked_cb runs, once per
 * press ("not like this"; walking close does nothing). item_reset (a new map) drops the locks and the callback. */
void  item_set_locked(int kind, int on);
int   item_locked(int kind);
void  item_set_blocked_cb(void (*cb)(void));
int   item_blocked_count(void);                         /* tests: how many presses were turned away */
int   item_ground_count(void);                          /* story chunk 8: how many things lie on this map */
int   item_ground_get(int i, int *kind, int *tile_x, int *tile_y);   /* the i-th of them (0 when i is out of range): the story keeps them across a death, the map is made again */
int   item_exists(void);                                /* something lies on this map */
void  item_tile(float *tx, float *ty);                  /* where, in tiles (minimap) */
int   item_has(int kind);
void  item_give(int kind);                              /* straight into the pockets */
void  item_take(int kind);                              /* out of the pockets (chunk 43: the orb goes to Dea) */
void  item_pack(unsigned char out[ITEM_COUNT]);          /* story chunk 6: what the pockets hold now (1 per kind), to be kept with a body */
void  item_unpack(const unsigned char in[ITEM_COUNT]);   /* ... and put back (a body's things in the pockets) */
int   item_count(int kind);                             /* story chunk 9: how many of that kind the pockets hold (a kind can be held more than once now) */
int   item_medium_held(void);                           /* story chunk 9: a MEDIUM thing is in the hands (one at a time; it slows us: CARRY_SPEED_MEDIUM) */
void  item_set_full_cb(void (*cb)(void));               /* the hands are full and the player reached for another medium thing */
void  item_clear(void);                                 /* empty pockets (a new body) */
/* every frame while playing: the drop animation and the pocket pulse run (nothing is picked up here any more) */
void  item_update(float dt);
int   item_in_reach(float player_x, float player_y);    /* chunk 68: an item lies within ITEM_REACH_R tiles of the feet (world px) and has landed: the hand button shows */
int   item_grab(float player_x, float player_y);        /* chunk 68: the tap on the hand. takes it and returns the kind; -1 when nothing is in reach; a locked kind is not taken (the blocked callback runs) */
void  item_draw_ground(SDL_Renderer *r, int cam_x, int cam_y, float t);

/* an icon, top-left at (x, y), each cell `cell` px. */
void  item_draw_icon(SDL_Renderer *r, int kind, int x, int y, int cell);
/* chunk 32: the magnifying glass (12 x 12, same style) on the interact button next to a body that can be searched (chunks 32, 39). not an item. */
void  item_draw_search_icon(SDL_Renderer *r, int x, int y, int cell);
/* chunk 68: an open hand (12 x 12) on the hand button. not an item. */
void  item_draw_hand_icon(SDL_Renderer *r, int x, int y, int cell);

/* chunk 70: the bag (12 x 12) on the collapsed inventory button. not an item. */
void  item_draw_bag_icon(SDL_Renderer *r, int x, int y, int cell);

/* the inventory widget. the box is `width` px wide (the minimap's width) and right-aligned to right_edge, starts at top */
void  item_ui_resize(int w, int h, int right_edge, int top, int width);   /* the screen changed shape: same arguments as item_ui_init (after minimap_resize); the bag and its panel keep their state */
void  item_ui_init(int w, int h, int right_edge, int top, int width);
int   item_ui_touch(int action, int x, int y);          /* Android MotionEvent action. 1 = this touch is the inventory's (do not pass it on) */
void  item_ui_draw(SDL_Renderer *r, float t);
int   item_ui_open(void);                               /* (tests) the panel is open (asked open: it may still be easing out) */
int   item_ui_hit(int x, int y);                        /* (tests) is (x, y) on the bag / the panel */
float item_ui_progress(void);                           /* (tests) how far the panel is out: 0 closed .. 1 fully out (it eases both ways) */
int   item_ui_selected(void);                           /* (tests) the kind whose name shows under the grid (-1 none) */
int   item_ui_flying(void);                             /* (tests) a picked-up icon is on its way to the bag */
int   item_ui_shown(void);                              /* (tests) the number on the badge (what is in the pockets, not counting what still flies) */
int   item_ui_slot_x(int kind);                         /* (tests) the centre of that kind's slot in the open panel (-1 if not owned) */
int   item_ui_slot_y(int kind);
int   item_ui_bag_x(void);                              /* (tests) the centre of the bag button */
int   item_ui_bag_y(void);

#endif
