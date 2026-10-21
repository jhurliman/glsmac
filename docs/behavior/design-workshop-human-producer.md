# Independent stock-human Workshop normal-open producer

2026-10-20. Original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Use [workshop_human_producer_reference.py](workshop_human_producer_reference.py)
for the executable controller reference. It generates proposals from current
raw state and installed component definitions; there is no faction-name,
fixture-ID, final-tuple or generated-name dispatch.

Additive private freeze:
`tmp/reference/independent-workshop-human-producer-evidence-20261020/`.
Manifest SHA-256:
`797eb636eb5190160c628f951534f0ee10c866fe09cc19c6f6bd6cc8b4776c71`.

**22 original597de0 controller controls match all414 generated proposals,
ordered policy transitions and return values, whole Design and Faction pools.**
Base/Unit/history buffers stay unchanged. This includes all upgrade-plan
branches, five full-bank distinctions, late Clean Reactor, negative
personality, and a mid-prefix alias/unexclusion case. Twenty-one controls use
the human threat-stage value0; one explicitly supplies synthetic threat3 at
59855c/local2c to distinguish the nearby conditional additions.

The producer also matches **all71 proposals from the15 previously frozen
complete normal-open controls**. Its composed `normal_open()` matches all15
final drafts, selectors, raw flags, automatic names and full Design/Faction
pools. Those original x86 controls were reused, not rerun. In total485 proposal
expectations are checked; the22 new controls enter at597de0 rather than claiming
another whole-window/modal open.

## Port integration

`normal_open()` composes independent capability refresh, proposal generation,
preview submission, selector readback and the existing naming reference. Pass
current pools/technology/cached counts and installed personality. It returns
updated pools, selected inactive slot, draft tuple, selector slots, display
name, ordered proposals/steps and final controller mode/filter.

For a controller-only call, use `proposals(..., on_submit=callback)` with
[the preview policy](workshop_recommendation_policy_reference.py). The callback
must apply submission immediately and return the updated Design pool. **Do not
batch all proposals against the original pool and apply them afterward.**
An early alias can unexclude a later custom record, making that later record
eligible when the prefix loop reaches it. The focused control starts with
Synthmetal64 and excluded Plasma65 (cached unit count1): upgrading64 aliases65,
clears only owner1's exclusion bit, then the controller visits65 and submits it
too. The independent interleaved sequence matches both requests and side effects.

The high-level helper requires an inactive own slot, matching the prior New-row
selector scope. With a fully occupied bank, use the controller/policy boundary;
it does not fabricate an inactive UI row. If mode1 never finds an admitted
alias or replacement candidate, preserve the incoming provisional tuple. The
native full-no-alias control retains its poisoned tuple and mode1/filter-1.
A fixed Scout fallback would lose this behavior.

Capability refresh runs before generation. The high-level helper currently
admits stock intrinsic capability inputs, rules1a0e and neutral project state.
Its reactor readback retains the selector's entry reactor, as the original
4490b0 does. The older early-profile reference remains available but is not the
general implementation to extend or dispatch into.

## Ordered human role generation

The source implementation carries the original selection, cost loops and
ability priorities. Complete proposal lists, including proposals rejected after
output locks, are verified. The following order is observable:

| Stage | Category |
| --- | --- |
| Existing custom upgrades, ascending raw ID | Existing category byte43 |
| Defensive Infantry/minimum-speed land | 2 |
| Offensive minimum-speed land | 3 |
| Drop variant, then Amphibious variant | 11,12 |
| Artillery, when known | 1 |
| Fast land attack | 4 |
| Scout | 10 |
| Naval combat | 6 |
| Naval transport | 7 |
| Sea former, then sea colony | 15,16 |
| Air attack, then interceptor | 8,9 |
| Copter/finite-range air attack | 17 |
| Conventional missile, then Planet Buster | 13,14 |
| Final land former | -1 |

Human mode skips the AI-only strategic defender variants. The general role
producer preserves the source's separate chassis selectors: exact minimum
speed is preferred for positive speed requests; nonpositive requests score
speed+1; equal scores keep earlier chassis IDs. Naval/air requests are not
replaced with the fastest globally known chassis. Air0 selects the earlier
Needlejet on equal speed, while air-2 requires range1 and excludes special
chassis. Air-1 selects special missile chassis.

Weapon budget selection prefers greater doubled strength, later IDs on ties,
ignores noncombat modes for positive budgets, excludes strength>=99 and payload20.
Negative budgets select the first known matching mode. Armor selection uses
known nonnegative strengths within component-cost budget, later IDs on ties.
The original equal-cost defender loop, artillery chassis/armor loop, fast-land
armor gate and naval cost loop are implemented separately using the checked raw
cost arithmetic. Naval human output overrides its weapon with strongest known
budget99 after its armor/cost optimization; removing the optimization would
still change armor. Utility modes and fixed final Former15 remain distinct.

