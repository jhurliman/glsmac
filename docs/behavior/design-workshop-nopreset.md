# NOPRESET modal and preset feedback order

An unmatched preset opens the original illustrated generic message, waits
for dismissal, then requests feedback39. A successful preset requests
feedback43 immediately. In this pinned executable's initialized stock
sound bank, **neither feedback slot has a sample binding**; do not invent
a success/error WAV mapping. [Scalar fixture](design-workshop-nopreset-cases.json).

## Caller and template

After tuple readback and ability-selector reconstruction, `4430f0`
compares controller mode`9498e0` with1. If mode>1, it pushes decimal43
(`0x2b`) at`4430f8` and jumps to the common sound request.
Otherwise`443112` calls:

```text
499b70(currentScriptFile, "NOPRESET", 0, "unit_sm.pcx", 54e8e0)
```

The wrapper's return value is ignored. Only **after it returns** does
`44311a` push decimal39 (`0x27`). Both paths call
`451f20(soundManager=768b50, feedbackId)` at`443121`, then clear mode to0
at`443126`, invoke Workshop virtual+30, then virtual+fc. Do not play39
on opening the message, clear mode early, commit the draft, or add another
fallback proposal on dismissal. The three bounded caller slices for modes
1/2/3 verify these event sequences, supplying only the modal return and
feedback sink.

Installed SCRIPT NOPRESET specifies `#xs 320`, caption **Chief Engineer**,
and body **We cannot design that kind of unit with our current technology.**
It has no choices, edit buffer, or template illustration directive. The
**caller** supplies `unit_sm.pcx`; do not omit it because the text section
does not mention the file. Generic flags are0, max input length-1, edit
buffer null, callback`54e8e0`. One localized OK button, LABELS zero-based
index101 (pointer offset`0x194`), action-1. No separate Cancel button.

## Actual measured1024 profile

One original wrapper executes asset lookup/load, template parsing, generic
measurement and GDI painting before its modal event loop. Stock MainFont16,
mode0, visible-console rail admission, installed fonts/text/interface art:

| Element | Exact bounds / draw |
| --- | --- |
| Client | `[0,0,758,300)` |
| Screen Window | `[149,255,907,555)` |
| Illustration |200x120 at dialog-local `(28,32)`, `unit_sm.pcx` |
| Main frame, style9 /`7b01c0` | `[23,0,703,305)`; client clips bottom |
| Caption outer, style3 /`7b00d0` | `[27,4,699,28)` |
| Caption inner, style6 /`7b0148` | `[30,7,696,25)` |
| Body outer, style3 | `[233,32,699,61)` |
| Body inner, style0 /`7b0058` | `[236,35,696,58)` |
| OK child | `[33,167,691,187)`, width658, height20 |
| Caption glyph | CHIEF ENGINEER, x304, baseline21 |
| Body glyph | one complete line, x239, baseline50 |
| OK final glyph | child-local x318, baseline16 |

Caption font is **Arial-14 bold**; body and final OK font are **Arial
Narrow-16 bold**. Caption/body use palette226, operand`0x10ff00e2`;
OK uses palette0, operand`0x10ff0000`. The earlier preliminary default-font
OK draw is not the final target. Body line step is20 when localization
needs wrapping. The image's120px height controls the button's vertical
placement for this one-line body; do not move OK immediately under the
line or bottom-align it to300.

This is the shared generic frame/rail/text.pcx/interface.pcx system, not
DESWIN art or an in-Workshop card. Use the established
[frame/font recipe](generic-dialog-fonts-and-frame.md) and
[illustrated wrapper contract](design-workshop-upgrade-confirmation-layout.md).
The template's xs320 is an input to that layout, not final width320.
The758x300 rectangle is a measured profile, not a universal localized or
other-resolution size.

## OK and keys

Four native fragments verify Enter13, Shift+Enter`0x1000d`, Escape27, and
OK/action-1 each set answer0 and modal-loop result0 and request one close.
With flags0 and no choice rows, Escape is dismissal with0, not a separate
-1 cancellation. `607910` maps keys to actions-1/-2;
`61d780` initializes answer0 and only overrides Escape to-1 when flag`0x40`
is set, which NOPRESET does not set. The preset caller ignores the result
in either case and requests39 after ordinary wrapper return.

These are bounded original handlers on the actual constructed dialog;
the close method is a receipt boundary. The OS event loop is supplied for
wrapper cleanup. No host window, desktop keyboard delivery, or game mutation
is implied.

## Feedback IDs are not stock WAV bindings

`451f20` gates playback on settings`959f98 & 0x400`, then selects
`768b50 + feedbackId*108`. Thus39 selects`769bc4`,43 selects`769d74`.
The two normal sound constructors initialize sample handle`+3c` and
filename pointer`+4c` to0, flags`+54` to0, resource-group`+68` to16.
The actual stock bank initializer`451550` does not load either object.
All its sample-loader receiver/name arguments were recorded in the bounded
initialization check; neither selected address occurs.

Four enabled/disabled playback controls then execute actual`451f20` and,
when enabled, actual inherited`4dc760`. With these unbound slots it returns
20 (`0x14`, unloaded sample) without a playback sink. Disabled requests
stop at the settings gate. No host waveform playback is performed.
The file/sample loaders for **other** initialized objects and resource
registration are supplied successes; the two tested objects' constructors,
bank-init selection and unloaded-playback branch are original.

Therefore preserve the logical43/39 requests and their order, but a faithful
stock mapping for those two IDs is **unbound/silent**, not `fx/ok.wav`,
`CPU system error.wav`, or an inferred voice sample. The same initializer
separately loads`fx/ok.wav` into another object (`76aa28`), and CPU voice
objects occupy another range; those are not aliases for these IDs.
Generic control sounds are separate from this feedback request. If a future
sound-bank customization explicitly binds these objects, the generic player
can use that binding; this does not establish universal silence for modified
banks or another executable.

## Evidence and limits

One measured wrapper, four key/OK controls, three caller-order slices,
two sound constructors, a bounded bank initializer and four dispatcher/
unloaded-playback controls. No broad preset/model/audio matrix.
Private owned DIBs include the dialog and its separate OK surface, not an
independently composed final desktop screenshot. The isolated key controls
restore the captured dialog/context/stack before original wrapper cleanup;
the completed v2 packet is accepted. Original game/production sources are unchanged.

Additive private freeze:
`tmp/reference/independent-workshop-nopreset-evidence-20261021/`.

Manifest SHA256:
`17f4ce094e3ed008f01bfada6a1b25dbb529ea1f04fa037f95ab1dfcf8b3b482`.
