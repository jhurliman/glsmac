# Native Detail minimap handoff request

The current World slice does not establish native Detail painter output. Do not present coarse World colors or a whole-world GPU thumbnail as that separate view. Existing authorities: behavior/minimap-world-detail-camera.md and behavior/minimap-console-controls-and-colors.md.

## Established callers to connect

- Detail command1007 retains choice1, shows a populated separate view and hides World; World1006 restores choice0.
- Detail initializes flags0x80000082, zoom−7,239×119, diamond56×28 and spans5/5. Its focus is a retained native tile center, independent of active-map camera thereafter.
- Shift overview click first validates mapping, then centers Detail on the current active-map center, not the clicked tile; active camera remains unchanged. Ordinary click only recenters active map.
- World yellow outline references the retained Detail geometry; existing measured seam/corner endpoints are reusable.

## Finite missing painter/knowledge evidence

1. Trace actual Detail flags0x82 through terrain, overlay and model painters. State which World/main-map raster helpers are genuinely reused, which are skipped, and ordering/clipping.
2. For an80×80 round world, zoom−7,239×119 and fixed center40,40, record first tile coordinates, submitted IDs/copies, anchors, diamond geometry and painted bounds. Include center near seam0,40 and pole40,0; one flat-edge context if admitted.
3. Execute representative land/sea/elevation, roads/forest/fungus/improvement, unit/base overlays only where those painters actually run. Preserve palette/atlas indices and source rectangles. Distinguish opaque background and unknown-tile behavior from test sentinels.
4. Pin observer admission for known, unknown, initial-survey and all-map-revealed tiles. Establish whether Detail can show current/retained foreign units/bases; never infer from full retained world availability.
5. State unsupported dimensions/global scale and any remaining animation/resampler boundary explicitly. Do not expand into unrequested mode persistence, right-click menus, political selectors or guessed drag cadence.

## Port consumers after evidence

- Backend GameMinimap.cpp: detached authenticated observer projection with only fields admitted to the actual Detail painter; no read-time mutation or installed-file fallback.
- frontend/widget/Minimap: separate mode and retained Detail center, measured raster composition, safe owned payload, hit mapping and original yellow World outline.
- frontend/Game: read-only active native tile center after parent camera/projection work lands; avoid reconstructing from stale selection or unit coordinates.
- bottom_bar/mini_map.gls.js: original Detail atlas control and Bool callback, World/Detail visibility/state transition, Shift action.
- Dedicated native and actual GSE caller cases: mode transitions, differing active/Detail centers, seam/pole clips, unknown admission, detached lifetime/late GPU safety.