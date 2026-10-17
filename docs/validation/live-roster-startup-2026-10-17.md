# Live roster and predefined-unit checkpoint — 2026-10-17

The tested integration is `75c280c857d16a40c3651c8a06b5cd8d89480012`, retained as
`codex/validation-20261017-live-roster-75c280c8`. Relative to the preceding
world-size checkpoint it adds frozen faction startup metadata in GlobalSettings
V6/world header V7, the owned predefined SMAC unit producer, production resource
overflow regressions, and the coherent live Client/Server roster protocol.
Its smaller publication PRs have separate heads, checks and dependency gates.

| Environment | Configuration | Result |
| --- | --- | --- |
| aleph0 native Windows | Clang Debug, full CTest | 8/8, 58.83 seconds |
| aleph0 Ubuntu/WSL | ASan/UBSan/LSan, leak detection and halt-on-error | 8/8, 90.63 seconds |
| honey native macOS arm64 | Apple Clang Debug, full CTest | 8/8, 43.10 seconds |

Linux uses the documented local non-PIE sanitizer configuration. These are
asset-free tests of the combined source, not a new interactive gameplay run.
The earlier user-confirmed Mac map/base/turn/restart/close check at `5cd78418`
remains separate. New-game latency is still untimed.

## Scope exercised

Twenty new native roster groups exercise actual Client and Server packet paths,
stable occupied Player identities, canonical faction lookup, initial settings
staging, stale-epoch rejection, bounded encoded records, queued notification
roots, exception retry, and observed collector overlap during retirement.
They preserve native state through continuous metadata refresh rather than
replacing Players. The implementation's
[contract and source receipt](https://github.com/jhurliman/glsmac/blob/75c280c857d16a40c3651c8a06b5cd8d89480012/docs/live-roster-publication.md)
describe the supported seven-slot protocol and its exclusions. Transport slots
are not original-game simulation faction IDs.

The predefined producer is an owned value, not a live backend unit definition.
Its separate Release comparison matches 546 original final records, 436 records
before overrides and 254 direct cost/capacity results. Those comparisons, their
negative controls and explicit automatic-plan limits are recorded in the
[pinned producer receipt](https://github.com/jhurliman/glsmac/blob/75c280c857d16a40c3651c8a06b5cd8d89480012/docs/smac-predefined-unit-catalog.md).
They do not establish unit placement, workshop behavior or conventional combat.

The production worker-score and base-intake callbacks now have explicit extreme
integer regressions. They exercise the existing checked GSE arithmetic; the
two copied old-interpreter controls reproduce undefined multiplication and
addition under UBSan. No new numeric resource bounds or saturation policy was
introduced. Full-range display formatting remains separate from whether a
simulation calculation has a representable result.

## Native Mac provenance

The isolated checkout is
`/Users/jhurliman/.codex-validation/glsmac-20261017-75c280c8/src`, cloned from
the prior validation checkout and advanced by a local Git bundle. Configure,
build and tests ran from 08:26:29 to 08:28:04 UTC on October 17 on macOS 26.5
(25F71), native arm64, with Ninja and four build workers. A later status check
confirmed a clean checkout. No existing interactive session was interrupted.

- Binary SHA-256: `966d6c4e75716a9d87032cb3ef312100c7b3dad6b4c7e489597663458a531415`.
- Transfer bundle SHA-256: `c451e80e5be387763dd7aab42ce7477fdfa05399f5cdce024a1c5245b6432064`.

## Retained evidence

Windows logs: `out/live-roster-windows-{build,ctest}.log` in the integration
checkout. CTest SHA-256:
`e3e35040906229f5cf30000002f94c26e2467e118bffc9caabccc0e5a66503ed`.

Linux logs:
`/home/jhurliman/src/glsmac-gc-audit/live-roster-asan-{build,ctest}.log`.
CTest SHA-256:
`b9f2713d43751d4749e08b3f11b6e75535e87478105ac9a2af017ac5a54f9f90`.

Mac logs: the isolated checkout's sibling `logs/` directory, copied to
`out/validation-honey-75c280c8/`. CTest SHA-256:
`c113f5c1bfc7c2fca117e8a186b16ff1f9afce222a361579c7b6f9e4f90d780d`.

The full Linux correctness scan analyzed 312 production translation units in
233.33 seconds with two workers and reported zero failures. The Windows-only
network implementation is explicitly outside that scan. The retained summary is
`/home/jhurliman/src/glsmac-gc-audit/live-roster-analysis/summary.json`, SHA-256
`8204a1767e2c3a9e25e899bad1a680c16731299b60650fc4a16dc29f6b4b4bff`.

The narrower roster publication initially lacked the previously reviewed empty
Buffer allocation repair and failed its optional tracking run on zero-byte
allocations. That failure is retained; adding the unchanged prerequisite made
the actual PR126 publication pass all six CTest entries in 28.47 seconds and
its full tracking run in 21.33 seconds without tracked leaks. The combined
source tested here already contains that prerequisite. Neither publication
repair nor these passing suites closes the separately held startup/failure/MT
findings in PR77. World downloads are not yet disk saves or deterministic replay,
and M0/M1 and the complete conquest scenario remain open.

Review also found that the complete base screen still performs narrow balance
arithmetic outside its tested resource panel. An accepted endpoint can therefore
raise a checked GSEMathError when opening the complete screen. PR32 remains held
for that correction; passing callback and isolated-panel tests do not cover it.
