# SMAC ordinary tile energy component

`game::backend::resource::smac::CalculateTileEnergy` implements the bounded,
pure arithmetic contract from **RE-SMAC-TILE-E-001**. The evidence status is
**original-x86 checked, not full-game observed**. It applies to valid non-center
tiles with a valid, consistent evaluating base/faction in normal evaluation
mode. It does not activate these rules in a running game or claim SMAX parity.

The reference is the installed GOG SMAC `terran.exe` with SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` and
`alpha.txt` with SHA-256
`38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
The research-owned handoff is `docs/behavior/ordinary-tile-energy.md` and its
50-case JSON. The test-owned semantic fixture records the source JSON hash
`f098518ffcab49a1dd406e83b2b8f8b7d5c3b6f78798d5488cc8499f549335c9`.
No original executable bytes, executable implementation or assets are copied.

## Required inputs and results

All input constructors require every field. There are no gameplay defaults.

- `TileEnergyRules`: signed energy columns for Ocean, Bonus, Forest, Improved
  Sea, Monolith and Borehole from the active rules.
- `EnergyTerrain`: decoded altitude band, effective resolved resource, eligible
  landmark point, forest, fungus, monolith, borehole and river.
- `EnergyImprovements`: collector/tidal flag, Echelon Mirror flag, and an
  already-resolved count of matching adjacent mirrors from 0 through 8.
- `EnergyContext`: actual Hybrid Forest presence, configured energy-limit
  technology ownership, constructed fungal capability, effective PLANET and
  ECONOMY, Golden Age, Merchant Exchange at this base, and already-active Heat
  Wave, Cloud Cover, atmospheric dust and solar multiplier state.

The result contains signed `int64_t` energy and this tile's pre-event
technology-capped excess. Nothing is written to global or world state. Tree
Farm and Governor are absent because this ordinary energy routine does not
read them. No capability, facility, technology or event is assumed neutral.

The component chooses monolith, borehole, qualifying shelf/land fungus, sea,
land forest, then ordinary land. Qualifying fungus clamps its capability plus
the PLANET penalty to 0..99 and bypasses common additions and the technology
cap. Other branches receive land river, effective energy resource, landmark,
Merchant Exchange and the effective ECONOMY >=2 point, then the cap when
applicable. Events run afterward. The ordinary path has no general 0..99 clamp;
negative custom/editor results retain the original nonzero decrement behavior.

## Mirror and resource boundaries

Mirror count is a resolved scalar, not a request to inspect neighbors. The
future resolver must separately establish doubled-x adjacency, horizontal
wrapping, pole bounds, the neighbor mirror flag, and matching decoded faction
affiliation against the current evaluating base. This API does not equate that
affiliation with any currently implemented territory/ownership field. Capture
timing, very narrow maps and live map integration are outside its contract.

The named fixture's mirror lists are reduced explicitly: cases 7, 8, 9 and 16
use count 8; cases 12 and 15 use count 1; every other case uses count 0. Names
are retained for provenance. Their seam, pole and affiliation descriptions
are **not production geometry tests**. The calculator tests whether supplied
counts affect collectors only, after special/forest branch selection.

Likewise, an explicit resource marker is not the effective resource. Source
cases and the comparison grid are converted to `None` below shelf, and to
`Energy` above that boundary when the source marker requests energy. A separate
native case uses the actual `ResolveResourceBonus` component to verify this
composition. Nutrient and mineral resources neither add energy nor exempt its
technology cap.

## Arithmetic domain

Rules and social ratings accept signed `int32_t`; the constructed fungal
capability must be nonnegative. Invalid enum values, resources below shelf,
negative capability and mirror counts outside 0..8 throw
`std::invalid_argument` in all build modes.

Each evaluated arithmetic step is performed in `int64_t` and checked against
the signed 32-bit range before continuing, including Golden Age's ECONOMY
increment and event tripling. Overflow throws `std::overflow_error`, even if
a later technology cap would reduce the value. No unchecked signed arithmetic
is used, and the implementation does not claim to reproduce unverified
original overflow/wrap behavior. An unselected terrain branch does not evaluate
its rule arithmetic.

## Validation

The normal GSE native suite registers all 50 semantic fixtures and seven
additional groups covering altitude/mirror combinations, real resource
resolution, special precedence, signed event order, endpoints, invalid inputs
and overflow before later caps. They need no original assets. For a short run
in a Debug build with the native selector:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^SMAC (TILE-E-|energy )' --quiet --prefix build/test-data-energy
```

Implementation validation reran the complete pinned original energy routine
and its resource/facility/technology helpers in Unicorn 2.1.4, without substituted
helper returns. The compiled production calculator, built with Clang 15 at
`-O2 -DNDEBUG` and UBSan halt-on-error, matched both outputs for **50 named cases
and all 3,072 combinations**. The comparison supplied the explicitly resolved
mirror counts above; it does not validate an absent port geometry resolver.

Three separately compiled source-copy negative controls made the semantic
fixture checker exit 1: granting adjacent points to a mirror without a collector
(one failure), adding an ordinary 0..99 clamp (three failures), and replacing
nonzero event checks with positive-only checks (one failure). The same copies
disagreed with the full original comparison in 1, 291 and 1 cases respectively.
Production sources remained unchanged during these controls.

Linux Debug CTest passed 8/8. All 57 focused energy tests also passed with
ASan, UBSan halt-on-error and leak detection enabled. Explicit analysis of the
production translation unit passed with both Debug and Release compilation
databases. Fifteen additional optimized `-DNDEBUG`/UBSan probes confirmed
release-visible invalid-input and overflow rejection, without sanitizer errors.
This is component evidence; native Windows and macOS integration are separate
checks.

Remaining integration work includes rules ingestion for these columns, actual
technology/facility/social/capability producers, mirror resolution, event
activation/refresh timing and an explicit live adapter. Worker assignment,
energy allocation/inefficiency/commerce and terrain legality are separate
systems. Full-game observations, preview modes, ownership transitions and
SMAX comparison remain unverified.
