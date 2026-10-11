# Vessel 2, part 2 — "The Orb, Again" (Doia's mission, and vessel 3)

> **Handoff file.** If you are an LLM picking this up mid-work: read this whole file, then skim
> `vessel-1.md` and `vessel-2.md` (their code maps are still true; `vessel-2.md` ends at chunk 24.1 and
> this file continues the numbering at 25). Find the first unchecked chunk in
> [Chunk checklist](#chunk-checklist), implement ONLY that chunk, commit, tick its box in this file
> **in the same commit**, push, then go to the next one.
> Do not skip ahead, do not batch chunks. The owner tests every push on their phone
> (`git pull` + `scripts/run.sh`), so every chunk must leave the game building and playable.

## Workflow rules

Same as vessel 1 and 2:

- One chunk = one commit = one push. Commit message: `chunk N: <short title>` + a paragraph of what changed.
- Tick the chunk's checkbox below in the same commit that implements it.
- `game/story.c` stays the single place where story text and scripted events live. Engine pieces get their
  own `game/*.c` + `.h` pair with the GPL header.
- Match the existing code style (C, SDL2, short explanatory comments).
- Strings shown to the player must be proper English (the drafts below are rough on purpose; polish them).
- Never commit tokens or secrets.
- Everything that draws goes through the renderer calls the game already uses (`GFX_REDIRECT`): **no
  `SDL_BLENDMODE_MOD`/`ADD`**, use `gfx_set_filter` or alpha instead. Pixel art only: square cells, `pdisc`
  for circles, no smooth curves.
- Sanity check: `cc -O1 -DGFX_REDIRECT -include game/gfx.h game/*.c $(sdl2-config --cflags --libs) -lm` must build,
  and `tools/groundtest.c` (line in its header) must still pass. Add a headless check to the tests for every
  behaviour you add. `tools/preview.c` renders screenshots without a phone.
- The game is open source and meant to be **semi horror**. Dark is fine. Keep it in the game's own style: stylised pixel
  art, the heart monitor, grey filters. No graphic detail beyond what Aonia's death already shows, except Alex's death (story section 4): the detached head and the endless red spark idle, drawn as stylised pixel squares only.

## Cast and names

| Who | Notes |
|---|---|
| **Vas** | The soul. **The same person in every body, with every memory** (see `vessel-2.md`). Doia is Aonia's soul in a new body, not a stranger. Every thought is first person: "she sent *me* to die". |
| **Aonia** | Vessel one. Dead. His body lies in the grave on the Grasslands. It still has the **hammer** he broke the shrine with. |
| **Doia** | Vessel two. The one this file is about. Same soul, `VESSEL2` look. |
| **Tria** | Vessel three (Dea plays on the word "three"). Appears when Doia dies at the end (chunk 42). New `Person` `VESSEL3`. |
| **Dea** | The goddess. Gives the orders. Toxic as ever. Wants her **orb** back and wants it **kept quiet**. |
| **Alex** | Lives in the house on the Grasslands. Has the orb and says it is **hers**. She does not know who Doia is. |
| **Dia** | The rival goddess. Her shrine is broken (stump, rubble). Silent in this part. |

Always call it **the orb** (never "the ball").

## Story rules

- **Dea's orders are followed.** She told us to be secretive. We **cannot complain and cannot tell anyone**. Typed chat is
  closed with everyone while the order stands (chunk 27); the vessel only *thinks* what it cannot say.
- **The soul remembers**: Aonia's death, the shrine, Alex's deal. It knows the grave is its own old body.
- The hammer is the story's running object: it broke the shrine, it is dug out of the grave, it kills Alex, it repairs the
  shrine. The thought "This hammer surely can do everything, huh." is the joke on that.

## Story, start to finish

### 1. The orders (in the sky)
At the end of Dea's scolding (the naming of Doia, `vessel-2.md` chunk 18) she gives the job: take the orb from Alex's house,
**tell no one, do not complain**. The ID card says DOIA. Then the hole opens and we fall (unchanged, chunk 20).

### 2. Alex says no
On the Grasslands the task **"Get the orb from Alex"** is already on the list. Walk to her house, enter, ask her. She
refuses, the orb is hers, and she does not care who we are. We cannot argue or explain (chat is closed): all we have are our thoughts.
The vessel thinks it needs something to persuade her. The hammer is with the one who broke the shrine: **with its own old body**.

### 3. The grave, and respect for the dead
Outside, find the grave (it already stands where Aonia died). Hold the interact button at it: we **pull his body out of the grave by
the legs**, feet first, and lay him on the grass beside it. The plot stays open. The task **"Bury Aonia"** coins in.
The interact button next to the body becomes a **magnifying glass (search)**. Searching him: **the hammer falls out of his hand
onto the ground** next to him.
We are not allowed to take it yet. Walking up to pick the hammer up **flashes the task "Bury Aonia" red and shakes it, with an error
sound**, and the hammer stays where it is: **first we put him back, out of respect for the dead.** Holding the button at the body
now **drags him back into his grave** and the plot closes. Task done. The hammer can be picked up now. Thoughts: "That's me."
and, as we take the hammer, "Rest well. I need this."

### 4. Alex
Back in her house: hammer out, a swing button next to her. She is hit again and again, says what people say, falls.
Her death uses the same stylised language as Aonia's: freeze, red tint on her, flatline, the words **"Alex has died."**
She does not get a grave. The orb does **not** drop by itself: it is still on her. Next to her body the interact button becomes the
**magnifying glass (search)** again (the same icon as on Aonia's body). A tap searches her and **takes the orb out**. We have it. The task
"Get the orb from Alex" is done and the next goal ("Repair the shrine") simply coins in: **silently**, no thought, no comment, no
music sting. The vessel just gets on with it.

**How she dies (the head).** The killing blow is a hit on her **head**: the last swing knocks her head **away from the
body**, it flies off and comes to rest a short way from her. Her body falls and lies on the floor of the house.
From the neck of the lying body, **small red sparks keep spraying out forever** as its idle animation (the room is never
quiet again; this runs for as long as the house is loaded). Each spray makes the body **shake** a little and plays a
**watery "pfft"**, like squeezing a plastic bottle with a pinhole in it: a thin jet of drops squirts out in a
**straight line**, sparkling, then drops back down. The sparks are small pixel squares (pixel art only, no smooth curves),
red on the grey of the death scene, in short rhythmic bursts (squeeze, pfft, pause, squeeze, pfft). Stylised, never
detailed: sparks and a shaking body, no anatomy.

### 5. The shrine
New task **"Repair the shrine"**. The broken shrine is on the map. Hold the interact button with the hammer: it
**mends** (the break played backwards). While we do it the vessel thinks: **"This hammer surely can do everything, huh."**

### 6. Die
The last task: **"Die"**. The mended shrine takes us: hold the button, the vessel's HP drains, the death cutscene runs (Doia has
died. Her grave is placed). **The orb stays in the pockets across the death** (the one thing that survives). In limbo:
"Uh. I died. Again." We are summoned as **vessel 3, Tria**, in the clouds. Dea is in her temple.

### 7. The orb goes to Dea
Walk up to her temple with the orb. Hand it over (a short scripted talk). That is the end of this part
(`story_vessel3_ready()` is the hook for whatever comes next).

## Draft strings (polish before shipping)

- Dea, orders (as `DEA`, to VAS): "Alex has my orb. Go to her house and take it." / "Do not tell anyone. Do not complain. Not a word."
- Chat lock thought: "I can't say that. I was told to keep quiet." (Dea's own chat: "You heard me. Go.")
- Task: "Get the orb from Alex"
- Alex, refusing: "The orb? It's mine. I found it. No." / "Why do you want it so badly?" (we cannot answer)
- Thought after: "She won't give it up. I need something heavy to change her mind." / "I know where there is one."
- Grave, pulling him out: "I'm pulling my own body out of the ground." / task: "Bury Aonia"
- Search: "He still has it. The hammer." (it falls on the ground)
- Pick up too early: error sound, the task "Bury Aonia" flashes red and shakes; thought: "Not like this. He goes back first."
- Buried: "Rest well." / taking the hammer: "I need this. I'm sorry."
- Alex hit (between hits): "Hey! What are you doing?" / "Stop it!" / "Please!" / "It's just an orb..."
- Death text: "Alex has died."
- After taking the orb: nothing. No thought, no line. (Silent on purpose.)
- Task: "Repair the shrine" / repairing thought: "This hammer surely can do everything, huh."
- Task: "Die" / limbo: "Uh. I died. Again."
- Naming vessel 3: "Fine. You are number three now. Tria."
- Giving the orb: "Here. Your orb." / Dea: "Finally. And no one knows?"

## Code map (what this plan builds on, from `vessel-1.md` and `vessel-2.md`)

- `story.c`: `think()`, `task_toast()`, `convo_ask_line`, `dialog_play`, `story_after`, the `green` struct (Grasslands as vessel 1 left them,
  `alex_there`), `graves[]` / `grave_n`, `lives`, `DEATH_LINES[]` (one entry per life), `story_reset_for_respawn()`, `landed_again()`,
  `story_vessel2_ready()` (the hook of this part, empty today), `on_talk_alex` (branches on `alex_dealt`).
- `shrine.c/.h`: the break (hold the interact button, cracks, stump, rubble). Chunk 40 plays it backwards.
- `grave.c/.h` + `world_place_grave`: stone, plot, portrait. Chunks 30-32 add the pull, the open plot, the body and the search; chunk 34 buries him again.
- `item.c/.h`: `ITEM_HAMMER`, inventory under the minimap, `item_give/has/clear`. `world_clear_items()` empties the pockets on a new life
  (chunk 41 keeps the orb). Chunk 33 adds `item_set_locked(item, on)`: a locked item on the ground cannot be picked up and says so.
- `missions.c/.h`: the checklist. Chunk 33 adds `mission_flash(id)`: the line flashes red and shakes, for a short time. `audio.c`: chunk 33 adds `sfx_error()`.
- `npc.c/.h`: static npcs only (no motion, no removal). Chunks 36-38 add hit flashes, a lying pose and removal.
- `world.c`: `world_death_*` (the death camera: zoom, grey, red on the player), `world_find_far_spot`, `world_house_*`, `world_set_player`.
- `thought.c`, `brainwin.c`: thoughts and their pictures (`BRAIN_SCENE_GRAVE`, `_HAMMER`, `_HEAVY`, `_ORB` exist and can be reused).
- `hud.c`: `hud_set_hp`, `hud_set_person`.

## Chunk checklist

Mark `[x]` when the chunk is pushed.

- [x] **25. Part 2 doc** — this file.

### Dea's orders and the secret
- [x] **26. Dea's orders** — `DEA_ORDERS` lines appended to the scolding after the naming (before her questions): take the orb from Alex, tell no one, do not complain. `secret_on = 1` in `story.c`. `groundtest` checks the lines run after the naming and the flag is set.
- [ ] **27. The chat lock** — `convo_lock(const char *thought)` / `convo_unlock()` in `convo.c`: while locked `convo_open` and `convo_ask_*` do not open the typed chat; the vessel *thinks* the lock line instead (Dea's own chat says "You heard me. Go."). `story.c` locks it with the orders. Scripted dialogs are not affected. Check: talk to Alex / Dea while locked, no chat window, thought appears.
- [ ] **28. The task** — `story_vessel2_ready()`: after the landing and the grave thought, the task **"Get the orb from Alex"** coins in (`task_toast`). Alex must not replay vessel 1's deal (`alex_dealt` stays set). Check: task exists after the second landing and not before.

### Alex says no
- [ ] **29. Alex refuses** — `on_talk_alex` for `lives >= 1` plays `ALEX_REFUSE` (scripted, her own face popup), ends with no free chat (locked) and no question. Then the thoughts: she won't give it, the hammer is with the old body. Brain picture: `BRAIN_SCENE_HEAVY`. Check: the refusal plays once, the thoughts come after it.

### The grave, the body and the hammer
- [ ] **30. Pull him out (interaction)** — engine: `world_enable_grave_pull(i, on_done)` (like `world_enable_shrine`): the interact button shows near that grave and HOLDING it fills a bar like the shrine's; letting go early makes it creep back; when full, `on_done()`. Reuse the hold code. Check: near the grave the button shows; hold = done.
- [ ] **31. The open plot and the body** — `grave.c`: `grave_open(i)` opens the earth plot, and the body (Aonia's look, the lying pose the landing already uses, greyed with `gfx_set_filter`) is **drawn sliding out of the plot feet first** over about 1.5 s, then lying on the grass beside it. `grave_close(i)` plays it backwards. The stone and portrait stay. Pixel art. Screenshot via `tools/preview.c`. `story.c`: when the pull is done, the grave opens and the task **"Bury Aonia"** (`VESSEL_NAME`) coins in.
- [ ] **32. Search him** — the interact button next to the body shows a **magnifying glass** (a new 12x12 icon in `item.c`'s style, used by the button): a tap searches (about 1 s), the hammer **falls onto the ground** next to him (`item_place(ITEM_HAMMER, ...)`) with a glint and a thought ("He still has it. The hammer."). The search happens once. Check: no hammer before the search, hammer on the ground after.
- [ ] **33. Respect the dead** — `item_set_locked(ITEM_HAMMER, 1)` while he is unburied: walking over the hammer does not pick it up; instead `mission_flash(task)` (the "Bury Aonia" line turns red and shakes for about 0.6 s), `sfx_error()` (a short low double buzz in `audio.c`), and a thought ("Not like this. He goes back first.") (at most once every few seconds). Check: the hammer is not in the pockets after walking over it; the flash and the sound are counted (`sfx_debug_errors()`).
- [ ] **34. Bury him** — the interact button next to the body (after the search) becomes a grave/hand icon: HOLD it and he is **dragged back into the plot** (the pull played backwards, `grave_close`). The task "Bury Aonia" completes. `item_set_locked(ITEM_HAMMER, 0)`. Check: hold = buried, the plot is closed, the hammer is free.
- [ ] **35. The hammer is ours** — walking over the hammer picks it up (inventory slot unfolds as in vessel 1), thought "Rest well. I need this. I'm sorry." (`BRAIN_SCENE_HAMMER`). The task stays "Get the orb from Alex". Check: pockets hold the hammer only after the burial.

### Beating Alex
- [ ] **36. The swing** — engine: with the hammer, an npc marked hostile (`npc_set_hostile(id, 1)`) makes the interact button a hammer swing; each tap hits: the npc flashes white and is shoved one pixel unit back, a hit sound, a short cooldown (`world_enable_fight(npc_id, hits, on_down)`). Check: N taps call `on_down` once.
- [ ] **37. Alex fights back** — Alex is hostile once the hammer is in her house with us. A small pixel health bar over her; she says one `ALEX_HURT[]` line between hits (face popup, tap to go on) and stays on her feet until the last. Check: bar goes down per hit; lines in order.
- [ ] **38. Alex falls** — `npc_set_lying(id)` (the lying pose) and `world_death_begin_at(npc_id)`: the death camera on her (zoom, grey on the scene, red on her), the peeps, the flatline, the words **"Alex has died."** (`world_death_text`), then the world is free again. Her body stays lying in the house, no grave. **The last hit knocks her head off the body** (a separate head sprite flies a few tiles and lies still), and the body then plays the endless idle: red spark bursts from the neck in a straight line, the body shakes a little, a watery "pfft" sound (`sfx_pfft()` in `audio.c`: short filtered noise burst, falling pitch) on every burst (see story section 4). Check: camera target is the npc, controls return afterwards; the head is detached from the body; the spark idle keeps running (burst counter goes up over time, `sfx_debug_pfft()`).

### The orb
- [ ] **39. Taking the orb** — `ITEM_ORB` (icon + name in `item.c`, a violet-gold glowing ball like `draw_orb_ball` in `brainwin.c`); no orb on the ground: next to Alex's lying body the interact button shows the **magnifying glass** (reuse chunk 32's icon and search timing); a tap searches her once and puts the orb straight into the pockets (inventory slot unfolds). The task "Get the orb from Alex" is done and **"Repair the shrine"** coins in **silently**: no thought, no dialog, no extra sound beyond the normal task toast. Check: no orb before the search, orb in pockets after, task swap, no thought queued.

### The shrine
- [ ] **40. Repair** — `shrine.c`: `shrine_repair` plays the break backwards (cracks heal, the stump grows, the rubble lifts, the gem lights again) while the interact button is HELD with the hammer. `world_enable_shrine_repair(on_done)`. The first time the hold starts, the thought **"This hammer surely can do everything, huh."** (`BRAIN_SCENE_HAMMER`). Check: progress reaches 100% only by holding; the shrine draws whole afterwards.

### Dying and vessel 3
- [ ] **41. "Die"** — when the repair is done the task **"Die"** coins in. At the mended shrine the interact button shows a heart: holding it drains the HP bar (`hud_set_hp`) to 0, then the existing death cutscene (chunks 12-15 of vessel-2) runs with `lives = 2`; `DEATH_LINES[1]` = "Uh. I died. Again."; Doia's grave is placed at the death tile (`grave_n`). **`story_reset_for_respawn()` keeps `ITEM_ORB`** and empties the rest of the pockets. Check: after the respawn the orb is in the pockets, nothing else.
- [ ] **42. Vessel 3: Tria** — `VESSEL3` `Person` (a clearly different look from Aonia and Doia) and `VESSEL3_NAME "Tria"`; summoned in the clouds again (`world_summon`), `DEA_NAMING3` ("Fine. You are number three now. Tria.") with the ID card flash and `world_set_player(&VESSEL3)`. Dea stands in her temple. Check on a screenshot that the three look different.

### The end
- [ ] **43. Giving the orb** — talking to Dea with `ITEM_ORB` in the pockets plays `DEA_ORB_TAKE`: the orb leaves the pockets, the task completes. Without the orb she nags. Hook `story_vessel3_ready()` (empty stub, like `story_vessel2_ready`).
- [ ] **44. Wrap-up** — the thoughts that no longer make sense after the new flow are removed or retagged, the brain window keeps what the soul remembers, `groundtest` runs the whole part in one headless pass (orders, refusal, dig, hammer, fight, orb, repair, die, vessel 3, orb to Dea). Update this file's status log.

## Open decisions (defaults so nobody has to wait)

- **How Doia dies at the end:** the repaired shrine drains us on a hold (chunk 41). Alternative: Dea's voice orders it and we simply fall. Default is the shrine.
- **Does Alex hurt us:** no. She only talks and takes the hits (chunk 37). Fighting back would need npc motion.
- **No grave for Alex.** Her body stays lying in the house.
- **Task list stays secret-quiet:** no task says "kill". The only tasks are "Get the orb from Alex", "Repair the shrine", "Die" (and "Give the orb to Dea" if wanted in chunk 40).
- **Chat lock ends** once the orb is handed over (chunk 43) or when vessel 3 starts, whichever the owner prefers. Default: it ends with the orb handover.
- **Dia** says nothing in this part. Repairing her shrine is just a thing the hammer does.

## Status log

- 2026-10-11: chunk 25 done: this file.
- 2026-10-11: the grave scene reworked: we pull the old body out by the legs, search him (magnifying-glass button) so the hammer falls to the ground, picking it up too early flashes and shakes the task "Bury Aonia" in red with an error sound, and the body has to be buried again first. Chunks 30-35 replace the old 30-32; the later chunks are renumbered 36-44.
- 2026-10-11: story section 4 extended: Alex dies by a hit to the head that knocks it off the body; her body lies on the floor with an endless idle of small red sparks from the neck (straight-line squirts, body shakes, watery pfft sound like squeezing a pinholed bottle). Chunk 38 updated to match.
- 2026-10-11: Alex's orb is taken by searching her body (magnifying-glass button), not dropped; afterwards the next goal comes in silently, no thought. Story section 4, draft strings and chunk 39 updated.
- 2026-10-11: chunk 26 done: `DEA_ORDERS` (two lines) play right after the naming and before her questions (`orders_given` in `story.c`), `secret_on = 1` is set then and cleared on a new game. New headless test `tools/orderstest.c` (naming, then orders, flag off before / on after, then the questions). Note: the long `tools/groundtest.c` chain was already failing at HEAD (60 FAILs + a segfault, stale against the cloud map / Alex's house); only its compile errors were fixed here and the chunk 26 checks added, so it is not a usable gate until it is repaired.
