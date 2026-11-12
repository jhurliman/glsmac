# Base entry, PSI defense and movement marker

2026-11-11. `RE-SMAC-BASE-ENTRY-STATIC-001`. Static causal audit of the pinned
original executable, SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
No new runtime matrix. Reuse [strength inputs](early-ground-combat-causal.md),
[movement admission/spending](movement-admission-and-spending.md),
[edge costs](movement-edge-cost.md), and
[failure marker](movement-fungus-and-failure-marker.md).
Addresses below identify original instructions; original assembly stays private.

## PSI against a defending Base

In composed strength `516390`, nonzero PSI context branches at `516fcc` to
`517d7b`, bypassing ordinary terrain/facility-defense composition and the
conventional Infantry-versus-Base attack modifier. For a valid defending Base
ID, apply RULES intrinsic Base defense `9ba4a8` (installed25) to the defender's
**intermediate strength before the final multiplication by4**:

```
intermediateD = trunc(intermediateD * (100 + intrinsicBasePct) / 100)
finalD = intermediateD * 4
```

The percentage executes at `517dfb`..`517e12`; multiplication by4 follows at
`517e14`. Missing Base or zero rule skips only the percentage. Preserve the
existing component-stage faction PSI percentage and morale order. For example,
neutral human morale1 land PSI defense has component16, intermediate112,
Base-adjusted140, final560. With faction PSI17%, component18, intermediate126,
Base-adjusted157, final628: applying25% to final504 would incorrectly give630.
These are source-derived arithmetic examples, not additional executable probes.

The conventional Infantry/Base attack rule `9ba4f4` is not an extra native
PSI attack bonus. Besides the PSI bypass, that conventional block explicitly
rejects owner0 at `517ba0`..`517ba2`; its other gates include ordinary mode,
valid Base, eligible non-predefined-negative weapon, land domain and raw speed1.
Do not grant a Worm an Infantry/Base attack bonus merely because its speed is1.

## Exact noncombat defense exception

The late stage `51840f`..`5184fb` checks **defender raw weapon power ==0 and
raw armor power ==1**. If both hold, the installed noncombat penalty is50%
(`9ba500`), except when **attacker owner==0 AND defending Base ID>=0**.
That exact conjunction skips the penalty (`518450`..`518460`). Otherwise:

```
finalD = trunc(finalD * (100 - noncombatPenaltyPct) / 100)
```

This is after PSI/Base composition. Thus an ordinary Former/Colony in a Base
does not suffer this penalty against a wild native; outside a Base it does.
A positive-owner attacker does not gain this exemption for its target, even
inside a Base. The check uses powers, not a generic civilian Boolean.

## Owner0 entering an undefended positive-owner Base

After successful admission/spending, `5afb22`..`5afb3c` selects a special raid
when destination Base exists, Base owner differs from moving Unit owner, and
moving Unit owner==0. This path does **not** enter ordinary relocation/capture
at `5aff04` or ownership-transfer helper `520730`.

1. It attempts random facility damage: CRT modulo39 selects IDs0..38, with
   native faction bonus exclusions, installed-facility checks, and a special
   facility26 sea-Base condition. IDs<=1 or>=34 are retried. It can examine101
   candidates (counter starts0 and repeats while incremented counter<=100).
   An eligible installed facility calls setter `4fa1c0(facility, Base, 0)` at
   `5afca1`. Do not reuse conventional capture's per-facility random loop.
2. At `5afd13`..`5afd1b`, it calls **`5d8c40(invadingUnit)`**. An ordinary
   ground Mind Worm with zero transport capacity takes that helper's direct
   `5d89f0` deletion route at `5d8d4b`. The invader is consumed; no surviving
   Worm is placed inside the Base and no movement allowance is retained for it.
   Admission already charged the edge before deletion.
3. A subsequent25-offset neighborhood sweep follows native stacks and keeps
   only the first quota of owner0 Units, deleting additional members. Its
   counter is shared across the sweep, initially0; quota is the attacked
   owner's DWORD at `95d608 + owner*20bc` (`5afd29`..`5afe3a`). The follow-up
   below identifies it as retained clean-mineral history H; use that authority.
