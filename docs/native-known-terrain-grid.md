# Native known-terrain grid

The native terrain renderer now consumes explicit raw Preferences and More
Preferences for the original known-grid continuation. Ordinary admission uses
Preferences bit `0x100`, not view flag bit 2. Special mode suppresses ordinary
admission; explicit renderer bit `0x04000000` forces the continuation. The
More Preferences sea/coast bit is `0x800000`.

Each admitted tile draws W-to-N, then N-to-E from its existing displaced
ordinary vertices, without new geometry queries. Invalid northern neighbors
clip; wrapping X preserves the original seam behavior. Land/land uses palette
index 37; admitted sea/coast uses 190. The separate unknown outline remains
index 194 and does not consume the known-grid preference.

The grid is in the per-tile feature continuation before Forest/Fungus,
territory/coast, River, resources and developed artwork. Painting it after
Forest/Fungus and River initially disagreed with the executable; the complete
frame comparison caught that mistake. The final ordering matches all supplied
indexed frames. Retained frames also carry raw draw flags and grid preferences;
changing them rejects incompatible bitmap history instead of retaining an old
interior. A caller must request a complete repaint for those changes.

Evidence is the frozen original-executable supplement with manifest SHA256
`9c36cbf7f85151152217cd54226497d09f2cc9f068684134b787cf002b6c317f`;
all 128 listed files were verified before use. Public numeric inputs and hashes
are in [the original cases](behavior/known-grid-and-console-fog-cases.json).
The native fixture covers 92 full-scene line arguments/source quads plus 18
admitted continuation contexts, totaling 70 cases and 118 expected lines.
Earlier unknown admission and knowledge tests remain separate.

Validation: cached incremental build, 26 selected native groups, correctness
analysis of three production consumers. The production renderer matches all
13 original 1024x511 indexed frames, totaling 6,802,432 pixels: grid off/on,
view bit 2 clear, repeated paint, east/back retained pans and fully unknown.
The same 13 comparisons pass with ASan/UBSan across all 12 standalone probe
and renderer translation units; leak detection is disabled.

Reproduce with purchased matching assets:

```sh
python3 tools/reference/verify_native_known_grid.py \
  --asset-directory '/path/to/Alpha Centauri' \
  --output-directory /tmp/glsmac-known-grid
```

Add `--sanitize` for the instrumented standalone renderer. Asset identities,
source-backed allocation-base variation and authored observer-memory inputs
are checked. Original executable and asset pixels stay external.

This controlled image uses 21 supplied explored tiles, viewer 3, empty rosters,
zoom 0 and relief 25. It is a renderer comparison, not a normal-game screenshot
or natural startup. Sea/coast preference and special-mode line admission are
bounded argument controls; they do not establish additional full pixel scenes.
The ordinary gameplay camera/picking, observer policy and complete layer
connection remain open, as do the reported normal-view color/coast/relief issues.

![GLSMAC controlled indexed viewport](https://raw.githubusercontent.com/jhurliman/glsmac/bcd8ad0db74a7d8368ffb9868a38782eb2a8ebfc/native-known-grid-media/glsmac.png)
