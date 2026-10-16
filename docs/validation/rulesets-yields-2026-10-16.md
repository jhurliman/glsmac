# Ruleset, resource and memory-debug checks — 2026-10-16

These are GLSMAC integration checks on aleph0, not original-game comparisons
or completion claims for either ruleset. Original assets were loaded locally
from the purchased GOG Planetary Pack; none are included in the repository.

## Reconstructible source

The successful interactive runs used
[`314bdc4c`](https://github.com/jhurliman/glsmac/commit/314bdc4c81cf8fde22cab2b855302163fa8a592f),
published on the immutable
[`codex/validation-20261016-ui`](https://github.com/jhurliman/glsmac/tree/codex/validation-20261016-ui)
branch. The subsequent checks through `4de263fc` are retained as ancestors of
the immutable
[`codex/validation-20261016-followup`](https://github.com/jhurliman/glsmac/tree/codex/validation-20261016-followup)
branch at `4de263fc548387dec7b0aa5ad04fba300cb648a4`. Checks after that revision
use the separate evidence pins identified below; they are not contained in this
older follow-up branch.

Short labels below resolve to these exact source revisions:

| Label | Full commit |
| --- | --- |
| `314bdc4c` | [314bdc4c81cf8fde22cab2b855302163fa8a592f](https://github.com/jhurliman/glsmac/commit/314bdc4c81cf8fde22cab2b855302163fa8a592f) |
| `ac7086c0` | [ac7086c07027688e50de76b715cc45bf5500cb0b](https://github.com/jhurliman/glsmac/commit/ac7086c07027688e50de76b715cc45bf5500cb0b) |
| `2ce03a96` | [2ce03a9697458cda3ba9f19bc44d6219f45faf71](https://github.com/jhurliman/glsmac/commit/2ce03a9697458cda3ba9f19bc44d6219f45faf71) |
| `100971a3` | [100971a3c7b8f7a1f76e516ee626da10963c209b](https://github.com/jhurliman/glsmac/commit/100971a3c7b8f7a1f76e516ee626da10963c209b) |
| `4de263fc` | [4de263fc548387dec7b0aa5ad04fba300cb648a4](https://github.com/jhurliman/glsmac/commit/4de263fc548387dec7b0aa5ad04fba300cb648a4) |
| `d57fc5a3` | [d57fc5a3f74ffc107884ae22439bed0a93a85121](https://github.com/jhurliman/glsmac/commit/d57fc5a3f74ffc107884ae22439bed0a93a85121) |
| `fa69e2d4` | [fa69e2d4c4722932b0241dd505fd66f85b8b9123](https://github.com/jhurliman/glsmac/commit/fa69e2d4c4722932b0241dd505fd66f85b8b9123) |
| `b36c56d2` | [b36c56d2c6599f51876ce1d2a71b7c3c395873a7](https://github.com/jhurliman/glsmac/commit/b36c56d2c6599f51876ce1d2a71b7c3c395873a7) |
| `3ddccad0` | [3ddccad0c5b6579784f0065012529a463083f2a7](https://github.com/jhurliman/glsmac/commit/3ddccad0c5b6579784f0065012529a463083f2a7) |
| `ddcf747a` | [ddcf747ad41d4035e9a2d844ed1c2da8a9dcd61a](https://github.com/jhurliman/glsmac/commit/ddcf747ad41d4035e9a2d844ed1c2da8a9dcd61a) |
| `9d258151` | [9d2581512718470e6dadbd5131303d2bd9d2cdc3](https://github.com/jhurliman/glsmac/commit/9d2581512718470e6dadbd5131303d2bd9d2cdc3) |

The `d57fc5a3` and `fa69e2d4` revisions are retained on the immutable
`codex/validation-20261016-host-integer` branch at the full `fa69e2d4` revision
above.

The later three pins are retained on immutable branches
`codex/validation-20261016-world-widget`,
`codex/validation-20261016-snapshot-bonus`, and
`codex/validation-20261016-unit-ui`, respectively.
The renderer/native-test integration at `9d258151` is retained on
`codex/validation-20261016-renderer-native`.

The constructor-failure experiment below has a separate pinned source and
does not depend on a moving PR branch.

Windows builds used the native Debug `x64-debug-win-clang` preset. All interactive
runs used a 1280 × 800 window and separate configuration prefixes; quickstart
maps were 32 × 32 with an unpinned random seed. Process exit codes were captured
using an explicitly waited process. Supporting stdout/stderr logs remain local.

## Explicit ruleset chooser

At `314bdc4c`, launch without CLI quickstart or ruleset selection, then:

1. Main menu → Quick Start → SMAC reached a map with Morgan Industries.
2. Menu → Game → Start New Game → Yes returned to the main menu.
3. Quick Start → SMAX reached a map with Courage: To Question. The log loaded
   `Faction_Base_USURPERS`, confirming expansion roster content.
4. Title-bar close completed with exit code 0 and empty stderr.

Local logs: `out/ruleset-fixed-ui-stdout.log` and
`out/ruleset-fixed-ui-stderr.log`; prefix: `out/ruleset-fixed-ui-20261016`.
This verifies chooser/content initialization and replacement in one process.
It does not verify SMAC/SMAX rule fidelity. Multiplayer Cancel/Escape history
was subsequently fixed and tested through the actual popup scripts; this
interactive sequence did not exercise that navigation path.

## Signed resource display and worker interaction

At the same commit, a disposable copy of the open-source `GLSMAC_data` directory
overrode only the `get_tile_resources` callback. Base-center tiles returned
N/M/E `1/1/1`; other tiles returned `-2/300/3`. This is a synthetic display
fixture, not an original rule. The normal data directory remained unchanged.

Launch flags after the local `--smacpath` and isolated `--prefix` were:

```text
--smactype gog --datapath <fixture-data> --windowed --window-size 1280x800 --quickstart --quickstart-mapsize 32x32 --ruleset smac
```

The Hive's base screen showed negative/large numeric tile amounts and the
ordinary energy icons. Totals were N `-1` with consumption `2` and surplus
`-003`, M `301`, and E `4`. Unassigning the worked tile changed totals to
`1/1/1`; reassigning restored `-1/301/4`. Labels remained visible after
maximizing/restoring the window and closing/reopening the base screen.
Title-bar close exited 0 with empty stderr. No further turn was advanced.

Local logs: `out/yields-mixed-plain-ui-stdout.log` and corresponding stderr;
prefix: `out/yields-mixed-plain-ui-20261016`. The local fixture path is retained
in `out/yields-ui-data-path.txt`. Extreme 20-digit label legibility was not
tested. The later overflow-safe native balance formatter has headless coverage
below; it was not part of this interactive build.

## Combined headless checks

| Commit | Check | Result |
| --- | --- | --- |
| `314bdc4c` | Linux / Windows CTest | 5/5 in 24.89 s / 25.25 s |
| `314bdc4c` | Linux ASan/UBSan CTest | 5/5, first pass, 26.40 s |
| `314bdc4c` | LLVM 15 correctness policy | 284 production translation units, 58.63 s, zero failures |
| `2ce03a9697458cda3ba9f19bc44d6219f45faf71` | Linux / Windows CTest | 5/5 in 30.79 s / 24.86 s |
| `2ce03a96` | Linux ASan/UBSan CTest | 5/5, first pass, 34.72 s |
| `2ce03a96` | LLVM 15 correctness policy | 285 production translation units, 53.99 s, zero failures |
| `100971a3` | Windows CTest, including focused UUID tracking | 6/6, 23.97 s; focused UUID check 1.14 s |
| `4de263fc` | Linux CTest, including focused UUID and mesh tracking and the resource-table parser | 7/7, 32.17 s; focused checks 1.08 s and 1.05 s |
| `d57fc5a3` | Linux / Windows CTest, including constructor registration and consuming rectangle fixes | 7/7 in 33.01 s / 27.28 s |
| `fa69e2d4` | Linux / Windows CTest, including host-rule synchronization and checked integer arithmetic | 7/7 in 49.21 s / 28.76 s |
| `fa69e2d4` | Linux ASan/UBSan CTest | 7/7, first pass, 44.23 s |
| `fa69e2d4` | LLVM 15 correctness policy | 286 production translation units, 64.53 s, zero failures |

CTest commands use `--output-on-failure`; analysis uses
`python3 tools/analyze_cpp.py --all --jobs 8` after reconfiguring its database.
Linux is WSL Ubuntu 22.04. The Windows-only network implementation is explicitly
outside the Linux analyzer's scope. The sanitizer cache uses local `-no-pie`
to avoid the previously observed empty-output startup failure, with
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`.
These are not leak-check results or a portable toolchain-policy change.

Linux logs are under `/home/jhurliman/src/glsmac-gc-audit/`, using prefixes
`roster-int64-`, `yields-roster-asan-`, `yields-analysis`, `growth-endpoint-fixed-`,
`growth-balance-`, and `parser-mesh-integrated-`. Windows logs under `out/` use
`roster-int64-windows-`, `growth-balance-windows-`, and `mesh-uuid-windows-`.
Later checks use `constructor-mesh-integrated-`, `host-integer-` and
`host-integer-asan-` on Linux, `constructor-mesh-windows-` and
`host-integer-windows-` on Windows, and `host-integer-analysis` for the scan.

## GC registration after constructor failure

At baseline `100971a3`, a GC object registers during its base constructor.
If a derived member or constructor body then throws, C++ destroys the base and
frees the allocation, but the accumulation still retains that address. Publishing
it allows a later collection to inspect freed memory.

The regression and fix are pinned at
[491381b9c532218f9ca71370b68734e4f8bdf01c](https://github.com/jhurliman/glsmac/commit/491381b9c532218f9ca71370b68734e4f8bdf01c),
also retained on `codex/validation-20261016-gc-construction`. To reconstruct the
negative control, check out the full `100971a3` revision above in an isolated
worktree and apply only the `src/gse/tests/GSE.cpp` diff from that commit's
parent to that commit. This adds four actual GC cases: member/body failure,
each caught within a nested accumulation or escaping it. No production fix is
included in that negative control. Build Debug with `SANITIZER=ON` and run
`bin/GLSMAC --gse-tests --quiet --prefix <isolated-prefix>`. These are native
GSE tests, not separate registered lifecycle CTests or a native-test CLI filter.
The negative runs used `UBSAN_OPTIONS=halt_on_error=0:print_stacktrace=0` so
ASan could report the heap-use-after-free after UBSan's invalid-vptr diagnostic
at `Space.cpp:232` during collection. To reach the member failure after the
body failure aborted first, the second negative run temporarily reversed the
new `fail_in_member` loop order, then restored it. Stopping UBSan on its first
error produces only its earlier diagnostic. Completed children must still be collected
exactly once, rooted survivors retained, and subsequent accumulations usable.

For the fixed focused run, a temporary `return;` immediately after
`tests::AddGSETests(task)` in `src/gse/tests/Tests.cpp` limited registration to
18 basic, mesh and GSE native tests. After rebuilding, the same CLI passed in
2.07 s with leak detection enabled and ASan/UBSan stopping on first error.
That registration edit was restored and the complete executable rebuilt;
no filtering feature was committed. Normal CTest passed 6/6 in 28.41 s;
ASan/UBSan CTest passed
6/6 in 38.45 s with leak detection disabled. The full sanitizer run with default
leak detection still reported the existing parser allocation baseline of
201 bytes in three allocations (`parser/JS.cpp`, through `Scripts.cpp:73`).
It is not a clean full-suite leak result. This fix covers pending construction
registration; it does not establish out-of-memory safety or support manual
deletion of already-published GC objects.

Local logs are under `/home/jhurliman/src/glsmac-gc-constructor-unwind/`:
`negative-address.log`, `negative-member-asan.log`, `focused-lifecycle-asan.log`,
`fixed-normal-ctest.log`, `fixed-asan-no-leak-ctest.log`, and `fixed-asan-ctest.log`.
This headless experiment does not identify the cause of any GUI access violation.

## Earlier memory-tracked interactive failures

The ordinary interactive successes above omitted `--memorydebug`. Subsequent
tracked runs exposed separate failures, each retained rather than treated as
a successful retry:

- At `314bdc4c`, making all tiles return `-2/300/3` reached an empty-base
  starvation error. The existing initialization path attempted to remove a
  nonexistent citizen. A positive-center fixture then exposed a null-storage
  texture upload rejected by pointer tracking. Both have focused fixes.
- At `ac7086c0`, the all-negative fixture reached the map/HUD before an abort
  during cleanup (exit 3). First-chance CDB tracing found a mesh getter applying
  a heap-tracking check to a caller's stack output. The cleanup error followed
  the interrupted geometry initialization. That getter has a focused fix.
- At `100971a3`, including the getter fix, a new disposable all-negative fixture
  failed without user input, exiting `-1073741819` (access violation). CDB
  subsequently captured a stale tracking entry after consuming rectangle
  conversion, followed by rethrows into the error handler. That debugger run
  did not reproduce the access violation; do not equate its final outcome with
  the uninstrumented run.
- At `d57fc5a3`, including both consuming-rectangle and GC construction fixes,
  the tracked run reported a new double-allocation at `Widget.cpp:54` and
  exited 1 after acknowledgement of its application error dialog. First-chance
  CDB tracing found that the attempted and stale allocations were both the
  widget's `scene::actor::Mesh` at that line, with size `0x3c0`. The DOM owner
  used plain deletion after tracked allocation. That debugger process was
  deliberately stopped after capturing the first error; it supplies no
  additional shutdown result. Logs: `out/constructor-mesh-ui-stdout.log`, its
  stderr file, and `out/widget-memory-cdb.log`.

The preceding local logs are `out/mesh-memory-ui-stdout.log`, its stderr file,
and `out/mesh-memory-cdb.log`. Earlier first-chance evidence is in
`out/yields-memory-cdb-fast.log`. No complete interactive memory-tracked pass,
heap-leak result, soak result, save/reload result, or multiplayer convergence
result was established by these earlier runs. Later results are recorded below.

## Tracked two-world runs and remaining allocations

At `b36c56d2`, the all-tile `-2/300/3` fixture reached Sparta Command.
The base showed N `-4`, consumption `2`, surplus `-006`, M `600`, E `6`.
Removing the worked tile changed these to N `-2`, surplus `-004`, M `300`,
E `3`; restoring it returned the original totals. Closing the base and choosing
Menu → Game → Start New Game → Yes produced another quickstart world.
Title-bar close exited 0 with empty stderr. Shutdown nevertheless reported
384 tracked objects and 785 raw allocations retained, including Surface and
Widget actors. Logs: `out/world-widget-ui-stdout.log` and its stderr file.

At `ddcf747a`, including the detached-root actor/texture repair, the same
sequence reached The Hive and then University Base. The worker interaction
produced the same synthetic totals above. Both maps rendered and title-bar
close exited 0 with empty stderr. Shutdown reported 20 tracked objects and
60 raw allocations retained: 18 object entries at `Render.cpp:172`, two at
`FBO.cpp:261`, and raw storage at `Mesh.cpp:17/18` and `Texture.cpp:76`.
No OpenGL resource warning group was emitted. These are functional tracked
passes, not clean leak checks. The worlds had different random seeds, so the
counts are observations rather than a controlled before/after leak measurement.

Both runs used the disposable data path in `out/mesh-memory-ui-data-path.txt`,
with original assets remaining in the local GOG installation. The later run's
prefix is `out/detached-ui-20261016`; stdout/stderr use `out/detached-ui-`.
The launch options after the local asset/data/prefix paths were:

```text
--smactype gog --windowed --window-size 1280x800 --quickstart --quickstart-mapsize 32x32 --ruleset smac --memorydebug
```

The live executable remained `ddcf747a` throughout this run even though source
work continued afterward. No turn advance, full save/load, multiplayer or
long-running soak result follows from these interactions.

## Later headless results and fixture timeouts

| Commit | Check | Result |
| --- | --- | --- |
| `b36c56d2` | Windows CTest | 7/7, 30.36 s |
| `3ddccad0` | Linux ASan/UBSan CTest | 7/7, 36.69 s |
| `3ddccad0` | LLVM 15 correctness policy | 288 production translation units, 56.14 s, zero failures |
| `3ddccad0` | Windows CTest | Main GSE test timed out at 300.11 s; other six passed; total 310.33 s |
| `3ddccad0` | Unchanged Windows main-GSE retry | Passed, 18.43 s; does not erase the timeout |
| `ddcf747a` | Linux ASan/UBSan CTest | 7/7, 36.75 s |
| `ddcf747a` | Windows CTest | Main GSE passed in 18.03 s; focused mesh script timed out at 30.19 s; remaining five passed; total 61.14 s |

The world-download fixture previously joined a worker and drained queued
accumulations while already holding an outer accumulation lock. Its repair
separates setup, dispatch/drain and assertions without changing production
locks. Reintroducing the old nesting now fails the two fixture preconditions
promptly (203 passing / two expected failures in 25.11 s), and the restored
source worktree passed all seven CTests in 35.92 s. That repair is included in
`ddcf747a`; its successful main GSE test is distinct from the later mesh timeout.

The focused mesh script's new UI fixture creates a nested GSE runtime while
inside the script's accumulation. Source inspection identifies an inversion
between the active space lock and the global GC spaces lock. Nine unchanged
direct Windows retries exited 0, so no stuck retry was available for a Windows
stack capture. A separate native-test selection repair is in progress; these
retries do not establish that the inversion is absent.

Windows logs: `out/snapshot-bonus-windows-ctest.log`,
`out/snapshot-bonus-windows-gse-retry.log`, `out/unit-ui-windows-ctest.log`,
`out/mesh-timeout-probe-stdout.log`, and numbered probe logs 1–8.
Linux logs under `/home/jhurliman/src/glsmac-gc-audit/` use
`snapshot-bonus-asan-`, `snapshot-bonus-analysis`, and `unit-ui-asan-`.
Sanitizer options and their leak-detection limitation remain as recorded above.

## Native ownership fixtures and renderer follow-up

At `9d258151`, generated sprite meshes have a tracked smart-pointer owner and
the minimap consumes capture-response textures after copying. The focused
memory-debug CTest now runs eight native ownership cases directly, outside
script accumulation. The script bridge retains only three pure mesh checks.
Invalid or unmatched native filters fail with exit 1; CLI checks also exercise
selector conflicts and verify that scripts are not run in native-filter mode.

| Check at `9d258151` | Result |
| --- | --- |
| Native Windows Debug CTest | 8/8, 51.05 s; full GSE 19.49 s; tracked ownership 1.42 s; filter CLI 11.45 s |
| Linux ASan/UBSan CTest | 8/8, 41.63 s; tracked ownership 1.06 s |
| LLVM 15 correctness policy | 291 production translation units, 56.28 s, zero failures |

The first sanitizer invocation could not load the local SDL image shared
library and ran no tests successfully. Supplying the existing dependency prefix
in `LD_LIBRARY_PATH` produced the passing run above. Leak detection remains
disabled for the separately recorded parser baseline. Passing these checks
does not establish freedom from intermittent concurrency failures.

The tracked Windows sequence used the same disposable all-tile `-2/300/3`
fixture. New Jerusalem showed N `-4`, consumption `2`, surplus `-006`, M `600`,
E `6`; removing its worked tile changed these to N `-2`, surplus `-004`, M `300`,
E `3`, and restoring it restored the original totals. Closing the base and
starting a new game in the same process produced The Hive with a rendered
minimap. Title-bar close returned 0 with empty stderr.

Shutdown emitted no tracked-object or OpenGL warning group. It still reported
22 raw bitmap allocations at `Texture.cpp:76`. This is not a leak-clean result.
The different random worlds preclude treating total counts as a controlled
before/after measurement. Surface background replacement is the next focused
ownership investigation. Turn advancement, save/load, networking and a long
soak were not exercised in this sequence.

The configuration prefix is `out/renderer-native-ui-20261016`; Windows logs use
`out/renderer-native-ui-` and `out/renderer-native-windows-`. Linux logs under
`/home/jhurliman/src/glsmac-gc-audit/` use `renderer-native-asan-` and
`renderer-native-analysis`. The published evidence pin contains the tested
source, not the locally installed assets or these machine-local logs.
