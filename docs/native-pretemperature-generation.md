# Native generation through first temperature entry

`GeneratePretemperature` now composes genuine algorithms from initial owned
storage through the original first temperature entry (`0x5e02af` calling
`0x5dc060`). It does not replay captured intermediate maps or helper journals.

The actual original setup's seed-1 Standard Rules case matches byte-for-byte:

| Output at first temperature entry | Bytes | SHA256 |
| --- | ---: | --- |
| Tile plane | 140,800 | `bbc3bdc35928316e48e2fc1f5e98370f00f4a59e2356697e17359e6555c1e215` |
| Full region storage | 3,584 | `a173f98907300d2531ba565eeb4ba86041c4d32ce163f0445ec304e0042fda5b` |
| Summary grid | 128 | `60a88af59ddf64f017c4530709020b1e834f5a0c9a2d76560cf6a3d2cf697cd4` |

This starts with zeroed allocation, seed 1 in both owned RNG streams, 80x80
geometry, categories `[2,1,1,1,1,1,1]`, Standard Rules flags, and a 16x16 summary
whose new storage starts at zero. It reaches carried continent ordinal 103,
one rejection/retry, CRT `3828047505`, shared state `1586005467`, resource pattern
17133, progress/target 642/640, painted count 10, dirty flags 4 and secondary
flags 0. Final generation flags remain 1/1.

## Composition

Each attempt bootstraps records/categories, performs at most 99 continent
calls, adds the erosion/ocean-dependent hill count, and runs contour morphing.
Morphing constructs native noise once, rebuilds regions, adjusts sea contours,
reconstructs altitude classes, rebuilds regions again, then adjusts land
contours. It preserves the original integer truncation, square-root scaling,
neighbor order/ties and in-place contour writes. Neither pass updates classes
per tile.

The caller then conditionally seeds polar contours, reconstructs altitude,
rebuilds regions, and compares the largest two land regions. Rejected attempts
preserve continent ordinals, RNG and region/summary history while bootstrapping
a fresh tile attempt. Four rejected attempts leave the last native plane ready
for temperature, as the source does; the returned result distinguishes this
from an accepted map. `Stage::Pretemperature` marks the precise stop boundary.

Caller-owned geometry, WORLD rule coefficients, primary rule flags, initial
record bytes, summary geometry and allocation initialization are explicit.
Ordinary generation forces wrapped topology during bootstrap. Direct morph
controls also cover flat geometry. Arithmetic guards retain the measured
signed32 domain; arbitrary overflow-inducing rule mods are not verified.

## Original controls and validation

Fifteen isolated morph cases cover land/erosion categories 0/1/2, ocean
extremes, flat/wrapped coasts, scaling and zero coefficients. They match full
tile/region/summary output, **72,093 ordered tile stores**, **27,751 ordered
region stores**, **12,580 ordered CRT draws**, and checkpoint counts.

Nine complete generation controls cover **19 attempts** and **205,014 ordered
CRT draws**, complete tile output hashes, all region areas, attempt histories,
outgoing random states/globals and checkpoint counts. Cases include the
99-call continent cap, zero budgets, four natural rejected attempts, and hill
search exhaustion. These start from deterministic poisoned initial records;
the separate actual-setup test starts from zeroed allocation and compares all
three full output buffers above.

Full-prefix ordered tile-store traces are not asserted: bootstrap's reset
stores are not exposed by the port trace API. Each constituent reset/shaping/
morph/region operation has its own original comparisons. Do not equate the
prefix's final-byte and ordered-RNG proof with an ordered-store proof.

The cached incremental build, eight affected native groups, correctness
analysis of all five production consumers of generation state, and focused
eight-TU ASan/UBSan run pass. No platform matrix was repeated locally.

Source executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The 32-file frozen `native-pretemperature-generation-evidence-20261020` bundle
was verified against manifest SHA256
`3ccbd8480e7ae54006bc663cb1336385046cf60e33d9897da2c097352d48c3d3`.
Fixtures retain numeric observations only; purchased assets remain outside Git.

## Remaining live integration

This completes the native producer up to temperature. Temperature, river-source
placement, initial fungus, landmarks, later climate/routing passes, final
preparation and faction startup must still compose to the verified full-world
state before this can replace live procedural generation. Current generated
gameplay rendering is unchanged. The [generated terrain comparison](generated-terrain-appearance.md)
verifies the destination rendering path, not completion of the live generator.
