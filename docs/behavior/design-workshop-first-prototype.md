# Saved Synthmetal design: production and first prototype

2026-10-20, original stock SMAC executable fingerprint from
[the Workshop first path](design-workshop-first-path.md).
This closes the human Infantry / Hand Weapons / Synthmetal / Fission /
no-ability production path with independent executable controls.

Private frozen evidence:
`tmp/reference/independent-workshop-first-prototype-evidence-20261020/`.
Manifest SHA-256:
`bb6e35e4cfa82d019c529a39fea75a8e49e856907ccdfcc86b518f5ea07e7040`.
Both earlier Workshop bundles were verified unchanged during freezing.

Checks passed: **927 complete cost queries, 288 complete eligibility queries,
six complete Base production/upkeep returns, and one admitted unit-limit
boundary**. An independent [history reference](workshop_prototype_history_reference.py)
matches all 26,624 design bytes in each of the seven completion/boundary cases.
Inputs and native outputs are separate in the receipts. No port source changed.

## Cost rows: use the design owner, then the queried Base

`5d9810(design, baseOrMinusOne, optionalPrototypePresentOut)` begins with the
stored ordinary cost at design byte `+29`. This exact design stores 2 rows.
Predefined IDs below 64 and custom designs with flag `+30 & 4` bypass the
prototype surcharge. Do not interpret active bit 1 or custom-name bit 2 as
prototype completion.

For an unprototyped custom design:

1. Owner is the design bank, `design / 64`, not the current player or queried
   Base's owner.
2. Resolved free-prototype capability is definition flags
   `[95a9d0 + owner*59c] & 0x200`. It waives the surcharge.
3. The owner's **stored difficulty**, `[95d1b0 + owner*20bc]`, also waives it
   at 0 or 1. Global difficulty `959fcc` is not this guard.
4. Otherwise take land/sea/air extra percentage from `9ba480/9ba484/9ba488`,
   using the chassis domain. Installed values are 50/50/50.
5. Recompute the component cost through `5bd4b0` with abilities omitted.
   Surcharge is `(basis * percentage + 50) / 100`, truncating the positive
   tested values. Do not apply the percentage to an ability-inflated stored
   cost. See [the independently checked full cost formula](unit-design-and-prototype-costs.md).
6. If an actual queried Base has Skunkworks, facility ID 10, surcharge becomes
   zero. Its local presence bit is Base `+8d & 4`. A no-Base query (`-1`)
   cannot borrow Skunkworks from the currently selected Base.
7. Return stored rows plus surcharge. The optional out value means **nonzero
   surcharge**, not “this design has not been prototyped.” Small configured
   percentages can round surcharge to zero while prototype bit 4 stays clear.

For this design and neutral human mineral factor 10:

| State/context | Rows | Minerals | Surcharge-present out |
| --- | --- | --- | --- |
| Stored difficulty 2..5, no exemption | 3 | 30 | 1 |
| Stored difficulty 0 or 1 | 2 | 20 | 0 |
| Free-prototype capability | 2 | 20 | 0 |
| Queried Base has Skunkworks | 2 | 20 | 0 |
| No Base, selected Base has Skunkworks | 3 | 30 | 1 |
| Already prototyped | 2 | 20 | 0 |

These are cost **rows**, multiplied by the existing actual mineral factor at
the production consumer. Faction INDUSTRY and AI/global difficulty effects in
that factor remain separate; do not hard-code 20/30 minerals for every faction.
The seven-bank grid checks owner resolution, not a claim that all seven stock
faction identities have identical natural capabilities. Free-prototype state
is supplied as the resolved flag. Installed Sparta declares FREEPROTO; bind
the existing capability parser/state rather than hard-coding a faction slot.
The additive [FREEPROTO parser controls](faction-freeproto-parsing.md) now
prove that producer: keyword presence ORs the capability, ignores its value,
and consumes no generic bonus-record slot. The original frozen bundle remains
unchanged.

The cost grid includes all seven banks, all six stored difficulty values,
prototype false/true, ordinary/Skunkworks/no-Base contexts, mismatched Base
owner, explicit global-versus-stored difficulty controls, resolved free flag
controls, unequal domain percentages 20/70/110, and rounding boundaries.
Infantry, Foil and Needlejet Synthmetal variants each have 2 raw rows in these
controls. They isolate domain policy, not natural technology availability or
the legality of building every domain at the fixture Base.

## Eligibility: prototype completion is not an admission gate

