# Generic modal OK button: initialized sprite paint

2026-10-21, pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This corrects the OK-owned DIB reference in the earlier illustrated
[Retire/Upgrade packet](design-workshop-upgrade-confirmation-layout.md).
The old `retire_two_wrapper-own-1.png` has already been painted, but its
button theme startup was incomplete. **Do not match its olive outline.**

[Checked scalar receipts](design-workshop-ok-button-paint-cases.json).
Independent checks reproduce the old export exactly, verify all13009
non-glyph pixels against installed source art, and pin10 source literals.
The remaining91 pixels are the native black glyph overlay.
Additive private freeze contains20 payloads at
`tmp/reference/independent-workshop-ok-paint-evidence-20261021/`;
manifest `e740e9ada7519ce3117b9e189f59c0f4b71a2f03b628c8ef58cf215bab54c72d`.
This provenance paragraph was added after freezing; earlier evidence remains
unchanged.

## Cause and final target

The earlier harness omitted `460720`. This routine loads iface.pcx theme
colors and then interface.pcx button sprites, and installs generic modal
sprite pointers at `9c1210`, `9c1228`, `9c1234` and their state variants.
The generic dialog constructor copies those defaults into its own fields;
`61b9d0` copies them into the OK child. With those pointers still zero,
the original painter used its simple two-pixel bevel fallback instead.

Calling `6222a0` again on that old state produced a byte-identical PNG.
Thus it is **not** an unpainted initialization surface waiting for the modal
loop. It is an already painted surface from incomplete startup.
`607010` enters the modal loop by calling Window+4; that can repaint, but
does not install the missing theme defaults.

A bounded correction executes original `460720`, reapplies `59f240`
generic defaults, and constructs one illustrated Retire dialog. No game
loop, model render or layout matrix. Correct normal OK-owned target:

```
tmp/reference/workshop-ok-paint-private-20261021/initialized-ok.png
655 x 20
PNG SHA256 3c1a0a39263dc7226b50516c0594cffb666e467bf7b289f598de9d67305878cb
```

Indexed rows use pitch656. Ignoring the pad byte, pixel counts are
palette174:6507, palette238:6498, palette0:91, palette9:4, total13100.
These are blue/teal horizontal stripes with black OK text and four
transparent-corner background pixels. Native RGB values:

| Index | RGB | Purpose in this target |
| --- | --- | --- |
| 174 | 28,80,99 | Alternating sprite rows |
| 238 | 103,91,181 | Alternating sprite rows |
| 0 | 0,0,0 | OK glyph |
| 9 | 9,0,0 | Four transparent corner pixels after background fill |

Client and placement remain655x20 at dialog-local `[33,217,688,237)`;
OK uses Arial Narrow -16 bold, x317, baseline16. The preliminary default
font paint at x318/baseline14 is overwritten by the final paint.

## Actual style and rectangle rules

Button vtable `68b934` +30 is painter `6222a0`. It first fills the owned
client using the byte at `6b4fb4` (9 in this profile). State selection is
`button+ab8` when explicitly0..2, otherwise pressed/disabled fields select2,
hover field+a74 selects1, and idle selects0.

The painter prefers a whole sprite at +ad4[state], then a frame descriptor
at +ac8[state], then the multi-piece path when +b10[state] exists. The
initialized Retire OK uses that multi-piece path. There is **no small-frame
index27**, nor a `5fbbd3` frame descriptor, in this final OK background.

The three normal pieces are:

| Piece | Native pointer | Installed interface.pcx rectangle, x,y,w,h | Target |
| --- | --- | --- | --- |
| Left | 777a18 | 64,127,3,20 | x0..3, y0..20 |
| Center | 777a44 | 68,127,67,20 | x3..652, y0..20 |
| Right | 777a70 | 136,127,3,20 | x652..655, y0..20 |

Intervals in the target column are half-open. `6223ba..6225a1` paints the
left/right pieces and fills the intervening width using native `5f37f0`;
the source centers are uniform across each row in this installed art.
For a generic width W, the middle occupies `[3,W-3)` and the right begins
at W-3. The captured button height is20, matching the source piece height.
No alternate height behavior is inferred from this control.

Startup `460df2..460ec9` extracts nine rows of three pieces with21px source
row stride. The generic modal selects rows6/7/8 (normal/hover/pressed):
source y127/148/169, same x and sizes above. The corresponding pointers:
left777a18/777a9c/777b20, center777a44/777ac8/777b4c,
right777a70/777af4/777b78. Only normal raster is newly measured here;
the other two mappings are source assignments.

Native loading maps installed PCX indices164/228 to target174/238 in this
profile; transparent255 becomes key0x109 and leaves the filled background.
Do not copy the raw PCX index values directly into the native palette gate.

The old fallback has zero sprite fields, +a84=36, +a88=30, +a8c=86,
+a90=2, +a94=0. The fallback draws an outer color0 outline followed by
two beveled inset outlines, palette30 on top/left and86 on bottom/right.
That explains the old green/olive pixels; these fallback fields are not
the final initialized sprite background contract.

Caption/body glyphs still use226 and choice text still uses0 after the
corrected startup. Their earlier color correction remains valid. The
existing illustration and parent frame matches do not need a change based
on this button-specific finding.
