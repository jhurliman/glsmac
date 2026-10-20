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
  These inputs do not yet drive ordinary main-map rendering; console admission
  now has its [live saved-knowledge reader](console-unexplored-admission.md);
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

The [mixed exploration renderer](mixed-exploration-renderer.md) now closes all
four original partial21 indexed frames with production material selection,
including the distinct remembered-center/live-neighbor inputs. Survey and live
ordinary presentation are still separate remaining boundaries.

The [production Unity Survey renderer](native-unity-survey-renderer.md) now
matches both four-frame survey sequences through its real material/raster
producers. Global lighting is explicit, known land alone calls face shade and
River geography remains visible. Ordinary main-map caller/entity integration,
other map/camera profiles and display-option state remain open.

## Live actor bridge: verified requirements

The [complete actor/name/stack consumer controls](behavior/live-object-name-stack-admission.md)
now distinguish the live presentation inputs. The frozen archive was verified
against manifest SHA256
`cd11abca9326e5237e8a0134b2869701d083507c81212952345a6b836fccc92a`.
Do not wire all scene actors to one terrain-known Boolean:

- Base bodies and primary names have separate admission. Own or stored-disclosed
  names can survive an unexplored tile while its body is hidden.
- Ordinary own Unit bodies bypass stored masks, except the native16-bit flag
  combination0x440. Retain that word separately from DWORD status. Conventional
  records now retain an optional native word in Unit10. Fresh factories set
  zero, while older port saves retain absence and native imports must preserve
  their original word. The [initializer/event controls](behavior/conventional-unit-flags16-authority.md)
  also require separate event mutations. [Disclosure effects](unit-disclosure-effects.md)
  now commit with admitted sight transactions; duration expiry, frontend
  transport and the ordinary actor consumers remain open.
- Admit the lookup/head Unit before native stack promotion; filtering and sorting
  all members independently changes the mixed-owner control. Use retained source
  links/status/domain and the actual selection/control/pending context.
- Survey alone does not admit bodies. Editor and special override pointers are
  explicit presentation contexts, rather than general exploration exceptions.
- Render lifecycle is independent of turn>0. Publish first Base roster/name data
  with founding and repaint under the current active context.
- Hidden bodies must remove model, ordinary overlays and separate stack badges.
  The stack-instance lifetime fix is in PR424; labels still need their own gate.

Terrain integration also must preserve disclosure across camera fallback,
selection, new-game reset and reload. The existing inspector cannot become the
ordinary game's reveal fallback when a camera or snapshot is unsupported.
