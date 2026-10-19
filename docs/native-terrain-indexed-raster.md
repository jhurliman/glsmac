# Ordinary indexed terrain raster

`game::frontend::smac_terrain_raster` implements the bounded ordinary triangle
contract in [the native reference](behavior/ordinary-land-triangle-spans.md).
It is not yet connected to the live terrain painter. Texture selection,
cached signed vertex lighting, loaded index rebasing, palette presentation,
layer order and view invalidation must feed this primitive before it replaces
the current RGB terrain mesh rendering.

`GenerateSpans` accepts integer screen vertices, integer UVs, signed vertex
lights and an exclusive clip rectangle. It preserves input winding and the
first minimum/maximum-Y index, walks both native edge chains, and retains the
direction-sensitive quotient/remainder rule. UV interpolation includes the
half-texel bias in the edge subtraction. Horizontal interpolation explicitly
chops reciprocal and product to a 64-bit significand before truncation;
unsigned two-word multiplication avoids host extended floats, compiler-specific
128-bit integers and changes to the caller's floating-point controls. Shade
overflow stores the native signed-word indefinite value.

`Draw` copies the caller's indexed destination once for an ordered triangle
batch, clips against that bitmap, samples the supplied loaded indexed texture,
and adds the shade integer with byte wrap. It returns owned pixels and the
number of writes. No GC, engine, graphics or asset pointers escape. Invalid
bitmaps, supported-domain violations and out-of-range sampled addresses throw;
an invalid later triangle cannot expose partial output or modify the caller.
Empty or reversed clips write nothing. This API does not convert to RGB.

The supported input domain is the reference's bounded integer domain:
screen coordinates strictly between -32765 and 32767, per-axis extent below
32768, UV coordinates from -8192 through 8191, lights clamped to [-256,256],
and signed32 interpolation intermediates. Negative UV addressing, enormous
coordinates, arbitrary polygons, stipple/keyed overlay shader variants and
other overflow behavior are not native parity claims.

Native tests compare every field of all 759 original spans from 52 triangles,
including clipped, tied, reversed and degenerate controls. Separate sampling
tests cover signed shade wrap, untouched caller/background pixels, malformed
textures, invalid later triangles and empty clips. A focused standalone
ASan/UBSan probe also exercises the 52 cases and all four host rounding modes.

With the user's owned stock installation, reproduce the four production C++
indexed frame hashes (8,967 writes) without starting the engine:

```sh
python3 tools/reference/verify_native_terrain_raster.py --texture-pcx /path/to/TEXTURE.PCX
```

This compiles only the production raster and a small probe using the selected
GCC/Clang-compatible driver (`--cxx`). Original pixel bytes stay in temporary
files and are not bundled. Matching these indexed frames proves this primitive
for the measured contexts, not whole-map color, palette, layer or zoom parity.
