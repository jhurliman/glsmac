# Optional Base allocation authority

This slice connects the [lossless allocation value](smac-base-allocation-state.md)
to an existing native Base. It adds explicit supplied-state publication and exact
persistence. It does not invoke the automatic allocator, adopt an allocation at
founding, initialize specialist padding, or supply map knowledge, support,
priorities, availability, or resource-calculation inputs.

## Authority and population identity

`Base::PublishSmacAllocation` takes the complete value plus explicit registered
worker and seven positional specialist definition IDs. The caller must already
be in this Game's writable owning Space accumulation. The function validates
the current canonical resource context and owner binding; it does not infer a
context for legacy Bases. An absent allocation remains absent on save/restore.

The deterministic projection is a **GLSMAC identity policy**, not a recovered
original Pop identity rule. Existing Pops are visited by ascending port Pop ID.
Selected worker radius positions 1..20, in ascending radius order, receive the
first Pops. The ordered active specialist slots receive the remaining Pops.
Publication keeps all Pop map nodes, IDs, presentation variants, and wrappers.
It never consumes RNG or creates/replaces a Pop. It requires exactly enough
existing Pops for the supplied workers plus specialists.

The bounded live domain admits at most sixteen active specialists. The raw
value codec still admits its original count domain 0..127. Live projection
requires every selected worker coordinate to be valid, distinct, and different
from the center, using the current native Tiles periodic-X topology and the
shared single-adjust radius resolver. It rejects other Base centers and existing
worker claims. Conversely, construction/restore of a later Base cannot occupy
a worker claimed by an allocation-owned Base. These are concrete native
conflict checks, not a complete availability/territory/visibility calculation.

All 32 selected bits and all sixteen specialist slots remain exact stored
authority. Bit zero and bits 21..31 do not create additional Pop positions.
Inactive slot contents are never cleared or synthesized. A stored specialist
must have the matching registered positional catalog role; it need not be
currently available or nonobsolete. Replacement eligibility belongs to the
separate calculation, not to persistent record admission.

## Prepared native publication

One same-call transaction validates the complete old/new projection and stages
replacement copies of the reverse assignment index, frontend update queue, and
script update set. All potentially allocating preparation precedes changes to
authority or Pop fields. The commit changes only native scalar/optional fields
and swaps prepared containers. It calls no scripts, resource queries, allocator
callbacks, or frontend dispatch. Later frontend payload generation reads the
complete committed Pop projection; a later presentation failure is not a
rollback of the native commit.

The existing update callback dispatcher holds its set mutex. Publication
explicitly rejects reentry during that dispatcher, with an exception-safe
dispatch marker, rather than recursively taking the mutex. It does not change
the dispatcher's callback order or introduce a general callback transaction.

Generic population add/remove/type/assignment mutation rejects an
allocation-owned Base. The script create path rejects before its presentation
variant RNG draw. Actual Base/Pop resource, assignment, frontend, and save reads
validate the canonical projection; public legacy storage is not silently
accepted as a competing authority. This is not a conversion of every public
legacy field into a private access-controlled API.

## Persistence and compatibility

Base manager format V7 appends an explicit presence flag and a bounded 64-byte
allocation frame to each Base body. Present records retain every raw bit/slot.
Decode validates the complete record/projection before claiming the center or
advancing the Base ID. It never recalculates allocation or replays founding.
Actual world restore retains the existing deferred Base-manager ordering:
the saved simulation is published before pending Base bodies are constructed.

World V20 rejects V19 and older headers before map/manager body effects. It
changes the Base body contract only; Global11 and Simulation7 stay unchanged.
The detached worker-map frame is not world-owned by this change. Whole-manager
restore is not newly made transactional across all definitions and records.

## Validation checkpoint

The source checkpoint is based on `dacac744371c01ec8c298e5f87576af68585f14c`.
The normal build and focused memory-tracked selection pass (7.03 seconds).
Nine new live groups cover stable wrappers/variants/IDs, both deferred queues,
cross-Base claims, both center-conflict orders, narrow-map alias rejection,
generic mutation/read guards, late bounded decode, update callback reentry,
and actual deferred world-body restore into absent/different current simulation.
The existing actual Client/header rejection matrix includes old World V19 for
immediate and queued dispatch in both rulesets.

An independent peer used copied production/test translation units to inject a
one-shot `bad_alloc` after both update queues were prepared. The real tracked
test retained authority, Pop fields, assignment index and queues, then retried
successfully. A copied variant publishing the first Pop's type/coordinates
before that failure fails the intended preservation assertion. Tracked source
was unchanged. Control manifest SHA-256:
`fb4673c56e248bdcada7148c9f05e7ff929a6d353bfe03319d365bc9fb3c287a`.

Production and fixture peer review is clean. Sanitizer, broader tests and
analysis are pending at this source checkpoint; their measured receipt follows
separately. No default activation, original executable rerun, UI/manual-worker
handler equivalence, or cross-platform result is claimed here.
