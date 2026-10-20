# Native retained-sprite scaling

Retained terrain sprites now use the original software skip/repeat sampling,
whole-expression origin truncation, key handling and repeated-block shadows.
The shared indexed batch supports keyed copies, shadow copies, and solid masks.
Terrain fans retain their quantized diamond unit; sprite ratios use
`(zoom+16)/16`. The former nonzero-zoom sprite rejection is removed.

## Production behavior

`types/texture/SmacIndexedSprite.h` prepares an owned sampling plan from retained
source dimensions/pitch/offset and a positive ratio. It retains no image pointers
or callbacks. Painting borrows source/destination/shadow data only synchronously.
The batch copies its destination once, then preserves command and store ordering.

- Round the ratio to cached IEEE binary32, then truncate complete
  `anchor + offset*ratio` expressions toward zero. Integer rational rounding and
  fixed-point products preserve this on MSVC and Clang without depending on host
  FP mode or extended `long double` availability.
- Restart the native 800-entry phase table for each sprite. Shrinking skips
  source positions; enlargement repeats source pixels before cropping to the
  separately truncated nominal extent. Clipping preserves the original phase.
- Test the source key before replacement/shadow processing. A literal output9
  still counts as an opaque store.
- Enlarged source0 shadow blocks read their first clipped destination once and
  fill the block with that lookup result. Downscaled/identity shadows use each
  sampled destination. Preserve source-row/source-pixel/repeated-row order.
- Preserve the measured solid-mask identity top-clip quirk for padded sources.
  Ordinary retained terrain art is compact, so it has no width/pitch mismatch.

The interface admits source dimensions within the native step-table budget,
16-bit positive ratios and bounded signed coordinates. A cached scale greater
than 4 draws nothing, as in the original; zero extents also emit no stores.
Arbitrary native minimum-size globals and hardware/DIB rendering are outside
this software profile. Stock terrain ratios and the recorded overrides fit it.

`NativeTerrainRenderer` supplies each trimmed sprite's original offset and raw
pixels, the actual projected draw anchor, and the unquantized ratio. It does not
pre-scale a full 100x62 crop, trim after scaling, or use q/12. Existing improvement
and resource selection gates remain in force.

## Original evidence and validation

The frozen 214-file `native-zoom-known-render-evidence-20261020` archive was
verified against manifest SHA256
`f674cb59801d950f3b34714275876f11a9344cfea2468e4043fb8cb17dcb215c`.
Its three independent verifiers pass 105 synthetic controls, seven retained-art
scenes, four actor-scale controls and 57 pinned guards. The imported receipts
contain synthetic pixels and semantic hashes; purchased art/map/executable
bytes remain external. Actor/camera integration is not part of this change.

- Production kernel: 105 original final-frame hashes and all 19,114 ordered stores
  match. Native CI uses synthetic fixtures with full-frame and ordered-store
  receipts; standalone verification additionally checks SHA256.
- Production material selection plus sprite scaling: all seven complete original
  tile buffers match, totaling 3,662,848 indexed pixels and 102,833 paint stores.
  These controls include 69 retained calls, 20 shadow calls, clipped zoom4/8 views,
  improvements, resources, pods and monoliths. Captured draws/UVs are not injected.
- Both replay programs pass ASan/UBSan. The six preceding zoomed-material controls
  remain exact across 3,139,584 pixels.
- Cached native build; 91 affected native groups plus the dedicated shared-batch
  binding test pass. The tests include key-before-solid replacement, opaque9,
  custom source keys and enlarged shadow destination latching.
- Four production translation units pass targeted correctness analysis:
  SmacTerrainRaster, NativeTerrainRenderer, SmacTilePreview and GameNativeTerrain.

```sh
python3 tools/reference/verify_indexed_sprite_scale.py \
  --out /tmp/native-sprite-synthetic --sanitize
python3 tools/reference/verify_native_retained_sprite_scale.py \
  --asset-directory '/path/to/Alpha Centauri' \
  --out /tmp/native-retained-sprites --sanitize
```

Controlled zoom3 tile crop, production output (identical to the original):

![Production retained sprite control](https://raw.githubusercontent.com/jhurliman/glsmac/c11879c0646ba45c0897d30f7540c32b9cb5161d/native-sprite-scale/zoom3-retained-glsmac.png)

[Original crop](https://raw.githubusercontent.com/jhurliman/glsmac/c11879c0646ba45c0897d30f7540c32b9cb5161d/native-sprite-scale/zoom3-retained-original.png)
uses the same bounds [84,76,236,210], original gamma100 palette, and no resizing.
The scene supplies item/memory contexts deliberately; it is a controlled renderer
comparison, not an ordinary gameplay screenshot.

The ordinary saved-exploration bridge still needs live display policy, native
camera zoom and unit/Base placement integration. Full interactive rendering,
audio/vsync performance and gameplay completeness remain separate release gates.
