# Workshop Retire prompts and multiple-target Upgrade chooser geometry

**Retire correction:** the Retire cases below are direct generic-template
diagnostics without the caller's unit_sm.pcx argument. Original499b70 loads
that illustration and changes the real Retire layout to755x300, choices
`[33,164,688,208)`, OK `[33,217,688,237)`. Use the
[actual wrapper supplement](design-workshop-upgrade-confirmation-layout.md)
for Retire; the image-free multiple-target chooser below remains valid.

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Four bounded native template/layout controls: Retire cached counts0/1/2,
and the two-target Upgrade list retained from the already checked action
fixture. Uses original template parsing, substitution, fonts, geometry and
GDI painting through `61caf9`, before the modal loop. No model generation,
production edits, live desktop capture or new engine mutation matrix.
[Compact receipts](design-workshop-retire-upgrade-modal-layout-cases.json),
[bounded geometry reference](workshop_action_modal_reference.py).
All four layouts and four caller-source literals pass independent checks.
Private additive freeze contains22 payloads at
`tmp/reference/independent-workshop-retire-upgrade-modal-evidence-20261021/`;
manifest SHA-256
`c3491945b5db7bd7ecad7d1db702593ccfe108c71307e0d24867eaf60ae831b9`.
This provenance paragraph was added after the immutable freeze.

The [engine/action contract](design-workshop-explicit-upgrade-and-retire.md)
continues to own admission, quotes, conversion, cancellation, network and
saved-record authority. This packet fills its requested Retire/chooser
presentation gap; Upgrade's later currency/prototype prompts are separate.

## Caller arguments and real retained inputs

Retire action0 at `442900..442993` substitutes the selected **stored name**
using `633910(0,name,-1,-1)`. Read the cached own-unit byte, not unsaved draft
components, a live roster recount or the production-reference count.
Zero chooses SCRIPT RETIRETYPE2. Nonzero substitutes `NUM0=count` through
`633860` and chooses RETIRETYPE. Caller `499b70` loads the generic template
with max -1, no edit buffer, flags0; its native modal callback is54e8e0.

Both templates specify xs440 and caption Chief Engineer. Zero body asks
whether to retire the quoted design; nonzero adds the cached count and warns
that the units still operating will be disbanded. The stock template has
**two selectable rows**, values0 and1:

| Value | Exact choice text for retained source |
| --- | --- |
| 0 | Never mind. |
| 1 | Retire "Upgrade Source" design. |

The single button is localized OK (LABELS byte offset `0x194`, zero-based
token101; `labels[102]` when the array includes its count), action -1. There is no
separate visible Cancel or Yes/No button pair. Row0 is initially selected
(native state2); row1 is initially normal (state0). The action handler exits
for returned0 and proceeds for nonzero. Do not silently turn selecting the
Retire row into a separate immediate engine action before generic acceptance.
These are pre-loop layout receipts, not new mouse/keyboard interaction tests;
retain the generic modal input contract rather than inferring Escape behavior
from the absence of a visible Cancel button.

Upgrade action7 loads WHICHUPGRADE2 with flags40 hex (`442c27..442c3b`).
It inserts admitted global target IDs as row values at `442e45..442e52`.
Zero targets uses NOUPGRADE; one target bypasses this chooser; two or more
enter `61b9d0(0,54e8e0)`. Negative chooser result exits before conversion.
Retained native `upgrade_many_cancel` rows are reused byte-for-byte:

| Global row value | Native stored row text |
| --- | --- |
| 65 | `Upgrade Target 65|2-3-1` |
| 66 | `Upgrade Target 66|2-3-1` |

The source stored name is Upgrade Source. The stock template caption is
Operations Director, and body renders `Upgrade Upgrade Source to which design:`.
The repeated Upgrade is the action verb followed by the actual stored name,
not a placeholder substitution error. First target65 is selected initially.
OK action -1 and Cancel action -2 are visible. The native return path
`61cbf7..61cc1e` uses flags40 to turn modal cancellation into -1; accepted
result comes from dialog `+a40`, carrying the global target ID.

## Exact captured profile: 757x300, not Rename's 760x300

