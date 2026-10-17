# Downloaded turn and player completion

World-download V4 follows the frozen profile and JSF32 state with the typed
string `turn-state-v1`, a signed integer turn ID, a signed occupied-player
count, and ascending `(integer slot ID, Boolean completed)` records. The old
trailing turn field is removed; map/resource/unit/base/animation frames retain
their order and bytes. V3, V2, V1 and unversioned downloads are rejected before map
allocation. Mixed versions cannot join through this format. Disk map files are
not changed.

The synchronous writer captures RNG, turn and completion at entry, before
serializing the world. Every occupied slot is included, even when its player
is disconnected; open/closed slots are omitted. Capturing these values and
writing the snapshot do not draw randomness or change completion flags.

The reader requires the established session's exact occupied roster. Slot
indices must match their vector positions; occupied slots must contain a
Player with the canonical reverse Slot link. Count, ascending IDs, duplicates,
omissions, unexpected slots, frame types, Boolean bytes and truncation are
validated before map allocation. Allocation is bounded by the established
local roster, never an unchecked wire count. Turn IDs must be nonnegative,
fit `size_t`, and permit `turn + 2100` in both `size_t` and GSE's signed 64-bit
integer. Zero is valid. These are GLSMAC ingestion policies, not claims about
the original game's handling of malformed saves.

## Staging and identity

Successful body decoding stages owned wire values separately from private
runtime bindings. Each binding retains the expected Slot address and a Player
wrapper token. Existing Wrappable invalidation detects the end of a Player's
lifetime even if a new Player occupies the same allocation address. Tokens
are GC roots only while pending; a mutex protects their publication, traversal
and retirement. Token construction occurs under accumulation before taking
that mutex. Retirement drops references and leaves destruction to the GC.

The downloaded turn number is available to `start` and `get_turn`/`get_year`.
Completion flags remain uncommitted during setup: `start` sees the earlier
session flags. Before the first cached-slot use in initialization, before and
after `start`, and after deferred unit/base registration, the live roster and
all lifetime bindings are revalidated. Captured old Player or Slot addresses
are never dereferenced. A replaced player, even in the same numeric slot or
at the same address, is a terminal initialization failure.

Early validation can run on the backend thread outside accumulation. It now
uses the existing native collection exclusion and only validates or throws;
it never queues an independent error callback. The admitted startup coordinator
records that error and publishes its claimed Failure checkpoint before the next
MT request extraction. Pending cancellation permits the real reset request to
complete without owner drain; Running cancellation waits for the full owner
operation to finish. Already-queued ordinary iteration work skips a known
admission failure. See [the owner-checkpoint correction](snapshot-owner-checkpoints.md)
for the concurrency boundary and tests that replace the earlier generation gate.

Only after all records validate does initialization apply every saved flag
through the scalar Player setters. It then restores the turn/RNG continuation
and publishes one native turn-number request and status. No gameplay turn,
unit-turn or base-turn hook runs. Restoration does not reset movement, health,
moved flags, nutrients or population. It never calls `CompleteTurn`'s gameplay
path and does not auto-advance an all-complete snapshot.

Readiness comes from registered local units. A completed local player selects
`waiting_for_players`; otherwise status is `active` or `turn_complete`. The
readiness cache is initialized at that boundary, avoiding a spurious status
change on the next check. The production minimap subscribes to future status
events and reads `get_turn_status()` to initialize UI created after backend
startup. Normal advancement and subsequent minimap updates are unchanged.

Reset, cancellation, setup failure and a replacement download discard pending
wire data and token roots alongside RNG state. Late roster rejection validates
all players before touching any saved flag, installing saved RNG or publishing
turn/status. Setup callbacks can still have their own effects before failure;
map/manager body reconstruction retains its existing terminal, partial-failure
contract. This is not whole-world rollback or a safely retryable load.

## Limits and regression coverage

This persists completion at the downloaded world's boundary rather than using
earlier lobby defaults. It does not repair canonical faction references in the
Player codec, capture pending events/checksums, establish future host/client
lockstep or provide full save/load. Event catch-up during asynchronous download
and broader checkpoint consistency remain separate work.

Native tests exercise the production writer, prefix/body decoder and real
`InitComplete`, with real canonical unit/base/pop reconstruction. They check
opposite earlier lobby flags and setup changes, status ordering, disconnected
players, all-complete restoration without advancement, mission-year limits,
collection while tokens are pending, and release after commit/cancel. Identity
cases cover an absent/extra player, broken reverse link, a different Player in
the same slot, deterministic destruction/reconstruction at the same address,
and replacement during deferred spawn processing. Rejection leaves the first
player unchanged when a later binding fails.

Current admitted-checkpoint tests drive actual MT initialization and failure,
Pending/Running reset, a completed Client disconnect response during full Start,
and actual collector exclusion. Decoder-layer fixtures that directly construct a
world still invoke validation/completion under owner accumulation, and explicitly
dispatch their synchronous failure. They do not establish production scheduling.
The earlier unadmitted generation/parking fixtures are superseded; their historical
results below are not evidence that the held concurrency findings were solved.

Malformed records run both directly through the production decoder and through
real Client final-chunk dispatch, immediately and queued from a worker. Those
fixtures assert safe disconnect before map allocation and unchanged flags,
RNG, managers, settings and frontend publication. Existing reset, replacement
download and native/GSE failure cases assert pending-state discard. Ordinary
`AdvanceTurn` with the actual unit-turn script remains a positive control. The
actual minimap script runs against a wrapped Game with lightweight asset-free
factories; these checks do not claim rendered multiplayer-join coverage.

```sh
./build/bin/GLSMAC --gse-tests --gse-tests-native-filter 'world completion|world turn restore|world RNG|real final download' --memorydebug --prefix build/test-data-world-turn
```

## Historical validation before owner-checkpoint correction

Local Clang 15 / Ubuntu WSL validation passed all eight normal CTest entries
(34.47 seconds), all eight ASan/UBSan/LSan entries (41.32 seconds, leak detection
enabled), and 24 focused cases with allocation tracking in both builds.
Explicit correctness analysis of Game.cpp and WorldSnapshotTurn.cpp passed.
The isolated sanitizer build used the documented local `-no-pie` linker-cache
workaround; no repository compiler policy or sanitizer suppression changed.

Five separately linked negative controls removed saved-flag installation,
Player-token validation, exact roster count validation, queued-error generation
validation, and parked ordinary-event protection. They failed the corresponding
regressions (three flag cases and one case for each other guard). The earlier
outside-accumulation test also reproduced `GC not in accumulation mode` before
owner-thread failure dispatch was added. Final intended sources stayed intact
during copied-TU controls. Independent read-only production and test reviews
found no blocker at that time; later PR77 concurrency findings required the
separate owner-checkpoint correction linked above. Native Windows and rendered
join validation were left to integration.
