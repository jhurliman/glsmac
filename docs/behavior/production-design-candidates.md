# Production unit choices from retained designs

Replace the fixed Scout/Former/Colony list with **retained raw design
records**, ordered by`5bda00(owner)` and filtered by
`5d1eb0(rawId,owner,currentBase)`. This exposes saved named customs without
inventing a separate Workshop-produced unit category. Read the actual
record's stored name and tuple, including mutable intrinsic overlays.

Scope: playable owners1..7, valid raw IDs0..511, valid component IDs.
Record offsets below are hexadecimal.
The existing [first path](design-workshop-first-path.md),
[1,440 eligibility controls](production-queue-and-build-eligibility.md),
and [first-prototype controls](design-workshop-first-prototype.md) remain
authoritative. Three new bounded native list controls distinguish ordering,
stable ties and a changed intrinsic Scout. [Scalar fixtures](production-design-candidates-cases.json).

## Exact candidate order: `5bda00`

Enumerate these128 raw IDs, in order:

```text
0..63, then owner*64 .. owner*64+63
```

For each ID, assign the following signed32 sort key:

```text
inactive or unavailable intrinsic ID<64: 999999999
inactive own custom ID>=64:             888888888
otherwise:
    ((rawPlan*16 + chassisDomain) << 18)
  + ((32 - attackEstimate(rawId,owner)) << 9)
  + ((8 - chassisBaseSpeed) << 6)
  + 8 - doubledDefenseEstimate(armorId,owner)
```

Active means record`+30 & 1`. Intrinsic availability uses the actual stored
prerequisite word`+32` through`5d17b0`; custom records do not check that
word. Raw plan is record`+2a`, **not category`+2b`**. Domain and unsigned
base speed come from the actual chassis definition. Attack is
`592d80(rawDesignId,owner)`; doubled defense is
`592de0(armorId,owner)`—the latter's input is armor ID, not design ID.
These are the [already documented PSI-aware estimates](design-workshop-summary-inputs.md),
not cost, morale, prototype cost, reactor-scaled combat odds, or a raw
negative-power rejection. Preserve native32-bit arithmetic for the key.

Sort all128 entries by ascending signed key, **stably**. Native
`5ccf20` swaps adjacent entries only when left key>right key; equal keys
retain intrinsic-before-custom and increasing-ID enumeration order.
Do not add an alphabetical or newest-created tie breaker.

The published count`94f7a0` is **64 plus the number of active,
technology-admitted intrinsic records**. It includes all64 own custom
slots, even inactive ones. Native sorts all128 entries, then the consumer
uses only the first count entries. It is not a ready-made list of eligible
production choices. Inactive placeholders and obsolete active records
still require the next predicate. The sentinels above are literal source
values, not a directive to sort inactive entries as conventional zero stats.

The builder neither reads the owner's exclusion bit nor examines another
faction's custom bank. Names, named/prototyped bits, active Unit counts,
production counts, current Base location and mineral costs do not determine
this ranking. It does not mutate the retained design records.

## Ordered production predicate: `5d1eb0`

For each candidate in the published prefix, apply in this order:

1. Reject if record active bit`+30 & 1` is clear.
2. Reject if exclusion byte`+2c` contains`1<<requestingOwner`.
3. For raw IDs<64 only, require positive owner and an admitted stored
   prerequisite: None/-1 passes; otherwise require ID0..87, enabled
   definition and ownership. Reject Disable/-2, other negatives,88 and
   out-of-range references. See the [exact technology predicate](technology-token-and-production-prerequisites.md).
4. Reject **raw plan8** when global primary rules`959fa4 & 0x400000` is set.
5. If a nonnegative current Base ID is supplied and chassis domain is
   sea1, require a qualifying adjacent ocean tile through
   `4fa2e0(base.x,base.y,0) != 0`.
6. Intrinsics now pass. Customs pass only when`rawId / 64 == owner`.

The sea test scans the eight immediate doubled-X neighbors, excluding
the center: `(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)`.
Wrap x on wrapping maps, clip x on flat maps, always clip y. Water is
the original altitude code below`0x60` in the tile altitude byte's upper
bits. A land Base next to water passes; an isolated water-center Base
surrounded by land fails. A negative Base argument bypasses only this
coast test. Do not use-1 in the live picker to avoid supplying its Base.
Land/air candidates have no analogous adjacency gate here.

