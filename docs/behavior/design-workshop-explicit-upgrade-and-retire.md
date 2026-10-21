# Workshop explicit Upgrade7 and Retire0: human caller and engine contracts

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
**33 focused complete dispatcher controls pass**, plus two bounded receiver
call fragments and four quote scalars. All Upgrade Design/Faction/Base/Unit/
history buffers match the independent
[reference](workshop_upgrade_reference.py), including unchanged bytes.
Compact results: [cases](design-workshop-upgrade-retire-cases.json).
No visual matrix or production changes. Private additive freeze:
`tmp/reference/independent-workshop-upgrade-retire-evidence-20261021/`.

The human Upgrade action uses the **selected stored design**, not the unsaved
component draft. It quotes from cached unit counts, converts own matching
units and own Base production references, and deactivates a custom source.
It does **not** call the retirement/history helper. Direct Retire is a separate,
confirmed destructive action with different owner and queue behavior.

## Upgrade caller admission and targets

Exact Retire0/1/2-unit prompt and multiple-target chooser geometry, fonts,
row values and button positions are now measured in the
[modal layout supplement](design-workshop-retire-upgrade-modal-layout.md).

Action7 enters442bdf in442600. Its first call is
`545560(2,-1,-1,0,-1,-1)`, this947728. Nonzero return exits before building
the target list. This helper immediately returns0 when949308 is zero. Its
network branch coordinates the connection's outstanding operation state;
it is not a technology or price test.

After admission the caller clears91872c, substitutes the selected stored
name, loads SCRIPT `WHICHUPGRADE2`, and scans only the owner's **64 custom
slots**, in ascending global design-ID order. Source can be intrinsic or
custom; a target cannot be an intrinsic row through this chooser.

For each candidate at442c4a..442e5b:

1. Exclude the source ID, inactive flag1-clear entries, and entries with this
   owner's exclusion bit at design+2c.
2. Require the **same chassis ID**, not merely same triad or speed.
3. Require matching ability bit8000 hex (artillery). Other ability differences
   are not checked by this scan.
4. Compare stored **plan byte+2a**: identical passes; different passes only
   when both values are<=3. Do not substitute group byte+2b.
5. Let OW/NW be signed raw weapon strengths. If OW!=0, require OW<=NW.
   If OW==0, require the **same weapon ID**, not just another zero-strength
   equipment package.
6. Let OA/NA be signed raw armor strengths. If OA<=NA, pass. Otherwise reject
   NA==1. For the remaining armor decrease, allow noncombat OW==0, or require
   NW>OW. Thus stronger offense can compensate for some armor reduction,
   but cannot justify dropping to strength1.

No target reactor comparison, general ability-compatibility call, prerequisite
test or displayed-cost comparison occurs in this scan. Those omissions do not
authorize constructing otherwise illegal designs; these are retained registry
entries. Signed comparisons matter for PSI. Do not replace the scan with a
generic "strictly better unit" predicate.

Each admitted row contains stored name, stat formatter5932d0 output and the
existing prototype marker when the source is prototyped but the target is not.
Stored row value is the **global target ID**. Zero targets shows NOUPGRADE;
one bypasses the chooser; multiple opens61b9d0; negative modal result exits.
Two valid targets65/66 and cancellation/selection66 are controlled. Seven
additional narrow controls distinguish chassis, artillery, plan, noncombat
identity, negative weapon strength, strength1 armor rejection and compensated
armor decrease. An unsaved PSI draft still upgrades the stored source tuple.

## Quote is stored target cost plus positive component-cost differences

Helper `504eb0(owner,target,source)` returns per-unit quote rows. All three
record operands are retained IDs; Workshop mineral multiplier, INDUSTRY,
Base mineral factor and displayed COST are not queried here.

Start B from target's unsigned **stored cost byte+29 hex**. If targetID>=64
and target flag4 (prototyped) is clear, execute both original operations:

```text
B = B + trunc((B+1)/2)
B = B * (B+1)
```

Then add three nonnegative differences:

