# Native Detail minimap handoff and implementation bounds

The current World slice does not establish native Detail painter output. Do not present coarse World colors or a whole-world GPU thumbnail as that separate view. Existing authorities: behavior/minimap-world-detail-camera.md and behavior/minimap-console-controls-and-colors.md.

## Established callers to connect

- Detail command1007 retains choice1, shows a populated separate view and hides World; World1006 restores choice0.
- Detail initializes flags0x80000082, zoomâˆ’7,239Ã—119, diamond56Ã—28 and spans5/5. Its focus is a retained native tile center, independent of active-map camera thereafter.
- Shift overview click first validates mapping, then centers Detail on the current active-map center, not the clicked tile; active camera remains unchanged. Ordinary click only recenters active map.
- World yellow outline references the retained Detail geometry; existing measured seam/corner endpoints are reusable.

## Finite missing painter/knowledge evidence

1. Trace actual Detail flags0x82 through terrain, overlay and model painters. State which World/main-map raster helpers are genuinely reused, which are skipped, and ordering/clipping.
2. For an80Ã—80 round world, zoomâˆ’7,239Ã—119 and fixed center40,40, record first tile coordinates, submitted IDs/copies, anchors, diamond geometry and painted bounds. Include center near seam0,40 and pole40,0; one flat-edge context if admitted.
3. Execute representative land/sea/elevation, roads/forest/fungus/improvement, unit/base overlays only where those painters actually run. Preserve palette/atlas indices and source rectangles. Distinguish opaque background and unknown-tile behavior from test sentinels.
4. Pin observer admission for known, unknown, initial-survey and all-map-revealed tiles. Establish whether Detail can show current/retained foreign units/bases; never infer from full retained world availability.
5. State unsupported dimensions/global scale and any remaining animation/resampler boundary explicitly. Do not expand into unrequested mode persistence, right-click menus, political selectors or guessed drag cadence.

## Port consumers after evidence

- Backend GameMinimap.cpp: detached authenticated observer projection with only fields admitted to the actual Detail painter; no read-time mutation or installed-file fallback.
- frontend/widget/Minimap: separate mode and retained Detail center, measured raster composition, safe owned payload, hit mapping and original yellow World outline.
- frontend/Game: read-only active native tile center after parent camera/projection work lands; avoid reconstructing from stale selection or unit coordinates.
- bottom_bar/mini_map.gls.js: original Detail atlas control and Bool callback, World/Detail visibility/state transition, Shift action.
- Dedicated native and actual GSE caller cases: mode transitions, differing active/Detail centers, seam/pole clips, unknown admission, detached lifetime/late GPU safety.

## Known-geography admission correction

Generated canonical quickstart campaigns may retain no optional campaign-rule
selection. That absence does not erase actual saved exploration or all-map
revelation. The World query admits those known cells and only enables initial
survey for an explicitly retained bit0x0100. Unknown cells remain transparent
when survey authority is absent. This fixes an empty World panel; it does not
implement or relabel the still-unclosed separate Detail painter.


## Implemented ordinary Detail profile

The source closure in `behavior/detail-minimap-painter.md` and its two case files
now drives the separate live Detail view. It uses an owned authenticated observer
snapshot, the measured 80×80 / 239×119 / zoom−7 painter, integer step14 ordinary
relief, indexed physical fills, and source-phase-preserving 9/16 Base sampling.
World and Detail retain distinct views; Shift-World click adopts the active-map
center for Detail without moving the active camera. Ordinary World click keeps
its existing active-camera navigation. Detail click/drag is deliberately unbound
until that original interaction is established.

Eleven original terrain frames pin 410 fills and 217303 ordered stores, including
unknown backing, seam/pole/flat clips and native padding. Thirteen Base contexts
pin visibility/anchors; seven admitted compositions pin 2663 actual source
samples and complete indexed output before the declared font boundary. Native
Base visibility is explored-or-revealed plus ownership-or-visited; survey alone
cannot reveal a Base. Units are bypassed by this painter.

Live admission remains intentionally bounded: 80×80 maps, ordinary default-relief
geometry and the measured stock four-size/two-land-sea/three-defense body banks.
Base variants now use saved population, Hab limits and facility IDs, with an HQ
size increment. CDF/Empath are explicitly absent under the admitted no-project
capability; project-bearing saves remain unsupported. Captured Bases lack a
separate retained art-family authority and are omitted. Ambiguous duplicate
faction-profile identities retain the body without guessing native owner colors.
Unclosed deformed tiles and dependent corners are omitted. Other map sizes retain
the functioning observer-filtered World view. Population labels use measured 8/88 boxes and signed decimal text. The
existing FreeType Arial Bold9 service supplies glyphs; no historical GDI raster parity is claimed.
The active camera is resolved through the port's existing projected recenter
coordinates, rather than a stale selected object. World outline geometry is
measured, but original blink timing remains unclosed. These limits do not amount
to complete lower-console or whole-frame parity.


The frozen `behavior/base-body-variants.md` matrix adds85 native helper contexts:
24 retained University banks,74 Detail pre-font composites and120058 source
samples. Population style uses actual garrison, directed Pact/infiltration,
revelation, Governor and neighboring-viewer-unit state, after Base visibility.
The main-map frontend still needs a separate safe selector DTO for HQ/defense
and custom Hab limits; this Detail-only slice does not silently read backend
mutable state from that renderer. Native glyph output remains a font boundary.
