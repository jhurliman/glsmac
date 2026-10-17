# Faction replacement and explicit transforms — 2026-10-17

Integrated source
[`e52da37ff0c1b6c8bdbf9daf0e7862599f9a5448`](https://github.com/jhurliman/glsmac/commit/e52da37ff0c1b6c8bdbf9daf0e7862599f9a5448)
is retained at `codex/validation-20261017-faction-transform-e52da37f`.
It includes map publication, independent base production storage, collector
exclusion during faction replacement, and the explicit CVR transform calculator.
These are integration results; narrower PRs retain their separate ancestry and
validation. No production-switching gameplay caller is included.

| Check | Result |
| --- | --- |
| Linux ASan/UBSan/LSan CTest | 8/8, 46.87 seconds; leak detection enabled |
| First native Windows Clang Debug CTest | 7/8, 68.96 seconds; native-filter child timed out |
| Unchanged native Windows follow-up | 8/8, 35.54 seconds; native-filter wrapper 4.89 seconds |
| Native macOS arm64 Debug CTest | 8/8, 29.43 seconds on honey |

The Windows executable SHA-256 is
`7c5d70e6c4be64b69766cf3aab6124145610aad575644afa0ae36fd7d19f2337`.
The follow-up ran on October 17 at approximately 05:38 UTC. Its success does
not erase the earlier intermittent failure or establish its cause.

## Native Mac refresh

The same integrated revision built in a fresh isolated checkout on honey at
`/Users/jhurliman/.codex-validation/glsmac-20261017-e52da37f/`, using the installed
Apple Clang, CMake/Ninja and dependencies documented in the
[earlier native receipt](macos-native-2026-10-16.md). Clone/configure/build/tests
ran from 05:43:59 through 05:45:18 UTC on October 17. The source checkout remained
clean. The binary SHA-256 is
`ffb834e9c14b1add21c789453661c3034dfda90998a44b8a0f6319b2d7c7699c`.

The transfer bundle requires the earlier `5cd78418` commit and has SHA-256
`fd977476239f6651b038fc5d717dc92f188ac01423f0628810875836728e0de8`.
It was fetched into a separate clone of the prior validation checkout; neither
the user's checkout nor the prior binary/logs changed. Configuration used Debug,
`BUILD_TESTING=ON`, native arm64 and four build workers. This was a headless GSE/
CLI run, without sanitizers, assets or a visible game launch. The native logs
are retained under that checkout's sibling `logs/` and copied to the Windows
integration's ignored `out/validation-honey-e52da37f/`.

## Unresolved Windows timing failure

The first integrated run failed in the wrapper's `matching` case: the child
printed its rectangle-test name and `All tests passed.`, but did not exit within
the unchanged ten-second child limit. The remaining seven tests passed. The
wrapper's new case markers distinguished this from the earlier aggregate
thirty-second timeout at source `55636ab5`.

The aggregate allowance was corrected separately to ninety seconds for eight
sequential children, each still bounded to ten seconds. That correction does
not explain or fix this later child timeout. No retry or sleep was added to
CTest, and no further timeout increase or shutdown change was made.

Three isolated quiet diagnostic children exited normally in 7.25, 5.62 and
3.15 seconds. In the second, success was observed about 0.28 seconds before
exit. Two further diagnostic children with normal logging exited normally in
3.02 and 1.40 seconds; both logged worker shutdown, the main-thread stop,
`Shutting down`, and completion of the memory check. None required debugger
attachment or forced cleanup. These observations do not reproduce a deadlock;
they also cannot classify the original timed-out execution. A future recurrence
needs process timing and a stack captured while that owned child is still alive.

## Evidence and scope

Local Windows logs use `out/faction-transform-windows-{build,ctest}.log` and
`out/faction-transform-windows-ctest-followup.log`. Their original and follow-up
CTest SHA-256 values are, respectively,
`e1ecef3e3dc38af73b6141e351b856c1762646d5dc78e5e13195d5ad88d732be` and
`62fc3b8caf134aa296cfad2d82b8692d202e617d2107715025ac20bfa334319c`.
The isolated process records are `out/filter-exit-diagnostics.json` and
`out/filter-exit-nonquiets.json`, with separate stdout/stderr logs per child.
Linux evidence uses `faction-transform-asan-{build,ctest}.log` under
`/home/jhurliman/src/glsmac-gc-audit/`. Preserve these receipts when reusing caches.

The prior actual Engine shutdown harness at the map-publication checkpoint
remains separate evidence. These checks add no rendered restart,
complete save/replay, or original-game UI claim. Native Mac interaction remains
user-confirmed at `5cd78418`; a later read-only follow-up confirmed that process's
normal exit and clean final tracker check in the [Mac receipt](macos-native-2026-10-16.md).
The provided Mac screenshots show Morgan Industries before and University Base
after Start New Game, and the user reported that restart took a while but worked.
The delay remains a profiling follow-up, without an assigned cause.
