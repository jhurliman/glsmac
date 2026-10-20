# RE-SMAC-DIALOG-VISUAL-001: generic dialog fonts, frame slices and console rails

Evidence: **static pinned executable bindings**, plus **current-Windows
offscreen GDI metrics** for source-derived requests. No native modal
framebuffer or screenshot match is claimed. [Numeric font advances, ABC
metrics, request IDs and atlas slices](generic-dialog-fonts-and-frame-cases.json).

## Font resource and font-bank IDs

The original registers the game's **arialn.ttf** before constructing its UI
font bank. `46c34d -> 630600` builds a scalable-font resource and calls
`AddFontResourceA`, then `46c352 -> 53f6e0 -> 5a59c0`; the latter calls
font initializer `59f880` at `5a5b24`, then console constructor `4660d0`
at `5a5b8b`. The resource is the bundled regular Arial Narrow file, not a
request to install all four bundled Arial Narrow style files.

This matters on this host: without registration, requests for Arial Narrow
silently resolve to **Arial**. With only the bundled regular file registered
privately for the measurement process, they resolve to **Arial Narrow**.
That registration was removed when measurement finished. No installed font
or foreground app was changed. The regular file's SHA and selected font-data
hashes are recorded. Do not use the unregistered fallback widths as the
intended font-bank widths.

Font bank base is `94ec68`, each font object is40 bytes. Sizes at `6aead0`
are descending: **40,36,32,24,22,20,18,16,14,12,10,8**. There are four
objects per size, with requested style flags **0,1,2,4**. Lookup
`59f0d0(requestedSize,styleColumn)` chooses the nearest size, retaining the
earlier/larger size on a tie. Object address is
`94ec68 + 40*(4*sizeIndex + styleColumn)`.

The bank producer changes both face and flags:

| Bank nominal size | Face | Effective flags |
| --- | --- | --- |
| <=12 | Arial | stored style flags |
| >12 | Arial Narrow | stored flags OR1 |

`630450` translates bit0 to weight700, bit1 to italic, bit2 to underline.
No bit0 means LOGFONT weight0/default, not an explicit400 request. Therefore
**plain and bold bank columns above12 both request bold Arial Narrow**;
the italic column requests bold italic and the underline column bold
underline. Do not assume column0 above12 is regular weight.

All request heights are **negative logical pixel heights**, not point sizes.
Width, escapement, orientation and strikeout are0; charset0/ANSI, output
precision7/TrueType-only, clip precision0, quality0/DEFAULT_QUALITY,
pitch/family0. No outline or antialiasing mode is established merely by
quality0; this new receipt measures metrics, not glyph masks.

## Roles in the stock setup/in-game dialog path

`MainFontSize` defaults16 and is16 in the inspected installed INI. At
width800 the producer subtracts2 before choosing the nearest bank size.
This is a discrete font choice, not scaling an already rasterized16-pixel
atlas. Other widths do not take that subtraction.

| Role/binding | Width1024, stock MainFontSize16 | Width800, stock setting |
| --- | --- | --- |
| FACTION list/body and generic form label/edit font inheritance | Arial Narrow, -16, weight700 | Arial Narrow, -14, weight700 |
| BLURB/DATALINKS preview explicit body tuple `94f404/94f3fc/94f400` | bank16 columns0/1/2 | bank12 columns0/1/2: Arial regular/bold/italic |
| Generic dialog button text tuple at `+316c/+3170/+3174`, default `9c1290` family | bank16 columns0/1/2 | same bank16 columns0/1/2 |
| App-painted window caption, including CHOOSE A FACTION / NAME / FIRSTBASE | **Arial, -14, weight700** | same explicit request |

The caption is a different binding from the button tuple: `4667d4/d6`
pushes flags1/size14 and `466852` constructs Arial font `7ece2c`.
Application dialog painter `409760` selects that exact object at
`409beb` before centering caption text at `409c3a`. Thus using the body
font for the caption gives the wrong face, size and measured advances.

