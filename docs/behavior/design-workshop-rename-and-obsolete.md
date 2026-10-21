# Workshop Rename5 and Obsolete6: authority, cancellation and dialog layout

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This supplements the existing 41-case
[custom-edit contract](design-workshop-custom-edit.md), preserving its frozen
evidence. New bounded coverage: eight complete action/cancel controls with
supplied UI boundaries, plus one DESIGNNAME generic-edit layout through the
native pre-modal-loop boundary. No production changes or new model matrix.
[Reference](workshop_rename_obsolete_reference.py),
[compact receipts](design-workshop-rename-and-obsolete-cases.json).
Additive private freeze contains 19 payloads at
`tmp/reference/independent-workshop-rename-obsolete-evidence-20261021/`;
manifest SHA-256
`471358e392d3dd4b3238bed3a2186aadd43764f251a54563630ce13e006b7b58`.
This provenance paragraph was added after the immutable freeze.

## Ordinary controls are exposed; handlers do not require a clean draft

Constructor `4469a0` creates Rename at actor `+4568`, action5, and Obsolete at
`+50b4`, action6. The Workshop show loop `447963..44797e` shows all 25 button
objects; only debug actions2/3 are subsequently hidden without debug flag40.
Rename and Obsolete have no intrinsic/custom, active/inactive, named,
prototype, cached-count, technology, or tuple-equality guard in their dispatcher
branches (`442a73`, `442a7f`). They do not call commit `441e20` first.
Do not add a save-first requirement or disable Rename just because the selected
ID is intrinsic. The intrinsic Rename control opens the dialog and sets its
named flag without writing its stored name; later commit has separate
intrinsic-name rules. An intrinsic Obsolete control changes its exclusion bit.
Normal browse admission remains responsible for which records can be selected.

Parent-local rectangles in the retained 623x387 workshop are Rename
`[135,347,240,367)` and Obsolete `[243,347,348,367)`. Stock Rename caption is
LABELS `1f4`, tooltip `c54`; Obsolete caption `bd0`, tooltip `c58`.
The constructor uppercases captions. Obsolete remains the same caption on an
excluded selection; the action toggles the selected record back on.

## Rename uses the draft for its default and the selected record for flags

Action5 directly calls `446550`:

1. Copy draft name actor `+14220` into a local default buffer.
2. If empty, generate a name through `594320(dst,selectedID,owner,draftCh,
   draftWe,draftAr,draftMask,draftReactor)`, then immediately clear named bit2
   in the **selected saved record**. This effect precedes the modal dialog.
3. Terminate the displayed default at byte31. Call
   `5d7880("DESIGNNAME",31,default,0,0)`; its generic wrapper adds flags44
   for an edit dialog. Return0 means accepted; nonzero cancels.
4. Cancel returns immediately. It does not restore that pre-dialog named bit.
5. Accepted empty text empties draft name and clears selected named bit2.
6. Accepted nonempty text different from the displayed default, with a
   case-sensitive byte comparison, is copied into draft name (max31 plus NUL)
   and ORs selected named bit2. Same default leaves draft and current flags as
   they stand, including the earlier clear when the draft was empty.
7. Every accepted result, even unchanged text, calls main painter `43f8c0(0)`
   then Window virtual+fc (`407cb0`) invalidation. Cancel does neither here.

The stored design name and component bytes are not changed by this dialog.
Draft components, ordered ability slots, selected ID and cached mineral factor
are also preserved by the action itself. No allocation, cost/cache rebuild,
unit conversion, queue edit or retirement history occurs. A later complete
Workshop repaint can run the separate
[status selection transition](design-workshop-data-and-status.md); that is
not a Rename commit.

New controls deliberately use stored armor1 and unsaved draft armor2. The
generated default is **Plasma Sentinels**, proving draft authority. Cancel
with an empty draft and flags3 leaves flags1; accepting that same generated
default also leaves empty draft/flags1. Accepting `Draft Guard` gives that
draft text and flags3 while every stored name/component byte stays unchanged.
The ordinary Workshop Cancel8 afterward preserves all these saved flag edits;
it does not roll them back. All eight controls retain cached factor8.

Accepting an automatic name that differs from a nonempty draft can first set
named bit; later `4490b0` may normalize it against its generated automatic
name. That reopening rule belongs to selection, not Rename. Preserve byte
case and NUL handling; a shorter replacement does not promise zeroed tail
bytes beyond the string terminator. The prior six Rename controls cover
nonempty cancel/same/new/empty/automatic/case-only paths and later commit.

## DESIGNNAME modal geometry: measured native generic edit layout

Installed SCRIPT section DESIGNNAME has `xs440`, caption
`What shall we call this unit type?`, body `Name:`. These are localized template
inputs, not hardcoded action strings. The generic caption painter uppercases
the heading; edit contents and Name: preserve case.

The new bounded layout uses the native template loader and layout code with
max31, flags44, declared default `Scout Patrol`, 1024x768 mode0, visible console
rails, normal MainFont16. It constructs the generic dialog directly using the
arguments derived from `5d7880 ->641090`; it does not boot a campaign or run a
real OS modal loop. Its natural minimum width is **760**, despite xs440.
Do not use440 as the final pixel width.

| Object | Native rectangle / placement |
| --- | --- |
| Outer Window, screen coordinates | `[148,255,908,555)`; client760x300 |
| Main frame, dialog-local | `[23,0,705,305)`, style9 (`7b01c0`) |
| Caption frame | `[27,4,701,28)`, style3 (`7b00d0`) |
| Caption inner | `[30,7,698,25)`, style6 (`7b0148`) |
| Name/edit row frame | `[30,35,696,67)`, style3 |
| Name/edit row child | `[33,38,693,64)`, parent=dialog |
| Edit child | `[53,1,663,21)`, relative to Name/edit row child |
| OK | `[33,79,360,99)`, action -1 |
| Cancel | `[366,79,693,99)`, action -2 |

