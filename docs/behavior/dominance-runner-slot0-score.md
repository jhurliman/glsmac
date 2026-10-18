# Dominance with retained runner-up slot0

RE-SMAC-DOMINANCE-SLOT0-SCORE-001, 2026-10-18. Pinned stock SMAC.
[Literal provider/caller controls](dominance-runner-slot0-score-cases.json).

**Dominance reads the current retained score-table entry for faction0. The
ordinary score provider skips slot0 and leaves its word unchanged.** It
explicitly writes zero for inactive **positive** slots; that rule is not how
slot0 gets its value. The [existing one-active-faction ranking receipt](ending-input-producers.md)
establishes leader1/runner0. These controls supply that retained reference and
test the score it indexes, without repeating the ranking matrix.

The predicate does not call a fresh scorer for the runner. It reads the table
at the retained runner ID. The score-only provider produces a table, rather
than a per-faction return value that dominance substitutes for this read.

## What zero means in the ordinary fresh profile

The untouched pinned PE mapping supplies score-table slot0=0. A complete native
ordinary score-only refresh preserves that0. This supports zero in that declared
cold-start profile; **a missing/inactive runner alone does not establish zero**.

Two otherwise matching native provider/caller controls poison slot0 with16/17.
The score provider preserves each value, writes leader score24 and makes no
slot0 write. Complete dominance reads the exact retained runner value:

| Slot0 before and after score refresh | Leader score | Dominance result |
| ---: | ---: | --- |
|0, untouched PE mapping |24 | True |
|16, diagnostic retained value |24 | True, equality at the normal ratio |
|17, diagnostic retained value |24 | False |

All have only positive faction1 active/human, six owner1 Base records, cached
population6, no technology ownership, no cached Units, no projects/future
economic deadline, difficulty2 and turn251. These are controlled ordinary
score inputs, not a replay of an organically developed stock campaign.

A separate actual whole-new-world prefix preserves a poisoned slot0 score17
through first faction-initializer entry. Its usual scenario-parameter resets
do not independently establish score0=0 at that boundary. This is a prefix
observation, not a complete frontend/restore/second-game origin audit.
Use the actual retained slot0 authority at the call site; do not manufacture
zero from the runner ID or active mask, or invent a fresh faction0 score formula.

## Which9999 is Objectives Required

The ordinary tuple is ordered:

| Position | Scenario parameter | Ordinary value |
| --- | --- | ---: |
|1 | Objectives Required for Victory |9999 |
|2 | Objectives for Sudden Death Victory |9999 |
|3 | Points for each Objective Achieved |0 |

Dominance's special comparison reads **position1**, testing it strictly greater
than1000. With Intense Rivalry off, difficulty below3 and Conquest disabled,
that selects leader>=2×runner. Otherwise this branch uses the normal
2×leader>=3×runner comparison. Existing Intense Rivalry behavior is unchanged.

Three complete caller controls hold native ordinary-provider scores24/13,
then change the specified parameter before querying dominance:

| Caller tuple | Result with Conquest disabled, difficulty2 |
| --- | --- |
|1000 /9999 /0 | True: normal comparison |
|1001 /9999 /0 | False: doubling selected |
|9999 /1000 /0 | False: first parameter still selects doubling |

The scores are deliberately **not refreshed after those diagnostic parameter
changes**: a separate score refresh under custom scenario parameters can select
objective scoring. This isolates the dominance consumer's field identity and
strict boundary; it is not a new custom-scenario score contract. Source mapping
also ties the first and second fields to their actual Scenario Parameters
dialog labels and the whole-new-world defaults.

Validation: six complete native provider/caller contexts and one poisoned
new-world prefix. No helper-result substitutions, broad grids or port changes.
The standing broad research goal remains paused.
