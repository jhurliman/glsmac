# Canonical Base specialist queries

This slice supplies a read-only specialist menu and automatic initial choice
from an actual canonical Base. It does not activate manual worker commands,
alter population assignments, or run the original resource/citizen handler.

## Existing authority and admission

`Base::GetSmacSpecialistMenu` accepts an optional stable Pop ID. An absent ID is
the Worker/new-specialist menu context; a present ID must belong to that Base.
The current type comes from the registered PopDef's positional CITIZENS role.
A worker's current type is absent. Neither display names nor definition names
select a specialist. The native result owns its seven eligibility bits, current
type and minimum-size explanation flag.

`Base::SelectAutomaticSmacSpecialist` returns the initial positional choice used
by the measured automatic selector. It does not perform the later obsolescence
replacement pass or create an overflow population record. It never stores a
cached choice in the Base.

Both methods require the Base's present owned resource context, exact manager
and center membership, current simulation/roster/session binding, matching owner
faction and valid registered population roles. Technology possession is copied
from that Base owner's saved simulation ID, rather than the local controller,
transport slot or faction template. Frozen CITIZENS and technology records are
validated before evaluation; an unknown prerequisite or obsolete token rejects.
`None` is available and `Disable` is unavailable. A resolved ordinary technology
must be ID 0..87, its first definition prerequisite must not be Disable, and a
second Disable is allowed only when the first is None. It must then be owned;
parent possession is not recursively required. Future Technology (88) resolves
but is unavailable through this helper, for both prerequisite and obsolete
queries. Unknown evaluated definition tokens reject instead of becoming Disable.

These are technology/population queries. They do not read social ratings and
can run while valid social state is Early without inventing a retained value.
Population is admitted in 0..127. An existing-specialist current-type query
rejects Bases with more than sixteen specialists: the current Pop representation
does not yet bind its stable IDs to the original sixteen stored slots plus
automatic overflow policy. Worker/new context and the initial automatic choice
do not read those overflow selections.

GSE exposes `base.get_smac_specialist_menu(pop_or_undefined)` as a detached
object with `current_type`, ordered `eligible_types` and
`size_explanation_requested`, and `base.get_smac_automatic_specialist()` as the
positional integer. Worker is the implicit non-specialist option. No new event,
UI action or default startup registration calls these methods. Legacy work and
unwork routes remain unchanged, including their canonical-context rejection.

## Measured rules

The menu keeps a type only when its prerequisite is available, it is either
nonobsolete or the current type, and population reaches the configured minimum
or the type supplies at least two psych. The current obsolete exception does
not bypass prerequisite or size checks. The explanation flag is set only when
a type passed the first two gates and failed the size/psych gate. Signed psych
metadata is retained, including negative values.

The automatic initial selector considers available prerequisites in source
order, including obsolete types. Below the configured minimum its score is
`3*psych`; at or above the minimum it adds economy and labs. Strictly greater
scores replace the current best, preserving earlier types on a tie. The
subsequent replacement/output loop is a separate consumer.

The port rejects evaluated signed32 overflow, including an intermediate sum
whose later addition would make the final value fit. Below the size threshold
unused economy/labs terms are not evaluated. At least one available positive
score is required. The retained original cases have positive winners; behavior
for an all-nonpositive or entirely unavailable custom catalog is deliberately
not inferred from an unverified original selector fallback.

## Original projections and actual Base checks

`tools/reference/project_smac_specialists.py` reads and fingerprint-checks the
human-owned JSONs, then writes literal expected results into the owned fixture.
It does not implement an expected-result algorithm:

- All fourteen `RE-SMAC-MANUAL-SPECIALIST-MENU-001` named cases, source SHA-256
  `81497122828840b077fca52640e59f289e21b8854ef59a8b6e3893e085bd093e`.
- Six of nineteen `RE-SMAC-SPECIALIST-REPLACE-001` cases that actually record
  overflow initial choices, source SHA-256
  `985c6f10bd3dcb83f852043ccafbed0ef38f720f27fd7838a1fdffe9e0e473d4`.
  These six contexts contain 119 repeated selector outputs. The other thirteen
  cases concern stored selection/replacement and supply no initial-choice
  oracle for this slice. Effective replacement results are not compared here.