4. **Global turn<10 skips population loss.** At turn>=10 decrement Base
   population byte (`Base+6`) by1 (`5afe40`..`5afe63`). If predicate `5c3a20`
   succeeds, clamp the result to at least1. Otherwise population0 calls actual
   Base destruction `4faad0` at `5afe89`, then refresh/aftermath and common exit.
   This is destruction, not transfer to faction0.
5. For a surviving Base (including before turn10), if facility search exhausted
   its counter>100, clear Base DWORD+`0x40` at `5afec5`. Then redraw/recalculate
   through `477790` and `4209a0`, and jump to common exit `5b1203`.

Protection predicate `5c3a20` is not a Pressure Dome test. It returns true for
global `959fa4 & 0x20000000`, or Base DWORD+`0x34 & 0x8000`; alternatively
global `959fa4 & 0x40000000` enables the Base-ID scan `95a01c..<95a0a0`;
global `959fc8 & 0x04000000` enables the selected-facility `95a2a0<34` check.
Record these objective/scenario gates literally until their producers are
connected. Ordinary rules with none set permit population1 to be destroyed
from turn10 onward. Facility damage and invader deletion still run before10.

## Conventional positive-owner undefended capture

For ordinary positive-owner land military Infantry/Rover, ordinary admission,
diplomacy, occupancy, domain and remaining-movement gates still apply. There is
**no exact Gun/Scout-armor/design2/speed1/full-allowance3/spent0 restriction**
on the shared capture branch. The earlier raw-weapon test at `5aceee`..`5acf00`
rejects zero power on that route; positive Laser/Impact powers do not fail an
equality-to-Gun check. Armor is not an equality gate for capture. Special
Probe, transport, air and civilian admission are outside this statement.

At `5aee16`..`5aee37`, require actual full allowance minus unsigned spent byte
to be positive. Call actual edge cost `5aaad0` with terrain switch0 at
`5aee51`..`5aee60`; preserve ordinary partial-point/fungus admission rules.
On admission, `5af338`..`5af342` writes **spent = priorSpent + edgeCost** using
native byte arithmetic. Do not clamp to allowance or replace with3. Connected
roads/Base links use their actual cost, including zero where applicable; zero
cost still does not admit an exhausted Unit.

Positive owner bypasses the raid at `5afb3c`, relocates through
`5d01c0(Unit,destX,destY)` at `5aff0f`, then the foreign non-Pact Base route
clears actor damage at `5aff53`, calls `520520`, and calls
`520730(BaseID,newOwner,0)` at `5aff67`. It jumps directly to `5b0185` after
capture, bypassing the unrelated scan/cost addition at `5aff81`..`5b0144`.
There is no capture-specific exhaustion call in this ordinary caller branch.
Reuse existing ownership/population/facility/booty/screen-return contracts;
this audit does not replace those helpers with a single owner assignment.

## Partial-movement prior-failure marker

Storage is **Unit DWORD+4 mask `0x00008000`**, equivalently Unit byte+5 bit80.
Unit roster base is `9951b0`, stride52. Spent movement is a separate unsigned
byte **Unit+`0x28`**, at `9951d8 + 52*unitID`; it does not own this marker.

The partial lottery reads flags at `5af010`, masks8000 at `5af01c`, and adds Q
to its admission threshold at `5af026` when set. Failure exhausts movement via
`5d9c80` at `5af2cd`, then ORs8000 into the flags DWORD at
`5af2d2`..`5af2e5`. Successful admission adds full cost to spent and clears
flags8000 and40 together with mask`ffff7fbf` at `5af34e`..`5af356`.
The retained failure-marker contract already executes turn-reset sequences:
upkeep resets spent while retaining8000; the next attempt uses thresholdR+Q;
a successful move clears the marker. Do not reset it merely at turn start.

Private source slices, this receipt and file hashes are frozen in
`tmp/reference/independent-base-entry-static-evidence-20261111/`.

## Former fungus entry and Sensor/Remove Fungus continuation

