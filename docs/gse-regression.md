# GSE regression and pre-push checks


CTest registers four stable native shards and each non-include GSE script as an
individual case. Two cases run concurrently in CI on the same runner. Every
native test name belongs to exactly one FNV-1a shard; registration order and
compiler hash implementations cannot change that partition. Each case has a
separate writable prefix. The former script-directory existence assertion is a
configuration precondition. Existing memory-debug and CLI checks remain.

The CLI's ordinary `--gse-tests` still runs the complete suite. Native source
selection and sharding are test-only environment options, require an explicit
`--gse-tests-native-filter`, reject malformed/empty selections, and never turn
zero selected groups into a pass. `GLSMAC_GSE_TEST_SHARD=INDEX/COUNT` partitions
native names; `GLSMAC_GSE_TEST_SOURCE_FILTER=REGEX` matches normalized registration
paths. Clang/GCC capture the AddTest call site; other compilers conservatively
retain groups when provenance is unavailable. CTest clears inherited source
selection for the complete shards and clears both options for script cases.

Every completed group logs its elapsed body **and GSE teardown**. A missing
completion after a start line locates a hung body/destructor. Process exit remains
authoritative even if a later engine teardown fails after printing success.
`tools/summarize_gse_timings.py` publishes slow groups and machine-readable timings
from the retained CTest log; timeout/failure logs are still uploaded.

After the usual cached incremental build, inspect affected consumers before
pushing a shared contract or snapshot change:

```sh
python tools/affected_tests.py --base origin/codex/faithful-1.0
python tools/affected_tests.py --base origin/codex/faithful-1.0 --binary /path/to/cached/GLSMAC --run
```

This helper follows transitive literal GSE/C++ includes, dynamic GSE section
prefixes, and shared include-only fixtures, including removed modules. Shared
production headers, embedded GSE contracts, serialization edits, runtime changes
and unresolved boundaries broaden native coverage. Unknown dynamic includes
broaden script coverage. It preserves the existing independent restore fixtures
that previously drifted when snapshot versions changed. It is a static dependency
selector, not a call-graph proof or a replacement for CI/release coverage. Use the
existing narrow arithmetic-rule loop during development; broaden pre-push checks
for the shared boundaries described above.

## Measured baseline and retained coverage

At the `5f0c18cf` gameplay baseline, the original single-process full GSE run
passed 1,469 registrations in 234.69 seconds on local WSL Linux. The split's
complete CTest run passed all 51 cases in 132.63 seconds on the same machine,
using two concurrent cases on one runner, including the additional source/shard
CLI checks. That is 3.91 versus 2.21 raw runner-minutes for these test segments;
the original baseline omits ancillary CLI cases, so this is a conservative wall
time comparison rather than a claim about total workflow duration.

An exact multiset comparison preserved all 1,429 native registrations and all
39 standalone script registrations, each once. The remaining old registration
was the directory precondition now enforced at configure. Native shards passed
twice: shards0–3 took53.39/32.04/38.06/63.48 seconds, then
53.12/31.05/35.03/61.57 seconds under CTest's saved scheduling order. The first split run exposed an absolute
versus relative script path mismatch in the exact stacktrace fixture; preserving
the original relative invocation fixed it without changing that fixture. The
focused retry and final complete run passed. Native source-selection CLI checks,
21 Python policy/timing tests, and targeted correctness analysis of the changed
production test task passed. The affected-check runner also exercised the real
Ecology registration source and the existing core registration consumers.

The isolated async script took approximately9 seconds locally. This does not
prove a macOS deadlock never occurs. The unchanged monolithic macOS suite in
PR291 hit its300-second limit before the final native groups; splitting preserves
coverage while giving native shards and scripts independent bounded deadlines.
Hosted timing, Windows/macOS stability, and workflow-level runner savings must
still be assessed on the reviewed head.
