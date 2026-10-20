# Landmark and high-elevation picking acceptance

The new original controls exposed a production height-cache error: cold corner
returns were capped at 127 before being returned. At uniform band7, zoom5,
corner (17,29,1), the original first returns 128 while publishing 127 into its
shared cache; the immediate repeat returns 127. Positive landmark profiles have
their own earlier upper clamp, while zero-profile ordinary centers and corners
retain their uncapped first return. Negative profile lower clamps are unchanged.

`LandmarkGeometry::Height` now separates profile-sign clamps, returned values
and published shared bytes. Picking keeps using the same mutable view-local
geometry. For example, original band7/zoom8 pixel (532,169) first selects
(22,38), then (21,37) without invalidation. A native regression checks both the
cold corner and that repeated-pixel result through production view geometry.

The frozen supplement manifest is
`4ac06012c36db50c754c3e333334d13febf7608ce7e249e1851131fe13f267d6`;
all 15 listed files were verified before use. The original's standalone verifier
also passes independently. The compact [production receipt](behavior/native-picker-landmarks-cases.json)
retains numeric map metadata, profile inputs, cold/repeat pixel sequences and a
complete original event/cache stream SHA256 per profile. Expected hashes derive
from the original traces, not production output. No executable or purchased map,
art, font or save payload is included.

Run `python3 tools/reference/verify_native_picker_landmarks.py
--output-directory /tmp/glsmac-picker-landmarks --sanitize` (as one command).
The production probe compares layout fields, results, ordered height queries,
edge comparisons and all 16,000 height bytes after every pick. Cold phases
reconstruct a view; repeat phases preserve that exact view's cache.

The 19 profiles contain 4,116 picks, 36,742 height queries, 18,371 edges and 295
equalities. They include 104 pixel pairs whose picked tile changes on immediate
repeat; the band7/zoom5 threshold also changes query streams without changing
final picked tiles. Planet controls cover Garland Crater, Mount Planet, Sunny
Mesa and Pholus Ridge at zoom0/16, including authored negative Crater/Ridge
details. The pinned Planet file has no Nessus Canyon membership. Arbitrary
prior paint histories, malformed profiles and other map files remain outside
this bounded supplement.

Validation: cached incremental build and 56 affected native groups; six known
production geometry consumers pass correctness analysis. Asset-free production
probes pass under ASan/UBSan for the new supplement, all 72 earlier picker
profiles (9,955 picks), all 2,376 landmark contexts (15,280 height/shade queries)
and 25 camera controls (154 receipts). All 13 original complete terrain/grid
frames remain exact (6,802,432 indexed pixels). No full platform or foreground
game run was repeated for this height-query correction. These receipts do not
establish terrain color/relief screen acceptance or finish ordinary map fog.
