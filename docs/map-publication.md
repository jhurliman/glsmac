# Map publication during initialization

Native map acquisition now constructs and fills an unpublished candidate. The
old current map moves to the existing retained-map root under the collector
exclusion introduced by [native retirement](native-world-retirement.md). After
successful acquisition and a final cancellation check, a second short excluded
operation publishes the completed candidate. Generation, file I/O, progress
callbacks, transport and presentation cleanup remain outside those new scopes.

A scope guard retires an unsuccessful candidate and its untransferred presentation
objects. It does not touch the current response envelope. The existing
`InitFailed` path can therefore restore the retained old map even when acquisition
fails before a new current map was published. The existing later old-map retirement
still invalidates its Map/Tile wrappers under collector exclusion.

The owner-thread world-download decoder is a separate path: dependent unit/base
records need the receiving map during decoding. A rejected body can leave its
empty Map rooted between the Ingress and Failure checkpoints. Map reachability
now treats absent tile storage as an empty child set. This null check alone would
not make native mutation of published tile storage safe; private acquisition and
short guarded publication address that distinct boundary.

## Contract and limits

The initialization coordinator continues to exclude overlapping owner transitions
and native setup/reset. These changes add collection exclusion only at native
map-root publication. A private candidate has no exposed script wrappers or
renderer borrowers. Its map and tile destruction can consequently happen outside
collection exclusion; presentation objects are detached and released through the
existing ownership helpers.

This does not synchronize arbitrary later Map mutation, roster or faction
publication, or all Game fields. It does not establish rendered map-reload
correctness, original-game behavior equivalence, or the cause of the previously
observed intermittent Windows test timeout. Allocation-failure recovery during
partial Map construction is outside this change.

## Regression coverage

Nine short native cases are registered in the normal GSE suite. They cover:

- Real native `AcquireInitializationMap` file loading and cancellation, with
  actual collection during the constructor's source-texture loads. The candidate
  stays private and the old map remains rooted. The completed tile bytes match
  the synthetic input; cancellation restores old ownership through `InitFailed`.
- Real collector barriers around old-root transfer and candidate publication.
  Both mutations must wait while collection holds its exclusion. Cleanup always
  releases the barrier and joins both threads.
- Malformed loading, cancellation, and a native exception after candidate setup;
  counted candidate-only texture/mesh objects are released exactly once and old
  map wrappers remain valid through restoration.
- A failed map-body decode followed by actual collection while its empty map
  remains rooted.
- A real Client final-chunk packet, coordinator Ingress execution, actual
  collection, and the later Failure checkpoint. This specifically exercises the
  empty-map interval that previously dereferenced null tile storage.

The constructor fixture supplies only two synthetic one-pixel source textures,
through the actual resource and texture-loader interfaces. It rejects any other
path or region load. Engine services are scoped and swapped under the existing
GC service mutex outside accumulation; the real runtime and transport outlive
all queued work. Native acquisition also supplies the Engine backend required
by the real slot notification. The packet test uses the real transport request
machinery with a local no-socket transport. No original assets or renderer output
are required, and generation geometry is not asserted by these tests.

Run the focused native tests in a Debug build containing the native-filter CLI:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^map publication' \
  --memorydebug --prefix build/map-publication-test-data
ctest --test-dir build --output-on-failure
```

## Validation evidence

Source base: `848d15512883b6319cb3a1d766c9857e43b87497`, which includes
initialization coordination plus the polled controller and Engine shutdown
prerequisites. Validation used Clang 15 Debug on Ubuntu under WSL. These are
local source-branch results, not native Windows/macOS or hosted PR results.

- Focused tracking: all nine cases passed in 1.07 seconds, with no tracked leaks.
- Normal full CTest: 8/8 passed in 41.43 seconds (41.46 seconds including runner).
- ASan/UBSan/LSan (`detect_leaks=1`, halt on error): all nine tracked cases passed;
  full CTest 8/8 passed in 46.82 seconds including runner. The existing local
  non-PIE diagnostic linker option was retained; no build-policy change is part
  of this patch.
- Focused correctness analysis of `Game.cpp` and `map/Map.cpp`: zero failures,
  18.72 seconds including runner.
- Copied production TU with both new root-publication guards removed: both
  collector-barrier assertions failed, exit 1, without relying on a crash.
- Copied production TU restoring unconditional tile traversal: the actual
  final-chunk interval crashed with SIGSEGV; its sanitizer variant exited 1 at
  `Map::GetReachableObjects` with a null `Tiles` member call. The intended source
  remained unchanged throughout both controls.

Retained local commands, copied controls, logs and hash manifest are under
`/home/jhurliman/src/glsmac-map-publication-audit/`. An initial acquisition fixture
run lacked the Engine backend needed by `Slot::Update`; its failed log is retained
separately and is not counted as production-defect evidence. All results above
use the corrected fixture.

### Integrated and publication validation

The integration at `8f772f95` passes native Windows Clang Debug CTest 8/8
in 55.94 seconds and Linux ASan/UBSan/LSan CTest 8/8 in 44.95 seconds.
The actual Engine shutdown harness also passes its default and single-thread
tracked sanitizer runs. Logs are retained as `out/map-publication-windows-*`
and `/home/jhurliman/src/glsmac-gc-audit/map-publication-*` respectively.

The narrower publication at `97aecc0f` passes its six registered CTest cases
in 26.43 seconds, including the full GSE suite with the nine new regressions.
Its only cherry-pick adaptation removes a duplicate `<filesystem>` include;
production code is unchanged. The publication does not include the later native
test-filter CLI, so its focused command above requires the source/integration
build. These runs add no rendered restart, native Mac or original-game claim.
