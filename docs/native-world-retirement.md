# Collector exclusion for native world retirement

Native reset now excludes actual GC traversal while unpublishing world roots and
destroying Map/Tile wrappers. `OP_RESET` remains synchronous: its response is
published after native retirement and detached presentation cleanup finish.

This is a retirement boundary, not a general synchronization policy for map
generation, roster publication, arbitrary native mutations, or every connection
callback. It depends on the separately reviewed admission, owner checkpoint,
public reset polling, payload ownership, and Engine shutdown ordering changes.

## Execution and lock contract

`gc::Space::WithNativeMutation` executes inline when its thread already owns an
accumulation. Otherwise it takes only the Space collection mutex, synchronously,
and releases it on any C++ exception. It does not claim script-owner identity,
register an accumulation, or acquire the pending queue/global GC mutex.

The native body may retire native roots and invalidate existing wrappers. It
must not allocate GSE objects, queue an accumulation, invoke scripts, wait for
another task, or recursively enter this guard outside an owner accumulation.
Its caller keeps Space alive and establishes the native/owner execution boundary.

Collection uses `pending -> collect -> native root/container locks`. Retirement
uses `collect -> native root/container locks`, never `pending`. Admission locks
are released before acquiring the guard. No whole `State::Iterate` or transport
packet batch is put under the new lock.

The [initialization coordinator](initialization-owner-coordinator.md) cancels a
Pending claim before reset; a Running owner operation keeps native dispatch
parked through its complete RAII finish. An error reset from that same Running
owner already owns accumulation and executes inline. The
[polled public controller](polled-reset-controller.md) retires old frontend request
IDs outside accumulation and skips old callback stages while awaiting reset.
Engine teardown keeps the runtime/Space alive until Game stops in both thread
modes. These prerequisites are necessary; adding a collection lock to the old
blocking reset flow alone would be unsafe.

## Native and external cleanup

Ordinary reset retains its published State/Connection while requesting disconnect
and draining the outstanding network event request. The private
`Connection::DrainEventRequest` factors the existing cancellation wait from
handler retirement. No concurrent connection iteration may create another event
request during this operation. Existing `ResetHandlers` still drains when used
elsewhere; inside reset's guarded core the ID is already zero.

Under collection exclusion, reset clears callback/event/response roots, manager
pointers, pending snapshot bindings, native status and State's game link. It
detaches the owning map-response envelope and untransferred presentation with
the existing partial-alias rules. It also detaches MapState, whose tile-state
destructors own moisture/river textures. The Map pointer is unpublished before
the real Map and Tiles are destroyed, invalidating their surviving wrappers.

A native RAII owner releases the detached response, terrain assets and MapState
after the new guard returns. Texture destruction can notify graphics; it is not
performed inside the newly acquired foreign-thread guard. The audited Map/Tile
dependency notifications only invalidate existing native listener metadata;
they do not execute script callbacks or enqueue work.

The existing owner-error checkpoint still releases untransferred presentation
at its original point before invoking the error hook. It already owns an
accumulation; this change does not remove all preexisting renderer activity
from owner accumulations.

Game retains a borrowed owning Space independently of State. This covers failed
initialization that clears State but retains an old Map. Binding another Space
before Stop is rejected visibly in Release too. Stop preserves its original
no-transport-work behavior, because Network may already be stopping. It retires
both maps and Random wrappers before clearing the borrowed Space. It never owns
or deletes Space. MT response payload cleanup remains outside the guard.

The retained `m_old_map` is now a collector root. Successful `InitializeMap`
uses the same guarded old-map retirement helper as Stop; presentation cleanup
stays outside that helper's newly acquired guard. This does not claim the other
map construction/generation publication sites are synchronized.

## Regression evidence

Ten short native cases run in the ordinary GSE suite, without original assets,
sockets, or a graphics context:

- Actual GC root traversal is paused while another thread processes a real MT
  reset, or the real retained-old-map retirement helper. Destruction and response
  completion must remain blocked until traversal is released.
- The reverse order pauses an actual Map destruction notification; collection
  may traverse roots only after retirement finishes. Both cases verify real
  Map/Tile wrapper invalidation, once-only destruction and repeated empty reset.
- A real Network event request is paused while reset enters its actual
  `MT_Cancel` wait (observed through the request's atomic cancellation flag).
  Actual collection completes independently before the transport is released.
- Real complete/partial response aliases and MapState moisture/river textures
  are released exactly once after Map destruction. A second native guard can
  enter during presentation destruction, proving the foreign guard is released.
- Foreign-thread Stop invalidates Random and Map wrappers with a bound State
  and with a state-null retained old Map. Owner nesting and arbitrary-exception
  guard unwinding are followed by real collection.

Barrier cleanup always releases and joins the workers before captured fixture
objects die. Bounded waits detect crossing or missing progress; tests do not
dereference retired Maps or intentionally perform a use-after-free. The old-map
case exercises the exact production retirement helper, not a complete rendered
`InitializeMap` run.

```sh
./build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^native retirement' \
  --memorydebug --prefix /tmp/glsmac-native-retirement-tests
ctest --test-dir build --output-on-failure
python3 tools/test_engine_shutdown.py --build-dir build
```

The native filter requires the focused-test CLI prerequisite. The opt-in actual
Engine harness supports an already built matching Linux/WSL Ninja checkout; it
is not a claim of portable harness coverage or a second default full suite.

Local Clang 15 Debug validation on WSL:

- Ten focused tracked cases: 1.07 seconds, no tracked leaks.
- Full normal CTest: 8/8, 42.53 seconds.
- ASan/UBSan/LSan full CTest: 8/8, 45.28 seconds; the same ten focused tracked
  cases also pass. Leak detection and halt-on-error were enabled. The existing
  local non-PIE sanitizer linker workaround was retained; no build policy changed.
- Actual Engine default and single-thread tracked shutdown cases pass in both
  normal and sanitizer builds. The normal suite also includes public reset
  polling and the owner coordinator's actual reset/initialization overlap cases.
- Existing correctness analysis on the three changed production TUs: zero
  failures, 15.84 seconds. No warning policy was widened.

Three separate copied-production controls, linked against the real fixtures,
all exit 1: omitting collection exclusion fails four overlap cases; retaining
MapState inside the guarded Map destructor fails both presentation cases; moving
the transport cancellation wait inside the guard fails its independence case.
Repository production sources were never temporarily reverted. These are
exclusion/order assertions, not a claim that a sanitizer observed a data race.

Source ancestry is coordinator `1676caf6`, polling `b2ec36b8` (local cherry-pick
`78c027a6`) and Engine ordering `f586b523` (local cherry-pick `ddf82c5b`). Native
Windows/macOS execution of this follow-up remains an integration check. The
[earlier production diagnostic](session-callback-retirement.md#separate-production-reset-diagnostic)
records the original missing-exclusion proof; neither it nor these tests proves
the cause of the previously intermittent Windows suite timeout.

Commands, copied controls, source/log hashes and manifests are retained locally
under `/home/jhurliman/src/glsmac-native-reset-guard-audit/`. No original assets
or test output are committed.
