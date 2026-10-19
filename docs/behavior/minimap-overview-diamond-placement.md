# Native overview diamond placement

This closes source-bitmap placement for the standard round80×80 world, native overview zoom−14, bitmap320×152, first tile(1,1), spans40/39/40/39. These are actual native caller and polygon-raster receipts; target stretching is covered separately in `minimap-world-detail-camera.md`.

The original base pass submits40 tiles per row for y=1..79, totaling3,160, with no duplicate tile IDs. Row0 is not submitted. For y=1..78 use dx=x−1, adding80 when negative. Row79 uses the offscreen nearest-copy branch: dx=x−1, subtract80 if dx>40, add80 if dx<−40. Tile parity requires x/y low bits to match. The anchor is (4(dx−1),2(y−2)); the W/N/E/S polygon offsets from it are (0,2),(4,0),(8,2),(4,4). This is not the source placement x*4,y*2.

| Tile | Native anchor | Actual painted bounds, exclusive end | Pixels |
|---|---|---|---|
|(1,1)|(-4,-2)|[0,0,4,2)|6|
|(79,1)|(308,-2)|[308,0,316,2)|12|
|(0,2)|(312,0)|[312,1,319,4)|15|
|(2,2)|(0,0)|[0,1,8,4)|16|
|(40,40)|(152,76)|[152,77,160,80)|16|
|(40,76)|(152,148)|[152,149,160,151)|12|
|(0,76)|(312,148)|[312,149,319,151)|11|
|(39,77)|(148,150)|empty|0|
|(1,77)|(-4,150)|empty|0|
|(0,78)|(312,152)|empty|0|
|(79,79)|(-12,154)|empty|0|

Clip is [0,0,320,152). Preserve the native raster behavior: the row77 interior diamond has a geometric tip inside that rectangle but paints no pixels. A generic polygon intersection is insufficient evidence of raster equivalence. There is no second tile draw supplying a left seam copy in this pass.

The complete controlled band3/known world paints47,980 pixels with index22 and leaves660 background pixels (test index100). This background is a diagnostic fixture, not a claim that the real minimap uses index100. Full-source hash and13 selected native cases are saved in the adjacent cases file. Model bodies, target presentation, optional overlays and arbitrary map sizes are outside this supplement.
