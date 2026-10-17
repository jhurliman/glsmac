# Explicit Colony choice: native preparation

This checkpoint supplies the native question prefix and Delay continuation. The
default turn controller still uses its existing strict upkeep entry. Accept,
the exact blocking Base Screen boundary, and saved continuation transport must
join before that controller switches to `BeginCanonicalBaseUpkeep`.

`Base::BeginSmacUpkeep` repairs workers and calculates the actual first resource
pass. At a size-one Colony question it publishes the posted minerals and worker
allocation, retaining the old mineral baseline and food. The owned pause holds
that first calculation and the explicit morale mode. The Base record continues
to own production, queue, allocation, context and food. No Unit is prepared.

The native request binds a monotonic request ID to the current owner, Base and
turn. `get_pending_smac_colony()` exposes the request ID, Base ID/name and owner
slot; `choose_colony` accepts an explicit string answer. Delay skips the first
calculation and mineral posting, then runs baseline publication, population,
final resources, research and credit through the existing turn continuation.
Duplicate answers retire harmlessly. Other commands remain fenced while paused,
including commands extracted before the question. Reset clears the request and
continuation without reusing request IDs.

`SmacBaseUpkeepCache` retains the last committed N/M calculation and ordered
support result. Actual upkeep and worker recalculation prepare it before the
same nonthrowing Base publication. Supplied allocation alone, terrain edits and
read-only resource queries do not create or refresh this history. The separate
abandonment provider must return the authenticated compacted successor and its
retained cache; the private successor tail consumes its live food and retained
surplus without posting minerals again. No missing cache is replaced by zeros
or a fresh query. Cache persistence and founding initialization are integration
work outside this checkpoint.

Evidence: `docs/behavior/colony-choice-continuation.md` and
`docs/behavior/colony-abandonment.md` in the primary research workspace. Screen
dispatch is deliberately not selected until its exact controller boundary is
established; Accept requires the distinct prepared abandonment provider.

Two new actual native groups use filter `^SMAC Colony (question|retained)`.
They cover question-time stores, event-based Delay, duplicate answers and
obsolete queued commands, plus retained calculation versus fresh queries.
Five changed production TUs and the Backend fixture passed focused syntax
checks before the final test-cast correction and successor-tail addition.
Linked validation is pending the combined cached build; no platform run or
original-executable experiment was performed for this checkpoint.

## Screen and non-last Accept join

The follow-up joins the prepared non-last abandonment provider. Accept first
checks the known record/cache/last-owned boundaries without consuming the
question. A successful provider retires the old Base; the controller continues
by stable successor ID and its copied cache, skipping fresh successor mineral
production. A final global record does not revisit an earlier Base. Last-owned
restart and foreign successor cases remain rejected before publication.

The accepted Screen boundary is now explicit: after current Base research and
credit, before the next Base. `get_pending_smac_colony_screen()` returns the
originating request ID and Base identity. `colony_screen` notifies with a
`pending` object or null; `close_colony_screen {request_id}` clears the screen
and advances traversal exactly once. Only the displayed Base's existing worker
and production events are admitted. Native enqueue stamps these edits with the
screen request; stale edits and duplicate close cannot affect a later stage.
These edits recalculate current outputs without repeating committed income.

The actual two-Base Screen fixture uses the literal treasury100→101→102 and
research0→2→3 boundaries, then checks real worker/production edits and a stale
post-close edit. Filter: `^SMAC Colony (question|Screen|retained|abandonment)`.
This source handoff has not been linked yet; root owns the combined affected
run. Default activation and pending-question persistence are separate root
integration changes. Screen snapshots remain unavailable.
