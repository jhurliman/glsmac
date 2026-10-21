# Parent component and ability cards: implementation contract

Use owned DESWIN backing, the existing component renderers, and the text rules
below. The two ability cards are **text-only**: their ability CVRs belong to
the assembled main/strip model, not to these cards. Exact native draw operands,
glyph runs, font receipts and crop hashes are in the
[three-state cases](design-workshop-card-decoration-cases.json).

## Backing and model placement

Coordinates are parent-canvas-local; rectangles are half-open. Painter order:

| Card | Painter | Restore source = destination RECT | Existing model anchor |
| --- | --- | --- | --- |
| Chassis | `43de60` | `[30,7,129,92)` | `(79,47)`, flags 2 |
| Weapon | `43e2b0` | `[30,101,129,186)` | `(79,143)`, flags 6 |
| Shielding | `43e850` | `[30,195,129,280)` | `(79,235)`, flags 6; armor zero skips model |
| Reactor | `43ed10` | `[462,7,561,92)` | `(511,45)`, flags 6 |
| Ability 1 | `43f120` | `[462,101,561,186)` | No model/icon |
| Ability 2 | `43f4f0` | `[462,195,561,280)` | No model/icon |

Each painter sets that canvas clip and calls
`(Workshop+13c6c).5f23e0(Workshop+444,&RECT)`. The child copy is exactly
`5f8d00(destination,left,top,left,top,99,85)`: identical source/destination
coordinates in the cached `deswin.pcx` bitmap. It is not a cropped texture
stretched over a card. DESWIN is 623×368, decoded with index relocation +10,
key 9. The surrounding borders also come from DESWIN; see the
[parent skin recipe](design-workshop-parent-visual-target.md).

