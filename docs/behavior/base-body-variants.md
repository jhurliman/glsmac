# Base body selection and Detail population glyphs

The original uses the **same body selector for ordinary zoom0 and Detail zoom-7**.
With standard rules, ordinary populations8,9 and10 select the same University
body. Headquarters increments the selected size class, capped at3. Defense and
land/sea are independent selectors. These inputs must be compared before
attributing a screenshot difference to population thresholds.

[85 native indexed controls](base-body-variants-cases.json) cover74 complete
Detail frames and11 direct ordinary zoom0 Base-helper frames. The
[reference](base_body_variants_reference.py) reconstructs all24 University banks,
120,058 Detail source samples, population geometry/colors and complete indexed
frames. Run `tmp/reference/check_detail_base_variants.py --verify-only` with the
emulation venv and installed assets. Production code is untouched.

## Exact size and defense rules

Let P be the signed population byte in measured range0..127, C the rules
population limit without Hab Complex, and D the limit without Hab Dome:

```
size = 0  if P < 4
       1  if P < max(7, C+1)
       2  if P < max(10, D+1)
       3  otherwise
if Headquarters: size = min(3, size+1)
```

Stock C=7,D=14 gives thresholds4,8,15. Thus the corrected port thresholds
`[1,4,8,15]` agree for normal positive non-HQ populations. The selector also
assigns diagnostic population0 to size0. Controls cover both sides of thresholds,
HQ at each tier, and explicit rules overrides2/3 and10/20. Do not hardcode stock
thresholds when supporting changed rules. Overflowing rules values and population
bytes128..255 are outside this contract.

Defense is2 when Tachyon Field is present, otherwise1 when Perimeter Defense is
present or the faction owns Citizens' Defense Force, otherwise0. Tachyon alone
selects2; it also wins over Perimeter/CDF. CDF compares the owner of its project
Base with the owner of this Base. A project belonging to another faction does
not supply walls. CDF does not perform the HQ size increment.

Sea art is selected whenever the retained altitude band is below3, including
deep/ocean/shelf controls. The art family is the Base's separate retained art
family byte, not inferred from the current owner. These pixel controls explicitly
use family3/UNIV while varying owners for population colors.

## Stock University banks and placement

There are four size columns, two land/sea groups and three defense rows per group.
For size S, sea flag W and defense V, the owned UNIV.PCX logical crop is:
`(1+101*S, 1+76*(3*W+V), 100, 75)`.
Bank index within the family is `3*(S+4*W)+V`. The cases publish all24 retained
sizes, offsets and source hashes; equal bounds do not imply equal pixels.

Some useful retained source controls, before zoom scaling:

| Selector | Width×height | Retained offset |
| --- | --- | --- |
| Land pop1, no HQ/defense |41×38|27,17|
| Land pop4, no HQ/defense |62×49|17,12|
| Land pop8/9/10, no HQ/defense |87×58|6,8|
| Land pop9 with HQ, no defense |91×67|2,0|
| Land pop9, Perimeter or Tachyon |96×59|2,7|
| Sea pop9, no HQ/defense |97×67|0,8|

Use the actual object dispatcher's Base anchor: projector X and projector Y minus
one native vertical step. Apply retained offset to that anchor. Scale is16/16 at
zoom0 and9/16 in Detail. Preserve the established integer sampling phase, key9
transparency and source0 shadow lookup. The destination origin truncates the
complete anchor-plus-scaled-offset sum towards zero; width/height scale separately.
Do not rescale the full100×75 canvas or center the retained bounding box anew.
See [Detail placement/clipping](detail-minimap-painter.md) for negative anchors.

These results do not establish which HQ, defense, art-family or terrain inputs
were present in the user's GOG screenshot. They supply exact comparable inputs
and rule out a population8-versus9-versus10 tier difference under the same state.

## Population number layout

The number is signed decimal text rendered with a native font request. It is
not a fixed numeral sprite. Font selection requests Arial bold, LOGFONT height-9
in Detail and-16 at zoom0. Current Windows metrics give logical layout height
`ascent-internalLeading` of7 and12, respectively.

The box width is **measured width of “8”** for population below10, or **“88”** for
population10 and above, plus5. It does not measure the actual number string.
Box height is logical font height+4. Text baseline is anchor+(3,logicalHeight+2),
left/baseline alignment24, transparent background. Native box rectangles are
half-open. In the captured font environment:

| Mode | One-digit box | Two-or-more-digit box | Baseline offset |
| --- | --- | --- | --- |
| Detail |10×11|15×11|3,9|
| Zoom0 |14×16|23×16|3,14|

Population100/127 still receives a two-digit box. Its full string can draw beyond
that box: text clips to the viewport, not to the population box. Native controls
verify this edge case. Host metrics and glyph pixels are current Windows GDI;
LOGFONT/TEXTMETRIC data and whole-frame hashes are retained. Historical Windows
font-pixel parity and cross-platform substitute-font parity are not claimed.

## Population box colors and access

Color selection occurs after Base visibility admission. These cases use known,
visited Bases; they do not change the earlier foreign-Base visibility rules.

A valid unit marker on the Base square forces the **filled** population style,
even for an owned Base or an otherwise accessible foreign Base. This means the
retained unit bit and low owner nibble below8; consistent Scout fixtures are used.

Without a unit on the Base square, style is unfilled for an owned Base, an
owner-to-viewer Pact, viewer-to-owner infiltration, all-map revelation, Empath
Guild ownership, governor access, or a neighboring viewer unit. Otherwise it is
filled. Treaty alone does not grant the Pact style. The Pact direction matters:
the reverse relation alone stays filled. A neighboring foreign unit does not
grant viewer access. Editor bypass is present in source but outside these new
pixel controls.

Unfilled style has a black border, retained background inside, and the owner's
text palette slot. Filled style uses the owner's primary-color inner fill and
black text. If the owner's faction filename is exactly `SPARTANS`, filled style
instead uses its contrast-color border and text index44. The Spartan unfilled
case keeps the ordinary unfilled rule. Palette slots are independently sampled
from stock PALETTE.PCX, not guessed RGB constants.

## Asset and fixture bounds

The pinned GOG terran.exe is the same original as prior receipts. Stock alpha.txt
SHA256 is `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`;
UNIV.PCX SHA256 is
`84ccbaa954808a6fe14dc6f3f41850fbb37350f1af34359a21b8820ecf3019a5`.
Other faction body assets are not pixel-certified by this matrix. Owner filenames
are explicit fixture inputs taken from stock #FACTIONS; the harness does not
claim full faction setup. Facility/intelligence helpers execute natively.
Main controls execute the complete shared Base helper directly on a diagnostic
bitmap; they do not rerun an ordinary viewport or the founding sequence.
