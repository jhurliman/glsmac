# Main-map exploration integration

This work is open. The release target is ordinary SMAC/SMAX gameplay across
supported map sizes and cameras, not a revealed Planet inspector. The frozen
[original contract](behavior/main-map-exploration-and-startup-zoom.md) separates
terrain exploration, remembered improvements, unit disclosure and base discovery.

## Current caller gaps

- `Map::CopyNativeTerrain` and `OP_GET_NATIVE_TERRAIN` copy live/remembered
  improvements and base admission into an owned80x80 local snapshot. The
  snapshot currently omits tile exploration, editor/reveal and survey policy.
- `NativeViewGeometry::Prepare` always schedules land material height/shade
  queries. Unknown terrain bypasses that material pass in the original; the
  unknown outline's geometry/cache schedule needs its own comparison.
- `NativeTerrainRenderer` emits terrain/features without an exploration gate.
  Its coverage-based alpha exposes the legacy plane beneath unpainted regions;
  an unknown region must not become a transparent hole over revealed terrain.
- `Game::UpdateNativeTerrain` presents that renderer only in the explicit
  inspector at the admitted viewport/zoom. The ordinary map remains the GPU
  path. Implementing a detached fog helper alone will not close the user issue.
- Legacy terrain sprites, units, bases, labels, selection and console previews
  have independent presentation paths. Hiding terrain alone is insufficient.

## Implementation and acceptance boundaries

1. Carry owned, viewer-authenticated exploration and separate object-disclosure
   inputs from their existing saved knowledge authorities. Preserve the current
   local-only request boundary; do not invent a network disclosure endpoint.
2. Implement the original standard unknown background/outline raster and its
   ordered geometry schedule. Compare first/repeat/pan/back and partial knowledge
   against original controls. Unknown outlines ignore the normal grid toggle.
3. Keep survey bit0x100, editor reveal and faction reveal explicit. Survey has
   its own reduced terrain policy; reveal does not automatically disclose foreign
   objects. Avoid a universal all-live fallback for missing remembered state.
4. Wire the ordinary presentation path, including opaque unknown regions and
   the separate sprite/body/name/preview gates. Keep the inspector's intentional
   reveal explicit. Unsupported cameras or map profiles must not silently expose
   hidden information through a legacy fallback.
5. Reset viewer, world-ticket, geometry and retained bitmap history together.
   Knowledge changes invalidate affected cached output; incremental redraw must
   not retain formerly visible objects or reveal stale pixels after load/reset.
6. Validate real gameplay startup, reveal by movement/founding, revisiting explored
   terrain, foreign-object disclosure, save/reload and new-game reset. Add map-size,
   camera and platform integration checks when the live boundary is complete.

The existing known-terrain view controls prove their declared geometry/material
profiles. The new unknown-frame receipt independently replays captured line
geometry; it does not yet independently derive every scheduled geometry query.
That distinction is retained while the reverse-engineering follow-up resolves it.