Both buttons are327x20, not bottom-aligned to the 300px Window. The frame
extends five pixels past client bottom, following generic dialog framing.
The edit child retains native max length31 at dialog `+a44`.
Caption Arial -14 bold: local x227, baseline21, palette index222.
Name:/edit/final button text use Arial Narrow -16 bold. Name: is x3 baseline17
on the row DIB (dialog x36, baseline55). Edit text is baseline16 on its DIB
(dialog baseline55), with measured clip `[4,0,606,20)`.
OK final glyph is x153 baseline16 on its button DIB; Cancel x140 baseline16.
The native journal also retains preliminary default-font paints; those are not
the final font/placement. Stock body/edit operand is palette224; final button
glyph journal carries palette0. Geometry is profile-specific; reuse generic
layout for other resolution/font/mode settings.

## Obsolete is an immediate selected-record toggle, then navigation

Action6 ignores the unsaved draft and reads selected designID `+141f8` and
owner `+141f4`. In record stride52, exclusion byte is `+44` decimal and flags
word `+48` decimal. With owner bit `1<<owner`:

| Before | Flags operation | Exclusion operation |
| --- | --- | --- |
| Included | `flags &= 0xfdf7` (clear8 and200) | XOR owner bit |
| Excluded | If old bit8 set, OR200; then OR8 | XOR owner bit |

Preserve every other flag/exclusion bit. This does not deactivate the record,
remove it from history, clear prototype/named bits, change its saved name/cost
or component tuple, delete units, or alter Base queues. Cached unit and
production-reference rows remain unchanged. There is **no confirmation** and
no local network-admission check before mutation. It calls
`5aaa20(selectedID)`, which submits design publication command2101 through
`547d50`; keep this publication distinct from Retire command2415.

After mutation/publication:

1. Search the **old** `94f7a4` browse list from current strip offset `+ce4`,
   wrapping for at most its old count. If selected ID is found, replace it
   with the immediately next old entry, wrapping. If absent, retain the ID.
2. Call `4490b0(chosenID,0)`, then rebuild browse list `5bdb60(owner)`.
   Selection can resolve an inactive entry to the first inactive own slot.
   Active selections reload their saved tuple/name; inactive zero-initialize
   components but retain the old draft name, as specified in
   [fresh draft](design-workshop-fresh-draft.md). Do not uniformly clear names.
3. If effective selection is in the rebuilt list at index j, set strip offset
   `(j+newCount-5)%newCount`; repaint the strip if this offset changed.
   Missing selection leaves offset unchanged. Request full Window+f8 repaint.

The selector's normal flag/name normalization still applies to the newly
selected record; that can have effects beyond the raw toggle itself.
Source assumes natural nonempty browse lists when doing modulo arithmetic.

Native unsaved-toggle controls select old-list next inactive65, reset draft
armor2 to0 and retain `Unsaved Guard`; the toggled saved64 retains its old
components/name. Flags020b/included ->0003/excluded; flags000b/excluded
->020b/included. An intrinsic2 control toggles only its exclusion byte and
advances into the custom bank. Subsequent Workshop Cancel8 leaves every toggle
intact and does not restore the old selection/draft.

## Retire0 and Upgrade7 are already separately executed

Use [explicit Upgrade and Retire](design-workshop-explicit-upgrade-and-retire.md)
and its 33 native dispatcher controls rather than treating either as Rename
commit or Obsolete. Both use the selected **stored record**, ignoring unsaved
components. Retire confirms with distinct zero/nonzero cached-unit templates,
then removes raw-ID units/queues with its documented owner asymmetry and
retirement history. Upgrade admits targets from the owner's custom bank,
quotes stored costs/counts, confirms currency conversion, updates own units
and production, and deactivates custom source without retirement history.
Their dedicated packet preserves cancellation, network sender boundaries,
multiple-target chooser and informational YESUPGRADE distinction. The separate
[modal layout supplement](design-workshop-retire-upgrade-modal-layout.md)
now establishes Retire zero/nonzero-count prompt and multiple-target chooser
geometry. Upgrade's later currency/prototype prompts remain separate.

## Port integration

Obsolete/reactivate is now a queued, owned-Base action with a saved-state fence
and UI request correlation. It preserves the old browse list supplied by the
open Workshop; rebuilding that list before navigation would change native
controls that retain their list while flags change. Identity bounds restrict
the list to unique intrinsic/owned entries. The backend prepares the toggle,
reselection and rebuilt offset before publishing the design state. It does
not commit the unsaved component draft or change cached Unit/production rows.
The original stored-name normalization during reselection remains separate.

The live UI always exposes the Obsolete control, keeps its caption when the
selection is excluded, uses original styles27/28/29 and LABELS caption757,
and restores the lower-row union frame. The fixed105x20 normal button matches
all2,100 pixels of the retained native Scout frame at1024x768. Compare via
`workshop_parent_pixel_gate.py --region obsolete`; no rescaling or tolerance.
A normal live session toggles Scout and advances into the saved Synthmetal
Garrison. Cancel then reopening retains the changed browse order; a second
Cancel closes cleanly. These are scoped button/behavior checks, not whole
Workshop parity. Rename is still pending; the native action5 material above
is its implementation contract, not a claim that its dialog is connected.