High Morale, Comm Jammer, Clean Reactor, Antigrav, Radar, AAA, Drop, Amphibious,
artillery and worker ability choices remain local to their role families.
The reference includes the original Neural Grafting gates. No blanket cleanup
or global ability priority is inserted between proposal generation and preview
matching. Some aliases accept extra nonsignificant proposed ability bits;
that is the separate597390 mask contract.

## Upgrade plans and two corrected source details

This supersedes the earlier missing-stronger simplification and the old plan1
threshold expression in the research supplement.

The upgrade detector scans admitted intrinsic/own records in order. For each
known candidate component, encountering a weaker record immediately remembers
that candidate. Encountering a later equal-or-stronger record stops this inner
scan **without undoing the remembered candidate**. It is not equivalent to
comparing the candidate with the maximum existing component. Thus a bank of
Laser records without Laser technology can generate Hand Weapons proposals:
an earlier intrinsic weaker record remembered Hand Weapons before the scan
reached a sufficient record. The full-bank controls retain all64 such prefix
submissions. The independent prefix now reproduces this order exactly.

Raw plan42 dispatch (`59ab94` table):

| Plan | Behavior |
| --- | --- |
| 0,4 | Weapon-upgrade branch |
| 1 | Weapon plus conditional armor; armor-only alternative |
| 2 | Armor-upgrade branch |
| 3,5 | Skip; no submission |
| 6 | Cost-gated weapon/armor upgrade |
| >6 | Skip |

Plan6 compares complete raw cost against
`faction[95d594] + faction[95d588] + max(5, reactor²)`. Weapon is considered
first; armor is then priced with the chosen weapon. It submits only when the
native component-change condition succeeds. Both affordable and rejected
controls pass without invoking actual replacement/allocation.

The original reactor scale local is **`2 << reactor`**, not `1 << reactor`.
For plan1's armor threshold the other factor is **2 for reactor1, 1 otherwise**.
Compute `q=factor*(2<<reactor)*4/weaponStrength` using integer division; use
`min(q,weaponStrength)` when both>=2, otherwise2. The prefix also retains this
factor in a local later read by the offensive armor-budget calculation; it
starts at the ten-row armor-loop count when no plan1 branch changes it. That
local is state carried between controller stages, not an independently reset
role input. Focused original local snapshots confirm reactor4 scale32 and
plan1 factor1. The producer carries it rather than deriving it from final tuple.

## Full-bank candidate existence

Preview requires some positive replacement score to publish first-new mode3,
but never consumes or builds the winning record. Matching still precedes most
replacement gates, except the excluded/unused early path.

| Active custom record state, after no alias | Candidate? |
| --- | --- |
| Excluded, both cached counts0 | Yes, before matching |
| Excluded, unit count0, production nonzero | Yes, later excluded/unit gate |
| Same category as proposal | Yes, including used/in-production records |
| Different category, either count nonzero | No, absent the preceding excluded case |
| Different category, both counts0 | Yes |

An inactive own record is also a candidate. Intrinsic records can alias but
cannot supply a replacement slot. Ordinary scoring's count penalty is clamped
at63, below the minimum eligible base score64; the preview reference can decide
existence without recreating mode-zero winner selection. Full-bank controls
cover live different-category failure, unused success, same-category success,
excluded-unused success, and mode1/no-alias/no-candidate retention. Cache values
are declared inputs, not a unit recount.

## Scope and remaining work

The implementation is a source-derived **stock-human conventional producer**,
not a lookup of observed outcomes. Inputs include current raw custom plans,
category/exclusion/flags, technology ownership, cached counts, refreshed Faction
fields and installed faction personality. The checks exercise owner1; other
owners use the same owner-indexed arithmetic and owner parity but do not gain a
new multi-owner matrix from this supplement. Extreme integer overflow or
pathological strategic values are not admitted.

The normal human branch computes threat local2c as0. `threat=3` is a declared
internal-stage extension test, not a claim that ordinary human opening derives
AI threat. AI diplomacy/threat policy and mode-zero allocation remain outside
scope. The supplied personality parameter and observed late Clean Reactor
fields are supported; arbitrary strategic-field combinations have source-derived
branches but only the stated distinctions have native controls.

Negative/PSI **custom records** explicitly raise in the producer: their597390
strategic evaluation/scoring needs a separate extension. Known Psi technologies
and stock intrinsic native units remain valid inputs to the existing capability
refresh under its established scope; they do not make a Psi custom bank admitted.
Modified component tables, alternate rules, project overrides, custom faction
policies and category-filtered AutoDesign menu callers remain unsupported by
the high-level composition. Do not silently substitute Scout for those cases.

No production files, model matrix or desktop run was added. Complete Workshop
parent/console screen capture remains outstanding as recorded in the
[parent-layout supplement](design-workshop-preview-parent.md).

## Semantic input map correction

Use the [input-authority map](design-workshop-input-authority.md) when adapting
this byte-addressed reference to typed engine state. The reference parameter
`personality` actually carries the installed MORALE bonus; the five3dc..3ec
fields are current AI priorities, and+004 is rank position. The frozen numeric
arithmetic is unchanged. These inputs must not be populated from social axes
or an assumed Base count.
