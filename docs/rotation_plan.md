# Rotation: the engine follows the screen (a plan in chunks, starting at chunk 1)

> **Handoff file.** If you are an LLM picking this up mid-work: read this whole file, then `game/screen.h`, `game/gfx.h`,
> the `r` handler in `game/game.c`, and `fit()` / `onViewSized()` in `app/src/io/github/brobordd/vessel/MainActivity.java`.
> Find the first unchecked chunk in [Chunk checklist](#chunk-checklist), implement ONLY that chunk, commit, tick its box
> in this file **in the same commit**, push, then go to the next one. One chunk = one commit = one push. Do not skip
> ahead, do not batch chunks.

## Rule for this plan: no tests, just code

**Tests waste time here.** Do not write test binaries, do not run `tools/*test.c`, do not build a harness for rotation.
Write the code, push it, and the owner pulls and tries it on his phone as the user (`git pull` + `scripts/run.sh`,
plus a fresh `vessel.apk` for the chunks that say so). The only check a chunk makes on its own is that it still
**compiles** (`cc -fsyntax-only` on the touched files if SDL headers are around; skip it silently if they are not, and say
"not compiled" in the status line). A compile is not a test. If a chunk needs a decision the owner has to make by
looking at the screen, push anyway and say what to look at.

Replies in the chat stay short: what was done, what to look at on the phone, what is next. No complaints about the
token or the process.

## What the owner asked for

The **apk already rotates** (commit `574a487`: the manifest follows the OS auto-rotate / rotate button instead of forcing
a sensor orientation, and `MainActivity` handles the config change itself). The **game engine does not**: it keeps the
pixel size it started with and the app letterboxes that picture on the turned screen. Make the engine follow the screen:
turn the phone and the game re-lays itself out for the new shape and fills it, with no restart and no lost state.

## What the code does today (checked, with where to look)

- `game/game.c` `main`: `W` and `H` come from `argv[1]`/`argv[2]` once and never change. Every module gets them once:
  `menu_init`, `nowplaying_init`, `musicwin_init`, `brainwin_init`, `pausebtn_init`, `space_init`, `world_init`.
- The `r <landscape> <w> <h>` line from the app only calls `screen_set()` (`game/screen.c`) and prints it. Nothing reads
  `screen_landscape()` / `screen_changes()` yet. `game/screen.h` says on purpose "a rotation never changes our layout".
- The app (`MainActivity.java`): the **first** surface decides the game size, `w = width / SCALE`, `h = height / SCALE`
  (`SCALE = 2`), and it never changes. `fit()` letterboxes that picture on the current surface and sets `viewScale`,
  `viewX`, `viewY`. `onTouchEvent` undoes the letterbox with those three numbers, so touches already arrive in game pixels.
- GPU path: `game/gfx.c` records draw calls into a shared file; header ints `[1]`/`[2]` are `W`/`H`, written once in
  `gfx_init_gpu`. `GfxRenderer.java` has `gw`/`gh` fixed on the first surface, makes the scene and zoom render targets
  at that size once (`makeTarget`), and draws the scene letterboxed with `glViewport`.
- CPU path: `game.c` mmaps a `W*H*4` framebuffer once and `MainActivity.startBlit` copies it letterboxed. Resizing that
  needs a new shared file; it is the fallback path, so it **stays letterboxed** (chunk 9 pins that down).
- Every UI module stores `W`/`H` (and usually `u = min(w, h) / 360.0f`, the density unit) in `*_init(w, h)` and lays
  itself out right there. `u` uses the **short side**, so a rotation keeps `u`, and with it text size, tile size
  (`world.c`: `px`, `tile`) and button sizes. Only the positions and the visible area change.
- `*_init` is also used as a **reset**: `world.c` calls `toast_init(W, H)` on a new life to drop a half-shown panel.
  So a resize must be its own function; do not just call `*_init` again on rotation (it would wipe open windows,
  dialogs, the zoom, the world itself: `world_init` also reloads the map and restarts the story).
- The talk zoom render target is created lazily in `world.c` (`ztex = SDL_CreateTexture(r, ..., W, H)`, around line
  1839) at the current `W x H`. It has to be recreated when `W`/`H` change.
- Old and new must keep working together: the owner updates the game binary with `scripts/run.sh` (C only) and the apk
  separately. **A new game on the old apk, and a new apk on an old game, must both still run (letterboxed).** The existing
  protocol already ignores lines it does not know, so new capabilities are announced, never assumed.

## Design (decided, so the chunks do not have to re-argue it)

1. **The game's logical size follows the screen**: on a rotation or a resize the app sends the real screen size, the game
   picks `W = sw / SCALE`, `H = sh / SCALE`, the app draws it 1:1 (scale `SCALE`, no bars). The letterbox stays only as
   the fallback (old game, CPU path, the frames before the new size arrives).
2. **Capability handshake, in the shared gfx file** (no new pipe lines needed for it): the app writes "I follow the
   game's size" into a spare header int (`hdr[9]`, version number) before it opens the magic word. The game only
   resizes when that int is non-zero. Old app: it is never set, nothing changes.
3. **Every frame carries its own size.** Each slot header (16 bytes: `[0]` gen, `[1]` nrec, `[2]` and `[3]` are free) gets
   the `W` and `H` that frame was recorded for. The app replays a slot at the size written in it, remakes its render
   targets when that changes, and keeps showing the last old-size frame letterboxed until the first new-size frame
   arrives. That kills the race between "the app turned" and "the game caught up" without any lock.
   (`hdr[8]` = a size generation counter, for the app to notice cheaply.) Update the layout comment in `game/gfx.h`
   **and** `GfxRenderer.java` together; the file says they must match.
4. **Resize is layout only.** Every module that has an `*_init(w, h)` gets a `*_resize(w, h)` that recomputes positions
   and sizes from `w`, `h` and `u` and **keeps all state** (open windows, scroll, dialog text, timers). `*_init` becomes
   "reset state, then `*_resize`". The world keeps the player, map, NPCs, zoom and story.
5. **Resize happens at one place, at a frame boundary**: top of the main loop in `game.c`, never inside a message
   handler, never mid-draw. Burst of `r` lines (split-screen drag, animation) = only the **last** size is applied.
   Sizes are clamped (a sane minimum short side so `u` never collapses; a maximum so the framebuffer / textures stay
   small). A size equal to the current one is ignored.
6. **Fingers on resize**: a resize cancels the held touches (`world_touch(3, 0, 0)`, same as pausing does) so the stick
   is never stuck after a rotate.
7. **CPU path stays letterboxed** and the game does not resize in it (`hdr[9]` simply never exists there).
8. The vessel.apk in the repo root is rebuilt only by the chunks that touch `app/`.

## Chunk checklist

Chunks 1-6 are **C only**: `git pull` + `scripts/run.sh` is enough, and with the current apk nothing changes on screen
(the capability int is never set), so each one is safe to push and look at. Chunks 7-8 touch Java and need a rebuilt
`vessel.apk` (see chunk 12). Numbering starts at 1.

- [x] **1. The contract.** Docs and the frame format, no behaviour change. `game/gfx.h` layout comment: slot header
  `[2]`/`[3]` = that frame's `W`/`H`, header `[8]` = size generation, header `[9]` = the app's capability version (0 =
  letterbox only). `game/gfx.c` writes the current `W`/`H` into every slot header it fills and keeps `hdr[8]` at 0 for
  now. Rewrite `game/screen.h`'s opening comment: the picture now follows the screen when the app asks for it, and the
  old "never changes our layout" only holds in fallback (the "no tests" note at its top is already there, keep it).
- [x] **2. `gfx_resize` and mutable size.** `game/gfx.c`: `gfx_resize(w, h)` sets `W`/`H`, bumps `hdr[8]`, rewrites the
  header `[1]`/`[2]`, and the next `begin_frame()` stamps the new size into the slot. Add `gfx_app_resizes()` (reads
  `hdr[9]`). `game/game.c`: `W`/`H` stop being fixed after startup (they are already locals; make them what the whole
  loop reads), add `static void apply_size(int nw, int nh, ...)` that for now only calls `gfx_resize` and prints
  `resize: WxH`. Not called yet.
- [x] **3. Resize for the small modules.** Split `*_init` into reset + `*_resize` for: `loading`, `toast`, `pausebtn`,
  `nowplaying`, `brainwin`, `musicwin`, `menu`, `space` (the stars: keep their motion, spread new ones when the area
  grows, drop the ones off the edge). `*_init` calls its `*_resize`. Keep `W`/`H`/`u` statics up to date. No call sites
  yet besides `*_init`. (If a module's layout is baked into a table or a texture at init, rebuild it in `*_resize` and say
  so in the status line.)
- [x] **4. Resize for the in-world UI.** Same split for `dialog` (it calls `talk_init`, so `talk` too), `missions`,
  `thought`, `hud`, `minimap`, `mapwin`, `item_ui`. **Order matters** and is the one in `world_init`: `hud` first, then
  `minimap` (needs `hud_card_rect`), then `item_ui` (needs `minimap_bottom`). Open windows (`mapwin`, `talk`) keep their
  open/closing/animation state; `talk` keeps its `pos`/`target` ratio, not the pixel value.
- [x] **5. `world_resize(w, h)`.** `game/world.c`: new function (and a line in `world.h`) that sets `W`/`H`, recomputes
  `ui`, `ucell`, stick position, interact button, hand button (`sx`, `sy`, `bcx`, `bcy`, `hbx`...), then calls the chunk 3/4
  resizes in the `world_init` order, destroys and recreates `ztex` (the zoom target) at the new size without touching
  `zoom`/`zoom_target`/`talk_npc`, and keeps the camera on the player (check how the camera is derived from `W`/`H` and
  that map-edge clamping still behaves in a wide, short view). Grep `game/story.c` (it still reads `W` twice) and the rest
  of `game/` for any other cached `W`/`H` and fix them. `world_init` must not change behaviour.
- [x] **6. Wire it up in `game.c`.** Top of the main loop: if the app announced resize support (`gfx_app_resizes()`) and
  `screen_changes()` moved, take the latest `screen_width/height`, compute `nw = sw / 2`, `nh = sh / 2` (the same `SCALE`
  as the app; put it in one constant in `screen.h`), clamp, skip if unchanged, then `apply_size`: cancel held touches,
  `gfx_resize`, and resize by state: `ST_MENU` -> `menu_resize` + `space_resize`; `ST_LIMBO` -> `space_resize` + dialog;
  `ST_WORLD` -> `world_resize`; always `nowplaying`/`musicwin`/`brainwin`/`pausebtn`/`loading`. States that are created
  later (`menu_init(W, H)` at the start, `world_init(W, H)` after the menu fades, `space_init` at game.c:272) must read the
  **current** `W`/`H`, not the startup ones. Still inert on the old apk.
- [x] **7. App: the GPU path follows the game's size** (Java, needs the apk). `GfxRenderer.java`: replace the fixed
  `gw`/`gh` with the size in the slot being replayed; remake `sceneTex`/`zoomTex` (`makeTarget`) when it changes; set
  `hdr[9]` before the magic word. `MainActivity.java`: stop treating the first surface as the permanent size for the GPU
  path (`w`/`h` follow the renderer), keep sending `r` on every `onViewSized`, and make `fit()` give `viewScale = SCALE`,
  `viewX = viewY = 0` when `w*SCALE == surface width` and `h*SCALE == surface height` (so the touch mapping, which already
  uses those three numbers, becomes 1:1 with no further change). The game is still *started* with the first size.
- [x] **8. Transition frames.** Between the turn and the first new-size frame the app keeps showing the last old-size frame
  letterboxed (existing `fit()` maths on the old `gw`/`gh`), never black, never stretched; the switch happens exactly when
  a slot with the new `W`/`H` becomes the latest. Check the `onSurfaceChanged` path in `GfxRenderer.java` for a clear or
  a viewport reset that flashes. Also re-check `hideSystemUi()` and the display-cutout mode (`SHORT_EDGES`) after a
  rotation: the game's UI must not end up under the notch in landscape (if it does, give the game a safe inset via a
  new `s <l> <t> <r> <b>` line and clamp the HUD to it: only if it is actually a problem).
