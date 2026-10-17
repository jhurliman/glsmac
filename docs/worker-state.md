# Worker assignment state

A citizen's optional (x, y) map coordinate is authoritative. Pop serializes
it with its ID, type, and variant. BaseManager rebuilds a coordinate-to-
(base ID, population ID) index when each base registers, including deferred
startup restoration. Tile wrappers resolve that index on demand; no worker
assignment stores a raw Tile or Pop pointer. Base worked-tile lists are derived
from its citizens, in population-ID order.

The base.set_pop_assignment(pop, type, tile) method validates ownership, map
membership, coordinate bounds/parity, worker-compatible type, and competing
claims before changing the type, coordinate, or reverse index. Passing undefined
or null clears an assignment. The base center is excluded because intake already
counts it independently. Changing a worker to a specialist, removing a citizen,
or despawning a base releases its claim. Base-manager cleanup clears numeric
state without following the map, which Game::ResetGame destroys before its
collected managers.

Production scripts and UI use pop.get_worked_tile() and tile.get_working_pop()
instead of mutable custom properties. Work/unwork rollback restores the prior
type and assignment, including reassignment of an already-working citizen. An
event cannot authorize a foreign citizen by supplying another base. Automatic
tile scoring, workable-area generation, resource yield, and nutrient/growth
arithmetic are unchanged.

Worker coordinates were introduced in GLSMAC_BASES_V3; the current marker is
GLSMAC_BASES_V4, which also stores [production state](base-production-state.md).
Earlier, unversioned, and unknown formats are rejected. Each restored
base validates all its citizen records before claiming its center or advancing
the base ID: unknown types, duplicate population IDs, mismatched population map
keys, colliding next-population IDs, invalid coordinates, specialist claims,
repeated claims within a base, and conflicts with earlier bases fail. This is not a transaction over an entire
manager: definitions and earlier valid bases can remain after a later record
fails. A failed immediate or deferred load must be discarded through game reset;
retrying its partially consumed records is unsupported.

The native backend regressions cover immediate/deferred reload, identical local
population IDs in different bases, preserved IDs/types/variants/coordinates,
reverse lookups and worked/unworked tiles, production base intake over synthetic
tile yields, rejected writes, release/reassignment, actual production event
apply/rollback, malformed records and competing restored claims, and map-before-
manager teardown with retained wrappers. They require no original assets.

This slice establishes persistence and consistency, not original-game fidelity.
The behavior research handoff RE-SMAC-BASE-001 supports independent base-center
contribution as static-derived SMAC evidence, not an observed SMAX result. Full
save/replay equivalence remains unfinished. In particular, remove-pop rollback
still recreates a citizen with a new ID/random variant; that separate identity
restoration defect is not corrected here.
