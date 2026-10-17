# Focused native GSE tests

Debug and FastDebug builds accept `--gse-tests --gse-tests-native-filter REGEX`.
The harness compiles one ECMAScript `std::regex` and uses `std::regex_search`
against each native test name during registration. Use `^` and `$` when the
whole name must match. An empty expression selects all native tests. Scripts
are not registered in this mode. Invalid expressions and zero matches print a
clear diagnostic and return exit status 1.

Like `--gse-tests-script`, the option must follow `--gse-tests`. Script and native
selection are mutually exclusive in either argument order. With neither
selector, the existing full native and script suite runs unchanged.

For example:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter 'rectangle getters' --quiet --prefix build/test-data-rectangle
```

The Debug `gse_mesh_memorydebug` CTest uses an anchored alternation of eight
explicit mesh and UI ownership test names. Six are registered in this isolated
base; the two renderer ownership checks are added by the dependent renderer fix.
Tracking is enabled from
process startup and the test uses its own prefix and a 30-second timeout. The
native harness calls each test outside script accumulation; the fixtures can
therefore create/destroy their local GSE runtime and temporarily replace the
headless graphics object without holding another Space's collection lock.

This matters because the collector holds the global spaces mutex while it
collects each Space. A script callback already holds its Space's collection
mutex. Creating another runtime or acquiring the graphics fixture's global
spaces mutex inside that callback can reverse the lock order and deadlock.
The script-callable `test.check_mesh_coordinate_outputs()` now contains only
three pure mesh checks. The native and full-suite registrations still run all
six existing checks (and the renderer fix adds its two native cases). No GC locking or production UI ownership is changed here.

`gse_native_filter_cli` checks real CLI parsing, substring matching, invalid and
unmatched expressions, missing values, required test mode, both incompatible
selector orders, and the pure mesh script bridge. Each subprocess has a distinct
prefix and a 10-second timeout; crashes and timeouts cannot pass as expected
exit failures. These invocations do not rerun the full suite.

## Evidence

The integrated Windows build at `ddcf747a` passed the full GSE suite but timed
out in the old script-based `gse_mesh_memorydebug` after 30.19 seconds. Nine
unchanged focused retries passed. No Windows stack was captured for that
intermittent timeout. The lock inversion described above follows from the
script accumulation, fixture, runtime registration and collector source paths;
this change removes those nested fixture calls rather than changing GC locks.

Fresh Linux Clang 15 Debug validation passed all 8 CTest entries in 37.45
seconds. The tracked six-test selection took 1.02 seconds; the new eight-case
CLI check took 4.10 seconds. The verbose CTest log names all six selected native
tests and shows that the focused run executes no script. The existing curated
clang-tidy checks passed for `Config.cpp` and `GSETests.cpp`.

A temporary negative control omitted the failure increment for an unmatched
filter. The real CLI check failed in 3.05 seconds with
`zero_match: expected exit 1, got 0`, despite the application's misleading
“All tests passed” output. After restoring the source and rebuilding, both
focused CTests passed in 5.15 seconds. The negative control is not committed.
Evidence is retained locally under
`/home/jhurliman/src/glsmac-tracked-native-test-filter/` in `ctest.log`,
`negative-zero-match.log`, `restored-focused-ctest.log`, and `analysis.log`.
The final filter also names the two pending renderer ownership tests; they are
not registered in this isolated base and are not part of its six-test evidence.
Native Windows and the combined eight-fixture run remain integration checks.

### Wrapper timeout accounting

At integration `55636ab5`, native Windows passed seven CTest entries, including
the full GSE suite, but the filter wrapper reached its 30-second aggregate limit
after 30.82 seconds without case-level output. An unchanged focused retry passed
in 9.82 seconds; no engine hang or production-state defect was established.
The wrapper runs eight subprocesses sequentially, each with its own 10-second
limit. Its CTest limit is now 90 seconds so that those individual limits can
report failures before the aggregate watchdog fires. Every case announces its
name before launching; passing CTest output remains quiet. No sleep, retry or
child-process timeout increase was added. The initial run and unchanged retry
are retained in `out/production-state-windows-ctest.log` and
`out/production-state-windows-filter-retry.log` in the integration checkout.
The updated focused CTest passed in 9.19 seconds; its verbose log confirms the
90-second outer limit and all eight case names (`out/filter-timeout-validation.log`).