Additional static audit, 2026-11-11. Ordinary land Former plan9 has **no
plan-specific exemption from the earlier native-birth encounter check**.
`5acb32`..`5ace0a` checks domain, empty destination, positive owner, fungus,
terrain band, airbase, profile/project/scenario gates, nearest Base and the
CRT encounter roll. The Former plan is not tested there. Zero raw offense
enters the `95a03c` project-owner check at `5acbd5`..`5acc05`, rather than
unconditionally bypassing encounters: ownership of that project suppresses
this stage for the zero-offense profile; no owned project means continue.
The nearest-Base query is `4f9450` at `5acc47`, distance<=1 skips the roll;
reuse [the existing encounter recipe](distant-fungus-encounters.md) and
[native birth](native-birth-producer.md), rather than treating Former entry as
automatically safe. A birth is not permission to immediately resolve an attack
with the discovering Former; preserve existing birth/command-return routing.

Former-specific plan9 branches at `5aef6f`..`5aef71` and
`5af1c1`..`5af1c3` occur **later**, in movement admission/fungus handling.
They do not erase an earlier encounter or its CRT draw. Keep raw cost,
encounter and movement lottery as separate stages.

The actual action command is `4e7d40(Unit,action)`; Sensor is action9 and
Remove Fungus action10, stored work orders13/14 respectively. Common gates
include Former role (`4e7d9b`), chassis/terrain domain compatibility
(`4e7e52`..`4e7e65`), available terrain action/prerequisite
(`4e7f44`..`4e80fc`), and refusal on an ordinary founded Base square
(`4e81c6`..`4e81d9`). The following statements concern ordinary land outside
special scenario/landmark contexts; generic landmark restrictions remain.

* **Sensor on clear, rolling or rocky land:** no rockiness-specific rejection
  in this action route. Forest is legal and preserved. The generic existing-
  result test (`4e823a`..`4e824d`) rejects an already installed Sensor.
* **Sensor on land fungus:** common fungus gate `4e850e`..`4e8610` requires
  the evaluating faction to possess technology selected by RULES `9ba464`
  (installed numeric tech63). It calls actual possession helper `5d17b0`.
  Sensor's own ordinary land prerequisite is -1, but that does **not** bypass
  the separate fungus-improvement gate. Completion preserves fungus and forest;
  it replaces Mine, Solar, Condenser, Mirror and Borehole as already documented.
* **Remove Fungus on land fungus:** no ordinary prerequisite (-1), no result
  mask, and therefore no fungus-improvement-tech gate. Fungicide ability can
  bypass a missing action prerequisite, but is not required in stock rules.
  Action10 takes default dispatch `4e93fd`, then immediately jumps to accepted
  order assignment at `4e9941`. There is **no fungus-present rejection in this
  command body**: a supplied no-fungus request can assign the no-op work order.
  A UI may offer removal on fungus; do not mistake that presentation choice
  for a proven native command refusal when fungus is absent. Forest/rockiness
  do not add an action10 refusal. Ordinary ocean is unavailable to this land
  Former through the common domain gate; special sub-shelf/scenario terrain
  must retain its common gates.

Both actions use the retained work/replacement contracts. Remove Fungus has
installed time6/threshold12 and ordinary rate2; Sensor time4/threshold8.
Partial movement still allows a full work-rate contribution, then exhausts the
Former through the existing dispatcher/work setter. Remove Fungus completion
removes only fungus, preserves forest and receives no forest-harvest award.
Its action10 special completion dispatch calls `5a99d0(x,y,0)` at `4dff79`,
which clears the upper nibble of map byte2 over the native21-offset neighborhood;
preserve this cache invalidation rather than only clearing the improvement bit.
Sensor skips that special dispatch. Both perform ordinary memory/redraw refresh.

Neither Sensor nor Remove Fungus enters the explicit completion diplomacy
branch: `4e01d7`..`4e01e8` restricts it to actions16,17,12, otherwise jumps
to `4e0369`. Do not attach that branch's treaty/war effects to action10.
This does not waive earlier occupancy/border movement legality. No new runtime
matrix or foreign-territory UI replay is claimed. Source and numeric input
hashes for this continuation are frozen separately in
`tmp/reference/independent-former-fungus-static-evidence-20261111/`; the earlier
Base packet remains immutable.

## Lazy combat-effect palette and shared RNG

