# RE-SMAC-HEADQUARTERS-CAPTURE-001: ordinary Headquarters capture

An original movement/capture request **successfully captures a population-3
Headquarters Base and reduces it to population 2**. It reaches the captured-Base
screen without answering or constructing that screen. HQ presence does not
refuse this ordinary capture. [Cases](headquarters-capture-cases.json) contain
the literal state, ordered events, facility removals and all random outputs.

## Controlled caller and observed result

This reuses the [undefended surviving-defender capture](undefended-base-entry.md):
local two-human Librarian, turn 6, mutual Vendetta, equal retained Ecology
technology, no projects, adjacent healthy Scout and no target defender. The
three Base identities stay 0 at (16,16), target 1 at (20,16), and remaining
defender 2 at (26,16). Scout 1 moves from (19,15) into target 1 through the
original movement caller. No combat, capture result, random value or gameplay
choice is supplied.

The new explicit inputs are target population 3, matching retained defender
population aggregate 4, and a unique defender HQ at target 1. Original facility
setters remove HQ from defender Base 2 and install it in target 1 before entry.
Captor Base 0 retains its own HQ. Target facilities are HQ, Recycling Tanks,
Perimeter Defense, Energy Bank and Network Node. Credits start at 100/200.

| State | Before capture | At captured-Base screen entry |
| --- | --- | --- |
| Target ID / coordinates | 1 / (20,16) | Unchanged |
| Target owner / former owner | 2 / 2 | 1 / 2 |
| Population / assimilation timer | 3 / 0 | 2 / 50 |
| Target facilities | HQ, Tanks, Perimeter, Bank, Node | Perimeter, Bank, Node |
| Captor / defender Base count | 1 / 2 | 2 / 1 |
| Credits, captor / defender | 100 / 200 | 220 / 80 |
| Stored nutrients | 5 | 5 |
| Stored minerals / baseline minerals | 7 / 7 | 0 / 0 |
| Production / baseline item | Former / Former | Scout / Stockpile Energy |
| Additional queue | Scout, Former | Empty |
| Defender Former 2 home | Target Base 1 | Remaining Base 2 |
| Defender Former owner / position | 2 / (30,16) | Unchanged |

Captor Base 0 and surviving defender Base 2 receive **no native record writes**
in this capture prefix. In particular Base 2 remains without HQ, retains Scout
production and its prior queue, and receives no instant replacement HQ or HQ
queue entry. No Unit is created, deleted or renumbered. The attacker retains
home 0, spends its three movement points, remains selected and finishes inside
target 1. Active/human membership remain 6; focus, controller and active faction
remain 1. Ending state remains 0 and no restart is entered.

## HQ removal, relocation admission and RNG

The HQ checks and relocation admission occur **before loot transfer, population
loss and the facility rolls**. Booty is calculated first using the original
population and retained population aggregate:

```
200 * 3 / (4 + 1) = 120
```

The relocation comparison then reads the defender's current **pre-loot credits**,
before any relocation payment: here 200, below the signed threshold 1,000.
It does not compare against the later 80-credit balance. If relocation pays
1,000 in the static alternative, the previously computed booty is not recomputed
from the reduced treasury before its later transfer.

Static admission requires HQ actually present at that early check, at least
1,000 defender credits, and a qualifying remaining defender Base selected by
the native scan. The scan excludes the captured Base, accepts only losing-owner
Bases, visits IDs ascending and keeps a strict minimum score below initial 9999.
Its final selected ID must be **greater than 0**; a selected ID 0 is rejected,
without retrying the next-best positive ID. These are additional predicates;
credits and HQ presence alone do not establish that relocation will execute.

For completeness, the static candidate score uses integer truncation:

```
256 * (1 + sum(distance to other remaining defender Bases,
               doubled when their region differs))
    / (retained defender Base count in candidate region + 2*candidate population)
```

Use the original wrapped nearest-Base integer distance metric. Exclude the
candidate and captured Base from that inner sum. Ties retain the earlier ID.
This selector is static evidence here, not a paid relocation runtime result.

