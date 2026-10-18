# Regional route marks persist across source components

2026-10-18, stock SMAC. **Keep one shared component-mark set for the complete
Base-pair caller invocation. Do not reset peer marks for each distinct owner
source component.** The proposed per-source reset disagrees with the measured
original caller. This clarifies the retained-mark rule in
[strategy overrides](regional-strategy-overrides.md), without changing it.
[Literal map and observations](regional-split-source-component-marks-cases.json).

The native caller clears all map traversal scratch once after its retained
regional-count gates. It scans owner Base records forward, marking each
previously unmarked owner component. For each source it scans peer Bases and
skips already-marked peer components. A failed path does not clear those marks
before the next source component. A fresh caller invocation clears them again.

One flat16×12 parity map separates two owner1 land components and one owner2
peer component. All tiles have stored region1. The first source at(2,2) is
isolated by water; the second at(2,8) can reach the peer at(10,8) through an
unowned land corridor. Each owned component includes an adjacent same-owner
land tile. All other tiles are water. Mode0 admits the unowned corridor.
Retained regional Base counts are owner2/peer1; actual Base records match.

| Native execution on this map | Observed calls and result |
| --- | --- |
| Base scan order: isolated source, reachable source, peer | One path query, (2,2)→(10,8), returns−1. When the second source reaches the peer mark check, peer scratch is still1; no second path query. Caller returns **0**. |
| Direct abstract path query: second source→peer | (2,8)→(10,8) is **reachable**. |
| Same Base records, reachable source placed first | One path query, (2,8)→(10,8), succeeds. Caller returns **1**. |

Both complete Base-pair calls execute the actual component marker and native
pathfinder. The direct query also executes the complete native pathfinder.
No reachability/helper result is supplied; the retained distance workspace is
explicitly invalidated before each of these three controls. No RNG calls occur.
The result therefore establishes caller suppression and Base-order dependence,
not an inferred graph improvement or a stale workspace artifact.

The [abstract path graph](abstract-regional-land-reachability.md) remains the
predicate used **when a pair is actually queried**. Do not replace native
pair selection with testing every reachable component pair. This one-map
discriminator does not broaden ordinary Unit movement, water endpoints or
other territory modes. Standing research remains paused.
