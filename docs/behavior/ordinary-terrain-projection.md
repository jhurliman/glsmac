# Ordinary terrain projection, shared heights and lighting

Stock SMAC, 2026-10-19. **Ordinary zoom0 geometry is implementable from the retained altitude bands. Land height does not come from the saved contour byte.** This bounded contract supplies an independent [named-field reference](terrain_geometry_reference.py), [18 native contexts](ordinary-terrain-projection-cases.json), and a [private source/execution map](../re-smac-ordinary-terrain-projection.md). No production renderer or owned asset changed.

## Reused authorities and scope

Keep the [actual Planet loader](standard-planet-map-import.md), [climate rules](climate-recalculation-rules.md), and climate/landmark mutation contracts. Those already establish the80×80 doubled-X world,40 tiles per row,3200 retained44-byte records, horizontal wrap, distinct contour/band/climate fields, and their gameplay producers. This task does not rerun those climate grids. Existing [water base](map-water-and-idle-producer-boundary.md) and [depth-overlay](map-water-depth-overlay-boundary.md) receipts supply the texture/sampler identities; their animation/timer grids were not repeated.

New controls execute original cache initialization, view geometry, projection, height and lighting queries, slope-table generation, and the terrain-renderer entry through completion of its base geometry. The final raster calls are observed sinks. Sixteen land cases match the independent reference, including three raw installed Planet locations; two additional sea cases close the actual zoom0 caller/plane geometry. This is not whole-scene pixel parity.

The supported reference domain is valid, matching-parity tile centers on the80×80 world, zoom0, ordinary terrain without named landmark height/shade deformation, and ordinary generated slope codes within−3..3. The viewport controls use1024×768 overall and a supplied1024×511 map area above the257-pixel console. Native window backing dimensions, camera and visible knowledge are supplied; full window construction, scrolling input and first-session world generation are separate.

## Projection and triangle layout

At zoom0, with no global scale override, native geometry has these exact steps:

| Quantity | Pixels |
| --- | ---: |
| Whole diamond width / height | 96 / 48 |
| One doubled-X coordinate / one row | 48 / 24 |
| Center-to-west/east / north/south | 48 / 24 |
| Ordinary land elevation step | 25 |
| Supplied low-relief preference elevation step | 8 |

The engine derives the half-height unit as `trunc((trunc(50*(zoom+16)/16)+1)/4)`, clamped to at least1 in view setup. At zoom0 it is12. Width is8 times that unit; height4 times it. A global positive scale numerator/divisor can replace the zoom expression; it is zero/unused in these controls. Do not substitute the water source crop's56×56 dimensions for projected tile dimensions.

For this1024×511 viewport on an80×80 world, native first-coordinate derivation is:

1. Horizontal half-span `ceil(1024/96)=11`; vertical half-span `ceil((511+24)/48)=12`.
2. `firstY=clamp(cameraY-12,-2,60)`.
3. Start `firstX=cameraX-11`; wrap it once into0..79 for a round map, or clamp to−4..62 for a flat map.
4. Replace firstX's low bit with firstY's low bit to preserve doubled-X parity.

For an admitted in-view tile, let `dx=x-firstX`, adjusted once horizontally for the visible wrapped copy. The flat projected center is `(48*dx,24*(y-firstY))`; tested view offsets are zero. The reference implements this fixed viewport profile. General viewport resizing, tiny-map centering offsets and offscreen nearest-copy picking remain outside that helper.

Label vertices **C,W,N,E,S**. Before height displacement they are flat center plus `(0,0),(-48,0),(0,-24),(48,0),(0,24)`. Subtract each vertex's height from its own screen y. Native land submission is four ordered triangles:

`C-W-N`, `C-N-E`, `C-E-S`, `C-S-W`.

For camera40,40, native first coordinates are28,28 and tile40,40 has flat center576,288. Its flat-band3 triangles begin `[576,288;528,288;576,264]`. A uniform band4 moves every y upward25 pixels.

The coordinate helper's returned anchor is **not the center**: it is flat center minus `(48,24)` and minus `floor((hW+hN+hE+hS)/4)` in y. The terrain renderer adds back that mean, adds48/24, then applies the individual heights. Preserve this cancellation if sharing anchors with Unit/Base placement; do not subtract the mean twice.

## Land heights and seam rules

Ordinary center height is `heightStep*(altitudeBand-3)` for land bands3..7. Sea centers return0. At default zoom0 this gives0,25,50,75,100. The native cache stores signed-byte heights; special deformation/clamp cases are outside this ordinary formula.

Each outer vertex is shared by four tile centers. Relative to tile `(x,y)`, the contributors, in native order, are:

| Vertex | Four contributing tile-center offsets |
| --- | --- |
| W | (−2,0), (−1,−1), (0,0), (−1,1) |
| N | (−1,−1), (0,−2), (1,−1), (0,0) |
| E | (0,0), (1,−1), (2,0), (1,1) |
| S | (−1,1), (0,0), (1,1), (0,2) |

When all four are valid land, the vertex height is their center-height sum shifted right2: **floor of the average**, not rounding to nearest. **Any sea or invalid contributor forces the shared vertex to0.** It is not an average that includes a zero sea sample. Horizontal neighbors wrap on a round map; y never wraps. Flat-map horizontal edges and poles therefore use the same zero-height boundary rule.

