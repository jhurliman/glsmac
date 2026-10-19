# World/Detail camera and map-slot color authority

This follow-up closes the tested World/Detail producer, overview navigation, wrapped outline, and loaded-slot color assignment paths. It reuses the [button/color receipts](minimap-console-controls-and-colors.md) without repeating their color grid. The reference's four-button discrepancy remains unresolved.

Use the [named-field cases](minimap-world-detail-camera-cases.json) and [executable reference](minimap_navigation_reference.py). Thirty-five mapping/outline comparisons match native execution; three click-handler cases, the full seven-faction list/text load with a custom order, and four political identity/observer controls are also recorded. [Private source/evidence map](../re-smac-minimap-world-detail-camera.md).

## Which view each button produces

The two verified buttons retain an overview/detail choice and swap visibility between two actual views, as established previously. Their tested producer settings are:

| View/pass | Original produced flags | Zoom | Native window/image backing |
|---|---|---|---|
| Detail view initialization | `0x80000082` | −7 | Minimap rectangle,239×119 |
| World overview base pass | `0x80000000` | −14 | Whole-world320×152 source bitmap |
| World overview overlay pass | `0x80000080` | −14 | Same320×152 source bitmap |

These are actual produced values, not flags inferred from button names. Neither button produces the political bit`0x10000` or moisture bit`0x10` in these paths. The prior explicit-flag political/rainfall color cases remain valid consumers, but **they do not establish a console palette selector**. Keep any unresolved political selector absent or identify it as a port choice; do not relabel World/Detail as political/terrain.

The overview initializer resets its retained detail choice to0, enables its initial geometry rebuild, and centers its internal view at half the world width/height: `(40,40)` for80×80. The detail initializer reads the supplied native focus/cursor coordinates through the normal view reset; the receipt supplies `(8,8)` and observes that center. Selecting Detail subsequently shows that separate detail view and hides the overview; the parent panel remains populated.

At zoom−7 the ordinary projected diamond is56×28. In a239×119 detail window, native geometry gives horizontal spans5/5 and vertical spans5/5. The world overview's zoom−14 internal diamond is8×4, with first source coordinate `(1,1)` and spans40/40 across,39/39 down in its320×152 backing. Those mesh first coordinates must not be substituted for the overview's normalized click-coordinate origin below.

## Overview layout, origin, and resampling request

Coordinates in this section are **local to the minimap window**, whose console rectangle is`[780,88,1019,207)`. Add that rectangle's origin for console pixels, then add511 to y for1024×768 screen pixels.

With no positive global scale override, for the tested even world dimensions:

- Source width=`4*worldWidth`.
- Source height=`4*(worldHeight/2-2)`.
- If **both** window dimensions fit that source, center the source without scaling using integer half-difference offsets.
- Otherwise request scaling of the entire source into the entire window and use zero offsets. This is not a uniform aspect-preserving fit.

| World | Window | Source | Result |
|---|---|---|---|
| 80×80 | 239×119 | 320×152 | Native bitmap-scale request320×152→239×119; offset0,0 |
| 80×80 | 400×200 | 320×152 | Unscaled image centered at offset40,24 |

The normalized world origin is x=0, with the wrapped seam at0/worldWidth. Overview mapping does not rotate its origin to the selected faction, active camera, or selected unit. The overview's retained center stays `(40,40)` while the tested active camera changes. Whole framebuffer sampling and resampler pixel-edge parity remain outside these request receipts.

## Pixel-to-tile and tile-to-pixel

Use native signed integer division, truncating toward zero. Let `(ox,oy)` be the producer's image offset and `(iw,ih)` be the image dimensions used for mapping: source dimensions when padded, window dimensions when stretched.

For a click:

1. When either padding offset is nonzero, reject pixels outside the half-open image rectangle, then subtract the offsets.
2. Compute `x=trunc(worldWidth*pixelX/iw)` and `y=trunc(worldHeight*pixelY/ih)`.
3. Replace **y's low bit with x's low bit**: `y=(y & ~1)|(x & 1)`.
4. Reject coordinates outside the world.

The zero-offset path has no separate pixel-boundary check before division. A diagnostic pixel`(-1,0)` therefore truncates to tile`(0,0)` and is accepted by this helper; normal UI window hit testing is a separate boundary. At `(239,118)` in the239×119 window, the helper rejects the computed x80. Do not use stale output values after a failed helper call.

The reverse helper wraps x **once** on a round world, clamps y to0..height−1, rejects any still-invalid x, and returns `offset + trunc(imageDimension*coordinate/worldDimension)`. Flat maps do not wrap x. It does not impose tile parity on its input.

| Context/input | Native result |
|---|---|
| 239×119 click `(119,59)` | Tile`(39,39)` |
| 239×119 tile`(40,40)` | Pixel`(119,59)` |
| 239×119 click `(238,118)` | Tile`(79,79)` |
| Round-world tile`(-1,40)` | Pixel`(236,59)` |
| Round-world tile`(80,40)` | Pixel`(0,59)` |
| Round-world tile`(-81,40)` | Rejected; no repeated modulo normalization |
| Tile`(40,-1)` / `(40,80)` | Pixel`(119,0)` / `(119,117)` |
| Padded400×200 click `(40,24)` / `(359,175)` | Tile`(0,0)` / `(79,79)` |
| Padded click `(39,24)` / `(360,175)` | Rejected |

