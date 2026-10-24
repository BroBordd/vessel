# trash

Archived, not built (the build only compiles `game/*.c`). Nothing here is deleted for good: git history has it all too.

- `story_old.c`, `story_old.h`: the whole first story (Dea, Alex, the graves, the shrine, the hammer, the orb, vessels 1-3, limbo, saves).
  `game/story.c` is now a small stub with the same interface (`game/story.h`).
- `world_before_temple.c`: `game/world.c` as it was before the generator and the temple: the Grasslands (`gen_map`), the cloud map and
  Dea's temple (`gen_cloud_map`), Alex's house. Those maps are still in `game/world.c` but nothing loads them any more.
- `docs_story/`: the old story notes.
