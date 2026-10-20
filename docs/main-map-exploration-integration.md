# Main-map exploration integration

This work is open. The release target is ordinary SMAC/SMAX gameplay across
supported map sizes and cameras, not a revealed Planet inspector. The frozen
[original contract](behavior/main-map-exploration-and-startup-zoom.md) separates
terrain exploration, remembered improvements, unit disclosure and base discovery.

## Current caller gaps

- `Map::CopyNativeTerrain` and `OP_GET_NATIVE_TERRAIN` now copy persistent saved
  exploration, observer presence, faction reveal and the retained optional Unity
  Survey selection into an owned80x80 local snapshot. The request authenticates
  the actual local single-player participant. `World::KnowledgeAt` distinguishes
  unknown, survey and known terrain, retaining unavailable authority separately.
  These inputs do not yet drive ordinary main-map rendering or console admission;
  editor translation and object disclosure still need their separate callers.
- `NativeViewGeometry::Prepare` without a knowledge input schedules land height/shade
  queries. Unknown terrain bypasses that material pass in the original; the
  unknown outline's geometry/cache schedule now has an independently matched
  [production core](unknown-view-renderer.md); the ordinary caller must supply it.
- `NativeTerrainRenderer` now gates unknown materials and owns opaque black
  backing when given a knowledge-profile plan. The ordinary caller does not yet
  supply that plan. Its legacy presentation must not expose revealed terrain
  beneath unknown regions.
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
profiles. The new [unknown query/backing receipt](behavior/unknown-map-query-order-and-backing.md)
independently derives the scheduled geometry and retained unknown frames. The
production core matches those controls; ordinary-game caller integration is
still required. Partial-known final pixels and known grid presentation retain
their separate unclosed boundaries.

## Owned exploration input validation

Seven focused native groups pass, including the real backend request path, saved
knowledge serialization/deserialization, later-state independence, unready knowledge,
foreign reveal isolation and distinct survey admission. NULL-view reads leave
geometry caches and gameplay RNG untouched. Five production translation units
(changed units and known frontend consumers) pass correctness analysis. A focused
five-TU ASan/UBSan run passes these groups; remaining objects are uninstrumented
and leak detection is disabled. Ordinary-game fog remains an integration task.
