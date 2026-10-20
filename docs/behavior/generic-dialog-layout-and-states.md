# Executed generic dialog layout and row states

Pinned `terran.exe` SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Nine bounded **original x86** profiles execute the stock FACTION, NAME and FIRSTBASE
templates at1024×768 through `61b9d0`, stopping at `61caf9` before modal input.
They use installed SCRIPT/LABELS/ALPHA/faction definitions, original theme/font/frame
loaders and a declared memory-only Windows-service environment. Native geometry,
frame calls, text calls and backing DIBs are captured. Host GDI supplies font
metrics/text pixels. These are not original desktop screenshots or a naturally
booted frontend trace. Private PNGs show individual backing surfaces.

All rectangles below use `[left,top,right,bottom)` coordinates. Outer rectangles
are the window's actual post-show `+13c` rectangle; child coordinates are relative
to their recorded parent. The layout working fields `+30b4/+30b8` are **not** the
final outer position when the console-visible show path repositions the dialog.

## Conditioned layout profiles

| Template | Plain mode0 outer | Plain mode6 outer | Console-visible mode0 outer |
|---|---|---|---|
| FACTION | `[472,545,1019,763)` | `[454,536,1019,763)` | `[215,255,840,555)` |
| NAME | `[337,649,1019,763)` | `[319,640,1019,763)` | `[148,255,908,555)` |
| FIRSTBASE | `[10,40,512,154)` | `[10,40,530,163)` | `[238,255,818,555)` |

Inputs are explicit: mode globals0/6; otherwise-zero active-view table; null dialog
parent input; MainFontSize16; NAME default `Deirdre` with the stock male NAME
template; FIRSTBASE default `Gaia's Landing`; seven native-loaded FACTION rows.
NAME2/female labels, preview callback/layout, customized identities, different
fonts/resolutions and mouse/input dispatch are outside these controls.

Mode is `dialog+30ac /10000` (decimal), **not** a16-bit shift. Source `4085b4..601`
starts with global `6c718c` and active-view type
`7cb6d8[7f0850]`. Types other than0/10 force modes0/5 to6, except type4 with
nonnull dialog parent. This bundle does **not** establish which active-view state
each naturally booted stock setup phase reaches; do not pick one table column as
unconditionally stock. Console-visible profiles explicitly admit console+9c bit0
after executing constructor `4660d0` through `466f98`, loading console2/console_x2.
Plain profiles stop its constructor at `4668ab` with the console hidden.

At1024 the original width rule makes FACTION `xs350` a525-pixel list and NAME
`xs440` a660-pixel form; FIRSTBASE `xs320` becomes480. This is layout scaling, not
font scaling. Plain mode0 outer sizes are547×218,682×114,502×114 respectively;
mode6 adds18 to width and9 to height. Console mode0 adds78 to the plain-mode0 width
and uses300-pixel height in these controls. Negative template x/y `-5` anchor the
plain NAME/FACTION right and bottom edges5 pixels from the screen; FIRSTBASE keeps
its explicit10,40 position in the plain profiles.

## Resolved frame, body and controls

For a plain-mode0 window of width W, the outer frame is `[0,0,W,H)` using style9;
caption outer `[4,4,W-4,28)` uses style3; caption inner
`[7,7,W-7,25)` uses style6; text layout rectangle is
`[10,10,W-10,22)`, with measured caption baseline21. FACTION body border is
`[7,35,538,171)`, NAME `[7,35,673,67)`, FIRSTBASE `[7,35,493,67)`:
these executed body borders use **style3**. They do not select the optional
style0 pane branch mentioned in the earlier general frame contract.

Mode6 uses the large outer bank `6a7a28`, adds9 to caption/body/control origins,
and puts the caption baseline at30. Its caption outer is
`[13,13,W-13,37)`, inner `[16,16,W-16,34)`, and text
`[19,19,W-19,31)`. Asset crop/tile definitions remain in the earlier font/frame
contract; this bundle supplies the actual layout calls.

Plain-mode0 child rectangles:

| Template | List/form parent | Edit child within form | Buttons, left to right |
|---|---|---|---|
| FACTION | `[10,38,535,168)` | — | OTHER `[10,183,181,203)`, OK `[187,183,358,203)`, Cancel `[364,183,535,203)` |
| NAME | `[10,38,670,64)` | `[101,1,663,21)` | Change Gender `[10,79,226,99)`, OK `[232,79,448,99)`, Cancel `[454,79,670,99)` |
| FIRSTBASE | `[10,38,490,64)` | `[53,1,483,21)` | OK `[10,79,490,99)` |

