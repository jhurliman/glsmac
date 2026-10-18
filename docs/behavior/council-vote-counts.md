# RE-SMAC-COUNCIL-VOTES-001: faction council vote counts

Status: original-x86 checked for the pinned SMAC reference in the directory
README. Twenty-three named cases and 1,200 combinations pass through the
complete original faction vote-count routine. No helpers, hooks, substitutions,
byte patches or skipped instructions are used. Election/candidate/winner logic
and full-game observations are outside this contract.

## Calculation order

For the queried playable faction:

1. Sum current population of every extant base currently owned by that faction.
   Foreign and native-owned bases contribute nothing to its total.
2. If the faction currently owns the Empath Guild's host base, add half of this
   faction-wide population total, truncating toward zero. For ordinary
   nonnegative population, this rounds the bonus down.
3. If it currently owns Clinical Immortality's host base, double the result
   after the Empath Guild addition.
4. Process each loaded faction VOTES modifier in list order. A nonnegative
   parameter multiplies the current result by that parameter. Ignore a negative
   parameter and continue to later entries.

The result is the faction's vote weight. Do not calculate the Empath bonus per
base, add project bonuses after faction multiplication, or replace these
stages with a single rounded percentage multiplier.

Project eligibility uses the current owner of the project host, not a permanent
record of who built it or a required match to the base currently being viewed.
A foreign host provides no bonus. Capture/transfer timing and project-location
maintenance remain separate research; this routine consumes their current state.

The base sum has no additional human/AI, faction base-count, governor,
specialist, facility, drone, energy, rank or diplomatic-relationship filter in
the checked routine. Population is the stored base population, not a count of
workers or talents. Other project/faction effects can still change population
or the resolved input state upstream.

## Rounding and stacking examples

| Population total | Empath Guild | Clinical Immortality | VOTES entries | Votes |
| --- | --- | --- | --- | --- |
| 3 | No | No | None | 3 |
| 3 | Yes | No | None | 4 |
| 3 | No | Yes | None | 6 |
| 3 | Yes | Yes | None | 8 |
| 3 | Yes | Yes | 2 | 16 |
| 3 | No | No | 2, 3 | 18 |
| 3 | No | No | 2, -1, 3 | 18 |
| 3 | No | No | 0, 3 | 0 |
| 1 + 1 in two bases | Yes | No | None | 3 |

Both projects with population 3 give 8, not 9. Applying VOTES2 before the
projects would give the wrong result. Zero is a valid multiplier here and
remains zero under later multiplication. Negative entries do not reduce votes
or terminate processing. The installed Peacekeeper VOTES2 rule is an example
of this faction multiplier; do not hardcode it as a faction identity check.

## Boundaries and evidence

The [23 named cases](council-vote-counts-cases.json) cover multiple bases,
ownership, odd population, project combinations, foreign hosts, VOTES stacking,
zero/negative modifiers, empty/zero totals, native bases and faction 7. The
1,200-case grid varies all playable queried factions, base counts/populations,
project-host ownership and modifier lists.

A synthetic negative-population case confirms signed truncation rather than
flooring: population -1 with Empath Guild returns -1. There is no lower clamp
in this routine. Negative population is not asserted to be a normal game state;
extreme integer overflow is unverified.

Faction modifiers are supplied as resolved loaded entries. The loader's VOTES
mapping is statically corroborated; the text parser and its shared modifier-slot
admission limits are not executed by these checks. No extra callback supplies
project arithmetic or ownership results.

This count is used by council-related consumers, including the potential
diplomatic-majority helper. It does not establish election eligibility,
abstentions, ballot choices, actual victory thresholds, candidates, ties,
Governor powers or Supreme Leader victory/defiance. Those consumers and the
remaining victory-pressure shortcut require separate specifications.

The separate [potential-majority selector](potential-diplomatic-majority-selector.md)
now specifies one consumer and integrates it into the original dominance chain.
Its half-vote comparison is not proof of an actual election-winning threshold.