Inputs: 1024x768, mode0, native visible-console rails, MainFont16, installed
stock SCRIPT/text/interface assets, source name Upgrade Source. All four
Window bounds are screen `[149,255,906,555)`; client `[0,0,757,300)`.
The actual generic width is **757** for these dialogs. Neither template xs440
nor the [Rename](design-workshop-rename-and-obsolete.md) dialog's760 is the
correct final width here. Other text/font/resolution profiles must run the
generic measurement rules; these coordinates are a measured profile gate.

Shared dialog-local frame rectangles:

| Frame | Rectangle | Style address / index |
| --- | --- | --- |
| Main | `[23,0,702,305)` | 7b01c0 /9 |
| Caption outer | `[27,4,698,28)` | 7b00d0 /3 |
| Caption inner | `[30,7,695,25)` | 7b0148 /6 |
| Body inner, one line | `[30,35,695,58)` | 7b0058 /0 |
| Body inner, two lines | `[30,35,695,78)` | 7b0058 /0 |
| Combined body/list outer, one line | `[27,32,698,111)` | 7b00d0 /3 |
| Combined body/list outer, two lines | `[27,32,698,131)` | 7b00d0 /3 |

The Main frame extends five pixels past the client bottom. Controls stay near
the text; they are not bottom-aligned to300.

| Case | Body baselines | List child rectangle | Buttons, dialog-local |
| --- | --- | --- | --- |
| Retire0 units | 50 | `[33,64,690,108)` | OK `[33,117,690,137)` |
| Retire1 unit | 50,70 | `[33,84,690,128)` | OK `[33,137,690,157)` |
| Retire2 units | 50,70 | `[33,84,690,128)` | OK `[33,137,690,157)` |
| Upgrade2 targets | 50 | `[33,64,690,108)` | OK `[34,117,359,137)`; Cancel `[365,117,690,137)` |

With the retained name, the nonzero Retire body's first wrapped line ends
`will be ` (including its trailing space); the second line is `disbanded.`.
Count1 uses `1 unit`; count2 uses `2 units`. Native substitution supplies the
plural form before wrapping. A longer localized/stored name can wrap
differently; do not choose line count solely from zero/nonzero count.

Each list is657x44. Child-local row rectangles are `[0,1,657,21)` and
`[0,23,657,43)`:20px frame height,22px stride. Row0 state2 uses style
7b04e0 (index29); row1 state0 uses7b0490 (index27). This selected-row style is
not a replacement for the generic hover/pressed state transitions.
Retire's OK is657x20. Upgrade's buttons are325x20 with a six-pixel gap;
retain the measured one-pixel horizontal offset in its first button.

## Text fonts, palette indices and column alignment

Caption is Arial -14 bold, baseline21 on the dialog DIB:
CHIEF ENGINEER x303; OPERATIONS DIRECTOR x279. Caption and body use the
resolved palette index222 (`10ff00e2` in the GDI journal).
Body is Arial Narrow -16 bold, x33, baseline50 then70,20px line step.
List and final button text use Arial Narrow -16 bold and palette0
(`10ff0000` journal). The journal also contains preliminary default-font
button paints; use the later Arial Narrow glyphs for the final baseline.

List text has child-local baseline17/39, with name/choice text at x3.
For Upgrade, `|` is column markup, not a drawn glyph: the actual native row
shows the name at x3 and `2-3-1` at x623, right-aligned to x654 (width minus3).
Do not center the stat field or draw the raw bar. These row values remain
65/66, not visual ordinals0/1.

Final OK/Cancel text positions within their own button DIBs:

| Button | x | Baseline |
| --- | ---: | ---: |
| Retire OK | 318 | 16 |
| Upgrade OK | 152 | 16 |
| Upgrade Cancel | 139 | 16 |

The private PNGs are the dialog's own native DIB, excluding separately owned
list/button DIB compositing. Geometry, child records and all glyph journals
are retained. They must not be presented as a full composite screenshot or
used as a whole-modal pixel target without composing the child surfaces.
UnionRect is supplied as scalar RECT union, reusing the existing Workshop
service boundary; template parsing, font measurement and geometry execute
original x86. The probe stops before54e8e0 callback delivery/modal interaction.
