# Ruleset, resource and memory-debug checks — 2026-10-16

These are GLSMAC integration checks on aleph0, with separately identified
original-routine comparisons in later sections. Neither is a completion claim
for either ruleset. Original assets were loaded locally from the purchased GOG
Planetary Pack; none are included in the repository.

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
| `705cc939` | [705cc939727188be70828399733cf4740afb00e6](https://github.com/jhurliman/glsmac/commit/705cc939727188be70828399733cf4740afb00e6) |
| `68081411` | [680814117f0423c4668c1ebf623e665e1a917227](https://github.com/jhurliman/glsmac/commit/680814117f0423c4668c1ebf623e665e1a917227) |
| `0d98cde8` | [0d98cde8ae7416c0a2810c795ebf55de0c27c74b](https://github.com/jhurliman/glsmac/commit/0d98cde8ae7416c0a2810c795ebf55de0c27c74b) |
| `bf00f411` | [bf00f4118f04f3f36ee8dc562a0d2cea58084b3a](https://github.com/jhurliman/glsmac/commit/bf00f4118f04f3f36ee8dc562a0d2cea58084b3a) |
| `82222d37` | [82222d378e7f79bc25ad5a1d4b66395ee2bdc9ed](https://github.com/jhurliman/glsmac/commit/82222d378e7f79bc25ad5a1d4b66395ee2bdc9ed) |
| `c9aafc83` | [c9aafc83a0e55870af33e0f479c11db064079a7b](https://github.com/jhurliman/glsmac/commit/c9aafc83a0e55870af33e0f479c11db064079a7b) |
| `bfe276d3` | [bfe276d3eab6b0f1bb2d6301693cd3a45e7990e1](https://github.com/jhurliman/glsmac/commit/bfe276d3eab6b0f1bb2d6301693cd3a45e7990e1) |
| `8083210a` | [8083210a84247a4ab906cb4491f897babab3ad5c](https://github.com/jhurliman/glsmac/commit/8083210a84247a4ab906cb4491f897babab3ad5c) |
| `575a5b3c` | [575a5b3c9405aac2d7d5e898cf128d07ea2eeb6d](https://github.com/jhurliman/glsmac/commit/575a5b3c9405aac2d7d5e898cf128d07ea2eeb6d) |
| `519a0bd7` | [519a0bd7ee26db3b62cb8d2e392244c7a132eaf5](https://github.com/jhurliman/glsmac/commit/519a0bd7ee26db3b62cb8d2e392244c7a132eaf5) |
| `b1935890` | [b19358908b8cc49bdb82f159e0b4c6763a4187dc](https://github.com/jhurliman/glsmac/commit/b19358908b8cc49bdb82f159e0b4c6763a4187dc) |
| `b819ecda` | [b819ecda1384a4fb3d9191618ef190bb5156c467](https://github.com/jhurliman/glsmac/commit/b819ecda1384a4fb3d9191618ef190bb5156c467) |
| `b66d017e` | [b66d017e5fcb854ba1157f374344348bb6325946](https://github.com/jhurliman/glsmac/commit/b66d017e5fcb854ba1157f374344348bb6325946) |
| `8c6ebef2` | [8c6ebef208e6062dd1b7730a5e63fdb11afb21dd](https://github.com/jhurliman/glsmac/commit/8c6ebef208e6062dd1b7730a5e63fdb11afb21dd) |
| `a16fa30b` | [a16fa30b4f8d1a812d306cf7a719de318cbcba3a](https://github.com/jhurliman/glsmac/commit/a16fa30b4f8d1a812d306cf7a719de318cbcba3a) |
| `eb29516b` | [eb29516b15fa83c7ccca2eaf5786a03d1f514c8f](https://github.com/jhurliman/glsmac/commit/eb29516b15fa83c7ccca2eaf5786a03d1f514c8f) |
| `9f66d2a6` | [9f66d2a61d42fea7761a3165c94235d0c5dfa669](https://github.com/jhurliman/glsmac/commit/9f66d2a61d42fea7761a3165c94235d0c5dfa669) |
| `236c4dca` | [236c4dca67a8a48b26b1bc8aec28446abfb52e71](https://github.com/jhurliman/glsmac/commit/236c4dca67a8a48b26b1bc8aec28446abfb52e71) |
| `0ce4e099` | [0ce4e0992576300dd0c408e2543acda9c5b773de](https://github.com/jhurliman/glsmac/commit/0ce4e0992576300dd0c408e2543acda9c5b773de) |

The `d57fc5a3` and `fa69e2d4` revisions are retained on the immutable
`codex/validation-20261016-host-integer` branch at the full `fa69e2d4` revision
above.

The later three pins are retained on immutable branches
`codex/validation-20261016-world-widget`,
`codex/validation-20261016-snapshot-bonus`, and
`codex/validation-20261016-unit-ui`, respectively.
The renderer/native-test integration at `9d258151` is retained on
`codex/validation-20261016-renderer-native`.
The Surface integration at `705cc939` is retained on
`codex/validation-20261016-surface-unit`; the energy, unit-design parser and
switch-ownership integration at `68081411` is retained on
`codex/validation-20261016-energy-units-switch`.
The component resolver and timer repair at `0d98cde8` are retained on
`codex/validation-20261016-resolution-timers`.
The evaluating-base context at `bf00f411` is retained on
`codex/validation-20261016-base-context`.
The RNG/exception integration at `82222d37` is retained on
`codex/validation-20261016-rng-exception`; the CVR reader at `c9aafc83` is retained
on `codex/validation-20261016-cvr-container`.
The initial RNG failure at `bfe276d3` is an ancestor of the immutable
`codex/validation-20261016-rng-exception` pin. Focused exception `8083210a`
and CVR `575a5b3c` are retained in the merged histories of PRs #72 and #73.
Turn restoration `519a0bd7` and its focused publication source `b1935890`
are retained on `codex/validation-20261016-world-turn` and
`codex/validation-20261016-world-turn-publication`, respectively.
The CVR scene integration `b819ecda` is retained on
`codex/validation-20261016-cvr-scene`; its focused publication source
`b66d017e` is the reviewed source of PR #75.
Fixture retirement `8c6ebef2` and completion restoration `a16fa30b` are retained
on `codex/validation-20261016-fixture-retirement` and
`codex/validation-20261016-world-completion`, respectively. The failed and
repaired focused completion sources `eb29516b` and `9f66d2a6` are ancestors of
`codex/validation-20261016-completion-publication`, pinned at `9f66d2a6`.
Loader integration `236c4dca` and focused publication `0ce4e099` are retained
on `codex/validation-20261016-loader-progress` and
`codex/validation-20261016-loader-publication`, respectively.

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

## Surface replacement and clean tracked shutdown

At `705cc939`, replacing an owned Surface background releases the previous
texture after the replacement has been created successfully. The ninth focused
native ownership case covers repeated replacement, borrowed/owned transitions,
invalid replacement, unset and destruction through the real Surface factory.

Windows CTest passed 8/8 in 40.31 s; Linux ASan/UBSan CTest passed 8/8 in
41.56 s with leak detection disabled for the parser baseline recorded above.
Explicit correctness analysis of Surface, UnitManager and UnitRulesTXTLoader
passed all three production translation units in 3.53 s. These checks also
include the unit-despawn rollback repair and component-table parsing.

The tracked Windows run used the same disposable all-tile `-2/300/3` fixture.
The Hive showed N `-4`, consumption `2`, surplus `-006`, M `600`, E `6`.
Removing its worked tile changed these to N `-2`, surplus `-004`, M `300`,
E `3`; restoring the tile restored the original totals. Starting a new game
in the same process produced Gaia's Landing with a rendered map and minimap.
Title-bar close exited 0 with empty stderr. MemoryWatcher reported
`No memory/opengl leaks detected.`

This is a clean tracked shutdown for that bounded sequence, not an exhaustive
heap or graphics proof. The random worlds were not pinned. No turn advance,
save/load, multiplayer or soak was exercised. The disposable fixture is older
than the source and does not exercise the new despawn event; native backend
tests cover that separately. Logs use `out/surface-unit-ui-` and
`out/surface-unit-windows-`; Linux logs under
`/home/jhurliman/src/glsmac-gc-audit/` use `surface-unit-asan-` and
`surface-unit-focused-analysis`.

## Switch ownership and full leak detection

The earlier 201-byte / three-allocation baseline came from the ordinary switch
parser path: Switch retained the cases while abandoning their containing Scope.
The original `types.gls.js` alone reproduced the same three allocations (199
bytes with its shorter filename). Switch now owns the Scope and borrows its
ordered case pointers. A counted ownership regression and real empty, nested,
fall-through, break and default scripts exercise destruction and semantics.
The old-behavior negative control fails the ownership assertion and leaks its
counted Scope; the restored implementation passes.

At `68081411`, including the energy calculator and counted unit-design parser,
native Windows CTest passed 8/8 in 53.07 s (full GSE 21.67 s). Linux
ASan/UBSan CTest passed 8/8 in 40.92 s (full GSE 28.04 s) with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`, without a leak report. This supersedes the
specific parser baseline for this revision; it does not erase earlier results
or establish cleanup of every exceptional parser path. The existing local
non-PIE sanitizer configuration and dependency-library prefix were unchanged.
Logs use `out/energy-units-switch-windows-` and, under the Linux directory above,
`energy-units-switch-asan-`.

The full LLVM 15 correctness policy analyzed all 292 production translation
units in 99.34 s at eight jobs, with zero failures. The database was reconfigured
after adding the energy source; the Windows-only network implementation remains
explicitly outside this Linux analysis. Concurrent local builds were active, so
this elapsed time is an observation, not a new development-loop budget. Logs
use `energy-units-switch-analysis` under the Linux directory above.

The separately reviewed weapon-table PR's older head
[e680240238ba3c3d963dc98245d1a0f65b8b404c](https://github.com/jhurliman/glsmac/commit/e680240238ba3c3d963dc98245d1a0f65b8b404c),
retained at immutable `codex/validation-20261016-world-timeout`, hit a Windows full-GSE timeout in
[job 116940520918](https://github.com/jhurliman/glsmac/actions/runs/38891098136/job/116940520918).
The repeating async callback continued beyond its scheduled cancellation.
Same-deadline timer cancellation was subsequently reproduced without relying
on the script's clock timing, as recorded below. The passing earlier local
runs do not erase this timeout or identify its exact hosted interleaving.

## Deterministic timer cancellation and owned unit resolution

At `0d98cde8`, predefined unit component resolution returns owned metadata and
the scheduler keeps repeating timers cancelable throughout deadline dispatch.
On the unchanged scheduler, five of seven initial deterministic native cases
failed promptly, including repeat cancellation, canceled sibling execution,
self-stop batch handling and nested timer loss. The corrected implementation
passes eight cases, including the additional exception bookkeeping control.
The fixture sets deadlines and dispatches the actual scheduler; it does not
sleep, replace scheduling logic or increase timeouts.

| Check at `0d98cde8` | Result |
| --- | --- |
| Native Windows Debug CTest | 8/8, 54.33 s; full GSE 21.46 s |
| Linux ASan/UBSan/LSan CTest | 8/8, 41.83 s; full GSE 29.01 s; leak detection enabled |
| Explicit LLVM 15 correctness analysis | Async and UnitDesignResolver, 2 production translation units, 0.98 s, zero failures |

The analysis database was reconfigured for the new resolver source. This is a
selected-file result, not another full scan. The real async script is included
in the full GSE runs, but no claim of exhaustive scheduler/concurrency coverage
follows. Logs use `out/resolution-timers-windows-` and, under the Linux directory
above, `resolution-timers-asan-` and `resolution-timers-analysis`.

The focused timer publication head
[95812c99f2be30286d3c666316808fdfc5a9da0e](https://github.com/jhurliman/glsmac/commit/95812c99f2be30286d3c666316808fdfc5a9da0e),
retained as the exact head of immutable branch
`codex/validation-20261016-timer-publication`, separately built on Linux and passed
its four registered CTests in 17.41 s (full GSE 14.10 s). Its older parent does
not contain the native-test selector or switch ownership repair, so that result
does not claim clean full-suite leak detection for the focused parent.

## Evaluating-base context

At `bf00f411`, tile resource queries carry the canonical evaluating base through
actual intake, worker scoring and base-screen widget queries. Legacy calls keep
their original callback shape. The regression uses real work/unwork events,
production scripts and restored manager identities, and rejects stale, foreign
or wrong-owner contexts before invoking callbacks. The old route fails because
the expected base context is absent.

Windows CTest passed 8/8 in 43.12 s (full GSE 21.69 s). Linux
ASan/UBSan/LSan CTest passed 8/8 in 43.65 s (full GSE 30.87 s), with leak
detection enabled. Logs use `out/base-context-windows-` and, under the Linux
directory above, `base-context-asan-`. No new interactive pixel comparison is
claimed. The focused PR #70 parent separately passed its four Linux CTests in
24.20 s; that older branch does not include all integration ownership repairs.

## RNG continuation integration: retained failure

The first RNG integration at `bfe276d3` passed Windows CTest 8/8 in 45.87 s.
Linux sanitizer CTest passed 7/8 in 44.14 s: LeakSanitizer reported one 63-byte
allocation from `gse::Exception::what()` reached by the new real initialization
error-path test. That method allocated an unowned `strdup` on each call. The
passing ordinary tests do not establish a leak-free revision. Logs use
`out/rng-windows-` and the Linux directory's `rng-asan-` prefix. Repair and a
fresh sanitizer result are required before recording this integration as clean.

## RNG continuation after exception ownership repair

At `82222d37`, GSE exceptions own their formatted diagnostic and return stable
storage from `what()`. The regression fails against the old allocation behavior
and checks repeated calls, independent copies and retained exception pointers
after the original is destroyed. The separate repair is PR #72; RNG continuation
is PR #71. The repair does not change the RNG implementation.

Windows CTest passed 8/8 in 48.55 s (full GSE 22.31 s); Linux
ASan/UBSan/LSan CTest passed 8/8 in 44.49 s (full GSE 31.55 s), with leak
detection enabled and no report. This supersedes the specific failure at
`bfe276d3`, without erasing it or claiming exhaustive exceptional-path coverage.
Logs use `out/rng-exception-windows-` and the Linux directory's
`rng-exception-asan-` prefix. The focused main-based exception PR separately
passed its four Linux CTests in 16.24 s at `8083210a`.

## CVR envelope integration

At `c9aafc83`, the pure CVR reader validates the complete observed four-chunk
envelope before copying owned opaque payloads. An optional Release probe accepts
all 140 installed files; no original payloads are committed. Five synthetic
groups cover bounds, byte order, ownership and malformed envelopes. This does
not decode geometry or establish conventional-unit model selection.

Native Windows ran all five groups with allocation tracking, exited 0 and
reported no tracked memory/OpenGL leaks. The process was explicitly waited;
logs are `out/cvr-windows-verified-stdout.log` and `-stderr.log`, with the saved
exit code in `out/cvr-windows-exit.txt`. The focused PR #73 branch separately
passed its four Linux CTests in 17.20 s at `575a5b3c`. The broader preceding integration's
eight-test results are recorded above; no repeated full Windows suite is
claimed for this isolated reader addition.

The freshly configured LLVM 15 correctness database analyzed all 295 production
translation units in 56.32 s at eight jobs, with zero failures. This includes the
new RNG and CVR sources and exception ownership repair. The Windows-only network
source remains explicitly excluded. Logs use `rng-cvr-analysis` under the Linux
directory above. Focused reader sanitizer checks, Debug/Release analysis and
three weakened-reader controls are recorded in [the versioned component notes](https://github.com/jhurliman/glsmac/blob/c9aafc83a0e55870af33e0f479c11db064079a7b/docs/cvr-container.md).

## Turn restoration and current-script smoke test

At `519a0bd7`, downloaded turn metadata is assigned without replaying ordinary
turn hooks. Setup sees the restored year; readiness and minimap status are
published after reconstruction. Existing destination completion flags remain
preserved, but this change does not yet serialize them. The actual world/body,
initialization and minimap-script regressions are described in
[the versioned contract](https://github.com/jhurliman/glsmac/blob/519a0bd7ee26db3b62cb8d2e392244c7a132eaf5/docs/world-snapshot-turn.md).

Linux ASan/UBSan/LSan passed 8/8 CTests in 44.81 s (GSE 31.96 s), with leak
detection enabled. The first Windows suite timed out at 300 s after logging
the pre-existing queued invalid-download case, leaving 7/8 passing overall.
That case passed standalone; a full retry passed 8/8 in 60.90 s, and twenty
additional quiet focused repetitions passed. A debugger attachment during the
retry caught a subsequent CLI test, not a stuck download; it is not causal
evidence, and the retry duration includes that instrumentation. The original
timeout remains unexplained. The focused publication source `b1935890`
separately passed 5/5 Linux CTests in 23.35 s.

The Windows binary built from `519a0bd7` also ran with allocation tracking and
the unchanged current production scripts. Gaia's Landing showed N/M/E intake
3/2/2, nutrient consumption 2, and nutrient surplus +1. Removing its worker
changed intake to 2/1/2 and nutrient surplus to zero; restoring the worker
restored the original totals. Completing the turn through its unmoved-unit
confirmation advanced the visible year from 2101 to 2102. Title-bar close
exited 0, with empty stderr and no tracked memory/OpenGL leaks reported.
This exercises the current evaluating-base UI route and ordinary advancement,
not a rendered multiplayer restore, original-game yield parity or a soak test.

Logs are `out/turn-windows-ctest.log`, `out/turn-windows-retry-ctest.log`,
`out/queued-download-windows-*`, `out/queued-stress-1.log` through
`out/queued-stress-20.log`, and `out/turn-visible-ui-*`; Linux logs use
`turn-asan-` in the directory above. An earlier hidden launch was explicitly
terminated because it exposed no targetable window; its `out/turn-current-ui-*`
logs and exit -1 are harness evidence, not a successful shutdown or game crash.

## CVR scene integration

At `b819ecda`, eight new scene groups plus the five envelope groups passed
native Windows with allocation tracking and exit 0. The explicitly waited
process logs are `out/cvr-scene-windows-stdout.log` and `-stderr.log`.
The main-based publication source `b66d017e` passed 4/4 Linux CTests in
16.25 s (GSE 13.09 s). Installed-file probing, focused sanitizer checks,
analysis and negative controls are recorded in
[the versioned scene notes](https://github.com/jhurliman/glsmac/blob/b819ecda1384a4fb3d9191618ef190bb5156c467/docs/cvr-scene-records.md).
The decoder accepts all 140 installed files and retains owned scene records,
including empty and duplicate object names. Geometry and original-renderer
equivalence remain unimplemented; these results do not activate conventional
units.

## Fixture retirement and persisted completion

At `8c6ebef2`, backend fixtures unpublish their Map before destroying it under
GC accumulation. Actual Map/Tile destruction observers verify exclusion and
wrapper invalidation; the old helper fails that check. Windows CTest passed
8/8 in 59.22 seconds and Linux ASan/UBSan/LSan passed 8/8 in 43.81 seconds.
This is a fixture lifetime repair (#76), not a demonstrated cause or repair
of the earlier Windows timeout. Details are in
[the versioned retirement evidence](https://github.com/jhurliman/glsmac/blob/8c6ebef208e6062dd1b7730a5e63fdb11afb21dd/docs/validation/backend-fixture-map-retirement-2026-10-16.md).

At `a16fa30b`, V3 world downloads preserve each occupied player's completion
at the same boundary as turn/RNG capture. Initialization validates exact roster
and lifetime bindings before applying any saved flag. Rejected early restores
park ordinary backend work and queue owner-thread cleanup; generation checks
retire obsolete errors after reset or replacement. This does not provide full
saves, event catch-up or general session callback retirement. See
[the versioned V3 contract](https://github.com/jhurliman/glsmac/blob/a16fa30b4f8d1a812d306cf7a719de318cbcba3a/docs/world-snapshot-turn.md).

Windows CTest passed 8/8 in 50.65 seconds (GSE 25.35 seconds); Linux
ASan/UBSan/LSan passed 8/8 in 48.20 seconds (GSE 33.02 seconds), with leak
detection enabled. Fresh LLVM 15 analysis covered 297 production translation
units in 55.98 seconds at eight jobs, with zero failures. The Linux scan still
excludes the Windows-only network implementation. Logs use
`out/world-completion-windows-` and
`/home/jhurliman/src/glsmac-gc-audit/world-completion-` prefixes.

The first focused PR #77 publication at `eb29516b` passed 163 native cases and
failed one malformed-Boolean case (CTest 4/5 overall). Its older parent lacked
the strict Buffer decoder already present in integration. Merging the existing
PR #24 prerequisite produced `9f66d2a6`, which passed all five focused CTests
in 22.51 seconds (GSE 16.05 seconds). No malformed-input assertion was relaxed.
Both runs remain in `/home/jhurliman/src/glsmac-unit-publication/`, named
`world-completion-missing-bool-ctest.log` and
`world-completion-bool-ctest.log`. These checks do not establish rendered
multiplayer-join correctness or explain the older intermittent timeout.

GitHub review of PR #77 at
[6805e3a3e5980eb01dba846f4fecbca6d400f0ba](https://github.com/jhurliman/glsmac/commit/6805e3a3e5980eb01dba846f4fecbca6d400f0ba),
retained at immutable `codex/validation-20261016-completion-reviewed`, subsequently identified three
unresolved concurrency/dispatch gaps: live roster mutation between validation
and flag commit, MT reset between queued-failure validation and InitFailed,
and unrestricted MT dispatch before the parked-failure guard. The tests above
exercise foreign-thread enqueueing but do not prove exclusion of those live
overlaps. PR #77 remains held pending deterministic concurrent regressions and
repairs. Session-token cancellation alone cannot protect an already-executing
callback from native teardown. The passing results remain valid for the cases
actually run and are not a claim that this snapshot implementation is ready
to merge.

## Nested loader request dispatch

At `236c4dca`, SetLoaderText admits only frontend polling and Ping while Map
methods are on the stack. Reset and other requests stay queued and cancellable
until ordinary iteration. An asset-free test drives the actual ProcessTiles
loop and observes Map destruction; the old pump fails because reset deletes
that executing Map. Two further cases exercise excluded opcodes and owned
request/response cleanup. The test holds accumulation during native reset to
isolate this reentrancy check from the separate collector-exclusion work.

Integration Windows CTest passed 8/8 in 50.75 seconds (GSE 24.08 seconds).
The source slice [3705812bbfa6eefa5ab5a635ccb5fc7bd1f0f231](https://github.com/jhurliman/glsmac/commit/3705812bbfa6eefa5ab5a635ccb5fc7bd1f0f231),
retained as the exact head of immutable branch
`codex/validation-20261016-loader-source`, passed Linux CTest
8/8 in 36.51 seconds and its three focused cases passed with allocation
tracking and ASan/UBSan/LSan. The narrower publication `0ce4e099`, based on
fixture retirement #76 without the later snapshot stack, passed Linux CTest
5/5 in 22.34 seconds (GSE 16.04 seconds). Fresh LLVM 15 analysis at integration
covered 297 production translation units in 57.70 seconds with zero failures;
the Windows-only network implementation remains outside that Linux scan.

Windows logs use `out/loader-progress-windows-`; focused publication logs use
`/home/jhurliman/src/glsmac-unit-publication/loader-progress-`; the full scan
and its per-unit summary use
`/home/jhurliman/src/glsmac-gc-audit/loader-progress-analysis`. Source-slice
positive/negative logs are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/loader-` and its worktree's
ignored `out/validation-loader/` directory.

Submitted as #78. This does not establish generic GC exclusion, retire old
session requests, serialize concurrent snapshot initialization, or fix nested
network dispatch. No rendered initialization/cancellation smoke is claimed.

## Map packets and queued session identity

Integration `20a643a35ac1163a30b20604350fcc4627863d4c` pairs decoded Player
allocation with State's tracked teardown (#79) and removes State::Iterate
from the synchronous map loop (#80). Actual Network queue/Client decoding
regressions show roster replacement and disconnect wait until the map pass
returns, while restricted progress polling remains available. Both old-pump
negative cases fail. Windows CTest passed 8/8 in 79.27 seconds; the focused
Slot publication `7340b20122164246707f904821a5f8b4dd270db4` passed Linux CTest
5/5 in 21.37 seconds, and map publication
`186ab971ef74c84c2ce100905fad267a5bcaf801` passed 5/5 in 22.42 seconds.

Integration `078a72fbd0265c8d709d646d5823cc842ed849d5` additionally retires
queued callbacks with identities captured before staging (#81) and guards
script run reentry. Windows CTest passed 8/8 in 53.00 seconds; Linux
ASan/UBSan/LSan passed 8/8 in 44.39 seconds with leak detection enabled. Fresh
LLVM15 analysis covered 297 production translation units in 61.53 seconds
with zero failures. Linux analysis excludes the Windows network implementation.
The focused session publication `ab9e5413194ab4f38c81667646836dd72fafcbfb`
passed Linux CTest 5/5 in 21.33 seconds without depending on held PR77.
The script-entry publication `3299f7d534d5831c08f9d5ede9c514451191ff14`
passed 5/5 in 22.34 seconds. Separate source-slice positive/negative controls
are recorded in the session callback and script run documents.

Every tested revision in this section has the following immutable retention
path. The Slot publication is an ancestor of the map publication; the other
five entries are the exact named branch heads.

| Tested revision | Retention branch | Relationship |
| --- | --- | --- |
| [20a643a35ac1163a30b20604350fcc4627863d4c](https://github.com/jhurliman/glsmac/commit/20a643a35ac1163a30b20604350fcc4627863d4c) | `codex/validation-20261016-map-packets` | Exact head |
| [7340b20122164246707f904821a5f8b4dd270db4](https://github.com/jhurliman/glsmac/commit/7340b20122164246707f904821a5f8b4dd270db4) | `codex/validation-20261016-map-publication` | Ancestor of pinned head `186ab971ef74c84c2ce100905fad267a5bcaf801` |
| [186ab971ef74c84c2ce100905fad267a5bcaf801](https://github.com/jhurliman/glsmac/commit/186ab971ef74c84c2ce100905fad267a5bcaf801) | `codex/validation-20261016-map-publication` | Exact head |
| [078a72fbd0265c8d709d646d5823cc842ed849d5](https://github.com/jhurliman/glsmac/commit/078a72fbd0265c8d709d646d5823cc842ed849d5) | `codex/validation-20261016-session-identity` | Exact head |
| [ab9e5413194ab4f38c81667646836dd72fafcbfb](https://github.com/jhurliman/glsmac/commit/ab9e5413194ab4f38c81667646836dd72fafcbfb) | `codex/validation-20261016-session-publication` | Exact head |
| [3299f7d534d5831c08f9d5ede9c514451191ff14](https://github.com/jhurliman/glsmac/commit/3299f7d534d5831c08f9d5ede9c514451191ff14) | `codex/validation-20261016-script-entry` | Exact head |

Windows logs use
`out/session-map-windows-` and `out/session-identity-windows-`; Linux combined
logs use `/home/jhurliman/src/glsmac-gc-audit/session-identity-`; focused logs
use `/home/jhurliman/src/glsmac-unit-publication/` with `decoded-player-`,
`map-packet-`, `session-identity-`, and `run-entry-` prefixes.

Rendered quickstart at `078a72fb` reached a SMAC map in the window titled
`GLSMAC v0.3-???????-debug`, showing Sparta Command and mission year 2101.
The title's version text does not identify the source; the explicit build pin
above does. The UI helper timed out on the Menu click and remained busy after
its documented recovery, so restart and clean shutdown were not verified.
The controlled test process was terminated (exit -1). Its logs are
`out/session-identity-ui-visible-stdout.log` and `-stderr.log`; stderr was
empty before termination. Two harness attempts remain separate evidence:
the missing-ruleset command correctly exited 1, and a hidden-window launch
exposed no targetable window and was terminated. Neither is a game-crash or
successful shutdown result. That Windows run leaves its rendered restart gate
open; the later native Mac restart evidence below is a separate run.

None of these checks establishes executing-callback/native-reset exclusion,
safe stale MT request admission, or the held PR77 concurrency fixes.

## Owned CVR geometry records

At [e022b688e4206ee9848050d816329dd125ade4ca](https://github.com/jhurliman/glsmac/commit/e022b688e4206ee9848050d816329dd125ade4ca), the geometry reader validates
the observed simple/grouped profiles before allocating owned raw records.
All seven focused Windows cases passed with allocation tracking, exit 0 and
no reported memory/OpenGL leaks. Fresh LLVM15 analysis covered 298 production
translation units in 55.88 seconds with zero failures, with the same Linux
platform exclusion. Source is retained on immutable branch
`codex/validation-20261016-cvr-geometry`. Logs use
`out/cvr-geometry-windows-` and
`/home/jhurliman/src/glsmac-gc-audit/cvr-geometry-analysis`.

The focused main-based source [3090f15e395257bd3eeec4e432b78af5959e1ccf](https://github.com/jhurliman/glsmac/commit/3090f15e395257bd3eeec4e432b78af5959e1ccf),
retained as the exact head of immutable branch
`codex/validation-20261016-geometry-publication`,
passed Linux CTest 4/4 in 46.71 seconds (GSE 43.44 seconds); publication logs
use `/home/jhurliman/src/glsmac-unit-publication/cvr-geometry-`.
Submitted as #83. The installed Release probe accepts 438 geometry payloads,
897 groups and 3,620,629 records, with two explicit unsupported variants.
That full installed-set geometry probe exits 1; envelope/scene modes retain
140 accepted files and exit 0. The source slice's synthetic sanitizer cases,
negative aggregate-count control and exact unsupported profile are recorded
in the [versioned geometry contract](https://github.com/jhurliman/glsmac/blob/e022b688e4206ee9848050d816329dd125ade4ca/docs/cvr-geometry-records.md).
These are structural ingestion results, not decoded rendering meanings,
original-renderer comparisons or a live conventional unit.

## Native Mac restart and request retirement

The [native Mac report](https://github.com/jhurliman/glsmac/blob/19d01b9afa306176f41779355263441a7a1c7a35/docs/validation/macos-native-2026-10-16.md)
records integration
[`5cd78418271231121fb9a8e1d136faa82bbc5dd0`](https://github.com/jhurliman/glsmac/commit/5cd78418271231121fb9a8e1d136faa82bbc5dd0)
on honey's Apple M2 Max. Native CTest passes 8/8 in 28.55 seconds, and both the
hidden context diagnostic and the game create OpenGL 4.1 core contexts. The
user confirms map interaction, base inspection, turn advancement and Menu >
Start New Game. User screenshots and logs corroborate distinct Morgan and
University worlds in one process. Restart delay is reported but unmeasured;
the process was still alive at the subsequent read-only check. This closes
the interactive restart check for #82's integrated guard, not clean shutdown,
performance, concurrency, or pixel fidelity. SMAC rules are hash-matched;
the differing Mac alphax.txt is explicitly not treated as SMAX validation.

The admission implementation is retained at
[`a5af8fe88515846535f0244066654f68ddda270c`](https://github.com/jhurliman/glsmac/commit/a5af8fe88515846535f0244066654f68ddda270c)
on immutable branch `codex/validation-20261016-admission-source`.
It binds pending/accepted/failed world ownership to exact session identities,
validates native requests at execution and retires every frontend MT ID.
Integration
[`8392f67aed7a331cef6c978a7e99abd6c89b580d`](https://github.com/jhurliman/glsmac/commit/8392f67aed7a331cef6c978a7e99abd6c89b580d),
retained at `codex/validation-20261016-world-admission`, passes Windows CTest
8/8 in 50.48 seconds and a fresh 298-unit LLVM15 scan in 62.67 seconds with
zero failures. Focused publication
[`8cb32997dbc0b05451a26f50da73244f2888dd0b`](https://github.com/jhurliman/glsmac/commit/8cb32997dbc0b05451a26f50da73244f2888dd0b)
passes Linux CTest 5/5 in 22.28 seconds. It is an ancestor of PR #86's retained
publication branch. Its older allocation fixtures are not claimed to reproduce
the integration's tracked/sanitizer results.

## Failure cleanup and polled reset

The failure helper source
[`67ac1e6357681bdc5060c2d44864b975cfb40b8c`](https://github.com/jhurliman/glsmac/commit/67ac1e6357681bdc5060c2d44864b975cfb40b8c)
is retained at `codex/validation-20261016-init-failure-source`; the public
polling controller source
[`b2ec36b83ee3bbd68f9bd6073eed2fcc4b35fb1b`](https://github.com/jhurliman/glsmac/commit/b2ec36b83ee3bbd68f9bd6073eed2fcc4b35fb1b)
is retained at `codex/validation-20261016-reset-poll-source`. Seven helper
regressions cover null maps, throwing hooks and ownership; five actual
controller cases exercise callback boundaries and same-thread progress.
The copied old reset controller times out on its self-wait. These tests do
not substitute for native reset/collector exclusion or startup coordination.

Combined integration
[`9663fcdabab888e3a77d7361dafd1bbec3ab3673`](https://github.com/jhurliman/glsmac/commit/9663fcdabab888e3a77d7361dafd1bbec3ab3673),
retained at `codex/validation-20261016-reset-poll`, passes Windows CTest 8/8
in 48.83 seconds and Linux ASan/UBSan/LSan CTest 8/8 in 111.62 seconds with
leak detection enabled. The sanitizer cache retains its documented local
non-PIE startup workaround. A fresh full LLVM15 scan covers 298 production
translation units in 56.86 seconds with zero failures. Linux analysis excludes
the Windows network implementation. Logs use `out/reset-poll-windows-` and
`/home/jhurliman/src/glsmac-gc-audit/reset-poll-`.

The first focused failure-helper publication timed out in its native hook
test because that older base omitted the existing #54 callback-lock repair.
GDB showed the handler check blocked on that mutex. Including #54 unchanged
gives publication
[`1f39d966247ee2fe0e6c25f9ca55d3d75a052a8d`](https://github.com/jhurliman/glsmac/commit/1f39d966247ee2fe0e6c25f9ca55d3d75a052a8d)
Linux CTest 5/5 in 23.29 seconds. This source is retained in PR #87's branch;
the original timeout and passing retry remain separate evidence.

PRs #20, #21, #22, #23, #24, #83 and #84 have now merged after their current
heads passed the required review and checks. New and revised heads remain
pending as detailed below. The local checks do not waive that review gate or
resolve #77's held concurrency findings.

## Reset publication and startup presentation ownership

PR #88 is published at
[`c05359e6ba3c6dfb82646e3821471cb0e5363d6a`](https://github.com/jhurliman/glsmac/commit/c05359e6ba3c6dfb82646e3821471cb0e5363d6a),
retained on `codex/validation-20261016-lifecycle-publication`. Its
[versioned controller contract](https://github.com/jhurliman/glsmac/blob/c05359e6ba3c6dfb82646e3821471cb0e5363d6a/docs/polled-reset-controller.md)
distinguishes the original `b2ec36b8` source checks and combined `9663fcda`
integration from the narrower publication stack. The callback-retirement
prerequisite now includes its direct `<algorithm>` header after hosted MinGW
and libstdc++ exposed reliance on a transitive include.

PR #89 is published at
[`c513707d8b7c73e1f6e51b8e5bb2fc5749c1d6a3`](https://github.com/jhurliman/glsmac/commit/c513707d8b7c73e1f6e51b8e5bb2fc5749c1d6a3).
The backend detaches response-envelope aliases before releasing terrain texture,
render mesh and data mesh. Failed initialization and reset release assets still
owned by the backend; a completed transfer leaves ownership with the frontend
or the unconsumed response. The
[versioned ownership contract](https://github.com/jhurliman/glsmac/blob/c513707d8b7c73e1f6e51b8e5bb2fc5749c1d6a3/docs/startup-payload-ownership.md)
records the six tracked cases and separates source implementation `62c816fa`
from focused publication `7e5d7334` (Linux CTest 5/5 in 22.25 seconds).

Combined payload integration
[`cd2613d12fce4613ec6e13ab4d2f3b9e50586f07`](https://github.com/jhurliman/glsmac/commit/cd2613d12fce4613ec6e13ab4d2f3b9e50586f07),
retained at `codex/validation-20261016-startup-payload`, passes Windows CTest
8/8 in 50.84 seconds, Linux ASan/UBSan/LSan CTest 8/8 in 49.10 seconds, and a
fresh full LLVM15 scan of 298 production translation units in 65.15 seconds
with zero failures. Leak detection is enabled. Windows-only networking remains
outside the Linux analysis. Logs use `out/startup-payload-windows-` and
`/home/jhurliman/src/glsmac-gc-audit/startup-payload-`.

These fixtures perform native reset within existing accumulation. They prove
payload ownership, not native reset/collector exclusion or startup phase
coordination. No newer native Mac or rendered-game result is inferred.

## Actual Engine single-thread shutdown

The source implementation is
[`f586b523b8764152586987bb3638105f740191e9`](https://github.com/jhurliman/glsmac/commit/f586b523b8764152586987bb3638105f740191e9),
retained at `codex/validation-20261016-engine-source`. Its
[versioned regression and evidence](https://github.com/jhurliman/glsmac/blob/f586b523b8764152586987bb3638105f740191e9/docs/engine-shutdown-order.md)
use real Engine, backend Game, scheduler, GC, GLSMAC, GSE and State with
asset-free presentation services. Original single-thread cleanup destroyed
State before Game::Stop; the copied original Engine fails the final harness's
lifetime assertion while default mode passes. An isolated unchanged-Stop
control triggers UBSan on the destroyed State. The fix stops the successfully
started Game before scheduler teardown without changing Start/Iterate order.

Combined integration
[`2b7ca61c39e34e56965a13edade6fd63f57e7a30`](https://github.com/jhurliman/glsmac/commit/2b7ca61c39e34e56965a13edade6fd63f57e7a30),
retained at `codex/validation-20261016-engine-shutdown`, passes Windows CTest
8/8 in 63.50 seconds, Linux ASan/UBSan/LSan CTest 8/8 in 51.63 seconds, and
the full 298-unit LLVM15 scan in 68.99 seconds with zero failures. The opt-in
actual Engine runner passes default and single-thread modes with tracking and
sanitizers, after exporting the existing build's compilation database. Its
initial invocation failed the runner's missing-database precondition; that
attempt is retained and is not a passing runtime test. The fast GCC sanitizer
lifecycle harness passes all 24 cases.

Integration logs use `out/engine-shutdown-windows-` and
`/home/jhurliman/src/glsmac-gc-audit/engine-shutdown-`, including
`engine-shutdown-actual.log` and `engine-shutdown-actual-retry.log`.
Source-slice results and copied-code negative controls remain separately under
`/home/jhurliman/src/glsmac-engine-shutdown-audit`. A narrower publication attempt
at `35bdc9ca58f8dc344dbd28c21ed1fcc535bdec1a` failed actual Engine tracking on the
older UUID allocation prerequisite; no passing actual-runtime result is claimed
for it. PR #90 includes the unchanged UUID repair and its focused tracked
regression. Publication `e48d050cc4c372ec916ddaedbe42aedfdf89add6` passes Linux
CTest 6/6 in 23.34 seconds, GCC sanitizer lifecycle 24/24, and both actual Engine
modes with tracking. Its documented head is
[`7c3794a0fb685928aaa10c06d37e82066afe8768`](https://github.com/jhurliman/glsmac/commit/7c3794a0fb685928aaa10c06d37e82066afe8768).
These publication checks do not claim the full sanitizer coverage of `2b7ca61c`.
The later publication chain is retained at immutable branch
`codex/validation-20261016-shutdown-publication`, pointing to
[`ccd8561a0e428bc251bee1a8bcf8f02ca5f9d157`](https://github.com/jhurliman/glsmac/commit/ccd8561a0e428bc251bee1a8bcf8f02ca5f9d157).
It retains the failed and passing publication revisions above after prerequisite
merges moved the live PR heads again.

This is bounded Engine shutdown evidence, not a rendered-world soak, native Mac
close confirmation, or the independent executing-callback and collector reset
guards. The user-confirmed native Mac remains pinned to `5cd78418`; restart
latency and close are still unverified.

## Original composition and owned CVR frame assembly

PR #91 publishes bounded original composition evidence at
[`1f572b450ea6e09c74412c515bb46c14d07872fa`](https://github.com/jhurliman/glsmac/commit/1f572b450ea6e09c74412c515bb46c14d07872fa).
PR #92 adds the pure assembler at `c9e009d3ec294ce57b58afba9e9906b48b83d91b`
and clarifies its arithmetic contract at
[`0ead74e464fcc48156ac0dfc39ca797895d5b761`](https://github.com/jhurliman/glsmac/commit/0ead74e464fcc48156ac0dfc39ca797895d5b761).
The [versioned assembly contract](https://github.com/jhurliman/glsmac/blob/0ead74e464fcc48156ac0dfc39ca797895d5b761/docs/cvr-frame-assembly.md)
separates raw geometry ownership, explicit scene/frame selection and bounded
location addition from model activation or rendering.

The Release production reader/assembler matches **30 original-executed
installed-input cases and all 1,755 records bitwise**. These are 18 direct cases
and 12 reached through the original selector. The original report's other six
positive cases use synthetic pre-existing prefixes or capacity inputs; native
tests cover the corresponding policies separately. They are not six additional
installed-file differential matches. Neither the original loader nor rasterizer
is executed by this comparison, and no conventional unit is rendered.

Combined integration
[`6500014756dbf2365e0ece1a2bc342b13112832a`](https://github.com/jhurliman/glsmac/commit/6500014756dbf2365e0ece1a2bc342b13112832a),
retained at `codex/validation-20261016-cvr-assembly`, passes Windows CTest 8/8
in 61.47 seconds, Linux ASan/UBSan/LSan CTest 8/8 in 50.13 seconds, and a fresh
full LLVM15 scan of 299 production translation units in 61.21 seconds with
zero failures. This integration validation is separate from the original-routine
comparison and does not extend the pinned native Mac result.

## Review checkpoint

The following are exact open heads observed during this update; merge-forward
means prerequisite changes were merged into the PR branch, not that the PR
itself landed:

| PR | Head | Review boundary |
| --- | --- | --- |
| [#81](https://github.com/jhurliman/glsmac/pull/81) | `e8774b2f5640c8fb2f3f4ef423181401f53219a5` | Direct `<algorithm>` fix; replacement-head review pending |
| [#82](https://github.com/jhurliman/glsmac/pull/82) | `4421cc971268d434eaa95270c2feba93b3b30b19` | Prerequisite merged forward; replacement-head review pending |
| [#86](https://github.com/jhurliman/glsmac/pull/86) | `685f13768571d1528e57efbd1791948db8b41d75` | Missing callback-lock prerequisite added after hosted timeout; replacement-head checks and review pending |
| [#87](https://github.com/jhurliman/glsmac/pull/87) | `05fccf546b00a74080f48a8d67549af9f7966c77` | Prerequisite merged forward; replacement-head review pending |
| [#88](https://github.com/jhurliman/glsmac/pull/88) | `c05359e6ba3c6dfb82646e3821471cb0e5363d6a` | Review unavailable under reported quota limit |
| [#89](https://github.com/jhurliman/glsmac/pull/89) | `c513707d8b7c73e1f6e51b8e5bb2fc5749c1d6a3` | Required current-head review remains pending |
| [#90](https://github.com/jhurliman/glsmac/pull/90) | `7c3794a0fb685928aaa10c06d37e82066afe8768` | Required current-head review remains pending |
| [#91](https://github.com/jhurliman/glsmac/pull/91) | `1f572b450ea6e09c74412c515bb46c14d07872fa` | Required current-head review remains pending |
| [#92](https://github.com/jhurliman/glsmac/pull/92) | `0ead74e464fcc48156ac0dfc39ca797895d5b761` | Required current-head review remains pending |

PR #86's [clean receipt](https://github.com/jhurliman/glsmac/pull/86#issuecomment-6218551761)
names the older `43a14890bb` head. It does not review the newer head above.
Hosted Windows CTest on `0592d21144a2e6289379021a5d05edf8ce159bea` timed out
after 300 seconds in the immediate initialization-error admission test. The
replacement adds the existing #54 callback-lock repair; its validation is still
pending at the initial head observation. A subsequent local run at `685f1376`
passes Linux CTest 5/5 in 22.28 seconds; this is not a hosted Windows retry or
new-head Codex review. The earlier Linux publication and broader Windows
integration successes do not make that failed hosted revision green. Applicable
hosted checks on the recorded #85, #87 and #88 heads pass, independently of review.
Quota failures are explicit on
[#85](https://github.com/jhurliman/glsmac/pull/85#issuecomment-6218357350),
[#87](https://github.com/jhurliman/glsmac/pull/87#issuecomment-6218724895) and
[#88](https://github.com/jhurliman/glsmac/pull/88#issuecomment-6219022882).
No additional review request is made while that reported limit is unchanged.
Local validation and independent peer review do not replace the requested
GitHub Codex review, and these PRs remain unmerged.
