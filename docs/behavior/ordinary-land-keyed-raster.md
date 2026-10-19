# Ordinary land keyed polygon raster

The portable `terrain_polygon_raster_reference.py` generates spans from submitted integer vertices, UVs and lights, then samples loaded indexed crops in submission order. It consumes no captured spans. All27 bounded native frame batches match:3332 spans,100917 sampling attempts,71562 actual writes and27 complete1024×511 indexed-frame hashes. Owned TEXTURE.PCX is supplied externally.

Run `python docs/behavior/terrain_polygon_raster_reference.py --texture-pcx PATH/TO/TEXTURE.PCX`. Numeric inputs, native spans and hashes are in `ordinary-land-keyed-raster-cases.json`. Raw pixels and write streams remain private.

## Exact rules

- Opaque and keyed shaded polygons use the same edge walker and horizontal interpolation as the closed ordinary triangle reference. Three and four vertices are supported. Input winding, first-index minimum/maximum Y ties, horizontal-edge skips, integer error correction, half-open rows/spans and clipping phase are preserved. Reversed winding is not normalized.
- Shaded U/V edge starts are `(UV<<16)+32768`; unlit keyed U/V edge starts are `UV<<16`. This changes texel coverage even with identical geometry and UVs. Both paths use signed fixed-point edge division and the measured x87 horizontal derivative sequence.
- Shaded light inputs clamp to[-4,+4] in these fixtures. Interpolated phase is16-bit8.8; its high byte is added to the loaded texture byte modulo256. Keyed shaded compares the **unmodified loaded index against9 before adding light**. Loaded9 stays transparent even when light would change it. A non-key index may shade to9 and is still written. The unlit path copies each non-key loaded byte unchanged.
- Rock triangles stay interleaved with their corresponding base triangles. Forest and Fungus are quads after the base fan. Rendering all base triangles followed by all Rock triangles changes the native submission order and is not this contract.
- PCX loading rebases source bytes by10 modulo256; key9 therefore corresponds to source255 in the tested load. Palette RGB conversion follows the separate palette contract.

## Bounded evidence

Rock and Forest each cover flat, hill and coast; Fungus covers all four observed UV rotations across those three shapes. Three hill batches clip all polygon work to[550,270,600,300]. Synthetic56×56 crops containing9,10,255,1 exercise transparency before shading and byte wrapping. Tied top/bottom Y quads, clipping and reversed winding are additional controls.

The Fungus rotation/shape cross-product is a bounded replay combining separately native-produced geometry and UV/crop plans. It does not claim that every combination was played through a natural world session. Native full raster routines and their actual stores execute in every batch; span and pixel results are observed, not supplied.

Supported inputs are the convex ordinary3/4-vertex submissions in these fixtures, integer coordinate/UV ranges enforced by the reference, nonnegative in-crop sampling and the initialized shade limits/key above. Concave/self-intersecting polygons, arbitrary out-of-crop/negative UV addressing, overflowing32-bit intermediates, other bitmap pitches/formats, zoom/camera/world draw ordering, RGB output, and whole-scene occlusion remain outside this closure. The generator's broader integer input checks do not establish parity for every accepted polygon.

The original triangle and selector archives are unchanged. New evidence is frozen separately under `tmp/reference/ordinary-land-keyed-raster-evidence-20261019`; broad reverse engineering remains paused.
