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
  art, the heart monitor, grey filters. No graphic detail beyond what Aonia's death already shows.

## Cast and names

| Who | Notes |
|---|---|
| **Vas** | The soul. **The same person in every body, with every memory** (see `vessel-2.md`). Doia is Aonia's soul in a new body, not a stranger. Every thought is first person: "she sent *me* to die". |
| **Aonia** | Vessel one. Dead. His body lies in the grave on the Grasslands. It still has the **hammer** he broke the shrine with. |
| **Doia** | Vessel two. The one this file is about. Same soul, `VESSEL2` look. |
| **Tria** | Vessel three (Dea plays on the word "three"). Appears when Doia dies at the end (chunk 39). New `Person` `VESSEL3`. |
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

### 3. The grave
Outside, find the grave (it already stands where Aonia died). Hold the interact button at it to **dig** (like breaking the shrine).
The plot opens and **Aonia's body** lies there, grey. The hammer is still in his hand. We take it
(the pockets show the hammer again). Thought: "That's me. And he kept the hammer."

### 4. Alex
Back in her house: hammer out, a swing button next to her. She is hit again and again, says what people say, falls.
Her death uses the same stylised language as Aonia's: freeze, red tint on her, flatline, the words **"Alex has died."**
She does not get a grave. The **orb** drops. We pick it up. The task is done, nobody has seen.

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
- Grave, digging: "I'm digging up my own grave." / found: "That's me. He kept the hammer."
- Alex hit (between hits): "Hey! What are you doing?" / "Stop it!" / "Please!" / "It's just an orb..."
- Death text: "Alex has died."
- After: "Nobody saw. Dea will be pleased."
- Task: "Repair the shrine" / repairing thought: "This hammer surely can do everything, huh."
- Task: "Die" / limbo: "Uh. I died. Again."
- Naming vessel 3: "Fine. You are number three now. Tria."
- Giving the orb: "Here. Your orb." / Dea: "Finally. And no one knows?"

## Code map (what this plan builds on, from `vessel-1.md` and `vessel-2.md`)

- `story.c`: `think()`, `task_toast()`, `convo_ask_line`, `dialog_play`, `story_after`, the `green` struct (Grasslands as vessel 1 left them,
  `alex_there`), `graves[]` / `grave_n`, `lives`, `DEATH_LINES[]` (one entry per life), `story_reset_for_respawn()`, `landed_again()`,
  `story_vessel2_ready()` (the hook of this part, empty today), `on_talk_alex` (branches on `alex_dealt`).
- `shrine.c/.h`: the break (hold the interact button, cracks, stump, rubble). Chunk 37 plays it backwards.
- `grave.c/.h` + `world_place_grave`: stone, plot, portrait. Chunks 30-31 add the dig and the open plot.
- `item.c/.h`: `ITEM_HAMMER`, inventory under the minimap, `item_give/has/clear`. `world_clear_items()` empties the pockets on a new life
  (chunk 38 keeps the orb).
- `npc.c/.h`: static npcs only (no motion, no removal). Chunks 33-35 add hit flashes, a lying pose and removal.
- `world.c`: `world_death_*` (the death camera: zoom, grey, red on the player), `world_find_far_spot`, `world_house_*`, `world_set_player`.
- `thought.c`, `brainwin.c`: thoughts and their pictures (`BRAIN_SCENE_GRAVE`, `_HAMMER`, `_HEAVY`, `_ORB` exist and can be reused).
- `hud.c`: `hud_set_hp`, `hud_set_person`.

## Chunk checklist

Mark `[x]` when the chunk is pushed.

- [x] **25. Part 2 doc** — this file.

