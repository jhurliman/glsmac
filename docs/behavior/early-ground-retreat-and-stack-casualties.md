# Early ground movement, retreat and stack casualties

2026-11-11. `RE-SMAC-EARLY-GROUND-LIFECYCLE-001`.
This is a compact source-routing supplement to
[early ground strength/selection](early-ground-combat-causal.md), using retained
movement and retreat controls. No new capture matrix is run. Stack-casualty
rules below are **static causal source evidence**, not newly replayed stacked
battles. The pinned executable and all earlier packets remain unchanged.

## Mind Worm movement

Reuse complete [edge cost](movement-edge-cost.md), entry`5aaad0`. For owner0,
stock ground Mind Worm design8, Q3, ordinary terrain switch0:

| Destination /connection | Cost |
| --- | ---: |
| Flat or rolling land | 3 |
| Rocky land | 6 |
| Forest land | 6 |
| Controlled rocky+forest | 9 |
| Land fungus, including rocky/forest combinations | 1 |
| Connected ordinary roads, tubes or diagonal river | Ordinary terrain cost |

Owner0 fails the positive-owner infrastructure-discount gates, so roads are
not cost1 and tubes are not free. Fungus reaches the earlier owner0/design8
return1 before rocky/forest charges. This is a raw edge query, not a rule to
charge one full movement point for fungus or reuse human conventional fungus9.
The normal movement caller at`5aee51` explicitly supplies terrain switch0 to
`5aaad0` at`5aee60`; do not suppress rocky/forest because Worm basic speed is1.
The existing cost grid includes owner0 infrastructure/fungus diagnostics.
Admission/spending, knowledge/ZOC and native upkeep remain their separate
retained contracts; exhausted units do not get fresh movement from a cheap edge.

## Rover retreat: already validated full native returns

Use [native offense and retreat](native-offense-and-retreat.md) and its22
literal fixtures:44 normal wrapper returns,252 rounds,539 continuous CRT
draws,13 retreats with both Units alive. This also **validates native offense**
and its pre-turn15 attacker-third width; it was outside the narrower43-case
human-offense packet, but is not an untested hypothesis project-wide.

For ordinary no-ability Fission ground Infantry attacking a singleton Rover
outside a Base/fortification, native source`51cf7e`..`51d127` can arm retreat.
The necessary speed condition is **defender full damage-adjusted allowance
with elite bonus suppressed > attacker allowance with elite suppressed**.
It does not subtract either spent byte. Thus healthy Rover6 versus Infantry3
can retreat; Rover6 versus Rover6 cannot. The retained native attacker/Worm3
versus Rover6 controls prove the shared path's actual escape behavior.

Additional native admission gates on this ordinary path include: normal mode,
nonzero defender weapon, neither chassis air, attacker lacks ability`0x200`,
defender Unit flags lack`0x40`, defender byte17 is not2, no defending Base,
defender tile flags do not intersect`0x40800`, and exactly one Unit in the
defending stack. The byte/flag gates are recorded literally rather than
assigning unverified names to them. A stacked Rover does not meet this singleton
gate. These are not remaining-movement checks: the retained spent6 Rover
still retreats and retains spent6.

Arm `ceil(initialDefenderRemainingHealth/2)`. After each nonlethal hit on the
defender, subtract actual hit damage from that countdown (`51d4ed`..`51d571`).
At threshold, scan for escape once; failure clears the countdown rather than
retrying after every later hit. Do not delete the defender when this succeeds.
Keep retreat as a distinct transaction outcome: the attacker may promote and
remain on its source while the defender survives at a different square.

The escape scan`51d589`..`51d788` uses eight native clockwise offsets, map
wrapping/bounds, candidate gate`5e0770`, domain/sea matching, gate`5a9570`,
fungus/project/predefined-Psi checks and occupancy/relationship checks. Its
circular separation from the direction toward the attacker must exceed2;
choose only a strict score improvement, preserving scan-order ties. Do not
replace this with an arbitrary legal AI neighbor. The retained southeast-attack
control escapes northwest, then west, then north as those routes are blocked;
all three blocked means combat continues. Ordinary Rover cannot retreat into
the supplied land-fungus escape square without the applicable exception.
Successful retreat relocates through the native Unit movement helper and
preserves spent movement.

