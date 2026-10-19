# Ordinary water and coast rendering

This finite handoff closes feature-free ordinary water/coast from the original renderer entry through its return. The independent producer matches **26 contexts /150 ordered draw commands**, every native contour-sampler result and both generated mask images. The executable vertex-to-pixel reference matches **3439 spans,83162 sampling attempts,70916 writes and29 complete1024×511 indexed-frame hashes**, including three clipped replays. Source assets and indexed buffers remain external/private.

[Numeric cases](ordinary-water-coast-cases.json), [portable producer/raster](water_coast_reference.py). Asset-free command checks and optional original-art checks:

```
python docs/behavior/water_coast_reference.py
python docs/behavior/water_coast_reference.py --texture-pcx PATH/TO/TEXTURE.PCX --rainfall-pcx PATH/TO/RAINFALL.PCX
```

## Ordered water commands

Sea centers have retained band<3 and stay on the height0 plane. Four sectors execute west,north,east,south. Each is a quad `[center,midpoint toward previous outer corner,outer corner,midpoint toward next outer corner]`. At flat center F, west geometry is `[F,F+(-24,12),F+(-48,0),F+(-24,-12)]`; rotate through the other sectors without normalizing winding. Fixed per-sector UV arrays are published in `tables.water_uv`; they differ from the coast UV arrays.

For each sector, call the contour sampler in modes0,1,2,3 and map each returned index through the original80-entry depth lookup. If **any depth>=16**, first submit the opaque depth-overlay quad from TEXTURE.PCX **(280,136,56,56)** with values `depth-16`. Then submit the base-water quad from **(280,79,56,56)** with the original depths and threshold16. Preserve this order independently in each sector.

The base-water shader returns immediately if all four input depths>=16. Otherwise it generates shaded spans and writes `(loadedTextureIndex+signedInterpolatedDepth)&255` only where interpolated depth<16. Equality16 skips the write. This is a depth threshold, **not key9 transparency**. The overlay uses the ordinary opaque shaded sampler, including negative values and byte wrapping; later shallow-water writes replace its shallow part. Water uses native shade limits[-256,+256], not the land[-4,+4] limit. Uniform contour10 gives depth35 and overlay19; contour30 gives depth16 and overlay0; contour50 gives depth5 with no overlay. The mixed gradient and map-edge controls exercise spans crossing16.

UV edge starts, x87 horizontal steps, half-open spans, first-index Y ties and clipping use the previously closed shaded polygon generator. The native pixel comparisons confirm it applies to these quads. Generated spans are computed from vertices/UVs/values in the executable; captured spans are expected outputs only.

## Exact contour sampler

Inputs are retained contour bytes, retained land/sea bands, doubled-X topology, sector0..3 and mode0..3. The reference translates the complete original sampler, not a fitted depth interpolation. Its significant rules are:

- Invalid center returns index0. Round-map X wraps once; Y never wraps. Flat-map invalid neighbors contribute0 to contour sums; denominators remain2 or4. An absent neighbor does not become a sea neighbor for shoreline overrides/masks. This is why shelf terrain at a pole/flat edge can acquire deep-water overlay values.
- Mode0 begins with the center contour. Modes1/3 average center and the relevant diagonal neighbor, after capping each contour at79; missing neighbor contributes0. Mode2 averages four incident centers using the published sector corner offsets, again with fixed denominator4 and each valid contour capped at79.
- Mode2 then adds7 when the sector's first diagonal contributor is land and subtracts3 when its other diagonal contributor is land. This deliberate asymmetry survives even a uniform contour input.
- Other modes inspect three sector neighbors. Opposite land/sea at the first neighbor in mode1 or third neighbor in mode3 replaces the calculated index with57 or67 according to sector and center land/sea state. All four literal override rows are expressed in the executable. Do not replace them with a symmetric sea-level interpolation.
- Normal calculated results clamp to0..79; lookup values then supply raster depth. Bands determine adjacency, while contours supply depth samples. Neither field substitutes for the other.

The26 fixtures distinguish deep ocean, ocean, shelf, four diagonal land neighbors, a cardinal neighbor, mixed land/deep neighbors, depth gradients, poles, flat edges and round seams. Every actual sampler result and its depth lookup matches the portable reference.

## Land-side coast continuation

Land centers keep their existing land fan/material passes. The tested dry/no-feature fixtures submit that fan first. Sea-neighbor membership constructs four3-bit coast patterns. For the west sector, the bits are SW=1,W=2,NW=4; rotate with the native neighbor ordering. Invalid neighbors set no bit. The cases cover every nonempty west pattern1..7, all four diagonal orientations, a raised band4 coast and a wrapped coast. Generic moisture/Rock/Forest/Fungus land selection remains the separately closed contract; these dry coast cases do not replace it.

Only sectors with a nonzero pattern sample four depths. After the existing feature continuations, append the coast quads in W,N,E,S order, using the deformed land geometry and midpoint floor shifts. Coast uses its own fixed `tables.coast_uv`, water crop(280,79,56,56), and land shade limits[-4,+4]. Raised-land geometry is executed natively and compared to the independent retained-band model.

The coast mask is **generated from RAINFALL.PCX**, not a directly sampled water texture. Native setup loads it with+10 index rebasing, converts loaded indices6/7 to key9, extracts the logical strip(2,333,113,56), and remaps it through a256-byte lookup built from53 executable source-index classes. It renders that strip on a zero-filled170×60 intermediate and extracts56×56 masks at(0,0) and(57,0). Both complete generated-mask hashes match reconstruction from the external PCX. The ordinary coast compositor here uses the first mask; the second is reconstructed/verified but has no new caller claim.

Each sector's pattern selects a53-entry mapping. The shader samples water and mask at the same generated source offset:

| Mapped mask value | Result |
| --- | --- |
|0|Preserve destination|
|1|Write loaded water index plus signed interpolated value, modulo256|
|>1|Write that literal palette index, with no shade addition|

There is no additional key9 test in this compositor. The public53×8 class table and twelve sampled color indices reconstruct the exact mapping. Pattern7 has a coordinate checker variation when `(x-y)&2` is nonzero: class5 and14..17 clear, and18..29 receive three repeated reversed color groups. The two checker controls verify this separately from the unrelated address-seeded land material variation.

## Supported boundary and next continuation

Closed profile: original SMAC, known viewer3, renderer flags1 with full-color variants, world80×80, viewport1024×511, zoom0/default relief, feature-free non-landmark retained fields and the bounded clipping controls. Native atlas construction, coast-mask production, height cache, projection, full per-tile renderer admission/selection, contour sampler, three full raster routines and actual pixel stores execute. Final raster calls are sinks only in the producer trace; the separate replay executes their full pixel code. Geometry/contour/adjacency results are not supplied. Indexed target/background and existing FILE/GDI services are supplied; frames are isolated tile batches, not a complete natural Planet scene or cross-tile visibility proof.

Other zooms/resolutions, fog/political/stipple variants, custom contours outside the measured domain, other land materials inside these coast fixtures, roads, rivers, landmarks and full-scene draw scheduling are not newly closed. A subsequent [sea-Fungus handoff](ordinary-sea-fungus.md) closes native47196c..4719fb on this same ordinary profile, with31 matching composed indexed frames. The deliberately excluded river compositor later at471c57 remains a separate shader/CLUT boundary; no river behavior was inferred from the coast mask. Broad RE remains paused.

Frozen evidence: `tmp/reference/ordinary-water-coast-evidence-20261019`; earlier land/palette/geometry archives remain unchanged.
