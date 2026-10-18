# RE-SMAC-CAPABILITY-BINDINGS-001: exact capability scan inputs

This completes the scan and startup bindings accompanying the
[regional producer](regional-research-summary-producer.md) and
[PSI formulas](psi-capability-estimates.md). **31 complete native capability
controls** pass, plus four separate native startup-dispatch prefixes with
initializer returns supplied. [Numeric cases](capability-scan-bindings-cases.json).
All refer to the pinned stock SMAC executable; no port implementation is added.

## Predefined native seed and no-Base eligibility

Use **zero-based predefined design slot8**, the ninth slot: installed **Mind
Worms**. Do not search the catalog by display name, look for any owned native
Unit, or substitute a custom design. Its installed prerequisite is Centauri
Empathy, technology index58. The capability producer queries this exact design
with the current owner and **no Base context**.

For a positive ordinary owner, that predefined-design query requires:

1. Active design flag bit0 set.
2. The design's faction-exclusion mask must not contain this owner's bit.
3. Design prerequisite âˆ’1 passes. Otherwise require technology index0..87,
   an enabled technology row as defined below, and this owner's ownership bit.
   Values belowâˆ’1 and index88 or greater fail.
4. If design plan8 (Colony) is present and No Colony Pods is enabled, reject.
   The stock Mind Worm plan is1, so this gate does not normally affect it.

There is no prototype-completion gate or additional component-technology check
for this predefined design query. With no Base context, it skips the sea-chassis
coastal-Base test. A synthetic sea-chassis Mind Worm still passes; that is a
predicate discriminator, not an admitted alternate stock design. No Colony
Pods alone does not suppress the stock Mind Worm seed.

The enabled-technology predicate used here and by the component scans is exact:
let p1/p2 be that technology row's two prerequisite values. Require
`p1 >= -1 && (p2 >= -1 || p1 == -1)` plus its owned bit. It does not recursively
require owning those prerequisites. In particular p1âˆ’1/p2âˆ’2 passes, whereas
p1â‰¥0/p2âˆ’2 and p1âˆ’2 fail. A no-prerequisite sentinelâˆ’1 passed directly to the
component query succeeds for positive owners; a disabled sentinelâˆ’2 fails.
Do not collapse these conditions into an invented early-technology whitelist.

When design8 is eligible, seed the separate PSI weapon cache by evaluating
**weapon definition12 (Psi Attack)**, and the separate PSI armor cache using
**armor definition9 (Psi Defense)**. These are fixed evaluator inputs; the
producer does not take the seed components from the design's component fields.
When ineligible, seed each separate PSI cache to0. Each seed query is made
before its respective weapon/armor scan. The ordinary maxima start separately
at1; native-design eligibility alone does not promote them.

## Component scans

All scans use catalog order and the prerequisite query above. They inspect
available components, not built Units or only previously generated designs.

| Scan | Count | Input interpretation | Acceptance and maximum |
| --- | --- | --- | --- |
| Weapons | 21 | Signed 8-bit raw power; signed 16-bit technology prerequisite | Require available prerequisite and raw power<99. Nonnegative power contributes directly; any negative power uses the PSI weapon evaluator. Maximize ordinary weapon capability, initially1. Negative components also maximize the separate PSI weapon cache. |
| Armor | 10 | Signed 8-bit raw power; signed 16-bit prerequisite | Require available prerequisite. **No raw>=99 exclusion.** Nonnegative power contributes directly; any negative power uses the PSI armor evaluator. Maximize ordinary armor capability, initially1; negative components also maximize separate PSI armor. |
| Chassis | 9 | Unsigned 8-bit raw movement, domain byte and signed 16-bit prerequisite | Require available prerequisite and domain exactly0 (land). Set land speed to `max(1, every qualifying raw movement byte)`. Other domains are skipped. |

Armor is **not doubled** in this capability scan; the restart design-rating
helper's doubled defense is a separate consumer. Land movement is not multiplied
by movement quantum, road rate, remaining allowance, reactor or social effects.
Installed land chassis have raw1/2/3. Synthetic raw255 remains255 and raw0 leaves
the floor1. A valid owned advanced technology enables the corresponding scan
entry even when earlier prerequisite ownership bits are absent.

## Cache storage and ranges

All five own caches and all five foreign caches use **32-bit integer storage**
and signed comparisons. None uses the regional counters' byte/16-bit storage.

| Quantity | Own producer initialization | Foreign rebuilding initialization |
| --- | --- | --- |
| Available weapon maximum | 1 | 0 |
| Available armor maximum | 1 | 0 |
| Available land speed maximum | 1 | 0 |
| Separate PSI weapon maximum | Native eligibility seed, or0 | 0 |
| Separate PSI armor maximum | Native eligibility seed, or0 | 0 |

Own ordinary weapon/armor floors remain1 when PSI estimates are zero or
negative. The separate PSI seeds are not independently floored at1. Foreign
outputs take signed component-wise maxima of selected peers' stored own
caches; a negative peer value cannot exceed their initial0.

The storage domain is signed32, âˆ’2147483648..2147483647; this is not a guarantee
that arbitrary extreme inputs can be multiplied without overflow. There is no
extra consumer clamp to98,127,255 or65535 on these stored maxima. Conventional
weapon power can contribute at most98 through this scan; armor at most127;
raw chassis movement at most255. PSI estimates can exceed those values. A
controlled prior foreign armor70000/weapon90000 produces own weapon/PSI105000
and armor/PSI60000. A foreign peer with all five stored values70000 produces
all five foreign values70000. These width controls do not claim legal acquisition
of such unusually large stored values. Existing PSI input/overflow limits apply.

## Per-owner startup ordering and MarkAndSelect

The turn-zero setup dispatcher makes **two sequential passes over slots0..7**:
first slots whose human-mask bit is set, ascending; then the remaining slots,
ascending. It calls and finishes each owner's initializer before advancing.
It does not initialize all owners' technologies, then refresh every owner in
one simultaneous batch. Slot0 is included in dispatch; only positive factions
participate in the capability producer's foreign-peer scan.

For the ordinary reached turn-zero owner branch:

1. Apply that owner's starting technology grants and reached setup inputs.
2. Run that owner's automatic design generator. Inside it, call capability
   refresh **before** the later generation/selection work.
3. After the generator returns, scan that owner's64 custom slots in ascending
   order, mark active designs prototyped and select the last active Recon
   (fallback predefined Scout2).
4. Continue that owner's Base/starting-unit work and complete its initializer.
5. Only then start the next owner from the dispatch order.

Thus capability refresh precedes this owner's **MarkAndSelect** stage. Peers
read whatever caches exist at their actual turn in this sequence, including
already processed owners' updated caches and unprocessed owners' retained
values. Do not replace that timing with a simultaneous snapshot, insert another
refresh after MarkAndSelect, or assume every unprocessed peer is necessarily
zero in a reused process. Earlier receipts establish ordinary initializer
clears and cold storage separately; full successive-game lifecycle remains open.

This ordering is established by exact caller/loop disassembly, the existing
[instrumented Gaian startup](startup-capability-and-ecology-rng.md), and four
dispatch-prefix controls. Those prefixes supply initializer returns to isolate
dispatch; they are not a continuous eight-owner world-generation execution.
The earlier 1,176 generator/mark/selection controls (reference receipt `startup-design-difficulty-and-mode.md`)
cover admitted owner-local generation. Nonzero-turn restart bypasses this
ordinary turn-zero generator/marking sequence. Full advanced-start/network/AI
world admission and original geographical placement remain outside this receipt.
