# RE-SMAC-QUEUE-001: queue advancement and build eligibility

Status: original-x86 checked for the SMAC fingerprint in the
[research ledger](README.md). All 2,489 scenarios pass: 969 complete queue-helper
checks, 1,440 complete unit-eligibility checks and 80 admitted Stockpile/credit
compositions. Presentation and a project-selection signal are supplied; actual
queue, eligibility, technology/facility, coastal and counter-rebuild instructions
execute. Full chooser/queue-edit UI, complete production aftermath and turn/income
sequencing remain outside the execution scope. The complete facility/project
predicate is now separately specified in
[facility/project eligibility](facility-project-build-eligibility.md).

## Exact queue advancement

The [completion-to-queue composition](production-completion-grants.md) now
executes the actual caller's grants before this helper. It verifies that newly
completed prerequisites and own-project substitutions/exclusions affect the
immediately promoted candidate.

The helper receives a base with a current target and a list of pending targets.
It performs this operation:

1. If the pending list is empty, return failure without changing or revalidating
   the current target.
2. Otherwise remove the current target's unit-production reference if it is a
   unit, move the first pending entry into the current target, and shorten the
   pending list by one.
3. Validate this now-current candidate. A unit uses the unit eligibility helper;
   a negative item uses the facility/project helper with **zero queue depth**.
4. If acceptable, request a refresh and return success immediately. A selected
   Secret Project also requests its project-selection signal.
5. If unacceptable and more entries remain, repeat from step two. If none
   remain, return failure with the **last rejected item still current**.

This helper does not choose a fallback, restore the old target or clear the
last rejected target. Later caller behavior remains separate. It does not
validate pending entries beyond the first acceptable candidate, nor wait for
an earlier rejected prerequisite to become available.

Success means an acceptable entry was popped. It can return success when that
entry is identical to the old current target. Conversely, an empty queue returns
failure even when the current target is acceptable; an inactive current design
is also left untouched in the empty-queue case.

No minerals or switching-baseline fields change in this helper. It does not use
the automatic retool setter. In particular, a queued unit-to-facility transition
can preserve a supplied 31-mineral store and 31-mineral baseline. Normal completion
carryover is handled earlier in the [production path](production-posting-and-unit-creation.md).
Do not apply another speculative switching penalty during this queue operation.

Refresh is requested only on success. If all candidates fail, the current target
may have changed without a refresh request from this helper. The project signal's
body, audio/display behavior and later caller handling are not executed.

## Production reference counters

An admitted original manual-chooser exit fragment rebuilds counters by counting
the current target and every valid pending position that contains a unit design.
Duplicates count separately. It does not test eligibility during this rebuild.
The full chooser exit is not executed, but that counter loop is.

The queue helper decrements the unit reference for each target it removes from
the current position. It does not increment a newly promoted unit reference:
that reference was already counted while pending. Thus three occurrences of
one unit in current/pending positions become two references after one pop,
even though the design identity stays the same.

If the final candidate is rejected and remains current, its reference remains
counted. These are production-reference counters, including duplicate queued
entries in one base. Existing [automatic setter](production-switching.md)
counter changes describe a different mutation of the same bookkeeping.

## Unit eligibility helper

The complete checked unit helper applies these guards in order:

1. The design must be active.
2. The design must not exclude the requesting faction.
3. For a predefined design, the requesting faction must be positive and its
   predefined prerequisite must be unconditional None or a valid, available,
   owned technology. Disable and an out-of-range technology reference are
   rejected. The fixture's technology reference 88 exercises the range refusal.
4. A Colony-role design is rejected when the resolved no-colony rule is active.
5. With a base context, sea-domain designs require adjacent sea as below.
6. A custom design must belong to the requesting faction.

The checked custom designs do not pass through the predefined prerequisite
test. An active own custom design with its stored predefined-prerequisite input
set to Disable can still pass. This does not establish component availability or
design-creation legality; those are checked in other workflows. A design need
not already be prototyped to pass this helper; prototype charges are specified
in [unit cost](unit-design-and-prototype-costs.md).

The faction-exclusion input's producer and lifecycle remain unresolved. These
tests use it as a resolved capability, rather than assigning it an unverified
retirement/obsolescence interpretation. Requester-zero cases are synthetic
boundaries; the checked predefined and custom designs reject them.

For technology references, possession alone is insufficient if the technology
definition is disabled. The synthetic definition controls include owned ordinary,
unowned, disabled-first-prerequisite and disabled-second-prerequisite records.
The unusual definition with first prerequisite None and second Disable passes
this validation when owned; an ordinary first prerequisite with second Disable
does not. These are definition-boundary checks, not default technology content.

