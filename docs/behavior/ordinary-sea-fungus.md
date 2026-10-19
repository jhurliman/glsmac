# Ordinary sea Fungus continuation

The bounded continuation now matches **28 native producer contexts /157 ordered commands**, plus **31 complete indexed frames,4275 spans,135484 sampling attempts and93309 actual writes**. The portable producer starts from named band/contour/Fungus fields; the raster starts from vertices, UVs and loaded indexed crops. Captured commands/spans are expected outputs, not executable inputs to those calculations.

[Numeric cases](ordinary-sea-fungus-cases.json), [portable reference](sea_fungus_reference.py). The existing water/coast geometry, sampler, depth passes and generated masks are reused unchanged:

```
python docs/behavior/sea_fungus_reference.py
python docs/behavior/sea_fungus_reference.py --texture-pcx PATH/TO/TEXTURE.PCX --rainfall-pcx PATH/TO/RAINFALL.PCX
```

No original art or binary pixels are embedded in the public cases. Optional pixel verification requires the user's original assets.

## Admission and source selection

Fungus is item bit0x20. In this ordinary known-viewer profile, live/remembered center fields are initialized consistently; this pass does not change the existing remembered-item authority. The continuation requires the selected center item bit and retained center band>=2. Consequently, a sea center draws Fungus only at **shelf band2**. Native controls with Fungus set on band0/deep ocean or band1/ocean submit no Fungus quad; their ordinary water/depth draws still execute. A shelf control without the item bit also submits none.

The source additionally gates this continuation on zero style override and zoom>-10. Only the ordinary zero-override/zoom0 profile is pixel-closed here; no alternate-style/zoom admission survey is implied.

For each mask bit i, inspect native neighbor_offsets[(i-1)&7]. Set the bit only for a valid neighbor with **Fungus item bit AND band>=2**. Round-map X wraps; Y does not. Flat-map invalid X and invalid pole neighbors set no bit. Neighbor band1 Fungus is ignored. Neighbor land Fungus qualifies, so the mask can connect across land/sea. Both cross-shore directions are captured.

The original256-entry pattern lookup supplies crop index0..14 and quarter-turn0..3. The sea bank is a separate15-crop grid in **TEXTURE.PCX**:

| Index | Crop X,Y, width56,height56 |
| --- | --- |
|0..3|X508,565,622,679; Y516|
|4..7|X508,565,622,679; Y573|
|8..11|X508,565,622,679; Y630|
|12..14|X508,565,622; Y687|

All15 crop identities are reached by native producer contexts. Isolated shelf Fungus selects(508,516); NW-connected selects(565,516). Center land instead uses the established land bank starting(280,516), even when its connected neighbor is sea. No coordinate hash/address-seeded variation is used for this crop/turn selection.

## Geometry, UV and shader

The overlay is one quad ordered **W,N,E,S** from the original tile outline. Sea geometry is the sea-level diamond `[F+(-48,0),F+(0,-24),F+(48,0),F+(0,24)]`; contour depth and neighboring land do not lift it. The diagnostic land/coast case reuses the original land outline and land texture bank.

For vertex j, take the second vertex UV pair from canonical land-fan UV triple `(turn+j)&3`. The native quarter-turn controls give:

| Turn | Ordered W,N,E,S UV pairs |
| ---: | --- |
|0|(0,55),(0,0),(55,1),(55,55)|
|1|(0,0),(55,1),(55,55),(0,55)|
|2|(55,1),(55,55),(0,55),(0,0)|
|3|(55,55),(0,55),(0,0),(55,1)|

Do not regularize the one-pixel V offset. The known full-color wrapper selects the **keyed unlit** raster. There are no vertex-light/depth arguments. U/V edge starts are `UV<<16` with **no shaded half-texel bias**. Source indices are already rebased+10 modulo256 by native PCX loading. Compare loaded index with9 before writing:9 preserves the composed destination; any other index copies unchanged. No depth dimming, shade addition, alpha fade or extra key0 rule belongs to this Fungus shader. Its underlying water/depth can remain visible through holes.

## Composition and boundaries

For sea centers, finish all four ordered water sectors first, including each sector's depth-overlay-before-base-water behavior. Then draw the Fungus quad. The mixed deep-neighbor control verifies that the same unlit Fungus survives above varying underlying water/depth.

For the cross-shore land diagnostic, the actual order is **four existing land triangles → land Fungus quad → coast-mapped sectors**. Coast can therefore paint over Fungus. Moving the Fungus overlay after coast changes native composition. This case is a continuation-order control, not a new general land-material survey.

The native cases include all15 crops, all four UV turns, band0/1 admission negatives, absent item, below-shelf neighbor rejection, qualifying land neighbor, round west seam, flat west edge, north pole and a shelf with deep-water neighbors. Three extra pixel replays clip a rotated sea quad, a wrapped sea quad, and the land/Fungus/coast stack to[550,280,600,300]. Every generated span, source attempt, actual write and complete composed frame matches. Native key skips preserve already-painted pixels at clipping boundaries.

## Evidence boundary and next continuation

Same measured ordinary profile as the water/coast handoff: known viewer3, renderer flags1/full-color wrappers,80×80 world,1024×511 viewport,zoom0/default relief, no roads/rivers/landmarks/fog-style overrides or additional feature combinations. Full native renderer entry-to-return, source atlas/mask initialization, geometry/cache/projection, contour sampling, adjacency selection and ordered command production execute. Only final draw calls are sinks in the producer trace. A separate replay executes the full opaque, depth-threshold, coast-mapped and keyed-unlit native raster routines through actual stores. Supplied FILE/GDI backing and isolated indexed target/background are declared; no geometry, UV, mask-selection or pixel result is supplied. This is not a naturally played whole-Planet scene or a new cross-tile visibility proof.

The next unresolved ordered material continuation is the **river/color-map branch starting471a00 and reaching638e40 at471c57**. Roads, rivers, fog and landmarks were not investigated in this pass. Broader RE remains paused.

Frozen evidence: `tmp/reference/ordinary-sea-fungus-evidence-20261019`. Earlier water/coast and land archives remain unchanged.
