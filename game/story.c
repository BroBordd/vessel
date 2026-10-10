/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * THE SCRIPT. Every line of text, every character and every scripted event lives here.
 * Building blocks you can call from anywhere in this file:
 *
 *   dialog_play(lines, count, on_done)      show pages of text (centred window or face popup)
 *   mission_add("Text") / mission_complete(id)   the top-left checklist
 *   task_toast("Text")                      like mission_add, plus a coin ding and a "New task added" toast (toast.h)
 *   npc_add(&PERSON, tile_x, tile_y, on_talk)    put a character on the map
 *   thought_say("Text", seconds)            the brain button: what the vessel thinks, opens next to the music button (thought.h). seconds 0 = auto
 *   world_death_begin(on_ready)             the death camera: freeze, zoom 300 %, grey world, red player (world.h)
 *   thought_wrath(text, seconds)            Dia's voice takes over the brain button: red, shaking, her eye instead of a face (thought.h)
 *   think(text, card, scene, tag, ding)    the vessel ACQUIRES a thought: it joins the brain window, optionally pops the brain card + ding (see "thoughts" below)
 *   brainwin_drop_tag(tag)                  the vessel silently loses every thought of that tag
 *   world_set_controls_visible(0 or 1)      show / hide the analog stick + interact button
 *   story_after(seconds, fn)                run something later
 *   npc_set_facing(id, facing)              how an npc stands when nobody is near
 *   world_open_hole(x, y, on_enter)         cloud hole in the floor you can jump into
 *   world_place_shrine(x, y)                Dia's shrine prop on the map (blocks the player, inert)
 *   world_enable_shrine(on_done)            the shrine can be used: hold the interact button next to it (it needs the hammer) to break it, then on_done()
 *   world_fall_to_green(on_up)              jump, fall, land face-first, get up, then on_up()
 *   hud_set_person(&PERSON)                 change who the ID card shows. a new name flashes, dings + toasts
 *   hud_set_hp(hp, max)                     the HP bar on the ID card
 *   dialog_on_page(fn)                      run fn(page) as each page of the NEXT dialog_play begins
 *   dialog_on_highlight(fn)                 run fn(page, span) when a {highlighted} span of the NEXT dialog_play has been typed out
 *   convo_ask_questions(&P, &MIND, on_end)  "do you have any questions?" then a free typed chat (convo.h)
 *   convo_open(&P, &MIND, again, on_end)    the npc speaks first, the player may type or skip
 *   world_summon(on_done)                   the summoning: a column of golden light and a ring on the floor, the player not drawn until it ends (summon.h)
 *   limbo_run(thoughts, n, gap, on_done)    LIMBO (vessel 2): the soul thinks these lines one after the other on the space screen, then on_done()
 *   limbo_end()                             leave limbo: the soul's voice goes, the HUD and the world come back (game.c switches the screen)
 */
#include "story.h"
#include "dialog.h"
#include "missions.h"
#include "toast.h"
#include "thought.h"
#include "brainwin.h"
#include "npc.h"
#include "world.h"
#include "grave.h"
#include "item.h"
#include "audio.h"
#include "jukebox.h"
#include "heart.h"
#include "hud.h"
#include "convo.h"
#include "lang.h"
#include <string.h>

#define VESSEL_NAME  "Aonia"        /* vessel one. later: Doia, Tria, Ceathia ... (Dea plays on the word "one") */
#define VESSEL2_NAME "Doia"         /* vessel two: Dea plays on the word two */
#define VESSEL_LATIN "Vas"          /* what Dea calls us before the naming. "Vas" is Latin for vessel */

/* ---------- characters ---------- */
/*                      name           hair            skin             shirt           pants          boots        long hair  accessories */
/* the soul: what we are before we have a body (limbo, vessel 2). pale ghost-blue and white; only its face is shown (the
 * brain card while in limbo, via thought_set_voice), the halo is for a full body should it ever be drawn */
