# Workshop manual ability chooser: list, layout and return contract

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Three narrow native row-production contexts, one bounded modal layout, five
isolated native input controls and nine cost-annotation fragments. Reuses
the existing accepted-picker transitions; no component or raster matrix.
Reference: [workshop_ability_chooser_reference.py](workshop_ability_chooser_reference.py).
Compact checked receipts: [cases](design-workshop-manual-ability-chooser-cases.json).

Private additive evidence directory:
`tmp/reference/independent-workshop-ability-chooser-evidence-20261021/`.
The layout uses supplied sprite-pixel boundaries and one-pixel minimum empty
backing DIBs in the memory harness. Those backing substitutions preserve
requested geometry but are not Windows CreateDIBSection behavior. Font metrics
and original layout/list/button calculations execute. **This is a conditioned
layout receipt, not a chooser screenshot or a fully initialized desktop claim.**

## Entry, admission and initial selection

The slot cards call `444e20(slot)`, with slot0 or slot1. Slot1 first requires
`5d17b0([9ba470],owner)`; failure shows SCRIPT tag `TWOSPECIAL` with the
required technology substituted. Any chassis with definition byte
`9b7145+144*ch!=0` is rejected with `MISSILESPECIAL`. Existing guard controls
already establish these no-mutation paths.

`444fb6..444fd1` calls the generic popup wrapper with SCRIPT tag
`UNITSPECIAL`, flags **200042 hex**, initial numeric argument -1, no default
text and the script filename token at `9c0f90`. The installed SCRIPT template
is at line1302: `xs400`, caption **Select Special Ability**, and no body prose.
It is a standard text-list modal, not a model grid or two-column ability tree.

`444fd6..44502c` first adds LABELS offset **220 hex**, installed text
**No special ability**, with stored return value **24**. The original loop
then examines ability indices0..23, ascending. Admit only if:

1. The ability's technology prerequisite passes for the Workshop owner.
2. `59bd40(index,chassis,weapon,armor)` returns compatible.
3. The other slot does not already hold this index.

The edited slot's existing index does not itself disqualify that row. Unlike
component pickers, unknown or incompatible abilities are omitted, not shown
as silhouettes. The other-slot test ignores only the slot being edited; it
does not inspect the draft mask as a duplicate substitute.

There is **no assignment of current-slot ability to the list selection**.
The generic list starts at ordinal0, selecting No special ability. In the
native all-tech Infantry/Laser/Plasma/Fission control with slots `[9,18]`,
slot0 produces values
`[24,1,2,3,4,5,8,9,10,11,12,14,15,16,17,20,22]`; slot1 replaces9 in that
set with18. The layout's selected index is0, hover=-1. With no granted
technologies and empty slots, the only row is24; the dialog still opens.
Do not silently preselect the current ability or omit the None row.

## Row text, cost annotations and decorations

The ability record is28 bytes. With base `9b6c68+28*index`, the first token
is the name, `+4` description, `+c` signed cost code, `+14` flags, `+18`
prerequisite. Tokens resolve through `62e860`, not through the unit-name
abbreviation field. The admitted row has this raw representation:

```text
localizedName + ": " + localizedDescription + "|" + localizedCost + ": " + annotation
```

`|` is the text renderer's left/right separator: the ability and description
are left-aligned; the cost annotation is right-aligned on the **same row**.
It is not a newline. The None row has only its label, with no cost suffix.
Native font calls separately record the two spans at the same baseline.

Let W be unsigned weapon **cost byte** `9b6fb3+16*weapon`, A unsigned armor
**cost byte** `9b6f12+16*armor`, and S raw chassis speed byte
`9b7140+144*chassis`. These are not the combat-strength helpers, reactor-adjusted
air speed, displayed movement or the final faction mineral factor.

| Ability cost code | Display quantity | Parenthesized annotation |
| --- | --- | --- |
| -1 | clamp(trunc(W/A),0,2) | Attack/Defense |
| -2 | W-1 | Attack-1 |
| -3 | A-1 | Defense-1 |
| -4 | S-1 | Speed-1 |
| -5 | W+A-2 | Attack+Defense-2 |
| -6 | W+S-2 | Attack+Speed-2 |
| -7 | A+S-2 | Defense+Speed-2 |
| >=0 | cost code, plus1 for flags400 hex on land | domain name if flags400 is set |

