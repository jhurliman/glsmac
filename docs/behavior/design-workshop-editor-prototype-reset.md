# Workshop editor controls: PROTO2 and RETIRE ALL3

2026-10-21, pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Two bounded complete dispatcher controls reuse existing fixtures and original
rules/commit/select/list/reset code. Conditional/all-faction variants are
source-backed. No new raster or campaign matrix.
[Scalar receipts](design-workshop-editor-actions-cases.json).

Checks pass:2 native actions,18 source literals,6 installed label identities
and7 retained-frame button regions. Additive private freeze:
`tmp/reference/independent-workshop-editor-actions-evidence-20261021/`,
19 payloads, manifest
`a9811cfe7e4faf4aa81fa9258b35cfae56e01974933e626b2d8466c8aa1bb111`.
This provenance paragraph was added after freezing.

## Scenario Editor gate and exact control identities

These are **Scenario Editor human controls**, not ordinary gameplay actions
or automatic design-family buttons9..22. Installed action2 label is **PROTO**;
its tooltip explicitly says it toggles the "prototype complete" flag.
RETIRE ALL's tooltip also identifies a Scenario Editor function.

Both dispatcher branches require `959fc8 & 0x40` (442777/442810). When clear,
they exit before commit, prompt or mutation. On parent show,447940 with
this=Workshop+a14 shows25 controls, then447980..4479b8 explicitly shows these
two when bit40 is set or hides them otherwise. There is no additional
selected-design/count enable gate. Construction alone does not imply visibility.
Source4ee3f0 is the Scenario Editor toggle: first entry can prompt SCENEDITOR;
4ee44e XORs bit40 and ORs bit20. Network/access restrictions can reject entry
through EDITORNET first. Workshop samples the bit on show and on dispatch.

| Action | LABELS hex byte offset | Zero-based token | Count-prefixed array index / TXTLoader row | Parent-local RECT | Child offset, hex |
| --- | --- | --- | --- | --- | --- |
| PROTO2 | 1024 | 1033 | 1034 | `[135,321,240,341)` | 2384 |
| RETIRE ALL3 | 1034 | 1037 | 1038 | `[351,321,456,341)` | 2ed0 |

Constructor label reads are447200/44734e; tooltip offsets1028/1038.
Their normal-mode absence is deliberate. Keep them conditional on editor mode.

## PROTO2: commit draft, reselect, toggle stored flag4

442777..44280b calls441e20 on the **unsaved draft**, stores its result in
Workshop+cdc, reselects the resulting stored ID with4490b0(id,0), rebuilds
the browse list with5bdb60(owner), recenters, then **XORs word flag4** on
the resulting selected record and repaints. It remains open.

It toggles completion in either direction, rather than buying a prototype,
upgrading live units or changing reactor power. The preceding commit can
reuse a match, allocate a slot, or enter the ordinary OVERWRITEPROTO path;
that path retains its existing retirement/rebuild effects.

**Failure edge:** no success test follows441e20. Even if commit returns0,
reselection/list rebuilding and the flag4 XOR still execute on the remaining
selected record. This includes a declined occupied-slot replacement. The
debug toggle also has no intrinsic-record guard.

Bounded receipt: live/queued source64, changed armor2 draft named Editor Draft,
available slot65. Native action2 selects65 with flags7 (active/named/prototyped).
Original64 flags3 and foreign128 flags3 remain unchanged. Design pool changes
20 bytes; Faction/Base/Unit/history buffers are byte-identical. The old live
instances and queues still reference64. Broader commit semantics reuse the
[occupied-bank contract](design-workshop-custom-edit.md).

## RETIRE ALL3: clear availability/history, preserve units and queues

442810..44288f calls499b70 with SCRIPT **RETIREALL**, flags0, `unit_sm.pcx`,
**callback0**. No numeric/string substitutions. The section has xs440,
caption Scenario Editor, and asks to clear faction prototypes and restore
the original list. Choices:

| Value | Exact text |
| --- | --- |
| 0 | Never mind. |
| 1 | Clear faction prototypes. |
| 2 | Clear ALL prototypes for ALL factions. |

Default0 is selected. Reuse flags-zero generic input: Enter/OK/double-click
accept the selected value, and **Escape also accepts it**. Escape on row1
clears this faction; Escape on row2 clears all. Native0 cancels; otherwise
answer>=2 chooses all, and other nonzero answers choose the current owner.

It **does not commit the draft**. The fixed loop visits faction IDs1..7,
excluding0. Choice1 applies to the Workshop owner; choice2 to all seven.
For each chosen f:

- Clear exclusion bit `(1<<f)` on every built-in ID0..63, preserving all
  their other fields and active flags.
- Clear only active flag1 on all64 custom IDs64*f..64*f+63, preserving
  names, tuples and other flag bits.
- Call5941e0(f), filling all64 eight-byte history entries with FF. The
  faction's history ring index and cached count arrays are not reset.

It never calls595ac0,5d8c40 or the economic Upgrade converter. Existing
units and current/pending production queues retain their design IDs; unit
and production-reference caches remain. This editor operation can leave
live/queued references to inactive custom records. Do not implement it as
deleting units, clearing queues or zeroing/rebuilding whole default records.

After reset,4490b0(0,0) selects intrinsic0 and discards the draft; the browse
list is rebuilt/recentered and repainted. Workshop remains open. Declining0
exits before this reset/reselection and retains the draft.

Bounded own-faction receipt:64 flags3→2; foreign128 remains3. Exactly512
owner1 history bytes become FF; seeded owner2 history remains unchanged.
Design pool changes1 byte; Faction/Base/Unit buffers, queues and caches are
byte-identical. Selection becomes0. Answer2 scope is source-only here.

## Dispatcher / receiver boundary

Neither action is Retire0 or an automatic-family seed. Their local XOR/reset
branches contain no dedicated network packet or simulation receiver call.
PROTO's preceding441e20 retains the existing commit/publication/network paths;
that does not create a synchronized prototype-toggle command. RETIRE ALL
must not be routed through2415 unit retirement once per slot.

## Apply/Done/Cancel labels and main-button style

| Control | LABELS byte offset | Zero-based token | Count-prefixed array index / TXTLoader row | Installed text |
| --- | --- | --- | --- | --- |
| Apply1 | 0xe58 | 918 | 919 | APPLY |
| Done4 | 0x438 | 270 | 271 | Done |
| Cancel8, selected flag80 clear | 0x198 | 102 | 103 | Cancel |
| Same action8, selected flag80 set | 0x864 | 537 | 538 | Auto Off |

Constructor uppercase conversion produces DONE/CANCEL/AUTO OFF. Cancel's
caption lookup447541..447556 depends on stored flag80. Tooltip offsets are
Apply0xe7c, Done0xc50, Cancel0xc60 or0xc64. These are main Workshop controls,
not the generic popup OK label/style.

Shared25-control initialization4477d7..447845 uses font7ece2c (Arial -14
bold), foreground65, normal **27 /7b0490**, and **both hover and pressed
29 /7b04e0**. The earlier parent packet's generic28/29 wording was overbroad;
the constructor sets the same29 descriptor into both fields. No28 assignment
appears in this initialization.

Runtime placement and original-frame evidence are in
[button visibility](design-workshop-button-visibility.md).