const Person SOUL = { "Soul", { 232, 240, 255 }, { 188, 226, 246 }, { 140, 186, 238 }, { 118, 160, 218 }, { 96, 130, 196 }, 0, ACC_HALO };
const Person VESSEL = { VESSEL_NAME,  { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
/* vessel two, Doia: long silver hair, green shirt, grey trousers, a little browner skin: clearly not Aonia (same body and proportions) */
const Person VESSEL2 = { VESSEL2_NAME, { 206, 210, 228 }, { 226, 172, 132 }, { 70, 160, 96 }, { 78, 78, 92 }, { 40, 30, 30 }, 1, 0 };
const Person VAS    = { VESSEL_LATIN, { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
const Person ALEX   = { "Alex",       { 236, 196, 84 }, { 244, 200, 164 }, { 60, 170, 170 }, { 70, 60, 110 }, { 40, 30, 30 }, 1, 0 };
/* "dea" is Latin for goddess. white shades, a halo, all in white and gold */
const Person DEA    = { "Dea",        { 252, 240, 190 }, { 252, 228, 206 }, { 255, 244, 200 }, { 250, 232, 170 }, { 226, 190, 90 }, 1,
                        ACC_SHADES | ACC_HALO };

/* ---------- minds: how each npc talks (see lang.h) ----------
 *                          name    style          mood warmth toxic patience(replies) */
static Persona DEA_MIND  = { "Dea",  STYLE_DIVINE,   0,   30,   65,   8 };     /* the goddess: toxic, condescending, "i do not care" */
static Persona ALEX_MIND = { "Alex", STYLE_STREET,  10,   70,    5,  12 };

/* ---------- timers ---------- */
#define MAX_TIMERS 8
static struct { float left; void (*fn)(void); } timers[MAX_TIMERS];

void story_after(float seconds, void (*fn)(void)) {
    for (int i = 0; i < MAX_TIMERS; i++)
        if (!timers[i].fn) { timers[i].left = seconds; timers[i].fn = fn; return; }
}

static void alex_watch(void);               /* the proximity triggers, defined with the scenes below */
static void dea_watch(void);
static void grave_watch(void);
static void hammer_watch(void);

void story_update(float dt) {
    alex_watch();
    dea_watch();
    grave_watch();
    hammer_watch();
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].fn) continue;
        timers[i].left -= dt;
        if (timers[i].left <= 0) {
            void (*fn)(void) = timers[i].fn;
            timers[i].fn = NULL;
            fn();
        }
    }
}

/* ---------- state ---------- */
static int mission_talk_dea = -1;
static int mission_find_orb = -1;           /* added by Dea's highlighted orb line (chunk 5) */
static int mission_ask_alex = -1;           /* added when the player first catches sight of Alex */
static int mission_pollute = -1;            /* added by Alex's highlighted line (chunk 9) */
static int alex_dealt;                      /* the scripted deal has been played */
static int shrine_fouled;                   /* the player has polluted Dia's shrine (chunk 10) */
static int dying;                           /* Dia's wrath has begun (chunk 11): no more thoughts of the player's own, no more walking */
static int alex_id = -1, alex_seen;         /* her npc id on the ground (-1 = not there), and "the player has spotted her" */
static int dea_spoken;
static int dea_id = -1, dea_seen;           /* her npc id in the sky, and "the player has sighted her" (chunk 11) */
static int dea_tx, dea_ty;                  /* where Dea stands (the hole opens a few tiles below her) */

/* ---------- thoughts (the brain window, brainwin.h) ----------
 * nothing is preloaded: the vessel acquires a thought when something happens, and loses it when it stops
 * being true. every thought has a TAG so a whole group can be dropped silently (leaving a map, a task done). */
enum { THOUGHT_CLOUDS = 1, THOUGHT_ORB, THOUGHT_GRASS, THOUGHT_ALEX, THOUGHT_COIN, THOUGHT_SHRINE, THOUGHT_AGAIN, THOUGHT_GRAVE, THOUGHT_HAMMER };

/* `text` goes in the window (up to ~90 letters). `card` is what pops out of the brain button (about 19
 * letters x 3 lines max; NULL = same as text, "" = no card). ding = the coin ding of acquiring something
 * (leave it off when a task toast is dinging at the same moment) */
static void think(const char *text, const char *card, BrainScene scene, int tag, int ding) {
    if (dying) return;                                      /* this head is not the vessel's any more */
    if (brainwin_acquire(text, scene, tag) < 0) return;
    if (ding) sfx_thought();                                /* the calm chime (the coin is for tasks) */
    if (!card) card = text;
    if (*card) thought_say(card, 0);
}

/* ---------- limbo: the soul thinks (vessel 2, chunk 6) ----------
 * limbo is the space screen (game.c ST_LIMBO): stars, the top buttons, nothing else. no map, no ID card, no mission list, no
 * minimap, no stick: game.c simply does not draw the world while limbo runs, so the HUD is hidden for as long as it lasts, and
 * limbo_end() gives it back. the story runs on its own little clock here (story_limbo_update), because the world, and with it
 * story_update and its timers, does not run in limbo.
 *
 * limbo_run(lines, n, gap, on_done): the voice becomes the soul's (SOUL, its pale face on the brain card). the first line is
 * said at once, each next one `gap` seconds after the one before, and on_done() runs `gap` seconds after the last (that is the
 * hold: the soul waits a moment before the story goes on). a limbo thought is only the card: no coin ding, and it is NOT
 * added to the brain window (a soul has no head to remember with yet). `lines` must stay alive (use static arrays).
 * on_done usually ends limbo with limbo_end(), or starts another limbo_run(). */
static struct {
    const char *const *lines; int n, i;     /* what to say, and the next one to say */
    float gap, left;                        /* seconds between lines, and until the next step */
    void (*on_done)(void);
    int running;                            /* a limbo_run is counting */
    int on;                                 /* we are in limbo (from the first limbo_run until limbo_end) */
    int ended;                              /* limbo_end() was called: game.c takes it once (story_limbo_take_end) */
} limbo;

void limbo_run(const char *const *thoughts, int n, float gap, void (*on_done)(void)) {
    limbo.lines = thoughts; limbo.n = thoughts ? n : 0; limbo.i = 0;
    limbo.gap = gap > 0 ? gap : 0; limbo.left = 0;
    limbo.on_done = on_done;
    limbo.running = 1; limbo.on = 1; limbo.ended = 0;
    thought_set_voice(&SOUL);
    if (limbo.n > 0) {                                      /* the first thought at once */
        thought_say(limbo.lines[0], 0);
        limbo.i = 1;
    }
    limbo.left = limbo.gap;
}

void limbo_end(void) {
    limbo.running = 0; limbo.on = 0; limbo.ended = 1;
    thought_set_voice(NULL);                                /* a body again: the ID card person speaks */
}

int story_limbo_active(void) { return limbo.on; }
int story_limbo_take_end(void) { int e = limbo.ended; limbo.ended = 0; return e; }

void story_limbo_update(float dt) {
    if (!limbo.running) return;
    limbo.left -= dt;
    if (limbo.left > 0) return;
    if (limbo.i < limbo.n) {                                /* the next thought */
        thought_say(limbo.lines[limbo.i++], 0);
        limbo.left += limbo.gap;
        return;
    }
    limbo.running = 0;                                      /* the last line has had its hold */
    if (limbo.on_done) { void (*fn)(void) = limbo.on_done; limbo.on_done = NULL; fn(); }
}

/* ---------- the beginning: "Where am I?" (vessel 2, chunk 7) ----------
 * PLAY -> the menu fades out into limbo (game.c) -> story_limbo_begin(). a beat of silence over the stars, the soul thinks
 * "Where am I?", a hold, and limbo ends: game.c fades the world in on the cloud island, starts the cloud music
 * and the summoning (world_summon, queued by story_start) runs. intro_done() follows it (see below).
 * the menu music keeps playing through limbo and fades out over the last LIMBO_FADE_T seconds. */
#define LIMBO_BEAT      1.0f                /* stars only, before the soul speaks */
#define LIMBO_MUSIC_FADE 3.0f               /* the menu music dies away over the dawn (it is about as long as the sky takes to brighten) */

/* the opening is a real popup, not a thought: the soul's face and name at the bottom, a tap on the arrow to go on (game.c runs the
 * dialog in limbo). the brain button stays away until the game starts (story_limbo_opening, game.c): it only comes in with the pause
 * button, when the world is running. after the line limbo ends and the world begins its dawn (world_begin_dawn): the sky brightens, the
 * cloud floor builds in, and then the summoning runs. */
static const DialogLine LIMBO_WHERE_DLG[] = { { &SOUL, "Where am I?" } };
static int limbo_stay;                      /* dev: stay in limbo for good (VESSEL_LIMBO=1) */
static int limbo_opening;                   /* the opening of the game is running: no brain button yet */

static void limbo_where_done(void) {
    limbo_opening = 0;
    jukebox_scene_fade(LIMBO_MUSIC_FADE);   /* the menu music goes as the sky comes */
    world_begin_dawn();                     /* the summoning is already queued (story_start): the dawn holds it back until the floor is built */
    limbo_end();
}
static void limbo_where_say(void) { dialog_play(LIMBO_WHERE_DLG, 1, limbo_stay ? NULL : limbo_where_done); }

int story_limbo_opening(void) { return limbo_opening; }

/* stay = 1: the dev entry (VESSEL_LIMBO=1), says the line and then stays in limbo forever */
void story_limbo_begin(int stay) {
    limbo_stay = stay;
    limbo_opening = 1;
    limbo_run(NULL, 0, LIMBO_BEAT, limbo_where_say);
}

/* ---------- scene 3: down on the ground ---------- */
/* nobody stands near the landing spot: the player lands alone in GRASSLANDS. Alex is placed far
 * away (landed() below); her scripted deal comes in chunk 9 (docs/story/vessel-1.md). */

/* what the player says out loud after the fall (face popup). the controls are hidden while a
 * dialog is open and come back by themselves when it ends (world.c) */
static const DialogLine LANDING[] = {
    { &VESSEL, "Ow. That hurts." },
    { &VESSEL, "She could at least have warned me." },
};

/* Alex stands far from where we land: between 26 and 40 tiles away, somewhere the player can really
 * walk to. the "far" is the point: it is a walk, and the first thought sends the player looking. */
#define ALEX_MIN_DIST   26
#define ALEX_MAX_DIST   40
#define ALEX_SIGHT      6.5f                /* tiles: about half a screen, so she is on screen when it fires */

/* the player has caught sight of Alex: a thought and the next task. safe to call twice */
static void alex_spotted(void) {
    if (alex_seen) return;
    alex_seen = 1;
    think("Maybe she has seen the orb.", NULL, BRAIN_SCENE_ORB, THOUGHT_ALEX, 0);    /* the task toast dings */
    mission_ask_alex = task_toast("Ask Alex");
}

/* the proximity trigger, run every frame: walking within sight of Alex (not while a dialog is open) */
static void alex_watch(void) {
    if (alex_id >= 0 && !alex_seen && !dialog_active() && world_dist_to_npc(alex_id) < ALEX_SIGHT)
        alex_spotted();
}

/* ---------- Alex's deal (chunk 9) ----------
 * she has the orb and will not hand it over for free: the player must pollute Dia's shrine. the page
 * holding the highlighted words is the last one. the moment they have been typed out, "Find the orb"
 * is ticked (we know where it is) and "Pollute Dia's shrine" coins in. then the free typed chat stays
 * available, as with Dea. */
static const DialogLine ALEX_DEAL[] = {
    { &VESSEL, "Excuse me. Have you seen a glowing orb around here?" },
    { &ALEX,   "A glowing orb? Sure. I have it right here." },
    { &VESSEL, "I just want the orb. Please give it back." },
    { &ALEX,   "Just the orb, huh? Finders keepers." },
    { &ALEX,   "But I am a fair person. Do one small thing for me, and it is yours." },
    { &VESSEL, "What kind of thing?" },
    { &ALEX,   "Dia has a shrine near here. She thinks she owns the whole place. Go {break the shrine}, then come back to me." },
};
#define ALEX_DEAL_COUNT  ((int)(sizeof ALEX_DEAL / sizeof ALEX_DEAL[0]))
#define ALEX_DEAL_PAGE   (ALEX_DEAL_COUNT - 1)

/* ---------- Dia's shrine (chunk 10) ----------
 * the prop is placed when we land (landed() below) and does nothing until Alex has asked for it. then holding
 * the interact button next to it pollutes it (shrine.c); the moment it is full this runs. */
#define SHRINE_MIN_DIST    16               /* tiles from the landing spot */
#define SHRINE_MAX_DIST    26
#define SHRINE_FROM_ALEX   18               /* and not on top of Alex: she is somewhere else entirely */

/* ---------- the hammer ----------
 * the shrine is broken, not just touched: the interact button only shows next to it for a player who carries the hammer (item.c).
 * the hammer lies somewhere across the map: HAMMER_MIN_DIST..HAMMER_MAX_DIST tiles from the landing and at least HAMMER_FROM_SHRINE
 * from the shrine, so it is a walk there and a walk back. picking it up gives a thought; standing at the switched-on shrine without it
 * gives a hint, once. if the map has no room for it the player simply starts with it (never a game that cannot be finished). */
#define HAMMER_MIN_DIST     12
#define HAMMER_MAX_DIST     30
#define HAMMER_FROM_SHRINE  14
#define HAMMER_HINT_NEAR    3.5f            /* tiles from the shrine */
static int hammer_hinted, hammer_said;

/* ---------- Dia's wrath (chunk 11) ----------
 * the shrine is polluted. a beat of quiet, then the goddess speaks in a plain popup, her normal face: "How dare you."
 * the player cannot move meanwhile. a short pause after the popup is tapped away, and the death begins (chunk 12). */
#define WRATH_BEAT        0.9f              /* seconds between the last drop of ooze and her voice */
#define WRATH_TO_DEATH    0.8f              /* after her line has been tapped away: a short pause, then the death */
static const DialogLine DEA_WRATH[] = { { &DEA, "How dare you." } };     /* a plain popup, her normal face (no red card any more) */

/* ---------- the death (chunks 12-15) ----------
 * chunk 12: the camera pushes in to 300 %, the world freezes and drains grey, the player goes red (world.c).
 * chunk 13 adds the sound, 14 the heart, 15 the words "Aonia has died." and the end hook story_vessel_died() (vessel 2 starts there;
 * until it exists the death picture stays and the pause button restarts). */
#define DEATH_BEEPS         3               /* "peep. peep. peep." ... */
#define DEATH_BEEP_GAP      0.85f           /* ... this far apart (the first one is at once) ... */
#define DEATH_FLAT_AT       (DEATH_BEEPS * DEATH_BEEP_GAP)   /* ... and where the fourth would be, the flatline starts and the heart bursts */
#define DEATH_FALL_AT       1.0f            /* after the flatline and the burst: a beat of stillness, then the vessel topples */
#define DEATH_TEXT_AT       1.5f            /* after the fall began (it lasts 1.1 s): the words fade in */
#define DEATH_END_HOLD      4.5f            /* the words stay this long, then the mission is over: story_vessel_died() */

/* the end hook of vessel 1's mission, chunk 12: after "Aonia has died." the world fades to black over DEATH_FADE_T, the cutscene is
 * put away behind it and the game goes to LIMBO (the space screen, game.c takes the request once: story_limbo_take_enter).
 * we remember how many lives have been lived and where this one ended (the grave comes later). chunks 13-15: the soul speaks, the
 * vessel is summoned again. STAND-IN until then: limbo just stays after the soul's line and its hold (no way back yet). */
#define DEATH_FADE_T        1.0f
static int vessel_dead;
static int lives;                           /* vessels lived and lost: 1 after Aonia */
static int death_tx, death_ty;              /* the tile where the last vessel died */
static int limbo_enter_req;

/* what the soul thinks on arriving in limbo after each death: the vessel table's death lines, one entry per life lived (lives - 1;
 * past the end the last entry is used). the first line is said at once, then the soul waits DEATH_LIMBO_HOLD seconds before the story
 * goes on (chunk 15: the vessel is summoned again). later vessels add their own entries here. */
static const DialogLine DEATH_LINES_1[] = { { &SOUL, "Uh. I died." } };      /* vessel 1, Aonia: a real popup from the soul, not a thought */
static const struct { const DialogLine *lines; int n; } DEATH_LINES[] = {
    { DEATH_LINES_1, 1 },
};
#define DEATH_LINES_COUNT ((int)(sizeof DEATH_LINES / sizeof DEATH_LINES[0]))
#define DEATH_LIMBO_HOLD    1.5f            /* after the popup is tapped away: the soul waits a moment */
static int death_hold_done;                 /* (tests) how many times the hold after a death has run out */

/* what the Grasslands looked like when vessel 1 died (chunk 14): the world forgets a map's npcs and props when it leaves it, the
 * story keeps them so that chunk 20 can put Alex and the shrine back exactly as they were. alex_dealt / alex_seen / shrine_fouled
 * are simply not reset. */
static struct { int alex_tx, alex_ty, alex_there; int shrine_tx, shrine_ty, shrine_there, shrine_polluted; } green;

/* the graves (chunks 21-22): every vessel that died on the Grasslands keeps a stone where it fell, with its own face on it. the world forgets
 * props when a map loads, so the story keeps the list (the tile where each one died, and who it was) and puts them all back on every
 * landing (landed_again). nothing here knows which vessel it is: world_player() at the moment of death is the Person on the stone. */
static struct { int tx, ty; const Person *who; } graves[GRAVE_MAX];
static int grave_n;
static void green_remember(void) {
    memset(&green, 0, sizeof green);
    if (alex_id >= 0) { float fx, fy; npc_tile(alex_id, &fx, &fy); green.alex_tx = (int)fx; green.alex_ty = (int)fy; green.alex_there = 1; }
    if (world_shrine_exists()) {
        float fx, fy; world_shrine_tile(&fx, &fy);
        green.shrine_tx = (int)fx; green.shrine_ty = (int)fy; green.shrine_there = 1; green.shrine_polluted = world_shrine_polluted();
    }
}

/* the next life begins (chunks 14-15): the last vessel's body and tasks are put away and we are back in the clouds with a plain
 * VAS card and an empty task list. THE HEAD IS NOT EMPTIED: the vessel keeps its memories always, whatever body it wears (story
 * change after chunk 22), so the brain window keeps every thought it had (the ones tied to the sky already went silently when it left). Dea is already there, looking down at us. the player's look is still the only one
 * there is (VESSEL; chunk 17 adds the second) so there is nothing to put back yet. the Grasslands story state is kept (green, above). */
static void on_talk_dea(int npc_id);                /* defined with the sky scene below */
static int scold_pending;                           /* the second summoning is over and she has not scolded us yet */
static void story_reset_for_respawn(void) {
    for (int i = 0; i < MAX_TIMERS; i++) timers[i].fn = NULL;
    missions_clear();                               /* (the brain window is NOT cleared: the soul remembers everything)  */
    mission_talk_dea = mission_find_orb = mission_ask_alex = mission_pollute = -1;
    dying = 0; vessel_dead = 0;
    world_clear_items();                            /* the new body has empty pockets */
    alex_id = -1;                                   /* off the map with the Grasslands (alex_dealt, alex_seen, shrine_fouled stay) */
    hud_reset(&VAS, 0);                             /* the ID card is plain VAS again, HP full */
    brainwin_clear();                               /* the new vessel starts with an empty head: none of vessel 1's thoughts */
    world_set_player(&VESSEL2);                     /* the new body is summoned already looking like the new character (the ID card stays VAS until the naming) */
    convo_set_player(&VAS);
    world_return_to_clouds();
    dea_tx = world_player_tile_x();                 /* Dea is already waiting 8 tiles above, looking down at us */
    dea_ty = world_player_tile_y() - 8;
    dea_id = npc_add(&DEA, dea_tx, dea_ty, on_talk_dea);
    npc_set_facing(dea_id, FACE_DOWN);
    dea_seen = 0;                                   /* walking within sight of her coins in "Talk to Goddess" again */
    dea_spoken = 1;
    scold_pending = 0;
}

static void open_the_hole(void);                    /* defined with the sky scene below */

/* ---------- Dea scolds us, number two (vessel 2, chunk 16) ----------
 * the second summoning is over: Dea has been standing there all along, looking at us, and she speaks FIRST (no walking, no sighting
 * task, no thought). the words are all hers and to VAS (we are plain VAS again). the last line gives us our new number: the naming
 * itself (ID card, player look) is chunk 18 (scold_page), and her questions chunk 19, so scold_done() is the hook those will take over.
 * scold_done opens her questions (chunk 19), questions_done the hole (chunk 20). */
static const DialogLine DEA_SCOLD[] = {
    { &VAS, "Why did I die?" },
    { &DEA, "Because I got angry. You broke my shrine." },
    { &DEA, "For a girl and a ball. My ball. How stupid can one vessel be?" },
    { &DEA, "I do not care that it hurt. There is a reason you are replaceable." },
    { &DEA, "Fine. You are number two now. " VESSEL2_NAME ". Try not to make me remember it." },
};
#define DEA_SCOLD_COUNT ((int)(sizeof DEA_SCOLD / sizeof DEA_SCOLD[0]))

/* chunk 19: after the naming Dea asks if we have any questions, meaner than the first time: a free chat (typed, or BYE to skip). her
 * mood starts below zero, so the answers are colder than before. when the chat is over, questions_done runs: chunk 20, the controls come
 * back and the hole opens in front of her; pressing its button jumps and falls to the Grasslands (on_enter_hole -> landed_again). */
#define DEA_RESPAWN_MOOD  (-30)
static void questions_done(void) {
    world_set_controls_visible(1);
    open_the_hole();
}
static void scold_done(void) {
    if (DEA_MIND.mood > DEA_RESPAWN_MOOD) DEA_MIND.mood = DEA_RESPAWN_MOOD;     /* meaner than at the first meeting (mood 0) */
    convo_ask_line(&DEA, &DEA_MIND, "Well? Do you have any questions? Be quick about it.", questions_done);
}

/* the last page is the naming (chunk 18): the moment her line "You are number two now. Doia." begins, the ID card flashes (coin
 * ding, "YOU ARE NOW DOIA"), our face in every dialog and thought is Doia's and the player's body changes to the new look. */
#define DEA_SCOLD_NAMING_PAGE (DEA_SCOLD_COUNT - 1)
static void scold_page(int page) {
    if (page != DEA_SCOLD_NAMING_PAGE) return;
    hud_set_person(&VESSEL2);
    convo_set_player(&VESSEL2);
    world_set_player(&VESSEL2);
}
static void dea_scold(void) {
    mission_complete(mission_talk_dea);
    dialog_on_page(scold_page);
    dialog_play(DEA_SCOLD, DEA_SCOLD_COUNT, scold_done);
}
static void respawn_summoned(void) {
    world_set_controls_visible(1);                              /* no auto talk: we walk to her and talk to her ourselves */
    scold_pending = 1;
}

/* the hold after the soul's line is over: the next life is made ready behind the scenes (chunk 14), limbo ends, game.c shows the
 * world (it fades in from black, the cloud music starts again) and the golden summoning runs again (chunks 8-9) */
static void death_limbo_hold_over(void) {
    death_hold_done++;
    story_reset_for_respawn();
    world_summon(respawn_summoned);
    limbo_end();
}
static void death_say_done(void) { limbo_run(NULL, 0, DEATH_LIMBO_HOLD, death_limbo_hold_over); }
static void death_to_limbo(void) {
    world_death_cancel();                   /* behind the black: zoom, colour, heart and words put away */
    int i = lives - 1; if (i < 0) i = 0; if (i >= DEATH_LINES_COUNT) i = DEATH_LINES_COUNT - 1;
    limbo_run(NULL, 0, 1000.0f, NULL);                      /* limbo on, the soul's voice (the clock waits: the popup decides when it goes on) */
    dialog_play(DEATH_LINES[i].lines, DEATH_LINES[i].n, death_say_done);   /* the soul speaks (a popup in limbo, game.c drives it), then the hold */
    limbo_enter_req = 1;
}
static void story_vessel_died(void) {
    vessel_dead = 1;
    lives++;
    green_remember();
    death_tx = world_player_tile_x(); death_ty = world_player_tile_y();
    if (grave_n < GRAVE_MAX) { graves[grave_n].tx = death_tx; graves[grave_n].ty = death_ty; graves[grave_n].who = world_player(); grave_n++; }
    world_fade_to_black(DEATH_FADE_T, death_to_limbo);
}
int story_limbo_take_enter(void) { int e = limbo_enter_req; limbo_enter_req = 0; return e; }

static void death_text(void) {
    world_death_text(VESSEL_NAME " has died.");
    story_after(DEATH_END_HOLD, story_vessel_died);
}
static int  beeps_left;
static void death_fall(void) { world_death_fall(); story_after(DEATH_TEXT_AT, death_text); }
static void death_flat(void) { sfx_flatline(); heart_burst(); story_after(DEATH_FALL_AT, death_fall); }  /* the long tone and the burst, together; then the words */
static void death_beep(void) {
    sfx_beep(); heart_pulse();
    if (--beeps_left > 0) story_after(DEATH_BEEP_GAP, death_beep);
    else story_after(DEATH_BEEP_GAP, death_flat);
}
static void death_ready(void) {
    /* everything has arrived (zoomed, grey, red): the monitor starts and the heart appears over the chest */
    beeps_left = DEATH_BEEPS;
    heart_show();                                           /* over the chest, thumping with every peep */
    death_beep();
}

static void death_begin(void) { world_death_begin(death_ready); }

static void wrath_done(void) { story_after(WRATH_TO_DEATH, death_begin); }
static void dia_wrath(void) {
    dialog_play(DEA_WRATH, (int)(sizeof DEA_WRATH / sizeof DEA_WRATH[0]), wrath_done);   /* a simple talk with her */
}

static void shrine_done(void) {
    shrine_fouled = 1;
    dying = 1;
    jukebox_scene_fade(0.15f);                              /* the music dies the moment it is fouled (map music only: a custom track keeps playing) */
    brainwin_drop_tag(THOUGHT_SHRINE);                      /* "I doubt that ends well" has come true, or is about to */
    brainwin_drop_tag(THOUGHT_HAMMER);
    mission_complete(mission_pollute);
    world_set_controls_visible(0);                          /* she is watching: the vessel stands still */
    story_after(WRATH_BEAT, dia_wrath);
}

static void shrine_thought(void) {
    if (shrine_fouled) return;                              /* too late: it is done already */
    think("Breaking a goddess's shrine. I doubt that ends well.", "I doubt that ends well.", BRAIN_SCENE_ORB, THOUGHT_SHRINE, 1);
}

/* the hammer's thoughts, every frame: picking it up (what the vessel thinks depends on whether it knows about the shrine yet), and the
 * hint when standing at a switched-on shrine with empty hands */
static void hammer_watch(void) {
    if (dying || dialog_active()) return;
    int knows = mission_pollute >= 0 && !shrine_fouled;
    if (!hammer_said && world_has_item(ITEM_HAMMER)) {
        hammer_said = 1;
        brainwin_drop_tag(THOUGHT_HAMMER);                  /* "I need something heavy" is answered */
        if (knows) think("A hammer. That should break the shrine.", "That should break the shrine.", BRAIN_SCENE_ORB, THOUGHT_HAMMER, 1);
        else       think("A hammer. This might come in handy.", "This might come in handy.", BRAIN_SCENE_GRASS, THOUGHT_HAMMER, 1);
        return;
    }
    if (hammer_hinted || hammer_said || !knows || !world_shrine_exists()) return;
    float sx, sy; world_shrine_tile(&sx, &sy);
    float dx = world_player_tile_x() - sx, dy = world_player_tile_y() - sy;
    if (dx * dx + dy * dy > HAMMER_HINT_NEAR * HAMMER_HINT_NEAR) return;
    hammer_hinted = 1;
    think("I cannot break it with my hands. I need something heavy.", "I need something heavy.", BRAIN_SCENE_ORB, THOUGHT_HAMMER, 1);
}

static void alex_deal_highlight(int page, int span) {
    if (page != ALEX_DEAL_PAGE || span != 0 || mission_pollute >= 0) return;
    brainwin_drop_tag(THOUGHT_ORB);                         /* "I need to find that orb": found, so it goes silently */
    mission_complete(mission_find_orb);
    mission_pollute = task_toast("Break the shrine");         /* the vessel never says Dia's name */
    world_enable_shrine(shrine_done);                       /* now the shrine can be used (and shows on the minimap) */
}

/* the free chat after the scripted part is over: the head catches up with what just happened */
static void alex_chat_end(void) {
    think("Alex has the orb. All I need is a broken shrine.", "Alex has the orb. I need a broken shrine.", BRAIN_SCENE_ORB, THOUGHT_ORB, 1);
    story_after(10.0f, shrine_thought);
}

static void alex_deal_done(void) { convo_ask_line(&ALEX, &ALEX_MIND, "Got any questions before you go?", alex_chat_end); }

static void on_talk_alex(int npc_id) {
    (void)npc_id;
    if (alex_dealt) { convo_open(&ALEX, &ALEX_MIND, 1, NULL); return; }     /* free chat. she nags about the shrine */
    alex_dealt = 1;
    alex_spotted();
    brainwin_drop_tag(THOUGHT_ALEX);                        /* asked her: "maybe she has seen it" is answered */
    mission_complete(mission_ask_alex);
    dialog_on_highlight(alex_deal_highlight);
    dialog_play(ALEX_DEAL, ALEX_DEAL_COUNT, alex_deal_done);
}

/* the complaint is over and the stick is back: the first thought, sending the player to look */
static void grass_thought(void) {
    think("This place looks really good. I happen to like grass.", "I happen to like grass.", BRAIN_SCENE_GRASS, THOUGHT_GRASS, 1);
}
static void landing_done(void) {
    think("I need to find that orb.", NULL, BRAIN_SCENE_ORB, THOUGHT_ORB, 1);
    story_after(16.0f, grass_thought);                      /* after a little while of walking about */
}

/* the end of vessel 2's opening (chunk 24): the landing is done, the Grasslands are put back and the graves stand. EMPTY ON PURPOSE:
 * vessel 2's own mission is planned later and starts here. the player just walks until then. */
static int vessel2_ready_calls;                     /* (the tests count them) */
void story_vessel2_ready(void) {
    vessel2_ready_calls++;
}

/* the second landing (chunk 20): the Grasslands are exactly as vessel 1 left them. gen_map() makes the same ground every time; what stood
 * on it is put back from `green`: Alex where she was (she remembers the deal: alex_dealt / alex_seen stayed), the shrine where it was,
 * still polluted. a stone stands where each earlier vessel died (chunk 22). nothing is added to the task list (vessel 2's own mission is
 * planned later) and there is no complaint. the only thing said is the placeholder line below; chunk 23 adds the thought at the grave. */
static void landed_again(void) {
    if (green.alex_there) {
        alex_id = npc_add(&ALEX, green.alex_tx, green.alex_ty, on_talk_alex);
        if (alex_id >= 0) npc_set_facing(alex_id, FACE_DOWN);
    }
    if (green.shrine_there) world_restore_shrine(green.shrine_tx, green.shrine_ty, green.shrine_polluted);
    for (int i = 0; i < grave_n; i++) {                     /* a stone where each one died: that tile, or the nearest where it fits */
        int gx, gy;
        if (world_find_prop_spot_near(graves[i].tx, graves[i].ty, &gx, &gy)) world_place_grave(gx, gy, graves[i].who);
    }
    think("Here we go again.", NULL, BRAIN_SCENE_GRASS, THOUGHT_AGAIN, 1);
    story_vessel2_ready();                                  /* the hook for vessel 2's own story (chunk 24) */
}

/* the grave (chunk 23): the first time we walk up to a stone, the vessel stands at its own grave. it keeps its memories in every body,
 * so this is not a stranger's stone: the thought is what anyone would think there. once only, in every life (grave_seen is reset by
 * story_start). the picture in the brain window is the grave at dusk (BRAIN_SCENE_GRAVE). */
#define GRAVE_NEAR      3.5f                /* tiles from the stone's feet: close enough to read the name, on screen */
static int grave_seen;
static void grave_watch(void) {
    if (grave_seen || dying || dialog_active() || world_grave_count() == 0) return;
    if (world_dist_to_grave() >= GRAVE_NEAR) return;
    grave_seen = 1;
    think("I am buried here, and I am standing here. The body stays. I walk on.", "The body stays. I walk on.", BRAIN_SCENE_GRAVE, THOUGHT_GRAVE, 1);
}

/* the player just got back on their feet after the fall */
static void landed(void) {
    int tx, ty;
    if (lives > 0) { landed_again(); return; }              /* a later life: nothing is made up, the world is put back */
    if (world_find_far_spot(ALEX_MIN_DIST, ALEX_MAX_DIST, &tx, &ty)) {
        alex_id = npc_add(&ALEX, tx, ty, on_talk_alex);
        if (alex_id >= 0) npc_set_facing(alex_id, FACE_DOWN);
        int shx, shy;                                       /* Dia's shrine: a walk too, but nearer than Alex, and not next to her */
        if (world_find_spot_away(SHRINE_MIN_DIST, SHRINE_MAX_DIST, tx, ty, SHRINE_FROM_ALEX, &shx, &shy))
            world_place_shrine(shx, shy);
        int hx, hy;                                         /* the hammer: across the map from the shrine (anywhere if that does not fit) */
        float sfx_, sfy_; world_shrine_tile(&sfx_, &sfy_);
        if (world_shrine_exists() && world_find_spot_away(HAMMER_MIN_DIST, HAMMER_MAX_DIST, (int)sfx_, (int)sfy_, HAMMER_FROM_SHRINE, &hx, &hy))
            world_place_item(ITEM_HAMMER, hx, hy);
        else if (world_find_spot_away(HAMMER_MIN_DIST, HAMMER_MAX_DIST, 0, 0, 0, &hx, &hy)) world_place_item(ITEM_HAMMER, hx, hy);
        else world_give_item(ITEM_HAMMER);
    }
    dialog_play(LANDING, (int)(sizeof LANDING / sizeof LANDING[0]), landing_done);
}

/* ---------- scene 2: Dea ---------- */
static const DialogLine DEA_TALK[] = {
    { &DEA,    "Hello, " VESSEL_LATIN ". I have been waiting for you." },
    { &VAS,    "Where am I?" },
    { &DEA,    "Between places. You are a vessel, and a vessel needs a name." },
    { &DEA,    "From now on, you are " VESSEL_NAME ". It plays on the word one, for you are the first." },
    { &VESSEL, VESSEL_NAME ". I will remember it." },
    { &DEA,    "Your purpose is to carry out missions. Nothing more, nothing less." },
    { &DEA,    "Finish each one, and stay secret. When your job is done, your life ends." },
    { &DEA,    "Now, your first mission. I {dropped my orb in the Grasslands}. Find it and bring it back to me." },
};
#define DEA_TALK_COUNT  ((int)(sizeof DEA_TALK / sizeof DEA_TALK[0]))
/* then Dea asks "do you have any questions?" (convo.c) and the hole opens when the chat is over */

/* pressed the arrow button at the hole */
static void on_enter_hole(void) {
    brainwin_drop_tag(THOUGHT_CLOUDS);                      /* leaving the sky: the cloud thought goes, silently */
    world_fall_to_green(landed);
}

static void open_the_hole(void) {
    world_open_hole(dea_tx, dea_ty + 3, on_enter_hole);         /* a hole of clouds opens in the floor in front of her */
}
static void dea_talk_done(void) { convo_ask_questions(&DEA, &DEA_MIND, open_the_hole); }

/* page 4 of DEA_TALK is the first line spoken as VESSEL: that is the moment the name sticks */
#define DEA_NAMING_PAGE 4
static void dea_talk_page(int page) {
    if (page == DEA_NAMING_PAGE) { hud_set_person(&VESSEL); convo_set_player(&VESSEL); }   /* ID card flashes, coin ding, "You are now Aonia" */
}

/* the last page of DEA_TALK holds the highlighted orb sentence. the moment those words have been
 * typed out, the task coins in: ding + "New task added" toast + "Find the orb" on the checklist */
#define DEA_ORB_PAGE  (DEA_TALK_COUNT - 1)
static void dea_talk_highlight(int page, int span) {
    if (page == DEA_ORB_PAGE && span == 0 && mission_find_orb < 0)
        mission_find_orb = task_toast("Find the orb");
}

static void on_talk_dea(int npc_id) {
    (void)npc_id;
    if (scold_pending) { scold_pending = 0; dea_scold(); return; }          /* the second life: the scolding, then the questions */
    if (dea_spoken) { convo_open(&DEA, &DEA_MIND, 1, NULL); return; }       /* free chat. she nags about the hole */
    dea_spoken = 1;
    mission_complete(mission_talk_dea);
    dialog_on_page(dea_talk_page);
    dialog_on_highlight(dea_talk_highlight);
    dialog_play(DEA_TALK, DEA_TALK_COUNT, dea_talk_done);
}

/* ---------- scene 1: up in the clouds (vessel 2, chunks 10-11) ----------
 * no welcome window, no toast: after "Where am I?" the world fades in, the summoning runs (world_summon, started by
 * story_start) and intro_done() is what runs when the light ends: the controls come back and Dea stands 8 tiles above,
 * looking down. we have to look around: a few seconds on, the player wonders where this is (cloud_thought); walking
 * within sight of Dea (dea_watch, like alex_watch) coins in the task "Talk to Goddess" and a thought. */
#define DEA_SIGHT       6.5f                /* tiles: about half a screen, so she is on screen when it fires */

static void cloud_thought(void) {
    think("Is this where dead people go?", NULL, BRAIN_SCENE_CLOUDS, THOUGHT_CLOUDS, 1);
}

/* the player has caught sight of Dea: the task and a thought. once only */
static void dea_spotted(void) {
    if (dea_seen) return;
    dea_seen = 1;
    mission_talk_dea = task_toast("Talk to Goddess");
}

/* the proximity trigger, run every frame: walking within sight of Dea (not while a dialog is open) */
static void dea_watch(void) {
    if (dea_id >= 0 && !dea_seen && !dialog_active() && world_dist_to_npc(dea_id) < DEA_SIGHT)
        dea_spotted();
}

static void intro_done(void) {
    world_set_controls_visible(1);              /* stick and interact button become usable */
    story_after(2.5f, cloud_thought);
    /* Dea stands 8 tiles above where we spawned, looking down at us */
    dea_tx = world_player_tile_x();
    dea_ty = world_player_tile_y() - 8;
    dea_id = npc_add(&DEA, dea_tx, dea_ty, on_talk_dea);
    npc_set_facing(dea_id, FACE_DOWN);
}

/* ---------- entry point ---------- */
void story_start(void) {
    for (int i = 0; i < MAX_TIMERS; i++) timers[i].fn = NULL;
    brainwin_clear();                           /* a new game starts with an empty head */
    mission_talk_dea = -1;
    mission_find_orb = -1;
    mission_ask_alex = -1; mission_pollute = -1; alex_dealt = 0; shrine_fouled = 0; dying = 0;
    hammer_hinted = hammer_said = 0; world_clear_items();
    alex_id = -1; alex_seen = 0;
    dea_spoken = 0; dea_id = -1; dea_seen = 0; scold_pending = 0;
    vessel_dead = 0; lives = 0; limbo_enter_req = 0; death_hold_done = 0; grave_n = 0; grave_seen = 0;
    memset(&limbo, 0, sizeof limbo);            /* a new game is not in limbo (the story enters it itself) */
    limbo_opening = 0;
    DEA_MIND.mood = 0; ALEX_MIND.mood = 10;
    convo_set_player(&VAS);
    world_set_controls_visible(0);
    world_summon(intro_done);                   /* the light, the sparks, the vessel; it starts when limbo hands over to the world (game.c) */
}
