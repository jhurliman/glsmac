# Public reset controller polling

`GLSMAC::Iterate` now treats a requested reset as a scheduler boundary. Before
running loader completion, GSE, UI, frontend, or console work, it reserves the
backend controller cutoff, retires every old frontend MT ID outside script
accumulation, and submits that exact saved cutoff. Later iterations poll only
that request. This lets the backend run on the same thread instead of waiting
inside the owner loop for work that the loop cannot service.

Preflight also runs after callback-producing stages, so a reset requested by a
script stops the following UI/frontend stages in that iteration. Repeated reset
requests retain the same barrier/request. The active-game and reset guards stay
set until the matching terminal response succeeds. Response retrieval releases
the controller record; the controller destroys the response once, clears its
handles and guards, and calls the actual `RunMain` once. That terminal iteration
does not resume old callback stages.

The ticket API remains responsible for cutoff identity and stale no-op behavior.
An abandoned preparation releases its own barrier. Backend MT shutdown owns an
unconsumed request/response; GLSMAC does not consume it again during destruction.
An unexpected terminal operation/ticket/result is a release-visible error and
leaves the controller guarded. Current `OP_RESET` only returns success, including
a stale no-op, so there is no claimed real-backend error-response regression.

## Tests and limits

Five native cases use actual `S_Reset`, `GLSMAC::Iterate`, `RunMain`, GSE pending
work, UI iterable registration, a real unstarted frontend, backend MT extraction
and responses. Engine services are temporarily installed under the GC spaces
mutex and remain alive through local runtime destruction. The tests cover:

- Same-thread pending/success and reset requested during the script stage.
- Every frontend MT ID, including pre-initialization and debug IDs, retired
  before the controller reset is submitted.
- No GSE/UI/frontend/load-completion callback while waiting or on the terminal
  tick, followed by normal stage resumption.
- Repeated requests, stale cutoff no-op, and shutdown before/after processing.
- Actual collection retaining pending owners, canceled bodies not executing,
  and exactly one owner destruction after drain or fixture shutdown.

The test main callable uses a cached, test-owned argument; it does not exercise
the global configuration wrapper. Initial full wrapper setup exposed an existing
552-byte `Config::Wrap` native-cache leak under LSan; the trace is retained
separately. No sanitizer suppression or production configuration change is used.

Run the focused cases from the repository root with a Debug build:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^public reset polling ' --memorydebug --prefix build/reset-poll-data
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^public reset polling ' --single-thread --memorydebug --prefix build/reset-poll-single-thread-data
```

The default full native suite includes these cases. There is no additional full
CTest suite or default slow test. A copied original `GLSMAC.cpp` from the ticket
prerequisite is tested in a bounded subprocess: the same-thread case cannot
complete its old busy wait. Separate copied-TU controls remove entry preflight,
post-script preflight, or frontend retirement and must fail the real assertions.
The working source is never temporarily reverted for these controls.

Local Clang 15 Debug validation on source commit
`b2ec36b83ee3bbd68f9bd6073eed2fcc4b35fb1b` passed full CTest 8/8 in
36.45 seconds and ASan/UBSan/LSan CTest 8/8 in 42.91 seconds. The five focused
cases passed with memory tracking in 1.07 seconds, and with `--single-thread`
in 6.08 seconds (sanitized: 1.07 / 6.13 seconds). LSan remained enabled.
Explicit-TU LLVM 15 analysis of `src/GLSMAC.cpp` reported no failures in
4.63 seconds. The focused publication commit
`31a37f6d405becdf9c9bfd2dc85c7372db942baf` separately passed all five available
Linux CTests in 22.32 seconds. Its older fixture does not include every allocation
tracking repair from the source branch; the source sanitizer results above are
not a sanitizer claim for this publication branch.

Combined integration commit `9663fcdabab888e3a77d7361dafd1bbec3ab3673`
passed Windows CTest 8/8 in 48.83 seconds, Linux ASan/UBSan/LSan CTest 8/8 in
111.62 seconds, and all 298 eligible production translation units under LLVM 15
analysis in 56.86 seconds (zero failures; Windows-only networking excluded).
Those combined checks include other pending fixes. No native macOS run of this
reset controller change is claimed. The earlier interactive macOS evidence is
for `5cd78418271231121fb9a8e1d136faa82bbc5dd0`.

The publication stack also carries the direct `<algorithm>` include required
by its callback-retirement prerequisite. Hosted MinGW and libstdc++ exposed that
missing include; the local libc++ build had provided it transitively.

The original copied TU hit its three-second subprocess limit; each of the
three omitted-boundary controls exited 1 with its intended assertion. Retained
WSL evidence is `glsmac-reset-poll-controls/manifest.json` under
`/home/jhurliman/src`, SHA-256
`2d2fba64985ce2ca6a54a923e5ba843b9e495a23a82c09fbd292b424d2cf7d2a`.
Commands, copied-source/test hashes, timing, and individual logs are recorded
there. Positive command records/logs are in
`glsmac-tracked-native-test-filter/normal/reset-poll-validation.json` and
`glsmac-unit-despawn-rollback/asan/reset-poll-validation.json` under the same
parent directory. The earlier global-wrapper leak log is retained beside the
sanitizer records as `reset-poll-global-config-wrapper-leak.log`.

This is the reset-controller scheduling prerequisite. It does not exclude an
already executing owner callback from native teardown, protect native reset from
the collector, or implement staged initialization. The fixture runs native reset
under accumulation to isolate those independent problems. It does not prove
rendered reload, arbitrary mod reentrancy, or full Engine shutdown in single-thread
mode. A separate actual Engine ordering regression now confirms that MAIN stops
Scheduler/GLSMAC before backend Game and destroys its State too soon. The focused
shutdown fix and its validation are separate from this controller change.
