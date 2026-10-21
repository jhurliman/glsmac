# RE-SMAC-UNIT-COST-001: unit design and prototype costs

Status: original-x86 checked for the SMAC fingerprint in the
[research ledger](README.md). All 9,005 scenarios pass, including 41 named
fixtures. Actual component arithmetic, an admitted design-cost writer and the
complete unit-cost/prototype routines execute without substituted calls. Four
named cases connect to the actual mineral factor and unit-production cost
comparison; one connects an admitted prototype-completion mutation to its next
cost query. This is synthetic routine evidence, not a full-game observation.

Use the active chassis, weapon/package, armor and ability rules. **Cost factors
are distinct from combat strengths and movement allowance.** The checked reactor
ratings are 1 through 4. Construction legality, component availability and
ability compatibility must be checked separately: many combinations below are
deliberately illegal designs used to isolate arithmetic.

The [predefined basic-design producer](predefined-basic-designs.md) extends the
admitted cost writer to complete installed-unit loading/full builders. Its
Colony/Formers/Scout costs3/2/1 and exemption from prototype surcharges are
checked through actual consumers, even with their prototyped state false.

## Ability surcharge accumulator

The [predefined ability extension](predefined-abilities-and-transport-capacity.md)
now identifies the full24-row bit mapping and the separate rule land-cost flag,
with actual reader/builder/cost tests. It also specifies the complete capacity
helper and all fourteen admitted predefined automatic-plan instances.

Let `W0`, `A0` and `C0` be the weapon/package, armor and chassis **cost factors**
before domain adjustments. Start accumulator `P=0`. Visit selected abilities in
their order in the active ability table, not the order the player selected them.
For each selected ability, first add one to `P` if `P` is already nonzero, then
add its cost contribution:

| Ability cost rule | Contribution |
| --- | --- |
| Nonnegative integer | That integer |
| -1 | clamp(floor(W0 / A0), 0, 2) |
| -2 | W0 - 1 |
| -3 | A0 - 1 |
| -4 | C0 - 1 |
| -5 | W0 + A0 - 2 |
| -6 | W0 + C0 - 2 |
| -7 | A0 + C0 - 2 |

After that loop, on land only, add one per selected ability whose rules flag
says its cost is increased for land units. These additions happen after the
ordered stacking loop and do not retroactively trigger its extra points.

Zero-contribution abilities therefore have an order-dependent interaction.
With Hand Weapons/Synthmetal/Fission Infantry, Comm Jammer followed by Clean
Reactor produces component cost 3, while Clean Reactor followed by Hypnotic
Trance produces 4. Both ratio-based abilities contribute zero in that design;
the latter is visited after the accumulator is already nonzero. Deep Radar's
listed cost is zero but its land-cost flag can still increase the result.

## Component-cost calculation

Use the following operation order. All divisions in this section have
nonnegative operands and floor unless explicitly described otherwise.

1. Begin with `W=W0`, `A=A0`, `C=C0`, and reactor rating `R`.
2. For sea chassis, replace `A` with `floor(A/2)` and add `R` to `C`.
   For air chassis, add `2*R` to `C`; if `A>1`, multiply `A` by `2*R`.
   Land leaves these three values unchanged.
3. If the chassis has the missile property and the weapon's offense is at least
   99, return `W0` immediately. This bypasses subsequent component/ability cost
   adjustments. The installed Planet Buster on Missile chassis follows this
   branch; Conventional Payload does not.
4. Replace `W` with `max(W, floor(A/2)+1)`.
5. Set `D=2^(R+1)`. Set `Q=floor(((C+A)*W + D/2)/D)`, rounding this stage to
   nearest with halves upward. This denominator is exponential in the reactor
   rating; do not replace it with a linear reactor divisor.
6. If adjusted `C==1`, replace `Q` with `floor(Q/2)+1`.
7. If original `W0>1` and original `A0>1`, add one to `Q`. On land, add one
   more when both original conditions hold and adjusted `C>1`.
8. For sea, replace `Q` with `floor((Q+1)/2)` unless the weapon/package mode is
   InfoWar (Probe Team). For air, divide `Q` by four for weapon modes 0 through
   2, or by two for the checked noncombat modes above 2.
9. Raise `Q` to at least `2*R-floor(R/2)`: the floors for reactor ratings 1/2/3/4
   are respectively 2/3/5/6. Then, if adjusted `W=A=C=R=1`, set `Q=1` instead.
10. Return `floor(((P+4)*Q+2)/4)`, applying quarter-cost increments and rounding
    the resulting cost to the nearest integer, halves upward.

The admitted design writer stores the low eight bits of this result as the
design's ordinary cost. A synthetic all-24-abilities Hovertank/Singularity
Laser/Stasis/Fission case returns 1,541 and stores 5. This verifies truncation
in that writer, not legality of that design or a claim that ordinary legal
designs reach this boundary. A wider native port field alone would not reproduce
that modified-state behavior.

## Prototype surcharge and exemptions