The stock output triples and source ordering are retained. Menu availability
uses the original fixture's supplied None/Disable gates. Selector possession
uses equivalent synthetic catalog keys for the five named technologies. No
original executable or unpublished exhaustive grid is run by this change.

The retained specialist/menu harnesses explicitly execute original helper
`0x5d17b0`, the same helper measured by
`technology-token-and-production-prerequisites.md`. This call identity is needed:
the five enabled stock technologies and None/Disable menu rows alone cannot
distinguish raw possession from the ordinary definition guard. The initial
draft used raw possession; peer review identified that mismatch before commit.
The final tests separately cover enabled 87, owned Future 88, disabled first/second
definition prerequisites, the asymmetric None/Disable case, and their obsolete
uses. A copied raw-possession control fails the new definition guard assertion.
The three retained harnesses and reports are fingerprinted in the local manifest;
none was rerun or modified.

Actual native/GSE tests construct registered Bases with deliberately repeated
display names, distinguish two Base owners' technology, reject a foreign Pop
with the same numeric ID, and mutate detached query output. They verify unchanged
Base serialization (including governor bits, timer and assignments), both gameplay
RNG words, the separate JSF generator and frontend queue. A real saved world body
restores into absent or different current simulation, then recomputes from saved ownership. Changing
supplied ownership changes the query without reassigning population; retiring
the roster/session rejects it. Invalid roles and overflow ambiguity also reject.

## Remaining manual-handler dependency

`manual-worker-handler.md` establishes a cacheable full calculation before
edit permission and a forced calculation after admitted edit cores, in the
order support, intake, nutrients, minerals, energy, citizens. An intake-only
callback is not that pipeline. Earlier rejection can preserve its preliminary
allocation; a later core rejection may still clear citizen management. Queries
here neither clear that bit nor claim those handler effects.

The existing Pop list remains the single population/assignment authority. It
cannot yet express the original temporary center-clear state (zero selected
workers and zero specialist counters while population is unchanged), provisional
specialist counts, or sixteen persistent selections plus automatic overflow.
Manual worker replacement additionally needs current aggression/directives,
support and complete intake; drag/click handling needs measured unavailable
square production and allocation repair. The complete recalculation must settle
those temporary states before command publication. A mutable core returning
“recalculation required” is intentionally not exposed here.

No universal Doctor population, worker auto-allocation, specialist E/P/L
aggregation, support/energy/citizen simulation, human UI permission, multiplayer
command admission or default activation is supplied by this slice.

## Validation

Source is based on Base context `8a086296c2cbb712e4870fcd04f391e2eda44d01`,
including the exact specialist-catalog prerequisite. Seven dedicated native groups
pass with allocation tracking in normal and ASan/UBSan/LSan builds. Seven copied
production controls fail their intended assertions: missing current exception,
psych-one admission, automatic obsolescence filtering, later tie replacement,
wrong Base technology owner, a fixed type instead of the current Pop role,
and raw possession in place of ordinary technology availability.
The checked-out source is never mutated by these controls.

Final corrected-source local Linux checks:

- Full normal CTest: 8/8, 87.22 seconds.
- Full ASan/UBSan/LSan CTest with leak detection and halt-on-error: 8/8,
  184.58 seconds.
- All seven groups passed with allocation tracking in both binaries.
- The projection script rechecks the exact owned fixture against both pinned
  original JSON files, without executing the original binary.

The full production correctness scan before the availability correction covered
342 translation units with zero failures in 122.18 seconds (126.15 including the
driver). Shared headers did not change afterward. The final changed production
TU passes explicit correctness analysis, zero failures in 1.21 seconds (driver
4.37). This is a full earlier scan plus a final changed-TU check, not a second
full scan of the corrected source. The earlier normal 8/8 (94.16 seconds),
sanitizer 8/8 (182.60 seconds), and six-group receipts remain retained separately.

Logs, copied TUs and source fingerprints are retained under
`/home/jhurliman/src/glsmac-startup-main-refresh/` with `specialist-query` names.
The first build's four diagnostic failures were test-message macro syntax,
corrected before execution; that failed log is retained separately. These are
local Linux checks, not native Windows/macOS or hosted CI evidence.
