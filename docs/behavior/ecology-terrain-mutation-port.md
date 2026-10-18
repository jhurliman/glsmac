# Ecology terrain mutation producer

`PrepareSmacBloomTerrain` stages the selected bloom's H increment, nominal
one/nine-cell decision and all final semantic terrain deltas. It follows the
[26 original mutation prefixes](ecology-sea-terrain-mutation.md) and
[exact class-only setter clarification](ecology-class-setter-guard.md).

The native notice scan overwrites the bloom-loop index, so only the first
eligible cell receives direct fungus and improvement changes. Protection or
clipping can move that first write to a later neighbor, or prevent every write.
H still increments once. The removal mask preserves every other item bit.

Deep-water mutation raises the direct cell to class2, performs the ordered
lower slope pass, then the upper rings with the original first-unchanged-ring
early stop. It stages indirect class/rockiness/landmark changes even on cells
protected against direct fungus. Contours and shared corner heights are not
inputs or outputs of this class-only producer; callers must keep those fields.

The class setter uses the actual zero/nonzero turn gate, strict land-lowering
rockiness conditions and exact unsigned landmark invalidation. Zero landmark
words remain zero; nonzero words clear bit0x80 and complement only a nonnegative
high detail byte. Unchanged-class calls preserve metadata/RNG while still
setting native dirty mask4. The same-category land-raising branch is also
implemented from the clarification's source contract; the supplied eleven
native helper controls do not separately execute that raising-land branch.
Cross-category conversion has additional effects and remains a separate path.

The plan contains before/after fields and unique changed coordinates. It does
not modify its terrain query source, publish live history, touch status/warming,
create units or render tiles. Its caller must stage the existing CRT stream and
publish the complete changed-cell set atomically; the native dirty mask alone
can be zero despite a real item-only mutation. New H is checked before overflow.

Validation: fourteen related native groups pass from the cached incremental
build. The native fixtures compose threshold, selection and terrain mutation
from actual sparse inputs, matching final cells, H/budget, direct fungus cell,
dirty mask and every CRT state, with shared RNG unchanged. The six bloom groups
also pass ASan/UBSan with SmacEcology.cpp instrumented; other objects reuse the
normal build, and leak detection is disabled. Targeted correctness analysis
passes that TU and its two existing production consumers. These are pinned
stock SMAC semantic controls; live map/render publication, full native-life
events and equivalent SMAX execution remain to be integrated and verified.