If a candidate passes, local mode asks for confirmation only when the **losing
faction equals current focus**. A zero response skips relocation. Network mode
or a different local focus bypasses that confirmation. That check uses focus,
not a human-membership test. If admitted, the static body immediately charges
1,000, installs HQ at the selected Base and clears it at the captured Base;
it does not queue an HQ build. The later nonhuman production-choice call is
outside this human fixture. The HQ admission block adds no independent
recapture/capture-mode/human-membership requirement.

HQ membership is sampled and retained for the early branch; it is re-read
after the relocation branch, still before the rolls. It is not inferred from
the facilities left after random destruction. The original uses scratch
locals that are later reused; this is not a permanent immutable HQ-history flag.

In the executed 200-credit case relocation is skipped. The facility loop draws
once for each ID 1..38 except Pressure Dome 26, even for absent facilities.
There are exactly **37 CRT draws** and no shared RNG draws. Seed 3 becomes
1951609278; shared state remains 3. The first draw, for HQ, is raw **48**
(`3 -> 3173050`), remainder 0 modulo 3, so it removes HQ. After the entire
loop the mandatory HQ-clear call executes again, idempotently in this run.
The unconditional clear is a static rule even when HQ's random draw would
not remove it; no extra seed control is needed to infer that instruction.
The ordinary non-recapture cleanup also explicitly clears Commons and Tanks.

## Population and assimilation inputs for the transaction

For a surviving ordinary size greater than 1, the inspected population decrement
requires all of the following:

- The caller's capture-mode argument is zero, as in this original movement call.
- Stored former owner differs from the recipient: this is not a recapture.
- Base status mask `0x40000` is clear.
- The objective-protection query is false.

That objective query includes all-Base objectives, a marked objective Base,
project-location objectives, and the configured objective-facility case; see
the existing [growth protection contract](growth-and-starvation.md). These
alternatives and the nonzero capture-mode branch are static limits here.
HQ presence itself does not suppress the decrement.

The population byte is decremented **before** owner assignment, governor donor
selection, facility loss, territory/home repair, production selection and final
resource calculation. Pass the resulting candidate population **2** into
worker, support and resource queries in this fixture. Do not retain the old
population 3 or hardcode population 1 from the separate earlier pop-2 case.
This case establishes 3→2; the prior receipt separately establishes 2→1.
Neither is extended here into a population-1 razing claim.

For a surviving transferred Base, assimilation compares its retained former
owner with the recipient. If equal, timer becomes 0. Otherwise former owner
becomes the losing owner and timer becomes **50**. This is independent of HQ
and the decrement amount; the ordinary observed case takes the latter branch.

## Donor, home and resource ordering

The captured Base is excluded from the governor donor scan. Actual donor Base 0
supplies its governor word after ownership and population change, before random
facility loss. See the [donor rule](capture-governor-donor.md) for population/HQ
weighting and tie handling.

After facility cleanup, territory rebuild precedes the old-home repair. That
repair temporarily excludes the captured Base, asks for the nearest defender
Base **from captured coordinate (20,16)**, restores ownership and assigns the
remote Former to Base 2. It does not move or transfer the Former. Home repair
finishes before the shared production selector chooses Scout for captured
Base 1. The surviving defender Base is not reselected or queued by that call.

Actual captured-Base resource calculation then returns these outputs:

| Output | Value |
| --- | --- |
| Nutrient / mineral / energy intake | 2 / 1 / 1 |
| Nutrient demand / surplus | 4 / −2 |
| Mineral demand / surplus | 0 / 1 |
| Energy waste / net energy | 0 / 1 |
| Economy / psych / labs | 2 / 0 / 0 |

These final values are decoded from writes inside the native resource call;
they are not a before/after HQ-only yield comparison. No correctly sampled
pre-capture resource comparison is asserted. Bank rounding and all ordinary
resource rules still execute. The surviving defender Base is not recalculated
in this prefix. Its missing HQ will affect a later real calculation through
the existing [HQ-distance waste rule](energy-waste-and-headquarters-distance.md),
including the no-HQ fallback distance, rather than an immediate synthetic
resource rebuild here. No upkeep, growth/starvation application, production
completion or per-turn energy posting is included.

The only answered presentation is the pre-existing display-only capture notice,
whose result is unused. Native history and technology comparison execute.
The next stop is the captured-Base screen with no answer. Full screen
construction, capture/movement return, paid relocation, later defender policy,
AI/network alternatives and other facilities remain outside the fixture.
