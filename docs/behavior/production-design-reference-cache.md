# Retained production references by owner and raw design ID

2026-10-20. Same pinned original terran.exe as the Workshop contracts.
Additive private bundle:
`tmp/reference/independent-workshop-production-cache-evidence-20261020/`.
Manifest `3bcd7aad01656310598b8f6e5ce0692669174662c425c29317d550bb66753202`.
Thirteen new bounded native producer/queue controls, one complete ordinary
save/read, and 41 existing complete Workshop controls checked for preservation.
No production edits, desktop or original UI rerun.

**Retain this cache alongside active design counts. Maintain queue transactions
and rebuild at the owner-summary phase. Workshop open and Apply do not recount
it; ordinary load restores saved bytes, including stale values.**

## Storage and contributions

The address is `95dbfc + owner*20bc + rawDesignId`: **512 unsigned bytes per
faction**, Faction-relative offset `a54`. It is separate from the active-unit
row at relative `854`, prototype flags, production completion history, and
design owner-bank membership.

For an owned Base, signed queue count `Base+4c` is the number of pending
entries. Current target is signed int `Base+50`; pending entries follow at
`+54,+58,...`. The recount iterates indices **0 through count inclusive**.
Count 0 therefore counts the current target. Negative count skips the entire
Base queue, including current; this is a native guard, not an ordinary valid
queue state. Storage beyond the declared count is ignored.

Every nonnegative target contributes one to the requesting owner's byte at
that **raw design ID**. Negative facilities, projects and Stockpile do not
contribute. Duplicate occurrences count separately, including current plus
pending duplicates within one Base and occurrences across multiple Bases.
The producer does not test design activity, exclusion, prototype status,
technology, role, eligibility or custom-bank ownership. A Base owned by 1 can
therefore contribute to row 1 / raw design 128 in a supplied queue. A foreign
Base is excluded regardless of its queue IDs. Valid design IDs are 0..511;
out-of-range nonnegative IDs have no bounds guard in these loops and are not
valid inputs to reproduce as supported port behavior.

All increments/decrements wrap modulo 256. No saturation or stale-cache repair
precedes a transaction. The new recount control with 256 owned Bases producing
design 64 yields byte 0. Manual controls prove decrement 0→255 and increment
255→0 while preserving other factions' rows.

## Producer and refresh timing

Owner summary/policy routine **`5773c0(owner)`** clears all 512 active and
production counts at `577475..57748d`. It scans the Base registry using the
Base's current owner byte at `+4`. For matching Bases, `57783a..57785e`
walks current/pending and increments production bytes at `577853`.

The new eight summary controls execute the original function from entry through
`577919`, including clearing, real owner filtering and all Base queue increments.
Unrelated location/facility-policy helpers are supplied; the count decisions
and writes are original. This is the Base-recount stage, before later Unit/
policy work. Later nonhuman policy can call the ordinary production setter,
which updates the cache transactionally after choosing another current target.

Direct callers are ordinary faction upkeep `53bb60@53bbbf` and initial-world
setup `5c6760@5c76e0`. The existing
[owner-summary lifetime evidence](faction-population-aggregate.md) places the
ordinary upkeep recount after that owner's Base processing. The faction
initializer also clears all 512 production bytes at `5c8ed8` inside its count
loop, including at nonzero turn; the adjacent turn-zero guard concerns a
different loss-history row.

## Queue transactions

| Path | Exact production-count effect |
| --- | --- |
| Automatic target setter `4fb2c0` | Keep-current sentinel 99999 returns; equal new/current returns. Otherwise write new current, decrement old if nonnegative, increment new if nonnegative. Pending references are unchanged. |
| Queue advance `5058c0` | Decrement each nonnegative current removed. Shift/promote pending without increment: it was counted already. Rejected candidates removed by another iteration are debited; the final rejected candidate left current stays counted. |
| Manual chooser `41cc10` | Detach this Base's old whole queue before editing, then attach its resulting whole queue on the exit branch. This is a per-Base delta against the retained owner row, not a full recount or a clear. |

Manual detach is `41d2e3..41d324`, decrement at `41d30b`. One attach branch
is `41e3dc..41e41d`, increment at `41e404`; other branches at `41dd34`,
`41e021` and `41eb22` perform the same addition. The five new paired controls
execute the actual detach/attach loops with different queues, duplicate IDs,
negative items, wraparound and stale canaries. Unrelated owner/design counts
remain intact. A canceled/unchanged queue detaches and reattaches identical
contributions, preserving even an initially stale row.

Existing [queue advancement controls](production-queue-and-build-eligibility.md)
and [automatic switching controls](production-switching.md) execute those
complete helpers; no large matrix was repeated. Inspected capture/reset paths
also debit every nonnegative old queue entry before clearing/replacing the
queue (`50a6d3..50a6fc`, `521190..5211b4`). Those extra paths are static
producer evidence here, not new capture controls.

## Workshop consumes the retained row

Workshop entry `449630` calls capability summary **`577140`** at `4496fe`;
that routine computes available component strengths and diplomacy maxima.
It is distinct from `5773c0` and does not recount Base queues. Entry next
selects the design and calls browse builder `5bdb60`. Browse building reads
design/technology state and sorts; it does not rebuild production counts.

Apply's `442600→441e20` path reads the cache for occupied-bank allocation and
reuse as specified in [the custom-edit contract](design-workshop-custom-edit.md).
All **41 frozen complete edit/rename/toggle/Apply/Done controls** preserve every
production byte in all eight faction rows. Twelve cases have nonzero input
production counts, so this is not solely a zero-cache observation. Live-design
retirement can rewrite Base queues while retaining the old production cache:
the established retired-64 case leaves its byte 4 even after queue replacement.
Do not insert an unverified recount on Apply, Done, reselect or retirement.

For safe allocation, consult both retained rows: active-unit count and queued
production-reference count. Unit count zero alone does not make a design unused.
Likewise, a current-target-only count omits pending references that protect slots.

## Save/read authority

The complete original ordinary writer `5c0340` and reader `5c05e0(name,0)`
preserve stale production canaries with **zero Units and zero Bases**:

| Owner / raw design | Saved and restored byte |
| --- | --- |
| 0 / 2 | 255 |
| 1 / 2 | 251 |
| 2 / 128 | 201 |
| 7 / 511 | 17 |

After saving, all four live bytes are poisoned to 99; reading restores their
saved values exactly. Both bodies return normally with no RNG calls. Memory-backed
FILE/allocator/TLS/view services are supplied; no original save file is written.
The row lies inside the complete 67,040-byte faction block written/read at
`95d1a8`. This proves ordinary file-body authority, not every frontend resume
scheduler or alternate serializer mode. Recount only when the native-equivalent
summary or an explicit lifecycle reset actually occurs.
