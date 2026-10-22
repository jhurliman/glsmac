# Workshop preview position across paints

The original Workshop reuses one scene position across its component and main
painters. Rebuilding every card with a zero position made the chassis preview
disagree with the original after the first full paint. Painting Shielding in a
separate preliminary pass also changed the state consumed by subsequent cards.

The live window now owns three binary32 position words. A full paint passes
them through Chassis, Weapon, Shielding, Reactor and Main in that order. Ability
cards remain on their existing text path. The main-only timer carries the latest
position into its next draw without repainting the component cards. Cached main
sprites do not alter the Workshop position. Hiding retains it; the next ordinary
open starts at zero, matching the native constructor path.

`smacworkshopdesign` accepts an optional thirteenth argument containing the three
words, and returns the resulting words. `smacworkshopcomponents` accepts the same
state as its third argument. Their older calls retain their Boolean return value
and independent-position behavior. The shared Shielding-only path skips the
other component renders entirely. Inputs require exactly three finite binary32
words represented as unsigned 32-bit integers; no pointer or GSE value is retained
by the C++ renderer.

## Original-game evidence

- [Shared scene and painter order](behavior/design-workshop-shared-position.md).
- [Complete phase-0 chassis color/depth expectations](behavior/design-workshop-chassis-raw-planes.md).
- [Native requested versus processed record counts](behavior/design-workshop-kernel-count-semantics.md).
- [Two phase-1 full paints](behavior/design-workshop-phase1-two-paints.md).

The owned-asset native test compares complete 256 by 256 indexed color and
little-endian depth hashes for both phase-0 chassis states and both phase-1 main
paints, plus exact intermediate/final position words. The processed-record
expectation is 42,810: the native kernel excludes one terminal from each of 64
requested streams. The raw count operands sum to 42,874. This distinction changes
no pixel expectation.

In a controlled live SDL capture at 1024 by 768, all 8,415 RGB pixels of the
99 by 85 chassis card match the native phase-1 capture on first paint, second
paint, and reopening. This fixture fixes main yaw at zero, hides model visibility
from the painter to retain phase 1, and supplies the reference mineral factor
10 in copied UI data. It establishes those card comparisons, not whole-screen
parity or normal campaign pricing/animation. A separate animated SDL session
uses normal visibility, pricing and timers; opening through production Help,
repainting, closing and reopening passes under focused ASan/UBSan and exits
without sanitizer reports or tracked OpenGL leaks.

The affected GSE window script and four native CVR groups pass focused sanitizers
with four production translation units and the renderer test instrumented.
Targeted correctness analysis passes for Surface, CvrScoutRenderer and its
backend consumer. Existing successful renderer-family checks remain applicable
to the unchanged rasterizer and frame assembly. Original assets, raw buffers and
capture tooling remain private; the committed contracts contain hashes/scalars.
