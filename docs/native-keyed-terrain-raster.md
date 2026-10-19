# Ordered indexed terrain batches

`smac_terrain_raster::DrawBatch` consumes an ordered list of opaque shaded,
keyed shaded and keyed unlit 3/4-vertex commands. Different loaded indexed
textures can be interleaved without copying the destination for each material.
The destination is copied once; commands retain their original submission
order, individual clip rectangles and explicit shade limits.

Shaded edges use the native half-texel bias. Unlit keyed edges omit it.
Both use the existing integer edge walker and explicit x87 chop sequence.
Loaded key9 is tested before shade addition; a non-key sample that shades to9
is still written. Unlit samples copy the non-key loaded byte unchanged.
Native ordinary batch shade limits default to [-4,4]; the original triangle
API retains its explicit [-256,256] control profile and existing behavior.

Each command's texture is a synchronous borrowed view. It must stay alive and
unchanged during the call. No source pointer escapes into the returned owned
bitmap. Invalid source metadata, shader kinds, polygon counts, arithmetic or
sample addresses throw without modifying the caller or returning partial
output. Viewport and command clips are intersected before drawing.

The [original keyed reference](behavior/ordinary-land-keyed-raster.md) covers
27 complete native batches: Rock/Forest on flat, hill and coast geometry,
all four Fungus rotations on those shapes, clips, tied/reversed quads and
synthetic key/shade controls. Native tests compare all 3,332 literal spans;
six asset-free synthetic whole-frame controls pin actual key and shade writes.
Existing 759 triangle span controls remain intact. The owned-asset verifier
matches all 27 native full-frame hashes, 100,917 sampling attempts and 71,562
writes using the production C++ batch:

```sh
python3 tools/reference/verify_native_terrain_polygons.py --texture-pcx /path/to/TEXTURE.PCX
```

This compiles just the production raster and a small probe with a
GCC/Clang-compatible driver (`--cxx`). Original loaded pixels and output frames
remain temporary; no artwork bytes are bundled. Shader coverage is established
for the bounded ordinary convex submissions in the reference, not arbitrary
concave polygons, out-of-crop/negative UV profiles, overflow, other bitmap
pitches, view/world draw ordering, RGB presentation or whole-scene occlusion.
The live terrain painter still needs indexed asset loading, view/invalidation
and the retained material/lighting producers connected to this batch.
