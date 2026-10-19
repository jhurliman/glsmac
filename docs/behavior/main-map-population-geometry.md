# Native main-map population box geometry

The original zoom0 Base helper anchors its population box at the same dispatcher point as the untrimmed body canvas. Original controls in `base-body-variants-cases.json` use that shared point `(48,16)`, a 16-pixel box height, and measured `8`/`88` widths plus five pixels. Glyph raster parity remains separate.

The live population texture already used that layout, but its world sprite retained the older projection ratios. An actual camera/instance/mesh regression with a synthetic seven-pixel-wide box measured 7.304 by 10.240 pixels and a top edge 14.720 pixels below the native body anchor. It fails before this correction.

Admitted native population sprites now use the same 96x48 world-to-pixel convention as the body canvas. A top pivot compensates for Entity's mesh-Y inversion; the instance uses the shared body dispatcher anchor. Non-native population geometry retains its existing path. Owner colors, glyph generation, save state and simulation rules do not change.

Validation: cached build; five affected native groups; targeted correctness analysis of SlotBadges, Base and BaseManager. The actual sprite/camera regression covers one/two-digit textures, filled/outline style and heights0/25/6/3. It uses synthetic font metrics to isolate geometry, not to claim original glyph pixels. Existing original population-layout and Detail selector controls remain green. No full platform or sanitizer matrix is repeated for this geometry-only change. A populated original whole-viewport comparison remains open.
