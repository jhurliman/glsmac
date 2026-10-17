# Manufactured conventional units

This runtime bridge consumes the actual selected design, producing Base and
registered presentation definition. It supports ordinary Infantry Combat and
Terraformer packages, plus the existing selected Spartan Rover admission. It
does not select production targets, debit minerals, advance queues or implement
Former work orders. The current conventional runtime admits idle/no-order state
only; this change does not add an order codec or silently activate terraforming.

`UnitManager::PrepareManufacturedConventionalUnit` validates the registered
same-owner home, available global slot below 2,048, ordinary no-project campaign,
and exact local facility roles. Ordinary land training adds two for Command
Center and two for Bioenhancement Center, then halves their combined bonus
when current MORALE is below -1. It adds the ordinary baseline 1 afterward.
Perimeter Defense, Network Node and Recycling Tanks do not add training or
block manufacturing. The admitted designs have no abilities and are already
prototyped; project training remains excluded by the saved campaign. It supplies
damage0, spent0 and a clear failure marker; effective morale and full allowance
are queried from the actual frozen design, retained social state and saved home
status. Existing generic explicit-state creation remains separate.

The move-only prepared value owns an unplaced Unit and copied manager, tile,
update and sight state. Abandonment changes no identity, queues, history or map.
`Commit()` performs only nonthrowing authority publication, including the ID
allocator; `Notify()` invokes the spawn hook once, after the caller has committed
its Base production state. Both operations must stay in the same owning
accumulation with no intervening callback, yield or world mutation. Notification
failure is postcommit and does not roll back a completed production transaction.

Friendly canonical stacks permit creation and departure without replacing the
old Unit. Stable IDs remain the port authority; map traversal order does not
claim original stack-link order. Formers are ordinary support consumers, not an
exempt noncombat package. Empty-land movement uses the real design's edge cost
and full allowance; the plan9 fungus pricing rule is present in the pure helper,
while live Former fungus encounters and foreign ZOC remain explicitly outside
this bridge. Homed restore resolves after Base publication and replays no spawn.

`RT_PORT_FORMER_MARKER` is an explicit amber F placeholder with owned texture
lifetime. It is neither Scout imagery nor an original-game render approximation.
Its separate version1 descriptor appends a render tag; old readers reject that
unknown tag. Existing Unit state, manager and World layouts are unchanged. The
faithful Former CVR recipe remains a separate presentation dependency.

The original behavioral inputs are the human-maintained
`docs/behavior/ordinary-unit-production-loop.md`,
`production-posting-and-unit-creation.md`, `ordinary-production-training-inputs.md`, `movement-edge-cost.md` and
`unit-support-and-convoys.md` in the research workspace. The five production-loop
sequences include actual factory, move and later upkeep returns. The tests here
exercise the bounded runtime stages in a synthetic canonical world; they do not
claim to execute the complete Base posting/upkeep sequence or rerun the original.

The private raw-startup sight path also stages membership and sight while keeping
the Unit pending until first-turn seal. It depends on the exact claimed-Start
structural projection prerequisite `f1b73232`; the generic API cannot request that
readiness exception. Snapshot-helper prerequisite `006ba7eb` is also included.

Validation: cached Linux native build; 29 affected groups passed with memory
tracking (2.069s), including five new groups for abandonment/delayed hooks,
support/stack departure/reset, deferred home restore, owned marker lifetime and
package edge pricing. Nine changed production translation units passed LLVM15
analysis (0 failures). Existing eight Early groups passed; the first-Base
integration owns the additional knowledge-present claimed-Start composition.
The same 29 groups passed the focused ASan/UBSan/LSan run with memory tracking
(5.175s), leak detection and halt-on-error enabled. This includes deferred
home restore, abandoned prepared ownership and retained marker texture lifetime.
No full platform or full-suite run is claimed.

The training follow-up passed all six manufactured-unit groups in the cached
Linux build with memory tracking. Its new cases distinguish current from retained
MORALE and cover neutral starting facilities, Command Center, and the combined
Command Center/Bioenhancement bonus. The updated UnitManager also passed targeted
LLVM15 analysis; custom designs without a completed prototype remain outside
this manufacturing path.
