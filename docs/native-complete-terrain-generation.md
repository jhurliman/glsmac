# Complete native terrain producer before faction startup

`GenerateTerrain` now runs the owned native producer from supplied initial
records/RNG through original `0x5e038e`, before faction startup. It composes
bootstrap/retries, initial climate, all thirteen landmarks, height-profile
preparation, contour normalization, final climate/routing, region rebuilding,
the postpass and final rockiness/resource features. No captured intermediate
maps or helper journals are production inputs.

The actual seed-1 Standard Rules acceptance starts from zeroed 80x80 storage,
initial region storage, a 16x16 zero-initialized summary and RNG seeds 1/1.
External clock values are explicit inputs: 100 for boreholes and 110 for the
final climate reseed. Seed alone does not determine the complete output.

| Output at the original pre-startup boundary | Expected |
| --- | --- |
| Complete 140,800-byte tile plane SHA256 | `17095ab34ed6a22f2e47429aa14d7eb0bb5d40001c6b21e9641d41cad9c733f9` |
| Complete 3,584-byte region storage SHA256 | `64af57cdefc3c6a7f0350113582503b15519f436fff9c8d2cf95672da0647590` |
| CRT / shared RNG | `3551643837` / `110` |
| Dirty / generation flags | `5` / `0,0` |
| Working thresholds | `0,20,40,60,80,100,120,140,160,180,200` |
| Names | 12 labels, all 480 encoded bytes match |
| Height profiles | All five 113-byte buffers match |

## Ordered completion

`CompleteTerrain` accepts the prelandmark continuation and completes the same
tail without restarting earlier stages. It prepares height profiles before
normalizing contours, clears the first generation flag, reconstructs classes,
computes temperature, routes rivers, computes rainfall, rebuilds regions and
runs the postpass. It then reseeds shared RNG from the supplied final clock,
assigns final features, clears the second generation flag and marks
`Stage::Prestartup`.

Reconstruction retains working thresholds. River routing deliberately uses the
existing static new contour bounds even when working thresholds differ. The
shared climate algorithms operate through an owned semantic view and publish
only their owned fields; raw owner/visibility/unrelated bytes remain intact.

Height-profile preparation is now one shared operation used by both producer
and renderer. It does not allocate rendering caches. The profile array order
is membership masks `2,1,0x200,0x100,0x800`, independently of the reference
receipt dictionary's insertion order. Negative-detail suppression and profile
admission remain the existing renderer behavior.

## Evidence and validation

Six original final-climate controls compare complete tile/region/summary
buffers, dirty/secondary flags, **1,250 ordered CRT draws** and **16,138 shared
seed/draw events**. Wrapped/flat worlds, stale regions, retained raw fields,
routed sources and old working thresholds discriminate the routing-table rule.

The actual setup test independently executes from zeroed allocation, checks
each final preparation boundary, all height/name bytes and the complete tail's
**3,008 ordered CRT draws** and **6,795 shared events**. The single-call producer
also matches that stepwise execution, including retry results, region/summary
storage and both cleared generation flags. This is not an ordered-store proof
for semantic river/rainfall publication or a globally interleaved instruction
trace; those adapters are checked through full buffers and ordered RNG.

The cached incremental build and nineteen affected native groups pass, including
existing native view, saturated-corner, negative-landmark and finalizer checks.
Correctness analysis covers eight production TUs; focused six-TU ASan/UBSan
checks cover the new producer, shared shaping/climate, backend geometry and test
consumers. Unchanged successful checks are reused; no platform matrix repeated
locally.

Source executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Corrected frozen v2 manifest SHA256:
`14bf175ce2548817e73cf887cc5ecdcbfd8d4cf0de8d5f7d07b023cc9796bdc4`
(all 106 files verified before fixture import). The earlier terrain-tail bundle
has manifest SHA256
`e617d515b91a3bec9f83c2ea0e7b469b94c386d099f84eb3273d0435f53265c3`.
The corrected area/freshwater controls are retained; inherited mislabeled
zero-area cases are not treated as threshold proof.

## Remaining boundary

This produces terrain, regions, labels and profiles ready for startup. It does
not construct factions, Bases, Units or initial exploration. The independent
terrain-to-original-startup composition is evidence for the input boundary,
not an independent startup implementation. The post-startup plane hash is
different and must not be substituted for the pre-startup hash above.

The live `Map::Generate` still uses its previous procedural path. Common raw
authority, faction startup, campaign/network persistence and the
[live integration gates](native-generated-world-integration.md) remain before
switching gameplay. Existing rendered gameplay has no intended visual change.
