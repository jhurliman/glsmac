# Native DESIGNNAME edit keys and byte limit

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
**39 bounded original character/key calls**, using actual string mutation and
CRT memmove. Repaint/beep are supplied boundaries; font-width helpers return
8px per byte in a10000px field so scrolling does not obscure key semantics.
No renderer, OS keyboard, clipboard or elapsed-time matrix.
[Checked cases](design-workshop-edit-keys-cases.json),
[bounded reference](workshop_edit_keys_reference.py).
All39 native calls match the independent scalar reference, with nine pinned
instruction checks and six decoded key jump-table entries. Additive private
freeze contains10 payloads at
`tmp/reference/independent-workshop-edit-keys-evidence-20261021/`; manifest
`ea4ac382a5014a68eba7e640d9e09658fc6679c8b206f4b2a6f8389dd39298c3`.
This provenance paragraph was added after the immutable freeze.
Selection/caret colors and timing remain in the
[drawing packet](design-workshop-edit-selection-and-caret.md).

## Route character input separately from key navigation

`62f180(character,unused)` inserts the low character byte. Ordinary nonnegative
control codes below32 return0 and leave text/selection unchanged. Other tested
character codes return1. Bytee9 replaces a selected ASCII default with the
single original ANSI byte representing é; this is a byte-oriented control,
not a Unicode code-point or UTF-8-length contract.

`62f360(key)` handles Backspace8, End35, Home36, Left37, Right39 and Delete46.
Shift is encoded as10000 hex and Ctrl as20000 hex, cached at Edit+b38 bits1/2.
Mask the low16 bits for key identity. Keydown and character delivery are
different paths; do not insert printable input once through each path.
Tab, Return and Escape are delegated outside this bounded mutation contract.

Native selection anchor is Edit+b40; active endpoint is+b44. Order can be
reversed. Initial default selection is `[0,byteLength)`.

## Insertion and deletion

Typing replaces the entire interval between anchor and active endpoint,
in either orientation, with the input byte; both endpoints collapse immediately
after that inserted byte. With no selected range it inserts at the caret and
preserves the suffix. Example `abcdef`, selection2..4, type X -> `abXef`,3/3.
Selecting the whole string in either direction and typing X gives `X`,1/1.

Plain Backspace/Delete both remove a selected interval in either orientation
and collapse to its lower bound. Without a range, Backspace removes the byte
before the caret; Delete removes the byte at the caret. Backspace at0 and
Delete at the terminator beep, return1 and preserve state.

**Shift or Ctrl with Backspace/Delete is rejected**, beeps and preserves text
and selection (`62f591`, `62f6b6`). It is not word deletion, Cut, or a plain
delete with modifiers ignored. Controls cover both modifiers with selection2..4.

## Navigation uses the active endpoint, not selection-edge collapse

| Key | Active endpoint | Without Shift | With Shift |
| --- | --- | --- | --- |
| Left | Decrement one, stopping at0 | Set anchor to new endpoint | Preserve anchor |
| Right | Increment one; see end-of-text rule below | Set anchor to new endpoint after success | Preserve anchor |
| Home | Set to0 | Set anchor0 | Preserve anchor |
| End | Set to current byte length | Set anchor to length | Preserve anchor |

An existing range is not first collapsed to its left/right edge. On `abcdef`
with initial anchor0/caret6, plain Left produces5/5; Shift+Left produces0/5.
From3/3, Shift+Home produces3/0 and Shift+End3/6. Left at0 is a handled no-op;
without Shift it still sets anchor0 if necessary.

**Right at the terminator appends an ASCII space**, then moves into the new
end. On `abcdef`, anchor0/caret6, plain Right yields `abcdef ` and7/7;
Shift+Right yields that same appended space and0/7. This is original behavior
at `62f424..62f47b`, not a generic modern editor rule.

Ctrl+Left/Right have separate source word-navigation branches based on literal
spaces, and are not represented by the plain/Shift reference or newly executed
cases. Do not infer Ctrl+A/C/V/X or other clipboard support from this packet.

## 31-byte limit and the original Right boundary quirk

Edit+b14 is31. For character insertion, if current byte length equals31 and
there is no selected range, the native control beeps and returns1 unchanged.
The31st insertion succeeds; a32nd ordinary insertion does not. Replacing a
selected range at the limit is allowed: selecting31 bytes and typing X gives
one-byte `X`, caret1. This is not a blanket rejection of character input
whenever the string already contains31 bytes.

Right's source increments the active endpoint **before** its limit check.
At length31, anchor31/caret31, it beeps and returns with text still31 bytes,
anchor31 but **caret32**. It does not append a32nd space and does not clamp
the endpoint back. This isolated post-limit state is retained as an explicit
native edge case; it is not evidence that ordinary character insertion permits
32-byte names. The bounded reference starts from valid initial byte positions
and reproduces this one-step output; subsequent operations from that anomalous
state are not newly characterized here.

All39 controls preserve caret visibility+b48=1: neither key nor character
mutation resets the blink phase. All test fields are intentionally wide;
native scroll remains0. Use the existing variable-width measurement/scroll
source for a narrower field rather than treating8px as a real glyph advance.

## Rename title authority remains separate

An unchanged intrinsic tuple displays its stored title despite an accepted
draft name/named bit. A changed intrinsic tuple or custom selection can display
the nonempty draft when selected named bit2 is set. See the
[Rename title clarification](design-workshop-rename-and-obsolete.md).
