# Early conventional startup identity

This slice permits an explicit raw conventional unit at the original startup
position: the HQ event may run first, followed by unit creation at turn zero,
then the existing first-turn social refresh. It does not change the default
world script or make default conventional startup playable.

## Admission and authority

`UnitManager::SpawnStartingConventionalUnit`, exposed as
`spawn_starting_conventional_unit({def, owner, tile, render_morale_mode})`, accepts
only the exact currently claimed, non-canceled, master new-world Start on its
owner thread. The game must be processing the running startup event batch at
turn zero; reload admissions, downloads, unclaimed calls, later turns and generic
spawn/restore paths cannot use this admission. The explicit mode remains required.

The producer obtains the actual saved `starting_scout` from the bound simulation
participant, never from a transport slot or faction-name arithmetic. It reuses
the owned design/component and strict definition/render checks. It creates a
real Unit with ordinary UnitID, manager and tile membership, stored morale 1,
damage/spent 0, no failure marker and explicit homeless state. All validation
precedes constructor ID/tile effects. One starting conventional unit per owner
is admitted by this producer. The ordinary `unit_spawn` hook runs once at actual
creation and can inspect the copied raw state and identity.

Early social retained ratings remain absent. Structural resolution returns an
owned context with absent retained morale; derived queries reject that absence.
The Unit additionally has a private transient readiness bit, so changing the
available social context alone cannot make a raw unit commandable. Generic
`spawn_conventional_unit`, updates and restore retain their Refreshed-only
contract. No caller-supplied float stats are introduced.

## First-turn seal and publication

The first-turn transition stages a complete bound simulation copy, performs the
existing social refresh there, and preflights every pending Unit against the
staged context. A late invalid selected design leaves social, turn and all Unit
readiness states unchanged. After all validation/allocation succeeds, social is
moved and all pending Units are sealed without callbacks; the existing scalar
turn assignment follows immediately, before frontend turn publication and upkeep
hooks. Raw fields and UnitIDs are not replaced. Conventional legacy healing and
float refill remain disabled.

`QueryStats` guards presentation, morale text and actionability. Whole-state
updates, selection, direct Unit/manager serialization and world snapshot writing
also reject pending units. `PushUpdates` retains queued spawn/update operations
without emitting them while any raw unit remains. Internal turn-completion
publication waits. The full claimed Start requires every raw unit to be sealed
before releasing its owner checkpoint: a missing advance event is initialization
failure, not an incomplete playable world.

Pending cancellation creates no units. Cancellation while Start is Running does
not tear down its native objects concurrently; a canceled first-turn seal rejects
and the existing failure/reset path retires the world. Actual collection of the
retired manager invalidates retained Unit wrappers. Ready records use unchanged
Unit V2 serialization. Restore supplies staged bound context and constructs Ready
units directly, without replaying startup or sampling gameplay entropy.

## Evidence and limits

The ordering and raw initialization come from the independently owned
`docs/behavior/startup-first-command-boundary.md` and
`docs/behavior/startup-morale-before-command.md`
in the research workspace; those human files are not modified here. Retained
social zero is never inferred from a fresh-image observation. Descriptor facing,
camera and placement remain explicit caller policy. This is not default
activation, complete original HQ/upkeep fidelity, or original save-file policy.

The native fixture drives actual claimed queued/immediate Start, actual event
validation/apply, an HQ fixture event, three actual GSE raw spawn calls, the real
first advance-turn script and actual base/turn hooks. Synthetic frozen profiles
and map/presentation inputs model Morgan, Sparta and Gaia with explicit Q3;
expected effective morale is 1/2/0 and full allowance 3/6/3. The raw hook tests
all derived/publication guards and temporarily makes social/turn consistent to
prove object readiness is independently required. A valid later-owner selection
change tests all-unit preflight. Failure/reset tests retain wrappers through
actual collection; the Ready world-body restore compares exact Unit records and
the paired gameplay RNG. That restore check does not claim to run the complete
claimed-Start restoration scheduler or compare the still-staged JSF/turn envelope.

No default script, human research file or proprietary asset is changed. The
separate live movement integration must call `RequireConventionalReady()` before
its copied-context query; direct pure queries cannot inspect Unit lifecycle.
This branch precedes that movement slice and does not silently widen its scope.

Validation on source parent `864ad416` plus this slice:

- Full normal CTest: 8/8 in 83.78 seconds.
- Full ASan/UBSan/LSan CTest with leak detection: 8/8 in 161.16 seconds.
- All eight new native lifecycle groups pass with allocation tracking in both
  normal and sanitizer binaries.
- Full LLVM15 correctness analysis: 336 production translation units, zero
  failures in 130.96 seconds; the Windows-only transport TU is excluded.
- Four copied-TU controls each fail the intended native assertion with exit 1:
  omit the claimed-Start check, omit all-unit preflight, ignore object readiness,
  or release Start without requiring a final seal. The intended checkout and
  binaries are not temporarily mutated for these controls.

Initial fixture failures and diagnostics are retained separately: the synthetic
Q and faction expectations were corrected; invalidation now waits for actual
collection; synthetic frontend coordinates are initialized before releasing the
fixture's Map owner; and body restore compares the units/pair it actually
publishes, not the still-staged JSF/turn envelope. No production fallback was
introduced to make those fixtures pass. These are local source checks, not
hosted CI, Windows/Mac runs or an interactive default-world fidelity claim.
