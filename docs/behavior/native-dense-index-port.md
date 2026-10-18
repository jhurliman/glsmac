# Native dense indices in live conventional removal

Original source: [five mixed-owner deletion controls](unit-dense-identity-cases.json).
Successful allocation appends to a dense global record order; deletion preserves
survivor order and shifts later indices left. Stable positive GLSMAC Unit IDs
remain independent of these original indices.

The live conventional-removal transaction now stages native survivor records
alongside registry/order/occupancy/knowledge updates. Each erasure repairs the
native source index and surviving stack links. For order1, equal first-waypoint-X
clears the order while retaining the payload; a greater payload decrements.
Erasures execute in caller order, including the point at which clearing an
order stops subsequent payload repair. This is not a batch final-index rewrite.
Opaque record fields, home Base identities and native counts are preserved.

Abandoned or failed preparations publish no repair. Resource/capture/restart
query views temporarily install the same staged records, then restore the live
records. Commit uses nonthrowing record copies with the registry and knowledge
publication. Querying a faction replacement does not add another full registry
or map-knowledge copy merely to install native indices.

World write/restore authenticates native indices against global record order.
Native-native stack links must be reciprocal, remain on one tile, reach a head
and account for physical membership. Invalid indices, dangling links, cycles and
disconnected same-tile stacks reject. The individual native decoder now admits
linked records so the completed manager can authenticate a real native stack.

Tests bind the five original field projections and add opaque-field canaries.
A real manager world with two conventional Scouts and a linked two-native stack
prepares/abandons/rejects/commits two conventional removals, checks stable IDs,
sequential order payload repair, unchanged stale native byte/word counters and
RNG, then performs actual world snapshot/restore. Four malformed graph controls
exercise the new untrusted-reference boundary. These do not claim native casualty
removal, mixed conventional/native physical stacks, AI/global traversal cursors,
transport gameplay or completed birth publication.

Validation: cached incremental build;64 focused native, worker, capture, restart
and Colony-related groups pass. Five production translation units pass targeted
correctness analysis: UnitManager, NativeUnit, SmacNativeUnitRecord, BaseCapture
and BaseRemoval. The same selected groups pass ASan/UBSan with those five TUs
instrumented, remaining normal objects reused and leak detection disabled.
After adding the stale-counter assertion, only the three dense-index groups
were rerun normally and with the unchanged instrumented objects relinked.
No local platform matrix was repeated. PR289 remains draft until live native
births and frontend publication complete the reviewed lifecycle finding.
