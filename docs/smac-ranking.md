# Retained ordinary faction ranking

The canonical turn now prepares the original ordinary no-project faction scores
before the turn increment and owner upkeep, then saves the inverse rank table.
It uses [the original score contract](behavior/ordinary-ranking-score.md) and
[the retained-order producer](behavior/ending-input-producers.md).
The count lifetime follows [the original cache contract](behavior/active-design-count-cache.md). This is the
ranking input needed by faction disposition; it does not yet implement the
single-human conquest controller or AI control.

Population and design quantities are saved caches. Each faction has 512 unsigned
byte design counts. Actual Unit creation/removal updates the exact design entry
modulo 256, before notifications. A successful nonzero-turn restart clears the
old row before its replacement factories. The owner summary clears and recounts
its current Units, alongside the population aggregate. A query or world restore
does neither. Financial candidate commits preserve the transaction's latest
counts instead of overwriting them with a pre-transaction copy.

The scorer sums four times cached population plus Future repeats, valid owned
ordinary technologies' four source ratings, and counted designs' cached costs
with the original weapon normalization and rounding. A count above 250 is
skipped, not clamped. Active prototyped custom designs across the owned catalog
supply the global weapon reference; predefined designs do not. Positive power
is divided by that reference before multiplication by count and cached cost.
The special/Psi arithmetic follows the static contract; the executed original
cases cover the basic Scout/Pod/Former domain.

All seven playable slots enter sorting. Inactive scores are zero, equal scores
prefer lower faction IDs, and inactive inverse positions remain zero gaps.
The retained leader can still identify a subsequently eliminated faction; only
an explicit ranking update replaces it. No focus/captor/survivor heuristic
initializes these identities.

Simulation V14 owns the optional inverse table. Economy V4 owns the optional
512-byte count row. V11–V13 simulation and V1–V3 economy frames still read;
missing historical caches/ranks stay absent. Their next owner summary establishes
current counts/population, allowing a following ordinary ranking update. Loading
never fabricates a prior order. New worlds explicitly initialize count rows to
zero and leave the inverse table absent until their first update.

Validation covers the original basic score cases, byte-cache canaries independent
of entities, retained gaps and inactive leaders, malformed rank identities,
older simulation migration, and a real two-owner turn/save/Start continuation.
The live discriminator supplies stale population/count inputs, observes the
pre-summary leader, then observes the next turn use the actual rebuilt caches.

Remaining conquest work: bind independent human membership from accepted setup,
consume retained references in the original disposition conditions, and join
saved ending reason/pending/processed/exit state to presentation and Continue.
Seven-faction AI campaigns, project bonuses and other scenario score modes need
their own source-backed admission and integration. M2 remains incomplete.

The focused native runs pass 42 core ranking/simulation/economy groups and
74 affected gameplay groups (115 distinct groups). Targeted clang-tidy 15 checks
pass on 13 production translation units, with the final score change rechecked.
