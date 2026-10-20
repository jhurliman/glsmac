# RE-SMAC-SETUP-IDENTITY-TXT-001: customization defaults from faction TXT

Static continuation of [setup-dialog identity binding](setup-dialog-continuation.md).
Source is original parser `59d1d0`, included in the updated private source
receipt. Row indices below are zero-based after the `#FACTIONFILE` section
header; comma token indices are also zero-based.

| CUSTOMNAMES row | Definition offset | TXT source | Morgan default | Parser site |
| --- | --- | --- | --- | --- |
| Name | `+34` | row 0, token 5 | Morgan | `59d33f..59d353` |
| Formal | `+384` | row 0, token 0 | Morgan Industries | `59d299..59d2ad` |
| Noun | `+2e4` | row 0, token 2 | Morganites | `59d2c9..59d2e0` |
| Adjective | `+304` | row 4, token 0 | Morganic | `59dcf7..59dd13` |
| Title | `+4c` | row 6, token 0 | CEO | `59dd64..59dd7a` |
| Description | `+64` | row 6, token 1 | shrewd | `59dd7e..59dd95` |
| Gender | `+0` | row 0, token 6, **first byte** | 0 (M) | `59d357..59d375` |

Gender is 1 if the first byte is uppercase `F` **or lowercase `f`**, otherwise
0. This is not a whole-token equality test. Header token 3 controls the
faction noun's grammatical gender at `+2fc`, including a separate neutral
case; it is not the leader gender used for NAME/NAME2. Header token 4 is a
different grammatical numeric value at `+300`.

The parser reads one token at a time through `616790`; `616690` advances
to the next row. After the two preference rows, it advances to row 4 and
takes its first token for `+304`. It then advances to row 5 (the assistant
name/location fields), consumes three tokens, advances to row 6, and takes
Title then Description. Thus row4 token1 (`Morgan`) is not the customization
Adjective, and row6 token2 (`monopolistic`) is not Description.

The parser's stored maxima differ from the customization edit maxima:
Formal 39 bytes; Name/Noun/Title 23; Adjective/Description 127. The UI passes
23 for Adjective/Description. These source facts do not establish exactly
when the generic edit control truncates an overlong default; keep parser
capacity separate from the editable field limit.

## Restore Defaults replaces the accepted name too

Normal leader-name acceptance first copies the entered name to definition
`+34` at `5a19fb`. The customization dialog then reads that updated field
as its Name default. Restore Defaults takes the dialog's nonzero secondary
action, calls `59d1d0(definition,0)` at `5a1c45`, reloads graphics and jumps
back to rebuild all customization rows from the reloaded definition.
Consequently **the earlier accepted name is replaced with the faction TXT
name**, alongside the other defaults. It is not preserved in a separate
leader-name override. The reload is before the next modal acceptance; later
cancellation is not evidence that the earlier name returns. Slot zero skips
the file reload; playable faction slots 1..7 take it.

This is static data/control-flow evidence. No new original UI input,
customized startup or save/load control was executed. The updated receipt
extracts the seven original profiles' default strings and gender separately
from the quotation/rules display blocks.
