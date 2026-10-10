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
 *   world_enable_shrine(on_done)            the shrine can be used: hold the interact button next to it to pollute it, then on_done()
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
#include "audio.h"
#include "jukebox.h"
#include "heart.h"
#include "hud.h"
#include "convo.h"
#include "lang.h"
#include <string.h>

#define VESSEL_NAME  "Aonia"        /* vessel one. later: Doia, Tria, Ceathia ... (Dea plays on the word "one") */
#define VESSEL_LATIN "Vas"          /* what Dea calls us before the naming. "Vas" is Latin for vessel */

/* ---------- characters ---------- */
/*                      name           hair            skin             shirt           pants          boots        long hair  accessories */
/* the soul: what we are before we have a body (limbo, vessel 2). pale ghost-blue and white; only its face is shown (the
 * brain card while in limbo, via thought_set_voice), the halo is for a full body should it ever be drawn */
const Person SOUL = { "Soul", { 232, 240, 255 }, { 188, 226, 246 }, { 140, 186, 238 }, { 118, 160, 218 }, { 96, 130, 196 }, 0, ACC_HALO };
const Person VESSEL = { VESSEL_NAME,  { 96, 58, 36 },  { 248, 208, 170 }, { 214, 60, 60 },  { 52, 70, 140 },  { 40, 30, 30 }, 0, 0 };
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

static void alex_watch(void);               /* the proximity trigger, defined with the ground scene below */

