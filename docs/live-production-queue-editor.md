# Local production queue editor

The Base inspector can edit its real pending production list. The current
target, mineral credit, switching baseline, citizen allocation and treasury are
not changed by a queue edit. Accept publishes one native transaction; Cancel
discards the optional view. The ordinary production processor still validates
and promotes pending entries after completing the current item.

This is a provisional **port interface**, not a reproduction of the original
chooser. [RE-SMAC-QUEUE-001](behavior/production-queue-and-build-eligibility.md)
establishes ordered advancement, duplicate references and eligibility at pop,
but does not establish original queue-edit admission or maximum length.

- The existing port storage bound is nine pending entries.
- New entries come from this Base's currently eligible native production picker.
  Future queued prerequisites do not make a new entry eligible in this editor.
- Existing entries may be retained, moved or removed even when no longer
  eligible. Each original position is a one-use token; retaining an unavailable
  item cannot create additional copies of it.
- Duplicates are preserved. Empty pending lists leave the current target alone.
- The native command authenticates the current local owner, ready campaign,
  complete Base/turn snapshot and writable owning GC scope. It does not decode
  caller snapshots. Intervening changes invalidate an optional view; replaying
  a previously accepted edit cannot overwrite newer state.
- Colony Screen edits use the existing stamped continuation dispatch, including
  its current-Base and request checks.

The editor closes on Base retirement/change, required questions and turn
transitions. Unsupported items remain outside the currently eligible picker.
Hurry remains explicitly unavailable while its original post-dialog blocking
mode has no connected producer.

The current admitted picker has at most nine choices (four unit packages and
five simultaneously eligible facilities, including habitation), which fit in the ordinary choice panel. A broader catalog
will need a scrolling editor. A nested scrolling prototype clipped its button
labels on the real renderer; it is not accepted as rendering parity.

## Validation

Fourteen distinct affected native groups pass: queue edits, existing production
selection, Rename consumers, the required Colony Screen and complete Base-screen
fixtures. These cover duplicate order, stale snapshots, invalid/oversized/reused
tokens, foreign control, unavailable retained facilities, actual Base save codec
and completion advancing the saved queue exactly once. The real GSE editor test
passes add, move, remove, limit, cancel, submit, stale-view and screen entry
callbacks. Complete-screen fixtures also pass the final unavailable-control
binding. All nine Base inspector GSE scripts also pass after adapting the
energy-screen and stored-production test doubles to the new section/control.

The new source is registered and compiled through genuine CMake. Correctness
analysis passes all four changed production translation units. Focused
ASan/UBSan instruments the command, Base wrapper and live regression translation
units: seven groups pass, with remaining objects reused from the normal cached
build and leak detection disabled. One initial sanitizer launch exited 139 before
producing diagnostics; the initialized diagnostic retry passed. Its cause was
not established by this task.

## Installed-Planet acceptance

Actual SDL input and OpenGL backbuffer captures at 1024x768 used the installed
GOG assets, WSLg/Mesa llvmpipe and a real two-Base Gaia campaign. These are
functional/rendering checks, not native Windows GPU performance measurements.

At `db417b09`, actual clicks added Scout/Former/Scout/Headquarters, removed
Headquarters, moved Former first and accepted Former/Scout/Scout. Reopening,
adding Colony and cancelling preserved those three entries. Current production
remained Scout with two stored minerals, year 2119, treasury 29, the same citizen
and homed free Scout. Menu→Save wrote 828270 bytes.

A fresh process at `bf42854e` loaded that saved file through the real Load Game
and Start restore path. The inspector and reopened editor retained
Former/Scout/Scout, showed `3 / 9 pending`, and preserved the current production,
stores, year, treasury and support. A retained-entry edit was accepted after
restoring. Both accepted runs shut down normally with no tracked memory/OpenGL
leaks; heap allocations were not checked by those renderer runs.

Local evidence files are `queue-populated-db417b09.png`,
`queue-accepted-db417b09.png`, `queue-restored-editor-bf42854e.png`,
`queue-restored-base-bf42854e.png`, corresponding renderer logs and
`gaia-queue-db417b09.glsmac`. Original assets and campaign evidence stay outside
Git. Earlier clipped-label prototype captures are excluded from acceptance.
