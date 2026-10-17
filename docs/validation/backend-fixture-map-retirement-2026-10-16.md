# Backend fixture map retirement, 2026-10-16

The Windows full `gse` CTest at integration revision `519a0bd7` timed out after
printing `real final download chunk rejects invalid world profile before map
allocation queued`. The retained log is `out/turn-windows-ctest.log` in the
primary checkout. A retry of that binary passed, as did twenty standalone
repetitions. `out/turn-windows-timeout-stacks.log` was captured after the retry
had moved on and is **not a stack trace of the timeout**.

This follow-up fixes a concrete fixture lifetime race found during source
inspection. It does not claim to have reproduced or explained that timeout.

## Finding and change

`BackendFixture::DestroyMapBeforeManagers` previously destroyed its owned Map
and then cleared `Game::m_map`, without entering that fixture's GC accumulation
scope. Direct callers include the queued download regression. Concurrent
collection follows its registered GSE root through State, Game, Map and the
Map's Tile wrappers. Their destruction could therefore race with traversal,
including access to wrapper mutexes and linked script values.

The helper now enters the fixture's existing `Space::Accumulate`, clears the
published map pointer, and destroys the Map and its tiles inside that scope.
The same helper remains valid when the caller is already accumulating.
Managers remain rooted until the fixture runtime is destroyed.

Only fixture/test code changes. No collector locks, production reset behavior,
network scheduling or timeout policy changes. Production `Game::ResetGame`
caller/exclusion and queued-callback lifetime are a separate audit item; the
fixture helper no longer implies that its exclusion is inherited from that
production path.

## Deterministic regression

The new native test is
`test backend fixture retires map and tile wrappers under GC exclusion`.
Existing `Wrappable::Depend` notifications observe the real Map and one Tile
at destruction. Both must see an active fixture accumulation and an already
unpublished map. Retained script wrappers must become undefined, while the
base manager and its definition remain usable. The test covers direct,
nested and repeated empty retirement. Cleanup detaches any partially
registered observers if setup fails before destruction.

A copied test translation unit restored the old helper and retained the new
regression. Its actual native run exited 1 with **0 passed / 1 failed** at the
missing-exclusion assertion. This negative used `--no-gc` to avoid deliberately
racing freed storage: the destructor notification itself deterministically
observes the missing ownership exclusion. The intended source and normal
objects stayed unchanged. Fixed validation runs with the collector enabled.

## Validation and limits

Validation used the reused checkout based on `b14c26db`; its relevant fixture
retirement and collector code match the inspected integration path, but it
does not include the newer downloaded-turn/RNG cases from `519a0bd7`.

- Before the fix, twenty focused queued-test repetitions under ASan, UBSan and
  LSan passed (21.43 seconds). This is evidence of non-reproduction, not safety.
- Fixed normal CTest: **8/8**, 36.45 seconds.
- Fixed lifetime test plus immediate/queued invalid-download tests pass under
  ASan + UBSan + LSan and `--memorydebug`, with no reported sanitizer or
  allocation-tracker errors. Leak detection is enabled. The existing local
  non-PIE sanitizer workaround is unchanged.
- Independent source review confirmed the fixture race and bounded repair.
  Native Windows/macOS validation is left to integration; no stuck Windows
  stack or dynamic causal link to the original timeout was obtained.

Evidence is retained at `/home/jhurliman/src/glsmac-queued-download-audit/`:
`stress-summary.txt`, `asan-queued-*.log`, `negative-lifetime.log`,
`normal-focused.log`, `normal-ctest.log`, and `asan-focused.log`.

The test is registered in the existing full `gse` CTest, with no additional
full-suite invocation. A Debug build can run it as part of
`build/bin/GLSMAC --gse-tests`. Builds with the separate native-filter feature
from [PR #57](https://github.com/jhurliman/glsmac/pull/57) can select its exact
name using `--gse-tests-native-filter` and an anchored regular expression.
