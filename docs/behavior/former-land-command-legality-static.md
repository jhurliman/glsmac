# Land Former command legality: rockiness, forest and landmarks

2026-11-12 static audit of actual command `4e7d40(Unit,action)` in pinned
stock SMAC. This is command admission, not an inference from the quote/cost
calculator. Scope: ordinary positive-owner land Former plan9, no abilities or
projects, real land class>=3, no fungus, ordinary installed prerequisites,
no Base/Monolith/protected Mount combination, and requested result absent.
No new runtime matrix. Existing flat command/completion and replacement
receipts remain their executed authorities.

## Terrain decisions

| Requested action | Flat | Rolling | Rocky | Existing forest, otherwise eligible terrain |
| --- | --- | --- | --- | --- |
| Farm0 | Allow | Allow | Reject | Allow replacement, with confirmation |
| Mine2 | Allow | Allow | Allow | Allow replacement, with confirmation |
| Solar3 | Allow | Allow | Allow | Allow replacement, with confirmation; energy inquiry below |
| Forest4 | Allow | Allow | Reject | Reject already-present Forest |
| Road5 | Allow | Allow | Allow | Allow; preserve forest, no replacement confirmation for it |
| Sensor9 | Allow | Allow | Allow | Allow; preserve forest, no replacement confirmation for it |

“Allow” means these terrain predicates permit continuing to command commit;
all common guards below still apply. A rocky+forest supplied combination does
not bypass the rocky Farm/Forest rejection. Ordinary Solar cost and Road cost
still use their existing rock/forest adjustments; permission is not flat cost.

At **4e849f..4e84b6**, on land, read raw rockiness `tile.byte5 & 0xc0`.
If it is **greater than0x40** and the action's **add mask** intersects
**0x00208000** (Farm8000 or Forest200000), reject. Rolling40 passes; rocky80
and syntheticc0 fail. It tests neither Mine10, Solar40, Road4 nor Sensor80000000.
There is **no generic forest-present veto** in this path.

The action switch at4e86c2 uses requested action minus3. For this task:
Farm0/Mine2 take the default path; Forest4/Road5/Sensor9 map to default4e93fd;
**Solar3 maps to4e86e4**. Solar calls actual energy query
`4fcde0(owner,-1,x,y,1)` at4e86fd and rejects only if it returns0 at4e8705.
That query's mode1 admits prospective elevation energy even without installed
Solar, and suppresses the ordinary technology cap. It is not a raw-rockiness
ban. On ordinary clear land the prospective elevation term is class-2>=1;
land forest takes the existing Forest-energy branch (stock1). Preserve this
actual zero-output guard for other rule/event contexts rather than deleting it
or substituting a guessed “Solar only on flat” test. The full energy helper
and its resource/landmark/event arithmetic already have the
[tile-energy receipt](ordinary-tile-energy.md).

## Common guards in actual command order

1. Require design plan9 (`4e7d9b`) and chassis/terrain compatibility
   (`4e7e52..4e7e65`). A land Former on sea is a separate rejection.
2. Resolve the command prelude: Farm on land fungus can become Remove Fungus;
   Farm on existing Farm can become Soil; Road on existing Road can become
   Mag Tube (`4e7ec9..4e7f13`). Those resolved commands need their own
   prerequisites. Do not treat a repeated Farm/Road click as an unconditional
   duplicate rejection. This command prelude is distinct from the later
   [work-entry resolver](terraforming-automatic-upgrades.md).
3. Require the selected domain/action prerequisite record to be enabled and
   actual helper `5d1fe0(action,seaFlag,actingOwner)` to accept it
   (`4e7f44`, `4e80c2`). None=-1 is accepted for a valid positive owner.
4. Reject a real Base square: actual item1 plus affiliation nibble0..7
   (`4e81c6..4e81d9`). Reject an already-complete nonzero action add mask
   (`4e823a..4e824d`). These test actual items, not remembered display items.
5. Actual **Monolith2000** blocks nonzero-result actions other than Road5,
   Tube6 and Bunker11 (`4e82bc..4e82e3`). Of the six requested actions,
   Road has that exception; the others do not.
6. Apply the protected-landmark and rockiness checks below, then the separate
   fungus-tech gates if fungus is present. For fungus Sensor and other
   non-Road result actions, retain the existing technology63 gate; the no-fungus
   table above grants no fungus bypass.
7. Run the action-specific switch, replacement confirmation if needed, then
   normal command admission/dispatch. The order store is **action+4** at
   `4e999f..4e99a2`; normal command machinery can still decline/cancel before
   that store. Issuing a work command does not itself complete terraforming.

## Existing forest and completion

At **4e9442..4e9451**, intersect actual items with the active action's
**removal mask** (`6afd14+action*20`). If nonzero, the command scans conflicting
improvement names and asks replacement confirmation through `5d6330` at4e96d7.
Zero answer cancels; nonzero continues to4e9941. Forest is not a hard rejection
for Farm/Mine/Solar. It is an improvement being replaced after confirmation.

Use the already executed [replacement rules](terraforming-improvement-replacement.md):
Farm removes Forest; Mine/Solar remove Forest and their other listed conflicts;
Road removes nothing; Sensor's removal set excludes Forest. Both Road and
Sensor retain existing forest. Sensor may require confirmation for a different
conflicting improvement, such as an existing Mine or Solar.

