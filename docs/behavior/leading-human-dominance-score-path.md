# RE-SMAC-DOMINANCE-001: leading-human dominance, score path

Status: original-x86 checked for the pinned SMAC reference in the directory
README. Twenty-two named cases, 1,000 combinations and three integrations pass
against the complete predicate and original consumer routines, without hooks,
substituted results or instruction changes. The separate victory-pressure
shortcut is held inactive; its true behavior is not specified here.

## Admission to the score comparison

The queried faction must be human-controlled and currently occupy the leading
faction slot. Its base count must be **greater than** the larger of 4 and the
integer quotient of current turn plus 25 divided by 50.

On valid difficulty settings 0 through 5, a mode argument of zero excludes
difficulty 0/1 unless Intense Rivalry is active. A nonzero mode argument bypasses
this particular exclusion; it does not bypass the remaining prerequisites.

Current turn must then be **greater than** the time threshold:

| Difficulty | Normal threshold |
| --- | --- |
| 0 | 350 |
| 1 | 300 |
| 2 | 250 |
| 3 | 200 |
| 4 | 150 |
| 5 | 100 |

With Intense Rivalry, the threshold is 100 at every difficulty. Equality at a
time threshold fails. For example, difficulty 2 at turn 250 fails; turn 251 can
qualify, but then needs more than five bases. Five fails and six can qualify.

These are elapsed game-turn values, not displayed mission-year values.
Negative turns and overflow are outside the tested contract.

## Score-path result

After admission, compare the leading faction's resolved ranking score with the
runner-up's resolved ranking score:

| Condition | Required leading score |
| --- | --- |
| Intense Rivalry | At least 1.25 times runner-up |
| Otherwise difficulty below 3, Conquest victory disabled, and scenario Objectives Required for Victory greater than 1000 | At least twice runner-up |
| Otherwise | At least 1.5 times runner-up |

Use exact integer cross-products; do not round a floating-point ratio.
Equality qualifies: scores 150/100 qualify for the normal branch, 125/100 for
Intense Rivalry, and 200/100 for the double-score branch. One point below each
fails. The objective threshold is strict: 1000 uses the ordinary ratio; 1001
can select the double-score branch when the other conditions hold. Intense
Rivalry takes priority over that branch.

The score inputs are ranking scores, not assumed population, territory, economy
or military totals. Their complete construction and rank-refresh lifecycle
remain separate research. A zero/zero synthetic score pair passes the ratio
comparison once admission succeeds; this is not evidence for a normal leading
faction with no score.

## Original consumer integrations

Three integrations use a human leader with six bases, a runner-up with one,
difficulty 2, normal rivalry, Conquest enabled and scores 150/100. The leader
has rank 7 and runner-up rank 6, so the war handler's independent rank-pair
penalty does not qualify. They compare turn 251, turn 250 and score 149:

| State | Dominance | AI nutrient cost per step | Human cost per step | Blemishes from checked Treaty break |
| --- | --- | --- | --- | --- |
| Turn 251, score 150 | True | 5 | 10 | 2 |
| Turn 250, score 150 | False | 6 | 10 | 1 |
| Turn 251, score 149 | False | 6 | 10 | 1 |

Costs use default nutrient multiplier, Standard world, zero GROWTH, no local
growth additions and AI global difficulty 5. Original AI cost first uses its
difficulty-table factor, subtracts one for a human leader in local mode, then
subtracts one more when this predicate is true. The human branch starts at 10
and does not apply those AI deductions. Subsequent scaling and local GROWTH
remain governed by the [growth contract](growth-and-starvation.md).

The [war integration](local-war-treaty-truce-integrity.md) has no instigator,
Pact, justification or penalty skip. One blemish comes from nonzero actor
difficulty and the second from the true dominance helper, not the rank pair.

## Scope and verification

The [case file](leading-human-dominance-score-path-cases.json) contains 22 named
cases and three consumer integrations. The 1,000-case grid varies human/leader
state, base counts, turn boundaries, difficulty/mode, rivalry, conquest,
objective threshold and scores. Tests execute the original helper and reached
original subhelpers. Consumer integrations execute complete original nutrient
cost calculations and the uninstigated non-Pact war handler.

An additional original victory-pressure shortcut precedes the final ratio
comparison. These cases disable Diplomatic victory, omit Voice of Planet and
set human future-pressure dates to zero, so the shortcut is inactive. Its
eligibility, faction selection and interaction with other victory modes remain
open; do not implement the ratio table as the complete predicate for all games.
Rule-setting menus and rank/score construction are statically traced for input
identity, not exercised as full UI flows. Full victory/growth/war transitions,
networking, saves and live-game confirmation remain open.

The underlying [council vote-weight constructor](council-vote-counts.md) is now
separately checked, including projects and faction multipliers. This does not
yet close the shortcut's candidate-selection or victory conditions.

A further [potential-majority follow-up](potential-diplomatic-majority-selector.md)
now executes that selector and five complete dominance chains, including a true
shortcut with resolved future-pressure state. The pressure input's lifecycle
and complete victory policies remain open.
