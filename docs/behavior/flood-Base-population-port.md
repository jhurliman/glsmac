# Retained flooded-Base population

The backend now keeps the current population separately when it differs from
the retained worker/specialist projection. `Base::GetPopulation()` supplies the
current count to gameplay consumers, including support, growth, capture, native
memory, council and the existing owner population-summary writer. The stored
Pop nodes, reverse assignments, allocation slots, stores and upkeep cache remain
unchanged on restore. Reading the count does not recalculate a faction summary.

A pending reduced count uses the explicit inner extension
`SMAC_RETAINED_POPULATION_V1` under Base-manager format `GLSMAC_BASES_V12`.
Only canonical records with a positive reduced count and a bounded old projection
are admitted. V11 explicitly rejects the extension; ordinary coherent snapshots
keep their previous V11 bytes. Neither an absent extension nor an old file
infers a flood. Deferred Start carries the format permission into the real Base
decoder. Clearing the manager also clears deferred records and that permission.

Frontend Base updates carry the current count independently from their retained
Pop list. Map badges and Base sprite size use the count. The existing owning
worker publication defaults to the current population, retires excess Pop nodes
only at its prepared commit, and then clears the optional override. A later save
therefore returns to coherent V11 framing. Physical projection validation, Pop
ID allocation and startup construction continue using the physical Pop records.

The new native test explicitly seeds a saved post-flood state: population12 to6,
eight old workers, four specialists, installed Dome, stored nutrients/minerals,
and a cache produced by the existing owning worker calculation. Actual Game
save/Start preserves the complete Base bytes and old cache, publishes count6
with all12 old Pops, and leaves their allocation intact. A subsequent real worker
recalculation produces six surviving Pop identities; a second save/Start keeps
the repair. Malformed counts, unknown/truncated/suffixed extensions and V11
admission fail without changing the source world or next Base identity.

Validation:134 distinct focused native groups pass. All24 changed production
translation units pass targeted correctness analysis. Focused ASan/UBSan uses
nine production TUs plus the live-allocation test TU, reusing current normal
objects for the rest of the engine; the new restore/repair and existing live
worker/capture boundaries pass, as do39 additional Base/context/cache groups.
Leak detection is disabled; this is not a whole-engine sanitizer claim.

This checkpoint provides representation and the later worker-repair consumer.
The test seeds an owned save record; it does not execute the climate flood
producer. That producer must still stage population/Dome, owner status/scratch,
objective/last-Base/network protection, ordinary deletion/home repair and
old-surface Unit cleanup in the complete climate transaction. Before enabling
that caller, connect the original [fresh-query and inspector repair boundary](post-flood-worker-repair-boundaries.md):
opening the inspector invalidates its resource-cache key and runs the calculation;
a matching cached query alone leaves stale allocation intact. Existing detached
read-only resource previews are not proof of that original mutating query path.
The live high-history climate guard remains.
