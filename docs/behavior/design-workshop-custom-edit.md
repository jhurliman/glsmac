# Workshop occupied banks, reuse, naming and retirement

2026-10-20. Contract for the existing frozen **41-case** bundle
`tmp/reference/independent-workshop-custom-edit-complete-evidence-20261020/`,
manifest `870aa01c5b1cacc8ed8840801ad7125900cd3c60fecd41f0bc97d3996e71da67`.
This documents existing evidence; no native rerun or production edit is needed.
The [independent reference](workshop_custom_edit_reference.py) predicts the
complete 26,624-byte design pool and relevant browse lists. Native full buffers,
ordered writes and stage snapshots are in `native-observations.json.z`;
`checked-summary-private.json` maps the case IDs below to decisions, prompts,
output selection and every changed record/byte. Executable fingerprint is the
same as [the first Workshop path](design-workshop-first-path.md).

**Reuse an existing active own design by exact component tuple before choosing
a new slot. Its name and named flag are not identity or admission gates.**
An unnamed startup-created Synthmetal design is a valid match. A helper
restricted to a fresh empty bank will reject valid live startup states.

## Record fields and match scan: 441e20

Design stride is 52. Tuple bytes `+20..+27` are ability mask (four bytes),
chassis, weapon, armor and reactor. Relevant fields are role byte `+2b`,
owner-exclusion bits `+2c`, word flags `+30`, prerequisite word `+32`.
Flags: active `1`, explicitly named `2`, prototyped `4`; preserve other bits
numerically rather than inventing meanings for `8`, `0x80`, `0x100`, `0x200`.

Scan active built-ins `0..63`, then the owner's 64-slot bank, ascending.
An exact tuple match wins. Built-ins additionally require their technology
predicate; own custom matches do not pass through that built-in guard.
Names, named status and prototype status do not distinguish otherwise matching
tuples. No foreign custom bank participates.

When switching to a matching excluded record, OR flag `8`, clear this owner's
exclusion bit, then select it. A matching custom record receives a nonempty
draft name only if the **source/current selected record** has named bit `2`;
OR bit `2` on the target and copy the draft name. An unnamed/empty draft leaves
the matched target's stored name unchanged. Source record remains unchanged.
Matching a built-in preserves its intrinsic name, warns `INTRINSICNAME` for
the attempted named change, and can still return success with that built-in.

| Case IDs | Native result |
| --- | --- |
| `match_reuse_named` | Reuse 70; copy source draft name over its existing name |
| `match_reuse_auto_draft` | Reuse 70; preserve its existing name and bytes |
| `match_reuse_obsolete` | Reuse/reactivate 70; update exclusion/flag bookkeeping |
| `match_builtin_name_warning` | Resolve Scout 2; intrinsic name stays unchanged |
| `same_tuple_named` | Rename current 64 through matching path |
| `same_tuple_auto_ignores_direct_draft` | Injecting a draft string without named bit does not rename 64 |

## Allocation is two scans, not a single first-free search

Cached unit and production-reference counts are owner-faction byte arrays
at `95d9fc + owner*20bc + designId` and
`95dbfc + owner*20bc + designId`. They are distinct from design flags.
The [production-reference lifetime contract](production-design-reference-cache.md)
specifies current-plus-pending contributions, wrapping updates, recount timing
and saved-state authority; Workshop open/Apply do not refresh that row.

For an unmatched changed built-in, first choose a **provisional** custom slot:
scan the owner's bank ascending; first owner-excluded slot with both cached
counts zero terminates this scan; otherwise remember the last inactive slot.
If no custom slot was found, retain the intrinsic selection and fail with the
intrinsic warning. An active provisional candidate enters the common scan below.

For a changed active custom/provisional record, scan the owner bank ascending:

1. First inactive record wins immediately and terminates the scan.
2. For an active owner-excluded record with unit count zero and production
   count zero, replace the reusable candidate. Consequently the **last** such
   candidate wins if the bank contains no inactive slot.
