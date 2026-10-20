# Exploration-aware native view core

The production native view scheduler and ordered terrain batch now implement the
standard unknown-terrain continuation for the measured wrapped80x80 Planet,
1024x511, zoom0/relief25 profile. This closes the CPU geometry/raster boundary;
the ordinary game still needs its owned knowledge inputs and presentation paths
connected. It is not a claim that the main-game fog issue is fixed.

`ViewGeometry::Prepare` accepts a synchronous knowledge array. It retains none of
that caller's memory. Unknown tiles retain projector and entry-corner queries,
then execute the12 height queries in triangle order without shade calls. Outline
vertices read the signed height-cache bytes after their queries. Sea height can
return0 without changing FF in that cache; replacing its signed−1 with return0
would lose the original shelf geometry and cold/warm difference.

The plan owns its unknown classifications and outline coordinates. The renderer
emits indexed194 lines at their original painter positions, bypassing hidden
material/features. Full knowledge-profile repaint starts at opaque index65;
incremental redraw copies the matching history and clears exposed strips to0.
The renderer rejects history with a different backing-opacity profile. Callers
must still invalidate history on world/viewer/knowledge/display changes; that
live state boundary is part of the remaining integration.

The absent knowledge argument preserves the existing revealed diagnostic path.
It must not become an ordinary-game fallback. Survey is a distinct reduced
terrain policy and is not implemented by treating every survey tile as known.
Object disclosure, sprite/name layers and console admission are separate gates.

## Evidence and checks

The87-file original query/backing bundle was verified against manifest SHA256
`1cbc3d69c53a4adb5113061c4cac4b695539e2e017cc9bda5beeefbb5f0c60fd`.
See [the original schedule](behavior/unknown-map-query-order-and-backing.md).

`tools/reference/verify_unknown_viewport.py` compiles the actual production
scheduler, geometry, renderer and raster code. Only the purchased Planet map and
the supplied exploration footprint enter the probe; captured queries, returns,
cache bytes and lines are comparison targets. Eight first/repeat/east/back
controls match25,112 outer queries,5,344 outlines and230,400 semantic cache bytes.
The four fully unknown frames match all2,093,056 original indexed pixels and
have opaque RGBA backing. Partial-known frames check queries/lines/caches only;
the probe's placeholder art is not an original known-material raster profile.

The existing valid-faction revealed viewport control was also rerun: all four
complete indexed frames and paint-store counts still match. Both standalone
checks passed with ASan/UBSan instrumentation of their compiled production code.
The cached game build, five focused native raster/view groups, and changed-unit/
known-consumer correctness analysis pass. A separate four-TU sanitizer run covers
the native tests and raster/renderer; its other objects are normal and leak
detection is disabled. The asset-free cold-sea test guards signed cache reads,
no hidden shade warming, borrowed-mask lifetime, opaque backing and incompatible
history rejection. Ordered line tests retain the existing polygon regression.

Example standalone check, with a local C++17 compiler and purchased map:

```sh
python3 tools/reference/verify_unknown_viewport.py \
  --planet '/path/to/Alpha Centauri/maps/planet.MP' \
  --out /tmp/glsmac-unknown-view --sanitize
```

The ordinary renderer bridge, known-grid presentation, other cameras/map profiles,
survey, separate object admission and preview/text disclosure remain open in
[the integration plan](main-map-exploration-integration.md).
