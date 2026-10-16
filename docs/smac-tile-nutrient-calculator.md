# Pure SMAC ordinary tile nutrient calculator

`game::backend::resource::smac::CalculateTileNutrients` implements the
non-center nutrient calculation described by RE-SMAC-TILE-N-001. It returns
the signed nutrient yield and that tile's technology-capped excess separately.
It neither changes a global excess counter nor connects to live tile callbacks.
Base centers continue to have a separate calculator.

The contract requires a valid evaluating base/faction, a non-center tile, and
normal fungus handling. Every input is explicit, with no default constructors
or neutral assumptions for unmodeled systems:

- Eight signed nutrient rule values: Ocean, Bonus, Forest, Improved Land,
  Improved Sea, Monolith, Borehole, and the mine nutrient effect. Loading these
  values from the selected rules is separate work.
- Altitude band, moisture and rockiness; resolved resource type and eligible
  nutrient landmark point; forest, fungus, borehole and monolith presence.
- Farm/kelp, mine/platform, Soil Enricher and Condenser presence.
- Actual Tree Farm and Hybrid Forest presence at the evaluating base,
  ownership of the configured nutrient-limit technology, the already-built
  fungal nutrient capability, effective PLANET, and active Bumper Crop/Famine.

Special-tile precedence is borehole, monolith, qualifying fungus, sea, land
forest, then other land. The custom nonzero borehole rule adds the Bonus rule
unconditionally. Ordinary branches apply Soil Enricher and Condenser as two
separately rounded signed halves, then the technology cap. Condenser exempts
the cap; Soil Enricher alone does not. Events run last on every branch. Signed
custom-rule yields remain signed, including a mine effect below zero and
Famine applied to a negative nonzero result.

The admitted domain is explicit:

- All eight altitude bands are accepted. Moisture is Arid/Moist/Rainy and
  rockiness is Flat/Rolling/Rocky; raw reserved encodings are not this API.
  Invalid enum values throw `std::invalid_argument` in release builds.
- Resource type is the effective result, not an original or port tile flag.
  A resource below shelf is rejected because the resolver cannot produce one.
  The semantic fixture's explicit nutrient markers are converted to None in
  deep ocean/trenches and Nutrients on shelf/land. A native integration case
  also calls the real resolver before the calculator across all eight bands.
- Fungal capability accepts 0 through `INT32_MAX`. RE-SMAC-FUNGAL-001 establishes
  that its builder floors the completed capability at zero, without a 99 cap.
  A negative capability is an invalid constructed input. Capability building
  and refresh timing remain outside this calculator.
- Rule values and PLANET accept the entire signed int32 range. Ordered
  arithmetic is evaluated in int64, and each evaluated result must fit int32.
  `std::overflow_error` rejects unsupported original overflow before any later
  clamp or cancellation. Valid negative rules are not rejected or silently
  clamped. These rejection rules are port input policy, not claims about
  original malformed-state behavior.

The code does not implement climate or landmark eligibility, terraforming
legality, capability construction, event scheduling, preview modes, mineral or
energy yields, a live map adapter, or SMAX equivalence.

## Evidence and validation

The test-owned semantic fixture comes from the research handoff
`docs/behavior/ordinary-tile-nutrients-cases.json`, with 38 named cases.
Its SHA-256 is
`191d4271b8df0b623a6ce9bd391cd409cbe948d50c8e531cc10bd4a72edb91b1`.
The pinned SMAC `terran.exe` SHA-256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The implementation and fixture contain no original code, executable layout,
instruction bytes or assets. Human-owned research files are not modified by
this change.

Local validation used Clang 15 on WSL Ubuntu. A separately compiled production
calculator with UBSan matched the complete original nutrient routine and its
original resource/facility/technology helpers for **5,110 / 5,110** synthetic
cases in Unicorn 2.1.4, with no helper return-value hooks: 38 named cases,
4,608 research grid combinations, 240 signed custom-rule combinations, and
224 fungal capability/PLANET/event boundary combinations. The private harness
was loaded without executing its research-file writers. This is routine-level
evidence, not a full-game observation or SMAX comparison.

Separate negative copies combined the multipliers into a single 2.25 factor
(857 mismatches) and incorrectly made Soil Enricher the cap exemption
(90 mismatches). The real production source was unchanged during these runs.
An additional UBSan probe passed 22 endpoint, invalid-input and evaluated
overflow cases without sanitizer diagnostics. Native tests cover the named
cases, branch precedence, facilities, all altitude bands, signed small-value
rounding, event ordering, cap excess and input/overflow rejection. The full
CTest suite passes all seven entries (36.04 seconds locally), focused `clang-tidy-15` is clean, and
independent code review found no actionable issue.

The ignored local `build/tile-nutrients/original-comparison.json` records source
and fixture hashes, counts and negative-control counterexamples. Validation
logs are under `build/tile-nutrients/validation/`; proprietary emulator inputs
remain outside the committed test suite.
