# RE-SMAC-PREFERENCES-PIXELS-001: page0 checkbox and common controls

2026-10-21. Original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This adds a controlled **1024×768 original Window-tree composite**, checked
and unchecked ID8 row rasters, exact sprite extraction and visible font
bindings to the [Preferences behavior handoff](preferences-ui-and-automatic-design.md).
It does not repeat the67-control inventory or preference-bit matrices.

[Scalar receipts](preferences-page0-pixels-cases.json) and the self-authored
[owned-asset extraction/geometry reference](preferences_checkbox_reference.py)
contain no original sprite pixels or font payloads. Complete images, indexed
buffers, row masks and original instruction receipts remain private in
`tmp/reference/independent-preferences-page0-pixels-evidence-20261021/`.

## Row geometry and hit area

The actual main checklist Window is `(256,10)-(713,386)` in the Preferences
root, with a457×376 client bitmap, pitch460. Its screen origin is **(418,160)**
for root origin(162,150). The following coordinates are checklist-local.

For ordinal `i=0..7`, native row spacing is **24**: minimum row height18,
gap6. There is one visible column. The native visible column width is451,
with another6 pixels in the horizontal step, making457.

| Element | Original coordinates |
| --- | --- |
| Row hit band | `(0,24*i)-(457,24*i+24)` |
| Checkbox tile | `(0,24*i+3)-(18,24*i+21)` |
| Native text request to `5f4b00` | x18, y`24*i+7` |
| Actual GDI text baseline | `24*i+19` |
| Label character origin after its literal leading space | x23 |

For **ID8 / ordinal1**, this means hit band y24..47, sprite at(0,27), text
request(18,31), baseline43. The corresponding screen sprite rectangle is
**(418,187)-(436,205)** and screen baseline203. The original string includes
one leading space. Its5-pixel advance is additional to the18-pixel icon
column; do not remove it and retain the same text anchor accidentally.

`629080` computes the item dimensions; `6293a0` paints a row.
The font's native `+10` baseline increment is12, despite its16-pixel nominal
request and20-pixel GDI text height. `5f4b00` adds this12 to the supplied
y31 to obtain baseline43. Using GDI ascent16 in that addition shifts the
label four pixels downward.

The native receiver `629a50(x,y)` divides by the horizontal and vertical
steps; it does not restrict toggling to the icon or the text ink. Twelve
direct native samples confirm x0,17,18,300,456 on y24 all toggle ID8, as do
y24 and47; y23 toggles ID7 and y48 toggles ID9. Thus the6-pixel gap belongs
to the preceding hit band. These are inputs within the actual child Window
bounds, not hypothetical clicks outside it.

There is an original helper quirk below the eight rows: `624010` rejects
an out-of-range ordinal without clearing its current linked-list cursor.
Consequently the direct receiver can toggle that current item. With the
cursor left on ID14 by the reset/paint sequence, y192 and375 toggle ID14;
y191 does too as the last populated band. This receipt does not claim an
OS mouse-message experiment. If preserving this quirk, preserve its cursor
dependency; it is not a general rule mapping every blank row to ID14.

## Exact checked and unchecked art

The producer is `460720@460dba..460f36`. It loads **interface.pcx** into
`7c9c48`, replaces loaded palette index7 with9 and6 with9 through `5f7380`,
then calls `5fc7d0` three times with flags/key operand `0x109`, width18,
height18. Original PCX loading into10..245 first adds10 to source indices,
with byte wrapping; apply the replacements **after** that remap.

| Sprite | Pointer | Source crop x/y/w/h |
| --- | --- | --- |
| First tile, not selected by this checkbox pair | `7c68a8` | 227,1,18,18 |
| Unchecked | **`7c68d4`** | **227,20,18,18** |
| Checked | **`7c6900`** | **227,39,18,18** |

The Preferences constructor explicitly assigns the second and third
sprites to the group. Do not use the first tile as the unchecked state.
`6293a0@62970f..62979e` chooses `+10` unchecked / `+14` checked, vertically
centers it in the actual painted24-pixel line, and invokes `5fd990` with
target, key9, x0, y27, scale1, scale1 for ID8.

All three native unpacked18×18 buffers match independently decoded and
remapped owned-PCX crops byte-for-byte. All324 pixels in each of these
particular tiles are non-key pixels; do not invent rounded transparency,
a vector tick, or a theme-colored outline. Native key9 handling still applies
to the sprite primitive generally.

