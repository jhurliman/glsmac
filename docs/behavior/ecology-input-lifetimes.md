# Ecology inputs: initialization, retained state and restore

RE-SMAC-ECOLOGY-INPUT-LIFETIMES-001, 2026-10-18. Pinned stock SMAC.
[Literal lifetime controls](ecology-input-lifetimes-cases.json). This closes
named input origins for [live Base ecology and its research aggregate](base-ecology-research-aggregate.md),
reusing its arithmetic, caller timing and CRT evidence.

## Actual reset and preservation boundaries

| Input | Fresh turn-zero faction initializer | Nonzero-turn faction initializer | Ordinary file restore |
| --- | --- | --- | --- |
| Faction clean-mineral history H | Clears poisoned value to 0 | Preserves poisoned value | Restores saved word |
| Faction major-atrocity count | Clears poisoned value to 0 | Preserves poisoned value | Restores saved word |
| Retained faction ecology/research aggregate | Preserved through tested initializer prefix | Preserved through tested initializer prefix | Restores saved word |
| Global cumulative sea-level offset | Separate map authority | Not a faction reset authority | Restores saved word |

Four native faction prefixes distinguish turn0 and turn1, two owners and peer
preservation. They run through the history/deadline initialization region;
they do not claim a complete restart controller. These are poisoned-state
reset tests, not deductions from cold executable zeros. Do not reset H or
atrocities whenever a faction is reintroduced at a nonzero turn.

The **complete native blank-map helper** resets a poisoned sea offset to0.
Source inspection identifies the corresponding reset in the map-generation
initialization path as well. This global counter belongs to map state; it is
not faction history, current displayed sea elevation or an estimate from
current tile classes. Fresh ordinary map preparation uses the actual reset
authority. Restoring a world must preserve its saved cumulative value.

An actual ordinary file round trip restores all eight slots of H, major
atrocities and the aggregate, plus sea offset: **25 saved words**. Inputs are
distinct diagnostic values, poisoned between save and load. No gameplay/CRT
RNG call occurs during those file bodies. The blank-map preparation has its
own existing shared-RNG work; it is not represented as an RNG-free operation.
Full frontend load/new-game caller admission remains separate.

## Subsequent writers and consumers

H remains an event history, not a count reconstructed from current facilities.
The existing [clean allowance contract](clean-minerals-and-ecology-burden.md)
establishes nonzero-H increments on Tree Farm, Hybrid Forest, Centauri Preserve
and Temple of Planet completion, plus the selected ecological fungus-event
increment and AI retention/rollback stage. Those controls are reused here;
full bloom placement, capture/removal histories and all action triggers are
not expanded by this receipt.

The major-atrocity routine increments the actor's stored count. Two admitted
native increment controls confirm0→1 and7→8. The final ecology producer reads
that count for its human atrocity term. This does not establish every action
that calls the atrocity routine or all diplomatic consequences.

Two source-identified sea-step paths update the cumulative offset by the
applied signed step and reduce the pending change by that step, then enter
terrain recalculation/notification. Admitted positive/negative controls on the
standalone step path confirm offset12→13 / pending2→1 and offset12→11 /
pending−2→−1, stopping **before** terrain recalculation. The global advancement
path contains the equivalent stores. These fields distinguish cumulative
change from pending future change; neither should be recomputed as the other.
No complete climate/council/terrain conversion audit is claimed.

Current final Base damage still follows the established
[ordered scaling](eco-damage-scaling.md), including the active perihelion bit,
current Voice marker/rule, native-life setting, PLANET, and stored acquisition
score minus future counter. The existing [perihelion schedule](perihelion-schedule.md)
and [technology-score transitions](technology-score-transitions.md) retain their
own writer/admission boundaries. Restored ownership count is not a substitute
for the stored acquisition score, and no extra refresh is added by this work.

## Aggregate and RNG ordering: unchanged measured contract

The owner Base/research pass clears only that owner's aggregate before Base
traversal. Each Base performs research before its ecology contribution. Then
the ecology handler adds
`trunc(4 * finalBaseDamage / clamp(4 - currentPLANET, 1, 99))`, and draws **once
from the existing CRT stream even when final damage is0**. Event-suppression
gates follow both operations. Resource preview alone does neither operation.

Thus first-Base research sees0, second-Base research sees only the first
contribution, and the later owner summary sees the completed total. The next
owner pass clears it again. The ten existing complete two-Base controls are
verified and reused, including a following low-damage pass and an untouched
peer canary. Loading a saved aggregate is not permission to clear it early
or synthesize the completed total at every research query.

The [startup/CRT receipt](startup-capability-and-ecology-rng.md) already counts
the zero-damage first-Base draw in its controlled six-advance startup. Do not
add a seventh draw or create a separate ecology RNG. This follow-up adds four
initializer prefixes, one complete map-reset observation, one25-word round trip
and four admitted writer controls; it does not repeat the earlier grids.
