# Production-reference cache across Base lifecycles

2026-10-20. Supplement to the
[retained production-cache contract](production-design-reference-cache.md).
Additive private freeze:
`tmp/reference/independent-workshop-production-cache-lifecycle-evidence-20261020/`.
Manifest `2aa8b0636ff314b698709825bca31e69ac70b42f0359ef0ff86cd9dff3c36714`.
Eight focused new native controls reuse 83 frozen founding controls, 1,156
defection-core controls and complete frozen Standard Rules startup buffers.
No original desktop, capture-world or startup rerun; no production edits.

**Do not implement a universal Base transaction that transfers queue references
from old owner to recipient. These native paths deliberately leave different
stale states.** Every count below is the unsigned wrapping byte indexed by
`95dbfc + owner*20bc + rawDesignId`.

## Transaction table

| Operation | Cache effect at this operation |
| --- | --- |
| Surviving military capture | Debit every nonnegative old current/pending occurrence from the **old owner's** row. Discard pending queue, reset current to Stockpile. The later direct selector's replacement target is **not credited to recipient**. |
| Successful riot defection | Debit old owner's whole queue, discard it, set current/baseline to Stockpile. Recipient row stays unchanged. |
| Direct ownership-transfer helper `563560` | Debit saved old owner's whole queue after writing the recipient owner. Clear pending and set current/baseline Stockpile. No recipient attachment. Static evidence here. |
| Ordinary Base deletion `4faad0` | **No production-cache debit or recount.** Removed Base's former contributions can remain stale. Registry compaction does not move design-count entries. |
| Capture destruction/raze branch | Calls ordinary deletion before the surviving-capture debit block, then skips that block. Thus no additional capture debit on this branch. |
| Founding `4fa490` | Write the selected current target directly, pending count 0. **Do not increment** production references. Preserve the existing owner row. |
| Faction initialization/reinitialization `5c8650` | Clear the selected owner's entire 512-byte row, including at nonzero turn. Later initial queues are direct, uncounted assignments. |
| Owner summary `5773c0` | Clear the requested owner's row, then recount live owned current/pending entries at its Base-recount stage. |

These rules describe changes at their named boundaries. Later ordinary target
switches and queue operations still apply their normal blind byte increments/
decrements. There is **no per-Base counted/un-counted flag** gating those deltas.
For example, switching away from an uncounted captured/founded target can
decrement zero to 255. Do not silently attach it first, clamp the debit, or
recount to repair the discrepancy. The next actual owner summary resolves it.

## Surviving military capture

`520730` reads the original owner before mutation. The old-owner stride is
computed into ESI at `52102a..521039`; recipient stride is a different value
in EDI. The Base owner byte is changed at `5210fe`, but the subsequent queue
loop still addresses **ESI / old owner**, not the now-current owner byte.

At `521190..5211b4`, iterate current plus declared pending entries and debit
each nonnegative raw design ID at `5211a7`. Duplicates debit repeatedly, without
activity/eligibility or bank remapping. Then set queue length 0 and current/
remembered baseline to Stockpile (-38) at `5211bb..5211c7`.

After facility reconciliation, `5215fa` calls the complete ordinary target
selector **`4fa3b0(baseId)`**. Its final instruction `4fa479` writes Base+50
directly. This is not the automatic switching setter `4fb2c0`; it has no
production-count update. No surviving new current target is attached to the
recipient cache by this path.

Three new controls supply old/new owner pairs 1→2, 2→1 and 7→3, with mixed
queue `[64,128,64,-6]`. They enter the exact debit/reset block with the Base
already owned by the recipient, then execute complete `4fa3b0` selecting Scout2.
All eight 512-byte rows match: only old-owner 64 twice and 128 once are debited;
recipient row remains its supplied canary through replacement selection.
Owner-stride preparation is source-traced; these are bounded mutation controls,
not new full capture runs.

The existing [full capture-to-screen-entry receipt](undefended-base-entry.md)
agrees on the live queue sequence: defender Former/current + Scout/Former
pending → attacker Stockpile/empty → attacker Scout/empty, all before the
captured Base-screen entry. Its later Scout target does not imply a cache
credit. Capture-to-screen evidence does not independently expose the byte rows;
the new focused controls close that operand distinction.

