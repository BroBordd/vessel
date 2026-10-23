# Vessel: the story (replaces the old vessel-1 / vessel-2 docs)

> **Handoff file.** Read all of it, take the first unchecked chunk in the checklist, implement ONLY that chunk, commit,
> tick its box in the same commit, push. One chunk = one commit = one push. The owner tests every push on a phone
> (`git pull` + `scripts/run.sh`), so every chunk must leave the game building and playable.
> `game/story.c` still holds the OLD story (goddess, Alex, Dia, Grasslands); it gets replaced by this one.

## Premise
You are **Vas**, a soul. You live in numbered bodies (vessels): **Aonia** (1), **Doia** (2), **Tria** (3), **Ceathia** (4) ...
without limit. Each death makes the next number. The only map is a set of **ruins**. No goddess, no NPCs, no old maps.
Goal: **escape the ruins**. A huge crack in the ground with a bottomless pit blocks the way out.

- **The look comes from the number.** Each body's appearance is generated from a seed = the vessel index (no stored art),
  so any number of vessels can exist and the player can die at any time.
- **Names:** vessel 2 learns its name by reading the note (the card says Vas until then). From vessel 3 on a body knows its
  name from the moment it wakes (no more notes).
- **Death is a choice.** Aonia was running out of time, so he and Vas decided he would die. He stabbed himself.
- **Memory loss:** each death wipes what the body knew. The only thing ever passed on is Aonia's note.

## Story
### 1. Limbo (press PLAY)
The menu logo and buttons fade out over the drifting stars. No ID card, no map. Vas thinks as a conversation, one line per tap
(speaker label "Vas"):
1. "Well. We did it. We died."
2. "So this is what being dead feels like. Quiet. Almost restful."
3. "Now it's up to him. Or should I say... me."

Then the text clears, the stars fade, and we cut to the ruins. Every later death returns to limbo the same way.

### 2. The circle
Vessel 2 wakes standing on a **magic circle drawn in blood**, inside a ruin. The blood still flows into the lines from
**Vessel 1 (Aonia)**, who lies dead on his face at the edge, still bleeding. A **paper** lies next to him. The ID card says **Vas**.
Vas: "...Where is this? Why is it so wet?" Task: **Read the note**.

### 3. The note (once only)
Written in blood. Aonia knows the name his successor will have, so the note addresses **Doia directly**. It is Vessel 1's note to Vessel 2 only: reading it flips the card to **Doia** (name flash, coin ding, toast)
and the paper **disappears**. No more notes after this.

> Hello, Doia.
> If you are reading this, it worked. You are Vessel 2.
> I am Aonia, Vessel 1, the one lying beside you. I died so that you could live.
> Do not be afraid. I am you.
> We are trapped in these ruins. Past the fallen hall, the ground has broken open. There is a pit with no bottom, and I could
> not cross it. I was running out of time, so Vas and I decided: I would die, and you would take my place.
> Every time we die, we lose what we knew. That is the price. I paid it. Now you have to.
> I left you everything I had. Find a way out.

Vas: "Doia. ...Fine. It's a name." Task: **Escape the ruins**.

### 4. A world that remembers
- Death never resets the map: it adds one more body. Everything stays where it is; a body dragged somewhere stays there forever.
- A new body wakes at the **latest filled circle**: the first circle at the start, a later one once a checkpoint is made (section 8).
- Every body keeps its **own inventory**. **Inspect** shows it (Aonia carries the **knife**).
- **Bodies can be dragged** by the leg anywhere; they leave a **blood trail** that fades after a while.
- The **big rock** can be dragged too, but it is heavy and slows the player.
- Big things (a whole body, the big rock) don't fit in the inventory; **small rocks** and **body pieces** do.

### 5. Inventory window (new)
Per item: **image**, **description**, **special actions**.
- Small rock: **Throw** (aimed, anywhere).
- Knife: **Stab**, pick any entity, **including yourself** (that is how Aonia died).
- Body pieces: collectable, used in crafting.