```text
max(0, target.weaponCostByte - source.weaponCostByte)
+ max(0, target.armorCostByte - source.armorCostByte)
+ max(0, target.chassisRawSpeed - source.chassisRawSpeed)
```

Weapon/armor values are unsigned definition **cost bytes**, not combat
strengths. There is no refund for a decrease. Reactor/ability changes have
no additional direct delta term; their cost can already be in stored B.
Do not apply the nonlinear prototype transform to an intrinsic target.

If Nano Factory host Base global95a074 is nonnegative and that Base's owner
equals the upgrading owner, truncate the **per-unit sum** in half. Then total
energy quote is:

```text
perUnitQuoteRows * cachedOwnUnitCount(source) * 10
```

The count is the unsigned byte at `95d9fc+owner*20bc+sourceID`, not a new live
roster count and not a production-reference count. Halving precedes both
count multiplication and the fixed10. The exact same quote is recomputed by
the apply helper; the caller does not pass an escrowed total.

The ordinary control has target stored B=3, weapon difference1, armor
difference1 and two cached own units: **100 energy**. Unprototyped target:
B3→5→30, plus2, for **640 energy**. Owned Nano Factory: floor(5/2)=2 rows,
for **40 energy**, not50. A foreign Nano host leaves100. Scalar controls also
verify B1 prototyped gives3 rows, owned Nano gives1, B1 unprototyped gives8,
and B2 unprototyped gives14.

## Human confirmation, affordability and application

`504fe0(owner,target,source,0)` first rejects when959fc8 bit1 is set and bit8
is clear (hex bit masks1 and8). Human-mask membership uses959ff0; the human
branch additionally requires owner==9430d4. The AI branch has separate budget
heuristics and is not generalized by these human controls.

For a positive human total, compare against faction energy at
`95d3c8+owner*20bc` (Faction+220 hex). Insufficient funds shows
UPGRADEPROTOCOST or UPGRADEPROTOCOST2, selected by target prototype flag,
and returns0. Otherwise, unless91872c is already nonzero, ask UPGRADEPROTO
or UPGRADEPROTO2. Answer0 declines; positive accepts; answer>1 also sets91872c
to skip later questions. Action7 resets that latch on entry. Total, available
energy and cached count are substituted into the prompt. If total is zero,
the human branch bypasses both affordability and upgrade confirmation.

For a local application call **504cc0(owner,target,source)**. It:

* Recomputes the quote and subtracts the total from energy. This core helper
  does not repeat the affordability/admission checks.
* Scans actual unit count959fd0, ascending physical slots. Only units whose
  owner byte+e equals owner **and** design word+a equals source are converted.
  Update design word to target; decrement cached source count and increment
  target count **per converted unit**, with byte wrapping.
* Adds one morale level, capped6, when target gains High Morale bit4000 hex;
  removes one, floored0, when it loses it. No change when both/neither have it.
  Native controls include morale2 and6. No damage, moves, orders, home Base,
  position, identity, roster count or other unit bytes change in these controls.
* For each **own** Base with queue count>=0, replace sourceID with targetID
  at current production+50 and every pending entry through queueCount+1
  elements inclusive of current. Keep order, count, minerals and retool state.
  Foreign Base queues remain byte-exact. Production-reference cache is not
  transferred or recounted here.
* OR target prototyped flag4 only for a custom target and a **nonzero charged
  total**. Do not mark it prototyped solely because actual units were found.
* Clear active flag1 on a **custom source** unconditionally, even with no
  units/zero charge. An intrinsic source keeps its active flag. Do not clear
  its other flag bits, record contents or name.
* Request its documented map/notification refreshes. These presentation
  boundaries are supplied in the engine controls.

There is no595ac0 call, unit deletion, history-ring write, Stockpile conversion
or production retool charge in this conversion. These differ from Retire.
The foreign same-design unit is preserved, while both own units convert.

Stale-cache controls establish the split deliberately: cached0 with two actual
own units charges0, still converts both, and decrements the byte through0 to254;
cached3 charges150 while converting only two and leaving source count1. The
zero-charge target is not newly prototyped. These are exact authority/edge
observations, not a recommendation to create stale counters in the port.

