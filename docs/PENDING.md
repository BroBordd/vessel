# PENDING.md: undone changes, in order

Written from the chat with the owner. Each item is one small chunk: one push, owner tests on the phone, then the next. Nothing here is done unless ticked.

## A. Loot window (chest style)
- [x] **A1.** Two grids (body on top, you below), details strip under the items, tap to select, tap again / TAKE / TAKE ALL (`item.c`: `chest_*`). Pushed as cb28804, untested.
- [x] **A2. Drag to loot** (pushed, untested). Touch has no motion events in the window yet (`item_ui_touch` only handles down/up). Add a drag state: hold a stack, a floating icon follows the finger, drop on the other grid to move it (body to you = `body_cb`). A short tap must still select.
- [x] **A3. Reorder** (pushed, untested; done as an order list per grid (`ord`, `order_move`): new kinds go to the end, gone kinds drop out, a drag inside a grid moves the kind to the place under the finger. not saved across a death: `item_clear` resets it). Needs a persistent slot array (today slots are derived from the kind order, `kind_at_slot`). Drag within our own grid and within the body's. Must survive death/respawn (`story.c` keeps ground items across a death; check the pockets too) and crafting/take/give.
- [x] **A4. Put back** (pushed, untested; `bodies_give` moves the whole stack; the orb and the hammer cannot be put in). Drag from our grid into the body's. Needs `bodies_give(vessel, kind)` in `bodies.c`, and the medium/hands rule in reverse.
- [x] **A5. Cleanup** (pushed: the old craft_a flow and the bw_on branches of the bag window are gone; the grid scroll for many kinds in a small window is NOT done, see below). Remove the dead `bw_on` branches in the bag window code (`hv`, `act_n`, `btn_*`, draw) once A1-A4 are tested. Handle many item kinds in a small window (grid scroll, or a smaller slot).

## B. Hand button and inventory hand/craft squares (new)
- [x] **B1. One button under the map** (pushed, untested). Replace the two small buttons (bag and the in-hand preview, `bag_rect` / `prev_rect` in `item.c`) with ONE button: same height as the current buttons (`bag`), same width as the map (`uw`). It shows a hand icon, or the equipped item when something is in hand. Tap opens the inventory window (like the bag button does today). Update `hit_btn`, the draw of the preview, `item_ui_bag_x/y`, and anything that places things relative to the two buttons (HUD, `world.c`, tests).
- [x] **B2. Hand square in the inventory window** (pushed, untested). A grey hand square. Drag an item onto it to equip it (replaces the IN HAND / PUT AWAY button; drag it out or tap to put away). Uses the drag state from A2, so do A2 first.
- [x] **B3. Crafting squares** (pushed, untested; recipes are pairs, so the result shows when exactly two squares are filled; the squares only point at things, nothing moves until the result is tapped; 3+ item recipes are a later change). Four squares, 2x2, like minecraft, in the inventory window. Drag items in; a result slot shows what the recipe makes (recipes: `story_craft`, `item_set_craft_cb`). Replaces the COMBINE flow (`craft_a`, WITH WHAT?). Needs drag from A2 and a way to give the items back if the window is closed with items in the squares.

## C. To check when testing what is already pushed
- [ ] Throw: random tile in the 3x3 round us, pit still reachable from the edge (`world_throw_item`, `world.c`).
- [ ] Vessel 1's knife: a real item above his body (`story.c`, `story_start`), pockets empty, painted knife removed (`ritual.c`). Check the tile is sensible, and that the knife still works for the cuts and the second circle.
- [ ] Loot window chunk A1 on the real screen: sizes, wide and tall layouts, an empty body, a medium thing with full hands (the story plays the error sound).

## D. Still open
- [ ] Grid scroll or smaller slots when a window holds many kinds (slots shrink to a minimum size and overflow today).
- [ ] Recipes with 3 or 4 items (the 2x2 squares only match pairs, `item_recipe(a, b)`).
- [ ] Slot order is not saved across a death (`item_clear`).
- [ ] Test everything in section C on the phone and report what breaks; nothing in A and B was run yet.

## E. Hands, buttons (to check on the phone)
- [ ] Pulling a body / the rock: the hands travel out to it when you take hold (0.18 s) and back to the sides when you let go (0.22 s); no jump when the nearest side changes. (`world.c`: `dg_k`, `dg_x`, `dg_y`.)
- [ ] Hands under / over the body: a body or item north of us = hands drawn under the body sprite; south or beside = over it. Pick-up reach does the same (`char_set_grip_behind`). Check the moment it flips (item crossing the feet line).
- [ ] Buttons: pressed fill (bluish on dark buttons, lighter gold on gold ones) on the bag/hand button, the window X, ACTION, TAKE / TAKE ALL, the action list and its X; a short dry CLICK (`sfx_tap`, filtered noise + a dull knock, nothing pitched) on the tap of every UI button (menu, pause, music/thought cards, dialog TALK/BYE, confirm, note, map, brain window, music window buttons, the inventory buttons, the CONTINUE button). NOT on the arcade controls (the hand / pick-up button, the pull / push button, the stick).
- [ ] Dead bodies (not vessel 1): pull one; the big pool of blood stays on the floor where it died, only the smears follow. A body dragged over a pool lies on top of it.
- [ ] Dying scene (self-stab): the eyes shut at the first hesitation (the knife stops and pulls back) and stay shut for good: second hesitation, the push, the fall, the body on the floor.
- [ ] Craft cells = scratch inventory cells (/tmp): tap one to select it (light frame, info + ACTION, empty one says EMPTY); drag between cells / out to the grid as before; what lies in them is MOVED there (the grid shows one less; a kind with no unit left in the grid leaves its slot and comes back to the same slot); closing the window sends everything back. A tap no longer empties a cell: drag it out to the grid (it lands under the finger).
- [ ] Tap sounds are on the tap (release on the button), never on touch-down / hover / slide-off / drags.
- [ ] Bag grid is positional: drag a thing onto a BLANK slot and it sits exactly there (its old slot stays blank, nothing slides); onto a full slot the others slide as before; a thing that is used up leaves a blank; a new thing takes the first blank slot; a thing out in the craft cells returns to its slot. (The loot / body window keeps its compact list.)
- [ ] Holding a body (or the rock): walk INTO it and it goes ahead of you (pushing; same step as yours, never into a wall / another body / the rock). The rope only pulls now: walking toward a held body no longer makes it creep away. Pushing leaves the blood smears and the scrape sound like pulling does.
- [ ] The hole: push a held body out over the crack (we cannot follow, the crack is not walkable): it hangs out over it, can be drawn back by the rope, and falls only once its MIDDLE is past the edge (we let go): a beat in the air, a slide on the way it was going, then it drops faster and faster, darker and darker, clipped to the crack, with an air rush (`sfx_swoop`). Only then is it counted (echo, the bridge). The rope never draws it further out over the crack. Its pool of blood stays on the floor. (The rock and vessel 1 never fall; rock pushed into a body still smashes it.)
- [ ] The three thoughts "Too wide to jump...", "Still falling.", "That was fast. Too fast." are gone (EV_PIT_EDGE, EV_DROP_IN_PIT, EV_SECRET_FAST_DEATH); the echo thought (EV_ECHO_SOONER) stays.
- [ ] The temple has no pillars / rubble heaps any more (gen.c step 8): nothing static blocks walking except the walls, the gate and the pit. (Same seed = same rooms and pit as before; the movable rock is untouched.)

