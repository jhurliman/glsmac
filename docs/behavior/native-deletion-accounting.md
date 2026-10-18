# Native Unit deletion accounting

Ordinary deletion of owner0 designs **8 (Mind Worms), 9 (Isle of the Deep), and
10 (Locusts)** debits both retained factory counters. It does not reconstruct
them from the live roster:

1. Decrement the owner's active-plan tally as a 32-bit word when the deleted
   design's plan byte is at most 7. These three installed designs have plans
   1, 7 and 4, respectively, so all qualify.
2. Decrement the owner's active-design byte for the deleted design.
3. Unlink the Unit from its tile stack.
4. Perform dense-array deletion/compaction and decrement the global Unit count.

The arithmetic wraps: active-design zero becomes **255**; active-plan zero
becomes **`0xffffffff`** (signed −1). There is no saturation or recount. With
diagnostic retained values 17 and 1000, one deletion gives 16 and 999, regardless
of how many matching Units actually remain.

Factory controls confirm the corresponding increments: global count first,
then qualifying active-plan tally, then active-design byte. Native initialization
and factories execute originally; the deletion canaries are then deliberately
seeded as a separate admission. The raw returned counters, rather than the live
Unit count, establish the accounting.

The first and last removals from a two-Unit same-design stack each debit once:
17→16→15 and 1000→999→998. Reusing the dense index after the first removal deletes
the survivor that compacted into that index; it is not an idempotent request
against the already-removed object. A negative index returns zero without
counter writes. Invalid nonnegative indices are outside the tested admission.

Within the observed owner0 runtime block, ordinary deletion writes only the
active-plan tally and active-design byte. The separate global count is also
decremented. Seeded native Base count71, population777, per-design loss-history
words100/200/300, neighboring design bytes, and peer-faction counters remain
unchanged. This helper does not maintain another per-owner total in these
controls. Other caller-specific casualty/loss bookkeeping is separate.

Two repeated, previously accepted class-conversion admissions connect this to
climate casualties. Land→sea removes design8 and changes active bytes
`[17,17,17]`→`[16,17,17]`; sea→land removes design9 and produces
`[17,16,17]`. Both change active-plan1000→999 and global roster3→2, retaining
native Base count71. Surviving design10 receives no debit. The climate caller
invokes ordinary deletion, so the port must avoid debiting these counters again
outside the deletion transaction.

[Literal cases](native-deletion-accounting-cases.json) include twelve successful
ordinary deletion returns, one negative-index no-op, twelve native factory
returns, and two complete class conversions with their own deletion calls.
Headless presentation services and controlled counter values remain explicit.
No combat, transport, arbitrary faction or broad deletion audit is claimed.