The [token and prerequisite contract](technology-token-and-production-prerequisites.md)
now connects these guards to the complete original technology loader/resolver.
It distinguishes all89 resolvable catalog keys from production references0–87,
and the loader's stricter definition-count predicate from this owned query.

### Sea adjacency

For a sea chassis queried with a base, inspect the eight immediate neighbors in
doubled horizontal coordinates:

```text
(1,-1), (2,0), (1,1), (0,2), (-1,1), (-2,0), (-1,-1), (0,-2)
```

At least one valid neighbor must be sea. Wrap horizontal coordinates on wrapping
maps; clip them on flat maps. Clip vertically in both modes. This check excludes
the center tile itself. A synthetic isolated sea-base tile surrounded by land
therefore does not qualify, while an adjacent sea tile qualifies a land base.
The no-base query omits this coastal requirement. Land and air designs have no
corresponding adjacency gate in this helper.

This predicate does not establish the complete legality of constructing every
unit at every base. The separately tested active design and faction-owner gates
still apply; command/workshop/technology lifecycle remains broader than this
one consumer.

## Actual negative-item checks exercised by the queue

The queue's zero-depth validation does not count a promised future facility as
an already available dependency. Its original facility/project helper executes
in these controlled cases:

| Candidate | Checked acceptance/refusal |
| --- | --- |
| Recreation Commons | Existing local copy rejects; absent copy with resolved prerequisite available accepts |
| Hologram Theatre | Requires the existing Recreation Commons in the tested no-free-project context |
| Hybrid Forest | Requires the existing Tree Farm |
| Sky Hydroponics Lab | Requires Aerospace Complex or same-faction Space Elevator; foreign Elevator does not substitute |
| Stockpile Energy | Accepts with its explicit prerequisite available |
| Human Genome Project | Rejects the resolved no-project rule or an already completed project, including completion by another faction |
| Other fixture local entries | Explicit Disable or unavailable technology rejects before later checks |

Prerequisite fields are controlled resolved rules inputs, frequently None to
isolate queue behavior. They are not a claim that the installed technology
requirements are None. The follow-up
[complete facility/project predicate](facility-project-build-eligibility.md)
adds all item-specific upgrades, own-project exclusions/substitutes and the
special Ascent query. Project races, grants and victory execution remain open.

For example, a queue `[Hologram Theatre, Recreation Commons]` with neither
facility present rejects the Theatre and selects the Commons. The rejected
entry is not deferred until the Commons completes. When Commons already exists,
the Theatre can be selected immediately. A queue ending in an unavailable unit
leaves that unavailable design current and returns failure.

## Stockpile baseline and treasury-credit composition

The [posting contract](production-posting-and-unit-creation.md) identifies the
shared switching-baseline gate for Stockpile. Eighty additional cases compose:

1. The actual queue helper selecting Stockpile, or an already-current Stockpile
   target with no pending entries.
2. Optionally the actual admitted outer baseline writer.
3. The actual Stockpile energy-addition fragment, with earlier energy-category
   output supplied as a resolved input.
4. The actual base-upkeep treasury-credit instruction.

At that credit stage, add the base's resulting economy output to the owning
faction's treasury. The checked reservation balance and foreign treasury remain
unchanged by that credit fragment. The fragment applies no further multiplier
to the Stockpile addition.

With old baseline a unit, newly queued Stockpile, mineral surplus five and
resolved other economy zero, the energy addition is zero before the baseline
writer. After the writer it is three; the admitted credit stage changes treasury
100 to 103. With an already-Stockpile baseline, the three-point addition does
not require another baseline commit. Nonpositive mineral surplus gives no
Stockpile addition. Earlier economy seven is preserved and the addition stacks
on it before credit.

Static caller tracing places the baseline writer before population processing
and later resource recalculation, and the treasury credit after additional
citizen/maintenance stages. Those intervening stages are not executed in this
composition. This verifies the gate, producer and credit consumer; **complete
first-turn Stockpile income and full completion-to-income timing remain open**.

## Fixtures and remaining scope

The [fixture bundle](production-queue-cases.json) contains 35 named queue cases,
24 selected unit-eligibility conditions and all 80 income compositions. The
additional grids produce 969 total queue checks and 1,440 unit checks. Queue
fixtures include empty, duplicate, invalid-prefix/all-invalid, maximum tested
nine pending entries, technology, ownership, Colony, sea wrapping/clipping and
selected negative-item dependencies. The helper's queue-edit admission and
maximum permitted queue length are not established by these calls.

Rules tables, active/exclusion state, technology definitions and project/facility
state are explicit synthetic inputs. All queue/eligibility/counter/coastal/
baseline/energy/credit instructions execute unmodified. Complete chooser and
queue editing, design lifecycle, fallback/governor decisions, full
production/turn/income order, saves and live observations remain open.
