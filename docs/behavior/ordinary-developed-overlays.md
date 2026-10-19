# Ordinary developed-tile overlays

This closes the zoom0 ordinary tile prefix through the common improvements,
before the named-landmark sprite tail. The native caller, installed rule loading,
TER1 extraction, nutrient query and indexed sprite routines execute. The
[cases](ordinary-developed-overlays-cases.json) and
[portable reference](developed_overlay_reference.py) contain 60 producer contexts,
286 polygon commands, 81 sprite commands and 64 complete indexed prefix frames.
All 6,042 polygon spans, 290,679 pixel attempts and 224,072 stores match, including
four clipped replays. These are controlled offscreen frames, not live screenshots.

## Authority and ordering

The viewer-selected item word admits the sprites: the known viewer uses its
remembered items, while the omniscient path selects live items. Never silently
replace the selected word with current terrain items. Solar live-only and
remembered-only controls demonstrate the difference.

Kelp is an earlier exception: selected Farm on any sea band draws Kelp under
terrain flag `1`, before Road/Tube. Its caller has no zoom gate. The following
common group requires improvement flag `0x20` and zoom greater than -8, in order:

| Order | Selected bit | Sprite | Additional condition |
| --- | --- | --- | --- |
| 1 | `0x800` | Bunker | Land |
| 2 | `0x40000` | Airbase | Land |
| 3 | `0x80000000` | Sensor | Land or sea |
| 4 | `0x8000` | Farm / Soil Farm | Land; nutrient tier below |
| 5 | `0x10` | Mine / Mining Platform | Land / sea band2 only |
| 6 | `0x40` | Solar / sea Solar | Land / sea band2 only |
| 7 | `0x400000` | Condenser | No land/sea gate |
| 8 | `0x800000` | Echelon Mirror | No land/sea gate |
| 9 | `0x1000000` | Borehole | No land/sea gate |
| 10 | `0x88000` together | Extra Soil patch | Land and nutrients >=5 |

Condenser, Mirror, Borehole and Sensor retain these permissive gates even on
synthetic combinations a normal terraforming command would not create. Do not
infer rendering admission from whether the game permits construction there.
Soil alone draws no Soil sprite. The Borehole variant uses the **live** shared
word: `(word & 0x80001000) == 0x1000` selects the first variant below. Setting
the high suppression bit selects the other variant.

When selected Farm and flag `0x20` are present at zoom >-8, the caller evaluates
[ordinary tile nutrients](ordinary-tile-nutrients.md) with the resolved faction,
base `-1`, tile coordinates and mode0. Flag `0x1000` can independently request
this query. Its result reads **live** terrain, items, technology and resource
state; it is not computed from remembered Farm/Soil bits. The evaluating faction
comes from the renderer viewer, resolved through tile byte5 low bits and the
global viewer when those low bits are zero.

Farm tier is `clamp(nutrients-1,0,3)`. Selected Soil chooses the Soil Farm bank;
otherwise the ordinary Farm bank. A result >=5 appends Soil tier0 after Borehole,
in addition to the initially selected Soil tier3. Rainy Farm after nutrient-cap
technology yields3; with Soil yields4; with Soil and Condenser yields6. Controls
where live and remembered Soil disagree preserve this two-source behavior.
The reference receives a resolved nutrient value explicitly; it does not replace
the already documented nutrient engine.

## TER1 crops and projection

All crops are `(x,y,100,62)` from the externally supplied original TER1.PCX.
Load index bytes with `+10 mod256`, substitute loaded indices6 and7 with key9,
then trim to the non-key bounding box. The cases include the resulting retained
offset, size and hash for each crop; no original asset bytes are distributed.

| Sprite | Crop origin |
| --- | --- |
| Bunker / Airbase / Sensor | `(506,316)` / `(607,316)` / `(708,316)` |
| Farm tiers0..3 | `(923,453+63*tier)` |
| Soil Farm tiers0..3 | `(822,453+63*tier)` |
| Mine / Solar | `(607,64)` / `(607,127)` |
| Mining Platform / sea Solar | `(506,64)` / `(506,127)` |
| Condenser / Mirror | `(506,253)` / `(607,253)` |
| Borehole first / other | `(708,190)` / `(708,253)` |
| Kelp | `(607,190)` |