These helpers are deliberately not exact inverses after integer scaling and parity adjustment. Implementing floating nearest-tile rounding changes the center-click case.

## Click action and camera changes

An ordinary accepted overview click calls the native recenter routine on the **active view** with the mapped tile and redraw request1. In the tested center click, the active center becomes`(39,39)` and overview mode remains selected. An outside click causes no recenter or view switch.

The native Shift path differs: after validating the overview click, it recenters the **detail view on the current active-view center**, selects Detail Map, shows detail, and hides overview. The receipt clicks the same pixel mapping to`(39,39)`, while the active center is`(22,30)`; the resulting detail center is`(22,30)`, and the active center remains unchanged. This is observed with the original Shift-key check reporting held, not an invented click outcome.

The actual recenter routine executes and stores the centers in these cases; only display cleanup/redraw requests are sinks. Right-click menu construction, drag cadence, keyboard zoom, and enlarged-window behavior remain outside this slice.

## Viewport outline and seam

The overview outline uses the view referenced by the native **detail-view pointer**, not an independently invented fixed rectangle. From that view's computed first coordinate and four spans, the endpoints are:

`first` and `first + (evenXSpan+oddXSpan, upperYSpan+lowerYSpan)`.

Map both endpoints through the reverse helper. If either fails, omit this outline. When projected x1≤x2, submit one half-open frame rectangle. When x1>x2, submit separate line segments at the left and right image edges. Both branches use palette index251, RGB`(255,255,0)`, in the tested enabled phase. Exact inclusive line endpoints, including their asymmetric one-pixel corner adjustments, are in the cases/reference; do not replace them with a full-width rectangle.

| Detail center | Native first coordinate | Outline |
|---|---|---|
| `(40,40)` | `(35,35)` | Frame`[104,52,134,66)` |
| `(0,40)`, round world | `(75,35)` | Six segments split across the left/right seam |
| `(79,31)`, round world | `(74,26)` | Six segments split across the seam |
| `(0,40)`, flat world | `(-3,35)` | Omitted because the reverse helper rejects the negative endpoint |
| `(40,0)` | `(34,-2)` | Frame`[101,0,131,11)` |
| `(39,79)` | `(34,74)` | Frame`[101,110,131,117)` |

This does not establish outline blink timing. Native view geometry performs pole/flat-edge clamps and parity adjustment; the named reference models the tested ordinary overview/detail domain and explicitly excludes tiny-map centering and global scale overrides.

## Map color belongs to the loaded native slot

The native default primary map-color table is indexed by **loaded faction slot1..7**, not faction filename, leader name, occupied transport order, or a newly assigned human-player number. The [setup transfer receipt](setup-wizard-choice-transfer.md) already proves that choosing a human faction preserves its loaded slot into startup.

The new discriminator swaps Alpha's FACTIONS rows1/3 **in the emulated file stream only**, then executes the complete original list loader, all seven faction text parsers, faction-PCX filename selection, actual PCX decoding, and palette-preserving graphics loads. Map-color table and active-palette writes are watched. Both remain unchanged. Actor material/cache construction and sprite storage are declared presentation boundaries, not substituted map-color decisions.

| Loaded native slot | Default faction at that slot | Custom swapped faction | Map index / RGB after custom load |
|---|---|---|---|
| 1 | Gaians | University | 241 / `(0,235,0)` |
| 2 | Hive | Hive | 252 / `(0,0,255)` |
| 3 | University | Gaians | 255 / `(255,255,255)` |
| 4 | Morgan | Morgan | 251 / `(255,255,0)` |
| 5 | Spartans | Spartans | 65 / `(0,0,0)` |
| 6 | Believers | Believers | 102 / `(235,92,0)` |
| 7 | Peacekeepers | Peacekeepers | 235 / `(164,176,232)` |

Full native political tile calls then paint custom-slot University1 green, Gaians3 white, and Spartans5 black. This distinguishes map-slot colors from faction art/text/vehicle tints. It also disproves assigning University white solely by filename when a custom loaded order is in use.

For the port, preserve the native loaded slot through the native-owner→simulation-ID mapping. For example, **default University native slot3 mapped to GLSMAC simulation1 still uses map index255/white**. University explicitly loaded in native slot1 instead uses241/green. Carry a source map-slot or equivalent assigned map-color entry on the corresponding faction through creation/import/restore; that is an integration consequence of the source, not a new native structure field. If source slot provenance is absent, dense simulation ID and filename alone cannot recover custom-order colors reliably.

## Observer distinction

The original coarse renderer's general exploration admission uses its renderer viewer argument. The political eligibility/territory query in this path uses the **current display/focus faction** separately. Normal UI callers should supply consistent observer context.

One additional native discriminator uses renderer viewer3, display faction1, a tile explored by both1 and3, and own claim1 in the custom order. It produces slot1 green. Do not pass the claim owner as an observer merely to force ownership colors; preserve the separate observer and owner roles already used by the engine's knowledge/recognition logic.

## Remaining boundaries

Four-button reference identity/bindings, an original UI command selecting the political/rainfall flags, mode save/load lifecycle, zoom/drag/context-menu policy, tiny-world centering, and whole minimap raster/resampler parity remain open. Source assets and production rendering are unchanged; the broader research goal stays paused.
