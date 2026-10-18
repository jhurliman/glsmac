# Ordinary retained ranking score

Research ID: RE-SMAC-ORDINARY-RANKING-SCORE-001. Original SMAC only.
This supplies the score construction preceding the
[retained ranking update](ending-input-producers.md). [Cases](ordinary-ranking-score-cases.json)
record one admitted first-command composition and nine focused native score-only
calls. Scope: scenario parameters 9999/9999/0, no Secret Projects, ordinary
nonnegative inputs and the existing basic designs. No seven-faction world or
victory/AI/report matrix was executed.

## Formula and saved inputs

For an active ordinary faction:

```text
score = 4 * (cached_population + future_technology_counter)
      + sum(rating_Growth + rating_Tech + rating_Wealth + rating_Power
            for each valid owned ordinary technology)
      + sum(design_contribution for each counted design)
```

Inactive factions receive score zero. The later rank assignment and inverse-table
filter are as documented in the ranking-update contract.

| Input | Authority used by the original |
| --- | --- |
| Population | Saved faction population aggregate from the owner summary pass; not a fresh sum of Base sizes. |
| Future technology counter | The separate retained repeat counter; not the historical technology score. Keep zero when unsupported by the port. |
| Technology contribution | Valid ownership plus the technology definition's four rating values, summed without faction personality weights. |
| Design quantity | The faction's cached active count for that exact catalog design. Factory/removal paths maintain these counts. |
| Design cost | Cached design cost before the Base mineral factor, INDUSTRY adjustment or prototype surcharge. |
| Weapon strength | Signed attack power from the design's weapon definition. |

There is **no direct Base-count term**, treasury term or historical acquired-tech
score term in this ordinary no-project formula. Centauri Ecology contributes
0+1+2+3 = **6**; Biogenetics contributes 0+3+2+2 = **7**. Ownership tests reject
disabled/invalid records and Future Technology as an ordinary owned row; holding
the prerequisite technologies is not independently required by this validity
test. The future repeat counter contributes through the separate first term.

## Exact unit arithmetic

First obtain a global reference weapon power **M**, initially 1. Scan custom
design slots for designs marked both active and prototyped, accepting positive
weapon power below 99. M is the maximum such power, or 1 if none exceeds it.
This scan is global: it does not filter by the scoring faction, active faction
membership, technology ownership or whether any units of the design exist.
Predefined designs do not raise M.

For each catalog design, take count N and cached cost K, then compute **C=N*K**.
Predefined designs must pass their technology prerequisite's availability test;
custom designs bypass that particular prerequisite check. Zero counts naturally
contribute zero. The native byte-count guard skips values above 250; it does not
clamp them. That guard is static, not a newly executed overflow case.

| Signed weapon power A | Contribution |
| --- | --- |
| A = 0 | floor(C / 4) |
| 1 ≤ A < 99 | truncate(A / M) * C |
| A < 0 | truncate(C / 2), static special/Psi branch |
| A ≥ 99 | C, static special-weapon branch |

The zero-power division follows multiplication of the **whole design count**
by its cached cost. It is not a per-unit rounded contribution. The positive
power division occurs **before** multiplication by C; do not replace it with
`floor(A*C/M)`. For ordinary positive inputs, truncation means floor.

For the installed Infantry/Fission basic designs, with M=1:

```text
units = Scout_count
      + floor(3 * Colony_Pod_count / 4)
      + floor(2 * Formers_count / 4)   # requires valid Ecology availability
```

These use cached costs 1/3/2 and weapon powers 1/0/0. Armor, chassis, reactor,
abilities and prototype costs can affect the stored design cost through their
own producers; this scorer does not recalculate those costs. Unit damage,
morale, movement spent, location and home Base do not enter this count-based
calculation directly.

## Native cases and timing

The admitted Gaians startup has one native Scout and one Base. With full ordinary
technology ratings loaded, the first global ranking update computes **7**:
population cache 0 + Ecology 6 + Scout 1. This precedes the turn increment and
first owner upkeep. At the turn-1 first-command boundary, the owner summary has
produced population cache 1; a score-only query then gives **11**. The same live
Base population is 1 at both points. Preserve this retained-cache timing.

Nine native queries on restored first-command state establish:

| Controlled change from score 11 | Result |
| --- | --- |
| No change | 11 |
| Cached population 3, live Base population still 1 | 19 |
| Cached Base count 9 | 11 |
| Historical acquired-tech score 999 | 11 |
| Future repeat counter 2 | 19 |
| Add valid Biogenetics ownership | 18 |
| Counts: two Colony Pods, two Formers, one Scout | 13 |
| Five Scouts; another faction's power-4 custom design active but unprototyped | 15 |
| Same input, custom design also prototyped | 10 |

The last case has M=4: five Scouts contribute **0**, not 1. The support-unit
case contributes **1+1+1=3**, establishing aggregate-before-division rounding.
Quantity/ownership/cache variations are explicit resolved inputs, not claims
that those extra units or technologies were acquired through gameplay.
All score-only calls return normally, preserve their declared source quantities
and ending references, and make no RNG call.

## Boundaries for the port

The prior first-command ranking receipt left technology rating fields unset;
its observed score 1 remains valid for that admitted input. This extension
executes the native rating-store fragment with installed parsed values before
the controller, producing score 7 instead. Prior receipts are unchanged.

The native routine also has Secret Project ownership bonuses, other scenario
score modes, special/Psi weapons, custom-design lifecycle and later policy work.
Those are outside the implemented basic-design provider contract. No-project
input must remain explicit; do not silently apply this reduced formula to a
world with projects or another scoring mode. The project and special branches
are recorded privately and are not covered by new end-to-end gameplay cases.
