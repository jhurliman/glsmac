# Base map-name font and spacing

Stock SMAC, 2026-10-19. [Seven native cases](base-map-name-font-cases.json), [private source map](../re-smac-base-map-name-font.md). This is the **name beneath a Base on the gameplay map**, separate from the population badge and the Base-inspector/console title. No production code changed. The completed [terrain geometry contract](ordinary-terrain-projection.md) and frozen evidence are preserved.

## Font and pen advance

At zoom0, the original Base-name caller requests **Arial, regular, logical pixel height−12**. LOGFONT weight0 resolves to400 on the pinned current Windows font. Width/rotation/italic/underline/strikeout are0; charset0/ANSI, output precision7/TrueType-only, clip precision0, quality0/DEFAULT_QUALITY, pitch/family0. It is not an Arial Narrow48 glyph sprite scaled down. Current metrics: height15, ascent12, descent3, internal leading3, external leading0.

Original measurement uses `GetTextExtentPoint32A` for the whole plain name, and original painting uses one whole-string `TextOutA` for its shadow followed by one for its foreground. There is no per-character left-bearing addition in this path. The measured widths equal the sum of individual advance widths for every sample:

| Plain name / control | Width in pixels | Individual advances |
| --- | ---: | --- |
| New Austin | **62** | 9,7,9,3,7,7,7,3,3,7 |
| New Chicago | **74** | 9,7,9,3,9,7,3,6,7,7,7 |
| Hilton Head | **65** | 9,3,3,3,7,7,3,9,7,7,7 |
| iiiWWW | **42** | 3,3,3,11,11,11 |
| jifj | **12** | 3,3,3,3 |
| AV To Wa | **52** | 7,7,3,7,7,3,11,7 |

The supplementary current-GDI ABC measurements on an indexed DC distinguish bearings from advance:

| Character | A: left bearing | B: reported black-box width | C: right bearing | Total advance |
| --- | ---: | ---: | ---: | ---: |
| i | 1 | 1 | 1 | **3** |
| W | 0 | 11 | 0 | **11** |
| j | −1 | 3 | 1 | **3** |
| f | 0 | 4 | −1 | **3** |

**Advance is A+B+C.** If an atlas already stores the total advance, add the left bearing only when positioning the glyph's ink quad, not again when advancing the pen. Negative bearings/overhangs remain valid. API black-box metrics and actual raster ink bounds are recorded separately; do not center a name using its cropped ink width.

For `iiiWWW`, cumulative pen advances are3,6,9,20,31,42. Adding the i bearing again would place the first W three pixels too far right. For `jifj`, all four pen increments remain3 despite differing bearings and ink widths.

The kerning control is deliberately discriminating: this host font advertises AV=−1 and To=−1, but the native name measurement/draw path does **not** apply those adjustments. `AV` advances14, and `To` advances14. The whole control is52, not50. This establishes the checked plain Latin-name behavior; it is not a general Unicode shaping contract.

## Baseline and centering

The caller derives the terrain-aware tile anchor from the original map projector, centers the label at `anchorX+horizontalCoordinateStep`, and places its logical y at `anchorY+floor(7*halfHeightUnit/2)`. At zoom0 the additions are48 and42 respectively. The text primitive then adds **ascent−internalLeading=9** to produce the GDI baseline.

Thus for a flat tile with projected center `(cx,cy)` and box anchor `(cx−48,cy−24)`, the label's baseline is **cy+27**. With terrain displacement, preserve the actual projector anchor; the name is not independently positioned using the tile's center height.

Horizontal foreground origin is `centerX-floor(measuredWidth/2)`. `Hilton Head` demonstrates odd-width handling: width65 subtracts32, not33. Placement uses baseline alignment (`SetTextAlign=24`) and transparent background (`SetBkMode=1`). There is no font-size fitting or glyph-sprite downscale in this checked caller.

The supplied1024×768 offscreen receipts use a1024×511 map bitmap, flat projection backing and camera8,8. Each name is on a different declared Base row to make the composite readable. Native foreground calls are:

| Name | Base coordinate | Foreground x / baseline |
| --- | --- | --- |
| New Austin | 8,8 | 545 / 267 |
| New Chicago | 8,10 | 539 / 315 |
| Hilton Head | 8,12 | 544 / 363 |
| iiiWWW | 8,14 | 555 / 411 |
| jifj | 8,16 | 570 / 459 |
| AV To Wa | 8,18 | 550 / 507 |

These are controlled inputs, not the original game's placement of those named cities. The fixture map is80×40 with flat height displacement; no installed Planet/Base founding sequence is implied.

## Color, shadow and raster

The name uses faction **text** foreground/shadow metadata. Normal painting draws the shadow first at foreground origin **(+1,+1)**, then foreground. This is a single offset shadow, not a multi-pixel outline. University controls use foreground index155, RGB **(216,224,235)** / `#d8e0eb`, and shadow83, RGB **(27,33,31)** / `#1b211f`. The Spartan control uses foreground75, RGB **(137,166,166)**, and shadow65, black. These are gamma100 values.

The original palette metadata source selects these remaining stock colors; actual name-paint execution here covers University and Spartans:

| Filename | Foreground index / RGB | Shadow index / RGB |
| --- | --- | --- |
| GAIANS | 242 / (16,228,0) | 37 / (21,41,24) |
| HIVE | 218 / (0,97,235) | 198 / (12,12,16) |
| UNIV | 155 / (216,224,235) | 83 / (27,33,31) |
| MORGAN | 251 / (255,255,0) | 135 / (62,41,13) |
| SPARTANS | 75 / (137,166,166) | 65 / (0,0,0) |
| BELIEVE | 98 / (224,156,28) | 135 / (62,41,13) |
| PEACE | 235 / (164,176,232) | 192 / (14,32,59) |

Native calls use indexed DIBINDEX colors. Current-Windows GDI replay on the indexed surface produces only background, foreground and shadow indices, with no intermediate antialiasing shades. Quality0 is a default-quality request, not an explicit NONANTIALIASED_QUALITY flag; retain the distinction from the raster observation and the [prior font contract](lower-console-faction-text.md).

The raster replay preserves the1024×511 bitmap bounds inside its1024×768 diagnostic canvas. The background is a solid palette80, not terrain. Eight private PNGs comprise seven individual cases plus the six-string composite. They are lossless **current-Windows offscreen GDI receipts**, not live original framebuffers or historical Arial pixel parity. Current Arial file hash, raw LOGFONT/TEXTMETRIC, cumulative advances, ABC data, actual native calls and PNG hashes are pinned in the frozen evidence.

## Validation boundary

Seven original map-label prefixes reach the return of their actual centered name draw; complete font initialization, projection, width and text routines execute. Fourteen actual TextOutA calls verify order and coordinates. The prefix stops before optional Base status/production/debug-label continuation. Supplied Base names/owners/view flags and ordinary known-owned state do not establish all foreign visibility, status suffixes or editor overrides.

Use these native advance metrics and regular Arial request to assess the spacing repair. Other font files/rasterizers can still differ at pixel level and need a separately labeled comparison. Frozen bundle: `tmp/reference/base-map-name-font-evidence-20261019`. No owned font/PCX payload or PNG is placed in Git. Broad RE remains paused.