Sprites use the native terrain **projector anchor**, not the flat tile center.
At the measured zoom0 viewport, flat center `(cx,cy)` gives
`P=(cx-48, cy-24-floor(sum(outerHeights)/4))`.
Place the retained sprite at `P + retainedOffset`; it is an axis-aligned bitmap,
not a terrain-warped quad. Raised and coastal controls verify the anchor shift.
Sprites receive no terrain light adjustment. Their overlap follows the order
above, after the existing terrain, coast, River and Road/Tube prefix. Separate
Kelp commands belong before Road/Tube, even though sea Road itself is rejected.

## Indexed pixels and Sensor shadows

The plain sprite routine skips key9 and copies every other source byte literally,
including source0. Sensor uses the shadow routine: key9 skips; source0 replaces
the existing destination index through the global 256-entry shadow lookup; all
other source bytes copy literally. Clip left/top inclusively and right/bottom
exclusively. Clipping retains the original source coordinates.

The lookup is generated from the **active game palette**, independently of the
sprite PCX palette. All 256 destination indices are mapped; nearest candidates
are game indices10..245, first/lower index wins a tie, with no protected ranges
in the measured stock constructor. The generator multiplies value by0.57 and
finds nearest in cylindrical hue/saturation/value distance.

Preserve the original conversion quirk. Normalize RGB by256. Let `v=max(r,g,b)`;
use `low=b` when `b<v`, otherwise `low=min(r,g)`. Then
`s=(v-low)/v`, or0 when v0. For s0, hue=-1. Otherwise use the ordinary dominant
channel hue formula with denominator **v-low**, multiply by60, and add360 once
if negative. Hue can exceed360. Distance is squared Euclidean distance between
`(s*cos(h*pi/180),s*sin(h*pi/180),v)`. Using conventional HSV or RGB nearest
does not reproduce this lookup. The reference matches every native lookup entry.

The earlier Base downscale fixture used an explicitly zero-filled lookup. It
remains valid for that fixture and is not evidence of the initialized game lookup.

## Bounds

Zoom0, viewer3, known synthetic retained tiles, default relief, palette gamma100
and the 1024x511 indexed viewport are measured here. Zoom gates above are traced
from the caller; this work does not establish scaled improvement pixel parity.
Resource/Monolith display under flag `0x200`, named-landmark art, fog, actor/cache
repaint scheduling and full renderer return remain separate continuations.
Full prefix receipt ends at a documented instruction boundary, not game UI return.

## Port composition checkpoint

`SmacDevelopedOverlay` now implements the measured selector, loaded TER1 trim
and active-palette shadow lookup. `SmacTerrainRaster::DrawBatch` accepts ordered
plain and shadow sprite commands alongside its existing terrain commands, with
one destination allocation and owned result. Source bitmaps and shadow tables
are borrowed only for that synchronous call. Key9 skips before any shadow
lookup; a plain source0 remains an opaque literal.

Run the production C++ replay with purchased assets:

```
python3 tools/reference/verify_native_developed_overlays.py --asset-directory PATH --cxx clang++
```

Add `--sanitize` for the same64 full-frame hashes under ASan/UBSan. The verifier
checks all60 selector contexts, loaded crop offsets/sizes/hashes, all256 shadow
entries, and frame attempt/store counts. Native engine groups additionally
exercise empty trim, extreme nutrient values, negative-origin clipping, lookup
validation and coverage of literal0 versus key9.

This is the composition layer. The live inspector has not yet replaced its
legacy improvement sprites: it still needs the complete remembered-word copy,
live base-less nutrient query, TER1 source/cache binding and coordinated legacy
instance visibility. Merely admitting improved terrain while both sprite paths
remain active would double-render the art. Those caller changes are the next
integration slice; this checkpoint does not claim new live screenshot parity.
