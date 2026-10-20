# FACTION descriptor alignment correction

**Use `x = 522 - textExtent` for the525-wide native FACTION list.** The previous
layout prose said519; that was a documentation error. Existing captured numeric
text coordinates and clips already use522. This correction does not change or
replace the earlier frozen capture. [Cases](faction-descriptor-right-alignment-cases.json)
bind the seven descriptors and repeated hover row to their original coordinates.

The original list's `+0x2c` padding field is2 in the saved dialog object.
Both ordinary and selected row paths shrink the right edge by `padding+1`:
`62613f..626154` and `626381..626396`. Thus `525-(2+1)=522`.
The `|` column delimiter is found and suppressed; the right column goes through
`6262ef` / `626531 → 5f6250`. `409600` draws row frames/theme colors; it does not
perform descriptor width measurement.

`5f6250` asks `5f55c0` for the bounded string's width, then subtracts the result
from rectangle.right at `5f62ce..5f62d1`. The ordinary font measurement reaches
`630870`, selects that font, calls **GetTextExtentPoint32A** at `6308c9`, and returns
the resulting SIZE.cx. It does not subtract a glyph ABC bearing, ink extent,
font overhang, or a three-pixel font correction. Markup-aware width measurement
can segment runs; these parenthesized descriptors have no such markup.

Fresh current-host GDI measurements use the exact captured Narrow−16 / weight700 /
monochrome LOGFONT, with only the installed game `arialn.ttf` privately registered,
matching the previous offscreen controls. `tmOverhang` is0. All eight whole-string
extents equal their summed individual character extents. This is a finite result
for this font/request/text set, not a universal additive-width rule for fonts.

| Descriptor | GDI width | Original TextOut x | x+width |
| --- | --- | --- | --- |
|(The Green)|82|440|522|
|(The Despot)|89|433|522|
|(The Scientist)|99|423|522|
|(The Mogul)|82|440|522|
|(The Survivalist)|111|411|522|
|(The Fundamentalist)|143|379|522|
|(The Humanitarian)|127|395|522|

The hover Despot repeats x433 with its changed color. Baselines remain
`14+19*i`. A port using `519-sum(advances)` lands exactly3 pixels left for every
row above. Correct the list inset/right boundary; retain the already verified
left origins, font and baseline.

`tmp/reference/check_faction_descriptor_alignment.py` verifies23 literal source
instructions, the existing capture/object hashes against their prior frozen
manifest, and eight fresh host string measurements. No original executable run,
full modal input, foreground desktop or screenshot comparison is added here.
Frozen supplemental evidence is
`tmp/reference/faction-descriptor-right-alignment-evidence-20261019/`.
