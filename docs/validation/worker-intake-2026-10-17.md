# Live intake and worker allocation stages — 2026-10-17

Validated combined production commit: `ce47b2b10226fd3e238dd120ed4f9b4c18f8fb20`.
Source tree: `0505ed2b4d2e9b4b12b68f67da45cb756ef63452`.
This follows the [base-intake foundation checkpoint](base-intake-foundation-2026-10-17.md).

| Slice | Source commit | Integration commit |
| --- | --- | --- |
| Actual-owner specialist eligibility and initial automatic choice | `e5c54fdd4dbd083ae285879a44ba1670db322288` | `fd324fa2` |
| Automatic worker priorities, scoring, ranking and add/swap decisions | `da31a3ff006ba2a6275d185d8aee7df239783e6f` | `0e85b298` |
| Canonical tile resources and live Base intake | `1f32440b5fcd859541c3f55ab6748bb017079c1f` | `ce47b2b1` |

[Live intake](../canonical-resource-intake.md) now connects the actual Tile and
Base callback path to the frozen resource rules, current social ratings, owned
technology, explicit facility roles, saved map policy and Base/global contexts.
It preserves existing worker selections. A separately named nutrient-demand
query supplies only nutrient consumption; full canonical consumption still
rejects because mineral support and energy maintenance are not yet implemented.
Other Base centers and unsupported contexts reject rather than entering an
ordinary-terrain calculation. Eleven focused groups cover actual queries,
seven-faction projections, configured rows, boundaries and world restoration.

[Specialist queries](../smac-specialist-queries.md) bind to the Base's actual
owner and positional catalog. Eligibility uses the original availability helper,
including its definition-sign guard and ordinary-technology boundary, rather
than raw possession. Seven focused groups cover captured menus/choices, real
Base and GSE routes, restore, rejected domains and unchanged allocation/RNG.
These queries do not implement the mutable manual specialist handler.

[Worker allocation stages](../smac-worker-allocation-stages.md) preserve the
automatic governor/AI weights, separate integer divisions, strict ranking,
Human Genome Project boundary, add/swap food guards and rollback behavior.
Sixty-six fixture cases retain original observed outputs. Six focused groups
also check detached callback ordering, failure isolation and the declared
checked-integer domain. The callback aggregates a whole tile; original
per-selector side effects are outside its contract. It is not a complete allocator.

## Combined validation

| Environment | Result | Elapsed |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 CTest | 138.69 s |
| Linux ASan/UBSan/LSan, leak detection enabled | 8/8 CTest | 213.97 s |
| Native honey arm64 Apple Clang Debug | 8/8 CTest | 79.28 s |
| Allocation-tracked regression groups | Same 111 groups on Windows and Linux | Windows 7.281 s |
| LLVM15 correctness analysis | 346 production translation units, zero failures | 268.39 s |

The Windows-only network source is built on Windows and excluded from Linux
analysis. An initial Linux compilation hit an internal LLVM15 code-generation
crash; unchanged source rebuilt successfully with two jobs. The failure log is
retained. No analyzer-policy or test-timeout relaxation was used.

Honey used a fresh clean detached checkout from 15:37:05 to 15:39:27 UTC.
Windows binary SHA-256:
`aaa647c7c5cf4ac9f651c4faa20869063a9a66270dfc7ec59f75ff46c98b7954`.
Honey binary SHA-256:
`02800e905d12a5b753dac0f3e3df75ac7f0d5c2bc6e5f704e1b5a970a3eb20ec`.
These are native headless tests. Human interactive/GPU validation remains at
`5cd78418`; the reported slow restart has not yet been measured.

Versions remain **GlobalSettings11 / World19 / Simulation7 / BaseManager6 /
Unit2**. Integration conflicts were additive test-fixture arguments/helpers;
the specialist and resource setup routes remain independently exercised.
Source checks, original-reference pins and all platform/log hashes are recorded
in `out/worker-intake-integration-manifest.json`, SHA-256
`7685820dadea194c5c0fd08de205c65a0f5479bb1796df29086ebddcb9da10d4`.
The three human-modified tracked documents retain their exact pre-validation
hashes. Researcher's uncommitted files and proprietary game data are excluded.

## Next dependencies

Existing worked-square collection has separately passed its source checks and
awaits the next integration checkpoint. Unavailable-square calculation needs
explicit exploration, recognized territory, visible units, convoy state and
directed relationships. The original startup capture now establishes the
21-square reveal before first-base founding; it must not be replaced by the
later Scout sight radius or a fabricated explored/owned overlay.

Complete allocation still needs specialist cleanup/competition/final fill and
their caller orchestration. Complete balances, support, production/research,
legal founding and terraforming, combat and a conquest match remain ahead.
No default canonical campaign or manual-worker command is activated here, and
no newly completed GitHub Codex review is claimed. Review-held heads remain
unmerged; golden 1.0.0 is not complete.
