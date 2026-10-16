# Backend teardown before the script runtime

In single-thread mode, Engine registers backend Game before Scheduler. This is
needed for startup, but reverse module cleanup used to stop Scheduler first.
Scheduler's task deletes GLSMAC, whose real GSE teardown destroys State. The
later `Game::Stop` then calls `State::RetireGameCallbacks` through its retired
borrowed pointer. Default worker mode already joins Game before MAIN cleanup.

`Thread::StopModuleFirst` now permits one explicitly registered module to stop
before the remaining reverse cleanup. Engine nominates Game only in single-thread
mode. Start and Iterate order remain unchanged. The nomination must be made
before the first Run, refer to exactly one registered module, and cannot replace
another nomination. Invalid use throws in Release too. This is a setup API,
not a concurrent module-reconfiguration interface.

Cleanup considers only the prefix whose Start calls returned. The nominated
module stops once; a failed or unstarted nominee is not stopped. Its Stop failure
does not replace an earlier exception or prevent other started modules from
stopping. The default worker shutdown path is unchanged.

## Actual-runtime regression

The opt-in [runner](../tools/test_engine_shutdown.py) links an existing application's
objects with [a standalone main](../tools/tests/engine_shutdown.cpp). It runs real
Engine, Thread, Simple scheduler, backend Game Start/Stop, GC, GLSMAC destruction,
GSE and State. Null presentation services and the existing asset-free GLSMAC
constructor avoid installed assets. The scheduler test task owns that runtime
and deletes it in Stop, as the ordinary Main task does.

The Game subclass gates only Iterate during setup; this is a shutdown ordering
test, not an initialized-world or rendered-game test. A State destructor sentinel
records its real lifetime. The actual backend Stop observer records whether
State remains alive. On bad ordering, the safe assertion clears only that already
retired pointer before delegating Stop, returns failure, and still permits native
cleanup. Passing runs delegate unchanged Stop with the original pointer alive.

Run after building this checkout with a Linux/WSL single-config Ninja Debug build
and an exported compilation database:

```sh
python3 tools/test_engine_shutdown.py --build-dir /path/to/existing/build
```

The runner validates the expected direct GCC/Clang object/link command layout,
replaces only main, and runs default plus `--single-thread` subprocesses with
memory tracking. It rejects unsupported platforms/build layouts and missing
objects. Build GLSMAC first after any source changes; the runner does not rebuild
or establish that existing objects are current. Compile/link are each bounded to
60 seconds and each child to 20 seconds. Logs, commands, timings and hashes remain
in `BUILD_DIR/engine-shutdown` (or `--output-dir`). Sanitizer/library environment
settings are inherited from the supplied build environment. No extra full-suite
CTest run or proprietary asset dependency is added.

The existing fast lifecycle harness also checks the single-thread Engine wiring,
normal early cleanup, early/later Start failure, early Stop failure, combined
Iterate/Stop failure, and invalid nomination. It still uses unrelated service
doubles; the standalone check above is the separate real-runtime evidence.

## Observed validation

Before the fix, the real Engine trace was:

```text
default:       backend.stop state_alive=1 -> runtime.stop -> state.destroy
single-thread: runtime.stop -> state.destroy -> backend.stop state_alive=0
```

The safe single-thread control exits 1, while default exits 0. An isolated
sanitized control allowed the unchanged old Stop to execute: UBSan halted on the
member call through destroyed State at `Game.cpp:340`, with actual
Thread::Run/Engine::Run in the stack. This is a demonstrated lifetime defect;
it does not attribute any previous intermittent network-download timeout to it.

Source commit `f586b523b8764152586987bb3638105f740191e9`
passes both actual-runtime modes with Clang 15 Debug and
memory tracking, and both also pass ASan/UBSan/LSan with leak detection enabled.
Each runner, including compile/link and both children, takes 6.53 seconds normally
and 8.03 seconds sanitized. Full CTest passes 8/8 in 36.45 seconds normally and
42.72 seconds sanitized. The 24 fast lifecycle cases pass with GCC and with
Clang 15/libc++ under ASan/UBSan. Explicit analysis of Thread.cpp and Engine.cpp
reports zero failures in 3.92 seconds. Native Windows/macOS execution of this
slice is not claimed. Local sanitizer linking used the retained diagnostic
`-no-pie` setting; this is not a committed build policy.

The focused publication initially passed its five ordinary CTests but the actual
Engine runner failed before creating its State: OSSP UUID export used a library
allocation that the application's memory tracker could not free. It now includes
the unchanged production fix and focused tracked regression from reviewed PR #37
(`b8e068e928` and `501702850cc02d89c047c915a776844042c86d7e`).
Publication `e48d050cc4c372ec916ddaedbe42aedfdf89add6` passes CTest 6/6
in 23.34 seconds, all 24 lifecycle cases under GCC ASan/UBSan, and both actual
Engine modes with allocation tracking. This is not a full sanitizer claim for
the older publication fixture.

Combined integration `2b7ca61c39e34e56965a13edade6fd63f57e7a30`
passes Windows CTest 8/8 in 63.50 seconds, Linux ASan/UBSan/LSan CTest 8/8 in
51.63 seconds, and LLVM 15 analysis of all 298 eligible production translation
units in 68.99 seconds, zero failures (Windows-only networking excluded).
Its actual Engine harness also passes both modes with sanitizers and allocation
tracking, and the GCC sanitizer lifecycle harness passes 24/24. The actual
Engine runner initially refused the missing compilation database; exporting it
and rebuilding enabled that independent check. No native macOS or rendered
shutdown test is claimed for this integration.

A copied pre-fix Engine.cpp from `b2ec36b83ee3bbd68f9bd6073eed2fcc4b35fb1b`,
linked against the final standalone harness and current remaining objects, still
passes default and fails single-thread with the exact lifetime assertion. The
working production source was not reverted. Retained WSL evidence is under
`/home/jhurliman/src/glsmac-engine-shutdown-audit`:

- Initial real-runtime/sanitizer diagnostic `manifest.json`:
  `641f1927589c590bcf15a2f9ee80ec6d085a04e2ae07edcc1c96f51638153000`.
- Final copied-Engine control `final-negative/manifest.json`:
  `e830fd671d3f2faa48bb7abba91abf8c3cbd44b07c09bad6c8e1961246155b5b`.
- Final command records and per-mode traces: `final-validation/`.

This repair does not add native ResetGame/collector exclusion, staged startup,
or cancellation of an already executing owner callback. Those remain separate
coordination and lifetime requirements.
