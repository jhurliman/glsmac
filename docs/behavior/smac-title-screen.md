# RE-SMAC-TITLE-001: settled SMAC title screen

Scope: original SMAC at 1024×768, seven top-menu entries, Start Game selected,
Gamma Correction 100. The private reconstruction matches the supplied reference
at **all 786,432 pixels**. Original assets, glyph masks, screenshots and the
reconstruction stay in ignored private evidence. The
[numeric cases](smac-title-screen-cases.json) contain geometry, palette/font
parameters, selected discriminators and verification counts only.

Evidence combines original executable PCX/FLC decoding, gamma correction,
tint-table generation, font creation and text-centering routines with host
Windows GDI offscreen glyph rasterization. Final composition is independently
assembled from those results and compared with the reference. This is not a
complete original WinMain/window-message/render-loop execution. Other resolutions,
SMAX, custom fonts, nondefault gamma and animated transition screenshots are
outside the pixel-exact acceptance profile.

## Artwork and layout

At width 1024, load `openinga.pcx` at its native 1024×768 dimensions. The title
logo and background are already in that image. The native width-800 branch
selects `openingb.pcx`; it is not the reference measured here.

The menu frame is **`openitem.flc`**, not `menu.pcx`, `menu2.pcx` or an
interface-sheet crop. Each decoded frame is 354×75 indexed pixels. No nine-slice,
CSS border or horizontal stretch is involved in the settled reference.

For seven entries the menu canvas is 354×525, anchored to the bottom right:
origin **(670,243)**. Draw a complete frame at local `(0,75*i)` for row i=0..6.
Thus its global row origins are y=243,318,393,468,543,618,693. The frame's
transparent padding is meaningful; do not crop to its visible rail/button.

The opening caller advances the FLC decoder six times, yielding file frames
0..5. **Zero-based frame 5 is the settled open artwork.** It then draws the
labels. The next six frames 6..11 retract the artwork when closing; frame 11
is blank. Title transition timing comes from the caller's minimum-10ms steps,
not an automatic assumption that every title step waits the file's 40ms speed.
Only the settled result is newly accepted here.

## Palette identity, offset and gamma

Use the common **`palette.pcx` palette**, not the palette embedded in
`openinga.pcx`, for the final title colors. The original PCX loader shifts
source indices by +10 into game palette entries 10..245, copying source palette
entries 0..235 into that range. Keep format indices distinct from game indices.
FLC frame pixels already use the game-index layout; do not add 10 to them.

This matters for source indices 128..138: their opening-image olive ramp differs
from the common palette. For example, opening source 138 is (31,26,16), while
the common source 138, hence game index 148, is **(39,31,12)**. Matching raw
opening RGB alone cannot reproduce the screenshot.

There is also a real native gamma stage. For game entries 10..245 and each RGB
channel c, it computes:

`q = clamp(pow(c / 235.0, 1.0 / gamma), 0.0, 1.0)`

`result = trunc(235.99 * q)`.

At Gamma Correction 100, gamma=1.0 and this gives **min(c,235)** for the checked
8-bit channels. System-reserved entries outside 10..245 are not part of this
loop. Do not apply a six-bit expansion, gamma-2.2 transform or multiply every
channel by 235/255. Values below the cap remain unchanged in this profile.

Across background x<630, all 145 used opening indices map unambiguously to
one reference RGB. Twenty-five mappings differ from raw opening colors; all
483,840 compared background pixels match the corrected common-palette result.
The original gamma routine produces the same full palette as the installed
palette-check cache; this is not merely an inferred screenshot correction.

## Frame keys and blue fill

Treat the decoded frame's indices as semantic drawing codes:

- **9:** leave the underlying background unchanged. Its diagnostic asset RGB
  is magenta; magenta is not a color to display.
- **0:** apply the menu's blue tint lookup to the underlying background index.
  Its diagnostic asset RGB is pure blue; it is neither an opaque blue fill nor
  ordinary alpha compositing.
- **Other indices:** display that game-palette entry after gamma correction.

Frame 5 contains 7,999 transparent-code pixels, 8,296 blue-fill-code pixels and
10,255 ordinary artwork pixels. All ordinary artwork colors and geometry match
the first reference frame exactly. The same frame is repeated for all rows.

Build the blue table **after** gamma correction. Let P be the corrected game
palette and target index 179, whose RGB here is (17,66,88). For each input
background game index i, calculate each channel of:

`mixed = trunc((20*P[i] + 80*P[179]) / 100)`.

Choose the smallest squared RGB distance to mixed among game entries 10..245.
Scan ascending and update only for a strictly smaller distance, retaining the
earlier index on ties. The generic native helper excludes protected palette
ranges and restores their identity mappings; the accepted startup palette has
no such ranges. System-reserved colors 0..9/246..255 are not nearest-color
candidates. Store the chosen index as the lookup result, then use its corrected
RGB for display.

The installed `bluetint.tmp` is a cached version of this 256-entry result, gated
by a full palette-check comparison. The fresh original computation **after
gamma** equals that cache byte for byte. Computing from the uncorrected palette
differs at two inputs and produces 92 wrong screenshot pixels, despite otherwise
matching frames and text. Do not compensate with hand-selected blue CSS colors.

