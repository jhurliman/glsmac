# Stock dialog font raster

The original font bank registers regular `arialn.ttf` and requests bold Arial
Narrow above12 pixels. Selecting the bundled `arialnb.ttf` gives a different face,
not that synthesized request. Stock1024-wide body/control text uses16 pixels;
application captions use the separate Arial14 bold binding. Source parameters
and measured host controls are in the [font/frame contract](behavior/generic-dialog-fonts-and-frame.md)
and [glyph contract](behavior/dialog-gdi-synthetic-glyphs.md).

`RasterMode::MONOCHROME_BOLD`, exposed to UI text as `:mono-bold`, loads hinted
monochrome glyphs and emboldens the resulting bitmap. It deliberately does not
embolden the outline before hinting: that produces823 differing pixels in the
measured body atlas instead of five. Cache keys include this mode separately
from ordinary mono and grayscale. Existing callers keep their current mode.

Stock popup body, input, button and list slots now use
`arialn.ttf:16:mono-bold`; the faction quotation preview uses the same binding.
Window captions use `arialbd.ttf:14:mono` and uppercase text. Explicit panel and
specialist fonts keep their separate bindings. Border slices, rectangles,
state colors, right-aligned faction descriptors and console attachment rails
still require the subsequent layout pass.

## Measured boundary

These references are current-Windows offscreen GDI controls using source-derived
font requests and declared registration state. They are not an original desktop
frame or proof for every Windows/font version.

| Control | Result with the installed reference fonts |
| --- | --- |
| Arial14 bold caption masks |95/95 exact glyph masks |
| Narrow16 bitmap-synthesized bold masks |91/95 exact glyph masks; five differing pixels in `M`, `S`, `^`, `k` |
| Advances and left bearings, both profiles |190/190 glyph controls exact |
| `CHOOSE A FACTION` advance | Caption138px; body144px |
| Actual captured caption vs independently composed GDI mask |0 differing pixels in138x32;618 ink pixels, allowing translation to current placement |

The actual caption mask comparison found origin655,511 and baseline535 in the
1024x768 faction capture. That is raster evidence, not original frame placement
or text-color parity. The public native fixture retains original hashes for all
95 glyphs in each profile; it tolerates only the four named body discrepancies
by their known port hashes, and permits future exact matches. Arbitrary changes
to those four glyphs still fail. No proprietary font or glyph payload is added.
The private source receipt has22 files and manifest SHA256
`88f4349d80bd8281ab98cb1055381123df1e6f08dfcc0ae162f0b8176f593d9d`;
the11-file source font/frame receipt has manifest
`453905730366b850a54253d275587be56b5d14de73cea4a882ab6865d4952667`.
Both receipts and the normalized imported contracts were verified.

## Validation

Cached incremental build passes. Five affected native font/input/render groups
and the faction-selection, faction-identity and first-base GSE scripts pass.
The two measured font groups were run with actual original Narrow and host Arial
Bold paths; they are opt-in without those resources. Set
`GLSMAC_DIALOG_BODY_FONT` to the original `arialn.ttf` and
`GLSMAC_DIALOG_CAPTION_FONT` to the reference `arialbd.ttf`, then run
`--gse-tests --gse-tests-native-filter 'SMAC dialog|regular Arial loader' --quiet`.
The cache test checks distinct mono/bold entries and reuse of each entry.

FreeType.cpp, FontLoader.cpp, Text.cpp and affected Input.cpp/Button.cpp pass
focused analysis. Four-TU ASan/UBSan instruments the three changed production
implementations and the real glyph test; both font groups and cache group pass.
Other objects are normal, leak detection is off and stack-use-after-return is on.
A real hidden-SDL/OpenGL run exercised SMAC setup, Morgan selection, keyboard
name editing, Cancel/Back and normal Exit. Exit0, no tracked GL resource leaks;
allocation tracking was not enabled. Screenshots are framebuffer PPM converted
to lossless PNG, with no artwork or text edits.

![Actual faction selection after font binding correction](https://raw.githubusercontent.com/jhurliman/glsmac/714ea4e42ca819a25355dbb0c6c55a20494c7af8/dialog-fonts/faction.png)

![Actual leader-name keyboard edit](https://raw.githubusercontent.com/jhurliman/glsmac/714ea4e42ca819a25355dbb0c6c55a20494c7af8/dialog-fonts/name-edited.png)