For the retained turn15 seed19 example, the human Rover retreats at damage5;
the attacking native has damage8 at combat exit, promotes its stored byte1→2,
then separately repairs one damage on its own origin fungus in movement
aftermath. Clearing only origin fungus preserves rounds/retreat/promotion but
leaves native damage8. Do not turn that later native repair into promotion healing.

## Ordinary ground loser-stack casualty stages

These source stages follow an attacker victory and selected defender defeat,
under ordinary ground/no-Probe/no-Artifact/no-cargo first50 conditions. They
are not a generic “delete every Unit at destination” rule. Core`51e745`..`51ed1c`
dispatches casualty handler`519db0`, which finally calls actual deletion
`5d89f0` and can itself perform special capture/ownership handling.

Define `nonExemptCount` by actual stack count minus counts of plans
**12,8,11,10** (Artifact, Colony, Probe, Supply). Former plan9 is **not** in
that list. Native stack-count mode2 tests plan, as confirmed by jump-table
case`5d0cf9`; do not implement this using a generic civilian Boolean.

1. **Guarded single-casualty route.** A defending Base or tile bit`0x800`
   enters the protected routing at`51e832`. A land stack containing plan8
   also enters that route (`51e7e4`..`51e830`). Subject to the ordinary
   no-cargo/no-Probe/no-Artifact conditions above, if`nonExemptCount>1`,
   call`519db0` on the selected defeated Unit and jump to`51ed1c`: no open-stack
   damage loop. Thus an ordinary Base with multiple military defenders does
   protect the other defenders from this collateral stage. Do not generalize
   that statement into an unconditional early return for every Base stack.
2. **Damage pass**, reached at`51e92d` otherwise. Follow the loser stack's
   previous links to its head, then next links. The selected defeated Unit is
   brought to its maximum damage. Non-selected plans12/8/11/10 skip damage.
   Other eligible members receive **3×reactor damage**, not half their health:
   for a negative attacker weapon use the affected member's reactor; otherwise
   use attacker reactor (`51ea8f`..`51eadf`). Reactor1 therefore adds3.
   Actual byte arithmetic and flag`Unit.flags|=4` execute; collateral can be lethal.
3. **Removal pass**, `51eb0c`..`51ebf9`. Rewalk the stack. A member with no
   remaining health goes through`519db0`. Source also routes owner0 members
   through this handler even with positive remaining health. Each deletion
   compacts roster indices: repair the saved next index and attacker reference
   before continuing. Do not iterate stale IDs or replace this with a bulk erase.
4. **Residual exempt-only cleanup**, `51ec28`..`51ed14`. Re-read tile membership
   after removals and recompute`nonExemptCount`. If it is zero, process all
   remaining members through`519db0`, again repairing indices after each
   removal. This is why exempt civilians can still be removed after the last
   defending non-exempt Unit dies, even though they skipped the damage pass.
   A surviving Former9 prevents this particular exempt-only cleanup.

Examples implied by these source stages: plain two-Scout stack loses the
selected defender and the other takes3 damage; a Base with two Scouts takes
the guarded selected-only route; an exempt Colony can skip damage but still
reach residual cleanup if no non-exempt defender remains. A healthy remaining
Former does not count as exempt. These are static implications, not fresh
full-stack battle fixtures. Preserve handler-level Artifact/cargo/capture
exceptions rather than treating every handler call as an unconditional delete.

The winner's existing source-square retention and spending/promotion order
remain authoritative. A retreating defender must bypass lethal casualty
processing. Changes to stack composition require resolving the selected
defender and current tile links again after every native-style compaction.

Private source slices and references are retained in
`tmp/reference/independent-early-ground-lifecycle-evidence-20261111/`.
