# Ecology selector: sparse terrain replay supplement

RE-SMAC-ECOLOGY-SELECTOR-TERRAIN-001, 2026-10-18. Pinned stock SMAC.
[Complete sparse inputs and literal outputs](ecology-selector-terrain-cases.json)
supplement the frozen [handler/cache contract](ecology-handler-research-cache.md).
The previous fixture and evidence bundle are unchanged.

The published Base(64,16) sequence is now independently reproducible from
**terrain inputs and CRT seed**, without supplying candidate weights. The fixture
specifies the full80×80 horizontal cylinder, its3,200 valid parity cells, the
complete ordered Base list (one Base, ID0, owner1, coordinate64/16), all48 ordered
candidate offsets, four diagonal neighbor offsets and the initial CRT seed.

Each valid cell has exactly these selection inputs: altitude class, fungus,
real-Base item, affiliation nibble and removable-improvement mask. Unlisted
valid cells explicitly default to class3, no fungus, no real Base, affiliation15,
no removable improvements. Invalid coordinates have **no cell**, not a default
cell. Coordinates satisfy0≤x<80,0≤y<80 and `(x+y)%2==0`. Horizontal wrapping
preserves parity; vertical coordinates clip. Flat maps clip both axes.

The baseline projects **77 actual installed-Planet cells** covering every
candidate and its four diagonal neighbors, plus the current Base. Seventy differ
from the declared default and are listed as overrides; the other seven equal
it. All other cells in this replay world are declared defaults. It is a bounded
selection-equivalent projection, not a claim that the entire original Planet map
has this default terrain. Original terrain classes and affiliation are preserved
on the footprint. Nonselector item bits, landmark/region/elevation detail and
rendering data are intentionally outside this selector input schema.

The original native selector was rerun after replacing the **entire** map with
this sparse/default reconstruction. It produces the same eleven candidates,
weights, scores and prior-best values, the same twelve CRT raw values (including
the initial event roll), winner ordinal17 at(63,19), score39 and final CRT state
as the previous complete handler control. Thus weights remain expected results,
not substitutes for map queries.

Four additional small declared synthetic maps exercise boundary rules:

| Control | Raw map setup | Native result |
| --- | --- | --- |
| Flat corner | Base(0,0), fungus(1,1) | Negative X/Y candidates clip; three candidate draws; ordinal2 at(2,0) wins |
| Cylinder corner | Base(0,0), fungus(1,1) and(79,1) | Negative X wraps, Y clips; five candidate draws; ordinal2 still wins, with different intermediate weights/results |
| Later peer wins tie | Ordered Bases ID0=(64,16), ID1=(66,16), currentID0; no fungus | Outer candidate25 at(65,11) ties and belongs to later peer1, so it is skipped; first drawing winner is ordinal36 at(62,20) |
| Later current wins tie | Ordered Bases ID0=(66,16), ID1=(64,16), currentID1; otherwise same map | Candidate25 ties and belongs to later current1; it draws and wins at(65,11) |

Both tie maps have two owner1 Bases. IDs are global record order, not faction
IDs. The native distance metric creates ties beyond a simple X midpoint; do not
replace it with Euclidean or Manhattan distance. Each tie control consumes only
the event roll and one outer-candidate CRT draw. First24 candidates draw nothing
without eligible fungus neighbors. Both topologies and the complete ordered
Base records are literal inputs in each case, with no external map dependency.

An independent semantic checker consumes only these sparse inputs, offsets and
initial CRT seed and recreates the candidate stream, nearest-Base choices,
strict winner comparison and final RNG state. All five replays match original
execution. It also checks that baseline query reads lie inside the projected
footprint, and that clipping/wrapping controls actually exercise their named
boundaries. The original native calls stop after selection, **before H increment**;
this fixture alone does not run landmark protection, mutation, unit creation or
climate. Use the parent receipt for those separate stages.

## Warming counter authority

`warming_count` is a **global saved word**, distinct from faction H and the
per-owner ecology aggregate. A new bounded ordinary file-body control saves17,
poisons it to777, and restores17, with no RNG. It uses a memory-backed file and
zero Bases/Units; it does not advance climate or modify an installed save.

The existing [initial resource-modifier reset](initial-resource-modifiers.md)
already poisons and checks the ten global event words including this counter:
the admitted new-world reset block clears it to0. That global reset is its
established fresh-world authority; the faction initializer/owner aggregate reset
is not a substitute. The parent handler control already proves H6→7 human/no-Voice
land spawning increments it0→1, while the one-shot/Voice holds leave it0.

Source also shows the handler clears this counter when its warming-frequency
threshold is reached, before later climate work. That threshold-clear path is
source-backed here; no new runtime climate/sea-level investigation is claimed.
This supplement establishes reset/restore and provides the missing selector
inputs without resuming broad standing research.