Static follow-up, 2026-11-11. Do **not** initialize this palette at every
combat transaction, every first native attack, or every first human attack.
The actual effect entry `518de0` tests process-global byte `91f048` bit4 at
`518e6d`..`518e80`. Only its first admitted call constructs object `918ce0`
via `643e10`; that constructs the embedded palette through
`6174d0` → `617560`. At `617670`..`617686`, the palette calls shared RNG
`6330f0(0,65535)` and repeats if the result is zero, then stores a nonzero
identifier at palette+`0x400`. The retained fixture happened to consume one
draw; the source is a reject-zero loop. The lazy lifetime is process-wide,
not one initialization per faction or per battle. Other effect callers can
initialize the same object before a battle does.

For ordinary ground combat `51ab20`, the presentation-enable local is produced
at `51c63b`..`51c70e` and consumed by hit/death effect routing. Let F be focused
faction `9430d4`:

* Initially enable if attacker owner==F, defender owner==F, or global
  `959fc8 & 0x80` is set (`51c647`..`51c660`). **Owner0 is not a rejection.**
* Separately, either participant is display-visible when its visibility byte
  Unit+`0x27` includes F, or it belongs to F and its word Unit+8 does not have
  both mask`0x440` bits. This produces the visibility local at
  `51c668`..`51c6d6`.
* For an uninvolved viewer, enable when that visibility local is true AND the
  appropriate display setting is set. If relation[F][attackerOwner] has bit1,
  test global `959f98 & 0x8000`; otherwise test `959f98 & 4`
  (`51c6da`..`51c70e`). Keep these setting bits literal rather than conflating
  ordinary combat eligibility with viewer effects.

The ordinary hit routes test presentation-enable and their damage-display
threshold before calling `518de0` (`51d3ce`..`51d461`,
`51d7ac`..`51d831`). Initialization happens upon reaching the actual effect
entry; enabling presentation alone is not the constructor call. Its lazy
constructor runs before the effect entry's later drawing decisions, so a
Fast Battle/render omission is not by itself proof that no identifier draw
occurred. Preserve the effect-call boundary if reproducing this RNG state.

Consequently, a native/AI attack against the **focused faction** can consume
the first-use shared draw when effects first run. An ordinary offscreen battle
between two other factions skips these calls with the extra setting/global
gates clear. The retained native-offense shared-state3 observations do not
establish a blanket owner0 exemption. Core combat rounds still use CRT; do
not insert this presentation draw into that stream or into every transaction.

## FANATIC versus ground PSI

FANATIC does **not** apply on the PSI branch. In `516390`, faction bonus flag
`0x400` at `95a9d0 + faction*59c`, nonzero RULES attack bonus `9ba4d8`
(installed25), and ordinary mode are checked at `516a9f`..`516ac3`.
Then `516ac9`..`516acf` tests PSI context: nonzero jumps directly to
`516b72`, bypassing FANATIC multiplication. Conventional context applies
`trunc(A*(100+bonus)/100)` at `516ad5`..`516af4` and skips the PLANET block.
The PSI route instead applies retained attacker PLANET at
`516b72`..`516c32`. Thus the implementation's conventional-only FANATIC gate
is correct for stock Believers versus a Worm: do not stack FANATIC25% with
the retained PLANET PSI percentage. The earlier component-stage faction PSI
percentage and morale still precede this branch.

These follow-up slices and the updated receipt are frozen separately in
`tmp/reference/independent-combat-presentation-static-evidence-20261111/`.

## Raid quota and failed-facility-search store: resolved authorities

`95d608 + owner*20bc` is the existing **faction clean-mineral history H**:
`4ff367` reads it and adds16 for the clean allowance. Reuse
[clean-mineral history transitions](clean-minerals-and-ecology-burden.md) and
[its initializer/save lifetime](ecology-input-lifetimes.md). These already
include poisoned native reset/restore controls: fresh turn0 faction initialization
clears H to0, nonzero-turn initialization preserves it, ordinary load restores
the saved value. Static reset is `5c8b9a` (zero EBX) through `5c8c32` (store H).
Do not introduce a second independently maintained raid-quota field.

Known writers are qualifying facility completions at `507cf0`..`507cfb`
(increment only when H was nonzero), qualifying ecological growth selection
at `50ba0b`..`50ba1a` (increment), and its later AI rollback
`50bed3`..`50bedb`. Thus fresh H0 really implies no neighborhood natives are
retained by the raid sweep's quota, but it must not remain hardcoded0 after
those producers or after loading a saved nonzero H.

