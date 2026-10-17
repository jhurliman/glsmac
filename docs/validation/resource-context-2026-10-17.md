# Resource context and remote movement integration

Production checkpoint: `9850933c8733a5942098adf60fa2dbc9ac4564b2`.
This follows the [UI/fungal checkpoint](unit-ui-fungal-2026-10-17.md).

| Component | Independently checked source | Integration |
| --- | --- | --- |
| Mineral/energy faction fungal tags | `859f93462e524c5bf82ccbf08f1dc401be5c57b5` | `42e3e965` |
| Frozen specialist catalog | `af35415aaa1791029bee59503bff0546a4a0d481` | `76f1c2d0` |
| Ordinary movement with distant bases | `d4f767137aef1a61147410e26445923bac714cca` | `5bd2f726` |
| Global resource modifier state | `b5361d9e27f122d890ed17c1e7768e2849a79564` | `9850933c` |

The [fungal extension](../smac-fungal-capabilities.md) retains the measured
shared record budget, order and repeated mineral/energy tags. The
[specialist profile](../smac-specialist-catalog.md) freezes all seven positional
definitions and minimum-size metadata in host settings and world snapshots;
display names do not become identity or eligibility rules.

The [movement extension](../conventional-remote-ordinary-ground.md) admits the
24 measured ordinary-ground outcomes and controls without requiring an endpoint
HQ. Its explicit terrain, unit and neighborhood restrictions remain in force.
The [global resource state](../smac-global-resource-state.md) preserves signed
dust countdown, both independent solar phases, and optional Planetary Governor
identity. New-world initialization and restoration remain separate operations.

## Combined validation

| Environment | Complete CTest result | Test time |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 | 132.91 seconds |
| Linux ASan/UBSan/LSan | 8/8 | 225.90 seconds |
| Native macOS arm64 Debug on honey | 8/8 | 78.64 seconds |

All 146 focused native groups pass with allocation tracking on Windows and
Linux; their recorded names match exactly. They include the existing
conventional/rendering checks, the new movement and specialist groups, fungal
and technology state, simulation state, real Start/restore/Client ingress,
and world-header boundaries. Focused rendering checks use purchased installed
assets with Null graphics. The Windows executable is waited through a subprocess
and checked for both exit status and success output. Linux retains leak
detection, halt-on-error and the established non-PIE sanitizer configuration.

LLVM 15 reports zero failures across all 339 applicable production translation
units in 264.23 seconds. This is a fresh full integration scan; the Windows-only
network implementation is excluded from the Linux analysis database.

Honey uses a clean detached checkout of the exact production commit, native
Apple Clang, and arm64 Debug. Its configure/build/CTest interval was
14:33:53 through 14:36:11 UTC on October 17. Its binary SHA256 is
`2168f4cca661679168ba7cbb98ac4aac6f2743cc77eff1d75fb04d62daae90fe`.
This updates automated Mac evidence; the human interactive smoke remains at
`5cd78418`, including the reported successful new-game restart with unmeasured
loading latency.

The private manifest `out/resource-context-integration-manifest.json` has SHA256
`b48268f6120af7c3cb74ff550b31571b974f6e7e823e958c5b6df5ab88d86afc`.
It binds the exact source, four component receipts, platform logs and binary
hashes, tracked groups, and analyzer summary. The only cherry-pick conflict was
the additive test-registration block; both UI and remote-movement groups were
retained. No original asset or human-owned research file is part of these
implementation commits.

## Remaining consumers

Formats are GlobalSettings11, World17, Simulation6 and Unit2. Older unsupported
formats reject explicitly. Specialist selection and worker allocation are not
implemented by the catalog; event activation/aging and council elections are
not implemented by global storage. Default spawning and live resource callbacks
remain unchanged, and this checkpoint is not a complete strategy match.

The next resource slice owns per-base context and saved map semantics, then
feeds the actual tile-query and base-intake callbacks while preserving existing
worker decisions. The next movement slice retains caller history before
broadening fungus encounter admission. These integration checks do not replace
GitHub Codex review; pending heads remain unmerged while its review quota is
exhausted.
