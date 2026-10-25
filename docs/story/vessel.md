# Vessel: the story (replaces the old vessel-1 / vessel-2 docs)

> **Handoff file.** If you are an LLM picking this up mid-work: read this whole file, then `game/story.h`, `game/story.c`, `game/gen.h`, `game/ritual.h`
> and `game/thought.h`. Take the first unchecked chunk in the [Chunk checklist](#chunk-checklist), implement ONLY that chunk, commit, tick its box
> in the same commit and add a line to the [Status log](#status-log), push, then go to the next one. One chunk = one commit = one push. The owner
> tests every push on a phone (`git pull` + `scripts/run.sh`), so every chunk must leave the game building and playable.
> `game/story.c` is the restarted story (the old one, goddess and Alex and the Grasslands, is archived in `trash/story_old.c`).

## Environment notes (for an instance that starts cold)
- **Repo and push access.** `git clone https://github.com/BroBordd/vessel.git`. The owner gives a token in the chat; it is never written into any file,
  commit or this plan. Push to `main` with `git pull --rebase` first. A fresh clone has no git identity: `git config user.name "Claude"` and
  `git config user.email "claude@users.noreply.github.com"` (local, not `--global`).
- **How the owner works.** They `git pull` and run the game on a phone (Termux, `scripts/run.sh`). Replies in the chat stay short: what was done, what
  is next, no complaints about the token or the process. They test by feel, so sounds and movement are tuned by their words ("too slow", "annoying").
- **SDL.** `apt-get update && apt-get install -y libsdl2-dev` works in the sandbox. Whole-game compile check (no link to the phone's static SDL):
  `ls game/*.c | xargs -P8 -I{} sh -c 'cc -O0 -Wall -DGFX_REDIRECT -include game/gfx.h $(sdl2-config --cflags) -Igame -c {} -o build/chk/$(basename {} .c).o'`
  then link with `$(sdl2-config --libs) -lm`.
- **Headless looks.** `tools/ruinshot.c` renders the ruins (spawn / near ledge / far ledge, `VESSEL_AT=edge|far` warps there) to `build/*.bmp`;
  `tools/ritualshot.c` renders the blood circle; `tools/gentest.c` checks the map generator over 500 seeds and prints one with `gentest <seed>`.
- **Dev env vars.** `VESSEL_SEED=n` (map), `VESSEL_INDEX=n` (start in vessel n), `VESSEL_AT=edge|far` (start at the pit), `VESSEL_LIMBO=1`.

## Workflow rules
- Tick the chunk's checkbox and add its line to the Status log **in the same commit** that implements it. Commit message `story chunk N: <title>`
  plus a paragraph of what changed.
- Match the code style (C, short comments explaining why, GPL header on every new file). Everything the player sees uses only the font's characters
  (letters, digits, space and `, . ! ? ' -`; no quotes, colons or parentheses in game text).
- Thoughts are short (about 19 letters a line on a phone, 3 lines). Never reproduce real lyrics, poems or quotes from other works.
- Sounds are tuned on the phone speaker: nothing under 120 Hz carries there, and steady buzz is annoying; prefer soft, uneven, low-passed noise.
- Never write the token anywhere.

## Premise
You are **Vas**, a soul. You live in numbered bodies (vessels): **Aonia** (1), **Doia** (2), **Tria** (3), **Ceathia** (4) ...
without limit. Each death makes the next number. The only map is a set of **ruins**. No goddess, no NPCs, no old maps.
Goal of part 1: **escape the ruins**. A huge crack in the ground with a bottomless pit blocks the way out.

- **The look comes from the number.** Each body's appearance is generated from a seed = the vessel index (no stored art), so
  any number of vessels can exist and the player can die at any time.
- **Names:** vessel 2 learns its name by reading the note (the card says Vas until then). From vessel 3 on a body knows its
  name from the moment it wakes.
- **Death is a choice.** Aonia was running out of time, so he and Vas decided he would die. He stabbed himself.
- **The soul talks, the body thinks.** In limbo Vas has no thoughts (nothing to think with), so it speaks as a conversation. Every body
  thinks through the **brain button**, as in the old game, but the thoughts are fired by **events**, not by story beats.
- **Memory loss:** each death wipes what the body knew. The only thing ever passed on is Aonia's note.

## Critic pass: what was weak, and the fix
1. **Death cost nothing, so nothing was at stake.** Fix: every death is a permanent object in the world (a body that blocks, rots
   into pieces, can be used), and the checkpoint costs a life on purpose. The vessel number is shown on the card, always.
2. **Limbo would get tiresome.** Fix: the 3-line conversation plays once. Later deaths get one tap, a line that depends on how
   you died, and can be skipped.
3. **Too few things to do with a body.** Fix: a full cut order, many body parts, many recipes (some useless on purpose).
4. **No feedback while building the bridge.** Fix: the pit echoes. Dropping something in gets a longer or shorter echo, and as
   the pile rises the echo comes back sooner (section 9).
5. **The story was told by one note.** Fix: the body thinks all the time through the brain button, fired by events (section 10), and in limbo Vas talks.
6. **Nothing rewarded curiosity.** Fix: secrets (section 11): odd things the player tries get a reaction or a small reward.
7. **The rope was the only real tool.** Fix: a second tool (spine hook) and a few more that help a little.

## Story
### 1. Limbo (press PLAY)
The menu logo and buttons fade out over the drifting stars. No ID card, no map. Vas speaks as a conversation (a soul has no thoughts, so it talks), one line per tap:
1. "Well. We did it. We died."
2. "So this is what being dead feels like. Quiet. Almost restful."
3. "Now it's up to him. Or should I say... me."

Then the text clears, the stars fade, and we cut to the ruins. **Later deaths:** one skippable tap, line by cause:
- stabbed yourself: "Again. Fine."
- fell into the pit: "I didn't plan that one."
- the checkpoint (section 8): "One life, one circle. Fair price."

### 2. The circle
Vessel 2 wakes standing on a **magic circle drawn in blood**, inside a ruin. The blood still flows into the lines from **Vessel 1
(Aonia)**, who lies dead on his face at the edge, still bleeding. A **paper** lies next to him. The ID card says **Vas**.
Thought: "...Where is this? Why is it so wet?" Task: **Read the note**.

### 3. The note (once only)
Written in blood. Aonia knows the name his successor will have, so it addresses **Doia directly**. It is Vessel 1's note to
Vessel 2 only: reading it flips the card to **Doia** (name flash, coin ding, toast), and the paper **disappears**. No more notes.

> Hello, Doia.
> If you are reading this, it worked. You are Vessel 2.
> I am Aonia, Vessel 1, the one lying beside you. I died so that you could live.
> Do not be afraid. I am you.
> We are trapped in these ruins. Past the fallen hall, the ground has broken open. There is a pit with no bottom, and I could
> not cross it. I was running out of time, so Vas and I decided: I would die, and you would take my place.
> Every time we die, we lose what we knew. That is the price. I paid it. Now you have to.
> I left you everything I had. Find a way out.

Thought: "Doia. ...Fine. It's a name." Task: **Escape the ruins**.

### 4. A world that remembers
- Death never resets the map: it adds one more body. Everything stays where it is; a body dragged somewhere stays forever.
- A new body wakes at the **latest filled circle**: the first at the start, a later one once a checkpoint is made (section 8).
- Every body keeps its **own inventory**. **Inspect** shows it (Aonia carries the **knife**).
- **Bodies can be dragged** by the leg anywhere; they leave a **blood trail** that fades after a while.
- **Sizes.** Small things go in the inventory. Medium things are carried in the hands, one at a time, and slow the player a
  little. Big things (a whole body, a torso, the big rock) can only be dragged, and slow the player a lot.

### 5. Inventory window
Per item: **image**, **description**, **special actions**.
- Small rock: **Throw** (aimed, anywhere). Knife: **Stab**, any entity, **including yourself**.
- Body parts and crafted things: their own actions (section 7).
- Stabbing yourself is death: limbo, next index, new body at the latest filled circle; the knife stays in the dead body's inventory.

### 6. Cutting bodies and the parts they give
Each stab on a body takes the next step. A thrown big rock does all of it at once, splashes blood, and scatters the body's items.
| Stab | What happens | Drops |
|---|---|---|
| 1 | a wound, the blood runs again | nothing |
| 2 | right arm comes off | **arm** (medium) |
| 3 | left arm | **arm** |
| 4 | right leg | **leg** (medium) |
| 5 | left leg | **leg** |
| 6 | head | **head** (medium) |
| 7 | the torso opens | **ribs**, **heart**, **liver**, **intestine**, **spine** (ribs, heart, liver, intestine: small; spine: medium) |

More from the parts: a knife on a **hand** takes the **fingers** (small); a knife on a **head** takes the **eyes** and the **teeth**
(small); clean a head with the knife for a **skull**; a knife on a leg or arm takes the **bone** (medium).

### 7. Crafting
Craft menu, two slots. Some recipes are tools, some are useless on purpose; every first craft gets a thought.
| Recipe | Result | Use |
|---|---|---|
| rock + intestine | **tied rock** | **Swing** across the pit (section 9) |
| intestine + intestine | **long rope** | more length for the rope; keeps a use for later |
| spine + intestine | **spine hook** | **Swing** that hooks the far ledge, no rock needed |
| bone + rock | **club** | **Smash** a body to pieces fast (all of section 6, no knife) |
| arm + bone | **long reach** | picks up things lying out of reach (the far side of the pit) |
| skull + intestine | **skull on a string** | none; swings; Vas dislikes it |
| fingers + intestine | **finger necklace** | wearable; changes how the portrait looks |
| teeth + intestine | **tooth necklace** | wearable; rattles when you walk |
| eye + rock | **watching rock** | none; set it down and it "looks" at you |
| ribs + ribs | **rib cage** | none; stands on the ground; a small shelter for the player |
| heart + intestine | **heart on a string** | none; it still "beats" for a moment when held |
| liver + knife | **dull knife** | worse knife; none |
| blood + paper | **blood paper** | **Draw** a mark on the ground in blood (any mark; they stay) |

### 8. Checkpoints: the second circle (end of part 1)
After the pit is crossed the player walks on a bit and finds **magic circle carvings in the ground**, like the one Aonia filled.
1. Use the **knife on yourself** to fill the carvings with blood.
2. Wait a moment. The thought: **"My consciousness is fading."**
3. You fall **face down**, the blood **flows into the lines**, and you die.
4. The soul goes to limbo and the next vessel wakes **at this new circle**.

A checkpoint is made **by consuming a life**. The old circle, the pit and every body stay behind. This is the end of part 1.

### 9. The pit
A huge crack in the ground past the fallen hall, with a deep pit. It is why Aonia was stuck.
**Echo:** drop anything in and listen. At first nothing comes back. A rock gets a long, faint echo; a body a heavier one; as the
pile of bodies rises the echo comes back **sooner and louder**. That is how the player sees the bridge growing.
1. **The rope.** Take the knife from Aonia, cut him up, collect the intestine, craft the **tied rock**. At the edge **Swing**:
   the rock flies over and wedges between two rocks on the far side. Jump in holding the intestine, swing across, climb up
   the other side, clothes covered in blood.
2. **The spine hook.** Same, but with a **spine hook**: it grabs the far ledge itself.
3. **The rock.** Carry the big rock (walk hard) and **throw it at Aonia**: he breaks into pieces and everything he carried flies
   around. Then craft as in 1 or 2.
4. **The bridge.** Die as often as needed, drag every body to the edge and push it in, until the bodies **stack into a vertical
   bridge**. Costs the most vessels, needs only the knife. A fall into the pit by accident also adds a body to the pile.

## 10. Thoughts: the body thinks, fired by events
Thoughts use the **brain button** (the card with the portrait that opens and folds on its own, never pausing the player). **No story
beat scripts them any more**: each one is fired by an **event** (something the player does, or something that happens) **while the
player acts**. Each fires **once** unless marked (repeat). Lines are short. Vas never thinks; it only talks in limbo.
| Event | Thought |
|---|---|
| wakes in the circle (vessel 2) | "...Where is this? Why is it so wet?" |
| steps off the circle first time | "Out. Slowly." |
| first looks at Aonia (inspect) | "That's me. Was me." |
| reads the note | "Doia. ...Fine. It's a name." |
| first drag of a body | "He's heavier than he looks." |
| after dragging 10 seconds (repeat, rare) | "Why am I doing this." |
| carries the big rock | "This is a mistake." |
| first stab of a body | "I'm sorry. ...Not really." |
| arm comes off | "That's not how arms work." |
| head comes off | "Don't look at the face." |
| torso opens | "So that's what's in there." |
| first intestine collected | "Long. Surprisingly long." |
| first craft | "Don't think about it." |
| arrives at the pit edge | "Too wide to jump. Too deep to guess." |
| holds an intestine at the pit | "That would reach... if something held the other end." |
| holds a tied rock at the pit | "Throw it. Hope." |
| Swing wedges on the far side | "...It held. It held!" |
| drops something into the pit | (waits for the echo) "Still falling." |
| echo comes back sooner than last time | "It's getting shallower." |
| falls into the pit | "Oh." |
| climbs up the far side | "Covered in blood. Good enough." |
| finds the second circle | "Another one. He'd have wanted this." |
| fills the circle | "My consciousness is fading." |

## 11. Secrets: odd things get reactions
| What the player does | What happens |
|---|---|
| stabs themselves in the first circle before reading the note | no note; the next vessel wakes beside two bodies. Thought: "That was fast. Too fast." |
| drags Aonia back onto the circle | the carvings glow. Thought: "He looks comfortable." |
| puts every part of one body back in the circle in a human shape | Thought: "He's not coming back. But he looks better." |
| wears the necklaces and the skull on a string together | Thought: "I look... festive." |
| draws a mark in blood next to every body | Thought: "A little memorial." |
| reaches vessel 5 / 10 / 20 | Thought: "I'm getting good at this." / "A crowd." / "There's no room left." |
| throws the knife into the pit | it is gone; Thought: "That was dumb." (the knife is also in Aonia's inventory only, so a lost knife means the bridge by falls only) |
| stands still on the circle for a minute | the body hums |
| smashes the big rock with the club | it breaks into small rocks (it can't be a throw weapon, but it gives more to throw) |

## Rules
- Death can happen any time: limbo, next index, new generated look, new body at the latest filled circle.
- Bodies, parts and inventories stay exactly where they were.
- No goddess, no NPCs, no old maps, no more notes. The ruins are the one map.
- Thoughts never block play: one line at a time, and the brain button opens and folds on its own. Vas (the soul) only speaks in limbo, as a conversation.

## Open questions (owner to decide)
- Is Doia the only vessel told its name by a note, or do all bodies, Doia included, know it from the start?
- Does the circle's blood flow again at each wake-up, or has it dried?
- How many circles are there, and does every circle need a life? What comes after the second circle (part 2)?
- What the long intestine rope is for later.
- Who or what built the ruins, and what is across the pit?

## Decisions made while building (they override the text above where they differ)
- **Whole-body dragging, not by the leg** (owner's call). One button, next to a body: a tap takes hold from the side nearest to us, a tap lets go. Our
  floating hands go to that side, our eyes shut while we pull. He is a **sandbag**: he eases toward where our hands would have him (lag 0.22 s, like the
  camera), has 1.5 cells of slack so he has to be started, trails at most 8 cells (past that we cannot walk on), keeps sliding a moment after we stop, and
  never goes into a wall. We walk at 75 % speed holding him. The sound is a soft, low, uneven scrape, as loud as he slides.
- **The map is the ruins, a line, not a maze** (owner's call): the circle room, a passage, the fallen hall, the pit chamber with the crack across it and
  the far ledge (second circle's spot, sealed gate). The far ledge is not reachable on foot (`tools/gentest.c` checks that over 500 seeds).
- **The first body's blood** was cut by 60 % so his face shows.

## Chunk checklist
Each chunk says what is in it, so a cold instance can tell done from not done. `(part)` = some of it is built, the rest is listed.
- [x] **1. Seeded looks:** `Person` generated from a vessel index (`vessel_gen.c`, golden test `tools/vgentest.c`), names from the Irish-number table,
  `story_vessel_look(n)`, `story_vessel_next()`; vessel 1 stays the hand-made Aonia.
- [x] **2. Ruins map:** `gen.c` builds the ruins (circle room, passage, fallen hall with two niches and broken columns, doorway, pit chamber with a ragged
  3-5 row crack right across it, far ledge, sealed gate). New tile `T_RPIT` (solid, open to the eye: black, a cliff face on its north lip, a crumbling south
  lip, a glint now and then), minimap colour, `world_dist_to_pit()`, `world_pit_spots()`. `tools/gentest.c`, `tools/ruinshot.c`. The old maps stay in the file
  until chunk 14.
- [ ] **3. Event thoughts engine:** brain-button thoughts fired by events while the player acts, once-flags, repeat flags, the thought also joins the brain window.
- [ ] **4. Limbo and the first wake-up:** Vas's three-line conversation, the circle, Aonia with flowing blood, the paper; the short skippable limbo after later deaths, by cause.
- [ ] **5. The note:** the paper on the floor, reading it flips the card to Doia, the paper disappears, task list.
- [ ] **6. Persistent bodies:** death adds a body in place, bodies keep their inventory, **Inspect**.
- [ ] **7. Dragging and carrying (part):** whole-body drag of vessel 1 is built (see Decisions). Left: the blood trail that fades, drag any body, size classes (medium things in the hands), the big rock.
- [ ] **8. Inventory window:** item image, description, actions; small rock **Throw**; knife **Stab** on anything or self.
- [ ] **9. Cutting and parts:** the stab order of section 6, the big-rock smash, scattered items, the extra part cuts.
- [ ] **10. Crafting:** two-slot craft menu, every recipe of section 7, first-craft lines.
- [ ] **11. The pit:** Swing (tied rock and spine hook), the echo, falls, the bridge of bodies.
- [ ] **12. Secrets:** the reactions of section 11.
- [ ] **13. Checkpoint circles:** the carved circle on the far ledge, knife on self fills it, "My consciousness is fading", death; the next vessel wakes there.
- [ ] **14. Remove the old story:** drop the goddess, Alex, Dia, shrine, hole and Grasslands code and its tests.

## Status log
- 2026-10-25: the first body's blood cut by 60 % (`ritual.c wounds()`), his face is clear. Whole-body dragging of vessel 1 (chunk 7, part): `ritual_body_box/shift`,
  `char_set_grip/strain`, `sfx_drag`, the button icon; sandbag follow and a calm scrape (owner tuned: faster, softer). Story doc untrashed.
- 2026-10-25: chunk 1 verified done (already in the tree). Chunk 2 done: ruins generator and pit tile. `gentest` 500 seeds ok; headless shots look right
  (the pit reads as a black crack with a cliff face on its far side). Game builds. Next: chunk 3.
