# Conventional empty-ground movement

The real `move_unit` event can move an explicitly created conventional Scout in
a bounded local single-player campaign. This is an integer-authoritative backend
path with a synchronous presentation update. It does not activate conventional
starting units in the default world script.

This note records the original globally base-free checkpoint. The subsequent
[friendly HQ extension](conventional-friendly-hq-movement.md) adds one narrowly
validated exception; its scope and validation are documented separately.
The later [destination-fungus correction](conventional-fungus-admission.md)
supersedes this checkpoint's base-free fungus admission: destination fungus now
requires an admitted endpoint HQ. Pure edge/lottery results remain unchanged.

## Admitted producer inputs

The existing canonical Unit, Tile, accepted simulation participant, frozen
component catalog, frozen movement quantum and retained social state supply the
request. Preparation checks the current session/roster binding and freshly queries
mode-1 allowance. The local controller must be the sole accepted Single player;
connections, multiplayer/scenario modes and nonempty network roles reject.

`SmacSimulationState::MovementCapability` is an explicit saved campaign contract:

- `Unavailable` (0) permits storing the world but rejects this movement route.
- `NoProjectsOrdinaryTerrain` (1) is set by the controlled new-world producer,
  which does not activate project effects. This is a GLSMAC supported-scenario
  policy, not an inference from absent project data or an original startup claim.

A future project consumer must invalidate or replace that capability before
activation. The current implementation additionally rejects every registered
Base, including remote bases with no facilities; this is an explicit empty-world
scope, not a proof of project absence. Both endpoint tiles must also be base-free.
Consequently an ordinary startup HQ blocks this slice, including movement elsewhere
in that world. The separately
measured friendly-HQ route is a follow-up, not an implemented exception here.

The Unit must be registered, living, conventional and compatible with its existing
explicit renderer/design admission. No home base, combat, stack or general order
semantics are added. The actual source membership must match; destination must be
empty, adjacent and unlocked, with no active animation. Foreign native owners in
an endpoint or either neighborhood reject rather than guessing ZOC or combat.

Tile projection is explicit:

- Both tiles must satisfy the port's `!is_water_tile` classification. They map to
  representative original altitude band 3; bands 3..7 have equal cost in the
  admitted edge kernel. Port meter heights are not quantized into original bands.
- Port rockiness Flat/Rolling/Rocky (1/2/3) maps to original 0/1/2. NONE rejects.
- Only River and Xenofungus features, and Road, Mag Tube and Forest terraforming,
  are admitted. Other interaction/landmark/improvement bits reject.
- Actual doubled coordinates, horizontal wrap and canonical Map neighbors are
  used. Polar neighbors alias their source; the same-tile guard rejects them.
- PLANET comes from the owner's retained axis 7, not the current/social preview.

The existing `SmacGroundEdgeCost` calculator receives these fields and the resolved
owned design; no second pricing implementation is introduced. It receives
`owns_xenoempathy=false` only under the explicit capability. The ordinary
`suppress_terrain_charges=false` input is grounded in RE-SMAC-MOVE-ADMISSION-001:
the complete original mode-3 wrapper at 0x5466c0 reaches cost 0x5aaad0 with final
switch 0 at 0x5aee60 (66 requests plus the two-request sequence). This does not
establish every original movement context.

## Admission, authority and presentation

The selected-unit actionability body is shared with the existing KEY-001 kernel.
Exhaustion rejects before cost evaluation, including a zero-cost tube edge. The
port event already supplies a destination; this change does not map physical
keys or claim the complete original keyboard/notification loop.

Validation and application both prepare from current state. Preparation operates
on a copy of the owned shared/CRT pair and invokes the existing empty-ground step
kernel. It validates the resulting complete byte record and bounded arithmetic
before writes. A declined/unsupported command changes neither stream, Unit nor
membership. A valid failed fungus lottery is a real attempt: it exhausts spending,
sets the failure marker and advances the shared stream once while staying at the
source. CRT and the unrelated JSF map generator remain unchanged.

