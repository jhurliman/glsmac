# Repeatable terrain when restoring campaigns

Loading identical campaign bytes in fresh processes must produce the same terrain presentation. `Map` constructs its coastline Perlin modules using the game's random stream before the renderer initializes. Previously, snapshot loading restored the saved stream only in `InitComplete`, after those seeds and terrain texture choices had already been made from the new process seed.

Snapshot decode now installs the saved random state before constructing `Map`. Failed decode restores the previous stream. `InitComplete` still reinstalls the exact saved gameplay continuation after presentation-only initialization; the save format is unchanged.

## Validation

- The actual staged campaign restore test loads one saved world after two different initial seeds and compares the post-constructor random state. The second restore advances the next actual turn; the existing world/settings/roster and malformed-load checks remain.
- Cached native groups `Campaign file restores owned settings roster world and next actual turn` and `Campaign malformed files and unstable saves preserve current world and existing file` pass. Targeted correctness analysis of the changed production `Game.cpp` passes.
- Two fresh 1024×768 software-rendered launches load identical campaign bytes, accept the pending research-priority dialog and reach the selected Scout map. The top terrain rectangle `(0,0)-(1024,190)` has **0 changed pixels out of 194,560**. Both applications quit normally through the game menu.
- Campaign SHA256: `19d528137995877f1688f0325ac05bf3d5c9f752f018b7c62f12cb04f85cebea`. Captures and the owned campaign remain external validation artifacts; original game data is not committed.

This check proves repeatability for the measured restored scene. It does not prove original-executable terrain parity, fresh-world-to-first-load preservation of cosmetic choices, whole-frame animation equality, hardware performance or audio timing.
