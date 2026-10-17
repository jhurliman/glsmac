# Owned SMAC gameplay RNG continuation

The admitted SMAC simulation now owns the two independent uint32 gameplay
words beside its roster, technology, designs and social state. This uses the
existing `SmacGameplayRandomState` recurrences and exact 16-byte `SMGR` codec.
Neither word is the map generator's JSF32 state. Every simulation value requires
an explicit pair at construction; `(0, 0)` and all other uint32 pairs are valid.

## GLSMAC new-world policy

During the claimed authoritative Start operation, `PrepareNewSimulation` builds
the participant and source-derived values, then calls the existing static
`util::random::Random::NewSeed()` twice: shared word first, CRT word second.
Each call directly samples `std::random_device`; no `Random` object is
constructed and no JSF seed, warm-up or draw occurs. The complete value is
bound and published before `create_world` and `start` consumers. A private typed
word-source callable permits deterministic tests of the actual production path.
It is not a script or remote mutation API.

This is an explicit GLSMAC initialization policy, not reconstruction of the
original application's clocks or full setup chain. Sampling or final binding
can fail; no partial pair or simulation is published. External entropy already
sampled is not rolled back. A cancelled Pending Start samples nothing. A Running
Start retains the existing owner-operation boundary; its later reset retires
the entire value. Ordinary repeated simulation preparation rejects before
sampling. SMAX has no SMAC simulation pair and never invokes this policy.

## Saved state and validation

Simulation V4 appends the existing RNG codec in a framed string capped at exactly
16 bytes before materialization. Its marker, version and exact length are then
validated by the existing codec. The complete simulation still has its existing
64 KiB port budget. World V14 requires this continuation; V1 through V13 world
headers and older simulation versions are unsupported. Frozen GlobalSettings
and individual Unit codecs do not change.

The actual world reader stages and validates the entire simulation before Map
allocation. Restoring neither samples entropy nor replays startup producers.
The locally bound value is published before `start`, checked again after setup,
and retired on reset/cancellation/failure with all other simulation fields.
Snapshot writing only copies the pair. The existing separate late JSF install
continues to discard map/setup-only JSF draws. Same-occupant roster refresh and
retained reconnect preserve the saved gameplay pair.

Owned continuation is a GLSMAC reproducibility contract. The original
RE-SMAC-RNG-SEAMS-001 establishes independent setter/draw replay, not the
new-world seed chain. RE-SMAC-RNG-SAVE-LOAD-001 observed ordinary original file
bodies preserving the current words rather than restoring their saved values;
Iron Man has a separate CRT clock policy. The first RE-SMAC-FULL-RULES-001
runs observed unchanged words through complete 1,071-numeric-read rules loads
only with a fresh parser and mostly zero global state. Later investigation
using the complete constructor table, clock seeds and initialized PCX/graphics
state reaches a shared draw during faction graphics/palette setup inside the
loader. The earlier fixture does not establish that rules loading universally
preserves either continuation. Final setup-to-command seed/draw origins remain
open. This change makes no original file-format, exact startup-seed, multiplayer
lockstep, movement, combat or live command-consumption claim.

## Validation boundaries

The native value tests cover both uint32 endpoints in each stream, literal
all-zero state, independent owned copies, repeated continuation, older framing,
wrong marker/version, missing/truncated/trailing records and complete 17/4096
byte outer frames. Oversized strings fail the outer budget; exact-size malformed
strings reach nested validation. An explicit cursor check distinguishes these
boundaries.

Existing actual claimed Start fixtures now inject ordered words, inspect both
create/start callbacks, and assert exactly two samples, unchanged JSF and no
startup gameplay draws. First- and second-sample exceptions leave no simulation
or world continuation. Pending and Running cancellation retain their actual
MT/owner sequencing. Actual SMAX Start/restore tests install a throwing entropy
source to prove the policy is unused.

The real writer/body decoder/claimed InitComplete fixtures preserve progressed
pairs, sparse simulation IDs, all mutable state and complete snapshot bytes.
They install a throwing destination entropy source, cover setup failure and
reset, real canonical Client roster refresh/reconnect, and compare the next 32
outputs of each stream. The actual immediate/queued Client final-chunk matrix
rejects malformed gameplay records before Map publication or callbacks. Earlier
whole chunks are preloaded by the fixture; only the final Packet is dispatched.
Maps and accepted participants are synthetic; this is not rendered/socket smoke.

Full native normal CTest passes 8/8 (94.42s); full leak-enabled ASan/UBSan/LSan
CTest passes 8/8 (156.18s). The same 45 focused native groups pass with allocation
tracking in both builds. Explicit LLVM15 correctness analysis of Game,
GameSimulation and SmacSimulationState passes all three TUs, zero failures,
in 17.50s; this is not a fresh full-tree/platform analysis claim.

Three outside-checkout copied translation-unit controls each exit 1 at the
intended assertion: omitting the second entropy sample; replacing a decoded
pair with zeros; and removing the pair's outer frame cap. The intended source
was never temporarily mutated. Receipts are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/gameplay-rng-*` on the validation
host. No original executable or proprietary asset is needed by these tests.
