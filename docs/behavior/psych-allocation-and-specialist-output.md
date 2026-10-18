# RE-SMAC-PSYCH-ALLOC-001: allocation, specialist output and psych bonuses

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 29 named and 1,400 deterministic sampled cases pass. Each executes
the original allocation/specialist-output/psych-multiplier stages, then the
original citizen consumer. Net energy and selected specialists are resolved
inputs; commerce, waste and specialist availability/assignment remain open.

## Initial energy allocation

Let E be net allocatable energy bounded to 0..9999, P the psych allocation in
tenths and L the labs allocation in tenths. For ordinary legal allocations,
P and L are nonnegative and P+L is at most 10. Compute in this order:

- `psych = clamp(trunc((E*P + 4)/10), 0, E)`
- `economy = clamp(trunc((E*(10-P-L) + 4)/10), 0, E-psych)`
- `labs = E-psych-economy`

Here trunc means integer truncation toward zero; all numerators are
nonnegative in the checked legal-rate scope. The +4 bias rounds exact halves
down. Labs receives the residual, rather than an independently rounded share.
For E=5 and P=1, psych is zero; E=6 gives one. For E=1, P=0 and L=5,
economy is zero and labs receives one. Psych's clamp has priority when energy
is too small to distribute cleanly.

The 9999 cap is applied to this allocation input. Specialist output is added
afterward and is not part of that cap. This handoff does not reconstruct the
earlier gross-energy, convoy, commerce or inefficiency/waste calculation.

## Resolved specialist output

Each selected specialist adds its table outputs to the allocated categories
before facility multipliers. Installed SMAC values are:

| Specialist | Economy | Psych | Labs |
| --- | --- | --- | --- |
| Technician | 3 | 0 | 0 |
| Doctor | 0 | 2 | 0 |
| Librarian | 0 | 0 | 3 |
| Engineer | 3 | 0 | 2 |
| Empath | 2 | 2 | 0 |
| Thinker | 0 | 1 | 3 |
| Transcend | 2 | 2 | 4 |

This confirms that the excluded-population count used by the citizen
normalization is the specialist count consumed by this energy loop. Specialists
are excluded from ordinary citizen capacity while still supplying psych to
the remaining calculation. Tests cover zero through sixteen selected slots.
They do not establish selection or storage behavior above sixteen.

The original loop can replace obsolete specialist types. These checks supply
the listed outputs with all selected types available and nonobsolete, by
neutralizing their prerequisite/obsolescence entries in supplied rule data.
This is an output-arithmetic test, not a stock technology progression or a
rule that obsolete specialists remain unchanged. Availability, automatic
replacement and worker reassignment require their own trace.

## Psych facility multiplier

Start a psych multiplier at four quarters and add:

| Condition | Additional quarters |
| --- | --- |
| Hologram Theatre OR Network Node plus faction-owned Virtual World | 2 once |
| Research Hospital | 1 |
| Nanohospital | 1 |
| Tree Farm | 2 |
| Hybrid Forest | 2 |

Then `final_psych = floor((psych_after_specialists * quarters + 3)/4)`.
For nonnegative inputs this rounds the multiplied result upward. Bonuses are
additive and combined before rounding. With every listed facility, the
multiplier is twelve quarters, or three times psych.

The Theatre and Virtual World/Node routes share one bonus. A Node without an
owned Virtual World supplies no psych multiplier; Virtual World without a
Node supplies none; a rival-owned Virtual World supplies none. The owned
project can be in another base. Hospital and Nanohospital bonuses stack with
each other and the other facilities. Completed local facility presence is
used by these checks.

A Doctor's two psych becomes three with Theatre. A Thinker's one psych becomes
two with Research Hospital after upward rounding. These modified points
feed the [psych citizen stage](citizen-conversion-stages.md), which takes
integer points/2; rounding the multiplier and converting to talents are
separate operations. The same facilities can also have separate
[drone/talent effects](citizen-facility-project-inputs.md).

## Acceptance scope

The [29 named cases](psych-allocation-and-specialist-output-cases.json) cover
allocation ties/residuals, negative/capped input energy, every specialist,
facility stacking, project ownership, fractional bonuses and sixteen slots.
Sampled cases combine legal rates, populations 1/3/7/20/127, selected slots,
facility subsets and ownership states. They also compare final citizen
counts after consuming the generated psych and specialist count, with
POLICE -2 and other citizen inputs neutral except ordinary difficulty and
the tested facilities' own drone effects.

Original technology and facility-bit helpers execute without patched
instructions or substituted returns. Supplied specialist availability data is
explicitly different from stock technology progression. Economy/labs values
in these cases are snapshots before their later multipliers; do not use them
as final reserve income or research. Full energy processing, specialist
availability/replacement/assignment, turn timing and full-game observations
remain outside these allocation/output checks. The processing loop's
obsolescence and slots beyond sixteen are now separately checked in
[specialist replacement and overflow](specialist-replacement-and-overflow.md).
The subsequent [energy tail](energy-multipliers-and-allocation-penalty.md)
separately specifies final category multipliers, project order, unequal
allocation penalty and Stockpile conversion. The preceding
[energy-waste handoff](energy-waste-and-headquarters-distance.md) now checks
HQ geometry, current-efficiency waste and its pre/post-commerce caller order;
the commerce amount itself remains a supplied input there.

The [fresh-campaign economic producers](startup-economic-state.md) now specify
actual startup sliders, reserves, debit, diplomacy/commerce and flat labs.
[Research submission scaling](research-submission-scaling.md) specifies the
per-base social multiplier, faction fractional carry and early suppression.

The [facility identity controls](research-submission-scaling.md#facility-identity-clarification)
explicitly distinguish Skunkworks10 from Hologram Theatre11. They confirm that
the Theatre name here refers to facility11; an adjacent facility is not a
replacement for this psych multiplier.
