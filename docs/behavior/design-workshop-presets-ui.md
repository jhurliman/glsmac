# Vehicle Presets: labels, bounds, actions and entry order

Reuse the existing normal human Workshop parent capture. The fourteen
presets belong to the **console Window**, not the623x368 DESWIN bitmap.
In the established1024x768 profile, console origin is `(0,511)`; the
Workshop parent is separately `(216,168)`, size623x387. Do not subtract the
Workshop origin from console-local bounds or fit these controls inside
DESWIN. [Compact scalar fixture](design-workshop-presets-ui-cases.json).

## Exact label/action table

Captions are SCRIPT **DESIGNAUTO rows1..14**, read sequentially by
`446e82 → 5d61e0`, `446ed2 → 616690`, and constructor `446f15 → 621620`.
They do **not** have fourteen LABELS indices. Drawn text is uppercased.
Actions are decimal dispatcher IDs, not keyboard codes. All RECTs are
console-local, right/bottom exclusive, from existing native child headers.

| Action | SCRIPT row | Caption | Filter | RECT |
| --- | --- | --- | --- | --- |
| 9 | 1 | Garrison | 2 | `[786,87,898,108)` |
| 10 | 2 | Infantry | 3 | `[786,110,898,131)` |
| 11 | 3 | Artillery | 1 | `[786,133,898,154)` |
| 12 | 4 | Tank | 4 | `[786,156,898,177)` |
| 13 | 5 | Scout | 10 | `[786,179,898,200)` |
| 14 | 6 | Naval | 6 | `[786,202,898,223)` |
| 15 | 7 | Transport | 7 | `[786,225,898,246)` |
| 16 | 8 | Air Assault | 8 | `[901,87,1013,108)` |
| 17 | 9 | Air Defense | 9 | `[901,110,1013,131)` |
| 18 | 10 | Chopper | 17 | `[901,133,1013,154)` |
| 19 | 11 | Missile | 13 | `[901,156,1013,177)` |
| 20 | 12 | Buster | 14 | `[901,179,1013,200)` |
| 21 | 13 | Paradrop | 11 | `[901,202,1013,223)` |
| 22 | 14 | Amphibious | 12 | `[901,225,1013,246)` |

This is column-major construction, seven left then seven right. Width112,
height21, row stride23, column stride115. The final button bottom246 is
what the constructor creates, even though its initial allocation rectangle
ends245. Screen RECTs add511 to y in this profile. Captions describe UI
families; do not replace Tank with an inferred generated chassis name.

Heading uses LABELS **zero-based index668**, pointer-table offset`0xa70`,
one-based token row669: Vehicle Presets. Its caption panel is
`[786,63,1013,84)`, style6; outer frame `[783,60,1016,248)`, style3.
All fourteen buttons share tooltip LABELS **index827**, offset`0xcec`,
one-based token row828. Constructor `446f1a..446f4d` assigns that same
tooltip, not a caption-dependent tooltip. LABELS' leading count1131 is
metadata and must not be counted as token0.

## Font, color and backing

Final visible captions use **Arial height-14, weight700**, measured height16,
ascent13/descent3, centered on each112px button with **local baseline16**.
Their recorded local x positions in table order are
`19,21,18,37,32,33,14,11,10,22,28,28,17,11`.
The shared normal foreground is palette**65**, glyph color operand
**`0x10ff0041`**. The earlier parent-visual prose's `0x11000041` was a
transcription error; the retained native glyph journal contains decimal
285147201. Preserve the palette/color mode, not packed RGB semantics.

The heading uses the same Arial-14 bold in this profile, palette**240**,
operand`0x10ff00f0`, at console-local x836/baseline79. Initial constructor
draws before font assignment can use a default font; these are not the
final visible target.

The25-control style loop `4477d7..447845` includes these fourteen presets.
It assigns normal frame27 (`7b0490`), both hover and pressed frame29
(`7b04e0`), foreground65, and font object`7ece2c`. Shared normal/hover/pressed
frame art uses the established interface.pcx nine-slice recipe. See
[parent visual target](design-workshop-parent-visual-target.md).

## Input and engine command binding

The button stores its action at child`+a78`; shared mouse handling forwards
that action through the console parent. Dispatcher `442600` routes actions
9..22 through table`4436ec`, writes the corresponding filter and mode1,
and copies all five current draft fields before `44306d → 597de0(owner)`.
Use the [existing caller contract](design-workshop-action-contract-audit.md)
for exact seeding/readback, ability-slot reconstruction, NOPRESET feedback,
mode clear and repaint. No draft commit precedes the preset request.

Workshop keyboard handler **`443b40` contains no direct preset shortcuts**.
Its decoded tables cover Enter→Done4, Escape→Cancel8, Left/Numpad4→23,
Right/Numpad6→24, PageUp/Numpad9→page-9,
PageDown/Numpad3→page+9, Home/Numpad7→start, End/Numpad1→end;
character M/m or O/o→Obsolete6, U/u→Upgrade7. None dispatches9..22.
Do not invent G/I/A/etc. accelerators from captions or use action numbers
as keys. Generic OS focus traversal/keyboard activation is not newly
executed or specified by this source-only shortcut inventory.

## Capability refreshes and the inactive/New condition

Full entry `449630` calls **`577140@4496fe`**, then obtains cached mineral
factor through`4f9d60@44970e`, then calls
**`4490b0(effectiveSelection,initializeFresh=1)@44972f`**.

For the **inactive/New draft** branch with initializeFresh nonzero,
`4490b0` computes provisional components, seeds mode1/filter-1, and calls
`597de0@449522`. That generator performs its own
**`577140@597ede` before any submission**. Thus the supported full-entry
New path has two real capability refresh calls. Preserve those phase
boundaries and intervening mineral-factor/default selection work.

This is not an unconditional two-refresh rule for every call to`4490b0`:
active-record reopen follows`449184..` and bypasses the fresh recommendation
generator; inactive selection with initializeFresh0 also bypasses it.
The ordinary parent capture opens active Scout2 and therefore must not be
relabeled as a New-branch two-refresh execution. Its geometry/glyphs remain
valid evidence for the preset controls.

A preset click does not reenter full Workshop entry or`4490b0`; it calls
the producer directly, hence one producer capability refresh per click.
Both refresh routines here are`577140`; neither refreshes the retained air
counts through`5773c0`. See [checkpoint order](air-summary-checkpoint-order.md).

## Evidence scope

All14 bounds are checked against the first existing Morgan parent frame's
child headers, all14 final captions against the existing normal parent
glyph journal. No new screens, models, or14-action gameplay matrix were
run. Action/filter mapping, keyboard tables and conditional entry order
are source-only. The compact fixture includes hashes of both reused private
capture inputs. Additive private source/projection packet:
`tmp/reference/independent-workshop-presets-ui-evidence-20261021/`.

Manifest SHA256:
`594ef7b0083ad5839febb76c9c98d4b78340923067d6aef3647bc30a4820250a`.