Reuse [forest harvest](forest-removal-minerals.md) at successful completion,
not command admission: forest before, no forest after, resulting land and a
valid recipient admit the configured award (stock5 stored minerals), with its
existing home/nearest-own selection and switching-baseline updates. Thus
Farm/Mine/Solar forest replacements can award; Road-on-forest and
Sensor-on-forest **do not** remove forest or award harvesting minerals.
Preserve the existing affected-owner item-memory update and ordinary work
cleanup; this supplement adds no broad site-rating invalidation.

## Landmark predicates

There is no blanket “any landmark” ban. The explicit common check is
`(tile.landmarkWord & 0x80000002) == 2` (`4e83ba..4e83c8`), the active Mount
Planet marker2. In that state reject if either:

* the action add mask intersects208000 (Farm or Forest), **or**
* the landmark word's entire high byte is zero (`4e83ce..4e83de`).

Thus a protected active Mount tile with high byte0 rejects all six; a nonzero
profile/high byte permits the other four but still rejects Farm/Forest.
Bit80000000 disables this particular active-marker equality. Keep this raw
predicate rather than inferring protection merely from the displayed name.

Monsoon Jungle4, Uranium Flats8 and Garland Crater1 alone do not satisfy that
predicate and add **no direct rejection** for these commands. Their generated
elevation/rockiness, actual improvements, or energy-yield effects can still
affect the ordinary guards above. In particular the Solar inquiry calls the
real energy producer, whose landmark bonuses remain live inputs. Landmark
name/render metadata is not itself a reason to require bare flat terrain.

Source scope is the complete command with its bounded action-switch decoding,
the exact prerequisite and Solar-energy helpers, and already frozen active
action add/remove records. No new climate or other-action survey is included.

## Solar inquiry: exact mode1 / Base -1 adapter

Follow-up 2026-11-12. The actual command calls
`4fcde0(owner,-1,x,y,1)`, before installing or removing any improvement.
For ordinary land, this means:

* Keep actual improvement precedence: Monolith/Borehole/fungus/forest branches
  are still tested before ordinary prospective elevation. In particular a
  forested square is evaluated **as existing forest**, not as the cleared
  post-completion Solar square. Rockiness is not a separate energy penalty.
* In the ordinary other-land branch, nonzero mode enables `max(class-2,0)`
  even without actual Solar/Echelon flags (`4fd17f..4fd193`). It **does not
  set an actual Solar bit**. The later adjacent-mirror scan requires actual
  Solar40 (`4fd195..4fd19c`). A normally admitted new Solar command lacks
  that bit, so **do not add adjacent-mirror energy to this inquiry**.
* Land river, effective energy resource and the eligible landmark point
  remain in the common calculation (`4fd239..4fd25d`). Faction **current
  ECONOMY95d3f4+owner*20bc still applies**: add1 when it is>=2
  (`4fd28a..4fd2ab`). Do not replace it with zero merely because Base=-1.
* Base=-1 skips Hybrid Forest in the forest branch (`4fd111..4fd11d`),
  Merchant Exchange and Golden Age (`4fd25e..4fd288`), and both Base-status
  event modifiers Heat Wave/Cloud Cover (`4fd30d..4fd313`). This is explicit
  negative-Base admission, not a lookup of the closest or selected Base.
* Mode1 **clamps pre-event energy to0..99**, stores that clamped value in
  scratch9186ac, and jumps over the ordinary energy-technology cap and
  removed-energy scratch update (`4fd2ac..4fd2d2`). Thus it is not exactly
  equivalent to merely supplying `energyTechnology=true`: the normal
  uncapped ordinary energy path has no general0..99 clamp.
* Then actual global dust95a298 subtracts1 if the current value is nonzero
  (`4fd334..4fd341`). Finally the global solar modifier95a2a4 multiplies by3
  when its selected bit is active: choose bit2 while upkeep9445fc!=0,
  otherwise bit1 (`4fd342..4fd35f`). There is **no post-event clamp**.
  Preserve this selected global input, not an inferred Base Solar Flare flag.

The command allows the action only when the final returned energy is **not
zero** (`4e8705..4e8707`). For ordinary lowest-band clear land at ECONOMY<2,
no river/resource/landmark, prospective1 minus active dust becomes0 and rejects.
At ECONOMY>=2 it becomes2 before dust, then1 and allows. Solar tripling happens
after dust and cannot revive zero. Forest rule1 with no bonuses similarly can
be reduced to0; future elevation yield after removing that forest does not
replace the inquiry's actual forest branch.

For a pure helper adapter, provide an explicit prospective-elevation/mode
input; keep actual Solar presence false for this new command, Base-only
effects absent, current owner ECONOMY and ordinary resource/landmark/global
inputs live, skip the technology cap, clamp before dust/tripling, then test
the returned nonzero result. Reusing `solar=true, energyTechnology=true` is
exact only if the adapter also excludes the artificial mirror scan and
implements that mode clamp/order. Do not mutate actual tile flags or award
completion effects merely to evaluate admission. This boundary is source-only;
no new matrix was run.
