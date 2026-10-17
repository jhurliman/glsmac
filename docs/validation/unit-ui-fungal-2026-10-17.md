# Conventional UI and fungal capability integration

Production checkpoint: `e670237216e020abbe57de06921a14012de59868`.
This follows the [three-platform foundation checkpoint](worker-foundations-2026-10-17.md).

| Component | Independently checked source | Integration |
| --- | --- | --- |
| Conventional bottom-bar stats | `0c346b61523d557176c64e7c41b32647763b7fe5` | `96e1ba9c` |
| Owned fungal capability producer | `3dced2ba2fc27bcb55355a5e680a5851f58d2c50` | `e6702372` |

The [bottom bar](../conventional-unit-preview-stats.md) now uses checked
conventional stats for morale and movement. Repeated previews refresh the
existing list item. Its actual GSE callbacks run against real Unit wrappers in
the tests; DOM/assets, sound and timers are doubled. This closes the old
undefined-morale exception without claiming a new rendered UI session.

The [fungal producer](../smac-fungal-capabilities.md) derives N/M/E capabilities
from owned technologies and the frozen faction profile. Recalculation and
restoration do not retain a second mutable capability cache. Source comparisons
cover twenty admitted original builder cases and seven installed first-worker
ownership contexts. The separate mineral/energy faction keyword extension is
not part of this checkpoint.

## Combined validation

| Environment | Complete CTest result | Test time |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 | 107.92 seconds |
| Linux ASan/UBSan/LSan | 8/8 | 203.92 seconds |

All 101 focused native groups pass with allocation tracking on both systems;
the recorded group names match exactly. They include the prior conventional,
rendering and resource groups, both new real-handler UI groups, and faction,
technology and fungal state groups. Focused rendering checks use the purchased
installed assets with Null graphics. The Windows executable runs through a
waited subprocess with checked exit status and success output.

LLVM 15 analysis of the two changed production translation units reports zero
failures in 2.62 seconds. This targeted integration check supplements the source
fungal change's full 338-unit scan and the prior combined full scan. It is not
another full scan. Linux sanitizers retain leak detection, halt-on-error and the
established non-PIE configuration.

The private manifest `out/unit-ui-fungal-integration-manifest.json` has SHA256
`59597d131c905e9851967da3dd416e2977356c4fabc46d866dca9a17734be145`.
It binds the exact source, both component receipts, test groups, logs, binary
hashes and analyzer summary. Original assets and the researcher's uncommitted
evidence are not included in these commits.

GlobalSettings10, World16, Simulation5 and Unit2 formats are unchanged. Native
macOS CTest evidence remains at `2f3b707b`; human interactive evidence remains
at `5cd78418`. Default starting units and live resource callbacks remain
unchanged. A complete strategy match is still ahead.

Ordinary remote-base movement and the specialist catalog are separate active
slices. The next resource consumer must retain worker choices, specialist
identity, base options and resource modifiers as authoritative state. These
integration results do not replace the requested GitHub Codex reviews; pending
draft heads remain unmerged while the review quota is exhausted.
