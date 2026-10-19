# Lower-console terrain preview

The original console paints a fresh terrain tile with **no renderer view**. It does not reuse or resize the main viewport image. For the measured console rectangle, it emits an 80 by 40 flat diamond at 85/100 scale, including for sloped land. All land vertex light values are zero. The numeric controls are in `lower-console-terrain-preview-cases.json`.

The original caller passes bitmap, renderer view 0, zoom 0, viewer 3, auxiliary 0, flags `0x621`, tile `(8,8)`, anchor `(148,67)`. The half-open clip is `[146,66,235,183)`. Its scale numerator is the preview rectangle width minus four, denominator 100. After the call, the console sets both override globals to -1.

The native quarter-height is `trunc((trunc(50*numerator/denominator)+1)/4)`: at 85/100 this is 10. Half-width is four times that value, 40. Thus the four triangle vertex arrays are:

- `(188,87), (148,87), (188,67)`
- `(188,87), (188,67), (228,87)`
- `(188,87), (228,87), (188,107)`
- `(188,87), (188,107), (148,87)`

They paint 380, 380, 420, and 420 pixels in that order. Geometric extrema are `[148,67,228,107]`; actual painted half-open bounds are **`[148,68,228,107)`**. The top geometric point emits no pixel. The call performs no clear: the supplied backing survives outside stores, including the large area below the diamond.

A null renderer view bypasses the corner-height, average-height, and face-lighting queries. No 85% height-cache scaling or face-light rounding applies to these land controls. Flat and sloped fixtures produce identical full indexed frames and ordered stores. The sloped fixture changes the center to band 4/contour 87 and diagonal neighbors to bands/contours 3/63, 5/103, 4/89, 3/73; neither geometric deformation nor slope lighting appears. The independently allocated native geometry cache remains byte-identical in every case.

Features use their ordinary painters at this smaller geometry. Road follows the four base triangles as one keyed shaded quad, with all light values zero. Fungus follows as one keyed unlit quad. The resource control follows as a scaled sprite with the existing shadow lookup. The numeric receipt includes exact source crops, UV arrays, draw order, shader limits, per-draw store counts and hashes, sprite trim and scale. Source color key is 9. Resource placement and sampling use the native rational sprite scaler, not a resized composite image.

Viewer 3 without the all-map revelation flag selects the remembered item word at tile offset 24. Remembered fungus with live items zero produces the same frame as matching live/remembered fungus. Live-only fungus with remembered items zero produces the bare frame. Resource queries retain their existing live-state authority; this control has matching live and remembered resource bits.

Seven controls independently reproduce **13,137 ordered native framebuffer byte stores** and all seven complete indexed frame hashes, including the resource sprite and shadow composition. Polygon spans are regenerated from vertices/UV/light; they are not copied from native span observations. A separate stream replay checks private CPU-write receipts and clip containment. The native atlas audit compares 217 texture, sprite, and coast-mask bindings to independently decoded owned assets.

These are controlled synthetic tile fixtures using the pinned GOG executable, native atlas preparation, native terrain/material/feature logic, and actual x86 raster stores. Destination backing is an explicitly supplied console-local 1024 by 257 indexed bitmap with pitch 1024 and initial index 73. Its bitmap header uses the initialized native bitmap layout. The map allocator low 16 bits are `0xbf60`; variation must preserve that declared input when comparing exact source crops. This is not a naturally played console screenshot or a complete lower-console repaint. Sea/coast controls remain separate follow-up work. Owned pixels remain under ignored `tmp/reference/console-terrain-pixels-private-20261019`.
