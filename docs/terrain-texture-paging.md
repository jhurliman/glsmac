# Terrain atlas GPU paging

The standard 80 by 80 Planet map produces a 4,536 by 17,920 terrain atlas.
Uploading it as one 2D texture fails on a driver whose maximum texture dimension
is 16,384. The private original-map rendering capture reproduced this with
`GL_INVALID_VALUE` on Mesa 25.0.5 / llvmpipe through WSLg.

Oversized, non-mipmapped textures now use a 2D texture array. The logical CPU
bitmap, normalized mesh UVs, save format and dirty rectangles remain unchanged.
The standard Planet atlas uses two 4,536 by 8,960 layers on that driver; no map
tiles or atlas pixels are downsampled. Page layout also supports multiple
columns and checks the driver's array-layer limit before allocation.

Uploads use the original CPU row stride and split dirty rectangles at page
boundaries. The orthographic shader samples logical pixels across layers,
including repeat wrapping and bilinear interpolation across page seams.
Ordinary textures retain the existing 2D sampling path. Oversized mipmapped
textures are rejected explicitly pending a separate paged-mipmap implementation.

Sampler units are initialized before program validation because 2D and array
samplers cannot share the default unit. The debug GL watcher tracks array
bindings per texture unit, counts allocation/update ownership, and permits
linked-program uniform initialization while still requiring validation before
drawing.

## Validation

- A native regression covers the actual Planet atlas dimensions, larger atlas
  layouts, invalid dimensions/layer limits, and dirty rectangles crossing both
  page axes without duplicating or omitting logical pixels.
- Five focused paging, Former terrain-render and font groups pass in the cached
  Linux debug build with memory debugging enabled.
- LLVM 15 correctness analysis passes eight affected production translation
  units, including the watcher, shader programs and drawing consumers.
- Actual full-Planet rendering passes shader initialization and atlas upload and
  produces a terrain framebuffer. A subsequent minimap-stage crash is under
  investigation; this is not yet a completed seven-faction visual smoke test.

Paging fixes GPU storage limits. It does not establish original-game terrain
geometry, contour shading or map-import fidelity. Those require matched
original-executable captures; the private map adapter is only a rendering probe.