The Base DWORD+`0x40` cleared at `5afec5` is **stored production minerals**
(`96f1f0 + BaseID*304`), already used by production/founding/capture receipts.
It is not food, facilities or politics. Clear that mineral store only on the
raid's facility-search exhaustion path (attempt counter>100); this instruction
does not also clear food, queue, target or the production-loss baseline.

### Former partial fungus admission: exact branch meaning

`5aeeea` compares remaining R with Q **before** the plan9 exemption:
R<Q jumps to the lottery path at `5aef8b`. Only R>=Q reaches the fungus tests
and plan9/12 success bypass at `5aef6f`..`5aef79`. Therefore a partial Former
with R2/C3 does take the normal movement lottery (subject to existing controller
execution-mode bypasses); being a Former does not automatically admit it.
Successful admission still stores S+C. The plan9 test at `5af1c1` is in the
**already failed** attempt's presentation/advice path; it skips fungus-specific
advice, not the exhaustion/marker-setting failure outcome. No Colony plan8
exception is present in either test: ordinary Colony fungus admission uses the
normal conventional route and raw cost, rather than rejection by a Scout-only
cost validator. All these movement stages follow the earlier encounter check.

## Sensor Array defense consumer

Static follow-up, 2026-11-11: `516390` scans Sensors at
`517e1f`..`517fef`, **after both conventional and PSI defense composition**.
It is not bypassed by PSI. The ordinary defender must stand on **land**
(tile elevation band>=`0x60`, `517e42`..`517e45`) and its owner must be
nonzero (`517e57`..`517e5f`). An owner0 defender receives no Sensor bonus
through this consumer. Sensor placement and its sight/encounter effects
remain separate from this combat predicate.

Scan exactly25 native offsets: center; eight adjacent; four `(±2,±2)`;
eight `(±1,±3)`/`(±3,±1)`; then `(4,0),(-4,0),(0,4),(0,-4)`.
For valid doubled-coordinate parity this is **abs(dx)+abs(dy)<=4** about the
defender, including the four axial tips absent from the21-square Base radius.
Wrap x on a wrapping map, clip x on a flat map, and always clip y
(`517e71`..`517ebe`). Do not use an unwrapped Manhattan test across the seam.
Candidate must carry actual Sensor improvement mask`0x80000000`.

Candidate ownership is resolved as follows, not by a generic friendly Boolean:

* **Land Sensor:** call `4f97a0(defenderOwner,x,y,0,0)`
  (`517f18`..`517f3d`). Accept if returned recognized owner is **negative or
  exactly defenderOwner**. Reject another recognized owner. Therefore unclaimed
  land and foreign claims hidden by the ordinary recognition gate can qualify;
  use the existing recognized-territory authority. Do not substitute a direct
  raw-claim equality check, or add Treaty/Pact acceptance for a recognized
  foreign claimant.
* **Sea Sensor:** call all-owner nearest-Base query `4f9450(x,y)` and compare
  that Base's owner exactly with defenderOwner (`517ef3`..`517f16`). This
  source branch does not query the candidate's raw claimant or grant an ally
  exception. Normally constructed Sensor action9 has an unavailable sea rule;
  this is the consumer's literal handling of a supplied existing sea Sensor,
  not proof that ordinary sea construction is legal. A Base-less synthetic
  sea-Sensor world is outside the coherent-world assumption of this branch.

Any qualifying candidate ORs a Boolean found flag. The scan continues, but
multiple Sensors **do not stack**. Once found, apply RULES `9ba510`
(installed25) at `517f66`..`517f89`:

```
D = trunc(D * (100 + sensorDefensePct) / 100)
```

This uses already composed final D, including terrain/Base/facility composition
and the earlier final multiplication by4. For PSI, intrinsic Base percentage
and its rounding occur before ×4, then Sensor percentage occurs after ×4.
Example source-derived neutral PSI defense448 becomes560 with Sensor; Base
PSI defense560 becomes700 with Sensor. The earlier faction-PSI17%/Base example
628 becomes785, preserving the separate divisions. Later air/ability and
zero-offense/armor1 noncombat stages remain later; the native-attacker/Base
noncombat exemption still applies there. Do not combine Sensor and intrinsic
Base percentages into an additive50% or apply a second Sensor for overlap.

