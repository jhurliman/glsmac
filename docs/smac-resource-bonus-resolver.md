# SMAC resource-bonus resolver

`resource::smac::ResolveResourceBonus` resolves explicit logical inputs to
`None`, `Nutrients`, `Minerals` or `Energy`. Its output enum is shared with the
SMAC base-square calculator. It is a pure component; no live map or yield
callback uses it yet.

Every input is required: doubled-x coordinates, one of eight decoded altitude
bands, a signed resource-pattern offset, the automatic-land-resource switch,
forced presence, type override, and energy/mineral markers. The five land-band
names retain their numeric identities without inventing height thresholds.
The offset is not a general game RNG seed. Port tile bonuses have a different
numeric ordering, so a future adapter must translate semantics explicitly.

Presence is resolved before altitude and type. Forced presence bypasses the
automatic gates and phase match, but trench/deep ocean still return none.
Otherwise a nonzero coordinate type wins unless type override is enabled.
Fallback gives energy priority over minerals, then nutrients. Zero offset
disables automatic presence, while a nonzero multiple of sixteen does not.
Floor division and nonnegative residues preserve the phase for negative `v`,
which occurs even when both map coordinates are nonnegative.

The port contract requires nonnegative, matching-parity signed-int32
coordinates and valid altitude bands. It does not wrap coordinates across map
seams or interpret negative coordinates as aliases. Invalid logical inputs
raise `std::invalid_argument`. Evaluated signed-int32 arithmetic intermediates
are checked using int64; values outside that domain raise
`std::overflow_error` rather than guessing original overflow behavior.
Unused automatic-pattern arithmetic is not evaluated when presence is forced.
The original still constructs the coordinate phase before its deep-water
altitude rejection, including when presence is forced. The checked port domain
therefore also rejects an overflowing coordinate sum in that branch. This does
not claim an original overflow result outside the verified arithmetic domain.
These validation boundaries are port policy, not claims about original error
handling or extreme-overflow behavior.

The behavior comes from **RE-SMAC-RESOURCE-001**, the locally retained semantic
handoff `docs/behavior/resource-bonus-resolution.md` and its 15 named cases in
`docs/behavior/resource-bonus-resolution-cases.json`. Those research files are
not included in this change; the test-owned case fixture is
[`src/gse/tests/fixtures/SmacResourceBonus.h`](../src/gse/tests/fixtures/SmacResourceBonus.h).
They are original-x86 checked using synthetic map/global state, not observed
full-game scenarios. The reference `terran.exe` SHA-256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`;
the semantic fixture SHA-256 is
`6dce77a0cad7947f7df5884f29b72277883aa5d65270d42a0e3c2a381a4ed7dc`.
No original code, executable bytes, map layout, or proprietary assets are
included in the production component or committed fixtures.

During implementation, the compiled resolver with UBSan was compared directly
against that entire original routine in Unicorn 2.1.4, without dependency hooks.
All 155,669 comparisons matched: 15 named cases, the handoff's 155,648-case grid,
and six extra negative-v/signed-offset cases. A separate compiled copy using
truncation instead of floor division mismatched 1,515 comparisons; a copy giving
minerals priority over energy mismatched 10,284. The production source was not
changed for these negative controls.

CI uses test-owned semantic fixtures and needs no original files or emulator.
The native suite also checks all phases and bands, offset-zero distinctions,
negative-v arithmetic, invalid inputs, representable boundaries and overflow
rejection. It runs through the existing `--gse-tests` / `gse` CTest entry.
Local Clang 15 validation passed all seven CTest entries; the standalone UBSan
probe also passed fourteen invalid-input, representable-boundary and overflow
cases.

Map generation and persistence of resolver metadata, editor/resource-removal
behavior, map seam handling, live center/ordinary-tile integration, and SMAX
equivalence remain separate work. This component supplies none of those
missing inputs implicitly.