Only -1 has that clamp. Negative code branches do not add the land surcharge.
For the nonnegative flags400 case, a Sea/Air chassis keeps the base quantity
but still appends its localized domain name. Below -7 the jump-table default
skips the annotation; do not manufacture a normalized cost. The native ratio
path divides by A directly; malformed zero-cost custom armor is not covered
by a protective UI guard in this routine.

All nine native annotation fragments match the reference for W=2,A=3,S=1.
Stock examples in the row receipt include Deep Radar cost1 with Land marker,
Comm Jammer cost0 with Attack/Defense, and Artillery cost2 with Defense+Speed-2.
These displayed cost factors are not mineral prices or literal percentage text.

The ability list uses generic list kind2 (`dialog+2350=2`). Row decorator
`409600` paints the shared selected/hover/ordinary row frame and text colors.
Its bitmap-icon branch is conditional on **kind1**, so it is not taken here.
There is no native ability icon, CVR model, checkbox, second-slot badge or PSI
stat decoration appended by this chooser. Use the existing generic row-frame
assets/styles27/28/29 and theme color pairs; selected state takes precedence
over hover. This source conclusion reuses the generic list frame contract.

## Conditioned 1024x768 layout

Inputs: console visible, mode0, installed MainFontSize16, the17-row all-tech
control above. The original `xs400` gives a600-pixel list layout width at1024.
The final outer dialog is **[178,210,878,555)**, size700x345. The installed
caption is uppercased; recorded caption baseline21 is dialog-local. Do not
copy the intermediate working fields `+30b4/+30b8` as final position.

| Element | Native rectangle / metric |
| --- | --- |
| List drawable client | [0,0,600,225) |
| List window parent placement (`+14c`) | [33,38,653,263) |
| Vertical scroll control, dialog-local | [657,21,681,266) |
| Scroll up/down children, control-local | [0,0,24,24), [0,221,24,245) |
| OK | [33,278,330,298), action -1 |
| Cancel | [336,278,633,298), action -2 |
| Selected ordinal / first visible ordinal | 0 / 0 |
| Visible row capacity | 12 |

The list client and parent-placement widths are different native fields;
preserve the distinction when matching the widget's chrome and drawable area.
Button construction passes negative x anchors; their final stored rectangles
above are the resolved placements. Do not use those raw negative arguments.

For visible row j, frame rectangle is `[0,19*j,600,19*j+16)`; text clip is
`[6,19*j,597,19*j+16)`. Left span starts x6. Right span ends at x597 minus
its measured width. Baseline is **14+19*j** in list-client coordinates.
For example the first cost span starts x500, the ordinary short cost x548,
and Comm Jammer's longer annotation x429. These x values depend on text width.
The native font is **Arial Narrow -16 bold**, measured height20, ascent16,
internal leading4 in this receipt. Treat font/layout preferences as inputs;
this does not make that font an unconditional hard-coded Workshop component font.
Long text is clipped to the row; it is not wrapped into a taller multi-line row.

## Input, cancel, acceptance and repaint

Mouse hover changes list hover state, not the Workshop draft. A normal row
click selects it; the list's auto-accept flag4 is clear in this popup. The
left double-click message203 routes through the standard left-button path
(`6098eb→60b1b0→Window+144/60f240→6075d0→list+50/624ee0`); it does not add
an ability-specific confirmation. The acceptance call in `624ee0@625393`
requires that separate flag4. Do not invent double-click-to-commit behavior.
This is source tracing, not a timed desktop double-click test.

Arrow handling reaches `6253c0`: Up/Down move selection by one and clamp at
the list ends; the helper also maintains the visible first row and repaints
old/new selections or refreshes after scrolling. Left/Right return without
moving in this one-column list (`625556/6255a0` test column count1).
Character matching passes through the existing
ToAscii branch and cycles from the current row looking at the first character
of row text; do not implement a new substring filter. Tab follows generic
focus traversal. No ability-specific keyboard shortcut table is installed.

`607910` maps Enter13 (also1000d hex) to action-1 and Escape27 to action-2.
`61d780` obtains the selected row's **stored value**, not its display ordinal.
Cancel with this popup's flag40 forces dialog result -1. Native controls prove
Down0→1 returns ability1; OK and Enter return1 and request one modal close;
Cancel and Escape return-1 and request one close. The draft remains byte-exact
during these isolated input controls; close/presentation is supplied.