3. An excluded record with zero units but positive production references can
   provisionally replace the selection only before a fully-unused candidate
   has been found, and only when its role byte equals the currently selected
   candidate's role byte.
4. A positive unit count prevents that record being a reusable candidate.

An earlier fully-unused excluded candidate does not prevent a later inactive
slot winning. For a built-in with only inactive slots, the first scan already
picked the last inactive slot; because it is inactive, the common active-record
scan does not run. This accounts for the different first/last behavior.

If neither an inactive nor a fully-unused candidate won, an active final
selection with positive cached **unit** count prompts `OVERWRITEPROTO`.
Production references alone do not prompt. With no alternative and no units,
editing a custom slot can silently rebuild that same slot.

| Case IDs / inputs | Final selected slot |
| --- | ---: |
| `builtin_last_inactive`: inactive 65/70/127 | 127 |
| `builtin_first_reusable`: excluded unused 65/70, full bank | **70**, despite the historical case name |
| `builtin_reuse_live_protected`, `builtin_reuse_queue_protected`: 65 has protected count | 70 |
| `builtin_full_failure`: full active bank, no reusable slots | Intrinsic 2; failure |
| `builtin_mixed_earlier_inactive`: inactive 65/127, unused excluded 70 | 65 |
| `builtin_mixed_later_inactive`: inactive 80/127, unused excluded 70 | 80 |
| `custom_first_inactive`: inactive 65/70/127 | 65 |
| `custom_last_reusable_full`: unused excluded 65/70 | 70 |
| `custom_first_inactive_beats_earlier_reusable`: excluded 64, inactive 65 | 65 |
| `custom_full_in_place`, `custom_queued_in_place` | 64, without prompt |
| `custom_reusable_live_protected_last`, `custom_reusable_queue_protected_last`: 70 protected, 65 unused | 65 |
| `custom_queued_obsolete_same_role`: excluded 65, zero units/one production ref, role matches | 65, without prompt |
| `custom_queued_obsolete_other_role`: same counts, different role | 64, without prompt |
| `custom_live_free_slot`: live/queued 64, inactive space available | New 65; old 64 and all units/queues unchanged |

The bounded builder reference covers Hand Weapons/Infantry/Fission/no abilities
and armor 1/2. Reuse the general native-derived design builder for other tuples;
do not turn this small reference's supported builder domain into a live
Workshop admission restriction. Matching an existing design preserves its
prototype flag; allocating/rebuilding a different tuple must use the builder's
normal flag/history producers, not blindly copy the prior slot's flags.

## Rename dialog and reopening: 446550, 4490b0

Workshop action 5 calls `446550` directly. It does not allocate or commit
before opening `DESIGNNAME`. The dialog's maximum is 31 bytes. Empty draft
name uses native generated name as the displayed default and clears selected
record's named bit before the dialog. Cancel leaves that pre-dialog effect.
Accepted empty text clears draft name and selected named bit. Accepted text
different byte-for-byte from the displayed default becomes the draft name
and sets selected named bit. Accepting the same displayed default changes
neither. This comparison is case-sensitive.

Stored name/components are not committed by the dialog. `rename_new` records
the changed draft and later committed name separately; short replacement names
leave bytes after their NUL untouched. `rename_empty` clears named bit but its
same-tuple commit leaves the old stored name bytes intact. `rename_case_only`
accepts lowercase as a different explicit name. `rename_cancel` and
`rename_same` leave these fixture arrays unchanged.

Reopening/reselecting with `4490b0(selection,0)` clears selected flag `0x80`.
An explicitly named custom selection supplies its stored name as draft; if
that text exactly equals the newly generated automatic name, clear named bit
and leave the draft empty. `rename_automatic` verifies this normalization.
Built-in/unnamed active selections use an empty draft. The separate
[fresh-draft supplement](design-workshop-fresh-draft.md) closes inactive
selection: argument 0 resets basic components while retaining draft name and
raw flags; normal opening passes 1 for technology/recommendation defaults.

