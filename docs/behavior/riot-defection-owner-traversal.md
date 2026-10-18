# Defection to a later or already processed human owner

RE-SMAC-RIOT-DEFECTION-TRAVERSAL-001, 2026-10-18. Pinned stock SMAC.
[Literal owner-pass sequences](riot-defection-owner-traversal-cases.json).

**The original owner traversal checks each Base's live current owner. It does
not freeze every owner's Base-ID membership at the start of the global turn.**
Two controlled sequences execute full human owner passes1 then2, with the same
declared default land-unit defection world as the
[Base completion receipt](riot-defection-completion.md). The inverse sequence
relabels complete faction/Base/Unit/map ownership state1↔2; it does not merely
change the transfer's owner byte after admission.

| Sequence | Actual Base entries `(owner pass, Base ID)` | Base returns |
| --- | --- | --- |
| 1→2; recipient later | (1,0), (1,1), **(2,0)**, (2,2) | 1,0,0,0 |
| 2→1; recipient already processed | (1,2), (2,0), (2,1) | 0,1,0 |

There are **four complete owner-pass returns and seven complete Base returns**.
The owner helper's incidental numerical return values2/4 are recorded; completion
is established by the normal stop/return, not by assuming it must return0.
The two-pass sequence is a controlled ordinary caller chain, not the complete
global year controller or a timing/presentation run.

## Transfer invocation and later processing

In either direction, the old-owner invocation processes production/food, the
second resources and riot decision, transfers ownership, performs final
new-owner resources and returns1. It does **not** submit research, run ecology,
post income or settle maintenance for that Base. Its three citizen passes and
one status check belong to that transfer invocation.

When recipient2 runs later, the actual traversal sees Base0 now owned by2 and
processes it again normally in the same declared turn6. That second invocation
has two resource/citizen passes and one status decision, then posts its actual
new-owner outputs to faction2. Base0 food is0→2 during the transfer invocation
and2→4 in the recipient invocation. There is no universal per-global-turn
already-processed guard suppressing this later-owner visit.

When recipient1 has already completed its pass, transfer during owner2's pass
does not re-enter owner1. Base0 remains at food2, with no research/income posting
from that Base in this two-pass sequence. The old owner continues with its other
Base normally. Its retained battle-plan references, history/count updates and
Unit cleanup follow the same completed transfer.

The total current-category postings from *all* visited Bases are recorded in
the fixture; they must not be attributed entirely to the transferred Base:

| Sequence | Final credits `[f1,f2]` | Final research `[f1,f2]` | Base0 food |
| --- | --- | --- | ---: |
| 1→2 | 104,206 | 45,37 | 4 |
| 2→1 | 204,104 | 35,45 | 2 |

Before the reversed sequence, the two factions' complete retained inputs and
initial balances are relabeled too, so f1 starts with the recipient's200/30 and
f2 with the old owner's100/40. Neither sequence tests technology completion.

## Exact scheduling and randomness boundary

Return1 makes the original traversal back up its index/pointer before the common
increment, so it rechecks that record position. The now-foreign Base is skipped
by the live owner test; the old owner's scan then continues. This same cursor
shape also supports a genuinely removed/compacted Base without assuming that
every return1 means deletion.

Keep the old owner cursor and continue its live owned-Base scan after return1.
When a later owner's pass begins, derive admission from live current ownership.
A controller that snapshots each owner's Base IDs for the whole year would
miss the actual second Base0 visit in1→2. Per-owner preparation must likewise
respect the live scan and ownership-change continuation; do not interpret1 as
an automatic second turn for the old owner or an immediate recipient pass.

The actual owner-start social refresh executes. Declared resolved intrinsic
POLICE inputs−3/0 keep the intended ratings; these are not unchanged neutral
stock Gaia traits. The earlier development diagnostic with only a transient
current rating was rejected as a transfer test because refresh replaced it.
Only the final coherent producer-input sequences are acceptance evidence.

Each actual CRT call advances normally; only riot gate/candidate returns are
overridden0. Forward1→2 consumes17 CRT draws:14 for defection, then one ecology
roll at remaining old Base1, one at recipient Base0 and one at recipient Base2.
Reverse2→1 consumes16: recipient Base2 ecology first,14 defection draws during
owner2, then remaining old Base1 ecology. There is no ecology draw for the
ownership-changing Base invocation itself and no shared RNG in these controls.
Native raw/used values, seed progression and actual Base entries are literal.

Headless notification/log/UI services remain supplied without intervention;
no Governor, score, research, Unit cleanup or territory result is substituted.
No retired-owner disposition is reached: the consistent source gate requires
at least two old-owner Bases and leaves one. Full diplomacy/model legality,
inactive-recipient revival, arbitrary prototypes/cargo and the global controller
remain separate. Existing frozen receipts are preserved; broad research remains
paused.
