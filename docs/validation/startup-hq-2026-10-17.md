# Conventional startup and friendly-HQ integration

Integration source: `5ed286fdb78a79fddb81b106fb0a8c7fdf914be4`.
This receipt describes the fork integration branch, not main or approval of
its component PRs.

## Combined behavior

- The [real conventional movement command](../conventional-empty-ground-movement.md)
  owns Unit bytes, tile membership and paired gameplay RNG continuation.
- [Friendly-HQ movement](../conventional-friendly-hq-movement.md) adds the
  measured healthy neutral design-2 Infantry/one-HQ context. Its 70 literal
  original-result comparisons and 22 rejection cases remain registered.
- [Early startup](../conventional-early-startup.md) creates the actual selected
  raw Unit before retained social values exist, then seals readiness after
  staged first-turn validation and before upkeep hooks or frontend publication.
  Restore creates ready records directly and does not replay this lifecycle.
- [Explicit render policy](../cvr-render-policy.md) separates fixed Infantry
  and Rover geometry from optional owned palette colors. Existing outputs and
  descriptor values remain unchanged.

The integration preserves the selected Spartan Rover admission and movement's
extracted owned-design resolver. The shared fixture keeps separate single-roster
and selected-Rover arguments; the Early restore fixture uses the correct one.
Movement validation and application now check object readiness before resolving
their copied context. The actual raw-start hook tests that guard after separately
making social values available, so another admission failure cannot mask it.
Independent review verified these seams and retained test registrations.

Unit V2, simulation V5 and world V15 remain in use at this checkpoint. Early
units cannot be serialized. Default world scripts are unchanged.

## Results

| Check | Result |
| --- | --- |
| Windows x64 Clang Debug CTest | 8/8, 91.16 seconds |
| Linux Clang Debug, ASan/UBSan/LSan | 8/8, 170.75 seconds |
| Native macOS arm64 Debug on honey | 8/8, 65.67 seconds |
| Focused allocation-tracked sanitizer groups | 50 passed |
| Full LLVM15 production analysis | 337 translation units, zero failures, 264.05 seconds |

Linux enables leak detection and halts on sanitizer failures. The analysis
excludes the Windows-only network implementation; Windows compilation is
separate coverage. The 50 focused groups include Early startup, conventional
Unit/movement/HQ cases, selected Rover ownership and real Scout/Rover previews.

Honey uses a new clean checkout at
`/Users/jhurliman/.codex-validation/glsmac-20261017-5ed286fd/src`.
Its executable SHA256 is
`ab39c6b86e78b9fd59b0ca47aadccf2dd53bd6690e3de207c43410a23db897a1`.
The prior interactive human Mac smoke remains `5cd78418`; this receipt adds
automated native validation, not a new visual/input check.

Private source, executable and log hashes are recorded in
`out/startup-hq-integration-manifest.json`, SHA256
`49ac09071c3998c1c85cc384a12b1c9085337d4a833b6bdb6997c822287e0779`.
Windows logs use `out/startup-hq-windows-*`; copied Mac logs use
`out/validation-honey-5ed286fd/`; Linux logs and analysis live under
`/home/jhurliman/src/glsmac-gc-audit/startup-hq-*`.

## Remaining boundaries

Rover/Gaian HQ movement, remote-base cardinality and initialized Infantry
presentation are subsequent slices, not part of this checkpoint. General
worker/economy activation, founding, combat, network command support and a
complete conquest match remain unfinished. The explicit no-project movement
capability and port new-world entropy policy are still documented limits.

The render-policy publication is draft PR #158, based on #152. Fresh GitHub
Codex review remains pending because its review quota is exhausted. Passing
tests and independent source review do not substitute for that requested review.
