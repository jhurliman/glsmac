# Native continent seeding and growth

`SmacContinentGeneration` consumes the owned raw plane and continuation from
[generation bootstrap](generation-bootstrap.md). It implements the original
seed routine 0x5da7f0, growth walker 0x5da700, paint stencil 0x5da370 and their
turn-zero class/contour/resource effects. The application still uses its current
generator while later stages are completed; this component does not publish a
partial map into gameplay.

## Evolving state

The component mutates the existing 44-byte records, CRT state, land progress,
per-walk paint count, dirty flags and flood/affected-Base scratch. Shared RNG
state, land target and the caller's continent ordinal/attempt counter survive.
Geometry and numeric continent rules are explicit; categories, topology,
resource pattern and working contour thresholds remain in the owned state.
Callbacks are synchronous, optional, and cannot mutate/reenter that state.

Every seed pass clears byte 3 across the complete plane before trying up to
100 candidates. Flat/round coordinate ranges and divisor<=1 draw suppression
follow the original order. Requested continent size uses land category, current
progress, ordinal-to-world-size ratio and the five strict ratio thresholds.
Reaching the land target changes both size scaling and the small-continent draw.

Growth preserves the unconditional direction draw after painting, repeated raw
direction/streak correction and boundary restarts. Restarts clear the repeated
direction state. The original 2,000-step check counts extra exit bookkeeping;
the no-paint control therefore makes 1,955 direction draws. Paint count can pass
the requested target within a stencil and counts admitted centers, independently
of neighboring contour changes.

Painting considers center/NE/SE in order, skips the E stencil entry, and enforces
the temporary membership and exact-band bridge restrictions. Membership is
written before shaping. The class setter preserves low climate bits; contour
jitter uses the bootstrap working thresholds and consumes CRT only for spans
greater than one. Changed rings preserve actual occupants or consult the existing
deterministic resource resolver before clearing the low occupant nibble.

Generation and climate now share the already verified 225-offset shaping order
and ring ends. Generation retains its bootstrap threshold profile and single
width adjustment at boundaries; later climate retains its existing typed world
and 20-unit contour profile. This change preserves the existing climate behavior.

Optional trace hooks expose ordered byte writes, semantic draw sites, candidate
positions, walks, paint calls, direction state and the post-walk checkpoint.
They allocate no traces when omitted. Test vectors initialize only when tests
request them, avoiding their vector allocations during ordinary startup.

## Original evidence

Frozen `continent-generation-evidence-20261020` has manifest SHA256
`e4cd97f9b086a0f0c610476a729c6204777ab6bc32171f4fd8be45a51a640d4b`;
all 12 files were verified. The independent original checker passes 50 controls,
7,331 CRT draws and 113,022 exact ordered tile-byte writes. Its real native
helpers execute; only the CRT TLS lookup and final event pump are supplied.
These are complete callee/return controls with declared raw inputs, not the
entire generator or a GUI run.

The [numeric fixture](behavior/continent-generation-cases.json) retains inputs,
tables, intermediate return hashes/counters, ordered-write and RNG hashes,
candidates/walks, paint/direction hashes and scratch results. Full private native
journals and purchased executable bytes remain outside Git. Byte-write hashes
include unchanged writes; RNG hashes include semantic sites mapped to the
original return-address tags, not C++ instruction addresses.

The first control executes the actual production bootstrap from seed 1 and
poisoned allocation bytes, then passes its output directly into the production
seed routine. Its complete input hash is
`662bea444e62e652ce65f01e8bfa6afdc8a511e397b470721de9eef49249efb2`.
This checks the original shared state 1015568748, CRT state 1, pattern 11446,
target 640 and preserved auxiliary/owner bytes at that boundary. Other controls
cover three consecutive continent calls, size/progress thresholds, candidate
exhaustion, draw suppression, resource settings, seams, flat restarts, bridge
rejection and the walk cap.

## Verification and remaining stages

The cached incremental build and six selected continent/bootstrap/climate groups
pass. Production matches every complete and intermediate return hash, ordered
write/draw trace, candidate/walk sequence, paint/direction trace, scratch summary
and progress/checkpoint count in all 50 controls. Three production TUs pass
correctness analysis. The same six groups pass under focused six-TU ASan/UBSan
(four production TUs and two generation test TUs); other objects are
uninstrumented and leak detection is disabled. No platform matrix or graphics
launch was repeated for this detached stage.

The actual outer generation controller, hill/polar shaping, contour morph,
altitude/region reconstruction, rejection/retry and later climate/landmark/flora
preparation remain required. Its outer iteration and retry limits must be ported
from their own caller boundary. Full generated startup remains a separate
acceptance gate, including active mask 255 and live map publication. Existing
[generated-scene renderer parity](generated-terrain-appearance.md) does not prove
the incomplete port generator produces that scene.
