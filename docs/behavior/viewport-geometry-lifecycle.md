# Viewport geometry order and cache lifetime

Geometry is **view-local and lazy**. Do not prime the whole-map shared corner
cache during engine finalization or snapshots when matching the original viewport.
Render selection and query order establish cache history. A camera pan preserves
that history; a reload rebuilds it.

The stable [reference](viewport_geometry_cache_reference.py) combines the bounded
[negative Planet model](planet-negative-landmark-geometry.md) with actual shade
caching and coastal partial-query effects. The [ordered controls](viewport-geometry-lifecycle-cases.json),
[boundary controls](viewport-cache-boundaries-cases.json) and
[Base/unit control](viewport-object-projection-cases.json) are separate receipts.
Run `tmp/reference/check_viewport_geometry_lifecycle.py --verify-only` with the
emulation venv and installed Planet asset. It verifies30,772 ordered outer query
returns, all201,600 height/shade cache-byte positions across seven controls,
selection/order, five invalidation controls and Base/unit routing. Full native
12-byte cache hashes are also retained; independent cache equality checks bytes0..8.

## Cache boundaries

Each tile has four face-shade bytes, five height bytes C/W/N/E/S, a status byte
and two packed slope bytes. Height FF is cold; a computed signed-1 remains cold
on the next query. Shade FF is cold; cached face shades encode value+128.

| Event | Native effect |
| --- | --- |
| Fresh view/cache initialization | New12-byte records; all shade/height bytes FF; no whole-map geometry queries |
| Same view repaint | Retains cached geometry; repeat control changes zero cache bytes |
| Camera pan or recenter | Retains cache; query only selected repaint work and preliminary anchors |
| Zoom changes | Clear bytes0..8 on all3200 records; retain status/packed slope bytes |
| Relief setting changes | Clear height bytes4..8 only; retain face-shade/status/packed slope bytes |
| Tested regional invalidation argument1 | Clear height slots on25 tiles, retain shade/status/slope bytes |
| Installed Planet reload | Close active auxiliary views; rebuild main and fixed renderer caches |

The25-tile stencil satisfies doubled-X parity and `abs(dx)+abs(dy)<=4`. The native
positive-detail landmark edit uses this stencil on active views, complements the
detail byte, clears its transient bit and marks the shared dirty flag. This is a
specific edit path, not a claim that every terrain mutation uses the same mask.
Interior stencil controls do not establish pole/seam behavior for edits.

Full reload was executed with seven auxiliary views explicitly active: those
views become inactive and lose their caches. The main view receives a new cold
cache, as do three fixed renderer objects. The underlying rebuild routine always
includes the main slot, includes active auxiliary slots, and handles the three
fixed renderers. Old pointers must not survive a map replacement.

## Query side effects

The projector asks S/E/N/W. A cold corner can query neighboring centers in native
stencil order. At the first invalid/sea contributor it takes the boundary result,
but center queries already made remain cached. Testing all contributors before
performing any center query loses those partial writes.

A **cold face shade requests all four corner heights W/N/E/S**, even though the
face's final light depends on two slope codes. Cache the resulting face value.
A warm shade returns without those height touches. Preserve this distinction
after height-only invalidation; recomputing a warm shade can change shared corner
history. The reference models both effects and matches every measured height and
shade cache byte.

## Selection and pan sequence

Use native first coordinates/spans, then candidate rows `0..sy+oy` inclusive.
Even rows submit `ox` columns; odd rows submit `sx` columns. Double-X advances2.
Wrap the map coordinate while retaining the visible copy for placement. Reject
invalid world rows/columns, not flat centers outside the pixel rectangle. See the
earlier [full-redraw order](terrain-view-order.md) for seam/pole controls.

The new sequence uses80x80 Planet,1024x511 viewport, zoom0, camera22,38:

| Control | First tile | Terrain calls | Object calls | Cache bytes changed |
| --- | --- | --- | --- | --- |
| First paint |10,26|288|288|3715|
| Repeat |10,26|288|288|0|
| Keep target24,38 visible |10,26|0|0|0|
| Explicit pan to24,38 |12,26|63|88|225|
| Explicit pan back22,38 |10,26|50|75|0|
| Zoom-1 repaint |11,25|338|338|1202|
| Low relief8 repaint |11,25|338|338|1050|

The keep-visible wrapper does nothing inside its safe inner rectangle. For the
first camera, that rectangle is `[15,30,29,44)`. Explicit small pans use a caller
that enables incremental repaint around recentering and resets the flag after it.
The receipt executes that actual caller fragment with supplied target coordinates;
it does not synthesize an input event or assert complete mouse/key dispatch.

Before incremental selection, the painter projects **new first tile**, explicitly
queries its S/E/N/W heights again, then does the same for **old first tile**. Those
queries precede bitmap scrolling and the restricted terrain pass. They belong in
cache history even when the first tile lies outside visible pixels.

For these horizontal interior pans the dirty terrain rectangles are respectively
`[30,26,36,50]` and `[9,26,14,50]`: X is half-open, Y inclusive. A second rectangle
is empty. Object admission expands each rectangle by2 on every side. The public
reference independently derives these bounded rectangles and ordered candidates.
Do not substitute the pixel damage rectangle for the tile admission rectangle.

