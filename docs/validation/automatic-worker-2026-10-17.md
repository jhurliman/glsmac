# Complete detached worker allocation — 2026-10-17

Validated production commit: `df61f01562451f42a45f9edc4710b6093ac44da4`.
Source tree: `80b540a8202b89efb9768ebbe55618972520bd63`.
This follows the [live-intake checkpoint](worker-intake-2026-10-17.md).

The [complete detached allocator](../smac-automatic-worker-allocation.md) now
composes existing-square collection, specialist cleanup, candidate scoring,
worker additions/swaps/rollback, specialist competition and final filling.
It preserves the original complete-allocation early exit, managed-upkeep path,
missing-center query timing and fresh yield queries. The independently callable
[availability builder](../smac-worker-availability.md) consumes explicit map,
unit and directed relationship knowledge. Neither publishes live Base state.

| Slice | Source commit | Integration commit |
| --- | --- | --- |
| Existing worked-square collection | `f2d76515` | `a996c6b2` |
| Single-adjustment horizontal wrapping | `ab6520a7` | `90a567fd` |
| Specialist bookkeeping | `b6d356d3` | `a16b5c24` |
| Complete allocator and adapter controls | `67a10f76` | `67a10f76` |
| Worker-square availability | `6314eb2e` | `dbf113c8` |
| Complete original return/trace projections | `8d7e16c2` | `34d948e1` |

The complete allocator matches 51 original full-routine outcomes, including
27 observed pre-fill counter pairs, 26 ordered candidate query states and eight
missing-center query states. Availability matches 736 original masks and their
selection cleanup results. Collection and specialist stages retain their own
pinned original fixtures. Seven copied allocator mutations each fail an intended
regression assertion; these are test-sensitivity controls, not new original runs.

The narrow-map correction came from inspecting the original collection loop:
it makes at most one horizontal adjustment before bounds checking. The earlier
modulo helper could incorrectly retain a square requiring two adjustments.
Ordinary 128-by-64 reference cases are unchanged.

## Combined validation

| Environment | Result | Elapsed |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 CTest | 139.13 s |
| Linux Debug | 8/8 CTest | 94.70 s |
| Linux ASan/UBSan/LSan, leak detection enabled | 8/8 CTest | 204.58 s |
| Native honey arm64 Apple Clang Debug | 8/8 CTest | 79.08 s |
| Allocation-tracked regression groups | Same 144 groups on Windows and both Linux builds | Windows 8.312 s |
| LLVM15 correctness analysis | 350 production translation units, zero failures | 243.23 s |

The full analysis started at `dbf113c8`; subsequent changes were documentation,
test fixtures and their registration. Production sources were unchanged.
Windows-only networking is built on Windows and excluded from Linux analysis.
Honey used a fresh clean checkout of the exact validated commit. These are native
headless tests; human interactive/GPU smoke remains at `5cd78418` and the reported
slow restart has not yet been measured.

Versions remain **GlobalSettings11 / World19 / Simulation7 / BaseManager6 /
Unit2**. All three human-edited tracked documents preserve their prior hashes.
Proprietary assets and the researcher's uncommitted files are excluded.
Source receipts, binaries and logs are hashed in
`out/automatic-worker-integration-manifest.json`, SHA-256
`554b0b12757d636b86c9b30ab34082bac2ac144a2aa5996a4f9bd7d361425c2b`.
The Windows/honey manifest SHA-256 is
`f92509e2ff81faaae15316c981591a6eec69a6a8c1ec74abc241ec584f68617f`.

## Next live dependencies

Saved exploration, recognized territory, unit visibility/convoys and directed
relationships must feed the availability builder. The accepted original
worker-frame replay reproduces 35 expansion states in fresh consumers; a
separate Base authority receipt establishes lossless full-width selections and
all sixteen ordered specialist positions. Unused padding cannot be discarded:
the original compaction can move it into an active slot.

These values need owned persistence and checked live bindings before allocation
can publish population changes. Full resource balances, support, production and
research posting remain separate work. Whole-tile callbacks do not reproduce
the original resource selectors' individual side effects. No default canonical
campaign or manual-worker command is activated here. Focused draft publication
continues; current unreviewed GitHub heads remain unmerged while Codex review
quota is exhausted. Golden 1.0.0 remains in progress.