- [x] **9. CPU fallback pinned down.** `scripts`/`game.c`/`MainActivity.java`: in CPU mode nothing resizes, the letterbox
  stays exactly as before. Make that explicit (`screen_resizable()` in `screen.h`, false unless the GPU path announced
  support) so a later reader does not try to resize the mmap'd framebuffer. Docs/comments only unless chunk 6/7 leaked a
  resize into CPU mode.
- [x] **10. The state edge pass.** Read, do not test, each of these against the resize code and fix what is wrong:
  turning during the talk zoom (and during its open/close animation), with the music window open (piano keys, the
  pinch/chord touches), the map window (pinch), the brain window, a dialog line mid-typing, the death cutscene, the
  summoning and the dawn, limbo's first conversation, the pause screen, the load-game fade, the loading screen, a
  toast on screen. Rotating twice quickly (the burst rule). A split-screen window that is both narrow and short (the
  clamp).
- [x] **11. Landscape polish.** What a wide, short screen needs that "same layout, new numbers" does not give:
  HUD card / minimap / inventory stack down the right edge and may not fit the short side (cap, or put the inventory
  beside the minimap), dialog and talk panels capped to a readable width and centred, the music window's piano keys and
  waves laid out for the wide shape, the menu title and buttons, the stars, the story's full-screen moments
  (`story.c`, `summon.c`, death). One commit per screen if it is more than a few lines; list what was changed in the
  status line so the owner knows what to look at.