Application prepares the destination map node and the pending-update entry before
committing authority. The same owner call performs no callback or yield between
preparation and commit. Existing integral `std::less`/same-allocator node transfer,
source erasure and trivial Unit/pair assignments then commit membership, byte
record and paired RNG together. No pending raw prepared record escapes the call.
Allocation failure before these writes leaves authority unchanged; this follows
the staging structure, not an injected all-allocation-failure test.

`PushUpdates` subsequently copies the committed state into an FR_UNIT_UPDATE.
The actual frontend request path relocates the Unit and updates exact actionability,
health, morale and tile previews without a legacy movement animation or completion
callback. Presentation allocation/callback failure after authority commit is not
an authority rollback guarantee. This route intentionally rejects network play;
no optimistic rollback or lockstep protocol is added.

The genuine turn path resets conventional spent movement to zero using the
existing checked upkeep kernel, preserving the failed-attempt marker, damage,
morale and home bytes. Legacy script turn handlers remain excluded; no healing
rule is invented.

## Continuation format

Simulation V5 appends one framed integer capability after the existing owned
16-byte gameplay-RNG payload. Construction requires an explicit value. Decode
accepts only 0/1 before enum conversion, validates the complete owned state, and
never chooses a capability from restored map content. World V15 requires that
simulation format; V1..V14 reject. Global settings and Unit/manager V2 formats
are unchanged. The existing 64 KiB runtime envelope budget remains in force.

Actual immediate and queued final-chunk Client fixtures reject missing, negative,
unknown and wide capabilities before Map allocation, callbacks or continuation
publication. Positive body restoration preserves the exact record, tile, pair and
capability, including Unavailable. A subsequent real event produces the same
complete world bytes as the uninterrupted world. This is GLSMAC continuation
reproducibility, not original startup-seed fidelity or original save-file layout.

## Validation boundary

The asset-free backend fixture feeds real canonical participants, native Unit and
Map objects, production `move_unit.gls.js` validation/resolve/apply, actual event
dispatch and the actual writer/body reader. Terrain cases exercise ordinary,
road, tube, rocky+forest, diagonal river, horizontal wrap and Q100/remaining1.
Independent scalar LCG recurrence selects success/failure/retry bins; the tests
assert exact shared-word continuation, unchanged CRT/JSF, membership, bytes and
committed frontend requests. The real AdvanceTurn retry preserves its marker.
Twenty rejection scenarios include endpoints with bases, remote HQ facilities,
remote empty bases, unsupported campaign capabilities and late byte-cost overflow.
Existing native Unit tests remain registered.

The frontend fixture uses the real Scout Unit/TileManager/FR dispatcher and owned
synthetic CVR assets; it proves relocation and absence of legacy animation state,
not a rendered interactive session. Client helpers directly dispatch real final
packets with earlier chunks preloaded; they do not claim complete socket traffic.

Source validation on the parent 8d31ba29 plus this change:

- Full normal CTest after the final registered-Base fence: 8/8, 85.45 seconds.
- Full ASan/UBSan/LSan CTest after that fence, with leak detection: 8/8,
  157.26 seconds.
- Fourteen focused native groups with allocation tracking pass, including the two
  real Client matrices (45 malformed cases each). The same fourteen pass with
  ASan/UBSan/LSan and allocation tracking.
- Ten directly affected production translation units: zero analysis failures in
  17.48 seconds. The full 337-TU LLVM15 scan also passed with zero failures
  (255.47 seconds); the Windows-only network source is excluded by that scan.
  These scans preceded the final one-condition Base fence; its sole changed
  production TU passed another explicit scan with zero failures in 2.40 seconds.
- Four independent copied-TU controls fail the intended assertions: omit authority
  commit, read current instead of retained PLANET, clear the marker during upkeep,
  or infer supported capability while restoring. Intended source and binaries
  remain unchanged by controls. A fifth copied-TU control restores the earlier
  facilities-only fence and fails the twentieth remote-empty-Base rejection.

No proprietary input, human reverse-engineering file or original binary is copied
into this implementation. Reference contracts remain independently owned under
`docs/behavior/` in the research workspace; this note does not extend their claims.
