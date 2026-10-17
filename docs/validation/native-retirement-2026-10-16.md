# Native world retirement integration — 2026-10-16

Combined source
[`f4f7958172131dc2336fbd333884b4119fb0f9a1`](https://github.com/jhurliman/glsmac/commit/f4f7958172131dc2336fbd333884b4119fb0f9a1)
is retained at `codex/validation-20261016-native-retirement`. It includes the
startup coordinator, collector exclusion for native retirement, ability metadata,
indexed rasterization and original basis-conversion evidence. The later mineral
factor calculator and production basis converter are absent from this checkpoint.

| Check | Result |
| --- | --- |
| Native Windows Debug CTest | 8/8, 86.59 seconds |
| WSL Linux ASan/UBSan/LSan CTest | 8/8, 50.30 seconds; leak detection enabled |
| Actual Engine shutdown harness | Default and single-thread modes pass with allocation tracking and sanitizers |
| Full LLVM 15 correctness analysis | 300 production translation units, 63.16 seconds, zero failures; Windows-only network implementation excluded |

The existing local sanitizer non-PIE workaround remains confined to its build
cache. No warning policy or suppression changed. The narrower PR #99 publication
at `a6be97c047410ce9f019b2e8467d89e2a6475c2c` separately passes its full six-test
suite and both actual Engine shutdown modes; its measurements are in the
[retirement contract](https://github.com/jhurliman/glsmac/blob/a6be97c047410ce9f019b2e8467d89e2a6475c2c/docs/native-world-retirement.md).

## Incomplete rendered follow-up

The native Windows executable, SHA-256
`6174ff3471ea3ceb8688230a38c473ae6e47c3ce6e121c6855250eb23547b538`,
rendered Morgan Industries, terrain, the bottom interface and minimap at year
2101. It used purchased GOG assets in explicit SMAC mode, a 1280 × 800 window,
a 32 × 32 quickstart map and supplied seed `1:2:3:4`. Its isolated prefix was
`out/smoke-win-native-retirement-f4f79581`.

Menu → Game → Start New Game opened the confirmation dialog. The first attempt
to confirm timed out in the Windows Computer Use helper. After recovering the
window inventory, a fresh screenshot still showed that dialog. A second input
timed out; subsequent capture reported that the helper already had an active
request. Resetting the JavaScript client did not clear that helper condition.
No restart receipt appears in the game log. This does not identify an engine
deadlock or establish whether either timed-out input reached the game.

The owned disposable process, 1509056, was explicitly stopped after checking
its executable path and start time. It ran from 16:46:50 UTC to 16:53:47 UTC
and reported exit **-1** from that forced cleanup. This attempt is **not** a
successful restart, a normal shutdown, or an observed engine crash. The earlier
[Windows coordinator check](windows-coordinator-2026-10-16.md) retains its valid
rendered restart/exit result at `12a58670`; neither result extends the native
Mac validation beyond `5cd78418`.

Local evidence uses `out/native-retirement-windows-{build,ctest}.log` and
`out/native-retirement-visible-{process,exit,outcome}.json`, with separate
`stdout.log` and `stderr.log`. Linux build/test/analyzer/harness evidence uses
the `native-retirement-` prefix under `/home/jhurliman/src/glsmac-gc-audit/`.
Preserve these files when reusing build directories. No original assets or
screenshots are committed.

## Remaining lifetime boundaries

Retirement exclusion does not make ordinary initialization and roster mutation
collector-safe. A subsequent source audit found that native map acquisition
publishes a Map before its Tiles exist, and failed downloaded-map decoding can
leave a rooted Map without Tiles between owner checkpoints. The next map slice
keeps acquisition candidates private and makes empty-map traversal safe.
Native roster/settings publication and retirement remain a separate audit.
Passing the tests above does not resolve those findings, establish save/replay
convergence, or complete the M0/M1 release gates.
