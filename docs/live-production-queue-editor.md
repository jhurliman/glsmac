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

Validation and rendered acceptance will be recorded at the feature checkpoint.
