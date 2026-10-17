# Terminal connection cleanup and collection

`State::Iterate` now uses the existing `Space::WithNativeMutation` exclusion for
the native cleanup after `Connection::IterateAndMaybeDelete` returns true. That
scope detaches the remaining Connection, deletes State-owned Players, and clears
the Slot and connection-ID registries. Existing owner accumulations reuse their
exclusion without relocking it.

Player destruction invalidates linked GSE Objects through `Wrappable`. Collection
can independently destroy an Object and unlink it from the same Player. The
terminal native poll path previously retired Players without excluding those
collector operations.

The guard starts only after polling and its native callbacks return. Transport
responses, callbacks and queued script work remain outside it. The guarded tail
only changes native ownership and invalidates wrappers; it does not queue Space
work or wait for transport. `Connection::SetState` only assigns its native State
pointer. This preserves the existing collection/queue lock ordering.

## Ownership boundary

There are two existing terminal paths. A failed connection can leave
`State::m_connection` non-null until this cleanup. The production initialization
disconnect callback can already have detached it. Cleanup must retire the
Players and registries in both cases. In the latter path the actual Game
admission retains its Connection after State detachment, so collection during the
callback does not destroy the Connection still returning from its poll. This
change does not introduce a generic poll root or alter that admission lifetime.

This is not general roster synchronization. Player/Slot adoption and replacement,
Server-side player departure, queued shallow Player copies, canonical Faction
identity, and arbitrary connection setters remain separate work. The regression
does not establish the cause of the historical untraced Windows test timeout.

## Regression

Four native cases exercise actual Network MT requests/responses, Client polling,
and State cleanup: non-null versus already-detached Connection, each from a
foreign native thread and an existing owner accumulation. Only socket operations
are synthetic. The detached cases use actual `MT_Init` admission and the real
native initialization-binding phase to install the production disconnect handler;
they do not generate or start a full game world.

The foreign-thread cases pause the native callback, drain its queued script
notification, and enter an actual collector root-traversal barrier. Collection
must enter while the callback is paused, confirming that polling is outside the
new guard. The callback then returns while collection stays paused. A dependency
observer records actual Player destruction; it must not happen during this held
interval. The bounded observation is an overlap regression, not a scheduling or
historical-hang diagnosis. All cleanup paths release both gates before joining.

The cases also check pending-response behavior, retained Player-wrapper
invalidation, exactly-once retirement, empty Player/Slot/CID registries, repeated
polling, and the detached Client's survival through its actual admission root.
The local transport and destruction observer outlive the final Client collection.
No assertion depends on allocator reuse or dereferencing retired native storage.

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^terminal connection cleanup' \
  --memorydebug --prefix build/terminal-connection-test-data
ctest --test-dir build --output-on-failure
python3 tools/analyze_cpp.py --build-dir build \
  --source src/game/backend/State.cpp --output-dir build/terminal-analysis
```

The filtered command and explicit analysis selector require their respective
test-tooling prerequisites. On older publication branches, ordinary `--gse-tests`
and the normal CTest suite include these four cases without those selectors.

## Local evidence

Validation used Clang 15 Debug on Ubuntu under WSL, based on faction-exclusion
source `c42d4bfe9fd725f373affbbffeeb14ac617aad16`. These are local Linux results,
not native Windows/macOS or hosted-check claims. The existing local sanitizer
non-PIE linker workaround remains a diagnostic setting, not a build-policy change.

- Original State implementation with the four new tests: two owner cases passed,
  both native cases failed `terminal Player retirement crossed an active
  collection`; exit 1, no tracked leaks.
- Fixed focused tracking: four cases passed in 1.027 seconds.
- Normal full CTest: 8/8 passed in 45.92 seconds (45.956 including runner).
- Focused ASan/UBSan/LSan with leak detection and halt-on-error, also with tracking:
  four cases passed in 1.082 seconds.
- A copied original `State.cpp` TU linked against the final fixture independently
  repeated exactly two passes/two synchronization failures, exit 1, in 1.047
  seconds. Intended source was not reverted for this control.
- Correctness analysis of the changed production TU: zero failures; 1.68 seconds
  reported analysis time (3.955 including runner).

Commands, copied original source, fixture, logs and hash manifest are retained in
`/home/jhurliman/src/glsmac-terminal-connection-retirement/`.

## Focused publication

The publication is stacked on faction-exclusion PR #106 at
`b3e8ebaad54a5e4d9273a070c30efbe343d814f7`. Cherry-pick `8ef3050a` of source
`4906ff19aac5fcf256d1b5eed3dc97857ecf86fb` applied without conflicts; the production
State file and the four new test bodies are unchanged. This older stack has no
native-filter CLI. Its normal CTest run passed all six registered tests in 25.59
seconds, and the main-suite log explicitly contains all four terminal-cleanup
case names followed by success. No test-tooling dependency or repeated sanitizer
run was added solely for publication. The build used four jobs. Publication logs
are retained in `/home/jhurliman/src/glsmac-terminal-connection-publication/`.