Stabbing a body cuts it into pieces. Stabbing yourself = death: limbo, next index, new body at the latest filled circle; the knife stays in
the dead body's inventory for the next vessel to take.

### 6. Crafting (new)
Craft menu, two slots.
- rock + intestine = **rock tied to an intestine** (action **Swing**).
- intestine + intestine = **longer rope** (extra length for the rope; no other use yet).

### 7. The pit: three solutions
1. **The rope.** Take the knife from Aonia, cut him up, collect the intestine, craft rock + intestine. At the edge **Swing**:
   the rock flies over and wedges between two rocks on the far side. Jump in holding the intestine, swing across, climb up the
   other side, clothes covered in blood.
2. **The rock.** Pick up the big rock (walk hard while carrying it) and **throw it at Aonia**: he breaks into pieces and
   everything he carried is thrown around the body (knife included). Then craft as in 1.
3. **The bridge.** Die as often as needed, drag every body to the edge and push it in, until the bodies **stack into a vertical
   bridge** across. Costs the most vessels, needs only the knife.

### 8. Checkpoints: the second circle (end of part 1)
After the pit is crossed (by any of the three ways) the player walks on a bit and finds **magic circle carvings in the ground**,
like the one Aonia found and filled with his blood.
1. Use the **knife on yourself** to fill the carvings with blood.
2. Wait a moment. The thought: **"My consciousness is fading."**
3. You fall **face down** on the ground, the blood **flows into the lines**, and you die.
4. The soul goes to limbo and the next vessel wakes **at this new circle**.

That is how a **checkpoint** is made: **by consuming a life**. The old circle, the pit and every body stay behind as they were.
This is the end of the first part of the story.

## Rules
- Death can happen any time: limbo, short Vas conversation, next index, new generated look, new body at the latest filled circle.
- Bodies and their inventories stay exactly where they were.
- No goddess, no NPCs, no old maps, no more notes. The ruins are the one map.

## Open questions (owner to decide)
- How many circles are there, and does every circle need a life? What comes after the second circle (part 2)?
- Is Doia the only vessel told its name by a note, or do all bodies, Doia included, know it from the start?
- Does the circle's blood flow again at each wake-up, or has it dried?
- Do later deaths (accidents in the pit) get different Vas lines?
- How many bodies make the bridge, and does the player see the pile grow?
- What the long intestine rope is for later.
- Who or what built the ruins, and what is across the pit?

## Chunk checklist
- [ ] **1. Seeded looks:** `Person` generated from a vessel index (deterministic), name from the Irish-number table (Aonia, Doia, Tria, Ceathia ... with a rule past ten).
- [ ] **2. Ruins map:** one map with the circle, the fallen hall, and the pit; remove the old maps.
- [ ] **3. Limbo and the first wake-up:** Vas conversation, fade, circle, Aonia's body with blood flowing, the paper.
- [ ] **4. The note:** blood-written paper, card flip to Doia, paper disappears, task list.
- [ ] **5. Persistent bodies:** death adds a body in place, new vessel in the circle, bodies keep their inventory, **Inspect**.
- [ ] **6. Dragging:** grab a body or the big rock by the leg, blood trail that fades, rock slows the player.
- [ ] **7. Inventory window:** item image, description, actions; small rock **Throw** with aim; knife **Stab** on any entity or self.
- [ ] **8. Cutting and pieces:** stab a body into collectable pieces; rock thrown at a body breaks it and scatters its items.
- [ ] **9. Crafting and Swing:** craft menu, rock + intestine, intestine + intestine, Swing across the pit, climb out.
- [ ] **10. The bridge:** bodies pushed into the pit stack up into a walkable bridge.
- [ ] **11. Checkpoint circles:** carved circle found past the pit, knife on self fills it, "My consciousness is fading", fall face down, blood flows, death; the next vessel wakes at this circle.
- [ ] **12. Remove the old story:** drop the goddess, Alex, Dia, shrine, hole and Grasslands code and its tests.
