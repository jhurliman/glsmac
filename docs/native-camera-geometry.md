# Native camera dimensions and zoom geometry

The ordinary native view scheduler previously assumed an 80x80 wrapped Planet
and a 1024x511 map viewport. These assumptions prevent using its original-backed
unknown-grid path for other ordinary map configurations. Layout and view geometry
now retain world dimensions, viewport dimensions, small-world centering offsets,
and topology. Terrain selection uses the actual record stride; owned exploration
masks must have exactly one entry per cell. Unknown traversal does not query face
shading. Projectors retain the original distinction between wrapped, flat, and
out-of-view displacement.

## Original evidence

[The captured cases](behavior/native-camera-geometry-cases.json) reproduce 25
original-executable geometry controls: zoom -14 through 16 (including odd zooms),
321x177/641x359/1281x503 viewports, wrapped and flat edges, 64x32/128x64 worlds,
16x12 worlds smaller than the viewport, and low relief. The 154 compared receipts
cover layout fields, projected anchors, height returns, and two high-zoom
cold/warm center-height edges. They do not prove every intervening zoom or map.

Cases derive from the frozen `native-zoom-stack-handoff-evidence-20261020`
handoff, manifest SHA256
`47b8e09e8f6bffcef687c176b9fe17a00aac7015fae073d1bc271ef9673afea1`.
The manifest and all 238 entries were verified before import. The fixture contains
semantic measurements, not executable or purchased art/map bytes. Captured
executable SHA256 is retained in the fixture.

Original function 47ca10/47cc48..56 stores a capped signed-byte center height but
returns the uncapped result on a cold query. Uniform band7/profile0 gives 148
then 127 at zoom8; 200 then 127 at zoom16. The production cache now preserves
that distinction. Corner sharing still uses the capped byte. This does not
establish the cause of the user's apparent excessive relief in ordinary play.

## Validation

- Cached incremental build and 57 affected native test groups pass, including
  small-flat-world unknown traversal, mask dimensions, and material admission.
- `python3 tools/reference/verify_native_camera_geometry.py --sanitize`: all 25
  controls and 154 receipts pass with ASan/UBSan. Also checks generic unknown
  traversal bounds and unsigned record offsets beyond signed-int range.
- Existing original controls remain exact: 2,376 landmark contexts / 15,280
  height/shade queries; 6,400 Planet projectors / 32,000 heights / 32,000 shades /
  508 primary triangles; 30,772 lifecycle returns; 15,964 production-scheduled
  returns; 2,780 ordered selections and both measured pan regions.
- Existing production unknown-view comparison passes 25,112 queries, 5,344
  outlines, 230,400 cache bytes, and 2,093,056 complete indexed pixels.
- Six production translation units pass targeted correctness analysis:
  NativeTerrainRenderer, GameNativeTerrain, Map, MapNativeTerrain,
  MapGlobalSpread, and Finalize. The two view consumers were rerun after the
  unsigned record-offset correction.

## Remaining integration

The material renderer and retained World remain Planet/1024x511/zoom0-specific.
`RenderPrepared` explicitly rejects incompatible geometry rather than drawing it
with fixed diamonds and strides. Original-matched geometry does not establish
textured rendering at other zooms, flat-map edge lighting, or general incremental
damage. The measured small horizontal-pan path remains bounded; other camera
changes require full redraws.

The ordinary main map still needs the saved-exploration presentation bridge.
No normal-game fog fix, color/coastline parity, or full zoom support is claimed.
Next work must generalize the bitmap/world/material path, preserve original
sprite scaling separately from quantized diamond size, then connect knowledge to
ordinary drawing without revealing unsupported hidden terrain.