Back in `444e20`, negative modal result returns without mutation or repaint.
Accepted value>=24 becomes -1. The caller changes only the requested slot,
then rebuilds mask by OR-ing both nonnegative slots. It does not compact,
sort, run automatic suggestions or call `443d20`. Thus clearing slot0 beside
18 leaves `[-1,18]`; choosing18 beside9 can leave `[18,9]`.

Accepted repaint order at `446236..446264` is **Ability1, Ability0, Main,
4487c0, 43d730, Window virtual+fc**. The retained accepted-transition evidence
already verifies all consumers see the final tuple. The later general full
Workshop painter has its separate cleanup contract; the preview timer does not.

## Source addendum: localization and short lists

Source-only clarification, 2026-10-21. This adds no layout/emulation matrix.
LABELS offsets below address the token vector at `9c15b8`, resolved through
`62e860`. Section rows are **one-based TXTLoader rows**, with
`offset = 4*(row-1)`; physical file lines are supplied only for the pinned
installed English file and must not be used as loader indices.

| Token | Vector offset (hex) | LABELS row | Installed physical line |
| --- | --- | --- | --- |
| Cost | 160 | 89 | 99 |
| Attack | 1f0 | 125 | 135 |
| Defense | 154 | 86 | 96 |
| Speed | b14 | 710 | 720 |
| No special ability | 220 | 137 | 147 |
| OK | 194 | 102 | 112 |
| Cancel | 198 | 103 | 113 |

`444e20` directly resolves the annotation and None tokens. Examples are Cost
at `445184..445190`, Attack at `4452db..4452e7`, Defense at
`445346..445352`, and Speed at `445681..44568d`; repeated annotation branches
reuse the same offsets. The domain annotation for flags400 uses the already
loaded triad token array `9595c4+4*triad`, rather than a LABELS lookup here.

OK/Cancel follow startup localization: `5a59c0@5a5a65..5a5abd` resolves
LABELS198 and calls `61ac80` to set Cancel (`9c126c`);
`5a5ad5..5a5b17` resolves LABELS194 and calls `61abf0` to set OK
(`9c1268`). `61b9d0` copies these defaults into the dialog buttons.
`61da30` also supplies literal English fallbacks at `6b524c`/`6b5254`;
those literals are not a reason to bypass the initialized localization values.

| SCRIPT section | Installed section-start line | xs | Caption identity | Body/substitution |
| --- | --- | --- | --- | --- |
| UNITSPECIAL | 1302 | 400 | Select Special Ability | Empty body; caller supplies rows |
| TWOSPECIAL | 1292 | 440 | Chief Engineer | Explains the two-ability research requirement; `$TECH0` is the required technology name |
| MISSILESPECIAL | 9646 | 440 | Chief Engineer | Explains missile special-ability prohibition; no caller substitution |

There is no numeric LABELS row for these captions: resolve each named SCRIPT
section's `caption` directive. The second-slot guard runs first. On failure,
`444e88..444ea1` reads technology ID `[9ba470]`, obtains its name token from
`9b7bac+44*tech`, and calls `633880(0,nameToken,-1,-1)` before opening
TWOSPECIAL. Thus TECH0 follows the loaded rules technology; do not hard-code
the stock technology's English name. The following chassis guard reads
`9b7145+144*ch` and opens MISSILESPECIAL at `444f39..444f51` without setting
a substitution. Both guards return before constructing chooser rows.

For the existing **console-visible mode0, 1024x768, MainFontSize16** profile,
let `N` include the None row and let `R=min(N,12)`. N is always at least1.
The following are source-derived formulas, not newly executed short-list
receipts:

- `624660@624737..62474e` selects R; `6249cc..6249e2` gives list drawable
  height **L=19*R-3**. The row stride remains19 and ink height16. The drawable
  width stays600 in this profile, and the list creation anchor stays `(33,38)`.
- `61cc80@61d654..61d6e2` gives initial outer height **H0=L+120**:
  list L, two gaps6, buttons20, caption/top38, bottom50. Width stays700.
  `4084f0@408889..4088af` then sets **H=max(300,H0)** and increases the
  bottom field `+30ec` by `H-H0`. It does not enlarge the list or button height.
- `61bff0..61c01f` computes button y as `H-bottom-20`;
  `408a20@408a56` adds3. Therefore **buttonTop=L+53** even while H is clamped.
  OK remains `[33,buttonTop,330,buttonTop+20)`; Cancel remains
  `[336,buttonTop,633,buttonTop+20)`.
