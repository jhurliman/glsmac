# RE-SMAC-MAJORITY-001: potential diplomatic-majority selector

Status: original-x86 checked for the pinned SMAC reference in the directory
README. Twenty-six named cases, 1,000 combinations and five complete dominance
chain integrations pass without hooks, substitutions, byte patches or skipped
instructions. Synthetic technology prerequisites are used. This helper is not
the ballot or actual election/victory winner calculation.

## Total and exclusions

With Diplomatic victory disabled, return no faction immediately. Otherwise sum
the [council vote weights](council-vote-counts.md) of all seven playable factions
to obtain the denominator. Native population is not part of this total.

Visit playable factions in ascending faction order. Skip the explicitly
excluded faction. When an exclusion is supplied and Cooperative victory is
enabled, also skip a candidate whose directed relationship toward the excluded
faction contains Pact. The reverse-only Pact does not exclude it. With no
exclusion selector, this Pact filter is bypassed; with Cooperative victory off,
the additional Pact filter is bypassed.

Excluded factions' votes stay in the denominator. For example, excluding the
6-vote faction from a 6/4 distribution leaves no qualifier; the 4-vote faction
does not become a majority of a recomputed smaller pool.

The case file uses exclusion 0 for no excluded playable faction. Negative
selectors and their Cooperative-victory interactions are not a general public
contract here. The dominance integrations use the original caller's negative
selector with Cooperative victory disabled.

## Query-perspective technology gate

The technology gate checks the **querying faction**, not the candidate being
visited. For a positive playable querying faction, admit candidates when it
knows the configured Supreme-Leader requirement technology or either of that
technology's two immediate prerequisites. The tests use a synthetic required
technology and prerequisite graph, with the original technology resolver.

A no-required-technology rule admits a positive querying faction; a disabled
requirement rejects all candidates. Querying faction 0 fails these technology
checks even under the no-requirement rule. A candidate's own knowledge does
not substitute for missing query knowledge. A qualified query can therefore
admit a candidate that lacks the technology itself.

This describes this helper's perspective gate. It does not establish legal
election candidates or prove that knowing a prerequisite is sufficient to
propose or win Supreme Leader in actual council play. The installed rule's
technology identity and full proposal/ballot consumers are separate contracts.

## Threshold and preference

A surviving candidate qualifies when twice its vote weight is at least the
unchanged total. Thus **exactly half qualifies**; more than half is not required.

Retain the first qualifier in faction order unless the querying faction also
qualifies, in which case prefer it. Examples:

| Votes | Query | Exclusion | Selected faction |
| --- | --- | --- | --- |
| Faction 1: 4, faction 2: 3 | 1 | None | 1 |
| Faction 1: 3, faction 2: 4 | 1 | None | 2, if query's technology gate passes |
| Faction 1: 4, faction 2: 4 | 2 | None | 2 |
| Faction 1: 4, faction 2: 4 | 3 | None | 1 |
| Faction 1: 6, faction 2: 4 | 1 | 1 | None |
| Three factions with 3 each | 1 | None | None |

There is no positive-vote/base-presence eligibility test in this loop. At a
synthetic zero total, zero-vote factions qualify under the arithmetic, so a
technology-qualified query can be selected despite having no base. This is a
boundary case, not normal election eligibility. No separate alive/human filter
is applied in the checked selector.

## Dominance-chain integrations

Five integrations execute the original [dominance predicate](leading-human-dominance-score-path.md),
its pressure helper, this selector, and original vote/technology routines. They
use a human leader with six one-population bases, a one-base rival, time/base
admission satisfied, equal leading/runner-up scores, Diplomatic victory enabled,
Cooperative victory off and no Voice of Planet.

A resolved human future-pressure date later than current turn activates the
tested pressure path. With the query's required technology known:

- Leader votes 6, rival votes 1 selects the leader and makes dominance true.
- Leader votes 6, rival votes 10 fails that shortcut; equal scores also fail
  the ordinary ratio, leaving dominance false.
- Votes 6/6 selects the querying leader and makes dominance true.

With that pressure input inactive, the tested state remains false despite the
leader's majority. A disabled technology requirement also prevents the shortcut.
The future-pressure field's producer and full victory meaning are unresolved;
these are resolved-state integrations, not a claim of completed victory setup.

## Verification and open work

The [26 named cases](potential-diplomatic-majority-selector-cases.json) cover
half/odd thresholds, preference, exclusions/denominator, asymmetric Pacts,
victory toggles, technology perspective/prerequisites, query 0, zero votes,
native population and faction multipliers. The 1,000-case grid varies those
inputs with the real vote and technology helpers. Five additional integrations
exercise the complete dominance chain on a coherent synthetic map/base state.

Full election eligibility, ballot choices, governor/supreme-leader winning
thresholds, victory/defiance, pressure-date lifecycle, the remaining economic
or transcendence pressure paths, saves, networking and observed full-game
behavior remain open.
