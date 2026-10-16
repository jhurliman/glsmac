# Pure SMAC ordinary tile mineral calculator

`game::backend::resource::smac::CalculateTileMinerals` implements the non-center,
normal-mode mineral calculation in RE-SMAC-TILE-M-001. The result contains
signed minerals and this tile's combined road-restriction/technology-cap removed
amount. It does not mutate the original-style global removed-resource counter,
calculate base inefficiency/support, or connect to live tile callbacks.

Every input is required, without neutral defaults for absent gameplay systems:

- Seven signed rules: the Ocean, Bonus, Forest, Improved Sea, Monolith and
  Borehole mineral columns, plus the mine-without-road limit. Loading these
  values from the selected rules remains separate work.
- Altitude, decoded rockiness, effective resource type, eligible mineral
  landmark point, and forest/fungus/borehole/monolith presence.
- Mine/platform and road presence. The API does not infer roads from mag-tubes.
- Actual ownership of the configured mineral-limit and advanced-platform
  technologies; already-built fungal mineral capability, effective PLANET,
  and already-active Industrial Boom/Bust.

The mineral-specific precedence is monolith, borehole, qualifying fungus, sea,
land forest, then other land. Monolith and fungus bypass the technology cap;
borehole does not. The road restriction affects only the separately calculated
mine contribution, before that contribution joins the rockiness/resource/
landmark total. A mineral resource exempts the technology cap but does not
exempt the road restriction. A resource and landmark together add only one
extra mine-contribution point. Removed amounts from both restrictions add, and
events run afterward. Negative custom rules and limits can produce negative
yields; no lower clamp is invented for them.

Tree Farm and Hybrid Forest are intentionally absent from this API: the audited
mineral routine does not use their presence. The research's facility comparison
case is retained, and the original-executable comparisons include forests
with and without both facilities present. This is a verified absence of an effect, not an assumption
that an unmodeled facility is missing.

## Input and arithmetic domain

All eight altitude bands are accepted. Decoded rockiness accepts integers 0..3;
3 is retained as an original-x86-tested encoded boundary, without naming it as
a normal terrain class. This differs from the nutrient API's three named
rockiness categories and does not alter that API. Invalid altitude/resource
enums, rockiness outside 0..3, and an effective resource below shelf throw
`std::invalid_argument` in release builds. A native test calls the real resource
resolver before mineral calculation across all altitude bands.

The constructed fungal capability accepts 0 through `INT32_MAX`, following
RE-SMAC-FUNGAL-001's floor at zero without a 99 cap. A negative capability is
rejected. Rules and PLANET accept the full signed int32 range. The calculator
uses int64 arithmetic and checks that every evaluated result, including removed
amounts, fits int32. It throws `std::overflow_error` for unsupported original
overflow before a later cap or cancellation. The original evaluates the initial
resource-plus-landmark sum before special branches replace it, so that sum is
also checked even on a monolith, borehole or fungus tile. These domain rejections
are port policy, not claims about original malformed-state handling.

The contract assumes a valid evaluating faction/base and a non-center tile in
normal mode. It does not define special preview/mining modes, landmark geometry,
capability construction or refresh timing, event scheduling, terraforming
legality, base production, a live adapter, or SMAX equivalence.

## Evidence and validation

The test-owned semantic fixture contains the research handoff's 42 named cases
from `docs/behavior/ordinary-tile-minerals-cases.json`, SHA-256
`0e00bc7400751550769bda989051934b0429f9deec9eff8b829940f6172274bf`.
Its explicit mineral selectors become None below shelf and Minerals otherwise;
facility fields with no mineral effect are omitted. The pinned SMAC
`terran.exe` SHA-256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
No original code, instruction bytes, executable layout or assets are committed,
and human-owned research files are not modified.

A local Clang 15 UBSan build of the production calculator matched the complete
original mineral routine and its original resource/technology helpers for
**2,658 / 2,658** synthetic cases in Unicorn 2.1.4: 42 named cases, 2,048 research
grid combinations, 210 signed custom-rule combinations, 224 fungal capability/
PLANET/event boundaries, 128 special-precedence combinations, and six further
signed endpoints/event cases. No helper return values were hooked. The private
harness was loaded without executing its reference-file writers, and the
installed executable remained unchanged. These are routine-level checks, not
full-game observations or SMAX comparisons.

Separate negative copies incorrectly exempted boreholes from the technology cap
(27 mismatches) and mineral-resource mines from the road restriction
(68 mismatches). The production source remained unchanged. An additional 22
UBSan endpoint, invalid-input and overflow probes passed without sanitizer
diagnostics. Fifty native cases/groups cover the named expectations and
interaction/boundary checks. The full local Clang 15 / WSL CTest suite passed
all seven entries (35.06 seconds). Focused `clang-tidy-15` and independent code/
evidence review are clean.

The ignored local `build/tile-minerals/original-comparison.json` records source
and fixture hashes, comparison counts and negative-control counterexamples.
Build/test/analyzer logs are preserved under `build/tile-minerals/validation/`.
Proprietary emulator inputs remain outside the committed test suite.
