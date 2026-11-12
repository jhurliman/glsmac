# Perimeter Base defense and ordinary native Worm stacks

2026-11-11 static continuation for pinned stock SMAC. Reuse the executed
[component/selector receipts](early-ground-combat-causal.md),
[Base assault receipt](scout-defending-base.md),
[PSI/Sensor stages](base-entry-psi-and-movement-static.md), and
[stack casualty source contract](early-ground-retreat-and-stack-casualties.md).
No new runtime matrix. The following extends the stated source scope without
claiming newly replayed fortified or multi-Worm battles.

## Conventional Infantry/Rover against Perimeter4

Admission: ordinary ground combat, positive-owner conventional Fission
Infantry or Rover attacker, ground defender in an actual Base, no special
combat abilities. Fortification comes from the **defending Base record's
facility4 bit**, including a free Hive Perimeter already granted to that Base;
this consumer does not have a separate Hive combat multiplier.

`516390` obtains the defender's component/morale intermediate from `5160d0`.
For this conventional Base profile:

```
D = defenseHelperIntermediate
D *= terrainFactor                         # 51766c..51767a; Base factor 2
fortificationFactor = 2
if Perimeter4 or owner-wide CitizensDefenseForce:
    fortificationFactor = 4
if Tachyon5:
    fortificationFactor += 2
if fortificationFactor == 2:
    D = trunc(D * (100 + intrinsicBasePct) / 100)
D *= fortificationFactor                   # 517cfe..517d08
D = trunc(D * (100 + sensorPct) / 100)      # later, if Sensor qualifies
```