### Dea's orders and the secret
- [ ] **26. Dea's orders** — `DEA_ORDERS` lines appended to the scolding after the naming (before her questions): take the orb from Alex, tell no one, do not complain. `secret_on = 1` in `story.c`. `groundtest` checks the lines run after the naming and the flag is set.
- [ ] **27. The chat lock** — `convo_lock(const char *thought)` / `convo_unlock()` in `convo.c`: while locked `convo_open` and `convo_ask_*` do not open the typed chat; the vessel *thinks* the lock line instead (Dea's own chat says "You heard me. Go."). `story.c` locks it with the orders. Scripted dialogs are not affected. Check: talk to Alex / Dea while locked, no chat window, thought appears.
- [ ] **28. The task** — `story_vessel2_ready()`: after the landing and the grave thought, the task **"Get the orb from Alex"** coins in (`task_toast`). Alex must not replay vessel 1's deal (`alex_dealt` stays set). Check: task exists after the second landing and not before.

### Alex says no
- [ ] **29. Alex refuses** — `on_talk_alex` for `lives >= 1` plays `ALEX_REFUSE` (scripted, her own face popup), ends with no free chat (locked) and no question. Then the thoughts: she won't give it, the hammer is with the old body. Brain picture: `BRAIN_SCENE_HEAVY`. Check: the refusal plays once, the thoughts come after it.

### The grave and the hammer
- [ ] **30. Dig interaction** — engine: `world_enable_grave_dig(i, on_done)` (like `world_enable_shrine`): the interact button shows near that grave and HOLDING it fills a bar like the shrine's; letting go early makes it creep back; when full, `on_done()`. Reuse the hold code. Check: near the grave the button shows; hold = done.
- [ ] **31. The open plot and the body** — `grave.c`: `grave_open(i)` draws the dug earth and a grey body in the plot (Aonia's look, the lying pose the landing already uses). The stone and portrait stay. Pixel art, grey filter. Screenshot via `tools/preview.c`.
- [ ] **32. The hammer** — in `story.c` the dig's `on_done` opens the grave, thinks the grave lines, gives `ITEM_HAMMER`, and shows the inventory slot (`BRAIN_SCENE_HAMMER`). Task stays "Get the orb from Alex". Check: pockets hold the hammer after the dig, not before.

### Beating Alex
- [ ] **33. The swing** — engine: with the hammer, an npc marked hostile (`npc_set_hostile(id, 1)`) makes the interact button a hammer swing; each tap hits: the npc flashes white and is shoved one pixel unit back, a hit sound, a short cooldown (`world_enable_fight(npc_id, hits, on_down)`). Check: N taps call `on_down` once.
- [ ] **34. Alex fights back** — Alex is hostile once the hammer is in her house with us. A small pixel health bar over her; she says one `ALEX_HURT[]` line between hits (face popup, tap to go on) and stays on her feet until the last. Check: bar goes down per hit; lines in order.
- [ ] **35. Alex falls** — `npc_set_lying(id)` (the lying pose) and `world_death_begin_at(npc_id)`: the death camera on her (zoom, grey on the scene, red on her), the peeps, the flatline, the words **"Alex has died."** (`world_death_text`), then the world is free again. Her body stays lying in the house, no grave. Check: camera target is the npc, controls return afterwards.

### The orb
- [ ] **36. Taking the orb** — `ITEM_ORB` (icon + name in `item.c`, a violet-gold glowing ball like `draw_orb_ball` in `brainwin.c`); it drops where she fell (`item_place`), walking over it picks it up; the task "Get the orb from Alex" is done and **"Repair the shrine"** coins in; thought: "Nobody saw. Dea will be pleased." Check: orb in pockets, task swap.

### The shrine
- [ ] **37. Repair** — `shrine.c`: `shrine_repair` plays the break backwards (cracks heal, the stump grows, the rubble lifts, the gem lights again) while the interact button is HELD with the hammer. `world_enable_shrine_repair(on_done)`. The first time the hold starts, the thought **"This hammer surely can do everything, huh."** (`BRAIN_SCENE_HAMMER`). Check: progress reaches 100% only by holding; the shrine draws whole afterwards.

### Dying and vessel 3
- [ ] **38. "Die"** — when the repair is done the task **"Die"** coins in. At the mended shrine the interact button shows a heart: holding it drains the HP bar (`hud_set_hp`) to 0, then the existing death cutscene (chunks 12-15 of vessel-2) runs with `lives = 2`; `DEATH_LINES[1]` = "Uh. I died. Again."; Doia's grave is placed at the death tile (`grave_n`). **`story_reset_for_respawn()` keeps `ITEM_ORB`** and empties the rest of the pockets. Check: after the respawn the orb is in the pockets, nothing else.
- [ ] **39. Vessel 3: Tria** — `VESSEL3` `Person` (a clearly different look from Aonia and Doia) and `VESSEL3_NAME "Tria"`; summoned in the clouds again (`world_summon`), `DEA_NAMING3` ("Fine. You are number three now. Tria.") with the ID card flash and `world_set_player(&VESSEL3)`. Dea stands in her temple. Check on a screenshot that the three look different.

### The end
- [ ] **40. Giving the orb** — talking to Dea with `ITEM_ORB` in the pockets plays `DEA_ORB_TAKE`: the orb leaves the pockets, the task completes. Without the orb she nags. Hook `story_vessel3_ready()` (empty stub, like `story_vessel2_ready`).
- [ ] **41. Wrap-up** — the thoughts that no longer make sense after the new flow are removed or retagged, the brain window keeps what the soul remembers, `groundtest` runs the whole part in one headless pass (orders, refusal, dig, hammer, fight, orb, repair, die, vessel 3, orb to Dea). Update this file's status log.

## Open decisions (defaults so nobody has to wait)

- **How Doia dies at the end:** the repaired shrine drains us on a hold (chunk 38). Alternative: Dea's voice orders it and we simply fall. Default is the shrine.
- **Does Alex hurt us:** no. She only talks and takes the hits (chunk 34). Fighting back would need npc motion.
- **No grave for Alex.** Her body stays lying in the house.
- **Task list stays secret-quiet:** no task says "kill". The only tasks are "Get the orb from Alex", "Repair the shrine", "Die" (and "Give the orb to Dea" if wanted in chunk 40).
- **Chat lock ends** once the orb is handed over (chunk 40) or when vessel 3 starts, whichever the owner prefers. Default: it ends with the orb handover.
- **Dia** says nothing in this part. Repairing her shrine is just a thing the hammer does.

## Status log

- 2026-10-11: chunk 25 done: this file.