- [ ] **12. Rebuild the apk.** `scripts/build-apk.sh` (it downloads the Android SDK from dl.google.com, which this sandbox's
  network policy may block: if it cannot run, say so in one line and the owner builds it himself, he has Codespaces for
  that) and commit `vessel.apk` as `build` like the earlier `build` commits. Chunks 7-8 do not tick as "tried" until this
  is done.

## Things to look at on the phone, per stage

- After 1-6: nothing should look different at all (old apk), and the log line `screen: WxH ...` still prints on a turn.
- After 7-8 (new apk): turn the phone, the picture fills the screen, no bars, no restart, music keeps playing; turn it
  back, same. Open a dialog / the map / the music window and turn: they stay open and re-fit. Hold the stick and turn: it
  lets go. Talk to someone (zoom) and turn.
- After 9: force the CPU path: it letterboxes exactly as before.

## Status log

(One line per finished chunk: what changed, "not compiled" if it was not, what to look at. Newest last.)
- 1: contract in gfx.h (slot hdr [2]/[3] = frame W/H, hdr[8] size gen, hdr[9] app capability); gfx.c stamps W/H into every slot and zeroes hdr[8]; screen.h intro rewritten. Not compiled (no SDL headers here). Look at: nothing should change on the phone.
- 2: `gfx_resize(w,h)` (frame boundary only: sets W/H, rewrites hdr[1]/[2], re-stamps the open slot, bumps hdr[8] last) and `gfx_app_resizes()` (hdr[9] != 0, false on CPU) in gfx.c/.h; `apply_size(&W,&H,nw,nh)` in game.c (W/H are main's locals, so the loop and the later `*_init(W, H)` calls already read the current values), prints `resize: WxH`, not called yet. Not compiled. Look at: nothing changes.
- 3: `*_resize(w,h)` added to loading, toast, pausebtn, nowplaying, brainwin, musicwin (re-clamps the track list scroll), menu (calls `space_resize`, then `menu_layout`), space (flakes keep their place in proportion, same count); each `*_init` = reset state + resize. `u` stays short-side based. Nothing baked into a texture in these modules. Not called anywhere yet. Not compiled. Look at: nothing changes.
- 4: `*_resize` added to talk (keeps its slide as a share of the panel height, text/chips stay), dialog (re-wraps and re-boxes the open page, typing clock/page/hooks stay; calls `talk_resize`), missions, thought (after `nowplaying_resize`), hud (after `nowplaying_resize`), minimap and item_ui (same args as their inits, after hud/minimap), mapwin (an open map keeps centre and relative zoom, min/max zoom recomputed). Inits = reset + resize. Order for chunk 5/6: nowplaying, thought, hud, minimap, item_ui. Not called yet. Not compiled. Look at: nothing changes.
- 5: `world_resize(w,h)` in world.c/.h. The screen-dependent numbers (W/H, `ui`, `px`, `tile`, `ucell`, stick, interact + hand buttons) moved into a shared `world_layout`; `world_init` = reset + `world_layout` (behaviour the same), `world_resize` = `world_layout` + dialog/missions/toast/thought/hud/minimap/mapwin/item_ui resizes in world_init's order + drops `ztex` (draw recreates it lazily at the new size; `zoom`/`zoom_target`/`talk_npc` untouched) + camera re-derived and edge-clamped (`cam_axis` already centres a map smaller than the view, so a wide short view is fine). If the short side changes (split-screen) `tile` changes too, so `pxp`/`pyp`/`zoom_fx`/`zoom_fy` are scaled by the tile ratio and the player stays on his tile; a rotation keeps `u`, so nothing is scaled. A held stick is let go. `story.c`'s two `W` are a local word list, not the screen: nothing to fix; no other module in `game/` caches the screen size beyond the ones already resized. Not called yet. Not compiled (no SDL headers). Look at: nothing changes.
- 6: `SCREEN_SCALE` (2), `SCREEN_MIN_SIDE` (180), `SCREEN_MAX_SIDE` (1600) in screen.h. game.c: right after the input lines are read (top of the frame, so a burst of `r` = the last size only) it checks `gfx_app_resizes() && screen_changes()` moved, takes `screen_width/height / SCREEN_SCALE`, clamps, and calls `apply_size(&W,&H,nw,nh,state)`: cancels held touches (world state only), `gfx_resize`, then nowplaying/musicwin/brainwin/pausebtn/loading, then menu (menu_resize, which moves the stars) or, in limbo/world, `space_resize` + `world_resize` (HUD, dialog, controls, camera). `menu_init`/`world_init`/`space_init(W, H)` already read main's current W/H. Inert on the old apk (`hdr[9]` never set). The Java `SCALE` must stay equal to `SCREEN_SCALE`. Open windows' held fingers (map pinch, piano chord) are NOT cancelled yet: chunk 10. Not compiled (no SDL headers). Look at: nothing changes until the new apk.
- 7: GfxRenderer.java: `reset()` writes `hdr[9] = CAP_VERSION (1)` right after zeroing the header (before the game is spawned, so before it opens the magic word). `pollFrame` reads each slot's `[2]`/`[3]` (0 = old game, ignored) with the gen re-check, and when a frame has another size `resizeTargets` deletes and remakes `sceneTex`/`zoomTex` at it, sets `host.w/h`, and calls `host.fit`; replay then uses the new `gw`/`gh` (viewport, scissor, uniforms). MainActivity.java: `w`/`h` are volatile and follow the renderer; `fit()` gives `viewScale = SCALE`, bars 0 (at most 1 px border, centred) when the surface is the game's size x SCALE, else the old letterbox; the touch mapping already used those three numbers, so it is 1:1 with no other change. `onViewSized` still sends `r` on every surface change; the game is still started with the first surface's size. This also gives chunk 8's core (the old frame stays, letterboxed by `fit` on the turned surface, until the first new-size slot is the latest). Not compiled (no Android SDK here). **Needs the apk rebuilt by you (chunk 12).** Look at: after rebuilding, turn the phone: picture fills the screen, no bars; touches land where you tap.
- 8: read through, one comment changed (present() in GfxRenderer.java). The transition already falls out of chunk 7: on a turn `onSurfaceChanged` only re-fits (`fit` with the OLD `w`/`h` on the new surface = the existing letterbox, never stretched), `present()` clears and redraws the old scene texture every vsync (no black frame of our own), and the switch to 1:1 happens in `resizeTargets` exactly when a slot with the new W/H is the latest one replayed. `onSurfaceChanged` has no clear or viewport reset that could flash. `hideSystemUi()` already runs on focus and on `onConfigurationChanged`. The display cutout (`SHORT_EDGES`) is NOT changed: no `s` safe-inset line yet, because whether it is a problem can only be seen on your phone. Look at: turn to landscape, check that the stick (bottom-left), the buttons (bottom-right) and the HUD card / minimap (right edge) are not under the notch; if they are, tell me which side and I add the `s <l> <t> <r> <b>` line and the clamp. Also watch for a one-frame bar flash at the moment of the turn (the OS resizing the surface, not our code). Not compiled. Needs the apk (chunk 12, yours).
- 9: `screen_resizable()` in screen.h/.c (= `gfx_app_resizes()`: 0 on the CPU path and with an old app); game.c's resize check in the main loop now uses it instead of calling gfx directly. Comment in `MainActivity.surfaceChanged` pins the CPU path: size fixed at the first surface, a turn only re-fits (letterbox). No resize had leaked into CPU mode, so no behaviour change. Not compiled (no SDL headers). Look at: CPU path letterboxes exactly as before (nothing changes on the current apk either).
- 10 (part 1, box NOT ticked yet): held fingers on a resize. `apply_size` now also cancels (a = 3, which only clears, never fires) the map window (pan/pinch), the brain window, the music/brain cards, the pause button and, in the menu, the pressed button; new `musicwin_cancel()` lets go of the piano keys (both the primary and every other finger), ends a seek drag where the tape is (`music_scrub_end(-1)`, NOT a cancel at x=0, which would seek to the start) and stops a list drag. Still to read for the rest of chunk 10: talk zoom (and its open/close), dialog mid-typing, death/summon/dawn, limbo's first conversation, load-game fade, loading screen, toast, burst rotations, narrow+short clamp. Not compiled (no SDL headers). Look at: hold a piano key / drag the seek bar / pinch the map and turn the phone (new apk): nothing sticks, the track does not jump to 0.
- 10 (part 2, box ticked): read through the rest, no code change needed. Talk zoom: `ztex` is dropped in `world_resize` and `draw` remakes it at the new W/H; `zoom_fx/fy` are recomputed every frame while talking or dying, and `world_layout` scales them if the tile changed; the open/close slide keeps its share of the panel height (`talk_resize`). Dialog mid-typing: the re-wrap swaps a space for a newline one-for-one, so `total` and the typing clock still index the same character. Death, summon, dawn, fall, limbo's first talk, toast, loading: they read `W`/`H` live (or have a `*_resize`), nothing cached in screen px. `menu_init`/`world_init`/`space_init` read main's current W/H; `world_init` at the menu fade and `space_init` on the way back to limbo run in the same frame as the state change. Burst of `r`: last size only (loop-top check). Narrow+short: clamped to 180..1600 per side; when the clamp bites the app's `fit()` letterboxes (size x SCALE != surface), never stretches. Not compiled. Look at: turn during a talk, a typing line, the death text, a toast, the loading screen, then turn twice fast.
- 11: landscape polish, three commits. (a) world.c `item_anchor`: when the screen is short (H < 480u, i.e. every landscape) the bag + its panel hang BESIDE the minimap (same top, just left of it, same width) instead of under it; portrait is unchanged. Reason: card + minimap + bag reach ~230u of 360u and the open panel (~290u) ran over the interact button (top ~246u, same x range). (b) dialog.c: the bottom dialog box is capped at 500u wide and centred (`L.boxw`; text wraps to the box width), the centred speech box was already content-sized. (c) talk.c: keys, chips, title, input and SEND sit in a column at most 560u wide, centred (`cw`/`ox`); the dark panel behind still spans the screen. Portrait phones are narrower than both caps, so nothing changes there. READ, NO CHANGE NEEDED: musicwin (already has landscape piano range C3..C8, drops track rows then shrinks the equalizer/piano to fit the short side), menu (title at 22% / 3 buttons centred at 62% = fits 360u without touching the title), stars (`space`, relative to W/H), story/summon/death/dawn (all relative to W/H or `px`, death text is shrunk to 90% of W). Not compiled (no SDL headers). Look at (new apk or not, this is C): turn to landscape; open the bag with an item: the panel is left of the minimap, off the buttons. Talk to someone: the dialog box is a centred card, not edge to edge. Open the talk keyboard: a centred column. If the zoomed NPC is hidden behind the keyboard in landscape, tell me.