## Label font and rasterization

The title requests **Windows GDI Arial**, not the supplied `arialn.ttf` family.
Startup separately registers that bundled Arial Narrow resource, but this menu
uses the ordinary Arial family pointer and creates a new bold font. The
`ALPHC___.TTF` asset is not the label font; the logo is background artwork.

Exact LOGFONT request:

| Field | Value |
| --- | --- |
| Face | Arial |
| Height | -18: character-height request in pixels, not 18 points |
| Width, escapement, orientation | 0, 0, 0 |
| Weight | 700, bold |
| Italic, underline, strikeout | 0, 0, 0 |
| Charset | 0, ANSI_CHARSET |
| Output precision | 7, OUT_TT_ONLY_PRECIS |
| Clip precision | 0, CLIP_DEFAULT_PRECIS |
| Quality | 0, DEFAULT_QUALITY |
| Pitch/family | 0, DEFAULT_PITCH / FF_DONTCARE |

The original draws with TextOutA onto an 8-bit indexed DIB, transparent text
background, and left/baseline alignment. In the accepted host/reference profile,
glyph pixels are **monochrome coverage**, not grayscale or ClearType: the
combined label raster contains only background index 9, active index 222 and
inactive index 225. There is no gray-coverage-to-palette blending stage to
approximate. Native GDI returns widths and glyph shapes that reproduce the
reference exactly. A different font rasterizer or hinting mode needs its own
pixel comparison; “Arial bold 18” alone is not sufficient evidence.

Active text game index **222** gives RGB **(176,212,235)**; inactive index
**225** gives **(104,140,196)**. Selection changes the text color in this settled
profile. The original explicitly disables the shadow color for these labels;
the supplied shadow offset fields do not imply an enabled shadow.

The measured GDI metrics are height 21, ascent 17, descent 4, internal leading
3, external leading 1. The native centering helper uses ascent minus internal
leading = **14** for its vertical measure, rather than the full 21-pixel cell.

## Text rectangles, clipping and placement

Coordinates below are local to the (670,243) menu origin, and rectangle right/
bottom bounds are exclusive. Row i has hit rectangle `(15,75*i,300,75*i+75)`.
The label redraw clip is `(15,75*i+31,300,75*i+57)`. The centered text rectangle
is `(27,75*i+33,304,75*i+55)`; preserve its independent horizontal offsets.

For width w, the native horizontal origin is
`27 + trunc((277-w)/2)`. Its baseline is **51+75*i** using the metrics above.
The actual label positions are:

| Row | Label | Advance width | Local x | Global x | Global baseline |
| ---: | --- | ---: | ---: | ---: | ---: |
| 0 | START GAME | 119 | 106 | 776 | 294 |
| 1 | QUICK START | 123 | 104 | 774 | 369 |
| 2 | SCENARIO | 95 | 118 | 788 | 444 |
| 3 | LOAD GAME | 110 | 110 | 780 | 519 |
| 4 | MULTIPLAYER | 129 | 101 | 771 | 594 |
| 5 | VIEW CREDITS | 131 | 100 | 770 | 669 |
| 6 | EXIT GAME | 99 | 116 | 786 | 744 |

These are advance widths and baseline positions, not cropped glyph-image
bounds. Do not vertically center by the bitmap's ink bounding box. The measured
glyph raster has 810 active-color pixels and 4,653 inactive-color pixels.

## Installed FLC decoder requirements

The installed file has a 128-byte FLC header, magic 44818 (AF12), declared
12 frames, 354×75, 8-bit depth, speed 40ms, first frame offset 128 and second
frame offset 1783. Its total file size is 55,651 bytes. There are 13 physical
frame chunks, including a final ring frame; declared count and physical chunk
count are not interchangeable. The settled title needs the first six frames.

Frame containers have a 16-byte header, magic 61946 (F1FA), a byte length and
child-chunk count. Child chunks have a 6-byte length/type header; lengths include
their headers. Installed child types are **4 (COLOR256), 15 (BYTE_RUN/BRUN), and
7 (DELTA_FLC/SS2)**. Frame 0 contains 4 and 15; subsequent frames, including the
ring frame, contain 7. There are no JPEG/image-sheet substitutions in this path.

COLOR256 packets skip palette entries and replace a run of RGB triples; encoded
count 0 means 256. BRUN expands each scanline's signed run packets: a positive
count repeats the following byte; a negative count copies that many literals.
SS2 is a **word-pair** delta: preserve the preceding frame, honor skipped-line
and last-pixel control words, skip unsigned byte counts per packet, then copy
literal word pairs or repeat a word for negative run counts. A byte-run-only
decoder cannot read this file. Bound all counts, lines and chunk ends before
publishing a frame. Palette changes remain separate from indexed pixel output.

All 12 native decoded frame rasters match an independent Pillow decode after
removing native 356-byte row padding. Neither padding nor a bitmap's storage
orientation should change the 354-pixel logical image. The accepted final
composition has **zero mismatching pixels**; no production port code was changed.
