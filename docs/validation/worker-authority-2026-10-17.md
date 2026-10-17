# Worker authority checkpoint — 2026-10-17

Production source: `dacac744371c01ec8c298e5f87576af68585f14c`.
Tree: `7419560a4fd75a5dac995db060c7794b557ee6d9`.

This composes lossless Base allocation storage (`82d67e13`) and the owned
worker-map consumer frame (source `faf82922`, integrated as `dacac744`). Both
are detached values with bounded port codecs. Neither is attached to live Base
or Map state yet. Versions remain Global 11, World 19, Simulation 7, BaseManager 6
and Unit 2. The preceding complete allocator checkpoint is
[recorded separately](automatic-worker-2026-10-17.md).

## Behavior and evidence

- Base authority retains the complete selected32 word, specialist count and all
  sixteen ordered stored types, including inactive padding. Tests preserve 49
  captured records and six complete original allocator continuations across
  encoding/decoding. High selected bits affect original admission; the first
  unused specialist slot can become active during compaction.
- The worker frame owns explicit geometry, knowledge, directed predicates and
  semantic entity records. It reproduces 343 original Base-query results across
  35 baseline and 210 omission states, from five unique semantic inputs. Native
  binary roundtrips preserve each input and query result. Native owner0 remains
  distinct from no owner and playable faction IDs.
- Decode bounds, invalid fields, exact framing, aliasing, native visibility and
  copied ownership have focused regression coverage. Two Base-codec and five
  worker-frame copied-source controls fail at their intended assertions. The
  inactive-slot codec control fails a full-storage assertion; it is not claimed
  to reach the later active-padding continuation after that first failure.

See the [Base contract](../smac-base-allocation-state.md) and
[worker-frame contract](../smac-worker-map-frame.md) for pinned reference inputs,
scope and projector provenance. Implementation validation reuses those original
reports; it does not rerun the original executable.

## Combined validation

| Platform | CTest | Time |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 | 134.95s |
| Linux Debug | 8/8 | 100.72s |
| Linux ASan/UBSan/LSan | 8/8 | 206.42s |
| Native honey arm64 Apple Clang Debug | 8/8 | 79.24s |

The sanitizer run enables leak detection and halts on sanitizer errors. The
same 154 ordered allocation-tracked groups pass on Windows, Linux Debug and
Linux sanitizers, retaining the prior 144 plus six frame and four Base groups.
Windows tracking takes 8.265s and reports no tracked leaks.

LLVM15 correctness analysis passes both new complete production translation
units in 1.82s with zero failures. Their headers have no existing production
consumers, so this uses targeted analysis rather than repeating the previous
350-unit scan. It does not claim a new full 352-unit scan.

Windows uses the existing configured build cache; the incremental rebuild takes
14.42s. An unnecessary fresh Windows build was stopped and excluded. An empty
PowerShell tracking-launch log was also excluded; the accepted run invokes the
executable with explicit subprocess arguments and verifies all 154 named groups.
Honey uses a fresh clean checkout at the exact source tree. All validation
processes completed. Native Mac tests are headless; the accepted human
interactive/GPU smoke remains at `5cd78418`.

## Retained receipts and next gate

The combined manifest is `out/worker-authority-integration-manifest.json`, SHA256
`f08032a1dea1373981cd7b2134c4ed82bfa13780c44a45ba81e42f60221ac19d`.
Its platform manifest SHA256 is
`2b0db4ebceb9358e7fb34eb479683d2aa9d36c94192b6c1e31e14863665235c1`.
The combined receipt records source/binary/log hashes, identical group names,
component controls and analysis scope. Private reports and build artifacts stay
outside Git. Three existing human-edited documents and the untracked research
inventory were preserved when fast-forwarding the integration branch.

Focused draft [#164](https://github.com/jhurliman/glsmac/pull/164) publishes the
Base value on the allocator dependency; draft
[#165](https://github.com/jhurliman/glsmac/pull/165) publishes the worker frame on
availability. Their distinct publication trees pass focused normal/sanitizer
checks and analysis. CI for drafts 159–163 is complete with passing applicable
checks; 164–165 was still running at its recorded check. No completed GitHub
Codex review or merge is claimed for these current heads while review quota is
unavailable.

The [live integration plan](../smac-live-worker-integration-plan.md) starts with
optional Base authority and atomic population publication, then actual entity
bindings and persistent knowledge. Sight/territory producers, authenticated
support/prefix inputs and the full turn pipeline remain separate gates. This
checkpoint does not activate a default canonical campaign or complete 1.0.0.
