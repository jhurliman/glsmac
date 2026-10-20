# Production Unity Survey terrain renderer

The native view renderer now implements the measured Unity Survey material and
raster profile. Both all-unknown and supplied-partial21 sequences match every
original first/repeat/east/back indexed frame, all outer query returns/order and
semantic cache bytes. The ordinary game caller is still inspector-only; this is
not a claim that live main-map exploration has been integrated.

## Policy and raster

`ViewGeometry::Prepare` accepts explicit observer knowledge and optional global
survey lighting. Survey without a knowledge mask is rejected. Unknown survey
land retains its original height queries but uses the supplied light, without
calling face shade. Known land still calls its face shader. Sea retains the
ordinary depth/geometry path. The plan owns the resulting policy and geometry.

Survey material zeroes center moisture/rockiness, clears center Fungus and skips
Forest/landmark art and late improvements/resources. Geography, live neighbors,
coasts and rivers remain. Remembered Farm can still replace the base texture.
The renderer represents the original render-metadata bit as its owned survey
classification; it does not mutate saved exploration or raw backend landmarks.

`Command::alternate_rows` binds the measured0x41 shader wrappers. Absolute even-Y
rows fill65; odd rows texture normally, with the opaque wrapper's one-pixel lag
restarting at the first visible clipped texel. Key and coast-mask transparency
still apply on black rows. Alternate water retains its whole-polygon depth
rejection but does not apply the odd-row per-pixel threshold on black rows.
Unmeasured alternate shader bindings are rejected. Both coast modes preserve
uncapped depth inputs; see the [source correction](behavior/survey-coast-clamp-and-clipped-opaque.md).

Retained horizontal painting and opaque backing use the existing original
copy/fill policy. Callers remain responsible for invalidating frame history on
world, viewer, knowledge, display-policy or rules changes. Current observation's
separate alternate-display option, editor translation, object disclosure, other
map/camera profiles and ordinary-game presentation remain open.

## Evidence and validation

- Frozen205-file survey archive verified against manifest SHA256
  `d2c20ab2e339041cdaacb0f6c3016b0bdea19287a34bb5b3b3244be685b0ebfb`.
- Frozen146-file primitive discriminator archive verified against manifest
  `800eceeb10e64b4cf878f8f1ee6c74f44c91b91e9370ca1c759ee8fedc80e3f0`.
- Eight production standalone controls pass ASan/UBSan:4,186,112 complete indexed
  pixels,20,504 outer queries/returns and230,400 semantic cache bytes. Only owned
  raw map/art and supplied exploration enter the renderer. Captured commands are
  expectations, not material-producer inputs.
- Four revealed and four non-survey partial21 frames still match their complete
  indexed hashes under ASan/UBSan; revealed paint-store counts also match.
- Cached native build, ten affected native groups and four production translation
  units' correctness analysis pass. A focused four-TU ASan/UBSan native run passes
  the same groups; other objects are uninstrumented and leak detection is off.
- Asset-free tests cover clipped UV lag, opaque backing, key/mask transparency,
  coast depth, water admission, global survey light and known-only face shading.

Full native command/disassembly records remain in the verified private archives.
The small [production receipt](behavior/unity-survey-production-cases.json) retains
frame/query/cache hashes needed to reproduce the actual renderer comparison.

```sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/reference/capture_native_viewport.py \
  --asset-directory '/path/to/Alpha Centauri' \
  --output-directory /tmp/glsmac-survey \
  --variation-low16 48992 --initial-index 65 --viewer 3 \
  --exploration-profile survey-partial21 --sanitize
```

Use `survey-unknown` for the other four controls. The image below is actual
production indexed output resolved through the verified original palette;
it is not an ordinary-game screenshot.

![Production Unity Survey control with partial exploration](https://raw.githubusercontent.com/jhurliman/glsmac/07d2cab7296cc38c2954b6d0d8a557127f0dafc3/unity-survey-view/partial21.png)
