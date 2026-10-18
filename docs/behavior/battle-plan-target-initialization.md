# Battle-plan target and fresh Base status initialization

The per-owner faction initializer sets that owner’s saved battle-plan Base target to **-1 unconditionally**, before the fresh/nonzero global-turn branch. It does not reset peers. Eight original instruction-prefix controls cover owner0/owner2, global turn0/3, and supplied second argument0/-1. Every target begins as a distinct positive canary701..708. Owner2 controls preserve slot0 and all peers; owner0 controls preserve slots1..7. These are prefixes through the store, not full initializer returns.

The fresh/nonzero distinction here is the global turn, not the second argument. Both second-argument controls also agree. The higher new-world caller has complementary human/nonhuman loops over all eight slots, including0; together they initialize each once. That all-eight conclusion is source-mapped, not a new full-world execution. A per-owner reset gives no authority to invent values for untouched peers or legacy absent fields.

Fresh Base creation writes the **entire 32-bit status word to0**. Two isolated original store-fragment controls clear all-ones and mixed-bit canaries, preserving a peer Base word. This certifies the initialization store, not a newly executed full founding transaction. Later capture/defection applies its separate preserving mask; fresh zero initialization does not authorize discarding opaque bits in existing records.

Cases: `battle-plan-target-initialization-cases.json`. Frozen supplement: `tmp/reference/battle-target-initialization-evidence-20261018`. Previous completion and retained-authority bundles remain unchanged.