Customs bypass the intrinsic prerequisite check and do not revalidate
component technologies. The new controls give active own customs a stored
Disable prerequisite and no owned technologies; they still pass. Prototype
completion is not an eligibility gate; it affects manufacturing cost.
This helper also does not check name, ability legality, existing Unit count,
2048-unit allocation capacity, or whether the supplied Base belongs to the
requester. Upstream caller/Workshop legality and allocation failure retain
their own contracts.

## Live caller and intrinsic identity

`4a1f50@4a20aa` applies that predicate using the actual current Base ID from
`6aae18`, then copies the admitted record's stored name at`4a20e1..4a210c`.
Keep this order after filtering. Rebuilding the picker after Workshop
returns should reveal any active, nonexcluded own custom that passes it;
do not automatically select it as production or rewrite the queue.

Intrinsic raw2 may contain a modified Scout tuple; raw11 may remain a
separate Rover Scout. Resolve each raw record exactly once. Do not replace
raw2 with a synthetic`starting_scout`, then append the same intrinsic again.
Saved intrinsic exclusion/flags/name/tuple overlays also remain live inputs.
The complete codec restores the retained registry; do not regenerate it
from immutable stock definitions on load.

## Distinguishing bounded controls

The three controlled registries use self-authored names and declared
conventional component rules. They run complete native`5bda00`, its real
stable sorter and score helpers, then actual no-Base`5d1eb0` filtering:

| Control | Candidate count | Eligible raw IDs in order |
| --- | --- | --- |
| Baseline, active saved customs64..68 and foreign128 decoy |70|`66,4,11,2,64,65,3,68,0,1`|
| Intrinsic2 changed to Speeder, same raw identity |70|`66,4,2,11,64,65,3,68,0,1`|
| Intrinsic2 requires unowned technology6 |69|`66,4,11,64,65,3,68,0,1`|

Baseline2/64/65 have equal key12599238 and retain that order. After the
overlay,2/11 have equal key12599174 and retain2 before11. Obsolete own67
remains in the sorted prefix but fails the predicate; inactive custom
placeholders also remain in that prefix but fail. Foreign128 is not
enumerated; directly querying it for owner1 rejects. Directly querying
owner1's64 for owner2 also rejects. All full128 orders, keys and record
preservation checks pass. No new coast/technology/prototype matrix was
needed; those distinguishing cases already execute in the linked receipts.

Private additive evidence:
`tmp/reference/independent-production-design-candidates-evidence-20261021/`.

Manifest SHA256:
`9dbb915069f8a86bfa5b6ed196ba1d1caee06535924d5e3d27320697a643609a`.

## Port integration receipt - 2026-10-21

The retained-registry Base picker uses this order, stored names and the actual
Base-aware prototype cost. Its queued selector accepts eligible saved custom
IDs instead of the fixed Scout/Former/Colony whitelist. Legacy port saves
without a native registry retain their prior limited projection; raw names and
ordering cannot be reconstructed from absent authority.

The live test saves `Factory Guard`, removes its component technology, supplies
a disabled intrinsic prerequisite on the custom record, and selects it through
the actual production UI handlers and authenticated event. Open/cancel are
read-only. Completion uses actual Base upkeep with a controlled mineral
balance, sets first-prototype history/morale, advances the saved queue, and
moves the newborn through the real movement command. An actual Start restore
repeats manufacture/movement to the same normalized world bytes. Test-only
presentation-role registration is supplied after Start, as in the existing
backend fixtures. This is a connected deterministic fixture, not a human-played
campaign or an on-disk save acceptance receipt.

Inactive, owner-excluded and foreign custom rows do not appear. Original
eligibility and current physical-engine support are separate: the picker also
withholds profiles the current Unit factory cannot execute. Infantry/Rover combat,
Former and Colony are currently admitted; sea/air, PSI and ability-rich physical Units remain
unfinished. These omissions are implementation gaps, not original-game rules.
The raw candidate sorter still ranks all native profiles. Manufacturing retains
existing wire roles, while live map/cards resolve the actual tuple and names
through [the ordinary-body integration](live-ground-design-render.md). Complete
shadow/animation and Workshop acceptance remain open.
