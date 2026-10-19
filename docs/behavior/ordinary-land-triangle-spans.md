# Ordinary land triangle vertex-to-span generator

The explicit vertex-to-span boundary in `ordinary-land-triangle-raster.md` is now closed for the ordinary integer triangle domain and the controls below. [Portable reference](terrain_triangle_span_reference.py), [native edge cases](ordinary-land-triangle-span-cases.json). The generator accepts screen vertices, UVs and signed lights; it never takes captured spans as input.

Exact equality passes for all401 prior native spans/16 triangles, then358 new native spans/36 triangles:759 spans total. Feeding generated spans into the indexed texture/shade replay reproduces all four original full-frame hashes and8,967 pixels. The edge cases require no original art; native execution uses a supplied constant texture and canvas.

## Exact rules

Keep the input cyclic vertex order. Select the **first** minimum-Y vertex and first maximum-Y vertex using strict comparisons; do not sort all vertices or normalize winding. The left chain walks indices−1 and the right chain+1, wrapping modulo3. Horizontal edges are skipped. A chain with a negative rise fails; reaching the selected maximum ends that chain. All-equal-Y triangles return empty.

Each nonhorizontal edge keeps integer X, signed truncating quotient `q=trunc(dx/dy)`, remainder `r=abs(dx)%dy`, and correction sign. Error starts at0 for nonnegative dx, `1−dy` for negative dx. Each row adds q and r; only if error>0 add the correction sign and subtract dy. This direction-sensitive remainder rule must survive translation; Python floor division or a generic rounded DDA changes pixels.

Edge U/V start at `(vertexUV<<16)+0x8000`. Their steps are signed truncating division of `(nextUV<<16)−start` by dy; the half-texel bias is inside that subtraction. Signed light starts at `clampedLight<<16`, with its edge step computed the same way without the half-texel bias. Edge states are advanced once per row, including rows clipped above the viewport. Scanline data is captured from the old states before that advance. Never restart interpolation at the clip top.

Positive-width spans are left-inclusive/right-exclusive; Y runs minimum-inclusive/maximum-exclusive. Reject nonpositive widths rather than flipping their endpoints. Clip X/Y against the supplied bitmap rectangle. Left clipping advances U/V by skippedPixels×horizontalStep and shade phase by skippedPixels×shadeStep. Right clipping shortens coverage without changing interpolation. Empty clip rectangles and fully excluded triangles emit no spans.

Horizontal derivatives preserve the original x87 sequence: round `1/(width<<16)` toward zero to a64-bit significand; multiply the signed difference of the two edge states and round again; multiply by65536 for U/V or256 for shade; then convert toward zero. The reference implements this using integer/rational arithmetic with explicit64-bit significand truncation, independent of the host floating-point mode. Exact rational division alone is insufficient: an observed derivative with delta926882,width11 is84261, while exact integer division gives84262. The comparison set reaches167 such derivative discrepancies across generated rows.

Shade derivatives convert through signed16-bit FISTP. A masked-invalid overflow stores0x8000; it is not ordinary modulo65536 conversion. A sharp synthetic light control reaches this branch after native input clamping to[−256,256]. Ordinary terrain cases also pass unchanged. U/V derivative FISTP32 overflow, nonfinite input and signed32 intermediate overflow remain outside the supported domain.

## Fresh native controls

| Case | Native spans | Native pixels |
|---|---:|---:|
|general_012|22|231|
|general_021|0|0|
|general_102|0|0|
|general_120|22|231|
|general_201|22|231|
|general_210|0|0|
|top_tie_012|21|231|
|top_tie_021|0|0|
|top_tie_102|0|0|
|top_tie_120|21|231|
|top_tie_201|21|231|
|top_tie_210|0|0|
|bottom_tie_012|21|220|
|bottom_tie_021|0|0|
|bottom_tie_102|0|0|
|bottom_tie_120|21|220|
|bottom_tie_201|21|220|
|bottom_tie_210|0|0|
|horizontal|0|0|
|vertical|0|0|
|diagonal|0|0|
|repeated|0|0|
|identical|0|0|
|clip_left|16|121|
|clip_right|22|168|
|clip_top|14|184|
|clip_bottom|11|84|
|clip_box|10|90|
|clip_outside|0|0|
|clip_empty_x|0|0|
|clip_empty_y|0|0|
|negative_left|17|137|
|negative_top|20|263|
|beyond_bottom|16|117|
|beyond_right|24|238|
|shade_clamp_and_word_overflow|16|16|

The three shape families test all six permutations with UV/light attributes permuted together. Cyclic rotations preserve output; opposite winding emits no pixels. Tied tops/bottoms preserve first-index selection and horizontal-edge behavior. Horizontal/vertical/diagonal collinearity, repeated vertices and identical vertices emit no pixels in these controls. Clip tests include every edge, an inner box, full exclusion, empty width/height, negative screen coordinates and vertices beyond the canvas. Literal span states are retained for every case.

## Run and integration boundary

`python terrain_triangle_span_reference.py` checks the401 original spans and all36 edge cases using only public numeric fixtures. Optionally pass `--texture-pcx PATH_TO_OWNED_TEXTURE.PCX` to verify the four original indexed-frame hashes with freshly generated spans. No asset or binary bytes are included.

This is a reference generator for integer triangles in the bounded domain stated in the cases, with valid ordinary32-bit intermediates. Original ordinary UVs and synthetic positive UVs are validated; negative UV texture addressing, enormous coordinates/overflow, arbitrary concave polygons/quads and other renderer variants are unproved. It generates raster spans from already projected screen vertices; it establishes no additional camera navigation, zoom, terrain-layer selection or whole-scene parity. Existing ordinary zoom0 geometry, UV and palette/shade contracts remain the input authorities. Broad RE stays paused.
