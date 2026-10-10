# Vessel 1 — "The Orb" (first mission)

> **Handoff file.** If you are an LLM picking this up mid-work: read this whole file, find the
> first unchecked chunk in [Chunk checklist](#chunk-checklist), implement ONLY that chunk, commit,
> tick its box in this file **in the same commit**, push, then go to the next one.
> Do not skip ahead, do not batch chunks. The owner tests every push on their phone
> (`git pull` + `scripts/run.sh`), so every chunk must leave the game building and playable.

## Workflow rules

- One chunk = one commit = one push. Commit message: `chunk N: <short title>` + a paragraph of what changed.
- Tick the chunk's checkbox below in the same commit that implements it.
- Keep `game/story.c` as the single place where story text and scripted events live.
  Engine pieces (overlays, effects) get their own `game/*.c` + `.h` pair, like the existing ones.
- Match the existing code style (C, SDL2, comment header with GPL line, short explanatory comments).
- Strings shown to the player must be proper English (the drafts below are rough on purpose; polish them).
- Never commit tokens or secrets.
- Sanity-check that it compiles before pushing (`scripts/run.sh` builds on the phone; on a PC use
  `cc -O2 -DGFX_REDIRECT -include game/gfx.h game/*.c $(sdl2-config --cflags --libs) -lm` or the `tools/*test.c` helpers).

## Cast and names

| Who | Notes |
|---|---|
| **Vas → Aonia** | The player, vessel one. "Vas" before Dea names them, "Aonia" after. Already implemented. |
| **Dea** | The goddess in the sky. Toxic, condescending, "I do not care". Already implemented (see `DEA`, `DEA_MIND`). |
| **Alex** | A girl on the ground (she). Has found Dea's orb. Will not hand it over for free. Far from the landing spot. |
| **Dia** | A rival goddess. Has a shrine on the map. Watches over it. (Spelling taken from the owner's notes; "Dia'" was a typo.) |

Maps: the sky is `MAP_CLOUD`; the ground `MAP_GREEN` is called **the Grasslands** in all player-facing text
(internal enum name can stay).

## Story, start to finish

### 1. The sky (already working)
Welcome window, "Talk to Goddess" task, Dea names the player Aonia (ID card flashes, coin ding).

### 2. Dea's task: the orb
While Dea talks she says she **dropped an orb in the Grasslands** and wants it back.
The words of that sentence are **highlighted in the dialog** (same green+underline look as the typed-chat
highlighting in `talk.c`). At the moment the highlighted words appear, a coin ding plays and a
toast says **"New task added"**, and the checklist gets **"Find the orb"**.

After that the normal flow continues: "do you have any questions?" free chat (`convo_ask_questions`),
the conversation ends, the cloud hole opens, the player jumps in.

### 3. The fall
`world_fall_to_green` already exists. After landing and getting up the player says "That hurts",
then complains, e.g. *"She could at least have warned me."* (polish the wording). Dialog popup with the player's face.

### 4. The brain button (new system)
A **brain button** in the top row, right of the music button (the pause button moves one place along):
a square button with a pixel brain on it. When the player *thinks* something it expands exactly like the
music button does (opens into a card with the player's portrait + the words, stays a few seconds, eases
back into the button). It is the game's way of talking to the person playing.

- Not interactive: cannot be tapped or controlled. Touches pass through to the game.
- Comes and goes by itself. Can be triggered by the story at any time (`thought_say`).
- Pushes things out of its way instead of covering them: the pause button slides along, the mission list
  and the "New task added" toast under it are pushed down while the card is open. Never taller than the
  ID card, so the name toast under the ID card stays clear.
- If the music card is open and leaves too little room for the whole thought, the thought waits (its time
  holds) until the music card folds away. A thought is never cut because of the music card.
- First thought, right after the player can walk: **"I need to find that orb."**

### 5. Alex
Walking around, the player finds **Alex**, far from the landing spot (no NPCs near where we fall).
- Seeing her (proximity trigger) → thought: *"Maybe she has seen the orb."* (polish) and the task **"Ask Alex"** coins in.
- Talking to Alex: she has the orb but **won't give it** unless the player goes to **Dia's shrine and pollutes it**.
  New task: **"Pollute Dia's shrine"**. (Alex's free typed chat stays available through `convo_open`.)

### 6. Dia's shrine and the death
- A shrine prop on the map (placeholder art is fine).
- The player interacts with it to trash / sabotage it (a short action, e.g. hold the button or a few taps).
- **Dia reacts**: she hijacks the thinking bar. It shows **"HOW DARE YOU"** (styled differently from the player's
  own thoughts: red, shaking, her name or no portrait). Then she **stops vessel 1's heart on the spot**.

### 7. Death cutscene (the end of this mission)
Triggered when the heart stops. In order:
1. Controls hidden, world frozen, camera **zooms to 300%** on the player.
2. A **grey filter** over everything *except the player* (map, npcs, props all desaturated),
   and a **reddish filter on the player**.
3. Heart-monitor **"peep. peep. peep."** (synth sfx) plays while the filter applies.
4. A **pixel heart** is overlaid at the player's chest position.
5. The heart **explodes** with blood splashing (pixel particles).
6. Sound becomes one long **"beeeeeeeeeep"** (flatline) at the explosion.
7. Overlay text: **"{name} has died."** → `Aonia has died.`

That is the end of vessel 1's mission. **Out of scope here:** vessel 2 (respawn at Dea, name Doia,
her scolding). Leave a clean hook (e.g. `story_vessel_died()` stub) but do not build it.

## Chunk checklist

Mark `[x]` when the chunk is pushed.

- [x] **1. Story doc** — this file.
- [x] **2. Grasslands + empty landing zone** — player-facing name "the Grasslands"; remove Alex from the landing area (no NPCs near the fall spot; Alex is not spawned yet).
- [x] **3. Dialog highlight markup** — engine: mark spans of a dialog line (e.g. `{...}` or a tag) to draw highlighted (green+underlined), plus an optional callback when the highlighted span has been typed out.
- [x] **4. "New task added" toast** — engine: small toast + coin ding, and `mission_add` hooked to it. API like `task_toast("Find the orb")`.
- [x] **5. Dea's orb lines** — rewrite `DEA_TALK` so she mentions the dropped orb with the highlight; the highlight callback adds the task "Find the orb" via the toast. Rest of the sky flow unchanged.
- [x] **6. Landing complaint** — after getting up from the fall: "That hurts" + complaint dialog with the player's face, then controls return.
- [x] **7. Thinking bar (engine)** — `game/thought.c/.h`: the brain button (right of the music button) + `thought_say(text, seconds)`; expands like the music card, portrait, non-interactive, pushes the pause button / mission list / toasts out of its way, doesn't overlap the ID card.
- [x] **8. First thought + Alex placement** — after landing: "I need to find that orb." Alex spawns far from the landing tile; proximity trigger fires the thought about asking her and the task "Ask Alex".
- [x] **8.6 Brain window** — tapping the brain button opens a window: a monitor showing the vessel's inner picture (looping pixel art) + the thought, an array of thoughts that loops, numbered boxes to pin one, AUTO to loop again. `game/brainwin.c/.h`.
- [x] **8.7 Event-based thoughts (hotfix)** — the brain window starts empty; thoughts are acquired by story events (coin ding) and silently dropped when they stop being true (leaving a map, a question answered). `brainwin_acquire` / `brainwin_drop_tag` / `brainwin_clear`, `think()` + `THOUGHT_` tags in `story.c`.
- [x] **8.8 Toast + brain tap fixes (hotfix)** — the "YOU ARE NOW <NAME>" notification uses the same panel as "New task added" (`toast_notice`); tapping the thought already on the brain monitor does nothing.
- [x] **9. Alex's deal** — Alex's scripted dialog: she has the orb, wants Dia's shrine polluted. Task "Pollute Dia's shrine". Free chat stays after the scripted part.
- [x] **10. Dia's shrine** — shrine prop on the map, interact → sabotage action, task completes when done.
- [x] **11. Dia's wrath** — thinking bar variant (red, shaking): "HOW DARE YOU". Short pause, then death starts.
- [x] **12. Death cutscene I: camera + filters** — freeze world, zoom to 300%, grey everything but the player, red tint on the player.
- [x] **12.1 Small fixes (hotfix)** — AUTO in the brain window is a toggle; the dialog SKIP button is now BYE (the player says goodbye and the npc answers it); loading screen and the fade after it are half as long; a finger on the screen types dialog letters (and the brain caption) 2x faster with 2x higher blips.
- [x] **13. Death cutscene II: sound** — synth sfx: `sfx_beep()` for the monitor "peep"s and `sfx_flatline()` for the long tone (in `audio.c`).
- [x] **14. Death cutscene III: heart** — pixel heart at the chest, explosion with blood particles, synced with the flatline.
- [x] **14.1 Small fixes (hotfix)** — the vessel never says Dia's name or that it is a vessel (task is "Pollute the shrine", it just wants the orb); the coin thought is gone; a new thought replaces the one on the brain card at once (no queue); the mission list, the task toast and the open music card share one width (`ui_panel_w` in `font.h`); Alex asks "Got any questions before you go?" after her deal (`convo_ask_line`) instead of "What do you want?".
- [ ] **15. "{name} has died." + end hook** — overlay text, then a `story_vessel_died()` stub for vessel 2. Update this file's status.

## Draft strings (polish before shipping)

- Dea, orb line: "I dropped my {orb} in the Grasslands. Bring it back." ({...} = highlighted)
- Task toast: "New task added" / task: "Find the orb"
- Landing: "Ow. That hurts." / "She could at least have warned me."
- Thought 1: "I need to find that orb."
- Thought at Alex: "Maybe she has seen the orb." / task: "Ask Alex"
- Alex: refuses unless the shrine of Dia is polluted. / task: "Pollute Dia's shrine"
- Dia (thinking bar): "HOW DARE YOU"
- Death: "Aonia has died."

## Code map (what exists today)

- `game/story.c` — all scripted scenes (sky: Dea; ground: Alex placeholder). Helpers listed in its header comment.
- `game/dialog.c/.h` — paged dialog, `DialogLine`, `dialog_on_page`, `dialog_on_reply`, `{highlight}` markup + `dialog_on_highlight(fn(page, span))`.
- `game/convo.c/.h` (`convo_open`, `convo_ask_questions`, `convo_ask_line(who, mind, "exact words", on_end)` = the npc asks, in her own words, if there are any questions), `game/talk.c/.h`, `game/lang*.c` — free typed chat with NPC personas.
- `game/npc.c/.h` — static NPCs (`npc_add`, facing). No movement/removal yet.
- `game/missions.c/.h` — top-left checklist (`mission_add`, `mission_complete`, `missions_bottom`).
- `game/toast.c/.h` — "New task added" toast under the checklist: `task_toast("Find the orb")` = `mission_add` + coin ding + toast (extra toasts queue).
- `game/thought.c/.h` — the brain button: `thought_say("I need to find that orb.", 0)`. Third top-left button (music, brain, pause), opens like the music card, non-interactive. A new `thought_say` while a card is up REPLACES its words and restarts its clock (no queue; only Dia's wrath holds own thoughts back). `thought_rect` / `thought_offset` tell the pause button and the mission list where to sit.
- `game/brainwin.c/.h` — the brain window (opens from the brain button): thoughts array at the top of the file (`DEFAULTS`) or `brainwin_add(text, scene)`; scenes `scene_orb/grass/clouds/coin` draw on a 40x24 pixel grid.
- `game/shrine.c/.h` — Dia's shrine (placeholder art: stone altar + glowing gem). `world_place_shrine` / `world_enable_shrine(on_done)`; inert until enabled; then the interact button shows a sludge drop next to it and HOLDING it for `SHRINE_HOLD_T` (2.5 s) fills it with ooze, letting go early makes the ooze creep back. Blocks the player, violet dot on the minimap while usable. One instance, owned by `world.c`.
- `thought_wrath(text, seconds)` in `game/thought.c/.h` — Dia's voice on the brain button: red card, shudders one cell at a time, her red diamond eye with a slit pupil instead of the player's face, red text with a dark shadow. It replaces whatever is showing, forgets the queue, cannot be tapped open into the brain window, and own thoughts that come while it is up wait behind it. `thought_is_wrath()` tells if it is up.
- `gfx_set_filter(grey, red)` in `game/gfx.c/.h` — a colour filter on every colour the game sets (0..256 each): grey = toward the colour's brightness and a fifth darker, red = greens/blues drain. Works in CPU and GPU mode (the recorded colour is already filtered, the Java side is untouched). Only for code built with `GFX_REDIRECT`, so tests that want to see it need `-DGFX_REDIRECT -include game/gfx.h` (groundtest has it now). Colours set before the call are not changed.
- `world_death_begin(on_ready)` / `world_death_active()` / `world_death_player(&feet_x, &feet_y, &chest_y, &pixel)` / `world_death_cancel()` in `game/world.c/.h` — the death camera. Controls go, the player faces us, the world clock `t` stops (no wind/water/clouds), a 2.2 s eased push-in to 300 %, the colour drains from 0.4 s over 1.6 s (scene grey via `scene_filter(0)`, the player red via `scene_filter(1)`; the HUD is unfiltered), `on_ready` runs at 2.6 s. `world_death_player` gives the player's screen position with the zoom (chunk 14's heart goes at `chest_y`, one sprite pixel = `pixel` screen px). `world_death_cancel` undoes it (stand-in/tests).
- `sfx_beep()` / `sfx_flatline()` / `sfx_flatline_stop()` in `game/audio.c` — the heart monitor, one voice, 1 kHz sine: a peep is 0.16 s with a quick decay, the flatline is one steady level for 3 s (then lets go over 0.4 s; `sfx_flatline_stop()` cuts it in 60 ms; a new peep replaces it). `sfx_debug_beeps()` / `sfx_debug_flatlines()` count the calls for tests. `story.c`: `death_ready()` starts `death_beep()` = `DEATH_BEEPS` (3) peeps `DEATH_BEEP_GAP` (0.85 s) apart, then `death_flat()` at `DEATH_FLAT_AT` (where chunk 14 puts the explosion).
- `game/heart.c/.h` — the pixel heart and its blood (engine only). `heart_show()` / `heart_pulse()` / `heart_burst()` / `heart_reset()`, `heart_update(dt)` (called from `world_update`, it keeps moving when the world is frozen), `heart_draw(r, cx, cy, pixel)` (called from `world_draw` after the zoomed scene while `death_on`, at the feet x and `chest_y` of `world_death_player`). A 9x8-cell heart (one cell = 0.55 sprite pixels, dark outline, shine, shaded underside) that thumps (+30 % for 0.24 s) on every peep; `heart_burst()` flashes a white heart for 0.14 s and throws 140 blood pixels (4 reds, half-cell splashes and whole-cell chunks) out of the heart shape with gravity, drag and fade over 1-2.4 s. Blood lives in cell units relative to the chest. `heart_state()` / `heart_particles()` are for tests. `world.c` resets it in `world_init`, `world_death_begin` and `world_death_cancel`. `story.c`: `death_ready()` calls `heart_show()`, `death_beep()` `heart_pulse()`, `death_flat()` `heart_burst()` next to `sfx_flatline()`.
- `game/hud.c/.h` — ID card (`hud_set_person`, `hud_set_hp`, name-change toast).
- `game/world.c/.h` — maps, camera, hole, fall (`world_fall_to_green`), `world_find_far_spot` (a walkable open tile N tiles away, for placing npcs), `world_dist_to_npc` (proximity triggers).
- `game/audio.c/.h` — music + synthesized sfx (`sfx_coin`, `sfx_blip`).

## Status log

- 2026-10-09: story written, chunks planned.
- 2026-10-09: chunk 2 done (GRASSLANDS / SKYLAND_NAME + world_map_name in world.h; Alex placeholder chat and spawn removed from story.c, `landed()` is an empty stub). Next: chunk 3.
- 2026-10-09: chunk 3 done (`{...}` markup in `DialogLine.text` draws green + underlined, spans may wrap over lines; `dialog_on_highlight(fn(page, span))` fires once when a span has been typed out; tested with `tools/dialogtest.c`). Next: chunk 4.
- 2026-10-09: chunk 4 done (`task_toast(text)` in `game/toast.c`: adds the mission, plays the coin ding, drops a NEW TASK ADDED panel under the checklist, queues up to 4; `hud_draw_coin` is now shared; `tools/toasttest.c` renders it on a PC). Next: chunk 5.
- 2026-10-09: chunk 5 done (`DEA_TALK` has a new last page: "Now, your first mission. I {dropped my orb in the Grasslands}. Find it and bring it back to me."; `dea_talk_highlight` in `story.c` calls `task_toast("Find the orb")` once that span has typed out; page count is now `DEA_TALK_COUNT`). Next: chunk 6.
- 2026-10-09: chunk 6 done (`landed()` in `story.c` plays the `LANDING` dialog as VESSEL with her face popup: "Ow. That hurts." / "She could at least have warned me."; `world.c` hides the controls while a dialog is open, so they come back when it ends). Next: chunk 7.
- 2026-10-10: chunk 7 done (`game/thought.c/.h`: the BRAIN BUTTON, a pixel-brain button right of the music button; `thought_say(text, seconds)` opens it into a card (portrait + wrapped text) with the music card's easing, holds, folds back; seconds 0 = auto; queue of 4; no touch handler; drawn from `game.c` between the music and pause buttons; `pausebtn.c` now sits right of it; `world.c` takes the larger of `nowplaying_offset()` / `thought_offset()` for the mission list, so the list and the task toast under it are pushed down while the card is open; the card is never taller than the ID card; a thought that does not fit next to an open music card waits, timer held, instead of being cut; new `hud_person()`; `tools/thoughttest.c` renders and checks it, run from `music/`). Chunk 11 can add a red/shaking variant here. Next: chunk 8.
- 2026-10-10: chunk 8 done (after the landing complaint `landing_done()` says the thought "I need to find that orb."; `landed()` places Alex with the new `world_find_far_spot(26, 40, ...)` = a spot the player can really walk to, 26-40 tiles away (today: tile 7,40, 33 tiles due west of the landing); `alex_watch()` in `story_update` fires `alex_spotted()` within `ALEX_SIGHT` = 6.5 tiles: thought "Maybe she has seen the orb." + `task_toast("Ask Alex")`, once only. `on_talk_alex` is a stand-in until chunk 9: ticks "Ask Alex", opens `convo_open` free chat. `tools/groundtest.c` runs the whole ground flow headless (fall, land, Alex placement, sighting, talk) and checks it.) Thoughts: about 19 letters per line, 2-3 lines depending on the screen. Next: chunk 9.
- 2026-10-10: chunk 8.5 done (UI polish: the minimap is exactly as wide as the ID card and still square, `minimap_init(..., width)`, tiles are sized with `i * side / VIEW` so the width need not be a multiple of the tile size; the whole top UI moves down by half a button (`top` in nowplaying.c, `card_y` in hud.c) to clear notches, and the mission list / toast / minimap / thought card follow; button order is now music, pause, brain: pause follows the music button, the brain button follows pause and has the whole space up to the ID card; `thoughttest` updated for the new order). Next: chunk 9.
- 2026-10-10: chunk 8.6 done (`game/brainwin.c/.h`: tapping the brain button or its card opens the BRAIN WINDOW, slides up like the music window and takes every touch while open; a monitor (40x24 big pixels, bezel + scanlines, static flicker between thoughts) with the player's face and a rising thought trail in the corner, the thought typed out under it, a bar for the time left; the window loops through the thoughts array, 6-9 s each by text length; header says `N THOUGHTS`; numbered boxes pin a thought, AUTO loops again; 4 thoughts to start: orb (animated `?` + a vague item that morphs ball/box/gem/flower), grass (the map scrolling by), clouds, coin; `brainwin_add(text, scene)` adds more at runtime, up to 12; `thought_say` cards do NOT add to the window; `thought_touch()` in thought.c is the button; `tools/brainwintest.c`, run from `music/`, checks 540x1170, 1170x540 and 360x640). Next: chunk 9.
- 2026-10-10: chunk 8.7 done (hotfix: no preloaded thoughts any more, the window starts empty and shows "Nothing on my mind yet."; `brainwin_acquire(text, scene, tag)` adds a thought and jumps to it when the window is open on AUTO, `brainwin_drop_tag(tag)` forgets a group silently, `brainwin_clear()` on `story_start`; `story.c` has `think(text, card, scene, tag, ding)` that adds to the window + coin ding + brain card, and the `THOUGHT_` tags. Events: sky, 2.5 s after the stick comes back -> "Why am I standing on clouds?" (THOUGHT_CLOUDS, dropped silently when the player enters the hole); landing complaint over -> "I need to find that orb." (THOUGHT_ORB); 16 s later -> grass thought; sighting Alex -> "Maybe she has seen the orb." (THOUGHT_ALEX, no extra ding because the toast dings; dropped when the player talks to her), then 9 s later the new coin thought "Every task rings a coin. Who is paying me, and what for?" replacing the "oddly satisfying ding" one. The window keeps room for 3 caption lines so it does not jump as thoughts come and go. AUTO is on by default and every time the window opens. `brainwintest` and `groundtest` check all of it). Next: chunk 9.
- 2026-10-10: chunk 8.8 done (hotfix: `toast_notice(head, body)` in `toast.c` shares the task toast's panel, spot (under the mission list), size, queue and coin ding; `hud_set_person` calls it for "YOU ARE NOW <NAME>" and the ID card no longer draws or dings its own toast, only flashes the name; the brain window ignores a tap on the number of the thought already on the monitor, so it no longer replays or pins; `toasttest` covers the name toast, `tools/*test.c` build lines now list `toast.c`/`missions.c`/`nowplaying.c` where `hud.c` needs them). Known, older: `thoughttest 1170 540` (landscape) fails 3 checks about the music card leaving no room, same before this change. Next: chunk 9.
- 2026-10-10: chunk 9 done (`ALEX_DEAL` in `story.c`: 7 pages, the player asks about the orb, Alex has it, "Finders keepers", she will trade it for the shrine; the last page has `{pollute Dia's shrine}`, and when those words are typed out `alex_deal_highlight` drops the "find the orb" thought, ticks "Find the orb" and coins in "Pollute Dia's shrine" (`mission_pollute`). Then `convo_open` free chat; when it ends `alex_chat_end` acquires "Alex has the orb. All I need is a polluted shrine." (THOUGHT_ORB) and 10 s later "Polluting a goddess's shrine. I doubt that ends well." (THOUGHT_SHRINE). Talking to her again is free chat only (`alex_dealt`). `groundtest` taps through the deal). Next: chunk 10 (the shrine prop). Thoughts are added / dropped by the story as chunks go on, see `think()` and the `THOUGHT_` tags.
- 2026-10-10: chunk 10 done (`game/shrine.c/.h` + hooks in `world.c`: `world_place_shrine`, `world_enable_shrine(on_done)`, `world_find_spot_away(min, max, avoid_tx, avoid_ty, avoid_dist, ...)` = `world_find_far_spot` that keeps clear of another spot. `landed()` places the shrine 16-26 tiles from the landing and at least 18 from Alex (today: tile 40,62, 21 tiles south of the landing); it is a solid prop but inert. Alex's highlighted line (`alex_deal_highlight`) switches it on with `world_enable_shrine(shrine_done)`; the minimap then shows it as a violet block. Next to it the interact button turns into a sludge drop; HOLD it 2.5 s (the button fills with green ooze from the bottom, the shrine shudders, drips and bubbles, the gem goes dark). Let go early and the ooze creeps back over 4 s. When full: `shrine_done()` in `story.c` sets `shrine_fouled`, drops THOUGHT_SHRINE (and `shrine_thought` no longer fires), ticks "Pollute Dia's shrine". The shrine stays polluted and solid, the button goes away. `groundtest` now has a second run (fresh world, skips the talk, calls `alex_deal_highlight` directly) that checks placement, inert-before-deal, range, collision, tap does nothing, early release, full hold). Chunk 11 starts at the comment in `shrine_done()`. Next: chunk 11.
- 2026-10-10: chunk 11 done (`thought_wrath` in `thought.c/.h`, see the code map. `story.c`: `shrine_done()` now sets `dying`, hides the controls (`world_set_controls_visible(0)`), and after `WRATH_BEAT` (0.9 s) `dia_wrath()` shows "HOW DARE YOU" for `WRATH_SECONDS` (3 s); `WRATH_TO_DEATH` (1.6 s) after that, `death_begin()` runs. `think()` returns early while `dying`, so the vessel has no thoughts of its own any more. `death_begin()` is the hook for chunk 12: for now it only gives the controls back so the build stays playable, replace that. `groundtest` checks the beat, the takeover, controls hidden, tapping her card does not open the brain window, own thoughts swallowed, the card folds, `death_begin` runs). Next: chunk 12.
- 2026-10-10: chunk 12 done (see the code map for `gfx_set_filter` and `world_death_*`. `story.c`: `death_begin()` = `world_death_begin(death_ready)`; `death_ready()` is where chunk 13's beeps and chunk 14's heart start, and for now holds the picture `DEATH_HOLD_STANDIN` (2.5 s) and calls `death_undo()` = `world_death_cancel()` so the build stays playable: chunk 15 replaces that stand-in with the "{name} has died." overlay and `story_vessel_died()`. The zoom reuses the talk-zoom path (scene drawn into the zoom texture, middle third stretched, nearest-neighbour), so the 300 % pixels are big and crisp. The mission list, ID card and brain button stay on screen and unfiltered. `groundtest` is now built with `-DGFX_REDIRECT -include game/gfx.h` and checks the zoom curve, the frozen clock, grey map pixels, red player pixels, the chest position and the stand-in undo. `gfxcheck` still gives 15/15 identical CPU/GPU checkpoints). Next: chunk 13.
- 2026-10-10: chunk 12.1 done (hotfix: `brainwin_touch` AUTO release toggles `auto_on` (off keeps the thought where it is); `convo.c` turns the BYE button (`text == NULL` from `dialog_on_reply`) into the player saying "Bye", so the npc answers a goodbye and the chat ends, and `end.silence` is no longer used; `dialog.c` SKIP button renamed BYE; `LOADING_SECONDS` 2.0 -> 1.0 (the menu music fade follows it) and `FADE_IN_T` 0.5 in `world.c` (was 1.0) with `story_after(0.5f, intro)`; `dialog.c` runs a typing clock `ty` at `FAST_X` (2x) while a finger is down (any touch that has not ended), letters and span hooks follow it and blips are `* speed` in pitch (`sfx_blip` max raised 2.5 -> 3.0); `brainwin.c` caption types 2x under a finger too. `convotest` has BYE and 2x-typing checks, `brainwintest` the AUTO toggle). Next: chunk 13.
- 2026-10-10: chunk 13 done (`sfx_beep` / `sfx_flatline` / `sfx_flatline_stop` in `audio.c`, see the code map; `death_ready()` in `story.c` now plays 3 peeps 0.85 s apart (the first at once, when the camera, grey and red have arrived) and the flatline where the 4th would be (`death_flat()`, 2.55 s in); chunk 14 hangs the heart and the explosion on `death_ready()` / `death_flat()` so they line up. The stand-in undo is now `DEATH_HOLD_STANDIN` = `DEATH_FLAT_AT` + 3 s, so the whole tone is heard before the game is given back; chunk 15 replaces it. Music is untouched, it keeps playing under the monitor. `tools/sfxtest.c` runs the mixer by hand (peep ~1 kHz and short, flatline steady for 3 s then silent, early stop, a peep replaces a flatline); `groundtest` checks 0 peeps before arrival, 1 at arrival, 3 + 1 flatline in the end). Next: chunk 14.
- 2026-10-10: chunk 14 done (`game/heart.c/.h`, see the code map. The heart appears over the chest at the first peep and thumps with each one; on the flatline (2.55 s after everything has arrived) it flashes white and bursts into blood, which falls and fades in about 2.4 s. The stand-in undo is unchanged (`DEATH_HOLD_STANDIN`, 5.55 s after arriving), and `world_death_cancel()` wipes the heart and blood, so chunk 15 should put its \"{name} has died.\" overlay in that gap (after the burst, while the blood is still falling or settled) and replace the stand-in with `story_vessel_died()`. `groundtest` checks: no heart before arriving, a red heart on the chest at the first peep (pixels), state BURST with >60 blood pixels together with the flatline, blood visible on screen, all blood gone ~2.4 s later, heart gone after the cancel; it writes `build/ground_heart.bmp`, `ground_burst.bmp`, `ground_burst2.bmp`. Run it from the repo root (from `music/` the bmps land in a folder that does not exist)). Next: chunk 15.
- 2026-10-10: chunk 14.1 done (hotfix: `story.c` - the player's lines never name Dia or say vessel: the task is "Pollute the shrine" (Alex still says Dia's name and her highlight is `{pollute the shrine}`), the player asks for "the orb" without saying whose, Alex answers "Just the orb, huh? Finders keepers."; `coin_thought` and its 9 s timer removed (`THOUGHT_COIN` stays as a tag for tests); `lines_d.c` Alex's joke no longer says "vessel". `thought_say` now replaces whatever card is up (`start()`), queue only while Dia's wrath is up. `ui_panel_w(w, h)` in `font.h` = the toast panel for 18 letters; `toast.c`, `missions.c` and `nowplaying.c` use it as their minimum width and grow past it only for longer text. `alex_deal_done` calls `convo_ask_line(&ALEX, &ALEX_MIND, "Got any questions before you go?", alex_chat_end)`, so the chat starts in the QUESTIONS stage (No ends it, Yes/anything opens the free chat, BYE leaves). `groundtest`, `thoughttest` (replace instead of queue) updated; groundtest, thoughttest 540x1170 + 360x640, toasttest, convotest, brainwintest pass). Left as is: the HUD label "VESSEL ID" on the ID card. Next: chunk 15.