Generic defaults come from `59f240 -> 61dda0/61dea0`; constructors copy
the resulting body and button tuples, and `61b9d0` assigns them to the
corresponding bitmaps/controls. The faction preview explicitly overrides
its body tuple. These are stock, unoverridden-template bindings; arbitrary
`#font` templates, custom settings and all visual control states are not
newly exercised here.

At widths>=1024 certain generic dialog width/layout calculations multiply
by3/2 when their independent flags/parent gates permit it. This is **not**
evidence to multiply all the font heights by3/2. The font requests above
remain their discrete logical pixel sizes. MainFontSize and the preview's
fixed size choice are separate inputs.

## Faction row delimiter and text metrics

FACTION rows contain `formalName + "|(" + descriptor + ")"`.
List painter `625940` searches for `|`, temporarily NUL-terminates the
left part and draws it, restores/skips the delimiter, then calls
`5f6250` for the right part. That helper measures the descriptor and uses
`rect.right - measuredWidth`, vertically centering it. **The pipe is not
printed or replaced with a space. The descriptor is right aligned in the
same row/font**, with no automatic italic switch in this split. The selected
and other row branches retain their own color/background rules; this is
not a new complete selected/hover/disabled framebuffer comparison.

Plain runs reach `630870/GetTextExtentPoint32A` and `TextOutA`. No explicit
kerning-pair application is present in this measured/plain path. The host
fonts advertise AV/To adjustments, but all measured complete-string widths
equal the sum of individual byte advances. Keep automatic font-engine pair
kerning disabled when reproducing these runs. Markup and pipe splitting
must be done before measuring the actual visible runs.

Advance is **A+B+C**. Place ink using its bearing, but do not add A again
to a stored total advance. The cases supply all95 printable ASCII advances
and ABC triplets for nine request profiles, plus whole-string measurements
and selected advertised kerning pairs. Negative bearings/overhangs are
retained. These are ANSI/byte measurements, not Unicode shaping rules.

Representative current-host results with the bundled regular resource active:

| Request | CHOOSE A FACTION | Gaia's Stepdaughters | (The Green) | Name: | OK | Change Gender |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Caption: Arial14 bold | 138 | 150 | 84 | 43 | 21 | 111 |
| Main/body: Arial Narrow16 bold | 144 | 148 | 82 | 44 | 21 | 108 |
| Preview at800: Arial12 regular | 116 | 119 | 66 | 37 | 17 | 88 |

The Narrow16 request falls back to Arial16 on the unregistered host, where
CHOOSE A FACTION measures160 instead of144. That is a diagnostic font-mapping
failure, not an alternate native caption measurement.

The original font object stores `+c = tmHeight + tmExternalLeading`,
`+10 = tmAscent - tmInternalLeading`, `+14 = tmInternalLeading`,
`+18 = tmAscent`, `+1c = tmDescent`. The latter +10 is the baseline offset
used by the text primitive. Current registered Narrow16 bold has height20,
ascent16, internal leading4, hence baseline offset12; Arial14 bold has
height16, ascent13, internal leading2, hence offset11. Preserve these
distinct metrics rather than making CSS font-size equal a complete line box.

## Actual border atlas and slice tables

The generic application dialog painter is `409760`, reached through
`61df90`'s virtual +180 hook. It uses `5fbbd3`, a corner/edge/center
composer. The source is **interface.pcx**, loaded by `45eea0`, not
frame.pcx. `46a110` adds the separate console-attached rails discussed below.

The small-frame style bank begins `7b0058`, stride40, with51 styles arranged
as17 rows ×3 columns in the atlas. For style index i:

`x = 1 + 39*(i % 3); y = 229 + 39*(i // 3)`

| Piece | Source x,y,width,height |
| --- | --- |
| Top-left / top-right | `(x,y,6,6)` / `(x+32,y,6,6)` |
| Bottom-left / bottom-right | `(x,y+32,6,6)` / `(x+32,y+32,6,6)` |
| Top / bottom edges | `(x+7,y,24,6)` / `(x+7,y+32,24,6)` |
| Left / right edges | `(x,y+7,6,24)` / `(x+32,y+7,6,24)` |
| Center tile | `(x+7,y+7,24,24)` |

