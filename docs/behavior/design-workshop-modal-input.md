# Workshop generic modal input and notation corrections

2026-10-21, pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
One illustrated Retire wrapper, twelve bounded native input controls, and
source traces. No engine retirement, model matrix or full game run.
The close routine is replaced with a receipt; active-dialog global `9c1264`
is supplied as original startup `61cb06` does. Independent key controls write
the selected index explicitly. Mouse controls execute original selection code.
[Compact receipts](design-workshop-modal-input-cases.json).

All12 input controls,11 source literals,2 retained wrap calculations and
7 prior glyph cases pass independent checks. Additive private freeze:
`tmp/reference/independent-workshop-modal-input-evidence-20261021/`,22 payloads,
manifest `13d061a9f64932622d93fcc6a4ee106f00046cf8d0a9b58e620e97e2932ccba0`.
This provenance paragraph was added after freezing. Earlier freezes remain
unchanged; this clarification supersedes their erroneous decimal notation.

## Acceptance

Retire passes flags0, has row values0 Never mind and1 Retire design, and
starts with row0 selected. The following table describes native results:

| Input | Selected row0 | Selected row1 |
| --- | --- | --- |
| Enter or Shift+Enter | Close, return0 | Close, return1 |
| Escape | Close, return0 | **Close, return1** |
| OK | Close, return0 | Close, return1 |
| Single left down | Select pointed row, stay open | Select pointed row, stay open |
| Left release | Stay open | Stay open |
| Double-click | Close with selected row value | Close with selected row value |

The surprising Escape behavior is intentional replication evidence:
`607910` dispatches Enter13 and Shift+Enter `0x1000d` to command-1,
Escape27 to command-2. `61d780` first reads the selected row value into
dialog+a40, then overrides it to-1 **only when command-2 and flags bit0x40**.
Both commands set dialog+3100=0 and close. Thus flags0 Escape accepts the
current selection. Retire's engine caller proceeds for a nonzero answer;
Escape after selecting Retire design therefore retires it. Do not impose
an unconditional Escape=Never mind rule.

WHICHUPGRADE2 uses flags0x40: Escape returns-1 regardless of the selected row.
The contrast control changes only that flag on the same native list object;
it does not claim a new full chooser layout.

`61b9d0@61cbf7..61cc1e` returns dialog+a40 after a normal zero modal-loop
result. Its nonzero/aborted loop path is separate from an Escape command.
`499b70` forwards the result unchanged. Other modified Enter/Escape values
are not recognized by the literal key comparisons above; no additional
shortcut behavior is inferred here.

## Mouse path

The native list is kind16, Window at dialog+2358. Left-down virtual+50
dispatches `6207b0` to `6281b0`; row stride is22. Down on the other row
changes the selected index and repaints. Its automatic acceptance branch
requires list flags bit4; the captured Retire list flags are0. Left-up
virtual+54 goes through `620aa0`, which does nothing for kind16.

Double-down has an additional path: `60f240@60f313..60f334` dispatches
Window+70, through `40a790` / `6208f0` / `628570` to callback `61d970`.
That callback reads active dialog `9c1264`, copies the selected row value,
sets modal result0 and closes. It does not issue command-1 through `61d780`.
The ordinary down event establishes selection before this double callback.
The native fragment test selects row1 with a single down/up, then executes
this actual double callback. This is source-routed input, not a timed OS
double-click measurement.

## Exact body wrap width

`61df90@61e28b..61e2d5` supplies this width to `5f3e40`:

```
W - 2*dialog[30e0] - dialog[30f4] - dialog[30d8]
  - dialog[30f0] - dialog[21c8]
```

Offsets are hex, values decimal. Retained illustrated Retire values are
W755, 30e0=0, 30f4=67, 30d8=206, 30f0=33, 21c8=0:
**449px at x239**. The old image-free diagnostic uses W757 and 30d8=0:
**657px at x33**. The text-frame interior is wider than this wrap measure;
do not derive659px from the frame. The illustrated758-wide notices use452px.

## Palette and localized OK identity

Both the caption and ordinary body use palette **226**, RGB(92,124,188)
in the retained palette. The journal's `0x10ff00e2` ends in hex e2,
which is226 decimal. Previous packet prose saying222 was a conversion error;
the journal and exported native images remain valid. Emphasis157 is unchanged.

Startup `5a59c0@5a5ac2..5a5b17` loads the LABELS token vector at `9c15b8`,
reads byte offset **0x194**, resolves the pointer through `62e860`, and
sets the OK default through `61abf0` / `9c1268`. Since 0x194/4=101,
this is zero-based token101, one-based TXTLoader row102, physical English
labels.txt line112. An array that retains the section count at index0
uses **labels[102]**. `LABELS194` in prior prose meant a hex byte offset;
it never meant decimal array index194. The fallback literal at `6b5254`
is used only when the initialized localized default is absent.