Private source and numeric offsets are frozen separately in
`tmp/reference/independent-sensor-defense-static-evidence-20261111/`.

## Native raid razing and last-Base faction disposition

Static continuation, 2026-11-11. **Base deletion `4faad0` does not itself call
faction disposition/restart.** Its full body handles Base loss history/counts,
homed-unit rehome/removal, Base-record compaction/reference repair, map marker
and radius/territory rebuild, then returns at `4fb2b2`. There is no call to
`5ca7f0` or faction initializer `5c8650` in that body. Reuse the existing
[Base removal contract](starvation-base-removal.md), retaining its unit and
compaction effects; deletion alone is not the completed faction-loss transaction.

The **native raid movement caller explicitly follows deletion with disposition**:

```
5afe88..5afe89: 4faad0(attackedBaseID)
5afe8e..5afe90: 4779e0(1)                 // display refresh
5afe95..5afe9c: 5ca7f0(attackedBaseOwner, 0)
5afea4..5afea6: 4779e0(1)                 // display refresh
5afeae:        jump common movement exit 5b1203
```

The owner argument is the saved pre-deletion destination owner, not a Base
record read after compaction. At `5afe95`, two still-pushed arguments shift
the local frame: `[esp+40]` refers to original local+38, that saved owner.
The second disposition argument is literal0 (native/no positive instigator).
Do not pass the deleted Worm Unit ID or treat faction0 as capturing this Base.
The invader was already consumed at `5afd1b` before the population test.

Disposition `5ca7f0(owner,0)` immediately returns for owner0 or if any Base
record still belongs to owner (`5ca803`..`5ca836`). Otherwise it removes all
remaining own Units in descending live-index order, clears the faction Base
count, and calls actual nonzero-turn faction initialization/restart
`5c8650(owner,0)` at `5ca893`. Thus reuse the existing
[last-Base restart/disposition authority](last-base-restart.md), including its
turn/Do-or-Die/site-search gates, treasury handling, replacement units and
elimination outcomes. The second-argument-positive defeat-credit/relationship
branches do not apply to this literal0 raid caller. Do not hardcode immediate
elimination or skip restart merely because the loss was caused by natives.

For the port transaction: delete/compact the Base through normal removal, then
prepare faction disposition from the resulting world and saved victim owner,
with instigator0, before finishing the raid command. Restart can change the
Unit roster again; no continuation should dereference the consumed native or
stale pre-compaction Base/Unit indices. Surviving-Base and before-turn10 raid
paths do not call this destruction/disposition tail.

These are source-backed caller links to retained executed deletion/restart
contracts, not a new full native-raid/restart replay. Evidence is frozen in
`tmp/reference/independent-native-raid-disposition-static-evidence-20261111/`.

### Raid facility exclusion reads faction0's definition

At `5afb5e` the raid loads count **`95a9d4` with no faction stride**, then
scans kind DWORDs at `95a9d8 + 4*i` and value DWORDs at `95a9f8 + 4*i`
(`5afb68`..`5afb81`). These are faction-definition slot0's bonus list, not
the attacked owner's list. Eligibility is suppressed exactly when a record
has **stored kind==2 AND stored value==candidateFacilityID**. Kind2 is the
stored `FACILITY` grant representation also consumed by ordinary founding;
it is not the positional index of the FACILITY token in BONUSNAMES. No generic
FREEFAC/tech-grant/project-equivalence or victim-intrinsic-facility predicate
is consulted by this loop. There is no parameter clamping in this comparison.

Consequently a victim Hive Perimeter or University Node is not protected by
this test merely because the victim received it as a faction grant. If the
native slot0 list is empty, no candidate is excluded by this bonus-list gate.
Use the retained native definition as input rather than assuming the victim's
grant list or inventing a separate protected-facilities set. Candidate26 is
separately ineligible on a sea Base; on land it can pass the normal installed-
facility test. HQ<=1 and IDs>=34 are excluded by their separate numeric gates.
