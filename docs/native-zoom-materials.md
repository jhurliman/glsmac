# Native terrain materials at nonzero zoom

The material renderer now uses the original quantized diamond unit for terrain
fans, coasts, rivers, landmarks, and their anchors, rather than fixed 48/24 pixel
half-diamonds. The plan retains the actual zoom separately: zoom3 and zoom4 both use
unit15 but have different 29/31 pixel height steps. Frame history retains both
values and rejects cross-zoom reuse even when bitmap dimensions and diamond
size happen to match. Improvement/resource selection receives the actual zoom
for its existing far-zoom gates.

## Original controls

The compact `behavior/native-zoom-material-cases.json` imports six original
controls from the verified frozen `native-zoom-stack-handoff-evidence-20261020`
archive, manifest SHA256
`47b8e09e8f6bffcef687c176b9fe17a00aac7015fae073d1bc271ef9673afea1`.
It retains asset hashes, caller inputs, complete pixel hashes and single-tile
paint-store counts. It does not contain executable, map/art bytes, captured draw
lists or vertex inputs to the production renderer.

The diagnostic decodes the user's owned assets using the same input preparation
as the existing viewport replay. `capture_native_viewport.py --prepare-only`
validates/decodes inputs without compiling or drawing a redundant zoom0 scene.
The production probe generates its own materials, geometry, shaders and UVs.

- Three complete mixed explored/unexplored viewports at zoom-1, first/east/back.
  The native sequence has no repeat between first and east. Inserting a warm
  repeat changes the cache history and is not the same original control.
- Three single-tile controls at zoom1/3/8, using the original supplied caller
  anchor(100,100), backdrop73 and measured clips. Geometry and material selection
  are computed from owned map fields; captured vertices are not supplied.

All six complete indexed buffers match exactly: 3,139,584 pixels. Single-tile
paint stores also match 3945/5441/4279. The six unknown zoomed bitmap frames from
the preceding renderer change remain exact across 2,536,992 pixels.

Production mixed-view output at zoom-1:

![Production nonzero-zoom terrain](https://raw.githubusercontent.com/jhurliman/glsmac/885ba582edc45cf4d93398358fb56f086c5c65ce/native-zoom-material/partial-zoom-minus1-glsmac.png)

[Original controlled-executable output](https://raw.githubusercontent.com/jhurliman/glsmac/885ba582edc45cf4d93398358fb56f086c5c65ce/native-zoom-material/partial-zoom-minus1-original.png)
has identical indexed bytes. Both use the original common palette at gamma100,
without image resizing. These are renderer controls, not normal gameplay
screenshots or a claim about faction/object overlays.

## Validation and remaining work

Cached incremental build, 90 affected native groups (89 existing plus the new
zoom/relief/history group), and targeted correctness analysis of
NativeTerrainRenderer, GameNativeTerrain and SmacTilePreview pass. The six
known-material controls pass ASan/UBSan. Existing unknown controls also pass.

```sh
python3 tools/reference/verify_native_zoom_material.py \
  --asset-directory '/path/to/Alpha Centauri' --out /tmp/native-zoom-material \
  --sanitize
```

Retained improvement/resource sprites now use the original nonzero-zoom
sampling, origin and shadow rules; see [native sprite scaling](native-sprite-scaling.md).
The material and retained-art paths use their distinct quantized/exact ratios.

The normal-game exploration bridge and camera/unit integration remain open.
This change does not disable zoom controls, reveal hidden terrain on zoom, or
switch the normal game over to an incomplete render path.
