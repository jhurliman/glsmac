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
old-surface Unit cleanup in the complete climate transaction. The ordinary active local-human
[inspector opening boundary](base-inspector-resource-query-port.md) now invokes the
owning calculation through the real Game command queue. The 12-to6 save/Start
test uses that actual opener instead of calling the worker producer directly.
The original [fresh-query boundary](post-flood-worker-repair-boundaries.md) still
distinguishes an invalidated cache from a matching cached query, which retains
stale allocation. Existing detached
read-only resource previews are not proof of that original mutating query path.
The live high-history climate guard remains.

## Owning Base-effect preparation

`BaseManager::PrepareSmacFloods` now prepares native land-to-sea Base effects
against the real registry. One move-only plan retains candidate Domes across
repeated conversions. Each first unprotected flood consumes one CRT draw from
the caller's detached climate stream, stages population/2 clamped between2+draw%3 and10,
and retains the old workers, Pop identities, stores, production and resource
cache. Existing Domes and sea-to-land changes bypass damage/draws. Native objective
status and retained last-Base count protect population1; HQ and Do Or Die are not
substitutes for those predicates. Campaign-wide/facility objective rules use
their explicit authorities; unsupported project-host/network authorities reject.

Survivor publication validates every Base preimage before changing any Base,
prepares refresh entries, then swaps facilities and the retained count and sets
owner flood status0x80000. It does not publish RNG or persistent faction summaries.
The enclosing climate owner must publish its detached stream and per-call flood
scratch with terrain, entities and notifications. Discarding a plan has no live
effect. A plan containing a required deletion refuses survivor-only publication;
no Base is silently protected merely because ordinary removal is not yet joined.

Eleven focused native groups pass. New controls cover ordinary damage, pending
Dome suppression, existing Dome/land bypass, last-Base/objective floors, deletion
rejection, discard and two-Base preimage atomicity. A state produced by the real
Base-effect plan survives actual save/Start at population6 with12 old Pops, then
repairs through the real queued inspector opening. This supplements the earlier
seeded codec/malformed-record tests. The complete climate callback, ordinary
removal and domain/transport cleanup, flood scratch, geography/render publication
and final reseed remain to join; the high-history live guard stays enabled.

Targeted correctness analysis passes BaseClimate.cpp and the Base-manager/worker
consumers. The same11 groups pass focused ASan/UBSan with twelve production TUs
and the live-allocation test TU instrumented; remaining objects reuse the current
normal build, with leak detection disabled. Instrumented objects are invalidated
against their actual Ninja source/header dependencies.

The public composition also passes39 focused native groups, two real GSE
inspector/transferred-screen scripts and correctness analysis of all24 changed
production TUs. Its flood production TU passes three focused ASan/UBSan groups
against current public normal objects. The richer development composition has
the broader twelve-production-TU instrumentation above; neither is a full-engine
or cross-platform sanitizer claim. This promotion does not include unrelated
private occupancy, native-world removal or live ecology-controller changes.
