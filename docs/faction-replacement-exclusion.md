# Native faction replacement and collection

`State::Deserialize` now runs its existing settings and faction-manager decoders
inside `Space::WithNativeMutation`. A native settings packet can therefore replace
linked Factions only while collection is excluded. Calls already inside the
Space owner's accumulation retain that existing exclusion without relocking it.

The concrete native path is `Game::Iterate` -> `State::Iterate` ->
`Client::ProcessEvent(PT_GLOBAL_SETTINGS)` -> `State::Deserialize` ->
`FactionManager::Deserialize/Clear`. Faction retirement invalidates linked GSE
Objects through `Wrappable::~Wrappable`. The collector can independently destroy
an Object and unlink it from the same native Faction. Those operations previously
had no shared exclusion on this packet path.

Only the two native decoder calls are guarded. Client notifications, `OnOpen`,
`AnnounceGameState`, protocol-error handling, transport and queued script work stay
in the caller outside this scope. The current decoders allocate native values;
they do not create GSE values, queue callbacks or wait for network/renderer work.
The caller retains the State and its owning Space throughout the operation.

## Preserved behavior and limits

The wire format and failure semantics are unchanged. Global settings are decoded
first; faction-manager decoding clears the old registry and publishes complete
records in order. A later malformed record leaves earlier complete replacements.
This is not a transaction or rollback mechanism. Linked Faction wrappers become
undefined when retired; detached color values returned by `Color::Wrap` remain
valid independent values.

This scope does not repair Player/Slot adoption, borrowed faction identities,
queued lobby copies, disconnect cleanup, arbitrary direct FactionManager calls,
or roster-wide synchronization. Those remain separate ownership/publication
work. It also does not establish a cause for a historical Windows test timeout.

## Regression and local evidence

Four asset-free native cases send the actual `PT_GLOBAL_SETTINGS` packet through
Client dispatch: valid and malformed-later-record inputs, each from a foreign
native thread and from an existing owner accumulation. The foreign cases hold an
actual collector root-traversal barrier and observe actual Faction destruction
through a dependency notification. The receiver must defer retirement until the
barrier is released. Every exit releases the gate and joins the workers; no test
relies on dereferencing freed storage or reproducing a crash.

The cases also check linked wrapper invalidation, detached color survival,
retained complete-record behavior, protocol error/notification counts and
subsequent collection. In the valid native case the actual settings notification
starts another collector and observes its completion before returning; this
checks that notification work remains outside the replacement guard. Its bounded
wait records failure, with joining deferred until the packet callback has returned.

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^faction replacement' \
  --memorydebug --prefix build/faction-replacement-test-data
ctest --test-dir build --output-on-failure
python3 tools/analyze_cpp.py --build-dir build \
  --source src/game/backend/State.cpp --output-dir build/faction-analysis
```

Validation is local Clang 15 Debug on Ubuntu under WSL, based on map-publication
source `a5a411a88c130c2e604dd96c1e7fb1e946ad9df5`. Results are not claims of hosted
or native Windows/macOS validation. The existing local sanitizer non-PIE linker
workaround remains local and is not a build-policy change.

The original decoder with the corrected fixture passed both owner cases and
failed both foreign-thread retirement assertions (2 passed / 2 failed, exit 1),
with no tracking leak. A copied original State.cpp TU linked against the final
fixture independently repeats that negative control; intended source is never
reverted for the control. The first fixture draft incorrectly expected a detached
Color value to invalidate; its failed log is retained but is not defect evidence.

Fixed results (including runner time where stated):

- Four focused tracking cases passed in 1.07 seconds; no tracked leaks.
- Normal full CTest: 8/8 passed, 41.45 seconds including runner.
- ASan/UBSan/LSan with leak detection and halt-on-error: four focused tracking
  cases passed in 1.12 seconds; full CTest 8/8 passed in 45.43 seconds including
  runner.
- Focused correctness analysis of `State.cpp`: zero failures, 3.92 seconds.
- Independently copied original decoder: 2 passed / 2 failed, exit 1, 1.07 seconds.

Commands, original/copy source, logs and hash manifest are retained at
`/home/jhurliman/src/glsmac-faction-replacement-audit/`.
