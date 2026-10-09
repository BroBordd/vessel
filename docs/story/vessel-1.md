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

### 4. The thinking bar (new system)
A **top overlay** with the player's portrait and a line of text: what the player is *thinking*.
It is the game's way of talking to the person playing.

- Not interactive: cannot be tapped or controlled. Touches pass through to the game.
- Comes and goes by itself: slides in, stays a few seconds, slides out. Can be triggered by the story at any time.
- Sits under/next to the ID card and the now-playing card without covering them (see `hud_card_rect`).
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
- [ ] **6. Landing complaint** — after getting up from the fall: "That hurts" + complaint dialog with the player's face, then controls return.
- [ ] **7. Thinking bar (engine)** — `game/thought.c/.h`: `thought_say(text, seconds)`, slides in/out, portrait, non-interactive, doesn't overlap the ID card / now-playing card.
- [ ] **8. First thought + Alex placement** — after landing: "I need to find that orb." Alex spawns far from the landing tile; proximity trigger fires the thought about asking her and the task "Ask Alex".
- [ ] **9. Alex's deal** — Alex's scripted dialog: she has the orb, wants Dia's shrine polluted. Task "Pollute Dia's shrine". Free chat stays after the scripted part.
- [ ] **10. Dia's shrine** — shrine prop on the map, interact → sabotage action, task completes when done.
- [ ] **11. Dia's wrath** — thinking bar variant (red, shaking): "HOW DARE YOU". Short pause, then death starts.
- [ ] **12. Death cutscene I: camera + filters** — freeze world, zoom to 300%, grey everything but the player, red tint on the player.
- [ ] **13. Death cutscene II: sound** — synth sfx: `sfx_beep()` for the monitor "peep"s and `sfx_flatline()` for the long tone (in `audio.c`).
- [ ] **14. Death cutscene III: heart** — pixel heart at the chest, explosion with blood particles, synced with the flatline.
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
- `game/convo.c/.h`, `game/talk.c/.h`, `game/lang*.c` — free typed chat with NPC personas.
- `game/npc.c/.h` — static NPCs (`npc_add`, facing). No movement/removal yet.
- `game/missions.c/.h` — top-left checklist (`mission_add`, `mission_complete`, `missions_bottom`).
- `game/toast.c/.h` — "New task added" toast under the checklist: `task_toast("Find the orb")` = `mission_add` + coin ding + toast (extra toasts queue).
- `game/hud.c/.h` — ID card (`hud_set_person`, `hud_set_hp`, name-change toast).
- `game/world.c/.h` — maps, camera, hole, fall (`world_fall_to_green`).
- `game/audio.c/.h` — music + synthesized sfx (`sfx_coin`, `sfx_blip`).

## Status log

- 2026-10-09: story written, chunks planned.
- 2026-10-09: chunk 2 done (GRASSLANDS / SKYLAND_NAME + world_map_name in world.h; Alex placeholder chat and spawn removed from story.c, `landed()` is an empty stub). Next: chunk 3.
- 2026-10-09: chunk 3 done (`{...}` markup in `DialogLine.text` draws green + underlined, spans may wrap over lines; `dialog_on_highlight(fn(page, span))` fires once when a span has been typed out; tested with `tools/dialogtest.c`). Next: chunk 4.
- 2026-10-09: chunk 4 done (`task_toast(text)` in `game/toast.c`: adds the mission, plays the coin ding, drops a NEW TASK ADDED panel under the checklist, queues up to 4; `hud_draw_coin` is now shared; `tools/toasttest.c` renders it on a PC). Next: chunk 5.
- 2026-10-09: chunk 5 done (`DEA_TALK` has a new last page: "Now, your first mission. I {dropped my orb in the Grasslands}. Find it and bring it back to me."; `dea_talk_highlight` in `story.c` calls `task_toast("Find the orb")` once that span has typed out; page count is now `DEA_TALK_COUNT`). Next: chunk 6.
