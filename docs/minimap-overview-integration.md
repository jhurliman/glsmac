# Physical overview integration boundary

The console World overview reads an authenticated local snapshot of retained
terrain bands, exploration, initial-survey rules and all-map revelation. Reads
do not change the world, simulation IDs, RNG or pending frontend requests.
Unknown tiles have no polygon; a late whole-world GPU capture cannot replace
an admitted observer-filtered snapshot.
The console's existing black underlay uses explicit RGB so it is fully opaque;
the legacy packed named-color conversion loses one alpha step and otherwise
lets traces of current camera terrain show through the skipped polygons.

The round 80x80 Planet profile reproduces the original 320x152 base-pass source
bitmap placement and raster clipping. Its controlled uniform band3 bitmap
matches original SHA256
`e5147aeeb3f2f8b8821670d4fe33d4f4e7c62c800232d040a4468081e3f57f12`.
Target stretching currently uses explicit nearest-neighbor sampling; the native
resampler's final pixels have not been established. Other map sizes and flat
worlds use a functional normalized tile sampler over the same observer-filtered
colors, without inheriting the native Planet placement claim.

The measured 239x119 console rectangle and normalized click mapping replace the
rotated half-map/four-pixel correction. Accepted clicks recenter the real active
map on a valid tile and preserve selection. Existing port drag and wheel gestures
remain functional; their cadence is not asserted as original behavior.

Legacy/diagnostic worlds with no retained knowledge authority keep their existing
GPU presentation. An existing knowledge authority that cannot be projected
safely returns an explicit empty frame. Loss of an already admitted snapshot
also clears the map instead of exposing the cached whole world.

This is the physical base-pass slice, not completion of the two-mode console.
The debug Detail action that simply hid the map is removed; its original atlas
artwork is retained for the immediate separate Detail-view implementation.
Native overlay pass, yellow detail-pointer outline, political/rainfall command
bindings, loaded-slot palette retention and the four-button reference identity
remain outstanding. No invented political toggle or historical map is exposed.