Native cache fills publish a computed shared height into the corresponding corner slots of all valid neighboring tile cache records. A stateless renderer can compute the same shared vertex from the named fields. An implementation that caches must invalidate all affected shared vertices when contributing bands, landmark deformation, zoom or the relief preference change.

| Controlled tile | Heights C,W,N,E,S | Signed lighting C,W,N,E,S |
| --- | --- | --- |
| Uniform band3 | 0,0,0,0,0 | 0,0,0,0,0 |
| Uniform band4 | 25,25,25,25,25 | 0,0,0,0,0 |
| Band4 center surrounded by band3 | 25,6,6,6,6 | 0,1,1,−2,−2 |
| West band3 / east band4 slope | 25,6,18,25,18 | 1,1,1,0,1 |
| Band4 with one western sea contributor | 25,0,25,25,25 | 0,0,0,0,0 |
| Low-relief single hill | 8,2,2,2,2 | 0,1,1,−2,−2 |
| Raw Planet43,9 | 25,6,12,25,18 | 1,2,3,0,1 |

The round-map seam controls prove tile0,40's W and tile78,40's E have the same height6 and signed light−2. Flat-map x0 and pole y0 controls instead demonstrate clipping. Changing uniform band4's contour from0 to125 leaves geometry and lighting identical.

## Lighting inputs are discrete slope values

The original submits a **signed scalar per vertex** to terrain rasterization. It does not pass a conventional per-vertex3D normal. Its lookup authority uses retained bands and shared-corner band totals, independently of the25/8-pixel relief choice. Recomputing Lambert normals from the displayed mesh changes behavior: the shoreline control has a steep geometric side but all-zero signed light, and the low-relief hill preserves its ordinary hill's light values.

For a land tile/corner with four valid land contributors, the corner slope code is `(sum(contributor bands)-4*ownBand+4)&15`; a boundary/sea contributor gives code4. Adjacent codes, each minus4, select an entry in one of four7×7 face tables. The generated ordinary domain is−3..3 for each selector. The reference rejects selectors outside that range rather than inventing values for corrupt or unsupported jumps.

For selectors `(a,b)`, the four face orientations use `(a,b),(-b,a),(-a,-b),(b,-a)`. Multiply the oriented pair by100; calculate `L=isqrt(A*A+B*B)`. The table value is signed truncation toward zero of `-3*(A+B)/L`, or0 when L=0. **All196 native generated entries match this formula.** Keep integer square root and truncating division distinct from the later floor shifts.

The tile center's signed light is the sum of its four face values shifted right2. A shared outer vertex sums the two incident face values from each of its four contributing tiles, then shifts right3. Sea/invalid face contributions are0. The reference implements the exact face-pair ordering. Native raster submission carries the resulting three scalar values alongside each triangle. Their interpolation, palette lookup and pixel coverage belong to the downstream raster/art contract; do not interpret these integers as literal RGB additions.

## Sea geometry and contour boundary

Sea geometry stays on height0. Native water submits four quadrilaterals, covering west, north, east and south sectors. The reference's `water_quads` supplies the exact zoom0 coordinates using the outer diamond and its edge midpoints `(±24,±12)`; it does not deform the sea plane using contour depth.

The complete original contour sampler is called16 times: four sample modes for each of four sectors. Its results index a retained depth lookup, producing a separate per-vertex raster input. In the new uniform controls, contour20 gives depth25 and contour37 gives11, but both submit identical sea-plane vertices. Depth≥16 also admits the prior fixed-texture overlay with per-vertex input depth−16; contour20 produces four overlay submissions plus four base submissions. This reuses the prior depth consumer contract, now reached from the real renderer entry at zoom0 rather than a supplied-scale fragment.

The full coastal contour-sampler algorithm and depth/palette raster have not been translated into this public geometry reference. They affect water appearance, not the closed planar vertex positions.

## Integration boundary and remaining dependencies

The port can replace its contour-to-land-elevation adapter with retained band centers, these shared-corner rules, native diamond/fan coordinates and the discrete lighting scalars. Preserve saved contour for the separate water-depth/climate consumers. Keep moisture, climate code and rockiness available to art selection; they are not substitutes for these ordinary heights.

Still required for the complete gameplay-map reference target:

- Named landmark height and light deformation tables and their production/invalidation. The native height/shade routines have explicit landmark branches; the reference excludes them. Raw Planet controls avoid those neighborhoods.
- Terrain texture/UV selection, rocky/fungus/landmark overlays, shore masks and their composition. One raw Planet land control already submits a detail overlay over the same four triangles; that is art layering, not eight different geometry faces.
- Full water-depth sampler semantics, interpolation, terrain palette/CLUT behavior and native triangle coverage. Current receipts stop at raster arguments and contain no new pixel-parity claim.
- Full viewport/camera UI integration, other zooms/global scale overrides, cross-tile draw ordering/occlusion, and matched natural first-session rendering after the existing world-preparation schedule.

Frozen evidence: `tmp/reference/ordinary-terrain-projection-evidence-20261019`. Owned Planet/PCX bytes and raster references remain outside Git. The independent reference and compact semantic cases contain no owned asset payload. Broad RE stays paused.