## Objects and limits

The terrain loop completes before the object loop. The latter independently
projects every admitted tile, including empty tiles. It passes projector X and
projector Y minus one vertical step to the actual object dispatcher. Base and unit
draws reuse those coordinates; they are not a whole-map geometry priming pass.
The separate own Base20,38/Scout22,38 fixture confirms both draw boundaries after
terrain. Names are a later pass; optional overlays have additional conditional
projector loops. They are not enabled in this new numeric control.

The sequence uses core terrain flag1 and empty rosters; the object fixture uses
C1 and explicit records. Terrain runs through its actual pre-feature boundary;
later feature drawing is omitted, and Base/unit drawing is observed at its draw
boundary. Generic window/copy/font/name presentation services are declared.
This is not full default-flags gameplay scheduling, feature callback ordering,
name/overlay cache history or whole-scene pixels. Only the measured horizontal
pan is modeled; arbitrary diagonal/seam pans and dirty-copy histories remain
outside this receipt. No production files are changed.

## Production checkpoint

## Production model

`LandmarkGeometry` now retains separate face-shade and height caches. A cold
face shade queries all four heights in W/N/E/S order. A corner query retains
center-cache writes made before the first invalid/sea contributor, even though
the final corner's ordinary sea-boundary contribution is zero. Face-shade queries
reject off-map X rather than wrapping it; height queries retain their own wrap
semantics. The signed height-1 cold sentinel remains distinct from other values.

`Configure(zoom,relief)` preserves both caches when values are unchanged, clears
both when zoom changes, and clears heights only when relief changes. The relief
scale is applied before profile arithmetic and corner averaging. Construction
starts fresh; callers must discard/reconstruct the model when its owned source
fields or topology change. This API currently bounds zoom to-14..8 and relief to
8 or25; the event receipt specifically exercises zoom0/-1 and relief25/8.

## Validation and remaining integration

Run `python3 tools/reference/verify_viewport_geometry.py --planet-map PATH --cxx clang++ --sanitize`.
The production C++ model matches30,772 height/shade/projector returns across the
seven controls. The verifier supplies original event coordinates and order; it
compares returned values, not the fixture's cache hashes. Those hashes retain
the original receipt provenance and do not assert that this tool compared every
private original cache byte. The prior two complete Planet traversals and508
primary triangles remain independent regression controls.

This is a model/lifecycle correction, not a production view-selector proof.
Live backend finalization and snapshots still prime a whole-map cache. The
frontend must execute the original padded view/pan schedule and keep terrain,
Base and unit projection coherent before negative-detail live admission and a
matched viewport can be declared complete. No new full-screen pixel parity,
climate setup, reload invalidation sequence, or SMAX behavior is claimed here.

## Production view selection

`NativeTerrainView.h` derives padded candidates from camera/layout inputs and
selects the measured small interior horizontal pan rectangles. The independent
`tools/reference/verify_viewport_selection.py --cxx clang++ --sanitize` compares
all2,780 ordered terrain/object selections, both dirty regions, and both pairs
of preliminary projection coordinates against the original event receipt.
The existing1,597 original full-view placements remain regression controls.

The live inspector now uses that padded enumeration instead of clipping flat
centers before painting. Its first grid coordinate is resolved from the existing port camera's flat
origin, then the actual camera is aligned to that native grid so terrain and GPU
objects move together. Translating only candidates left edge wedges between
native rows. This does not establish original input-controller fidelity.
The live path still performs full repaint and uses backend-copied geometry.
The measured incremental selector is available but has not yet replaced live
bitmap scrolling, object projection, name/overlay scheduling, or cache lifetime.
Negative-detail live geometry remains gated pending those connections.

### Owned view scheduler integration checkpoint

`NativeViewGeometry` owns a separate lazy view cache and produces selected terrain and object projection schedules from camera actions. The standalone production probe matches 30,772 original lifecycle query returns without feeding recorded query coordinates into the model. `verify_native_view_geometry.py --full-features` additionally matches all 15,964 original outer geometry query returns from the complete feature-pass controls, using the purchased Planet map. Both pass with ASan/UBSan. The full-feature receipt preserves numeric observations only; original artwork remains external.

This producer is not yet connected to the live renderer. Indexed bitmap history, separate material admission, object anchor propagation and mutation invalidation remain required; these checks do not establish full viewport pixel parity.

The prepared renderer now consumes owned terrain heights/lighting and original projector anchors, and retains indexed pixels plus coverage for the measured zoom0 horizontal96-pixel pan. Exposed-strip clipping follows the original full-feature receipt: east[928,0,1024,511], west[0,0,96,511]. An incremental submission requires matching retained bitmap history. Both renderer paths match all486 original tile-frame controls under ASan/UBSan; the prepared controls deliberately poison backend geometry after detaching their submissions. Additional checks cover both retained bitmap shifts, alpha coverage and rejection of absent history. Targeted correctness analysis passes NativeTerrainRenderer.cpp, GameNativeTerrain.cpp and SDL2.cpp. The live Game path still awaits geometry admission/session lifetime and object-anchor integration; no original full-pan pixel capture is claimed by the bitmap ownership checks.
