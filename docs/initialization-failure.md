# Initialization failure cleanup

`Game::InitFailed` supports failure before a Map or any game manager exists.
It owns a copy of the original failure and publishes it on the current world
admission before invoking the script `error` hook. A caller-owned message cannot
be changed by that hook into a different terminal error.

When a map exists, the helper releases its untransferred terrain texture and
render/data meshes before notification, preserving the previous presentation
cleanup order. The hook still receives the original Game and State. A secondary
GSE or native exception from this hook is retained only for diagnostic logging;
it cannot skip `ResetGame`, replace the original error, or revive an admission
retired by the hook. Cleanup clears the State pointer and publishes loader hide
after ResetGame has cleared the old frontend queue. This tests the queued hide
request, not its rendered delivery after world-ticket retirement.

Map-data responses retain the original error only for the failed ticket. An
unrelated or explicitly retired ticket receives `R_ABORTED`, with ordinary
response destruction preserving ownership. The helper does not add a second
exception response or treat failure cleanup as successful initialization.

The existing retained-old-map branch transfers its native pointer exactly once,
clearing `m_old_map`. Subsequent ResetGame or Stop releases that map once. This
is an ownership correction only: State, managers and lifecycle handlers have
already been cleared, so this does not establish a working rendered reload or
restore the previous world's gameplay lifecycle.

## Regression scope

Seven asset-free native cases call the production helper: absent map/managers,
ordinary map presentation ownership, throwing GSE and native error hooks,
retirement from the hook, and retained-map cleanup by ResetGame and Stop.
They inspect the real MT map-data/error/reset responses and exact original
message, including mutation of the caller's message during notification.
The Stop fixture transfers real tracked queue/Random allocations to the backend.
All destructive calls run inside fixture accumulation to isolate the separately
identified native reset/collector-exclusion gap.

With unchanged production code, the throwing-hook case failed its cleanup
assertion (zero passed, one failed). The absent-map case exited with SIGSEGV;
GDB identified the unconditional `m_map->m_textures.terrain` access inside
InitFailed. Logs are retained in the isolated worktree's ignored validation
directory. No proprietary assets are needed.

Implementation integration
[`67ac1e6357681bdc5060c2d44864b975cfb40b8c`](https://github.com/jhurliman/glsmac/commit/67ac1e6357681bdc5060c2d44864b975cfb40b8c)
passed all seven focused Linux cases with allocation
tracking, normal CTest 8/8 in 50.58 seconds, and ASan/UBSan/LSan CTest 8/8 in
44.64 seconds with leak detection enabled. The sanitizer build retains the
documented local `-no-pie` startup workaround. LLVM 15.0.7 correctness analysis
of Game.cpp passed in 10.75 seconds. Source-copy controls that reintroduced the
old-map alias or borrowed the original error each failed their targeted case
(zero passed, one failed); the production sources were not changed by those
controls. The first fixed run exposed a test observer calling the map-required
getter in the no-map case; that observer was corrected before the final runs.
These are local measurements, not hosted or cross-platform guarantees.

The focused publication initially timed out in the native-error-hook case
because its older base omitted PR #54's callback-mutex exception repair.
GDB showed the test waiting in `Wrappable::HasHandlers` and collection waiting
for its active accumulation. The unchanged #54 repair and its regressions are
included as a prerequisite; no test is skipped. Publication
[`1f39d966247ee2fe0e6c25f9ca55d3d75a052a8d`](https://github.com/jhurliman/glsmac/commit/1f39d966247ee2fe0e6c25f9ca55d3d75a052a8d)
then passes Linux CTest 5/5 in 23.29 seconds. The older focused fixture lacks
some integration allocation-tracking repairs, so the sanitizer results above
apply to the named integration, not this publication. Initial and corrected
logs use `early-failure-` and `early-failure-with-trigger-` under
`/home/jhurliman/src/glsmac-unit-publication/`.

Combined integration
[`9663fcdabab888e3a77d7361dafd1bbec3ab3673`](https://github.com/jhurliman/glsmac/commit/9663fcdabab888e3a77d7361dafd1bbec3ab3673)
adds the public reset polling prerequisite. It passes Windows CTest 8/8 in
48.83 seconds and a fresh LLVM 15 scan of 298 Linux production translation
units in 56.86 seconds with zero failures; Windows network code is outside
that Linux analyzer scope.

This prerequisite does not schedule early startup failure notifications itself.
The owner-transition coordinator must establish when initialization/configuration
work and this helper execute, prevent concurrent reset, and route deferred
exceptions. Native collector exclusion, general lobby concurrency, out-of-memory
transactionality, and restoration of the old world's lifecycle remain separate.