Begin the unit-cost query with the design's stored ordinary cost. For a Colony
role queried with an actual base context, raise a stored zero to one. The same
zero with no base context remains zero. This guard is independently checked
using a synthetic overridden stored cost.

Predefined designs and custom designs already marked prototyped have no
prototype surcharge. Otherwise:

1. Resolve the custom design's owning faction. If it has the free-prototype
   capability or its own stored difficulty is 0 or 1, use extra percentage zero.
   This is separate from the global difficulty used by the AI mineral factor.
2. Otherwise select the active prototype extra percentage for land, sea or air.
   Installed SMAC values are 50% for all three; custom unequal values are tested.
3. Recalculate the component formula **with every ability omitted**. Use that
   full recalculated result, not the stored ordinary cost, as the prototype basis.
4. Surcharge is `floor((basis * extraPercentage + 50)/100)`, rounding to nearest
   with halves upward for the nonnegative tested percentages.
5. If an actual queried base has Skunkworks, replace the surcharge with zero.
   A no-base query cannot use that facility, even if the currently selected base
   has it.
6. Return ordinary stored cost plus this surcharge. The returned “prototype cost
   present” result means **nonzero surcharge**, not merely unprototyped status.

Thus a Laser/Fission Infantry has ordinary factor 2 and surcharge 1, total 3.
Adding Clean Reactor raises ordinary cost to 3 but leaves surcharge at 1, total
4. Computing 50% of its ability-inflated stored cost would give the wrong answer.
A tiny configured percentage can round surcharge to zero without prototyping
the design. Faction exemptions follow the design's owner; Skunkworks follows the
queried base. Synthetic mismatched owner/base cases isolate those sources.

The admitted post-construction prototype mutation is also executed in one case.
It marks the custom design prototyped, preserves unrelated design flags, and
changes the next Clean Reactor/Laser/Fission Infantry cost from 4 to 3. Unit
creation and the complete production completion path are outside that case.
Broader prototype state propagation and free-prototype capability construction
remain separate work; these tests supply their resolved state.

## Total production cost and examples

Multiply the returned unit-cost factor by the already rounded
[human/AI mineral factor](mineral-cost-factors.md). The original unit-production
consumer executes both routines in the connected cases, then checks stored
minerals against the product. Cost-minus-one fails that comparison; equality
passes it. Other production admission/completion conditions remain separate.

The following are custom, unprototyped designs with Fission, default rules,
owner difficulty at least 2, no exemptions and no abilities unless shown:

| Design | Ordinary factor | Prototype surcharge | Total unit factor |
| --- | --- | --- | --- |
| Infantry / Hand Weapons / No Armor | 1 | 1 | 2 |
| Infantry / Laser / No Armor | 2 | 1 | 3 |
| Same with Clean Reactor | 3 | 1 | 4 |
| Speeder / Particle Impactor / Plasma Steel | 7 | 4 | 11 |
| Foil / Particle Impactor / Plasma Steel | 4 | 2 | 6 |
| Needlejet / Particle Impactor / Plasma Steel | 4 | 2 | 6 |
| Missile / Planet Buster / No Armor | 32 | 16 | 48 |
| Missile / Conventional Payload / No Armor | 11 | 6 | 17 |

At human Standard-world INDUSTRY 0, the Laser Infantry total production cost is
30 minerals; with Skunkworks or already-prototyped state it is 20. At INDUSTRY
+1, Clean Reactor Laser Infantry costs 36: unit factor 4 times mineral factor 9.
These do not assert that identical predefined designs incur prototype charges.

The separately checked [unit build-eligibility consumer](production-queue-and-build-eligibility.md#unit-eligibility-helper)
now specifies active/exclusion, predefined technology, custom owner, Colony and
sea-adjacency checks. Full workshop/component legality and lifecycle remain broader.

## Fixtures and evidence limits

The [41 named fixtures](unit-design-and-prototype-costs-cases.json) include all
the examples, domain exceptions, unequal prototype percentages, all seven
negative ability-cost forms, stacking order, exemptions, ownership, colony-zero,
cached overflow, four actual production comparisons and one admitted prototype
mutation. Additional grids cover:

- 7,560 combinations of all nine chassis, 21 weapons/packages, ten armors and
  four reactors, with the prototyped flag set to isolate component arithmetic.
- 288 single-ability combinations across land/sea/air and all reactors.
- 828 ability pairs across the three domains, including incompatible pairs.
- 288 prototype difficulty/capability/Skunkworks/status/base-context combinations.

Tables are initialized from pinned alpha.txt fields, not through the full parser.
The original design writer fragment stops before role/naming/legality logic;
the appropriate Colony/non-Colony role and prototype status are resolved inputs
to the cost consumer. No movement, interface or cost helper is substituted.
Complete design admission/limits, prototype acquisition/transfer/history,
free-prototype token decoding, combat behavior, upgrade/disband pricing, unit
creation, production iteration/carryover, save/load and live-game observations
remain open. Do not infer a legal design or a successful build from a cost alone.
