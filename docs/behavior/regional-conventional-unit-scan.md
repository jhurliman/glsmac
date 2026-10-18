# RE-SMAC-REGIONAL-UNITS-001: conventional unit-phase counters

This closes the positive-human owner scan for the currently admitted Scout
Patrol, Unity Rover, Formers and Colony Pod packages. All130 complete native
summaries return normally; the checker matches49,920 literal regional entries
(128 each for units, attack and flags per case).
[Cases](regional-conventional-unit-scan-cases.json) distinguish real factory
creation from supplied order/position/morale and diagnostic design fields.
Outputs are **after the unit phase, before the new region-strategy pass**.

## Scan and unit-count gate

Walk current unit records in reverse record order. Only the current owner's
units enter its owned-unit path. The earlier owner-summary reset/recount
boundary and peer-cache lifetime remain as previously documented.

After the earlier position/transport and role handling admits the unit:

**Increment regional unit count exactly when plan<=7, chassis domain is land0,
and the current tile is land (elevation band>=3).** Orders, home, damage, chassis
speed and regional strategy do not add another count gate in this checked
positive-human path. The region index is the current tile's stored byte; the
tested0,62,63,64,65,127 all receive counts when their tile is land. Do not use
`region<64` as a replacement for the terrain test.

| Actual predefined package | Plan | Domain | Land-tile regional count |
| --- | ---: | ---: | ---: |
| Scout Patrol | 3 | 0 | +1 |
| Unity Rover | 3 | 0 | +1 |
| Formers | 9 | 0 | 0 |
| Colony Pod | 8 | 0 | 0 |

A valid Base marker means the tile's Base bit and an affiliation nibble0..7;
it is not an owner-equality test. In supplied placement controls, ground units
on water can survive at such a Base or with an actual co-located Transport Foil.
They add **no regional unit count**, but can still add regional attack below.
The unbased/untransported water controls execute actual removal and contribute
neither. Sea-domain units on unbased land likewise take the native invalid
placement cleanup. These are diagnostic placements, not new movement admission
or full transport-command contracts; special protected-state exceptions are
outside the admitted clean-unit slice.

## Attack and morale: a separate gate

This is not restricted to units counted by the previous gate. Surviving ordinary
owned units reach native component attack A, armor D and effective morale M.
Use the design's actual weapon/armor component and current owner; nonnegative
component powers are signed raw bytes. Negative/PSI components use the already
documented [PSI estimates](psi-capability-estimates.md), not physical damage.

Morale is queried with **mode0 and the third argument0**. Reuse
effective morale (reference receipt `scout-creation-morale-and-movement.md`), including retained
MORALE, direct faction bonuses, home Crèche and immunity. Do not substitute
stored morale or combat/movement mode1. A native riot-home control returns
M1 in this scan versus M2 in mode1, changing its attack contribution1 versus2.

For each such unit, compute `T = trunc((M+6)*A/8)` separately. Add T to owner
attack and its regional attack word **unless A<=D and current order code2**.
An A>D unit contributes even with order2. There is no damage-percentage or
chassis-speed multiplier, and no floor forcing a contribution of1.

The phase also adds `trunc((M+6)*(A+D)/8)` to a separate owner strength total,
even when order2 suppresses attack. That total is not the regional attack sum.
A separate special-plan query skips this strength/morale/attack subpass when
plan5 returns a nonzero reactor value; it does not undo its earlier unit count.
Neither of the four admitted packages uses that special plan.

Stock Scout/Rover have A1/D1; stock Colony/Former modules have A0/D1. Thus stock
civilian attack is0 because A is0, **not because the attack accumulator filters
out their plans**. Explicit positive-module diagnostics make both contribute
while their regional unit count stays0. A water-Base Scout with effective M2
similarly has regional units0/attack1.

The admitted Gaian retained MORALE is−1. Stored1 gives effective0 and attack0;
stored3 gives effective2 and attack1. The stored-morale and retained-rating
controls are explicit state inputs, not organic promotions/social commands.
Damage9 leaves the checked contribution unchanged. Two individually truncated
weak combat contributions remain0; do not divide their combined numerator once.

Both regional arrays are unsigned16 storage and wrap modulo65536. A declared
mid-phase65535 canary plus one eligible count/attack becomes0. This is a storage
discriminator, not a world containing65,536 organically created units.

## Every regional flag written by this unit phase

These are OR operations on the existing regional flag word. They are separate
from unit-state flags, Base governor flags and the later peer/strategy flags.

| Reached owned-unit condition | Regional flag value |
| --- | ---: |
| Plan8 Colony: first one in the region | 2 |
| Plan8 Colony when flag2 is already set | 2048 |
| Plan11 Probe | 4096 |
| Plan3 outside a valid Base marker | 8192 |

The second Colony retains flag2, yielding2050. Probe and nonstock plan/domain
controls are diagnostics for closing these flag branches, not newly admitted
port unit packages. A plan3 Rover/Scout at a valid Base does not set8192;
affiliations0/2/7 suppress it, while8/15 do not. This gate has no land-only
restriction: a transported plan3 Scout can set8192 in a sea region.

No additional regional flag stores occur in the watched unit-phase calls,
including their executed helpers. The separate later peer phase can add other
flags; final owner-summary output must not be mistaken for this boundary.

## Old strategy and limits

The unit scan runs before strategy entries are rebuilt. Its nonhuman AI branches
can read **old** strategy to redesign/disband/reassign units, and later tactical
coordination also reads retained state. Those AI operations are not certified
by this positive-human receipt. For these clean human Scout/Rover cases,
old codes0/1/2/3/7/8/99 give identical unit/attack/flag arrays. Preserve the old
array until its real update; do not invent a fixed7 input or move the strategy
pass earlier.

All unit factories and count/attack/armor/morale helpers execute natively; no
gameplay return is supplied. Orders and terrain/chassis/plan changes are field
controls, not executed player commands. Existing capability, territory and
general morale receipts are reused. No new full AI, arbitrary design, visible
gameplay, phase scheduling or port implementation claim is made.
