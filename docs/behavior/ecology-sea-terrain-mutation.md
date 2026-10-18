# Ecology sea terrain mutation: native one/nine-cell and depth behavior

RE-SMAC-ECOLOGY-SEA-MUTATION-001, 2026-10-18. Pinned stock SMAC.
[Literal sparse inputs and complete semantic deltas](ecology-sea-terrain-mutation-cases.json).
Twenty-six native handler prefixes and independent terrain/RNG replays pass.
They stop after terrain mutation, before native-unit spawning or warming.

**Choosing the nine-cell branch does not produce nine direct fungus writes in
this pinned executable.** The first eligible cell's improvement-notice scan
writes19 into the bloom loop index; the next iteration test ends the loop.
This refines the source-only sea extension in the earlier
[handler/cache handoff](ecology-handler-research-cache.md). Preserve the native
quirk rather than implementing an assumed nine-square bloom.

## Admission and one/nine-cell decision

The existing ordered selector runs unchanged, using the sparse terrain and CRT
seed in each fixture. Current damage51 and first `rawCRT%100==50` admit the event.
H increments once after selecting the winner, even if every mutation cell is
subsequently protected. The sea decision uses **new H**:

- Selected altitude class≥3, or new H<4: nominal budget1, no cluster draw.
- Otherwise `n = 10 - trunc(newH/8) - ownerDifficulty`.
- For n≤1: nominal budget9, **no** cluster draw.
- For n>1: one further existing CRT draw; remainder0 modulo n gives budget9,
  any nonzero remainder gives budget1. No shared RNG is used.

| H before → after | Difficulty | n | Native cluster control |
| --- | --- | --- | --- |
| 2 → 3 | 2 | not evaluated | Budget1, no extra draw |
| 3 → 4 | 2 | 8 | Raw29208 remainder0 → budget9; raw29249 remainder1 → budget1 |
| 4 → 5 | 2 | 8 | Same remainder0 boundary gives budget9 |
| 23 → 24 | 5 | 2 | Raw23942 remainder0 →9; raw30539 remainder1 →1 |
| 31 → 32 | 5 | 1 | Budget9 without draw |
| 39 → 40 | 5 | 0 | Budget9 without draw |

High-H rows are declared counter controls that stop before spawning/climate;
they do not replay natural campaign arrival at those values. Selected class0,
1 and2 all take the sea decision. Earlier selector eligibility still permits
existing fungus below class2, and excludes existing fungus at class2 or higher.

## Exact iteration, protection and the loop-index quirk

The nominal nine cells use this fixed order relative to the winner:
`(0,0),(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)`.
For each attempted cell:

1. Wrap X once on a cylinder, then reject out-of-range X/Y. Flat maps clip X;
   Y always clips. Clipping does not renumber the remaining offsets.
2. For noncenter cells only, skip altitude class≥3.
3. Skip existing fungus at class≥2.
4. Skip real-Base item bit1 with affiliation nibble0..7.
5. Skip landmark word satisfying `(word & 0x80000002) == 2`.
6. At the first eligible cell, perform the original nineteen-entry improvement
   notice scan. Its final counter19 overwrites the current bloom loop index.
7. Set fungus bit32, clear exactly `0x00288050`, and, if class<2, run the actual
   terrain shaper targeting class2 in **class-only mode**.
8. The loop increments that overwritten index to20, which exceeds budget1 or9.

Thus a normal unprotected winner is the only directly mutated cell. If the
winner is protected, scanning continues until the first eligible neighbor; that
neighbor is then the only direct fungus mutation. If all eligible cells are
protected/excluded/clipped, it can attempt all nine and mutate none. The same
index overwrite occurs when the first eligible cell has **no removable
improvements**; it is not conditional on finding an improvement.

Literal examples on16×12 worlds:

- Unprotected shelf winner(9,5), budget9: only(9,5) receives fungus.
- Protected winner(9,5), next neighbor(10,4) already fungus: first eligible(11,5)
  receives fungus; processing stops there.
- Protected winner, existing-fungus neighbor, a real second Base and land
  neighbor: processing reaches(9,7), mutates it and stops.
- All nine protected/excluded: H3→4, nine attempts, no item/terrain writes.
- Winner(0,0) with valid neighboring cells protected: flat map has no write;
  cylinder wraps west and mutates(14,0). Raw negative Y attempts still clip.
- Setting the protected winner's sign bit makes it writable; the exact mask
  test, rather than the low membership bit alone, governs suppression.

The fixture records every attempted index/coordinate, the actual scan's
index-before/index-after values, ordered item/class helper calls and final
terrain deltas. These are actual unchanged original instructions, not an emulated
replacement for the loop or a supplied selected tile.

## Exact item changes

