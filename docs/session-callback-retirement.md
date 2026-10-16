# Queued game-session callback retirement

`State::WithGame` binds a callable to the current State/Game session. Every
`SetGame` creates a new native identity; an old identity is never reactivated,
even if the same State and Game objects are rebound. Reset entry, State reset,
and unbinding retire the old identity. The guard runs before entering the
callable or accessing its captured world objects. The callable receives its
bound Game instead of resolving the engine's current global Game.

The native queue owner retains both State and the caller-supplied GC owner.
The collector traverses those roots until the queue releases its callable.
The owner is a shared RAII object, not a GC-registered object, so its own
lifetime does not depend on collection. Immediate execution, nested calls,
cleanup callbacks, and exception behavior remain Space's existing contract:
cleanup does not run after an immediate exception; a throwing queued callback
leaves the queue retained until a later successful drain or Space teardown.
Retiring that session makes a later drain skip its body and perform cleanup.

The migrated producers are Game initialization/start, turn/status/message,
event processing and responses, world download completion and snapshot-failure
notification; Client/Server game-event ingress; animation completion; and the
TileManager's deferred lock-request batch. Animation IDs and tile-lock batches
are copied before their request storage leaves scope. Packet delivery also
checks that its captured connection remains State's current connection.

Initialization queues `initialize` after any prior reset and rebind, before
`configure`. Reset clears pending responses as well as pending/waiting events,
so a response cannot be applied to an event ID reused by a later session.

Extracting an event batch removes it from Game's normal pending-event roots.
A temporary native batch owner is published under the pending-events mutex
before extraction completes. Collection traverses it under that same mutex.
Queue registration occurs after releasing the mutex, because Space collection
locks its pending queue before traversing Game. An exception-safe scope removes
the temporary root after immediate execution or queued-owner registration;
the queued batch owner then retains every Event until execution/cancellation.

## Deliberate boundary

This change cancels **not-yet-executing** callbacks. It does not exclude native
reset or roster mutation from an already executing callback, synchronize a
check with the rest of a callback, or make world initialization/reset atomic.
It does not resolve the concurrent snapshot validation/commit and failure/reset
findings on PR77. No new collection or render locking policy is introduced.

General `WithGSE`, Connection `WTrigger`, and `Slot::Update` are unchanged.
They include lobby behavior that must remain usable before an active game
session is bound. Slot/Player publication, lifetime, and canonical ownership
need their own coordinated repair. Game itself retains its existing native
engine lifetime; the session token does not own/delete Game.

## Regression contract

Eleven native `test session ...` cases are part of the normal GSE suite. They use
real backend objects, MT reset/init requests, packet dispatch, event handlers,
Space queues, and collection without original assets or socket traffic.

- State and supplied owner survive actual collection while queued; cancellation
  and queue retirement make them collectible exactly once.
- Immediate/nested ordering, arbitrary C++ exceptions, retained throwing queues,
  cleanup, same-object rebinding, and new initialize/configure ordering.
- Old Game messages and responses are retired by actual reset; new work runs.
- A barrier holds the session mutex while a real producer extracts its batch;
  real reachability is checked before destructive collection; the positive path
  runs actual collection before queue registration, then the producer resumes.
  The fixture State is independently rooted. Cleanup always unlocks before join.
- Events survive both that handoff and later queue collection; execution,
  cancellation, and immediate throwing handlers release their roots.
- Actual Client/Server packets queued in an old session are rejected; fresh
  packets resolve tile references in the bound fixture world. Client responses
  follow the same boundary.
- A real final download packet queued before reset cannot resume afterward;
  a fresh completion still performs its header validation.
- Animation requests are destroyed before dispatch, with IDs retained by value;
  tile-lock batches survive their producing stack and cancellation draws no RNG.