void story_update(float dt) {
    alex_watch();
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
static int dea_tx, dea_ty;                  /* where Dea stands (the hole opens a few tiles below her) */

/* ---------- thoughts (the brain window, brainwin.h) ----------
 * nothing is preloaded: the vessel acquires a thought when something happens, and loses it when it stops
 * being true. every thought has a TAG so a whole group can be dropped silently (leaving a map, a task done). */
enum { THOUGHT_CLOUDS = 1, THOUGHT_ORB, THOUGHT_GRASS, THOUGHT_ALEX, THOUGHT_COIN, THOUGHT_SHRINE };

/* `text` goes in the window (up to ~90 letters). `card` is what pops out of the brain button (about 19
 * letters x 3 lines max; NULL = same as text, "" = no card). ding = the coin ding of acquiring something
 * (leave it off when a task toast is dinging at the same moment) */
static void think(const char *text, const char *card, BrainScene scene, int tag, int ding) {
    if (dying) return;                                      /* this head is not the vessel's any more */
    if (brainwin_acquire(text, scene, tag) < 0) return;
    if (ding) sfx_coin();
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
#define LIMBO_BEAT      1.0f                /* stars only, before the first thought */
#define LIMBO_WHERE_HOLD 3.5f               /* from the thought to the end of limbo: the soul waits (about 3.5 s) */
#define LIMBO_FADE_T    0.8f                /* the last part of the hold: the menu music fades out */

static const char *const LIMBO_WHERE[] = { "Where am I?" };
static int limbo_stay;                      /* dev: stay in limbo for good (VESSEL_LIMBO=1) */

static void limbo_where_end(void) { limbo_end(); }          /* game.c shows the world, the summoning is already waiting there */
static void limbo_where_fade(void) {
    jukebox_scene_fade(LIMBO_FADE_T);
    limbo_run(NULL, 0, LIMBO_FADE_T, limbo_where_end);
}
static void limbo_where_say(void) {
    limbo_run(LIMBO_WHERE, 1, limbo_stay ? 1000.0f : LIMBO_WHERE_HOLD - LIMBO_FADE_T, limbo_stay ? NULL : limbo_where_fade);
}

/* stay = 1: the dev entry (VESSEL_LIMBO=1), says the thought and then stays in limbo forever */
void story_limbo_begin(int stay) {
    limbo_stay = stay;
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
    { &ALEX,   "Dia has a shrine near here. She thinks she owns the whole place. Go {pollute the shrine}, then come back to me." },
};
#define ALEX_DEAL_COUNT  ((int)(sizeof ALEX_DEAL / sizeof ALEX_DEAL[0]))
#define ALEX_DEAL_PAGE   (ALEX_DEAL_COUNT - 1)

/* ---------- Dia's shrine (chunk 10) ----------
 * the prop is placed when we land (landed() below) and does nothing until Alex has asked for it. then holding
 * the interact button next to it pollutes it (shrine.c); the moment it is full this runs. */
#define SHRINE_MIN_DIST    16               /* tiles from the landing spot */
#define SHRINE_MAX_DIST    26
#define SHRINE_FROM_ALEX   18               /* and not on top of Alex: she is somewhere else entirely */

/* ---------- Dia's wrath (chunk 11) ----------
 * the shrine is polluted. a beat of quiet, then Dia hijacks the brain button: "HOW DARE YOU", red and shaking,
 * with her eye where the player's face would be. the player cannot move or open the brain window meanwhile.
 * a short pause after her card has folded away, and the death begins (chunk 12). */
#define WRATH_TEXT        "HOW DARE YOU"
#define WRATH_BEAT        0.9f              /* seconds between the last drop of ooze and her voice */
#define WRATH_SECONDS     3.0f              /* how long her card stays up */
#define WRATH_TO_DEATH    1.6f              /* after her card has had its WRATH_SECONDS: it folds away (~0.5 s), then a short pause, then the death */

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

/* the end hook of vessel 1's mission. vessel 2 (respawn at Dea, the name Doia, her scolding) is NOT built yet: it starts here.
 * until then the picture just stays (the pause button still restarts). */
static int vessel_dead;
static void story_vessel_died(void) { vessel_dead = 1; }

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

static void dia_wrath(void) {
    thought_set_wrath_person(&DEA);                         /* a goddess's face, in red */
    thought_wrath(WRATH_TEXT, WRATH_SECONDS);
    story_after(WRATH_SECONDS + WRATH_TO_DEATH, death_begin);
}

static void shrine_done(void) {
    shrine_fouled = 1;
    dying = 1;
    jukebox_scene_fade(0.15f);                              /* the music dies the moment it is fouled (map music only: a custom track keeps playing) */
    brainwin_drop_tag(THOUGHT_SHRINE);                      /* "I doubt that ends well" has come true, or is about to */
    mission_complete(mission_pollute);
    world_set_controls_visible(0);                          /* she is watching: the vessel stands still */
    story_after(WRATH_BEAT, dia_wrath);
}

static void shrine_thought(void) {
    if (shrine_fouled) return;                              /* too late: it is done already */
    think("Polluting a goddess's shrine. I doubt that ends well.", "I doubt that ends well.", BRAIN_SCENE_ORB, THOUGHT_SHRINE, 1);
}

static void alex_deal_highlight(int page, int span) {
    if (page != ALEX_DEAL_PAGE || span != 0 || mission_pollute >= 0) return;
    brainwin_drop_tag(THOUGHT_ORB);                         /* "I need to find that orb": found, so it goes silently */
    mission_complete(mission_find_orb);
    mission_pollute = task_toast("Pollute the shrine");         /* the vessel never says Dia's name */
    world_enable_shrine(shrine_done);                       /* now the shrine can be used (and shows on the minimap) */
}

/* the free chat after the scripted part is over: the head catches up with what just happened */
static void alex_chat_end(void) {
    think("Alex has the orb. All I need is a polluted shrine.", "Alex has the orb. I need a polluted shrine.", BRAIN_SCENE_ORB, THOUGHT_ORB, 1);
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

/* the player just got back on their feet after the fall */
static void landed(void) {
    int tx, ty;
    if (world_find_far_spot(ALEX_MIN_DIST, ALEX_MAX_DIST, &tx, &ty)) {
        alex_id = npc_add(&ALEX, tx, ty, on_talk_alex);
        if (alex_id >= 0) npc_set_facing(alex_id, FACE_DOWN);
        int shx, shy;                                       /* Dia's shrine: a walk too, but nearer than Alex, and not next to her */
        if (world_find_spot_away(SHRINE_MIN_DIST, SHRINE_MAX_DIST, tx, ty, SHRINE_FROM_ALEX, &shx, &shy))
            world_place_shrine(shx, shy);
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
    if (dea_spoken) { convo_open(&DEA, &DEA_MIND, 1, NULL); return; }       /* free chat. she nags about the hole */
    dea_spoken = 1;
    mission_complete(mission_talk_dea);
    dialog_on_page(dea_talk_page);
    dialog_on_highlight(dea_talk_highlight);
    dialog_play(DEA_TALK, DEA_TALK_COUNT, dea_talk_done);
}

/* ---------- scene 1: up in the clouds (vessel 2, chunk 10) ----------
 * no welcome window, no toast: after "Where am I?" the world fades in, the summoning runs (world_summon, started by
 * story_start) and intro_done() is what runs when the light ends: the controls come back and Dea stands 8 tiles above,
 * looking down. we just have to look around. (the "Talk to Goddess" task and the first thought come in chunk 11.) */
static void intro_done(void) {
    world_set_controls_visible(1);              /* stick and interact button become usable */
    /* Dea stands 8 tiles above where we spawned, looking down at us */
    dea_tx = world_player_tile_x();
    dea_ty = world_player_tile_y() - 8;
    int id = npc_add(&DEA, dea_tx, dea_ty, on_talk_dea);
    npc_set_facing(id, FACE_DOWN);
}

/* ---------- entry point ---------- */
void story_start(void) {
    for (int i = 0; i < MAX_TIMERS; i++) timers[i].fn = NULL;
    brainwin_clear();                           /* a new game starts with an empty head */
    mission_talk_dea = -1;
    mission_find_orb = -1;
    mission_ask_alex = -1; mission_pollute = -1; alex_dealt = 0; shrine_fouled = 0; dying = 0;
    alex_id = -1; alex_seen = 0;
    dea_spoken = 0;
    memset(&limbo, 0, sizeof limbo);            /* a new game is not in limbo (the story enters it itself) */
    DEA_MIND.mood = 0; ALEX_MIND.mood = 10;
    convo_set_player(&VAS);
    world_set_controls_visible(0);
    world_summon(intro_done);                   /* the light, the sparks, the vessel; it starts when limbo hands over to the world (game.c) */
}