Changing ID8 changes exactly **324 pixels** in both the full-screen composite
and the457×24 row crop. Every changed full-frame pixel lies in the18×18
rectangle above. Text requests, glyph pixels, other rows, buttons and frame
pixels remain identical. The row's label mask has737 foreground pixels,
ink bounds `(24,8)-(205,22)` within the cropped457×24 row. The unchanged
background/label area uses indices65,196 and224; native224 is the label ink.

## Actual visible fonts and colors

With stock MainFontSize16 at1024, the main labels use **Arial Narrow,
LOGFONT height−16, weight700**, no italic or underline. The actual font object
is `94f0c8`; its native metrics are height20, baseline increment12,
internal leading4, ascent16, descent4. Both ID8 states use this same font,
foreground **palette index224** (active RGB120,164,212), shadow−1/no shadow.
The active selected font data hash is recorded in the JSON. The bundled
regular `arialn.ttf` is registered privately before GDI creates the bold face;
the resolved face was queried and confirmed as Arial Narrow.

The ten visible buttons use **Arial, height−12, weight0/default**, through
font object **`7f0968`**, not the checkbox font or the Preferences constructor's
unused reserved10/12/14 bank slots. Actual normal-state captions use
foreground **index0** (active RGB0,0,0), shadow−1; the native constructor
explicitly supplies zero colors. The measured caption coordinates are
button-local, baseline14:

| ID | Caption | x |
| --- | --- | --- |
| 0 | Preferences | 73 |
| 1 | Adv. Preferences | 61 |
| 2 | Warning Preferences | 49 |
| 3 | Automation Preferences | 41 |
| 4 | Audio/Visual Preferences | 38 |
| 5 | Map Preferences | 60 |
| 6 | Gamma Correction | 54 |
| 90 | Restore Defaults | 60 |
| 92 | OK | 98 |
| 91 | Cancel | 87 |

The native GDI color operands are **`0x10ff00e0`** for labels and
**`0x10ff0000`** for these captions: indexed-DIB color encoding, not RGB
values or ordinary PALETTEINDEX flags. In the captured row, glyphs write
index224 directly; there are no intermediate antialiasing shades. Do not
interpret that color operand as magenta or synthesize gray alpha pixels.
These are normal/closed-frame bindings; no new hover/pressed-state matrix
is claimed.

## Admission, startup correction and reproducibility

The guest executes the original map-parent construction/show, complete
Console construction, Preferences constructor, open painting and `6101a0`
Window-tree composition. The map backdrop is deliberately blank. The stock
profile defaults are supplied from an empty in-memory profile, and original
profile writes are suppressed. There is no OS window or real input pump.

The reused UI harness initially retained the library bootstrap Times New
Roman fallback at7f0968. **That diagnostic is not the accepted visual target.**
Original startup `46c339..46c34d` supplies face`[6affac]` (Arial), size12,
flags0 through `630600`; after the already-private font registration, the
receipt executes its final native `630450(Arial,12,0)` construction, rebuilds
Preferences and captures again. The accepted button font was queried as
Arial. This correction avoids exporting fallback-font button widths as rules.

Two ID8 whole-frame captures, two complete row crops, three independent
sprite-crop matches, twelve native hit samples and ten actual button paints
are retained. Whole-frame checked/unchecked SHA-256 values and row/font
receipts are in the JSON. A fresh guest replay is compared against the same
accepted whole-frame hashes. These establish one controlled closed frame
and its ID8 states; OS modal delivery, campaign backdrop, alternate display
sizes, alternate font settings and Alt+F4 are outside this packet.

## Port control receipt

The shared `smac_checkbox.gls.js` row and `Surface.smac_checkbox` producer
implement this1024/MainFontSize16 control. Both18×18 indexed buffers match
the independent original unpacked hashes. The source6/7 replacement stays
local to checkbox extraction, preserving the shared frame atlas.

An actual SDL/OpenGL software-rendered UI harness paints the production row
in copied UI data. Both complete **457×24 ID8 rows match all10,968 RGB
pixels** in the checked and unchecked original captures. A real click at the
right-hand blank space toggles the control, changing exactly324 framebuffer
pixels. The process exits cleanly. Use
`python docs/behavior/preferences_row_pixel_gate.py ORIGINAL.png PORT.png`
with native1024×768 frames and matching states to repeat that comparison.

This is a control harness, not the normal Preferences route: the shared
window, other option effects, page changes and staged acceptance remain to
be connected. No whole-modal parity,800-width or alternate font-setting
parity is claimed. The row setter changes appearance without invoking its
user-edit callback, so reset/reopen can retain the eventual modal's draft
semantics.
