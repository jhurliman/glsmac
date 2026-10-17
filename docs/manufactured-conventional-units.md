# Manufactured conventional units

This runtime bridge consumes the actual selected design, producing Base and
registered presentation definition. It supports ordinary Infantry Combat and
Terraformer packages, plus the existing selected Spartan Rover admission. It
does not select production targets, debit minerals, advance queues or implement
Former work orders. The current conventional runtime admits idle/no-order state
only; this change does not add an order codec or silently activate terraforming.

`UnitManager::PrepareManufacturedConventionalUnit` validates the registered
same-owner home, available global slot below 2,048, ordinary no-project campaign,
and local untrained HQ-only facility context. It supplies stored morale1,
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
`production-posting-and-unit-creation.md`, `movement-edge-cost.md` and
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
No full platform or full-suite run is claimed.