- Scroll admission requires rows greater than capacity
  (`624a50..624ae1`, `624bb9..624bdd`); **N<=12 has no vertical scrollbar**.
  There is no disabled scrollbar merely to fill the 17-row rectangle.
- Final console show placement is `4082f0@408360..40838e` through `606650`:
  **left=trunc((screenWidth-W+32)/2)** and
  **top=screenHeight-consoleHeight-H**. Canvas `+4c8` stores **negative H**.
  This receipt's console height is213, so left178, bottom555, top555-H.
  This is the final show placement, rather than the earlier working position.

For **None only**, these formulas give L16, H0=136, H300, outer
`[178,255,878,555)`, buttons `[33,69,330,89)` and `[336,69,633,89)`.
Nine rows still give H300; ten give H307; twelve give H345 with buttonTop278.
These examples are arithmetic consequences of the traced formulas. In this
profile unused minimum-height space remains below the buttons.

The constants above are conditioned on the recorded font/theme. For a different
profile, list height is `stride*R-gap+topInset+bottomInset`
(`6249cc..6249e2`); the generic outer sum uses `+30c4` list height,
`max(+30d4,+30dc)` body/image height, `2*+30e0`, button height `+30d0`,
one `+30e4` gap for each nonempty list/button block, and `+30e8/+30ec`
top/bottom margins. The 300 minimum requires a null explicit parent, visible
console and mode0 or5. It is not a universal popup minimum. In particular,
`408763..40877e` chooses bottom30 at screen width exactly800 and bottom50
otherwise; the 1024 profile uses50.

### Frame, rails and separate console scrollbar

The console scrollbar is deliberately outside the narrowed main frame. The
chooser uses the same generic painter `409760` as the short form dialogs;
there is no row-count branch that widens the main frame to contain it.
`4099ea..409aa1` copies the backing RECT and, for visible-console mode0/5,
adds23 to its left, subtracts55 from its right and adds5 to its bottom.
`409ab4..409bc4` constructs the caption rectangles. With W700 and height H:

| Frame call | Dialog-local RECT | Style |
| --- | --- | --- |
| Main outer | `[23,0,645,H+5)` | 9, bank `7b01c0` |
| Caption outer | `[27,4,641,28)` | 3, `7b00d0` |
| Caption inner | `[30,7,638,25)` | 6, `7b0148` |
| Caption text layout | `[33,10,635,22)` | Text |
| List border | `[30,35,636,41+L)` | 3, `7b00d0` |

The list border follows `409c3f..409dad`: with no body/image, take list origin
`(33,38)`, add calculated drawable size `(600,L)`, and expand all sides by3.
It does not expand to enclose the separate console scrollbar. The main backing
clip remains `[0,0,700,H)`; the outer frame's bottom+5 is clipped.

Console attachment calls in `46a110@46a4df..46a5aa` place the same four rail
sprites as the earlier generic-dialog receipt. At this profile they are:

| Sprite | Destination x,y | Installed x2 crop size |
| --- | --- | --- |
| `7ecf64` | `0,H` | 15x529 |
| `7ecf90` | `15,0` | 8x529 |
| `7ecfbc` | `645,0` | 8x529 |
| `7ecfe8` | `653,H` | 15x529 |

The two outer destinations start at exclusive clip bottom H. Preserve that
source arithmetic rather than moving them upward to make them visible.

For an overflowing kind2 list, `4082f0@4083af..4083df` separately calls
`46ac10(dialog,dialog+3230)`. This builds the console's right-side attachment
and sizes/positions the **auxiliary scrollbar**, rather than extending the
main frame. `46b29e..46b2c2` sets its width24 and places it at
**`(W-43,21)`**, giving x657. The existing 17-row receipt records its height245,
RECT `[657,21,681,266)`, and child buttons `[0,0,24,24)` and
`[0,221,24,245)`. Its height is calculated by the attachment's sprite/tile
arithmetic (`46b012..46b03e`, then subtract10 at `46b2a5`), not by stretching
it to the outer dialog bottom. This explains why the scrollbar extends past
main-frame right645 while remaining within the full700-wide backing surface.
The `N>capacity` admission above omits this attachment entirely for N<=12;
the main frame and four basic rail placements still apply.
