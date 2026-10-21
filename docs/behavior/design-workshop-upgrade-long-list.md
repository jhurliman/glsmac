# WHICHUPGRADE2 long target lists: columns, no scrollbar

2026-10-21, pinned original terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Two bounded native geometry controls with13 and63 targets, plus valid
last-cell mouse selection. Same1024x768, mode0, visible-console profile
as the [two-target chooser](design-workshop-retire-upgrade-modal-layout.md).
No new font/pixel/model matrix. Flags are0x40; target values65 onward;
text is `Upgrade Target NN|2-3-1`, preserving the actual caller's pipe syntax.
[Reference](workshop_upgrade_grid_reference.py),
[receipts](design-workshop-upgrade-long-list-cases.json).

Checks pass for2 native layouts,2 last-cell selections,76 row rectangles,
10 source literals and4 keyboard source routes. Additive private freeze:
`tmp/reference/independent-workshop-upgrade-long-list-evidence-20261021/`,
14 payloads, manifest
`1143225421882fa88fae362be0f780c8e8ed19a70c799900b6ffac38f4650be5`.
This provenance paragraph was added after freezing.

The measurement/height/rounding clarification is source-only and checked
against these retained objects and the existing font129 receipt:
[source receipts](design-workshop-upgrade-grid-source-cases.json).
Its additive freeze has19 payloads at
`tmp/reference/independent-workshop-upgrade-grid-source-evidence-20261021/`,
manifest `08c1f3253dc50521cbf0c36b2565566d1d5855fef697d5e584835f6ef15510be`.
This source-freeze provenance paragraph was also added after freezing.

## The native solution is a grid

The generic kind16 TextList uses **multiple columns**, not a vertically
scrolling viewport. Its configured row limit is12. `627740@627870..6278df`
uses this exact formula for N>0:

```
columns = N <= 12 ? 1 : floor(N/12) + 1
rowsPerColumn = ceil(N/columns)
```

The `floor+1` is literal: N24 produces3 columns of8 rows, not2 columns
of12. Max rows per column is12; all N targets are laid out. N13 gives
2 columns x7 rows; N63 gives6 columns x11 rows. Ordinals flow **down each
column, then right**, preserving candidate insertion/global-ID order.
There is no scrollbar Window, arrow button, thumb, track rectangle or
scroll offset in either native control. Do not clip the data vector to the
number of rows in the first column.

For this font profile, row stride22, painted row height20, row top
`22*r+1`, glyph baseline `22*r+17`. Column stride is cellWidth+3; each
column's glyph starts columnX+3. Stats after `|` retain the caller's existing
right-aligned field within that cell.

### Unprototyped target marker in a chooser row

Action7 `442dfa..442e3f` appends the literal at **6a6fac**, exactly
**one ASCII space followed by `*`** (`20 2a`), when the selected stored
source has flag4 set and this target has flag4 clear. A target with flag4
set bypasses it; a source with flag4 clear also bypasses it.

The row builder has already appended the name, `|`, and5932d0 stats before
testing these flags. The marker therefore belongs to the **right stats
column**, after the complete stat/reactor notation. Native bounded receipt:

```
Upgrade Target 65|2-3-1 *
```

The620600 row value remains65. This marker does not enter the stored name,
does not precede the stats, and is not an ability token. It is separate
from an attack `*` or reactor `*2`. It also participates in the whole-row
width measurement described below. This is the chooser's5932d0 row path,
not the5935c0 economic-prompt string path.

[Marker receipt](design-workshop-upgrade-row-marker-cases.json) records the
one native row fragment and its declared source/target flag values.
It passes seven source literals and the exact two-byte marker check.
Private additive freeze: `tmp/reference/independent-workshop-upgrade-row-marker-evidence-20261021/`,
six payloads; manifest
`17734c25df2c2b60a0fa19ffd6518821020a42f19a1836bef65871a0c211ad11`.
This marker provenance paragraph was added after freezing.

Width is text dependent. `627740@6279df..627aac` first takes the maximum
native measured text width M, adds padding3 and gap3, and expands the
minimum660px allocation when necessary:

```
allocation = max(660, columns*(M+6))
cellWidth = trunc(allocation/columns)-3
listWidth = allocation-3
listHeight = rowsPerColumn*22
```

The measured maximum text width is174 for the63-target fixture's names/stats.
Longer names/localization can therefore widen the dialog further. The
13-target cell width327 is determined by minimum allocation660; the
63-target cell width177 is determined by text width174.

### Measurement font and literal pipe

`627740@627981..627995` calls **5f5590** on the whole resolved row string
and keeps the maximum return. Before this, `6277f1..62780a` applies the
list's data+70 font through5f3a70. The retained data+70 and bitmap+52c
are both **94f0c8**. This is **Arial Narrow -16 bold**, the final glyph
font129; it is not the preliminary default-font100 button pass.

Source `59f880@59f9c1..59fa0f` initializes the font matrix from94ec68,
160 bytes per size and40 bytes per variant. 94f0c8 is size slot7/variant0.
The size table6aead0 gives16, the >12 branch chooses face6affb0
(Arial Narrow), and the variant flags produce bold. The retained LOGFONT
for129 has height-16, weight700, face Arial Narrow.

5f5590 obtains the full length, then calls5f55c0. That routine handles
its brace/bracket/dollar markup but does **not** strip the pipe. Its scan
at5f5665..5f5697 treats `|` as ordinary text, and the later jump-table
entry for it also uses the ordinary path. The width chain ends at
**630870 → GetTextExtentPoint32A**. Thus M includes the literal pipe's
advance, together with the name and stat text. Do not measure the two
parts separately and remove that advance.

