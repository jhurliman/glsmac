# Owned first-command social continuation

SMAC worlds now carry social state alongside the admitted simulation roster,
technology ownership and startup designs. The actual new-world producer runs
inside the existing claimed Start operation. The actual first `AdvanceTurn(1)`
stages the refresh before changing the turn ID, sending the frontend turn
notification or invoking unit, base and game upkeep hooks. This supplies an
authoritative first-command context for later unit consumers; it does not
create conventional units or implement general social-engineering changes.

## Source and admitted domain

The source is the frozen faction profile, including its four selected neutral
model-zero rows. `SmacSocialState` reads the existing ordered `Records()`
projection. SOCIAL records contribute to the separate eleven-axis intrinsic
vector. After the entire sum, each admitted IMMUNITY record clamps its named
final rating to `[0,999]`; intrinsic remains unchanged. Duplicate immunity
records retain their individual cost in the shared eight-record budget. Their
repeated final clamps have no additional rating effect. Records beyond the
budget acquire no social effect.

This is the neutral startup subset established by `RE-SMAC-START-FACTION-001`,
the retained-morale and first-command handoffs, `social-axis-immunity.md`, and
`base-square-parsed-economy.md`. Selected and pending model indices must all be
zero. The frozen profile rejects nonneutral model-zero modifiers and direct
MORALE entries. Projects, Nexus effects, later nonzero model selection, base
Golden Age adjustments, and general later-turn social refresh are outside
this implementation. Signed intrinsic accumulation is checked before each
signed32 store; the checked arithmetic policy does not claim original overflow
compatibility.

The direct-MORALE absence is validated, not a value inferred from a faction
name: the complete existing profile decoder rejects every direct entry,
including entries after the shared-record budget. SOCIAL/MORALE and
IMMUNITY/MORALE remain separate supported records. This change does not add a
direct-MORALE scalar or flag to runtime state.

## Early state, transition and restoration

Each owner has an explicit simulation ID and frozen source-profile ID, four
selected and pending model indices, eleven current ratings, optional intrinsic
and retained eleven-value snapshots, and an Early or Refreshed phase. New-world
admission keeps current ratings zero and both snapshots absent. Absence is
observable and must not be interpreted as retained MORALE zero. The original
early reset does not initialize those snapshots, so the value-level
`ResetForStartup` preserves supplied snapshots while resetting only the model
indices, current ratings and phase. World retirement still clears the whole
simulation through the existing reset path.

New-world admission preflights intrinsic arithmetic without exposing it to
initial founding. In the authenticated default SMAC Start, each owner's late
intrinsic producer runs after its actual Base and Scout/Rover factories and
before global compensation. Current ratings stay zero, retained state stays
unavailable, and the owner stays Early until upkeep. Compensation Pods observe
the completed late intrinsic producer.

The owner-specific first refresh validates all bound state, consumes that owner's
owned intrinsic vector and publishes only its current/retained ratings. Legacy
port snapshots without intrinsic authority derive it from their frozen profile.
The existing all-human controller stages these transitions for all owners before
publishing the first turn. Native per-faction scheduling must instead call the
owner transition at each measured upkeep boundary; that controller is unfinished.
Malformed roster/profile/context leaves social state, turn, frontend requests,
RNG and upkeep callbacks unchanged at that boundary. A later callback failure
is governed by the existing game lifecycle; this is not rollback of completed
upkeep effects.

Early is valid only at world turn zero; Refreshed is valid only after turn zero.
An Early zero snapshot is restored inertly, including supplied snapshots, and
refreshes on the next genuine `AdvanceTurn(1)`. A Refreshed snapshot preserves
all signed current/intrinsic/retained values exactly, even when they differ from
the original source-derived values. Decode never resets or replays refresh.
Subsequent ordinary turns leave this first-command state unchanged.

The outer simulation requires exact owner cardinality and matching admitted
simulation/source identities across roster, technology, designs and social
state. Existing canonical participant, session and roster-epoch checks remain
in force. Saved simulation IDs are preserved rather than reallocated. The
script method `game.get_social_state(player)` returns fresh scalar/array values;
unknown snapshots are `undefined`. Mutating the returned object cannot mutate
the stored state. The method uses the same current-participant validation as
the existing technology/design queries.

## Wire contract

Social V1 is an owned framed value with marker `SMSO`, version, bounded owner
count, identities, phase, model arrays, current ratings and explicit optional
snapshot presences. Integers are range-checked before narrowing. Counts are
limited to seven; source IDs are bounded before string allocation; the entire
nested payload has an 8 KiB port budget. Unknown versions/phases, unsupported
models, duplicate owners, incomplete Refreshed snapshots, truncation and trailing
data reject before publishing a result.

