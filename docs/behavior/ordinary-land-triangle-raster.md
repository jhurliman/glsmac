# Ordinary known-land triangle raster and lighting

The formerly open vertex-to-span boundary is now closed for the bounded integer triangle domain in [the executable span-generator handoff](ordinary-land-triangle-spans.md):759 native spans match from vertices/UV/lights, including the401 original spans below. Camera/zoom and broader renderer limits remain as stated.

This closes the pixel sampler/shade operation and literal base-fan output for four native flat/hill/coast fixtures at zoom0. It extends `ordinary-terrain-projection.md`, whose height/light authorities remain unchanged. The adjacent cases contain16 native triangles,401 native scanline spans and8,967 actual pixel writes. An executable span replay reproduces every full indexed-frame hash from the user's stock TEXTURE.PCX; no asset bytes are bundled.

## Actual input and branch

The original ordinary terrain renderer queries cached heights and signed vertex lights, constructs the C-W-N/C-N-E/C-E-S/C-S-W fan, and passes three screen vertices, three signed lights, and three texture coordinates per triangle. These known-land calls select the full texture/shade branch. The alternate stippled/fog path was not used. This zero-moisture/rock/detail fixture selects a56×56 crop of TEXTURE.PCX at(1,58). Other terrain/rock/fungus/landmark layers and later shoreline composition are separate.

The four native UV triples are:

| Fan triangle | Texture coordinates in matching vertex order |
|---|---|
|C-W-N|(27,28),(0,55),(0,1)|
|C-N-E|(27,27),(0,0),(54,0)|
|C-E-S|(28,27),(55,1),(55,53)|
|C-S-W|(28,29),(55,55),(2,55)|

Do not map a single square uniformly across the whole diamond: the native fan has these four distinct UV triangles.

## Palette and shade operation

The native PCX load rebases this complete crop by+10 palette indices: the replayer verifies the rebased56×56 crop hash byte-for-byte before rasterization. Thus raw PCX indices are not yet the renderer texture indices. The pixel shader samples one loaded indexed texture byte, extracts the signed integer part of an interpolated8.8 shade phase, and adds it to that byte with8-bit wrap. Equivalently `(textureIndex + ((phase &65535)>>8)) &255`. This branch performs no per-pixel RGB/Lambert calculation or CLUT lookup. Palette-to-RGB conversion is downstream; the active palette hash is retained separately. Palette index addition is not RGB addition or a clamp to0..255. For example1 plus shade−2 gives255, and255 plus2 gives1 (byte-arithmetic diagnostic controls).

Native input shade values are the discrete cached vertex lights, not geometric normals. The flat fan's values are all0; the single hill carries0,1,1,−2,−2 at C/W/N/E/S. The tested coast fixture carries−3,0,0,−2,−2. Native arrays are clamped against the renderer's global shade limits before edge setup; these ordinary inputs are within those limits.

## Interpolation and coverage

This is affine scanline texture/shade interpolation with fixed-point stepping and nearest indexed sampling, not bilinear filtering. Native edge setup starts U/V at `(integerUV<<16)+0x8000` and computes edge deltas using signed truncating division. Shade edge state starts at `vertexLight<<16`. X edges use integer quotient/remainder stepping with direction-sensitive error correction. The renderer selects the minimum-y vertex, walks the two polygon chains, clips scanline spans and excludes the right endpoint. Horizontal derivatives are converted using x87 truncation toward zero to16.16 U/V and8.8 shade steps. Texture sampling takes the integer parts of U/V; the shade phase wraps as a16-bit word while stepping each pixel.

The original public pixel replay starts from captured native span states and independently checks every texture address, shade phase and final pixel. The separate [portable span generator](terrain_triangle_span_reference.py) now derives those same states from screen vertices, UVs and signed lights; it also matches36 additional native edge cases. Both preserve native integer/x87 behavior instead of substituting a floating-point barycentric rasterizer.

| Fixture | Pixel counts per triangle | Painted bounds, exclusive end | Full1024×511 indexed frame SHA256 |
|---|---|---|---|
|flat|[552, 552, 600, 600]|[528, 265, 624, 312]|e9c14c74703323787c0ce79ec9f499498fc9e472a25efd9f6bf10f5470609451|
|uniform4|[552, 552, 600, 600]|[528, 240, 624, 287]|7a2158362b5d2ff86c190dba652070be85725ba37c6fbadd3d15864f7c7e1a26|
|single_hill|[129, 111, 1041, 1023]|[528, 259, 624, 306]|c955f0cee6fb5d8747b89f710ff60bf45b30862606a0bb80cc62b89d62452e6f|
|coast|[0, 0, 1041, 1014]|[528, 264, 624, 306]|43cf037908c34d4de33f85ebda69dd8a2527a9eb75082ad5de118508b899b95f|

All spans execute native pixels before capture. There is no triangle overlap in these four base fans. Flat and uniform4 have identical cropped indexed output, shifted up25 pixels. The single hill retains2,304 pixels; the coast fan covers2,055. Do not enforce an idealized constant diamond coverage on a deformed fan. Background index250 is a diagnostic fixture, not a game background-color claim. Each case also provides a cropped hash and literal indexed samples.

## Reproduction and limits

Run `python terrain_triangle_raster_reference.py --texture-pcx PATH_TO_OWNED_TEXTURE.PCX`. It decodes the supplied stock PCX, checks the crop hash, replays401 native spans and verifies all four literal full-frame hashes and samples. The private probe re-executes the native geometry and shader to regenerate independent receipts.

Measured camera is(40,40), world80×80 round, viewport1024×511, tile(40,40), zoom0, no scale override, default relief. Only the selected dry ordinary land crop is covered. Other cameras/visible copies, zooms, subpixel translation and global scaling have not been pixel-validated here. The separate span-generator supplement closes vertex-to-span conversion and clip-edge behavior for its bounded integer triangle domain. Other polygon/shader variants and overflow domains remain explicit limits. This is not whole-scene screenshot parity. Broad RE stays paused.
