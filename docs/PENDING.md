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