Run from the repository root (the native filter requires PR57 or its integration):

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^test session' --memorydebug --prefix build/session-test-data
```

Normal `--gse-tests` also includes these cases. Reset tests themselves own GC
exclusion; they do not deliberately race native Map deletion with collection.

## Validation on the implementation revision

The base is `a16fa30b4f8d1a812d306cf7a719de318cbcba3a`. Linux Clang 15 Debug
full CTest passed 8/8 in 37.45 seconds. The eleven focused native cases passed
with `--memorydebug` in 1.067 seconds. ASan+UBSan+LSan full CTest passed 8/8 in
44.36 seconds, and its focused session run passed in 1.067 seconds with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The existing local sanitizer
build uses a non-PIE linker diagnostic workaround; no build policy was changed.
Five changed production translation units passed the existing analysis policy
in Debug and Release; the final Game-only connection guard was rechecked in
both configurations. Native Windows validation is left to integration.

Logs are retained under `/home/jhurliman/src/glsmac-tracked-native-test-filter/normal/`
and `/home/jhurliman/src/glsmac-unit-despawn-rollback/asan/`, named
`session-final-build.log`, `session-final-focused.log`, and
`session-final-ctest.log`. Analysis logs use `analysis-session` plus
`analysis-session-game-final`; the Release database/logs are under
`/home/jhurliman/src/glsmac-tile-energy/release-analysis/`.

Copied-TU negative controls, never applied to repository production sources,
are retained in `/home/jhurliman/src/glsmac-session-retirement-controls/`:

| Control | Result |
| --- | --- |
| Disable the retired-session guard | Exit 1; one passed, ten failed: old callbacks and packets reach replacement state. |
| Omit temporary event-batch root publication | Exit 1; both handoff cases fail the reachability assertion at extraction-to-registration. |
| Restore the original animation request `&r` capture | Exit 1; ASan reports heap-use-after-free after the MT request is destroyed. |

The missing-root negative uses `--no-gc` to prevent unsafe destructive cleanup;
it verifies actual Game reachability while the producer is paused, **not**
collection of the negative event. The fixed tests run with GC enabled and do
perform collection at that boundary. An initial control without the reachability
precheck only failed later reclamation because `--no-gc` also disables manual
GC::Iterate; that log is retained as `handoff-unrooted-invalid-initial-control.log`
and is not counted as handoff evidence.

The control manifest records exact compile/link/run argv and working directories,
production/copied/test source hashes, and log hashes. Its SHA-256 is
`de6aedc91aba6ae807fe4e06380dac082504f2f67f4a5daa60dbcbaa1c6f03bc`.

## Separate production-reset diagnostic

The external, copied-TU diagnostic at
`/home/jhurliman/src/glsmac-reset-collector-audit/` used production Game, Space,
and GC from `fedf33617dcfeea6f36b498465331eba4ccdf76d`. Only the native Backend
test translation unit was substituted when linking. No repository production
source or original asset was changed. Exact compile/link/run commands, source,
objects, binaries, logs, and hashes are retained there.

An ordinary GC root paused **actual** root traversal after Space acquired its
collection lock. A separate worker then processed actual `OP_RESET`. Map/Tile
destruction observers only read the barrier state; they never dereferenced
retired objects. The released-barrier control reported two destructions with
zero overlap. With the collector paused, reset returned success before release
and both destruction notifications occurred while collection still held its
lock. The diagnostic intentionally failed its exclusion assertion: one passed,
one failed, exit 1. Normal run: 1.016 seconds. ASan+UBSan+LSan run: 1.067 seconds,
same assertion failure, no sanitizer diagnostic. This proves missing exclusion;
it does not intentionally perform a use-after-free or prove a sanitizer race.

Source SHA-256:
`52a233901967e581c80a24270d70048b8c3a8c6d0d37ed8ddce5f09aa8e5e228`.
Evidence-manifest SHA-256:
`5d8adc380b4eacb87b6f4895d921d08da9766256d13898d838a042a299e14527`.
The retained comparison against `a16fa30b` shows only snapshot-state and turn
status additions to ResetGame, without collection exclusion; the diagnostic
was not rerun against that newer revision. It does **not** establish the cause
of the previously observed intermittent Windows suite timeout.

A lock-only reset change is unsafe: current owner-thread frontend stop paths
can wait for backend work, and State packet processing can queue callbacks.
Holding collection exclusion across such queueing would invert Space's
pending-queue/collection lock order. Native publication/destruction and the
pending/running/cancelled owner-transition design require separate proof.
