# Manual specialist selection

Research IDs RE-SMAC-MANUAL-SPECIALIST-MENU-001,
RE-SMAC-MANUAL-SPECIALIST-COMMIT-001 and
RE-SMAC-MANUAL-SPECIALIST-PIPELINE-001.

Status: original-x86 checked for the SMAC fingerprint in [the ledger](README.md).
Fourteen named menu cases plus 800 combinations, fifteen named commit cases plus
900 combinations, and ten connected selection/intake/output cases pass. Actual
technology and overflow helpers execute. Edit-lock and worker hit-test results
are supplied where required. Full-game UI observation remains pending.

This covers the type menu, separately from [worker tile edits](manual-worker-edit.md).

## Menu eligibility

The menu includes Worker and specialist types in installed order: Technician,
Doctor, Librarian, Engineer, Empath, Thinker, Transcend.

A specialist is listed only when all three hold:

1. Its prerequisite is available.
2. Its obsolescence technology is unavailable, or it is the current type.
3. Population reaches the configured minimum specialist size, or its psych
   output is at least two.

An obsolete current type can remain listed, but a missing prerequisite still
excludes it. The current-type exception does not bypass size/psych eligibility.
Below the installed minimum five, stock Doctor, Empath and Transcend qualify;
Thinker's one psych does not. Custom psych and minimum-size values apply
directly. Unlike allocation competition, this gate uses the configured minimum.

If an otherwise eligible type fails the size/psych test, the dialog requests
the minimum-size explanation. Prerequisite/ordinary obsolescence failures alone
do not request that explanation through these gates.

## Current type and unchanged selections

Read the stored type for existing slots zero through fifteen. For later slots,
resolve the current type with the automatic overflow selector. Worker/new-type
invocations start with Worker as the current selection.

Cancel, choosing the same type, and a refused edit lock return before clearing
citizen management. They retain settings and selections at this boundary. The
whole wrapper performs an earlier recalculation; this does not prove globally
side-effect-free dialog cancellation.

An accepted different choice clears citizen/specialist management, retaining
governor activation and directives. It does not change the provisional counter.

## Accepted choice mutations

- **Change an existing specialist:** store the new type only for the first
  sixteen slots. Count and workers do not change. An accepted overflow change
  still disables citizen management but has no stored type write.
- **Choose Worker:** decrease specialist count by one if nonzero. Do not shift
  the clicked type out of the list or directly select a worker square. The last
  counted specialist disappears, which need not be the clicked slot's type.
  Subsequent intake can fill the freed worker slot.
- **Create a specialist from Worker context:** remove the resolved positive
  worker index, append at the previous specialist count, then increment count.
  Store the appended type only if its position is below sixteen.

Worker-hit context uses a supplied screen hit. Background context instead removes
the **highest selected non-center radius index**, scanning 20 down to 1. It does
not choose a worker by score. A supplied nonpositive hit or no non-center worker
still permits the local count increment without removing the center. Such boundary
inputs do not establish that the UI normally permits creation without a worker.

## Connected intake and output checks

Ten pipeline cases execute selection commit, complete original intake, then the
actual specialist-output/replacement loop. All worker squares yield `2/1/1`;
Doctor and Engineer are available and nonobsolete with stock outputs. These
connected cases have coherent population, worker and specialist counts.

Confirmed results:

- A stored Doctor changed to Engineer at population five persists through
  intake and contributes `3 economy / 0 psych / 2 labs`.
- Creating Engineer from a worker preserves `[Doctor, Engineer]` and contributes
  `3/2/2`; worker-hit and background paths remove their specified different indices.
- With `[Engineer, Doctor]`, choosing Worker on the first slot decreases count
  without shifting. Intake adds a worker; Engineer remains and contributes
  `3/0/2`. The last Doctor was dropped.
- Choosing Worker for the only Doctor leaves no specialists and intake assigns
  an additional worker.
- Creating the seventeenth specialist with Engineer chosen, or editing an overflow
  specialist to Engineer, does not persist Engineer. Under the controlled rules,
  overflow selects Doctor, so seventeen specialists contribute `0/34/0`.
- Cancel, same type and lock refusal retain Doctor and citizen management in
  the controlled complete-allocation cases.

Do not hardcode Doctor for overflow. The
[automatic selector/replacement contract](specialist-replacement-and-overflow.md)
determines its actual type. The later output loop can also replace obsolete
stored types independently of the menu's current-type exception.

## Evidence and limits

- [14 menu cases](manual-specialist-menu-cases.json), plus 800 combinations,
  execute all seven gates with actual technology helpers. Resolved availability
  and obsolescence use synthetic None/Disable records; stock tech acquisition
  is not being re-tested.
- [15 commit cases](manual-specialist-commit-cases.json), plus 900 combinations,
  verify current type, cancel/same/lock branches, storage, counts, worker source
  and management. Some slot/count combinations are synthetic local boundaries.
- [Ten pipeline cases](manual-specialist-pipeline-cases.json) compare final
  workers, count, effective output types and contributions after actual routines
  compose, with supplied edit-lock and hit-test inputs.

Remaining work includes complete hit testing/rendering, permission/network
admission, earlier refresh effects, full general recalculation and downstream
citizen consequences, malformed type encodings and live-game confirmation.
