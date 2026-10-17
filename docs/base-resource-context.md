# Owned Base resource context and positional population roles

This slice adds an optional authoritative record to the existing backend `Base`.
The existing `Pop` definition pointer and optional worked-tile coordinates remain
the population allocation authority. No default world opts in, and no new base
resource calculator, population allocator, governor algorithm, or complete
economy is activated.

## Produced context and persistent state

`SmacBaseResourceContext` owns the evaluating owner's explicit simulation ID,
Golden Age, six local resource-event flags, their signed 32-bit shared timer, and
one unsigned 32-bit governor configuration. The governor-active and
citizen-management accessors read independent bits (`0x80000000` and `0x40`).
All other governor bits are retained. The six event bits use a named compact port
encoding; they are not original memory offsets or an original save format.

An absent record means legacy/unavailable. `GetSmacResourceContext()` returns a
copy only after checking registered Base/tile membership and the current bound
simulation's exact owner Slot and faction. The native setter preserves owner
identity and checks mutation authority; it replaces supplied scalar state without
aging events, recomputing Golden Age, allocating workers, changing faction
remembered settings, or interpreting governor-save actions. The script getter
returns copied values, and the Boolean presence property never supplies neutral
state for an absent context.

The explicit `spawn_base` option `smac_starting_resource_context: true` invokes
the first-base producer before construction or ID/tile publication. Admission
requires the actual claimed new-world Start, turn zero, no reload, an admitted
owner, no existing Base for that owner, and original difficulty 3 from canonical
settings and the bound startup design state. It produces Golden Age false, no
active local events, timer zero, and governor configuration `0x00dfbe41`.
The ordinary spawn event forwards the option only when supplied. Other
difficulties and later founding need their own admitted producers.

The semantic source is
[first-base context](behavior/first-base-worker-context.md) and the measured
[local timer producer](behavior/first-base-local-event-timer.md). The latter
executes seven original controlled compositions with nonzero timer poison at
founding entry, including both signed endpoints; each observes the original zero
store and unchanged zero through first command. Its case fixture SHA-256 is
`6b01a7573cf039c662f6a41fd4d70a20c12dff7720bd9bb69b7c74aa5408be15`, raw report
`658311cf03c60262a3965030f0e8dd3146e264a9bb4cb164f369744c27614c3d`, and bound
receipt `cb7639b97804935b9e881726d7bc1b23c785a7e113a34cd70640b32b9f6bacc9`.
These human-owned evidence files/assets are not modified or rerun by this work.

This producer initializes context only. It does not grant HQ or faction
facilities, create population, assign a specialist, or select production. The
original center-plus-specialist example is a controlled arid-map allocation,
not a universal first-base package. In particular, an opted-in empty Base remains
empty until a separate admitted population/allocation producer acts.

## Roles, restore, and rejected operations

`PopDef` can own an optional positional `smac_specialist_id` in 0..6. Typed roles
require the frozen specialist and technology catalogs. Display name and port
definition ID do not determine identity; a definition cannot simultaneously be a
tile worker and a specialist. This validates source-role identity, not technology
ownership, obsolescence, minimum-size eligibility, or specialist output.

For a canonical Base, every nonworking Pop must use a registered typed specialist
definition; every worked Pop must use a registered tile-worker definition.
Constructor, assignment and create/restore paths check the complete role/owner
requirements before publishing a logical mutation. Invalid create requests reject
before their presentation-variant RNG draw. An in-use canonical role cannot be
undefined. Legacy Bases retain their existing population semantics.

`DefinePop` retains caller ownership until successful return. It builds the owned
frontend payload before manager insertion, rejects duplicate IDs at runtime, and
erases the exact inserted entry if the frontend vector cannot copy the request.
The local request and queued copy own separate strings. Definition decoding and
the GSE definition path hold candidates with RAII until transfer completes.

