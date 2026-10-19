# Authored Planet negative-detail geometry

The original **does reach negative-detail height lookups** on installed standard
Planet. Ordinary terrain fallback loses real geometry. Preparation and runtime
lookup use different indices; do not substitute a positive suppression row.

[Cases](planet-negative-landmark-geometry-cases.json) and the bounded
[reference](planet_negative_geometry_reference.py) extend the existing
[positive profiles](landmark-height-shade-profiles.md). Run
`python3 tools/reference/verify_planet_geometry.py --planet-map PATH --cxx clang++ --sanitize`
with the owned installed `maps/planet.MP`. The verifier compares production C++
against the independent [full original returns](planet-negative-landmark-full-cases.json);
it does not derive expectations from the portable reference.

## Preparation and lookup

Run whole-map preparation first. It restores stock profiles. Negative words with
Crater/Mount bits clear five profile entries selected by positive row
`-1-detail`. Neither the retained word nor runtime detail is rewritten. Ignore
unrelated negative words when applying these Crater/Mount clearing rules.

On a cold height query, runtime sign-extends the high byte and reads a mapping
byte at signed `5*detail + vertex` relative to the normal mapping table. Negative
values read stable preceding bytes in this pinned original. Authored Planet uses
these bounded rows, in C/W/N/E/S order:

| Signed detail | Mapping indices |
| --- | --- |
| -48 | 4,3,4,3,5 |
| -46 | 4,3,7,1,8 |
| -45 | 1,1,1,1,1 |
| -30 | 2,2,5,2,7 |
| -28 | 10,2,12,0,6 |
| -26 | 13,0,1,1,1 |
| -8 | 7,3,7,3,12 |
| -1 | 3,13,3,14,0 |

Use normal height membership priority and the prepared profile selected by the
word. Mount profile bytes are unsigned; other height profiles are signed. Apply
established scaled deformation, corner averaging and clamps. Extra landmark
shade skips negative detail; ordinary slope lighting remains active. This is a
bounded compatibility table, not a rule for arbitrary negative detail or another
executable version.

There are nine negative height-bearing tiles. Prepared Crater profiles give zero
additional scalars at all indices their negative tiles select. Pholus Ridge gives
nonzero values at several indices: -48 selects scalars80,0,80,0,80. Zero local
deformation does not imply flat corners; neighbors and shared caches contribute.

## Shared cache

Retain the original shared corner cache and query order. A corner query publishes
into four adjoining tile slots. The projector requests S/E/N/W; cold corners can
recursively request neighboring centers. Cached signed byte -1 is also the cold
sentinel and must be recomputed. Do not calculate tiles independently or
precompute corners in an arbitrary order.

Two explicit whole-map orders starting with fresh native caches change geometry
on21 tiles. Both orders request projector, five heights, then five shades per
tile. The reference reproduces all returns. At relief step25, heights are C/W/N/E/S:

| Tile and word | Forward order | Reverse order |
| --- | --- | --- |
| (11,35), 0xd0400800 | 45,33,28,11,48 | 45,33,48,11,28 |
| (9,39), 0xf8400800 | 20,0,27,33,27 | 20,0,47,33,27 |
| (11,39), 0xff400800 | 25,33,23,6,37 | 25,33,23,6,17 |
| (11,43), 0xd2400800 | 45,45,22,11,42 | 45,45,42,11,22 |

Do not hardcode either order's final heights. Preserve caller order and cache
lifetime for the view being rendered.

## Receipt bounds

The installed Planet loader and full profile preparation execute. Two explicit
3200-tile traversals verify6400 projectors,32000 requested height returns and
32000 requested shade returns, plus recursive native calls. Each records18 actual
negative height-mapping reads; other queries can hit the shared cache.

A separate receipt runs the actual terrain painter from entry to its pre-feature
boundary on all127 authored height-bearing tiles with one shared fresh cache.
The independent reference matches508 primary triangle vertex/shade arrays.
Raster submissions are observation sinks; no height, shade or projector return
is supplied. Observer0 exploration is the only tile mutation in this painter
receipt. The two complete-map traversals keep loader terrain unchanged.

These are raw imported Planet controls at zoom0/default relief25. They do not
certify complete first-session climate/setup state, full viewport scheduling,
arbitrary camera/cache histories, other relief scales or feature-art pixels.
Use the actual viewport call sequence when integrating this rule.

## Production integration checkpoint

`LandmarkGeometry` implements these bounded rows only when its caller explicitly
selects the pinned Planet profile. Default callers retain the unsupported-detail
gate. It also models projector request order, the cold -1 sentinel and the height
cache side effects of face shading. The standalone original-return controls pass
with ASan/UBSan, as do the existing positive geometry and indexed assembler
controls.

Live backend finalization still primes the shared cache in a different order from
the original viewport painter. Enabling negative geometry there alone would not
prove viewport fidelity. The remaining integration must preserve the actual view
request order and keep terrain, unit and base projection coherent. This change
does not yet enable negative geometry in the live renderer.
