# RE-SMAC-UNIT-REPAIR-001: Unit recovery during faction upkeep

Status: original-x86 full faction-upkeep checks, with controlled damage,
placement, components, facilities and project ownership. The accepted startup
world and reached gameplay routines execute; inherited presentation/host
services are supplied. This covers ordinary recovery after the
[conventional combat loop](scout-conventional-combat.md), rather than monolith
interaction or a general repair command. [Fixtures](unit-upkeep-repair-cases.json)
retain damage in integer health points, not rounded display percentages.

## Activity and timing

The original keeps prior activity separately from movement points spent. During
the unit pass, it resets movement points before checking recovery eligibility.
A unit marked active from movement does not repair in that pass. Later faction
processing clears that activity marker, enabling recovery on the next idle
upkeep. Do not decide repair eligibility from the freshly reset movement count.

The continuous checked sequence starts a damaged Scout at its base, moves it
one ordinary eastward step, then runs two full faction upkeeps without further
commands. Damage7 remains7 on the first upkeep, movement resets to0, and the
activity marker clears later. The second upkeep repairs damage7 to5 on its
own territory. No reseeding, relocation or activity reset is supplied between
the move and either upkeep. A synthetic spent-points-only control repairs;
that control distinguishes the inputs but is not a normal movement history.

## Ordinary repair amount and field limit

For the checked valid conventional components, let R be the reactor multiplier
(health capacity10R) and D the current integer damage. Start with repair rate1:

- Add1 on the unit's own territorial claim.
- Add1 for a ground unit in a bunker, or an air unit on an airbase.
- Add1 at a base unless that base is in the tested unrest state.

These are additive before multiplying by R. Ground and sea checks verify the
territorial increment; the tested ordinary ground/sea/air bases each repair3
points with R1. Unclaimed and foreign territory each repair1. A ground bunker
repairs2 on unclaimed land and3 on own land. Ordinary fungus grants a
conventional unit no additional repair rate.

For ordinary field recovery, damage cannot be reduced below2R (80% maximum
health). A base removes this field floor even without a repair facility.
Recovery never increases existing damage: a unit already below the field floor
stays there. Thus an unclaimed R1 unit at damage2 stays2 and damage1 stays1;
an R2 unit at damage5 repairs to4. Other R2 checks repair damage15 to13 in
unclaimed territory,11 on own territory and9 at its base.

For these ordinary paths, the new damage is
`min(D, max(field_floor, D - R * repair_rate))`.
The actual consumer has other special branches; do not apply this reduced
formula as a replacement for those branches.

## Facilities and projects

With no prior activity, the matching base repair facility restores the tested
conventional unit completely in one upkeep:

| Unit domain | Matching facility | Checked wrong facility |
| --- | --- | --- |
| Ground | Command Center | Naval Yard gives ordinary repair |
| Sea | Naval Yard | Command Center gives ordinary repair |
| Air | Aerospace Complex | Naval Yard gives ordinary repair |

Command Nexus and Maritime Control Center are the respective project
substitutes in the static decision path; the Maritime Control Center sea case
executes and fully repairs. Only that project substitution is runtime-checked
here. The tested unrest state suppresses the ordinary base increment and
Command Center full repair, but the location still removes the field floor.

Owning the Nano Factory fully repairs the tested ordinary field/base units,
including a base in unrest. Conventional ground units on land fungus with
Xenoempathy Dome ownership repair an extra point and can heal beyond the
ordinary field floor; damage7 becomes5 in the unclaimed case. Project
construction/acquisition and loss are supplied inputs, not traced lifecycle
events in this receipt.

## Native life and sentry orders

The loaded predefined Mind Worm has a distinct fungus path. On unclaimed
ordinary land, damage7 becomes6 with the ordinary field floor. On land fungus,
damage7 becomes5 and its floor is0; own territory adds another point, giving4.
A custom design equipped with Psi on the same fungus instead gives6: a
negative-strength weapon alone does not establish the predefined-native path.

At the tested ordinary base, Mind Worm damage7 becomes4. A Biology Lab or
faction-owned Xenoempathy Dome makes that recovery complete, whereas a Command
Center alone still gives4. Additional native lifecycle bonuses and other native
designs are outside these runtime controls.

For a human ground unit with Sentry/Board Transport order on ordinary land,
repairing damage3 to the field floor2 clears the order even though the unit is
not fully healed. At a base, damage2 to0 also clears it. An already-below-floor
unit at damage1 makes no recovery and keeps its sentry order. This receipt
checks the order transition, not a complete user gesture for issuing sentry.

## Scope and remaining branches

There are48 distinct prepared contexts across49 executions (the continuous
move sequence is replayed), totaling51 full faction returns and51 complete
unit passes. Main entry/return stack and instruction-stop checks are retained;
follow-up worker calls check the native stop, not a complete saved-register
audit. Checked unit/base counts, coordinates and shared RNG stay stable during
recovery; no extra damage draw occurs in these contexts.

Component/terrain/project/facility inputs are explicitly controlled. Custom
Foil/Gravship Gun designs test resolved domain behavior without claiming full
workshop creation or legal field placement. Full transport eligibility,
neutral native faction recovery, other native bonuses, air fuel/return rules,
allied-base admission, base capture and full rendered repair commands remain
open. SMAX behavior is not established by these SMAC checks.

The [Repair Bay/boarding extension](repair-bay-boarding.md) now checks the exact
ground/capacity/order/other-unit gate with positive controls and six actual sea
boarding sequences. The ordinary loaded Scout skips the Bay scan even aboard
a Bay-equipped Foil, retaining normal field recovery; broader transport
movement/unloading and workshop admission remain open.

## Live first50 connection

`UnitManager::ResetConventionalSpent` now applies ordinary positive-owner ground
repair using actual retained activity bit4, recognized claim, Bunker, Base unrest
and Command Center27. Fresh accepted movement/capture/attack actions retain bit4;
planning clears it separately through `ClearConventionalActivity`, after Base
processing. Missing whole-word history on migrated Units remains unknown and
does not manufacture idle eligibility. The current no-project ground campaign
does not admit the separate native0, project, air or Repair Bay branches above.
The composed live fixture checks field floor, own territory, Base, matching
facility, unrest, and actual move→maintenance→planning→idle maintenance.