This is not one scaled38×38 image: corners retain their native extent,
edges are composed across the target rectangle, and the center uses its
separate pattern bitmap/mode7. The loader suppresses a small style's center
pattern when its first loaded pixel is key9. In the inspected stock asset,
style3 has no center pattern; styles0/6/9/27/28/29 do.

Useful bindings in `409760`: ordinary outer style9 (`7b01c0`, x1/y346),
caption outer style3 (`7b00d0`, x1/y268), caption inner style6 (`7b0148`,
x1/y307), and body-pane style0 (`7b0058`, x1/y229) on its admitted branch.
FACTION's row callback `409600` selects style `27+state` at
`7b0490 + 40*state`; do not invent meanings for every state ID from this
table alone. The numeric cases retain styles27..29.

Frame modes2 and6 use the **large** style table at `6a7a28` for the outer
frame. This uses a different interface.pcx grid:

| Piece row | Left x296 | Center x358 | Right x420 |
| --- | --- | --- | --- |
| y711 | top-left | top edge | top-right |
| y756 | left edge | center | right edge |
| y801 | bottom-left | bottom edge | bottom-right |

Each source piece is **61×44**. These exact atlas crops are built at
`45f195..45f300`. The large table is not frame.pcx or console2.pcx.

Frame mode is derived from dialog `+30ac /10000`. `4084f0` starts from
global `6c718c`; when the active view type is not0/10 (and is not type4
with a nonnull parent), it changes a starting mode0/5 to6. Mode1000 bypasses
this application frame painter. Thus avoid assigning one unconditional
border variant to every setup/in-game dialog based on the caption string.
The full static layout producer and its gates are retained privately; no
new live modal-state sample establishes each runtime mode here.

Caption placement is also frame-dependent. In the ordinary non-console,
non-large branch, relative to the client RECT R, the title outer rectangle
is `[L+4,T+4,R-4,T+28)`, then inset3 for its inner style and another3 for
text. Large modes first inset the starting title rectangle by9; the
console-attached mode0/5 path uses separate asymmetric x offsets26/58
before its one-pixel title adjustment. `5f5f30` centers the caption using
measured advances. Treat these as branch-specific integer operations;
replacing them with one generic centered flex header changes placement.

## Console-connected rails and backdrop

`4082f0` invokes `46a110` when presenting the application dialog.
That helper captures the current console/dialog backing, composes a
transition and finally draws four vertical sprites joining the dialog to
the lower console. Their source is the console extension sheet loaded by
`4660d0`, **console_x.pcx** at width800 or **console_x2.pcx** otherwise.

| Source | Four x origins | y | Widths | Height |
| --- | --- | ---: | --- | ---: |
| console_x.pcx | 683,699,708,717 | 1 | 15,8,8,15 | 529 |
| console_x2.pcx | 713,729,738,747 | 1 | 15,8,8,15 | 529 |

These become sprites `7ecf64/7ecf90/7ecfbc/7ecfe8` and are drawn at
`46a526/54f/572/5aa`. Their placement/clip comes from the live window and
console rectangles. They are not four edges of the modal's interface.pcx
border. The underlying bottom-console backing still comes from console.pcx
or console2.pcx. The helper copies current backing images; no new fullscreen
frame.pcx background is loaded for generic NAME/FIRSTBASE.

All these PCX paths use the established common-palette/index-key rules.
The new receipt does not reproduce transition timing, every clip/rail
placement, the finished composite, per-state colors, arbitrary display
scaling or physical-monitor rasterization.

## Validation and limits

The static extractor pins29 call/store/argument instructions, font-bank
tables, import names, startup registration order, atlas metadata and relevant
function byte hashes. Two offscreen GDI profiles distinguish unregistered
fallback from private bundled-font registration; nine font requests each
have95 printable ASCII advance/ABC records and22 whole-string controls.
All those controls satisfy width=sum(advances). No font payload or owned
artwork pixels are published in public fixtures. No production code changed.

The source establishes requests, font IDs and composition rules; the GDI
measurements establish current-host metrics for declared resource state.
Neither alone is proof of historical Windows glyph masks or a native full
dialog framebuffer. Wrapping, reveal timing, IME, all control states and a
complete modal input/render loop remain outside this handoff.
