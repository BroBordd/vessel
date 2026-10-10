# Vessel 2 — "Limbo" (death, rebirth, the second vessel)

> **Handoff file.** If you are an LLM picking this up mid-work: read this whole file (and skim
> `vessel-1.md`, its code map is still true), find the first unchecked chunk in
> [Chunk checklist](#chunk-checklist), implement ONLY that chunk, commit, tick its box in this file
> **in the same commit**, push, then go to the next one.
> Do not skip ahead, do not batch chunks. The owner tests every push on their phone
> (`git pull` + `scripts/run.sh`), so every chunk must leave the game building and playable.

## Workflow rules

Same as vessel 1:

- One chunk = one commit = one push. Commit message: `chunk N: <short title>` + a paragraph of what changed.
- Tick the chunk's checkbox below in the same commit that implements it.
- `game/story.c` stays the single place where story text and scripted events live. Engine pieces get their
  own `game/*.c` + `.h` pair with the GPL header.
- Match the existing code style (C, SDL2, short explanatory comments).
- Strings shown to the player must be proper English (the drafts below are rough on purpose; polish them).
- Never commit tokens or secrets.
- Everything that draws goes through the renderer calls the game already uses (`GFX_REDIRECT`): **no
  `SDL_BLENDMODE_MOD`/`ADD`** (the GPU path only knows none/alpha, a multiply blend paints solid white),
  use `gfx_set_filter` or alpha instead. Pixel art only: square cells, `pdisc` for circles, no smooth curves.
- Sanity check: `cc -O1 -DGFX_REDIRECT -include game/gfx.h ... tools/groundtest.c ...` (line in its header) must
  still pass, and `tools/preview.c` can render screenshots without a phone. Add a headless check to the
  tests for every behaviour you add.

## Cast and names

| Who | Notes |
|---|---|
| **Vas** | The soul, before it gets a body or a name. Also what the ID card says in between. |
| **Aonia** | Vessel one. Dead. Gets a grave (with his picture on it) on the Grasslands, where he died. |
| **Doia** | Vessel two. Same job, different look (new `Person`, see chunk 17). Dea plays on the word "two" like she did with "one". |
| **Dea** | The goddess. She is still toxic. She scolds Vas for dying and hands out number two. |
| **The soul** | A new `Person` (`SOUL`): a pale, ghost-blue version of the player. Only its *face* is used: the portrait on the thought card while the player is in limbo. |

Later vessels (Tria, Ceathia ...) must be cheap to add. Keep what changes per vessel in one table in
`story.c` (the `Person`, its name, its death thoughts) and write the limbo, summon and grave pieces as
engine code that does not know which vessel it is.

## Story, start to finish

### 1. LIMBO: the space screen (new, used at the very start and after every death)
The game already has the right background: the menu's **black field with white pixel "stars" drifting past**.
We reuse it as **limbo**: nothing but the drifting stars, the top buttons, and the thought card.
- No map, no ID card, no mission list, no minimap, no stick, no pause button. The brain button works.
- The stars keep moving all the time (and the player's own thoughts type into the brain card, as usual).
- The thought card wears the **soul's face** (`SOUL`), not the ID card person.

### 2. Beginning of the game (changed)
- On **PLAY** the menu does not go to a loading screen any more. The **logo and both buttons fade out**
  (about 0.8 s) over the same moving stars, and **the game begins from that frame**: we are in limbo.
- The soul thinks: **"Where am I?"** After a few seconds the **summoning** starts (chunk 8-9): a column of
  golden light on the cloud island, golden particles gathering, the vessel forming out of the light, a
  rising chime.
- The old welcome windows are **gone**: no "Hello, Vas!", no "You have been summoned. Talk to Goddess."
  popup and no toast. We just stand there, controls back, and have to **look around**.
- A couple of seconds later the player thinks **"Is this where dead people go?"** When we **see Dea** (she
  stands 8 tiles above, looking down: a proximity/sight trigger like Alex's) the task **"Talk to Goddess"**
  coins in and a thought clears it up (e.g. "Someone is up there.").
- Everything after that (Dea names us Aonia, the orb, Alex, the shrine, the death) is vessel 1, unchanged.
  Thoughts keep explaining things to the player; add them wherever the screen is confusing.

### 3. The death, and the way back
Vessel 1's death cutscene ends as it does today ("Aonia has died.", chunk 15), then:
1. The world fades to black into **limbo** (same space screen as the start). HUD gone.
2. The soul thinks: **"Uh. I died."** (a short beat, then the soul waits a few seconds more.) Every later
   death has its own line(s) in the vessel table.
3. **Respawn in the cloud land**: the world fades in on the sky island, the player is **summoned again with
   the golden effect**. Dea is already there, facing us. The ID card says **VAS** again (name reset), HP full,
   the mission list and the brain window are empty again.
4. **Dea scolds us** at once (she speaks first, no walking): she is not impressed that we died, makes it clear
   it was our own fault, then **gives us a new number: Doia (two)**. At the naming the ID card flashes
   ("YOU ARE NOW DOIA", coin ding) and **the player sprite changes to the new look**.
5. Dea sends us down again. The hole opens, we jump, we fall into the Grasslands.
6. The Grasslands are **as vessel 1 left them** (Alex, the polluted shrine, nobody resets the map), plus a
   **grave where Aonia died**, a stone with **Aonia's portrait** on it.

**Out of scope here:** vessel 2's own mission. After the landing and the grave the game just lets us
walk (`story_vessel2_ready()` is the hook, empty for now). The owner will plan the mission later.

## Draft strings (polish before shipping)

- Start, limbo: "Where am I?"
- Sky, a few seconds after we can walk: "Is this where dead people go?"
- Sky, Dea sighted: "Someone is up there." / task: "Talk to Goddess"
- Death, limbo: "Uh. I died." (later deaths: see the table)
- Dea, scolding (all as DEA, to VAS):
  - "Oh. You again. Dead after a single errand."
  - "You pollute a rival goddess's shrine for a girl and a ball. My ball."
  - "I do not care that it hurt. There is a reason you are replaceable."
  - "Fine. You are number two now. Doia. Try not to make me remember it."
- Landing as vessel 2: "Here we go again." (placeholder; the owner may rewrite)
- Grave, first time we come near it: "Aonia. Whoever that was, he looked like me."

## Code map (what exists today that this plan builds on)

- Menu: `game/menu.c/.h`. Black clear + 170 white square flakes drifting left in the wind (`Flake`, `wind()`,
  `frand`), the "VESSEL" title in the font, two buttons (PLAY, EXIT). `menu_touch` returns `MENU_PLAY/EXIT`.
- `game/game.c`: states `ST_MENU, ST_LOADING, ST_WORLD`. PLAY starts the loading screen (`loading.c`, 1 s),
  then `world_init(W,H)` + `jukebox_scene("ascendant_soul.ogg", 1)`. `pausebtn_set_enabled` /
  `thought_set_enabled` are `state == ST_WORLD`. The top buttons (music, pause, brain) are drawn on top of every state.
- `game/story.c`: `story_start()` -> `intro()` -> `HELLO` -> `SUMMON` dialogs -> `intro_done()` (controls on,
  `mission_add("Talk to Goddess")`, Dea `npc_add` 8 tiles above, cloud thought after 2.5 s). The death:
  `death_*()` ... `death_text()` -> `story_vessel_died()` (today only sets `vessel_dead`). `think(text, card, scene,
  tag, ding)` = brain window + coin ding + card. `VESSEL`/`VAS`/`DEA`/`ALEX` are `Person`s; `VESSEL_NAME`,
  `VESSEL_LATIN` macros. `hud_set_person(&P)` flashes the new name; `convo_set_player(&P)`.
- `game/world.c`: `world_init`, `load_map(MAP_CLOUD / MAP_GREEN)`, `world_fall_to_green(on_up)`,
  `world_open_hole`, `world_find_far_spot`, `world_death_*`, `world_place_shrine`. The player is drawn with
  `&VESSEL` in 7 places (`char_draw*`, hard-coded). The first fade-in is `FADE_IN_T`.
- `game/thought.c/.h`: `thought_say`, `thought_wrath`; the portrait comes from `hud_person()`.
- `game/shrine.c/.h`: the model for a solid, drawn, minimap-visible prop (`grave.c` copies it).
- `game/npc.c/.h`: `npc_add`, `npc_set_facing`; no removal.
- `game/audio.c`: synthesized sfx (`sfx_coin`, `sfx_beep`, ...): `sfx_summon` goes here.
- `game/missions.c/.h`, `game/brainwin.c/.h` (`brainwin_clear`, `brainwin_acquire`), `game/hud.c/.h`.

## Chunk checklist

Mark `[x]` when the chunk is pushed. Chunks are tiny on purpose; each leaves the game playable.

**A. Limbo (the space screen)**
- [x] **1. Story doc** — this file.
- [ ] **2. `space.c/.h`** — move the menu's black field and drifting white pixel flakes into `space_init(w,h)`, `space_update(dt)`, `space_draw(r)`. The menu uses it and looks exactly the same (screenshot before/after).
- [ ] **3. `ST_LIMBO` state in `game.c`** — a state that draws `space_*` plus the top buttons; brain button enabled (`thought_set_enabled`), pause button disabled. Dev stand-in: a debug entry (e.g. argv `limbo`) so it can be looked at. No story yet.
- [ ] **4. PLAY fades the menu out** — no loading screen: on PLAY the title and the buttons fade out over about 0.8 s (alpha), stars keep moving, menu music fade follows; then `world_init` (hidden, nothing drawn) and the state becomes `ST_LIMBO`. Remove the `ST_LOADING` path (keep `loading.c` only if something still needs it).
- [ ] **5. The soul's face** — `SOUL` `Person` in `story.c` (pale ghost-blue skin and shirt, light hair, no boots colour) and `thought_set_voice(const Person *)` in `thought.c`: NULL = the ID card person (as today), otherwise that person's portrait on the card. Check it on a screenshot.
- [ ] **6. Limbo script helper** — `limbo_run(const char *const *thoughts, int n, float gap, void (*on_done)(void))` in `story.c`: sets the voice to `SOUL`, says each thought (no coin ding, not added to the brain window), waits, then `on_done()`. The HUD (ID card, mission list, minimap, controls) is hidden while limbo runs; `limbo_end()` gives it back.
- [ ] **7. "Where am I?"** — the beginning of the game: PLAY -> limbo -> "Where am I?" -> a short hold (about 3.5 s) -> stand-in `limbo_end` + the old intro (the old dialogs still run for now so the build stays playable).

**B. The summoning effect**
- [ ] **8. Summon I: the light** — `world_summon(on_done)` in `world.c` (+ `game/summon.c/.h` for the drawing): on the cloud map, at the spawn tile, a tall beam of golden-white light drops from the top of the screen and a glowing pixel ring spreads on the floor; the player is not drawn yet; the beam holds, then ends. Pixel cells only, alpha only.
- [ ] **9. Summon II: the particles and the vessel** — golden pixel sparks rise and swirl inward around the beam; the player sprite forms out of the light (a gold silhouette -> the real colours over about 1.2 s, use the colour filter or alpha, no ADD blend); a new `sfx_summon()` in `audio.c` (a soft rising arpeggio plus a shimmer, one voice like the monitor). Controls stay hidden until `on_done`.
- [ ] **10. Use it at the beginning** — after "Where am I?" the world fades in on the cloud island and `world_summon` runs; cloud music (`ascendant_soul.ogg`) starts with it. The **HELLO and SUMMON dialogs and the toast are gone**: `intro_done()` is what runs after the summon (controls on, Dea placed 8 tiles up, facing down). **No "Talk to Goddess" task yet.**
- [ ] **11. Look around** — `is this where dead people go?` thought a few seconds after control returns (replaces the old "Why am I standing on clouds?" thought, keep its `THOUGHT_CLOUDS` tag); a `dea_watch()` sight trigger (like `alex_watch`) fires when Dea is within sight: task **"Talk to Goddess"** (`task_toast`) + thought "Someone is up there." Once only; `groundtest` checks no task before the sighting and the task after.

**C. The death leads to limbo**
- [ ] **12. Fade to limbo** — replace the `story_vessel_died()` stub: after "Aonia has died." the world fades to black over about 1 s, `world_death_cancel()` runs behind it, the state goes to `ST_LIMBO`. Remember `lives` (1 now) and the **tile where vessel 1 died** (`death_tx`, `death_ty`).
- [ ] **13. "Uh. I died."** — the first death line in the vessel table, said by the soul (`limbo_run`), then a hold of a few seconds. `lives`-indexed table so later deaths have their own lines.
- [ ] **14. Reset for the next life** — `story_reset_for_respawn()`: story timers, missions (`missions_clear()`, new), the brain window, `dying`/`vessel_dead`, the ID card back to **VAS** (silently, or with the normal name flash), HP full, `convo_set_player(&VAS)`, the player sprite back to the plain look. Back on the cloud map (`world_return_to_clouds()`: cloud map, spawn tile, hole closed, Dea placed, **Grasslands state kept in memory**).
- [ ] **15. Summon again** — after the hold the world fades in on the sky island and `world_summon` runs again (reuse of chunk 8-9, music restarts). No intro dialog. Dea is already there and looks at us.

**D. Dea scolds us, number two**
- [ ] **16. The scolding** — `DEA_SCOLD` dialog (as in the draft strings, spoken to VAS), starts by itself right after the summon (Dea speaks first). No free chat yet, no naming yet; at the end `scold_done()` is the hook.
- [ ] **17. Doia's look** — `VESSEL2` `Person` (different hair, shirt, pants; same size and proportions as the sprite system allows; maybe `long_hair`), the name `VESSEL2_NAME "Doia"`; the 7 hard-coded `&VESSEL` in `world.c` become `world_set_player(const Person *)` (default VESSEL). Check on a screenshot that vessel 2 looks clearly different.
- [ ] **18. The naming** — the last page of `DEA_SCOLD` ("Fine. You are number two now. Doia...") is the naming moment (like `DEA_NAMING_PAGE`): `hud_set_person(&VESSEL2)` (ID card flash, ding, "YOU ARE NOW DOIA"), `convo_set_player(&VESSEL2)`, `world_set_player(&VESSEL2)`.
- [ ] **19. Dea's questions** — after the naming `convo_ask_line(&DEA, &DEA_MIND, ...)`, her mood meaner than the first time (a free chat the player may skip); then the conversation ends.
- [ ] **20. The hole again** — the hole opens, the player jumps and falls to the Grasslands (the existing `world_open_hole` / `world_fall_to_green` flow). The Grasslands **keep everything from vessel 1** (Alex stays where she was, the shrine stays polluted and solid): check that `load_map` does not regenerate or reset props; if it does, fix it here. Landing line: "Here we go again."

**E. The grave**
- [ ] **21. The grave prop** — `game/grave.c/.h`, copied from `shrine.c`: a solid, drawn, minimap-visible prop: a stone slab with a small plot of earth, a flower, and a **framed portrait of the dead vessel on the stone** (`char_draw_portrait` at a small size, scaled to the stone, in a dark frame, greyed with `gfx_set_filter` so it reads as an old photo). `world_place_grave(tx, ty, const Person *dead)`; grey block on the minimap.
- [ ] **22. Where he died** — on the second landing `story.c` calls `world_place_grave(death_tx, death_ty, &VESSEL)`; if that tile is not free pick the nearest free one (`world_find_far_spot` with a small range). `groundtest` checks the placement, collision, and that the portrait pixels are on the screen.
- [ ] **23. Coming near the grave** — a proximity thought once: "Aonia. Whoever that was, he looked like me." (`think()` + brain window picture: the grave).
- [ ] **24. The end hook** — `story_vessel2_ready()` (empty stub) runs after the landing and the grave; update this file's status. Vessel 2's own mission is planned later.

## Open decisions (defaults so nobody has to wait)

- The PLAY button is still labelled "PLAY" (the owner says "start"; rename it in chunk 4 if wanted).
- No soul sprite is drawn in limbo: only the face on the thought card. Easy to add later (a small drifting ghost).
- Music in limbo: the menu music keeps playing through the first limbo and fades out with the summon; at a death it is silent in limbo (map music is already dead), and the cloud track starts with the summon.
- The brain window starts empty again after a death (a new vessel, a new head).
- The ID card name goes back to VAS silently-or-with-flash: use the existing `hud_set_person(&VAS)` flash unless it looks wrong.
- Vessel 2's mission is **not planned**: the orb is still Alex's, the shrine is still polluted.

## Status log

- 2026-10-10: vessel 2 planned (this file), chunks 1-24 listed. Next: chunk 2.
