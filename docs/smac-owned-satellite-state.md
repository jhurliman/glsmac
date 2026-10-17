# Owned satellite assets and orbital intake

`SmacSimulationState::satellites` owns nutrient/mineral/energy satellite
counts by explicit simulation faction ID. Every admitted roster participant
must have exactly one record; absent or extra owners reject rather than
supplying zero. `GetSatelliteCounts` returns a detached value. Signed32 counts
are retained exactly, including diagnostic negative values.

`Game::PrepareNewSimulation` explicitly constructs zero satellite assets for
each participant. The later `RE-SMAC-ORBIT-CALLER-001` receipt confirms zeroing
at turn zero in four poisoned calls to the actual faction-initialization
fragment, preserving other factions and skipping the reset at turn one. Twelve
complete forced resource calls also confirm owner-specific prefix replacement.
Those bounded original calls do not establish frontend or load timing.
Loading never invokes that
initialization: Simulation V8 appends the bounded owner/count records and
restores every value. V7 and malformed/truncated/duplicate/mismatched records
reject. The existing world prefix decodes the strict simulation version before
Map allocation; outer World21 and settings formats are unchanged.

`SmacSatelliteCounts::Intake` implements the measured independent N/M/E prefix:
retain full counts with Aerospace Complex or an owned Space Elevator;
otherwise compute signed `(count + 1) / 2`, then cap at actual population.
It replaces the prefix rather than adding retained intake, and does not consume
assets. Population is admitted at 0..127. The unmeasured `INT32_MAX + 1` branch
rejects when halving is required, while full-output queries do not evaluate it.
Negative counts follow the captured diagnostic arithmetic, not a gameplay
initialization policy.

`Game::GetCanonicalOrbitalIntake(const Base&)` reads the registered Base's
actual population, bound owner counters and registered positional Aerospace
role29. The current live adapter requires the saved no-project campaign;
that explicit admission establishes no Space Elevator. It also rejects project,
orbital and unbound facility records instead of inferring semantics from names.
The pure rule accepts an explicitly resolved owned-Elevator input for future
project consumers. The query does not alter population, queues, counts or RNG.

The literal fixture copies all sixteen named `RE-SMAC-ORBIT-INTAKE-001`
prefixes from `docs/behavior/orbital-resource-intake-cases.json`, SHA-256
`fc9445afaa71bbfe9b91b9ed74fb01d2e64efe0c16f88bf8eb7a425a973eb6b7`.
Local/remote owned Elevator cases project to the explicit ownership Boolean;
foreign/absent cases remain false. Center-after-prefix values and the private
384-case grid are not claimed as newly tested. The source receipt is
`docs/re-smac-orbital-intake.md` in the human-owned research checkout.

Tests cover these literals, branch-sensitive arithmetic bounds, non-dense
owner IDs, all signed32 restore bits, independent wire framing, missing/extra
owners, malformed late fields and rejection of V7. The actual Base test uses
the new-world producer, nonzero counters, a misleadingly named Aerospace
definition and a deferred world restore into a destination without current
simulation. Existing invalid-Client prefix tests retain their current-version
records so they continue reaching their intended rejection boundaries.

Completion/loss handlers, Defense Pods, project ownership and automatic
allocation scheduling remain separate consumers. No script/default activation
or full economy pipeline is added here.

Validation: 33 selected native groups pass with allocation tracking (3.129s),
including the four new orbital/state groups and existing actual Start, Client
prefix, failure/reset and restore cases. Three changed production TUs pass
LLVM15 analysis (4.13s). The first focused run caught an older test fixture
remapping simulation IDs without remapping the newly required asset records;
that fixture now supplies nonzero counters for the saved IDs and the failed
log is retained. One cached normal build plus that fixture-only incremental
rebuild was used; no full suite, sanitizer or platform matrix is claimed.