The BaseManager wire marker is `GLSMAC_BASES_V5`; older/unversioned records reject
before definitions are published. PopDef appends explicit role presence/position.
Base appends a strict version-1 optional context frame: 21 bytes absent, 84 bytes
present. Its outer `ReadString(84)` rejects before copying or consuming an
oversized frame; inner types, ranges, version and exact end are checked. All
signed timer and unsigned governor bits restore verbatim, without a founding
reset or `base_spawn` callback. This is a nested Base format change; no simulation,
GlobalSettings, or outer world schema changes here.

The actual world-body reader defers Base buffers while the game is initializing.
Claimed Start publishes and validates the downloaded simulation before marking
the game running and executing `BaseManager::ProcessUnprocessed`. Canonical Base
construction therefore uses that published saved context, not the old current
simulation during decode. This differs from Unit decoding, which needs its
explicit private staged resolver earlier. Manager-wide transactional restore and
general allocation-failure rollback remain outside this slice.

The old heuristic growth callback skips canonical Bases. Existing work/unwork
event validation and apply paths reject them explicitly, because the measured
[manual worker handler](behavior/manual-worker-handler.md) has permission,
citizen-management and forced-calculation ordering that those legacy callbacks
do not implement. No equivalent handler is claimed. Canonical nutrient
consumption must later read frozen `nutrients_per_citizen`; the legacy callback's
hardcoded population-times-two is not validated as canonical balance. Mineral
support, energy posting, global modifier timing and resource-event aging remain
separate consumers. Global resource state remains owned by the simulation.

## Validation and ancestry

Source starts at `cab3b0453c1c6c8cde926993d13e7ddceb5e4b60` with exact specialist
catalog source `af35415aaa1791029bee59503bff0546a4a0d481` cherry-picked as
`6aa98a6d`. The feature commit remains separate from that prerequisite.

Twelve dedicated native groups cover scalar/wire ownership, all 64 event masks,
signed timer endpoints, independent governor bits, all seven positional roles,
actual GSE definition, logical rejection before ID/tile/RNG effects, actual
manager restore, actual world-body deferred restore with absent/different current
simulation, and real legacy growth/edit callbacks. The immediate and queued
startup tests use actual claimed Start and event dispatch with three admitted
participants, then first-turn persistence; they do not claim seven original
founding executions or default world activation. The existing specialist groups
run alongside them.

Linux validation of the final source:

- Full normal CTest: 8/8, 100.14 seconds.
- Full ASan/UBSan/LSan CTest with leak detection: 8/8, 182.32 seconds.
- The 12 feature plus five specialist native groups pass with allocation tracking
  in both binaries (17/17, 2.02 and 4.12 seconds).
- LLVM 15 correctness analysis: all 340 production translation units, zero
  failures, 121.69 seconds (124.79 seconds including the wrapper). The Windows-only
  network TU is outside this Linux scan.
- Copied Game and test TUs inject one `std::bad_alloc` at the real PopDef frontend
  submission. Fixed production passes map/queue rollback, exactly-once candidate
  destruction, clear/retry and duplicate rejection. A copied old manager ordering
  fails its map-ownership assertion (0 pass/1 fail); the test relinquishes the
  duplicate test owner before reporting failure, so detection does not depend on
  a double deletion or allocator reuse. This injects a submission failure; it is
  not an exhaustive allocation-failure sweep of every request copy.
- Four independent copied production controls fail the intended tests: resetting
  the saved timer, removing the outer frame cap, removing role validation, and
  dropping the claimed-Start gate. The last control reaches the late-producer
  assertion after the fixture removes prior Bases, independently testing the
  claim gate rather than merely rejecting another existing Base.

Receipts and exact copied sources remain under
`/home/jhurliman/src/glsmac-startup-main-refresh/base-resource-*`, with the source,
script, log and control hashes in `base-resource-final-manifest.json`.
Initial retained failures include test compile corrections, a queue observer that
incorrectly cleared the queue, a missing test TileManager, an invalid GSE throw
syntax corrected to `throw Error(...)`, and a test envelope-length mistake. No
interpreter change or timeout adjustment is part of this feature. These are local
Linux results, not Windows/macOS, hosted CI, or an original-game rerun claim.
