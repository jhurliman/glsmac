# RE-SMAC-SPECIALIST-REPLACE-001: obsolescence and overflow slots

Publication scope: this repository includes this handoff and its literal cases. Other research notes named as unpublished follow-ups below are outside this published evidence boundary and do not expand its claimed scope.

The allocator orchestration follow-up (unpublished follow-up `worker-allocator-orchestration.md`)
separately observes final inline filling with its own−999 initial best score.
Its stock Technician/Doctor comparisons do not establish custom all-nonpositive
or no-eligible-catalog behavior for this selector or that inline fill.

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 19 named and 896 exhaustive technology/type/count combinations
pass. These execute the specialist-processing loop, original technology
helper and original automatic selector. Selected slots, specialist count and
technology knowledge are supplied. Worker assignment and UI legality remain
outside this scope.

## Replacement of selected types

For each of the first sixteen selected specialist slots, begin with its stored
type. If the faction has that type's obsolescence technology, search specialist
types in their installed order:

Technician, Doctor, Librarian, Engineer, Empath, Thinker, Transcend.

Choose the first candidate satisfying all three conditions:

1. Its prerequisite is available to the faction.
2. Its obsolescence technology is not available to the faction.
3. Each of economy, psych and labs output is at least the old type's
   corresponding output.

Persist the replacement in that selected slot and use its outputs immediately.
Equality in all outputs qualifies. The routine accepts the first qualifying
candidate; it does not search for the highest weighted score or the largest
improvement. If no candidate qualifies, retain and use the obsolete type.

This loop tests obsolescence before attempting replacement. It does not repair
a selected type merely because its own prerequisite is missing. For example,
a supplied Engineer or Transcend without its prerequisite remains unchanged
here. That is a boundary of this loop, not permission to select it through
the normal UI or proof it survives every earlier worker-assignment stage.

## Stock prerequisite and obsolescence rules

| Type | Prerequisite | Obsolete with |
| --- | --- | --- |
| Technician | None | Fusion Power |
| Doctor | None | Centauri Meditation |
| Librarian | Planetary Networks | Mind/Machine Interface |
| Engineer | Fusion Power | Never from this table |
| Empath | Centauri Meditation | Secrets of Alpha Centauri |
| Thinker | Mind/Machine Interface | Secrets of Alpha Centauri |
| Transcend | Secrets of Alpha Centauri | Never from this table |

With the installed [output triples](psych-allocation-and-specialist-output.md),
Technician becomes Engineer after Fusion Power, Doctor becomes Empath after
Centauri Meditation, and Librarian becomes Thinker after Mind/Machine
Interface. Empath and Thinker become Transcend after Secrets of Alpha Centauri.
The search can skip obsolete intermediate types: a Doctor with both Centauri
Meditation and Secrets of Alpha Centauri goes directly to Transcend.

These are direct technology-possession tests. Cases deliberately include
knowledge sets that omit technology-tree prerequisite closure to isolate the
gates; they do not establish legal research or editor transitions.

Two explicit custom-output cases verify the algorithm's boundaries. Reducing
Engineer's economy below Technician's leaves an obsolete Technician unchanged
when no candidate dominates it. Giving Engineer the same triple as Doctor
allows it to be selected before a later, strictly better Empath. Do not turn
the stock upgrade examples into a hardcoded upgrade chain.

## Seventeenth and later specialists

The first sixteen have individual persistent selections. Every specialist
beyond those sixteen obtains an initial type from the automatic selector,
then runs the same obsolescence/replacement test. Those overflow choices do
not extend or overwrite the sixteen persistent slots.

The automatic selector considers types whose prerequisite is available. It
does not exclude obsolete types at this initial selection step. For each
candidate, calculate:

- Below the configured minimum specialist base size: `score = 3*psych`.
- At or above that size: `score = economy + 3*psych + labs`.

Choose the greatest score; ties keep the earliest type in installed order.
The default minimum-size rule is 5. This threshold changes the selector's
weights; it does not, by itself, establish manual specialist eligibility or
worker-assignment rules.

The subsequent replacement stage matters. With all five relevant technologies
and a custom threshold above current population, the selector initially chooses
the obsolete Doctor because of its psych-score tie priority, then replaces it
with Transcend. At the threshold, the selector initially chooses Transcend
directly. Output agrees, but the selection paths differ.

## Acceptance scope

The [19 named cases](specialist-replacement-and-overflow-cases.json) cover
stock changes, unavailable selected types, custom dominance/tie boundaries,
mixed slots, sixteen/seventeen specialists, threshold equality and count 127.
The additional grid covers all 32 knowledge subsets of the five relevant
technologies, all seven initially selected types and counts 1/16/17/20.

Each comparison checks stored selections afterward, effective type per
specialist, overflow selector choices before replacement, and total
economy/psych/labs contribution. Original instructions and helper returns are
unmodified. Default specialist definitions use installed prerequisites,
obsolescence and output values; the two custom-output cases are explicit.

This closes the availability-neutral restriction of the earlier output test
for this processing loop. Specialist count construction, tile/worker
reassignment, manual selection rules, technology acquisition timing, final
energy processing and full-game observations remain open.
