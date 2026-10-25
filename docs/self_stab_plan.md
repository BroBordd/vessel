# Self-stab death scene + bug fixes (plan)

Written before any work, as asked. One commit per chunk, in order.

## A. The self-stab death scene (extends the "final action", world.c / story.c / audio.c)

Current: knife goes in, 1.1 s later the world fades and the death runs. Eyes reopen after the cutscene. New timeline (t = seconds from the knife going in, `FIN_HIT`):

0. **Hesitation.** The knife comes in, stops and pulls back (twice), the hands tremble; the eyes shut after the second.
1. **Push in.** The knife accelerates (cubic) and the blade goes INTO the neck: it is hidden past a vertical line at the neck's entry edge (`char_set_stab_clip`), so the tip vanishes gradually; it does not come out the far side. No blood stripes drawn: the fountain is enough. Eyes stay shut from now on, for good (also after the scene: nothing may reopen them until the next life, `char_set_strain` stays on until the continue button).
2. **Silence** ~1.2 s: standing, eyes closed, knife in the neck. No sound, no movement.
3. **Fountain** ~3 s: blood sprays out of the neck (particles, arcs, falls to the floor), the body shakes (small fast x jitter on the whole sprite), a fountain sound (wet rushing "woosh", looped noise, filtered) plays.
4. **Red filter + text, instantly** (no fade, a hard cut for punch): the whole screen gets a red filter and the text `<NAME> HAS DIED` (the current vessel's name, from `story_vessel_name`, upper-cased: Doia -> `DOIA HAS DIED`) appears with it at the same frame. A hit sound on the cut.
5. **Hold** ~2 s with the filter and text on. In that window:
   - the sprite switches to the sleeping one (`char_draw_lying`), still shaking;
   - the fountain stops and turns into a weak shake of small blood sparkles (a few drops, now and then);
   - the fountain sound ends and a `bzzt` follows: the last squirts of an empty ketchup bottle (short wet sputter/fart noise bursts, spaced out, fading), synthesised in audio.c (`sfx_blood_fountain`, `sfx_blood_sputter`).
6. **Continue button**: a button labelled `CONTINUE` fades in (a normal button, same look as the confirm window's). Tapping it runs the existing death (`story_die(DEATH_SELF)`: black, limbo line, next vessel wakes). The filter, text, eyes-shut and the shaking all end there.

Notes:
- The scene state lives in world.c (`fin_*`), the death still goes through `stab_self_go` -> `story_die`, but only after the tap on CONTINUE (the old auto-timer `FIN_END` goes away). The back button / pause are inert during the scene.
- On the second circle (checkpoint) the same scene plays; CONTINUE then runs `checkpoint_begin` as today.
- Music already fades out when the scene starts (done). Its fountain/bzzt sounds go on their own sfx voice so the blip/hit voices stay untouched.
- The red filter is a full-screen fill drawn over everything in the scene (not the CONTINUE button and text). GPU path: plain alpha fills, no new texture.

## B. Bug: dragging vessel 1 (Aonia) away from the circle

Symptoms: past some distance his sprite goes invisible (the blood trail stays), and after pulling him a while we cannot move in any direction.

Causes found:
1. **Invisible sprite**: `draw_scene` only draws the first circle's floor (`ritual_draw_floor`: circle, pool, knife, **vessel 1's body**) when the circle's HOME is on screen (`hsx > -40*px ...`). The body is drawn by that same call, so once the circle is off screen the body is culled even though he is on screen. Fix: widen that test by his drag offset (new `ritual_body_off(&dx,&dy)` getter) so it follows the body.
2. **Stuck in every direction**: `blocked()` refuses any position whose feet box overlaps a body (`body_hits`). While we pull, the body trails on a rope; when we turn round or he is stopped by a wall and we keep going, we end up standing inside his box, and then every step (all four directions) is "onto a body" = blocked forever. Fix: a step is refused only if it would enter a body from outside; when our feet box already overlaps one we may walk out (same rule `drag_blocked` already uses for bodies pulled apart). Also stop `drag_move` from sliding a body onto us.

## C. Naming

- Vessel 2 shows the wrong name on the ID card: STORY_CANON is 1, so index 2 comes from the generator (`Deax`). Fix: `story_vessel_look(2)` is the generated look with the name `Doia` (VESSEL2_NAME) so the ID card, body inventory title and the death text all say Doia.
- Remove the `YOU ARE NOW <name>` notification (hud.c `toast_notice("YOU ARE NOW", ...)` after reading the note): the name still flashes on the card, the toast and its coin ding go.
- Every vessel from 3 up keeps coming from the generator (`vessel_gen_name`), and EVERYTHING that shows a name asks `story_vessel_name(index)` (ID card, body windows, `<NAME> HAS DIED`), so the name is always the dynamic one of the vessel in question; never a hard-coded string except the story's own Aonia (1) and Doia (2).

## Order
1. this plan (pushed first)
2. B (bug fixes), C (names)
3. A (the scene), then push, you test on the phone
