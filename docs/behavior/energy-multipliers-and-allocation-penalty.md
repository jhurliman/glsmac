# RE-SMAC-ENERGY-TAIL-001: multipliers, allocation penalty and Stockpile

Publication scope: this repository includes this handoff and its literal cases. Other research notes named as unpublished follow-ups below are outside this published evidence boundary and do not expand its claimed scope.

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 32 named and 1,600 deterministic sampled combinations pass.
These execute the complete post-specialist energy tail through the final
economy/psych/labs values, stopping before the citizen calculation. Category
inputs, project locations, commerce contributions and commerce-blocked states
are supplied. This is not the later crediting of income or technology progress.

## Facility bonuses

Begin with categories after allocation and specialist output. Add the resolved
flat faction labs adjustment, and add 2 labs if Biology Lab is present. Then
calculate additive multipliers in quarters:

| Category | Initial quarters | Each applicable bonus |
| --- | --- | --- |
| Economy | 4 | +2 each Energy Bank, Tree Farm, Hybrid Forest, Fusion Lab, Quantum Lab |
| Labs | 4 | +2 each Network Node, Research Hospital, Nanohospital, Fusion Lab, Quantum Lab |
| Psych | 4 | +2 once for Theatre OR Node with owned Virtual World; +1 each Hospital/Nanohospital; +2 each Tree Farm/Hybrid Forest |

For each category use `floor((input*quarters+3)/4)`. With nonnegative inputs,
this rounds the combined bonus upward. Do not round each facility separately.
Biology Lab's two points and the flat faction adjustment receive the labs
multiplier. Specialist category outputs also receive these bonuses.

Virtual World psych equivalence depends on faction ownership and a local
Node, and shares its bonus with Theatre. Its project can be elsewhere in the
faction. The [psych handoff](psych-allocation-and-specialist-output.md) covers
that stage and subsequent citizen consumption separately.

## Projects after facility multiplication

The following effects require the project to be located in the current base:

- Supercollider doubles labs, then Theory of Everything doubles labs again.
  Both give four times the already multiplied labs, rather than one additive
  three-times multiplier.
- Space Elevator doubles economy. This does not double psych or labs here.
- With Free Market selected as the economics model, Longevity Vaccine adds
  `floor(economy/2)` to economy after the Space Elevator step. Another model
  gives no economy bonus here. This is a model check, not an ECONOMY rating.

Projects elsewhere do not grant those income/labs effects to this base.
Longevity's separate citizen effect is faction-wide, as documented in the
[citizen final stage](citizen-final-projects-and-suppression.md).

## Network Backbone additions

When Network Backbone is located in the current base, add one lab for every
completed Network Node across all live bases, including the current base and
bases owned by other factions. There is no owner filter in this scan.

Then add the resolved incoming commerce contributions whose commerce gates
permit them: the current faction must not be commerce-blocked, and each partner
must not be commerce-blocked. A blocked current faction suppresses all commerce
additions, but not the Network Node addition. These gate states are supplied;
their lifecycle and the earlier commerce formula are not defined here.

The original scan covers the seven playable faction entries. The fixture uses
faction 1: seven incoming values correspond to factions 1..7, while the six
partner blocked flags correspond to factions 2..7. The faction-1 entry shares
the current faction's gate. Its ordinarily expected self-commerce value is
not established by these supplied-input tests.

Backbone additions occur after Supercollider/Theory multiplication. They do
not receive that multiplication, but they do receive the allocation penalty
below. Network Backbone elsewhere does not provide this local bonus.

## Unequal-allocation penalty

Use current psych tenths P, labs tenths L, and economy tenths C=10-P-L.
Checked ordinary rates have P,L,C nonnegative. Let:

`A = floor(abs(L-C)/2)`

`K = 2 * (4-clamp(EFFICIENCY,-4,4)) * A`

If L>C, reduce labs by min(K,40) percent and economy by min(2K,80) percent.
Otherwise reduce labs by min(2K,80) percent and economy by min(K,40) percent.
For each reduction R, store `floor((value*(100-R)+50)/100)`.
Psych is unchanged by this penalty.

At EFFICIENCY +4 or above, there is no penalty. A one-tenth difference gives
A=0 and also no penalty. At EFFICIENCY 0 with all nonpsych allocation going
to labs, labs retains 60% and economy retains 20%, each rounded as above.
This is a later allocation penalty, separate from earlier energy waste.
Apply it after facility/project bonuses and Backbone additions.

## Stockpile Energy last

If both the current production item and its switching-baseline item
are Stockpile Energy, and mineral surplus M is positive, add
`floor((M+1)/2)` to economy. A newly selected Stockpile item without the
matching baseline item does not trigger this branch. Neither does the
baseline item alone, or nonpositive surplus. The legacy fixture label
`previous_stockpile` refers to that baseline-item input.

This addition occurs after all multipliers and the allocation penalty. It
therefore receives neither those bonuses nor that penalty. The condition is
the two resolved item selections. The production-posting follow-up (unpublished follow-up `production-posting-and-unit-creation.md`)
now connects the admitted outer baseline writer to this actual gate and verifies
that Stockpile skips mineral storage posting. Complete income/turn sequencing
remains open; those compositions do not establish first-turn treasury credit.

The [queue/credit follow-up](production-queue-and-build-eligibility.md) additionally
executes the actual treasury-addition fragment after the baseline/energy gate.
Its intervening upkeep stages remain excluded, so complete first-turn income
is still not established.

## Acceptance scope

The [32 named cases](energy-multipliers-and-allocation-penalty-cases.json)
cover each facility, project locality/order, global Nodes, commerce gates,
EFFICIENCY clamping, allocation imbalance and Stockpile conditions. The
sampled grid combines these with nonnegative category inputs and legal rates.
Each compares multiplier quarters, values before/after the allocation penalty
and final values after Stockpile.

Original facility-bit helpers and the world-base scan execute without patched
instructions or substituted returns. Upstream energy/waste/commerce, the
builder of the flat faction adjustment, project/status transitions, production
selection timing, income crediting, technology discovery and full turns remain
open. The [allocation/specialist](psych-allocation-and-specialist-output.md)
and [replacement](specialist-replacement-and-overflow.md) handoffs describe
earlier stages; this test family supplies their resulting category totals.
