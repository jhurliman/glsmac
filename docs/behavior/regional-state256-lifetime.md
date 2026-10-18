# Regional threshold input: retained game-state bit256

RE-SMAC-REGIONAL-STATE256-001, 2026-10-18. Pinned stock SMAC.
[Twenty-two native controls](regional-state256-lifetime-cases.json).
This closes an input of the [regional strategy threshold](regional-research-summary-producer.md),
without repeating its dominance or pressure arithmetic.

## Identity and actual consumer

This is **mask `0x00000100` in the secondary game-state word**, the same
runtime/status storage family previously identified for Hurry's transient
blocking state. It is not mask256 in the primary Standard Rules options,
a faction field, the regional flag256, or a permanent setup preference.
The regional summary reads the current word, shifts right7 and masks2:
subtract2 from Q when bit256 is set; subtract0 when clear.

Its identified geometry writer is the **native region builder**. A descriptive
port name is `retained_land_region_concentration`; the original symbol name
is unknown. “Retained” matters: the word can differ from a fresh classification
of current terrain between its real update boundaries.

## Exact builder update

After region discovery and area posting, read the stored signed area words
for **region slots1 through62 inclusive**. For ordinary nonnegative counts:

```
total = sum(storedArea[1..62])
largest = max(storedArea[1..62])
threshold = floor(4 * total / 5)
bit256 = largest >= threshold
```

The update sets or clears only that bit and preserves the rest of the word.
Do not use a strict greater-than or rounded floating-point80% comparison.
80/100 qualifies;79/100 does not. The small rounding discriminator1+1
qualifies because the threshold is1. Zero total also sets the bit (0>=0);
the no-land controls establish this branch, not a playable-world guarantee.
Extreme negative counts and arithmetic overflow remain outside these controls.

Region0, polar land63 and all sea slots64–127 are excluded. Use **stored
areas**, not counts of current final tile labels. The existing
[area producer receipt](coastal-metadata-and-region-area.md) establishes polar
posting and unchanged unused area entries. Here one complete all-land builder
sets the bit with region1 area256; seeding unused normal slot42 with256 makes
the same terrain clear the bit (largest256,total512,threshold409). That is an
explicit stale-state discriminator, not a claim that fresh ordinary maps have
such a canary. Adding huge excluded polar/sea values does not affect the bit.

## Fresh startup and restart are different events

The **whole-new-world initializer** replaces the secondary word with only its
two randomization-preference bits. It therefore clears bit256. Two actual
entry-to-first-child prefixes confirm this from both prior bit values, retaining
those unrelated preferences and setting current turn0.

The ordinary setup source can call map generation before that whole-world
initializer. The generator reaches region building and can set bit256, but
the later new-world reset clears it again. **A prior map build is not evidence
that the bit remains true at the strategy caller.** The earlier admitted
first-worker world is a staged preparation that invokes the builder and faction
fragments; it does not execute this complete whole-world reset sequence.
Its retained set bit cannot establish a universal original opening value.

Conversely, the checked **nonzero-turn faction initializer prefixes preserve
the current bit**, both0 and1, for owner0 and positive owner1. They do not
rerun the whole-new-world reset or synthesize a new geometry value. This is a
faction restart boundary, not a claim about every frontend/session restart.

For integration, represent a saved mutable state input and apply the actual
events in caller order: whole-new-world reset clears it; a later executed
region builder recalculates it; nonzero faction setup preserves it. Read that
retained value in the strategy caller. Do not hardcode true, call the builder
from valuation merely to resolve it, or continuously derive it from terrain.
This receipt does not certify a continuous full frontend-to-first-command
value for every generated/imported/scenario route.

## Later writers, clearing and restore

The region builder is reached through the terrain rebuild/postprocessing path.
Source-inspected callers include the generator, completed terrain-action
branches, applied sea-level steps, selected ecological events, battle/terrain
effects and map/editor paths. Existing full terrain-rebuild controls execute
the builder through that caller. Each actual rebuild can set **or clear** the
bit by the same rule; not every movement, Base event or resource preview calls
it. These call sites are not a new reconstruction of every action's admission.

The other identified whole-state clear is the new-world reset above. The direct
executable-reference inventory found the builder's set/clear operation and that
reset; it is not an indirect-alias proof of every possible memory writer.

Two actual ordinary file-body round trips save bit0/1, poison the current word
to the opposite value and restore the saved value. **Neither read calls the
region builder.** Restore therefore preserves the retained bit even when a
query-time terrain classification might disagree. These file controls use a
declared0-Base/0-Unit diagnostic world and consume no gameplay/CRT RNG in the
file bodies. Complete frontend post-load work remains a separate boundary.

Validation comprises eight complete region builders, six original area-reduction
fragments, two whole-new-world prefixes, four nonzero faction prefixes and two
ordinary round trips. No port production code, original file or live window
was changed; SMAX/PRACX behavior is unexamined.