No alternate selected/unselected, hover or disabled skin is selected by these
six painter bodies. Empty slots still paint their heading and NONE text.
Chooser-grid selected borders/unavailable overlays are separate painters;
do not transfer them onto parent cards. The existing
[component composition](design-workshop-components.md) and
[Probe caller substitutions](design-workshop-stock-composition-families.md#probe-substitutions-are-scoped-to-each-painter)
remain authoritative for model content, scales, fixed yaw and CVR blits.

## Fonts, colors and ordinary Scout text

Retained card fonts resolve to **Arial**, not Arial Narrow:

| Role | LOGFONT height / weight | Observed logical wrap step | Foreground |
| --- | --- | ---: | --- |
| Headings | −12 / 700 | 14 | Palette 224 |
| Component names | −10 / 0 | 10 | Palette 222 |
| Component values; empty ability NONE | −10 / 0 | 10 | Palette 224 |
| Ability name-and-colon brace span | −10 / 0 | 10 | Palette 224 |
| Ability description | −10 / 0 | 10 | Palette 222 |

Glyph journals use `10ff00e0` / `10ff00de` respectively: palette operands with
transparent background, not RGB literals. The GDI metric heights are 15/13;
do not substitute those for the native 14/10 wrap steps. The earlier parent
summary's bold numeric-value font was incorrect; actual side-card value runs
use regular −10.

All four component headings use `5f4520` in `[left,top,right,top+14)`.
Their LABELS sources are Chassis `140`, Weapon `204` / Equipment `200`,
Shielding `1f8`, Reactor `13c` (hex table offsets). Zero weapon offense selects
Equipment; otherwise Weapon. Component names come from the installed chassis,
weapon, armor and reactor definition tokens; armor zero uses LABELS `64`, NONE.
Long chassis/weapon/armor names take their measured-width wrap branch; reactor
names always use measured wrapped height. Preserve that layout rather than
pinning every possible name to these short-name coordinates.

For ordinary Scout, exact `(x,baseline)` runs are:

| Card | Heading | Name | Value |
| --- | --- | --- | --- |
| Chassis | CHASSIS `(54,18)` | INFANTRY `(54,80)` | Land 1 `(63,90)` |
| Weapon | WEAPON `(53,112)` | HAND WEAPONS `(38,174)` | Weapon 1 `(55,184)` |
| Shielding | SHIELDING `(49,206)` | NONE `(65,268)` | Level 1 `(62,278)` |
| Reactor | REACTOR `(484,18)` | FISSION PLANT `(472,77)` | Power 1 `(492,87)` |

Value formatting, including Psi and armor-class omission, stays in the
[strength-text contract](design-workshop-strength-text.md). No Armor has no
model but still shows NONE and Level 1 in this parent card.

### Long component names: exact rectangles and baselines

All names use the full 99-pixel card width. Chassis, weapon and armor measure
with `630800` and choose the single centered line when **width <= 99**
(`43e0e4`, `43e5ba`, `43eb29`). On width >99 they shift the name rectangle
up **one logical line, 10 pixels**, call centered wrapping `5f68b0`, then
restore the original rectangle before drawing the value. They do not measure
the total wrapped height to reserve N lines. The value line stays fixed.

| Card | Short-name RECT; baseline | Two-line name RECT; baselines | Value RECT; baseline |
| --- | --- | --- | --- |
| Chassis | `[30,73,129,80)`; 80 | `[30,63,129,70)`; 70,80 | `[30,83,129,90)`; 90 |
| Weapon | `[30,167,129,174)`; 174 | `[30,157,129,164)`; 164,174 | `[30,177,129,184)`; 184 |
| Shielding | `[30,261,129,268)`; 268 | `[30,251,129,258)`; 258,268 | `[30,271,129,278)`; 278 |
| Reactor | `[462,70,561,77)`; 77 | `[462,60,561,67)`; 67,77 | `[462,80,561,87)`; 87 |

The table's small RECT bottoms are **alignment bounds**, not text clips.
Wrapped text continues below them; clipping remains the full card. Do not
clip a two-line name at the listed first baseline. Three or more lines in
the first three cards receive no extra reserved space and can overlap the
later value line; the source does not cap the wrapper at two lines.

Reactor is different: `43ef32` always measures wrapped height H with
`5f5160(name,99)`. For card bottom B, its name starts at `B-5-H-7`, nominal
RECT height seven. N ordinary lines have H=`10*N`, first baseline `B-5-H`,
and final baseline `B-15` (77 here). Its value baseline remains `B-5` (87).
The name's measured height is also reserved before the reactor model paint.

For the other three cards, short-name top is `B-5-2*7`, nominal height seven;
the wrap branch subtracts ten from top and bottom. Seven is the retained
font baseline offset; ten is its logical line step. Generic `5f5f30` centers
the short line horizontally using measured advance and vertically using the
font baseline-offset field; `5f68b0` starts each wrapped line at top plus
seven and advances ten. Use the pinned font metrics rather than GDI's total
13-pixel TEXTMETRIC height as the line step.

**Over-width words are not split into characters.** The width helper
`6308f0` searches for ASCII spaces (`63091d..630920`). A word that overflows
after earlier text moves whole to the next line. A first word already wider
than the available width is consumed whole (through the following space, or
to the end); the wrapper does not hyphenate or find a fitting character prefix.
The resulting over-wide line is centered and clipped by the outer card.
The helper's width tests treat exact fit as fitting.

This clarification is source-derived, with the four short-name rectangles
checked against the existing native journals; no new long-name raster is
claimed. [Formula/source receipts](design-workshop-component-name-wrap-cases.json)
and [self-authored layout reference](workshop_component_name_layout_reference.py)
are frozen in private
`tmp/reference/independent-workshop-component-name-wrap-evidence-20261021`,
manifest SHA256
`5bf9a845f6d9cd8a2613b819acc7f96b0cfbbd8bc9339c613f07b81296986786`.

## Ability slot text and inline decoration

Read signed indices at draft `14218` / `1421c`. Negative means empty. The
captures are `[-1,-1]`, `[5,-1]`, and `[4,5]` for no abilities, SAM, and
Drop+SAM. Caption is LABELS `228`, Special Ability, plus a space and literal
slot number 1/2. It is uppercased and centered/wrapped by `5f4a00`:

| Slot | Heading layout RECT | Heading glyph baselines | Empty body RECT / NONE position |
| --- | --- | --- | --- |
| 1 | `[467,103,556,115)` | SPECIAL at `(485,112)`; ABILITY 1 at `(485,126)` | `[462,131,561,186)` / `(497,162)` |
| 2 | `[467,197,556,209)` | SPECIAL at `(485,206)`; ABILITY 2 at `(485,220)` | `[462,225,561,280)` / `(497,256)` |

The heading helper returns 131/225 after wrapping. Its initial rectangle's
bottom is not a clipping boundary: the second line extends below it. The
outer card remains the actual clip. Empty text is centered with `5f5f30`.

For a populated slot, resolve ability name and description from
`9b6c68+28*index` and `+4`. Construct exactly:

```text
"{" + localizedName + ":  }" + localizedDescription
```

There are **two spaces after the colon**. Braces are inline color markup and
are not printed. Preserve original name/description case. Measure with
`5f5160(text,99)`, then draw with `5f68b0` in
`[left,bottom-measuredHeight,right,bottom)`. Center each complete wrapped line,
including its differently colored spans; do not center name and description
independently or put them in separate paragraphs.

Both sampled descriptions measure 20 pixels. SAM in slot one produces name
span `Air Superiority:  ` at `(473,173)`, description at `(475,183)`.
Drop in slot one produces `Drop Pods:  ` at `(467,173)` and `Makes ` at
`(524,173)` on the **same line**, then `air drops` at `(491,183)`.
SAM in slot two uses `(473,267)` / `(475,277)`. These distinguish inline
markup/wrapping from a title-plus-body widget. Additional cost/prerequisite
annotations from the ability chooser do not appear on these parent cards.

## Assets, fonts and evidence boundary

No missing original card artwork was found: use DESWIN and the existing owned
CVRs. The missing explicit font provenance is now supplied. GDI GetFontData
exactly matches installed `C:/Windows/Fonts/arial.ttf` for regular card text
(SHA256 `b3658eadae55e682b5f69eb64c439c1ecc8f196c0bb8d4756d145d13bc86476a`)
and `arialbd.ttf` for headings
(`e8f4e3baf6cc35fed6fcce3a540e8b39e8f6cda1d22a28f2ec8f526fef7a43f5`).
The owned game `arialn.ttf` stays registered throughout capture but is not
the resolved font for these runs. A Linux substitute is not a pixel-equality
receipt for these Windows GDI glyphs. Font bytes are not exported in public
evidence; the requests, metrics and resolved-font hashes are.

The later [Arial regular10 diagnostic receipt](design-workshop-arial10.md)
adds all95 printable ASCII advances/bearings/masks, cumulative description
extents and a direct foreground-mask match to the native SAM description.

Three new native 1024×768 Window-tree composites are retained, together with
their native parent canvases and all 18 card crops. The empty whole frame is
byte-identical to the prior Morgan-material Scout frame. Independent checks
verify draw/copy operands, glyph invariance for component text, 1,020 untouched
DESWIN margin pixels per frame, and exact equality of each card crop between
the parent canvas and final composite. They do **not** independently regenerate
every new full-parent pixel or compare port output. Captures are sequential
native paints with the existing shared model scene; a component model's pixel
changes are not evidence of selected-card decoration. Renderer kernels,
actions/buttons and chooser grids are not rerun as new validation matrices.

All three use the declared Morgan owner-one material from that prior parent
receipt, phase/yaw inputs zero, inert timers, and blank uncovered map context.
They are controlled native UI captures, not a naturally started Morgan campaign.
Private immutable bundle:
`tmp/reference/independent-workshop-card-decoration-evidence-20261021`,
manifest SHA256
`1465dd9cde013adc637fd5776d7b63ebb2441cafc49b2f645c4bd1d25fe1740b`.