If capture instead deletes the Base at `5210c9`, it sets the Base index to -1
and bypasses the ownership/debit/select path. Apply ordinary deletion semantics
on that branch, not the surviving-capture row in the table.

## Defection and direct transfer

The existing [defection-core contract](riot-defection-transfer-core.md) already
checks old-owner debits, duplicate queue entries, zero→255 underflow and
unchanged recipient bytes in all **1,156** named/sampled/faction-pair controls.
Although the Base's owner changes first, the loop at `50a6d3..50a6fc` uses
the saved old-owner stride. The core finishes with empty pending queue and
Stockpile current/baseline. This supplement rechecks those existing results;
it does not rerun that matrix or promote the bounded core into full-defection
coverage. Static inspection of the later successful-defection body finds no
recipient queue attachment or direct replacement-target selection.

The separate transfer helper `563560` similarly retains old owner at stack+10,
writes recipient at `5638da`, and debits the saved old row at `56391c..563923`.
It then clears the queue and writes Stockpile at `563934..563944`. Later unit
conversion/removal affects active-unit counts, not production references.

The earlier optional automatic-chooser calls in riot/capture entry are distinct
transactions. If reached, their ordinary setter performs its own deltas before
the transfer core. Debit the queue that actually exists at transfer time;
do not debit a pre-action snapshot a second time after such an update.

## Removal, compaction and reset

Complete deletion body `4faad0` contains no production-cache writer and calls
no production setter or owner summary. It handles Units, Base registry
compaction, home references, bookkeeping, radius and territory work. Those
actions do not detach the removed queue from this retained row.

Three new complete deletion-body controls remove Base0 with queue
`[64,64,128]`, for owners 1, 2 and 7, with another Base retained and zero Units.
All eight production rows remain byte-identical while the live Base count falls
2→1 and compaction runs. Unrelated replay, presentation and map/radius services
are supplied, with a declared empty UI registry; original deletion/cache
instructions execute. Existing [starvation deletion](starvation-base-removal.md)
supplies the wider Unit/home/traversal behavior; the new controls isolate the
production row without replaying those worlds.

A later last-Base disposition/restart can invoke `5c8650`. Its clear at
`5c8ed8` belongs to initialization, not deletion. Two new admitted count-loop
controls prove full selected-owner row clearing at turn 0 and turn 42, while
other factions' rows remain intact. Keep these operations separate in the port.

## Founding and Standard Rules startup ordering

Founding calls `4fa3b0` at `4fa65b` for ordinary initial selection. Turn-zero
logic may overwrite that target at `4fa6a4`. Both are direct Base-field stores;
neither credits the production cache. Pending count is initialized to 0.
The 83 frozen founding controls preserve the complete patterned 512-byte owner
production row. Subsequent founding radius/resource calls are not production
setters or owner-summary calls.

The **ordinary Standard Rules startup does not recount these new queues**.
In the existing complete seed1 setup at `5a1f2d`, all eight production rows are
zero, despite seven Bases with these live targets:

| Owner | Pending count | Current target |
| --- | --- | --- |
| 1 | 0 | Former1 |
| 2..7 | 0 | Scout2 |

The complete native faction buffer hash is
`66f976c0e91e2b7b3857847b8fc400c7243966cea527250d9b58cda1ad0dafae`;
all 67,040 bytes were already frozen in
[generated startup composition](generated-model-startup-composition.md).
This supplement reads those bytes without rerunning startup.

There is a separate **Time Warp / accelerated-start** path. Setup tests rules
bit `80` at `5cb970`: set calls `5c6760@5cb979`; clear calls balancing
`5c7c90@5cb986`. Standard Rules disables Time Warp. The accelerated routine's
per-owner loop creates its Bases (`5c723b`) and initial Units before explicitly
calling `5773c0(owner)` at `5c76e0`; later accelerated work can choose targets
again. This is static ordering, not a new accelerated-start completion claim.

For ordinary startup, the first scheduled positive-owner upkeep occurs after
introduction/global advance. Its `53bb60` processes that owner's Bases before
calling `5773c0` at `53bbbf`, where the row is actually rebuilt. Do not insert
that recount between initial founding and the first Workshop opening. This
qualifies the earlier broad description of `5c6760` as an initial-world summary
caller: it is conditional accelerated setup, not an unconditional Standard
Rules step.
