# Campaign cursor and ordinary reopen

New campaign saves retain the primary UI cursor independently of an eligible
Unit. Loading a campaign with no ready Unit therefore returns to its saved
cursor instead of the unexplored world midpoint. Panning alone does not replace
the cursor. Ordinary reopening resets zoom and primary display fields, matching
the original control rather than preserving arbitrary saved camera settings.

## Original contract

The frozen `primary-view-restore-evidence-20261020` handoff has manifest SHA256
`8493892b549aa1b8f705d05119c5c70fb143017a1a998f22b670a5aa4a6ea4ec`.
All 12 files were verified. Its independent original checker passes 16 complete
save/reopen controls, 304 checks and 1,888 UI DWORD transfers. Only compact numeric
[acceptance cases](behavior/primary-view-cursor-cases.json) are committed.

Original save/read routines at 0x5c0340/0x5be250/0x5c05e0 retain the cursor;
ordinary reopening through 0x47dd60, 0x47d630, 0x47d4c0 and 0x46f2a0 resets the
primary view. In the measured 80x80 world, an in-bounds cursor supplies camera
coordinates even with odd parity or unknown terrain. An out-of-bounds cursor
opens the camera at (40,40), but the raw invalid cursor remains unchanged.
No ready Units, no Units and no Bases do not trigger an own-Base fallback.

Production `ReopenCamera` keeps cursor and camera distinct. The measured original
profile is bounded to 80x80; midpoint parity for other world sizes follows the
port's existing physical-tile convention and is not a general original proof.

## Save and lifecycle connection

Tile selection updates a frontend value and a matching backend world admission.
The admission mutex protects the cursor; stale or retired world tickets cannot
update another campaign. Queueing a save freezes a value copy, so later selection
cannot change the queued record. No frontend pointer enters the owner checkpoint.

The GLSMAC campaign envelope writes version 2 when a cursor is present, appending
two typed signed integer fields. Each must fit int32; raw invalid coordinates are
preserved. Version 1 remains readable and records without a cursor retain their
version-1 encoding. Missing old data cannot be recovered. Truncation, trailing
bytes and coordinates wider than int32 are rejected. This is GLSMAC's campaign
format, not an implementation of original `.SAV` files.

Load passes the cursor into the new frontend and applies it once after map reset.
The native view resets its primary fields and camera; a valid physical tile also
restores normal selection. Automatic selection uses an in-bounds UI cursor.
An empty selector result without an existing selected Unit no longer selects the
default center tile and overwrites the restored cursor.

## Validation

The cached incremental build and 13 affected campaign/controller/native groups
pass, including actual frontend reopen, queued-save ownership, stale admission,
legacy absence, signed extremes and malformed fields. Eight changed production
translation units pass correctness analysis. The same 13 groups pass a focused
11-TU ASan/UBSan build; other objects are uninstrumented and leak detection is
disabled. No full platform matrix was repeated.

A hidden Linux SDL/OpenGL run loaded Morgan 2105, exhausted/held the Scouts so
none was automatically eligible, and saved cursor (34,68) through the normal UI.
Panning into unknown terrain and saving again produced identical complete save
bytes. Reset/reload returned to the saved known patch. A second round trip saved
at maximum zoom and reopened at default zoom with the same cursor. Normal Quit
returned exit 0 with no tracked OpenGL leaks. Audio and allocation debugging were
disabled; this is not audio/performance or macOS validation.

| Panned away before save | Reopened at saved cursor |
| --- | --- |
| ![Unknown camera area](https://raw.githubusercontent.com/jhurliman/glsmac/09d55424046eb4d568c54ae49c540161740cf703/campaign-ui-cursor/panned-away.png) | ![Saved cursor restored](https://raw.githubusercontent.com/jhurliman/glsmac/09d55424046eb4d568c54ae49c540161740cf703/campaign-ui-cursor/reloaded-focus.png) |

| Maximum zoom before save | Default zoom after reopen |
| --- | --- |
| ![Maximum zoom](https://raw.githubusercontent.com/jhurliman/glsmac/09d55424046eb4d568c54ae49c540161740cf703/campaign-ui-cursor/zoom-before-save.png) | ![Default zoom restored](https://raw.githubusercontent.com/jhurliman/glsmac/09d55424046eb4d568c54ae49c540161740cf703/campaign-ui-cursor/reloaded-default-zoom.png) |

Raw saves across reload differed because definition registries changed iteration
order. Sorting only Unit morale/definition, Base population/facility and animation
definition dictionaries by ID made the world bytes identical, SHA256
`334972b72c2ff4d442d313db76b9cc358089f29f34983ff67b1b9d009130b89b`.
Unit/Base record order and values, map, RNG, turn and continuation were unchanged.
Production serialization ordering was not changed in this slice.

## Remaining scope

Later original Unit-controller selection is separate: signed invalid-cursor
origins, persisted selected Unit identity, retaining exhausted/Sentry selections
and the interior recenter margin still need implementation. The original 32-slot
cursor history, nine-view ledger and profile preferences are not serialized here.
Odd-parity cursors cannot select a physical tile in the legacy renderer; broader
legacy visual behavior remains unverified. This change does not generalize native
terrain authority to generated/flat/SMAX maps or close actor/font/UI fidelity.