The painter independently finds `|` at627f6b..627f6e and splits its
left-name/right-stats presentation. That paint split happens after sizing.

### Height and final placement formulas

For this flags40, mode0, visible1024-console chooser, two buttons, no
illustration and nonempty list, let B be the native measured body height:

```
rawHeight = listHeight + B + 120
clientHeight = max(300, rawHeight)
```

The one-line body has B20, so **H=max(300,22*rowsPerColumn+140)**.
The general field sum at61d654..61d6da is listHeight +2*30e0
+max(bodyHeight,imageHeight) +buttonHeight +listGap +buttonGap
+topPadding +bottomPadding. Here those values are0, B,20,6,6,38,50.
4089f0 forces the buttonHeight20 before this sum. 4084f0@40888f..4088af
then enforces minimum300 and adds the deficit to30ec bottom padding.
At7 rows: raw294, bottomPadding56, H300. At11 rows: raw382,
bottomPadding50, H382. Longer wrapped bodies can increase B; this formula
does not claim a universal B20 for all stored names/localizations.

The generic61b9d0 calculation initially writes centered coordinates into
30b4/30b8. Those are **not the final Window position**. On first show,
the derived vtable+4 routine **4082f0@408360..40838e** overrides placement
when the console is visible and mode is0/5:

```
x = truncTowardZero((screenWidth - clientWidth + 32) / 2)
y = screenHeight - consoleHeight - clientHeight
```

The x code uses CDQ, subtracts the sign, then SAR1, implementing signed
division by2 toward zero. W1177 gives(1024-1177+32)=-121 → **-60**,
not floor(-121/2)=-61. W757 gives299/2→149. Native consoleHeight213
gives y=555-H, hence255 for H300 and173 for H382. No horizontal fit clamp
is hidden in this formula.

## Exact measured geometry

All child/frame rectangles below are dialog-local and half-open. The screen
Window is in screen coordinates. Both have one body baseline50, list top64,
and screen bottom555. The thin main painted frame extends5px below the
client, as in the existing native profile.

| Item | 13 targets | 63 targets |
| --- | --- | --- |
| Client | 757x300 | 1177x382 |
| Screen Window | `[149,255,906,555)` | `[-60,173,1117,555)` |
| List Window | `[33,64,690,218)` | `[33,64,1110,306)` |
| Grid | 2 columns x7 rows | 6 columns x11 rows |
| Cell width / column stride | 327 /330 | 177 /180 |
| OK action-1 | `[34,227,359,247)` | `[34,315,569,335)` |
| Cancel action-2 | `[365,227,690,247)` | `[575,315,1110,335)` |
| Main frame, style9 | `[23,0,702,305)` | `[23,0,1122,387)` |
| Caption outer, style3 | `[27,4,698,28)` | `[27,4,1118,28)` |
| Caption inner, style6 | `[30,7,695,25)` | `[30,7,1115,25)` |
| Body inner, style0 | `[30,35,695,58)` | `[30,35,1115,58)` |
| Combined body/list outer, style3 | `[27,32,698,221)` | `[27,32,1118,309)` |

Row frames use style29 for selected0, style27 for others. For ordinal i:
column=floor(i/rowsPerColumn), row=i%rowsPerColumn; painted rectangle is
`[column*columnStride,22*row+1,column*columnStride+cellWidth,22*row+21)`.

**Native overflow edge:** the63-target fixture really requests1177px on a
1024px screen and places the Window at x-60. It does not cap the width or
introduce horizontal scrolling. Thus the native grid contains all63 rows,
but this profile does not guarantee all rendered columns fit the screen.
The short-text diagnostic (`Target NN (2-3-1)`) instead measured811x382;
that is evidence of text-dependent width, not a competing fixed63-row size.
A port that keeps the chooser entirely on-screen needs an explicit
adaptation; a scrollbar should not be described as measured original behavior.

## Selection and addressing

The row value is the global design ID, not its column/row ordinal.
Selection starts at ordinal0/value65. Bounded native left-down controls:

| N | List-local click | Selected ordinal | Native getter value |
| --- | --- | --- | --- |
| 13 | (335,115) | 12 | 77 |
| 63 | (905,159) | 62 | 127 |

No scrolling precedes either selection. `623f30` resolves the selected
ordinal through the linked row records and returns the stored value field.
In retained native data, root=dialog+21d0; dynamic data=root+[root.vtable+8]
=dialog+2d70. Data+cc=count, +ec=selected ordinal, +f0=hover ordinal,
+4c=column count, +50=row limit12, +58=rowsPerColumn, +64=cell width,
+68=text height19, +44=gap3. Other linked-list traversal cursor fields
can move during painting; they are not scroll offsets.

The list-local input route maps x/y through the same cell geometry.
`6281b0` uses the y traversal for row and x/columnStride for column, then
adds column*rowsPerColumn. The controls above verify the final valid cell
in each grid. No new claim about clicks in unused cells is made.

Keyboard source `6283b0` changes this single flat ordinal, then repaints;
it does not scroll. Up38 decrements/clamps0; Down40 increments/clampsN-1.
Literal code37 (and keypad100) **adds rowsPerColumn** unless that would
exceed N-1; code39 (and keypad102) **subtracts rowsPerColumn** unless negative.
Those horizontal signs are as written in the native branch table; no timed
OS keyboard delivery was added to these geometry controls.
Enter/OK/double-click return the selected row value, while Escape/Cancel
return-1 under flags0x40, per the [generic input trace](design-workshop-modal-input.md).
