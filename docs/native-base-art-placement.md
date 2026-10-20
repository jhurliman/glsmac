# Native Base atlas geometry

The original Base dispatcher passes O=projector-(0,24), corresponding to flat diamond center F-(48,48) at zoom0. The default source atlas is a100x75 cell; the loaded University size0 sprite trims it to offset(27,17),width41,height38. Its native destination is[555,257,596,295) when F=(576,288). Uniform band4, isolated hill and coast shift this rectangle up25,6,3 pixels respectively. These values come from the separately executed loaded-rectangle contract corrected in PR325; they are independent of the rejected improvement labels.

The port previously mapped this atlas cell into a square world quad centered on the tile. Native map projection now uses dimensions100/96 by75/48 and source-center parameters(52,48), accounting for the existing sprite destination-offset convention and instance Y inversion. The complete source cell begins at O; original trimmed artwork then retains its own offsets. Base forwards the active map projection through BaseManager to Faction's actual sprite caller. Canonical land atlas profiles use this geometry; custom atlas/scale/pivot descriptors and unmeasured sea layouts retain their explicit paths. Native and legacy cache entries have different actor keys.

Validation exercises the actual frontend Base, Faction sprite selection, generated quad, instanced actor matrix and scene::Camera. It checks all four measured loaded rectangles, independent legacy/native cache entries, and existing Base identity/ownership retirement behavior. No owned artwork bytes are copied into tests. The geometric source bounds are not an opaque-ink or RGB screenshot comparison. Palette, sampling, lighting, other original artwork selections and the remaining model/overlay geometry still require their fidelity passes.

## Native Base palette in the world shader

Canonical native Base actors now bypass the world shader's additional lighting
and hardcoded `color*1.1-0.1` adjustment, as native Unit and overlay actors already
do. The original indexed sprite path selects display-palette pixels directly;
this extra RGB terrain adjustment is not part of that copy. Explicit legacy or
custom atlas actors retain their existing render path. Source palette loading,
source0 destination lookup, resampling and nonzero-zoom layout remain separate
fidelity work; this flag does not establish complete Base pixel parity.

The affected frontend Base ownership/identity fixture and correctness analysis
of Faction.cpp pass. An actual hidden Linux/software1024x768 run loads the
preserved campaign (SHA256
`aed85217e7684412e0de762d4ccf23df5892f20fc0355a8b2fa15ea55580ae92`),
showing Morgan Industries in2105. Compared with the existing same-save capture,
only2,029 pixels change, all within[483,232,550,288), around the Base body.
These are before/after port captures, not an original-game comparison. The
ordinary terrain path, exploration gap and console remain unchanged. The build
is f8cf0d5e plus the native Base shader-flag patch; normal window close exits0.
Sound is disabled and this software run does not measure hardware performance.