The edit coordinates are the native nested window rectangles, including the
reported right edge3 pixels past the form's width; do not silently normalize them
to a replacement inset. Full resolved native parent pointers, rectangles and
alternate rectangles are in the cases file. Button action IDs are1 for OTHER/
Change Gender,−1 for OK,−2 for Cancel. The source initializer `61da30` supplies
the default labels before font bank initialization; omitting it made early
development controls incorrectly fall back to OK for Cancel. Those early results
are not accepted evidence.

## FACTION row state and color

The original painter selects the first row without host mouse interaction, then
the probe sets native hover index1 and repaints that row through `625940`.

| State | Meaning | Frame style | Text palette index | Gamma-adjusted RGB |
|---|---|---|---|---|
| 0 | Ordinary | 27 (`7b0490`) | 0 | 0,0,0 |
| 1 | Hover | 28 (`7b04b8`) | 223 | 144,184,228 |
| 2 | Selected | 29 (`7b04e0`) | 0 | 0,0,0 |

`625f24` assigns2 for the selected-row condition; otherwise `625f30..37` compares
the native hover index and produces0/1. Selection takes precedence over hover.
`409600` indexes frame `7b0490 +40*state` and ordinary theme text pair
`8d09bc -8*state`. The parent-signature `752900` branch instead uses
`8d09f4 -8*state`; its three sampled color pairs are identical in installed
TEXT.PCX. Secondary color9 is converted to−1 by the theme loader. Independent
PCX decoding checks the color indices against the executed theme tables.

FACTION list width525; row i is `[0,19*i,525,19*i+16)` for i0..6. Native text
origins are x6 and baseline `14+19*i`; the descriptor right edge is519. Both
columns use the same Narrow16 bold font. The `|` delimiter is suppressed, and
the parenthesized descriptor is right-aligned. Row styles preserve the original
atlas scanlines; their center texture is not a newly invented solid color.

## Console attachments and clipping

Console-visible mode0 shifts form/list/button origins23 pixels right. At width W,
the outer frame call becomes `[23,0,W-55,H+5)`, caption outer
`[27,4,W-59,28)`, inner `[30,7,W-62,25)`, text
`[33,10,W-65,22)`. The backing surface clips at H, including that bottom+5.
`408360..38e` repositions the actual window to
`x=(screenWidth-windowWidth+32)/2` with native signed rounding, and
`y=screenHeight-windowHeight-[7cc53c]` (213 in these console constructor controls).

The executed `46a110` issues four1:1 sprite blits into the dialog backing surface:

| Sprite | Source crop in console_x2.pcx | Destination x,y |
|---|---|---|
| `7ecf64` | 713,1,15,529 | 0,H |
| `7ecf90` | 729,1,8,529 | 15,0 |
| `7ecfbc` | 738,1,8,529 | W−55,0 |
| `7ecfe8` | 747,1,15,529 | W−47,H |

All four have zero trim offsets. For the admitted H300 profiles, destination
clip is `[0,0,W,300)`: the outer two calls change **zero pixels** because their
top is exactly the exclusive bottom clip. The two inner calls change2391 and2393
pixels respectively, measured before/after the original blitter. Preserve these
calls and clipping semantics; do not force every rail into visible bounds.
Width800 uses a different atlas/crops and is not newly executed here.

## Evidence and limits

Public cases: `generic-dialog-layout-and-states-cases.json`. Private corrected
receipts and component PNGs: `tmp/reference/dialog-layout-native-development-20261019-v2/`.
The frozen bundle preserves all nine corrected receipts and their dependencies;
earlier development outputs are excluded. Verification checks nine literal outer
rectangles,24 faction row-state observations,12 rail observations, independent
TEXT.PCX samples, and13 instruction literals against the pinned PE.

Services explicitly modeled include allocation/file access, memory DC/DIBs,
palette/region selection, fixed cursor(0,0), inert cursor handles, empty window
enumeration and recorded invalidation. GetDC supplies a memory DC; the main HWND
is an opaque guest sentinel. Profile writes return success without host mutation.
CharUpperA delegates casing to host User32 on copied buffers. Original default
font bootstrap `630b40(7f0968)` and `61da30` run before theme/font/dialog setup.
Only1:1 top-down SRCCOPY StretchBlt is supported and asserted. No x86 layout,
frame, row-paint or sprite-blit routine is replaced, and no foreground window,
host pointer or live game session is operated. Modal input and naturally reached
view/console admission remain separate gates. For glyph fidelity use the separate
`dialog-gdi-synthetic-glyphs.md` handoff; bundled bold fonts differ from stock GDI
synthesis.