The direct fungus cell becomes `(oldItems | 0x20) & ~0x00288050`.
Removed bits are Mine, Solar Collector, Farm, Soil Enricher and Forest. Road,
Mag Tube, Condenser, Mirror and Borehole are preserved. Ordinary canary bits
`0x01c0000c` survive; a separate control sets **every other non-event item bit**
and proves the complete complement is preserved, not just those named examples.
Those simultaneous improvement/unknown-bit inputs are explicit diagnostics,
not natural construction-admission claims.

Slope-propagated cells do not receive fungus or the event's improvement clear.
Their removable improvements can remain intact even when their altitude band
changes. Existing deep fungus is retained while its cell is raised to class2.

## Deep-water target-class2 producer

For direct class0/1 mutation, the original terrain helper first sets that cell
to2, then executes ordered slope passes. Shallow class2 does not call it.
The complete offset order and ring boundaries are literal fixture inputs:
center0, ring1 indices1..8, ring2 indices9..24, ring3 indices25..48, ring4
indices49..80, ring5 indices81..120.

- Lower pass: ring1 cells below1 become1. For target2 this is the only lower ring.
- Upper pass: ring r cells above `2+r` become `2+r`, for r1..5. **Stop the upper
  walk after the first ring with no class change**; do not apply a global radial
  clamp to every distant cliff.
- Each pass wraps/clips each ordered coordinate independently. No extra sorting
  or deduplication is introduced.

The all-class1 neighborhood changes only the direct cell1→2. In the class0
fixture, the direct cell becomes2 and six eligible surrounding class0 cells
become1; the preexisting class2 fungus cell and class3 Base remain unchanged.
A class6 cliff in ring1 lowers to3; if that changes ring1, a class6 cliff in
ring2 lowers to4. The same farther cliff remains6 when ring1 needed no change.
These target2 slope changes preserve each cell's land/sea category and saved
region/affiliation in the measured controls; no region rebuild occurs here.

Protection above governs **direct fungus mutation**, not the shaper's slope
writes. A protected neighboring class0 cell can become1 indirectly. Its items
stay intact, including removable improvements. The native class setter also
invalidates landmark detail: word2 becomes `0xff000002`, preserving low
membership while changing the high detail/sign byte. A protected center can
likewise change indirectly when the later eligible neighbor becomes the actual
fungus/shaping center. Persist the full resulting landmark word.

Land lowering can add CRT work. For old rockiness R>0 and `R > newClass-3`, the
class setter draws `rawCRT % (R+1)`: zero leaves rockiness; nonzero reduces it by
one and sets landmark metadata bit `0x00400000`. Literal class6→3, R2 controls
show remainder0 retains2, while remainder1 reduces2→1. Two cliff rings consume
two such draws in actual setter order. The ordinary all-sea raising controls
consume **no** additional terrain RNG. No shared RNG entry occurs in any case.

## Contours, shared corners and staged render deltas

This path passes mode0 to the terrain producer: it uses the **class-only**
setter, not the contour-resampling setter. Across all26 controls every contour
byte remains unchanged, including raised abyss cells and lowered cliffs.
Moisture/climate codes, region, affiliation, low terrain auxiliary bits and
remembered-item words are also preserved here. Rockiness and landmark detail
change only in their recorded setter paths.

The current GLSMAC installed-Planet display projection derives shared corner
heights solely from contour samples. Consequently the supplied corner output is
an **empty shared-corner height delta**: keep those existing corners. Do not
synthesize new contour noise, infer geometric height from the changed resource
altitude band or advertise measured original per-vertex mesh geometry. The
fixture includes complete semantic input cells and changed-cell before/after
values, with explicit empty contour/corner updates.

Stage actual fungus/improvement changes and all class, rockiness and landmark
changes over the entire shaper footprint, then publish the corresponding map
views together. The class setter sets dirty mask4; changed rockiness adds mask1.
Direct item-only invisible controls leave that native global dirty word0 despite
real item writes, so it is not a complete list of semantic tiles to publish.
Use the recorded changed-cell set. These headless controls deliberately use
viewer0, actor1 and zero visibility; visible notification/redraw callbacks and
live raster output are separate from these terrain authorities.

## Scheduling boundary and evidence limits

All26 controls retain stored damage51, add the already established aggregate
contribution7→58 and leave the remembered/current calculation Base IDs unchanged.
H increments once. They explicitly stop before post-mutation retention,
one-shot/H/Voice suppression, native factory calls and warming. The warming
canary777 stays777 and no unit factory runs. Do not interpret those untouched
outputs as a claim that high-H full handlers never spawn or warm; those stages
remain the earlier receipt or later bounded work.

No resource-upkeep posting or extra post-bloom resource refresh is added by this
terrain handoff. The independent checker reconstructs selection, RNG, the actual
loop-index behavior, class/slope writes, rockiness and landmark changes from
semantic inputs and matches every final delta/call order/CRT state. No original
terrain, selection or random result is supplied. No production edits, original
asset writes, live GUI or broad climate investigation was performed; the broad
standing research goal remains paused.
