# Regional military counters in live owner upkeep

This records the V18 counter slice. The subsequent
[V19 unit-phase flag integration](regional-unit-phase-flags.md) adds the separate
scoped contribution rows; it does not yet implement the final regional flags.

The owner-summary stage now retains regional unit counts and attack contributions
for every positive faction slot and all128 regions. These are outputs of the
post-Base summary, alongside the existing regional Base counts and component
capabilities. Research must consume the preceding saved totals during Base
processing, rather than count today's units inside a query.

The [original conventional-unit scan](behavior/regional-conventional-unit-scan.md)
contains130 complete native controls. The port's compiled projection uses literal
native resolved component/morale inputs and checks all33,280 unit/attack entries.
It does not claim the separate flag arrays, invalid-placement cleanup, diagnostic
workshop designs or transport commands are implemented by the port. The declared
mid-phase wrap canary uses the watched native word-write before values65535;
the owner-summary entry arrays precede that injection.

## Runtime integration

Current Scout/Rover/Former/Colony identities are resolved through the real bound
catalog and conventional-unit resolver. The pass walks saved Unit record order
in reverse and validates it against the current registry. It prepares one owner's
replacement arrays, preserves all peers, and publishes only after all queries
succeed. Capability refresh is prepared before the unit scan and committed with
these counters; a late invalid input cannot partially update capabilities.

Unit count requires plan<=7, land domain and a land tile. Attack is a separate
accumulator: each unit contributes trunc((effectiveMode0Morale+6)*attack/8), with
per-unit truncation and native ushort wrap. No HP/damage scaling or minimum1 is
applied. Civilian modules contribute0 because their actual attack is0; there is
no civilian-plan filter in the attack accumulator. A ground unit at a valid sea
Base can therefore add attack without adding a land unit count.

The pure contribution routine also implements the measured order2 suppression
(A<=D) and special-plan strength skip. The current live combat state admits no
Sentry/order2 or special-plan5, so the actual caller supplies those explicit
ordinary values. Sea/air/PSI designs, transport and native invalid-placement
removal remain future live callers. An unbased ground unit on water is rejected
until its cleanup/transport producer exists, rather than silently adding a false
contribution. Regional flags, final regional strategy, the separate total-strength
aggregate and the complete Blind Research dispatcher remain unfinished.

## Saved lifetime and validation

Simulation V18 appends optional military history as bounded unsigned16 data.
Fresh world admission explicitly initializes all seven positive slots. V11..V17
restore keeps absence; neither restore nor a query reconstructs missing history.
Creation, movement and removal do not eagerly recount the retained totals.
Nonzero faction initialization preserves them; the next actual owner summary
performs its own recount. Saved unoccupied slots and polar/special regions are
preserved without an active-faction filter.

Focused checks cover the130 literal native projections under address and undefined
behavior sanitizers, all word counters in the codec, malformed/truncated frames
and older-version absence. Live tests create two Scouts, a Former and a Colony
Pod, distinguish effective morale and HP damage, preserve peer canaries, exercise
failed-preparation rollback, remove a real Unit, and restore through actual Start
before the next recount. Existing simulation/migration tests and affected real
startup/upkeep/capture/restart groups provide integration coverage. C++ correctness
analysis covers five relevant production files, reusing unchanged results after
the final two-file update. No new platform matrix is required by these arithmetic
and saved-state changes; normal CI remains a merge gate.