On success the dispatcher selects the target, calls4490b0(target,0), rebuilds
5bdb60(owner), recenters the saved strip when found, and requests full Window+f8
refresh. Failure/decline keeps source selection and engine buffers unchanged.
If cached source count was zero **before** the upgrade, caller442f85 shows
**YESUPGRADE**, an informational "Upgrade complete!" template. Its return
value is ignored. This is **not** an optional source-retirement confirmation;
source deactivation already happened inside504cc0.

## Direct Retire0 is separate

Action0 starts442900 without committing the unsaved draft. It reads the same
cached own source-unit count. Nonzero uses confirmation **RETIRETYPE**, with
the count substituted; zero uses **RETIRETYPE2**. Decline returns without
engine mutation. Accepted single-player invokes595ac0(selectedID,owner).

That helper's existing removal/history contract is reused and newly verified
through the direct caller. For a human owner it matches physical units by
**raw design ID without another owner test**, unlike Upgrade. Three same-ID
units belonging to two factions are all removed. An intrinsic Scout control
also removes all same-ID own/foreign units, while the intrinsic design remains
active and its Base queues are not scrubbed by the custom-design branch.

For a custom source, it records old eight component bytes in the owner's
retirement ring and advances the cursor once; removes matching live units
through5d8c40; clears source active flag1; changes any Base's current matching
production to Stockpile-38 and removes matching pending entries. This queue
surgery has **no Base-owner filter**. It does not promote the next pending
choice or alter minerals/retool baseline, and does not recount production caches.

Queue removal increments its index after shifting. Thus adjacent matching
pending entries can be skipped: the controlled `[current64,pending64,pending64]`
becomes `[current-38,pending64]`, count1. This is a reproduced original edge,
not a fully filtered queue. Empty retirement still records history/deactivates
the custom source, and zero cached count does not itself prove no live units.

After acceptance the caller reselects the same raw ID via4490b0(id,0), rebuilds
the browse list, recenters when found, and requests a full repaint. It does
not select the Upgrade destination or allocate a replacement design.

## Network sender/admission and receiver boundary

The network flag is949308. The new sender controls supply network admission
results/transport/wait boundaries; they do **not** claim a live multiplayer
round trip. Receiver controls execute the original decoded-argument call
fragments and real engine helpers; packet validation/log/RNG outer dispatch
remains outside those fragments.

Upgrade's545560 gate runs **before** candidate enumeration. In network mode
the source checks connection+1bac outstanding state; if already busy with
+1bc4 clear it reports the busy condition and returns1. Otherwise it starts
the coordinated operation via command2210 and waits for connection+1b70,
servicing network/events; this is not a local rule permission bit. The whole
transport interlock is source-traced, not faked as a gameplay mutation.

After a confirmed human Upgrade in network mode,504fe0 emits:

```text
5aa510(2411 hex, 0, owner, targetID, sourceID, 0)
```

It then uses545250 to coordinate delivery. It does not also call504cc0 locally
in that branch. The supplied no-delivery sender control changes UI selection
on the success return but preserves engine arrays. Real receiver
`5493c0@54bc11..54bc25` reads decoded packet fields+10 owner,+14 target,+18
source and calls504cc0. The new receiver fragment's complete engine arrays
match the same independent conversion reference.

Retire confirms **before** its network wait gate54e820(25 decimal,19 hex).
This helper normalizes a nonzero argument below100 to100, polls5459c0 while
servicing5347c0, and on its elapsed/spin threshold can ask whether to abort;
nonzero return blocks submission. It is a wait/admission operation, not an
assumed25ms delay. Accepted submission emits:

```text
5aa510(2415 hex, 0, sourceID, owner, 0, 0)
```

Then546240(0) and54e8c0 coordinate completion/clear status, before the usual
UI reselect/rebuild. The sender does not additionally retire locally. The
original decoded receiver call at54bc49..54bc59 supplies packet+10 source,
+14 owner to595ac0; the focused fragment reproduces removal/history changes.
Both blocked and submitted sender paths are checked separately.