Simulation V3 appends this framed social payload after technology and designs,
within the existing 64 KiB simulation budget. World V13 requires that complete
continuation and rejects V1 through V12. GlobalSettings remains V9 and the
frozen faction profile remains V2; the previously frozen raw movement profile
is retained unchanged. SMAX still requires absence of SMAC simulation state.
The real download reader validates the saved phase against the saved turn and
binds the complete simulation before allocating a Map. This guarantees prefix
rejection, not transactionality of arbitrary later world-body reconstruction.

## Original-output comparison

`tools/tests/smac_social_state_probe.cpp` is an opt-in production reader and
codec driver. It parses supplied alpha/faction metadata with the existing
production loaders and calls the same owned transition and refresh helpers
used by the game. `tools/reference/compare_smac_social_state.py` verifies the
input and report fingerprints, reconstructs explicit inputs, and compares to
literal guest-memory output arrays. It does not import the original checker or
use fixture expectations as a replacement oracle.

The Release/UBSan comparison matches 613 captured projections and 613 owned
codec roundtrips: 16 initial faction compositions, 454 immunity cases and 143
duplicate-immunity/neutral-economy cases. It compares intrinsic, current and
retained arrays plus admitted-record counts. The 16 initial compositions also
compare early model/current fields. The explicit unknown-state policy and
codec roundtrip are production assertions, not original memory observations.
Where the evidence injects a resolved intrinsic endpoint, that value remains
an explicit helper input; those cases do not claim that the source parser
produced that endpoint.

Fifty contexts are excluded and listed in the generated comparison report:
twelve isolated early-reset cases, thirty-seven nonzero model selections and
one direct-MORALE case. The original third refreshed vector, unit queries,
base yields and complete original startup/upkeep execution are not compared.
Value-level reset preservation has separate native tests.

Pinned report SHA256 values:

- START-FACTION: `068bd93903781b0bdf4a06d25ad6f5b5f39cbac0e48bf66b79f01e06998648af`.
- Immunity actual vectors: `113fc86c3715ff139a245644315ebcd54a6f92cb4ec50e5b9585f07f0b79d5d7`.
- Duplicate-immunity actual vectors: `fedd520771ae72b0ed9c71db50ba0a823d29f19146716c69288dff1e6ce2a7be`.

Fixture and executable/alpha fingerprints are also checked by the comparator.
Proprietary binaries, source rules and full reports remain outside Git.

## Native validation boundary

The eight pure native groups cover all seven stock profiles, explicit sparse
IDs, source lifetime, signed endpoints, every truncated payload prefix,
duplicate/shared-slot order, malformed fields, staged failure and exact saved
values. Existing actual startup/simulation fixtures now read and mutate copied
social query values in the production script; real unit/base/game turn hooks
observe the refreshed context. The unit in this fixture is a synthetic test
definition, not a conventional Scout substitute.

The complete snapshot writer/body/InitComplete fixtures preserve modified
saved values and sparse IDs, test Early-zero continuation, and exercise actual
canonical reconnect without rederivation. Actual immediate and queued Client
final-download dispatch rejects malformed social payloads and phase/turn
mismatches before Map allocation, independently of later missing Map data.
Existing Pending/Running cancellation, reset, RNG/turn and SMAX controls remain
registered. These fixtures use synthetic map/admission setup and direct actual
Client packet dispatch; they do not claim rendered, socket-driven or full
interactive startup coverage.

Four copied-source controls fail their intended assertions: omit the live
first-refresh call, omit saved phase/turn validation, omit the immunity clamp,
or replay refresh during decode. The intended checkout and ordinary binaries
are never replaced for these controls.

Source parent: `72ebf9ff5fbc3160db1c0549debde57fb8d4f9ff` (the reviewed frozen-Q
handoff). Dedicated Linux/WSL Clang 15 receipts are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/social-final-*`; the original
comparison is `social-state-original-comparison.json` and copied-source
controls are in `social-runtime-controls/`.

Final combined source receipts:

- Full normal CTest: 8/8, 74.95 seconds.
- Full ASan/UBSan/LSan CTest: 8/8, 153.58 seconds, leak detection enabled.
- Focused social/simulation/startup/world groups with `--memorydebug`: 69/69
  in both normal and sanitizer builds, including the final V13 rejection cases.
- Release/UBSan original-output comparison: 613/613 projections and owned
  roundtrips, with the 50 exclusions described above.
- Four copied-production negative controls: each exits 1 at the intended
  assertion. These ran before the Q handoff; their social/transition production
  logic is unchanged in the final source.
- Full shared-header LLVM 15 correctness analysis: 330 production translation
  units, zero failures, 251.87 seconds with two jobs.