## Obsolete/reactivate and browse order

Action 6 on an already excluded record reactivates it: if old flags contain
`8`, OR `0x200`; then OR `8`. On a currently included record, obsolete it by
clearing `8` and `0x200` (`flags &= 0xfdf7`). In either direction XOR this
owner's exclusion bit. Preserve active/named/prototype and other flags.
Cases `toggle_3_0`, `toggle_3_2`, `toggle_11_2`, `toggle_523_2` exercise these
flag histories. Only design 64 changes in the recorded engine arrays.

The UI then advances to the next entry in the **old** browse list, wrapping,
reselects, and rebuilds the list. It does not necessarily remain on the toggled
design. Native `5bdb60` sorts candidates by these keys:

| Record category | Sort key |
| --- | --- |
| Active, included | ID |
| Inactive own | ID + `0x400` |
| Excluded own with cached units | ID + `0x800` |
| Excluded own without cached units | ID + `0x1000` |
| Excluded built-in | ID + `0x2000` |

Inactive own slots are present in this list. Built-ins require activity,
technology and a nonnegative weapon power. All 41 input lists and relevant
rebuilt lists match the independent reference.

## Apply, Done, Cancel and confirmed retirement

Commit `441e20` returns success 1/failure 0. Dispatcher `442600` may return
cleanup's EAX; use editor result field `+cdc`, not dispatcher EAX, as the
modal result. Done (4) commits and closes only on success. Apply (1) commits,
then **always reselects** the chosen record with refresh 0 and rebuilds the
list. A declined overwrite therefore retains the edited draft after Done,
but Apply restores the old selected components. Cancel (8) writes result 0
and closes without committing. Cancel after a successful Apply does not undo it.

`custom_live_cancel/accept`, `done_live_cancel/accept`,
`apply_live_cancel/accept`, `cancel_unsaved`, `apply_then_cancel` supply the
corresponding transitions. Declines/unsaved Cancel preserve every design,
Faction, Base, Unit and retirement-history byte; UI draft/selection may change
as described. Non-retirement cases preserve all non-design buffers.

Accepting the live overwrite calls `595ac0(selectedId,owner)` **before** the
new design builder. In the three accepted live cases:

- Existing own unit 0/design 64 is removed through `5d8c40`; global count
  changes 2 to 1. Foreign unit 1/design 128 survives byte-exact and is copied
  into physical slot 0. The inactive backing tail is not promised zeroed.
- Every Base referencing raw design ID 64 has current production changed to
  Stockpile `-38`; pending references to 64 are removed, preserving pending
  Scout 2. Queue count changes 2 to 1. There is no promotion of pending Scout
  into current production and no mineral/retool-baseline mutation here.
- The loop uses the raw global design ID, with no additional Base-owner
  filter. The fixture's foreign Base intentionally exercises that distinction.
- Owner 1's Faction unit-count byte at `+894` changes 1 to 0; total-unit field
  `+1054` changes 1 to 0. The production-reference cache for 64 stays **4**,
  despite removal of those queue references. This helper does not rebuild it.
- `594210` records the **old** eight component bytes (mask then
  chassis/weapon/armor/reactor) into the 64-entry per-owner history ring at
  `9af1e8 + (owner*64 + oldCursor)*8`. Faction cursor `+207c` increments modulo
  64. Here owner 1/cursor 0 writes history record 64, then cursor becomes 1.

Silent no-unit in-place rebuilds do not call this retirement/history producer.
Do not record overwrite history, remove units or scrub queues merely because
the new tuple differs. The confirmation/retirement branch is the gate.

Native full Design/Faction/Base/Unit/history buffers and unchanged regions are
retained. Notification, visibility and UI paint/close callbacks are supplied
offscreen boundaries; this does not claim booted UI actor-index repair or a
whole-game screenshot. The counter-only protection cases deliberately isolate
cached-count gates; the live acceptance cases execute actual roster removal,
queue surgery and history writes.
