# Base intake prerequisites and movement history — 2026-10-17

Validated combined production commit: `990cc59ab24f073185c4a839f4a0bc67e51ed1c4`.
This follows the [resource-context checkpoint](resource-context-2026-10-17.md).
It adds persistent inputs needed by live base intake and later native encounters;
it does not activate a complete canonical base economy or native combat.

| Slice | Source commit | Integration commit |
| --- | --- | --- |
| Explicit generated-map resource policy | `40d993fb0a01ee3e49dd7194dce70209bfb5d293` | `7c8b5393` |
| Owned Base resource context and specialist roles | `8a086296c2cbb712e4870fcd04f391e2eda44d01` | `26aaa7ba` |
| Positional facility identities and safe definition ownership | `90b9b712ae569131db6a5b05591753389f537ee3` | `f3bbf8d5` |
| Persistent placement and nearest-base movement history | `98b1c3bea3fb52008c19ee0b5265d6ab8ffa7975` | `990cc59a` |

The Base context retains owner identity, Golden Age, six local resource-event
flags, their signed timer, and independent governor bits. The explicit first-base
context producer requires the actual claimed startup and measured difficulty;
it grants no population, facilities, production or worker allocation. PopDef
specialist roles bind to frozen catalog positions. A failed frontend submission
now restores PopDef registration ownership before the caller deletes its candidate.
See [Base context](../base-resource-context.md).

Facility definitions retain optional source positions independently of names.
The default SMAC producer identifies HQ and Recycling Tanks; SMAX remains untyped.
Removing an installed definition is refused, while removing an unused definition
releases its owner. Actual manager restore preserves source roles and rejects
malformed later records. See [facility identity](../smac-facility-identity.md).

The [map policy](../glsmac-map-resource-policy.md) explicitly defines how GLSMAC's
displayed terrain maps to resource-query altitude and bonuses. Successful SMAC
generation produces and saves the policy. Deep-water bonus filtering preserves
the candidate RNG draw order. This is a declared port policy, not an equivalence
claim about the original map generator. Raw/imported maps do not silently acquire it.

[Movement history](../smac-movement-caller-history.md) retains successful non-air
placement bits and the optional nearest-base result. Generated-world initialization
is explicit, and restore never replays placement to rebuild the bitmap. The
existing admitted HQ/fungus path retains its query result even on an accepted
failed movement attempt. No distant encounter roll or native creation is enabled.

Versions at this checkpoint: **GlobalSettings11 / World19 / Simulation7 /
BaseManager6 / Unit2**. These are development snapshot versions; unsupported older
records reject instead of acquiring fabricated defaults. Integration conflicts
were limited to additive test registration and the deliberate Base V5→V6 marker
union. Both sides' tests are present in the combined run.

## Combined validation

| Environment | Result | Elapsed |
| --- | --- | --- |
| Windows x64 Clang Debug | 8/8 CTest | 149.11 s |
| Linux ASan/UBSan/LSan, leak detection enabled | 8/8 CTest | 209.68 s |
| Native honey arm64 Apple Clang Debug | 8/8 CTest | 79.12 s |
| Allocation-tracked native regression groups | Same 87 groups pass on Windows and Linux sanitizer binary | Windows 6.20 s |
| LLVM15 correctness analysis | 342 production translation units, zero failures | 253.83 s |

The Windows-only network source is outside the Linux analysis scan and is built
by the Windows check. No timeout or analyzer-policy relaxation was made.

Honey used a clean checkout at
`/Users/jhurliman/.codex-validation/glsmac-20261017-990cc59a/src`, from
15:08:43 through 15:11:03 UTC. Binary SHA-256:
`feab07f7d59527afa94548d9e4723463e5270789c782f087c568f820c996f909`.
Windows binary SHA-256:
`e66ee6a3efe414cb57e1401d5c4cb00bb41128892a78e6703dfd22f7ed04fc4d`.
This is native headless validation; the human interactive/GPU smoke remains at
`5cd78418`, with the reported slow restart still unmeasured.

Each source slice also passed its own normal and sanitizer suites. Their
independent production reviews found no remaining blocker. Source checks include
malformed save rejection, exact continuation, tracked ownership, and copied
negative controls for map policy, Base context and movement history. Facility
identity adds five focused groups covering all 71 positions, malformed/wide
inputs, native binding, actual producer/handler, and manager/Base restoration.

Local receipt: `out/base-intake-foundation-integration-manifest.json`, SHA-256
`881117e4bf43d62a7d38ee08cd5ed66df9e48c1c0037b9d0f5b11aaa00177438`.
It records the exact component source hashes, platform binaries, ordered test
names, source manifests and individual log hashes. Proprietary reference files
and the researcher's uncommitted documentation were not included or modified.

## Remaining consumer work

The next slice connects these owned inputs to actual tile and Base intake. A
separate nutrient-demand scalar can use the frozen per-citizen rate; it must not
pretend that omitted mineral support or energy maintenance equals zero. Complete
allocation/repair, specialist output, energy/citizen calculation, and the manual
handler's preliminary/forced calculation ordering remain separate dependencies.
Recalculation must never post another turn of stored resources.

Native reference work now distinguishes complete component inputs from the old
incomplete-armor diagnostics, and initialized native runtime from supplied zero
coordinates. Neither one nor two creation-stage CRT draws is a universal rule.
The next reference priority is the AI-reference writer/lifetime and optional
assignment stage before native encounter implementation, then offense/retreat.

No newly completed GitHub Codex review is claimed. Focused review publication and
merge gates remain separate from this integration checkpoint; current review-held
heads stay unmerged. A complete conquest match and golden 1.0.0 remain unfinished.
