# Gameplay Unit and Base population overlays

Pinned stock SMAC, 2026-10-18. [Compact cases](map-unit-and-population-overlays-cases.json), [private execution/source map](../re-smac-map-unit-and-population-overlays.md). No gameplay code changed. Existing [blink/overlay timing](selected-unit-animation-port-handoff.md) remains authoritative; its timer tests were not repeated.

## Order and status glyphs

**H means the retained Hold order.** The original installed `#ORDERS` loader stores order2→`Hold`/`H`, and the actual Unit draw routine fetches that glyph through its original string table. It is neither a screenshot inference nor the Sentry order.

| Order | Meaning / displayed glyph before status overrides |
| --- | --- |
| 0 | No Orders: `-` |
| 1 | Sentry/Board Transport: `L` |
| 2 | Hold: `H` |
| 3 | Convoy: `O` |
| 4..8 | Farm `f`, Soil Enricher `f`, Mine `M`, Solar Collector `S`, Forest `F` |
| 9..13 | Road `R`, Mag Tube `R`, Bunker `K`, Airbase `.`, Sensor Array `O` |
| 14..19 | Remove Fungus `F`, Plant Fungus `F`, Condenser `N`, Echelon Mirror `E`, Thermal Borehole `B`, Aquifer `Q` |
| 20..23 | Raise Land `]`, Lower Land `[`, Level `_`, Monolith `?` |
| 24..28 | Move to `G`, Move `>`, Explore `/`, Road to `r`, Tube to `t` |
| 29 | Empty glyph in this installed table |

The renderer duplicates a leading `[` or `]` in its markup buffer to escape the text parser; actual text painting receives **one visible bracket**. It interprets the order byte as signed and clamps to0..29: diagnostic255 selects0/`-`,30 selects29/empty. Terraform labels/glyphs were loaded by original code; command admission/work completion for those orders is outside this receipt.

Status can replace the glyph or color. Executed discriminators retain raw masks in the fixture: mask`0x4000` replaces the ordinary glyph with `/`; combined`0x1000200` selects `P`; mask`0x200` can turn No Orders into `a` and uses the faction secondary text color. With that mask, retained automation selectors11/10 select `*`/`B`. These selector producers and broader semantic names were not traced here. Additional status-color branches exist. In the tested ordinary human-viewer foreign-Unit case, its Hold glyph is hidden as `-`. Do not publish the retained enemy order merely because it is available to the engine.

For a University Unit with badge anchor `(x,y)`, zoom0, supplied displayed morale1 and remaining moves, original badge selection is owner3/level1/fresh variant. Health/order positions share that anchor. The order text box is `[x+10,y+5,x+21,y+19)`, centered by native text layout. It requests **Arial, bold700, logical pixel height−12**, transparent background and no shadow in this ordinary case. Current Windows metrics give text width8 for `H`, and original placement is `(x+11,y+7)`, baseline`y+16`. The font's native sizing field is ascent minus internal leading (9 here), not its15-pixel line height. Exhausted-movement input selects the separate spent badge variant; it does not turn `H` into another order.

## Health bar

At zoom0 the original paints black `[x+6,y+6,x+8,y+27)`, then paints the remaining-health segment from its **top**. For ordinary maximum health `H`, filled height is integer `21*(H-damage)/H`, truncated, after remaining-health guards. The unfilled bottom remains black; the surrounding flag artwork is a separate sprite.

| Fission maximum10, damage | Filled height | Index / RGB |
| --- | --- | --- |
| 0 / 2 | 21 / 16 | 250, `(0,255,0)` |
| 3 / 5 | 14 / 10 | 251, `(255,255,0)` |
| 6 / 9 | 8 / 2 | 249, `(255,0,0)` |
| 10 | No colored fill | Black backing remains in this diagnostic draw |

For ordinary reactor health, green applies at damage below `floor(H/3)` (also the explicit zero-damage case), yellow below `floor(2H/3)`, red otherwise. Maximum health is10 times reactor clamped1..100; the new controls also exercise reactor2,0 and255. A source-only special design-plan branch uses maximum1 and is not a newly validated ordinary-Unit rule.

Badge, bar and order text are one overlay group. Reuse the prior selected-Unit blink contract: its admitted off phase can keep the body while hiding all three; Whole Unit Blinks/near-mouse paths can instead omit the body too. The new draw-mask controls confirm overlay suppression and its explicit force path, not a new blink timer or full fog/zoom visibility policy.

## Base population9 and10

At zoom0 the font request is **Arial bold700, logical pixel height−16**. Current Windows metrics give digit width9 and native sizing height12. The original measures `8` for a one-digit box and `88` for population≥10, rather than measuring the actual population string.

| Population | Outer box relative to Base draw anchor `(x,y)` | Text start / baseline |
| --- | --- | --- |
| 9 | `[x,y,x+14,y+16)` | `(x+3,y+2)` / `y+14` |
| 10 | `[x,y,x+23,y+16)` | `(x+3,y+2)` / `y+14` |

Width is measured placeholder width+5; height is native font sizing height+4. The one-pixel frame and padding remain pixel constants as the requested font scales. The box stays left-anchored when becoming two digits. Its content is the original signed population byte converted to decimal; this is not a flexible arbitrary-digit layout.

Background/foreground depend on the admitted information/occupancy branch. Measured University cases:

- Owned, empty, no valid cached Unit marker: black outline, no new interior fill, faction population-text index155.
- Valid Unit marker plus cached occupant owner0..7, **including an owned occupied Base**: black outline, inset faction-primary fill (University255/white), text0/black.
- Tested foreign empty Base with no nearby viewer context: the same faction-filled style. Other friendly/knowledge branches remain conditional.

The filled interior is `[left+1,top+1,right-1,bottom-1)`. Spartan filename identity selects a different text-color case; do not hardcode black for every faction. All eight factions' metadata colors are in the fixture. Population drawing does not inherit the selected Unit's blink phase.

For the same tile, an executed map dispatch calls Base drawing first, then Unit drawing. Within Unit drawing the badge sprite precedes black health backing, colored fill, then order text. Within population drawing, frame/fill precedes digits. Thus a later Unit overlay can cover the Base badge where their rectangles overlap; this is not a separate always-on-top population layer. Neighbor-tile ordering and complete world occlusion were not audited.

## Captures and limits

Private lossless outputs: `tmp/reference/map-overlays-indexed-1024x768-20261018.png` plus `map-overlay-indexed-H-20261018.png`, `map-overlay-indexed-population9-20261018.png`, and `map-overlay-indexed-population10-20261018.png`. These are native4:3 **controlled offscreen composites**: original indexed badge/rectangle drawing, original font requests/layout to `TextOutA`, then current Windows GDI text on an8-bit DIB with the native palette. Models and live world/UI are absent; the background is a diagnostic solid index100. The current Arial Bold font hash is pinned. These are not live original-game screenshots or a claim of historical font-raster parity.

The supplied reference is1920×1080. A1024×768 origin would imply independent scales1.875×1.40625; that is a comparison hypothesis, not proof of original resolution or map zoom. Additional−4/−3/+4/+8 zoom controls and their actual font/box results are included; do not force zoom0's numbers onto an unmeasured reference zoom. No broad menu/HUD, SMAX/PRACX, full fog/stealth, native-body or live screenshot parity is claimed.