The exact facility4 query/bit test is `517989..5179c5`. If absent, the
project substitute at `5179c7..5179f2` reads host index95a030 (project facility
ID44, Citizens' Defense Force) and compares host ownership to the defender's
owner. Either route sets **ESI=4 at5179f9**. This **literal instruction** is
the amount producer: there is no separate parsed/frozen RULES percentage for
Perimeter's amount and no read of a facility description's percentage.
The displayed percentage derives from `50*(factor-2)` at517d1d..517d24.

Tachyon facility5 is checked at `517a0a..517a46`; if present, **add2 at517a4e**.
It adds to the selected factor rather than applying a second percentage:
Perimeter+Tachyon gives6. A supplied Tachyon-only Base gives4 in this literal
consumer, although normal build admission requires Perimeter or its project
substitute. The no-ability branch with factor>2 jumps from517a5e to517b26,
**bypassing** the intrinsic percentage block517a8c..517b22.

Intrinsic Base defense remains the separate frozen RULES DWORD **9ba4a8**,
installed25; it is used only by the unfortified fallback in this profile.
The late Sensor rule is **9ba510**, installed25, with its existing qualifying
scan and one-bonus limit. It follows the final fortification multiplication;
do not fold Sensor into the factor or combine it additively with intrinsic25.

Source-derived neutral morale1/Gun/Scout examples, without partial-movement
reduction or other abilities/modifiers:

| Defending Base facilities | Defense without Sensor | With qualifying Sensor25 |
| --- | ---: | ---: |
| Neither | 280 | 350 |
| Perimeter4 | 448 | 560 |
| Perimeter4 + Tachyon5 | 672 | 840 |

The intermediate for these examples is56: terrain2 and ordinary final factor2
give224 outside the Base percentage. Perimeter does not multiply the already
intrinsic-adjusted280 by2. Conventional Infantry's separate attack-vs-Base
rule9ba4f4 still runs later at517b6b..517c90: in the same neutral profile its
attack is280; ordinary Rover does not meet the raw-speed1 gate and remains224.
Special ability20000 can cap the fortification factor at2 (`517b26..517b54`)
and is outside the admitted no-ability slice; do not erase that separate path
when adding more abilities.

**PSI remains different:** nonzero PSI context jumps516fd2 ->517d7b, skipping
this Perimeter/Tachyon composition. It uses intrinsic25 on the intermediate
before final x4, then the late qualifying Sensor percentage. Do not apply
Perimeter to a conventional attacker fighting a native PSI-armored defender
merely because that attacker's weapon is conventional.

## Signed native weapon in defender selection

`5187e0` performs **MOVSX ECX, byte [weaponRecord+power] at518b23**, then
subtracts that signed value at518b41. Stock Mind Worm's raw weapon power is
**-1**, not255. Therefore the subtraction adds1 to its score0. There is no
negative-weapon rejection in this scoring block. Stock Worm plan1 takes the
ordinary plan<8 scale branch (`518b43..518b4d`).

For the plain, same-owner ground military/no-priority/no-cargo profile, retain
the existing staged selector exactly:

```
Aref = actual 515ec0(attacker, initialSeedDefender, 0,0,0)
for candidate in linked defending stack:
    A,D = actual composedStrength(attacker,candidate)
    Dh = trunc(D * candidateRemainingHealth / candidateMaxHealth)
    ratio = trunc(Dh * Aref / A)
    score0 = trunc(ratio / 8) - signedRawCandidateWeaponPower
    score = score0 * 32768 + currentDenseCandidateID
choose greatest score
```

Aref is queried **once before traversal** (`5187f3..518803`), not recomputed
from each candidate or replaced with a fixed conventional component. Each
candidate receives its own complete `516390` call at518a68. Ordinary ground
PSI admission is **attacker weapon<0 OR candidate armor<0**, using signed
component powers (`516419..51648f`). Candidate weapon<0 alone does not select
PSI; stock Worms also have negative armor. Thus a conventional attacker can
score an armored conventional candidate conventionally and a Worm candidate
with PSI ratios within the same traversal. A PSI-weapon attacker selects PSI
against both. Keep component-stage faction PSI truncation, mode1/native morale,
retained attack PLANET, Base/Sensor differences and later modifiers in their
existing order; no conventional weapon/armor magnitude multiplier carries
into PSI ratios. The pre-turn15 owner0 width reduction belongs to later battle
setup (`51bd4b..51bd85`), not an extra selector-score health multiplier.

Follow prev links to head, then next links; final score includes the **current
dense ID**, so equal unflagged candidates select the higher ID. There is no
newborn-Worm special tie rule. Special plan/domain/priority branches including
Unit flag80 and the adjustment518d8f remain outside this plain profile.

## Multiple owner0 land Worms after an attacker victory

For ordinary ground combat into an open unfortified square, no Base, no
Colony8, no cargo/Probe/Artifact exception, the source follows the existing
open-stack stages, not the protected selected-only route:

1. `51e92d` prepares the stack, then walks linked members. The defeated chosen
   defender is made lethal. Other stock Worm plan1 members are not in exempt
   plan set12/8/11/10; they receive collateral and activity flag4.
2. `51ea8f..51eadf` adds **3*reactor** damage: use the affected member's
   reactor if attacker weapon is negative, otherwise attacker's reactor.
   In the requested Fission-versus-stock-Worm slice this is3 either way.
   Preserve native byte arithmetic rather than substituting half-health.
3. The removal walk **explicitly tests owner0**, not just lethal damage:
   a positive-health member falls through to owner test51ebbd..51ebc5;
   nonzero owner skips the casualty call, **owner0 reaches519db0 at51ebd9**.
   Consequently all remaining owner0 Worms in this admitted losing stack are
   processed through the casualty handler even if collateral left7 health.
   Do not leave them alive merely because damage<10.
4. Use the actual casualty/deletion transaction for each member. Each deletion
   compacts dense IDs; `51ebe5..51ebee` repairs the saved next member and
   attacker reference before traversal continues. Do not bulk-delete by stale
   IDs or score again using IDs captured before deletion. The existing reward
   accumulator admission/formula remains attached to each519db0 call; there
   is no new unconditional per-battle bounty rule here.

The owner0 removal condition does not waive protected-stack routing. A Base,
Bunker800, land Colony or other earlier exception can prevent reaching this
open removal walk. These statements concern defeated defender stacks; they do
not assert that every surviving native stack anywhere is removed after combat.

## Live first50 connection

The shared ground strength helper now carries explicit Base fortification
factor2/4/6, preserving the intermediate intrinsic rounding only for factor2.
The live land battle caller reads actual facility4/5 flags and applies Sensor
last; PSI keeps its distinct path. Perimeter4 is admitted for production.
Defender traversal uses retained linked order and signed native weapon power.
The open Worm-stack casualty transaction retires every owner0 member in source
order and applies the existing reward once per casualty. Prepared retirement
payloads preserve compaction while recording lethal/collateral damage and
activity bit4. Positive collateral writes that activity bit in the retained
status DWORD, preserving the separate original flag word.