Complete `5d1eb0(design, requester, baseOrMinusOne)` executes in the 288 new
controls. The saved active own Synthmetal design is eligible **before** it is
prototyped. Prototype state affects price, not this admission predicate.

The guards are: active record; no requester bit in design exclusion byte
`+2c`; built-in prerequisite validation when ID <64; Colony-plan/no-Colony
rule; sea adjacency when a Base is supplied; and custom bank ownership.
Custom records do not pass through the built-in prerequisite guard. The new
controls explicitly set their stored prerequisite to either None or Disable
and obtain the same custom eligibility result. Component technology belongs
to Workshop admission, not a new extra guard in this consumer.

The new matrix varies active/inactive, exclusion bit, owner/foreign requester,
stored prerequisite, Base/no-Base, and land/sea/air chassis. For the sea
variant, adjacent sea passes, all-land fails, and an isolated sea **center**
tile still fails: the center is not one of the eight queried neighbors.
No-Base skips that coastal test. The existing
[complete eligibility contract](production-queue-and-build-eligibility.md#unit-eligibility-helper)
supplies built-in, Colony, technology-definition, wrapping and edge controls.

## First completion: mutation order and newborn morale

The native producer creates design 127 with complete `5bd4f0`, then supplies
a short self-authored fixture name and named flag 2. Thus starting flags are
`0x0003`: active + named, unprototyped. A prior real unit of this design is
allocated at another square to test preservation; two other Bases carry
current/pending references as queue decoys. Reference counters are rebuilt
by executing `41e3dc..41e41d` for each Base with its actual owner.

For the normal first production, the producing Base starts with 29 minerals
and the actual resource pass supplies one. Native cost admission charges 30.
The complete `50c760(0)` Base return executes production, allocator, training,
queue advancement, outer baselines and later resource calculations.

Relevant order within production `5059b0`:

1. Successful cost/production admission reaches `506275`.
2. For custom design IDs >=64, `506296` ORs **word** flag 4, preserving all
   other bits. It derives the first-completion boolean from the **old** bit 4.
   Here flags change `0x0003 -> 0x0007`.
3. Only when old bit 4 was clear, call `595db0(owner, completedDesign, 0)`.
4. Subtract the previously admitted cost and apply normal carryover/baseline
   rules. Do not recalculate this build's debit using the newly cheaper price.
5. Allocate/place/home the new unit.
6. Compute stored morale as the native training-helper result, plus one for
   High Morale if present, plus one for the first-completion boolean, plus
   the normal one; clamp to 0..6. These controls add no training or High Morale.
7. Continue normal production flags, queue advancement and Base aftermath.

The first newborn has stored morale byte `+24 = 2`; a repeated production
has 1. This is not a retroactive bonus to every unit with that design.
The first existing Synthmetal unit remains byte-identical in every case.
Do not give a prototype morale bonus merely because unit-produced count is
zero: the gate is old prototype bit 4. Built-in designs take the separate
non-custom branch with first-completion boolean zero.

## Charge exemption and prototype state are independent

The six complete returns establish:

| Case | Charged minerals | Old prototype bit | First newborn morale | Later cost rows |
| --- | --- | --- | --- | --- |
| Ordinary first | 30 | clear | 2 | 2 |
| Ordinary repeat | 20 | set | 1 | 2 |
| Skunkworks first | 20 | clear | 2 | 2 |
| Free-capability first | 20 | clear | 2 | 2 |
| Stored difficulty 1 first | 20 | clear | 2 | 2 |
| Already-prototyped control | 20 | set | 1 | 2 |

Waiving the extra mineral charge does not pre-mark the design or remove its
first-completion morale bonus. Conversely, component history can mark a design
prototyped without that exact design ever having been manufactured.

## History propagation: cover components, not an identical whole design

The human `595db0` pass scans the owner's 64 custom slots, considering active
unprototyped candidates. For each, it accumulates three independent booleans
from available active built-ins and active **prototyped** own designs:

* Weapon: exact weapon ID, or positive candidate offense covered by at least
  that offense in a history weapon.
* Armor: exact armor ID, or positive candidate defense covered by at least
  that defense in a history armor.
* Chassis: exact chassis ID, or history chassis in the same domain whose
  chassis speed byte is at least the candidate's speed byte.

The three facts can come from different history records. Weapon and armor
coverage are accumulated independently of the chassis-domain comparison.
Built-in history uses the owner's actual technology predicate; custom history
requires flag 4. Reactor rating and ability mask are not inspected in this
scan. Advanced reactor consequences are source-traced, not newly exercised.

If all three booleans are true, OR word `0x0104` into the candidate. Preserve
the extra `0x0100` bit as observed; this handoff does not assign it a UI meaning.
Do not replace the word with 4 or rewrite the candidate's components/name/cost.

The fixture bank contains these deliberately declared comparison designs:

| ID | Difference from completed design | Flags after first completion |
| --- | --- | --- |
| 127 | Completed named Synthmetal design | `0003 -> 0007` |
| 64 | Same components, separate design | `0001 -> 0105` |
| 67 | Same components plus Clean Reactor ability | `0001 -> 0105` |
| 65 | Laser / no armor | remains `0001` |
| 66 | Synthmetal Speeder | remains `0001` |
| 68 | Plasma Steel Infantry | remains `0001` |
| 128 | Identical tuple in foreign bank | remains `0001` |

The comparison designs isolate history behavior; they do not claim legal
availability of Clean Reactor, Laser, Speeder or Plasma Steel at this start.
The independent Python implementation reproduces the entire design pool for
all six complete returns and the admitted allocation-failure control.

## Existing units, queues, record fields

History propagation changes **design flag words**, not unit design IDs or
component tuples. The completed design retains all 52 bytes except its flag
word; inferred designs retain all bytes except their flag words. The complete
26,624-byte comparison verifies the absence of other design mutations.

The other same-owner and foreign Base records remain byte-identical, including
their current targets, pending IDs, mineral stores and baselines. Because their
references point to shared design records, subsequent cost queries see the
new prototype status; there is no need to replace their queue entries.
The foreign queue fixture intentionally contains otherwise ineligible IDs as
preservation decoys, not a claim that the chooser admits those entries.

The producing Base normally advances its one queued Scout entry: target `+50`
becomes 2, pending count `+4c` becomes 0, and the outer baseline item `+44`
becomes 2. Current minerals `+40` and baseline minerals `+98` become zero.
The owner's design-127 production-reference byte changes 3 -> 2, preserving
the two references in the other same-owner Base. Those references include
queued duplicates. Do not decrement all design references on prototyping.

The new unit has design `+0a=127`, owner `+0e=1`, coordinates (16,16),
home `+2e=0`, damage/order/spent zero, and the normal production flag
`+04 & 0x2000`. The first/new repeated morale difference is above. Existing
same-square units' stack-link words can change through normal insertion;
that is not an upgrade. The pre-existing Synthmetal unit at the other square
is completely unchanged. On the repeated pass, the additional homed unit also
changes normal support/mineral-surplus fields in the later resource pass;
this is separate from prototype history.

`checked-summary-private.json` supplies **every changed byte offset and complete
input/output record** for all changed Designs, Bases, Units and Factions in
each case. Native before/after buffers also cover all 512 Designs, all eight
Factions (67,040 bytes), three Bases and eight Unit slots. Ordered writes
include native PCs, distinguishing prototype stores from surrounding upkeep
caches/counters. Global unit counts are recorded separately. Do not collapse
all Faction aggregate changes into the prototype-history operation.

## Allocation-limit edge and execution boundary

An additional sparse unit-count-2048 control executes `506275..50639d`, ending
at the allocation-failure UI boundary. It proves that the current prototype
bit and inferred-design flags are already written, and the 30 minerals already
debited, before allocator refusal. No unit record is created or changed.
Later failure UI/refund/aftermath is not executed by this admitted control;
do not claim that a failed allocation is an atomic rollback.

The six successful cases execute original gameplay helpers and return from
`50c760`. They inherit the accepted headless movement/visibility services and
explicit notification/destructor/deallocation boundaries. The three-Base
fixture additionally reaches a path-query tail that reads a foreground-view
presentation flag. Only during `5b2db9..5b2dd1`, a guest view object supplies
that flag as zero; the original view pointer is immediately restored. No
cost, history, path result, resource, queue or allocator helper is substituted.
This is controlled engine execution, not a naturally booted live-game capture.

The successful lifecycle controls are **human** owner 1. Cost queries cover all
seven banks. `595db0` has an additional nonhuman branch after `595fd7` that can
apply AI obsolescence/replacement policy; these preservation conclusions must
not be generalized to that unexecuted branch. Full Workshop frame capture
remains the separate documented runtime gap.
