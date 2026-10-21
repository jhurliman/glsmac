# Indexed console panel backdrop and pattern phase

**Yes: use style18 from the existing51-style `interface.pcx` small-frame
bank for the selected and roster inner panels.** The native address is
`7b0058 + 18*40 = 7b0328`. Their outer frame is style21 at`7b03a0`.
This is the same source bank and compositor described in
[generic dialog frames](generic-dialog-fonts-and-frame.md) and
[small-frame pixel semantics](small-dialog-frame-pixels.md).
[New native cases](console-panel-backdrop-cases.json) pin both the style
identity and console phase. No RGB reverse lookup is required.

Original executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
All coordinates below are fixed1024×768 **console-bitmap coordinates**.
The screen placement origin is`(0,511)`. Rectangles are half-open.

## Indexed atlas recipe

Original loader`45eea0` loads `interface.pcx` at`45eeba` with index offset10
and236 palette colors, then builds the style table from`45ef68` onward.
For style`i`, the atlas origin is:

```text
x = 1 + 39*(i % 3)
y = 229 + 39*(i // 3)
loadedIndex = (rawPcxIndex + 10) & 255
```

| Piece | Atlas crop `(x,y,width,height)` relative to style origin |
| --- | --- |
| TL / TR |`(0,0,6,6)` / `(32,0,6,6)`|
| BL / BR |`(0,32,6,6)` / `(32,32,6,6)`|
| Top / bottom |`(7,0,24,6)` / `(7,32,24,6)`|
| Left / right |`(0,7,6,24)` / `(32,7,6,24)`|
| Center pattern |`(7,7,24,24)`|

Style18 origin is`(1,463)`, so its exact center crop is
**`[8,470,32,494)`**,24×24. Its first loaded index is196. All576 bytes of
the original generated center bitmap match independently decoded atlas
indices; SHA256:
`966f54c285391cfbe12046e6cd897cdf2614b48ca737b5ee5292b65abd591db3`.
The style's pattern mask/mode field`+24h` is7.

Style21 origin is`(1,502)`. Its nominal center crop begins`(8,509)` with
loaded index9, so the original loader leaves the center bitmap pointer null.
**Style21 paints its border but supplies no center fill or masked pattern
overlays.** Do not turn its nominal key9 center crop into a filled bitmap.

Use the established indexed frame semantics: border sprite source key9,
then style18 pattern replacement only where the destination currently equals7,
then its final unconditional center-pattern copy. Raw253 becomes loaded7;
raw255 becomes loaded9. A final center pixel is copied literally, including
7 or9; it is not a per-pixel transparent source.

## Exact native panel rectangles

The complete native`4cb9f0` caller, including its selected-card subcall,
produces these frame calls:

| Region | Outer style21 rectangle | Inner style18 rectangle | Inner final center-copy rectangle |
| --- | --- | --- | --- |
| Selected card |`[8,60,136,248)`|`[11,63,133,245)`|`[17,69,127,239)`|
| Shared lower roster |`[139,192,764,248)`|`[142,195,761,245)`|`[148,201,755,239)`|

Selected calls are at`4c9909` and`4c994a`; roster calls are at`4cddba`
and`4cddf6`. The receipt records the corresponding return addresses
`4c990e/4c994f/4cddbf/4cddfb`. Each panel paints the outer frame first,
then insets3 pixels for the inner frame. The model's content clip is another
3-pixel inset for the selected card, not the inner frame's6-pixel center inset.
Paint the frame using its full rectangles, then install the model clip.

The roster has **one shared frame/background**, painted before its Unit loop.
The loop does not repaint style18 inside each Unit cell. Later text, health,
badges and selection borders are separate operations. In particular, the
selected-Unit border at`4ce59a` follows that Unit's model/statistics; it is not
a replacement for the shared indexed backdrop.

## Preserve console phase

For style18's center pattern and masked pattern overlays:

```text
patternIndexAtConsolePixel(x,y) = center[(y % 24)*24 + (x % 24)]
```

`5fbbd3` passes the destination bitmap x/y as pattern phase to`5f34c0`
and`5f3660`. Clipping does not restart phase. The border edge sprites have
their own piece-relative tiling phase, as in the existing frame contract.
Apply the absolute formula to pattern passes, not to every border pixel.

The origin`(0,511)` is for screen placement only. **Do not add511 before
sampling the pattern**: that would shift its vertical phase by7 pixels.
With a separate widget backing bitmap whose console-local top-left is`(L,T)`,
sample center at`((localX+L)%24,(localY+T)%24)`. Do not start its pattern at0,0.

The native Unit body/overlay calls and clip rectangles for five linked Units
are observed in one complete caller run:

| Cell | Model clip | Body anchor / zoom | Overlay anchor / zoom | Pattern phase at clip top-left |
| --- | --- | --- | --- | --- |
| Selected |`[14,66,130,242)`|34,66 /−4|14,66 /0|14,18|
| Roster0 |`[143,196,208,244)`|156,197 /−8|145,197 /0|23,4|
| Roster1 |`[209,196,274,244)`|222,197 /−8|211,197 /0|17,4|
| Roster2 |`[275,196,340,244)`|288,197 /−8|277,197 /0|11,4|
| Roster3 |`[341,196,406,244)`|354,197 /−8|343,197 /0|5,4|
| Roster4 |`[407,196,472,244)`|420,197 /−8|409,197 /0|23,4|

Roster cells advance66 pixels. Therefore the interior pattern's horizontal
phase advances18 modulo24 and repeats every four cells. This does **not**
mean every complete cell crop is identical after four cells: the first cell
can include the shared panel's left border. Likewise y196..200 and239..243
are not entirely within its final center-copy rectangle. Reuse the indexed
frame compositor for those pixels rather than filling the entire cell with
the center tile.

For [live shadow blending](live-ground-shadows.md), keep the indexed panel
target and apply`shadowLookup[existingTargetIndex]` before body color copy.
The established flat body/shadow silhouettes lie inside the center-fill area
for these selected/roster fixtures. Their backdrop indices can thus be obtained
directly from the atlas formula; the full frame remains necessary for the
surrounding UI.

## New controls and private evidence

`probe_console_panel_callers_20261021.py` executes the full native roster
caller and selected subcall with five explicit linked Units. Frame, body and
terrain painters are observation sinks in that layout run; no full UI screenshot
is claimed. Actual rules/geometry callers execute and return normally.

`probe_console_panel_pixels_20261021.py` separately executes original interface
loading and complete`5fbbd3`/software-blitter panel draws on1024×257 indexed
targets: full selected, full roster, selected-content clip and five roster-cell
clips. All **eight** targets match an independent atlas-index/frame-compositor
model byte for byte. Every clipped crop matches the same rectangle from the
unclipped full panel, proving that the clip keeps parent-bitmap pattern phase.
Original center bytes, full targets and crops stay private; public cases retain
geometry, phases, hashes and the relevant primitive arguments.

Panel-only targets start from a declared index23/41 diagnostic background to
expose preserved key9 holes. They exclude console2.pcx shell, Units, terrain,
text and later selection decorations. No new production rendering code or
earlier evidence freeze is changed.

Private immutable packet:
`tmp/reference/independent-console-panel-backdrop-evidence-20261021/`.

Manifest SHA256: `13e3b90e2d6c515c7130a54d3312a8827a422617f629758f7717d9f4f9d145e6`; all 37 payloads rehashed successfully.
