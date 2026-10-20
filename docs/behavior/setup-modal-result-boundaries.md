# RE-SMAC-SETUP-MODAL-001: action, byte limit, cancel and empty-result boundaries

Static follow-up to [setup continuation](setup-dialog-continuation.md) and
[TXT defaults](setup-identity-txt-bindings.md). No new rendered/input-driven
original dialog was executed. The expanded source receipt contains the
generic modal, action handler, edit construction/setter, character handler
and result publisher, in addition to the original setup/controller/parser.

## Change Gender and the seventh customization result

The normal NAME/NAME2 wrapper adds flags `44` hex. On modal completion,
`641090` publishes dialog `+3100` as `9c4550`. The setup caller first
requires a zero modal return, copies the accepted leader name to the selected
definition, and then enters CUSTOMNAMES if that action field is nonzero.
This branch contains no immediate XOR or replacement of the gender DWORD.
The installed NAME/NAME2 secondary button is labelled `Change Gender`.
Thus a successful nonzero secondary action routes into customization, rather
than proving an immediate gender flip or a NAME-to-NAME2 retry.

The generic action handler `61d780(action)` records ordinary action IDs in
dialog `+3100`. Special -1 clears it; special -2 also clears it and, when
flag40 is set, changes selected result `+a40` to -1. Mapping every physical
button/key event to these action IDs remains outside this static receipt.

The result publisher resolves the earlier gender scratch uncertainty:
`61cbca..61cbf5` exports edit rows 0 through 7 to the global string-result
slots, using `62c380` and `633910`. Row6 (Gender) therefore has result
`9c3ff4` when its control exists. `633910` copies to the global slot only;
it does not write back into the pointer originally supplied as a default.
Registration `62c280` clones that default. The setup caller copies rows0..5
back into the definition but compares its **original stack gender scratch**
to the Female label instead of reading row6's result. There is no generic
write-back restoring that missing connection in this traced path.

Consequently, the static source has an ignored edited Gender result. For
the normal Male/Female defaults with distinct initials, accepting a changed
row still calculates the original gender. This is stronger source evidence
than the earlier unresolved scratch observation, but not a native UI smoke
test. A port should describe any explicit working gender toggle as an
intentional behavior choice until native input evidence settles fidelity.

## Byte limits include default text

The row constructor `62bde0` sets each text control's `+b14` maximum from
the registered row maximum (`62c13e`), then calls `62ece0` with the cloned
default string. That setter empties the control, measures the default with
byte-oriented NUL scans, and repeatedly replaces its final byte with NUL
until its length is at most the maximum. It then copies it into the control.
It accepts a null pointer or an empty string as an empty control. Trimming
mutates the dialog's cloned default, not the original definition string.

NAME and FIRSTBASE have a 23-byte maximum. In CUSTOMNAMES, Formal has39;
the other six rows have23. In particular, the parser's up-to-127-byte
Adjective/Description defaults are truncated to23 **when the edit controls
are built**, before any user edit or acceptance. Cancelling NAME skips the
definition copy; cancelling customization skips its six result copies, so
this trimming alone is not evidence that a definition was changed.

The character handler `62f180` uses a byte string and stores the incoming
character's low byte. It rejects control-character values0..31 from this
handler (other key handling is separate). At the maximum length it rejects
additional insertion when the selection is empty; replacement of a selected
range takes the separate branch. These are byte/code-page limits, not23
Unicode code points. Paste and IME paths have not been exhaustively traced.

## Cancel results and text results are independent

For dialog type4, `61b9d0` publishes current edit text **before** converting
the modal status into its final return. This occurs even when the eventual
return reports cancellation. It exports rows0..7; a missing control gives a
null pointer and `633910` leaves that global slot unchanged. An existing
empty control gives a nonnull pointer to NUL and replaces its global slot
with an empty string.

The final return has two layers:

- Modal status zero returns the selected result at dialog `+a40`.
- Nonzero modal status returns -1 when flag40 is set, otherwise global
  `6b5120`. The earlier special action -2 can also set selected result -1
  when flag40 is set.

NAME's wrapper sets flag40, and the setup controller rejects **any nonzero
return** before copying its global result. Therefore text exported during
cancel does not overwrite the selected definition's leader name at that
copy site. This does not undo an earlier accepted name or Restore Defaults.

FIRSTBASE is initialized with flags4, and its caller does not inspect the
return at all. On every modal return that reaches `5c6231`, it copies the
current first string result into the existing Base name, then runs duplicate
normalization. There is no controller-level "cancel restores the old name"
branch. A physical close path that never returns or changes text elsewhere
has not been tested; do not infer exact Escape/window-close behavior solely
from this transfer rule.

## Empty and whitespace-only accepted results

The result publisher does not reject an empty or whitespace-only string,
substitute the old default, or trim spaces. NAME's successful-return copy
also has no such guard. FIRSTBASE copies its string unconditionally on
return, then applies only the already-documented exact-duplicate suffix
normalization. Thus these **transfer layers accept an empty result** and
preserve whitespace. No native click/keyboard proof of every empty-input
acceptance route is claimed; physical event dispatch remains a separate gap.

The practical port boundary is to preserve both outputs: modal action/status
and current text. Apply each caller's own acceptance rule. Do not make the
generic dialog discard text automatically on cancel and then assume that
matches FIRSTBASE's caller. Earlier leader acceptance and Restore Defaults
are live definition mutations with their own already-established timing.
