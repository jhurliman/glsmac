# RE-SMAC-UNDEFENDED-BASE-ENTRY-001: move through captured-Base UI entry

**One original movement request transfers an undefended population-2 Base,
acknowledges the display-only capture notice, and reaches the captured Base's
interactive screen.** No Base-screen action or normal movement return is
supplied. The [literal case](undefended-base-entry-cases.json) records both the
notice and the subsequent Base-screen entry. Do not use it as a completed-command
fixture.

This is pinned SMAC execution on controlled resolved inputs: local two-human
Librarian, mutual Vendetta, turn6, a healthy full-movement Scout, adjacent clear
land, no projects, no recapture, no exotic facilities, and a second defender
Base. Both factions use the Gaia definition and own only Centauri Ecology.
The second faction's coherent runtime/definition and historical state are
supplied inputs; this is not a replay of full multiplayer world creation.

The original move wrapper, movement body and capture body execute. There is no
combat round, supplied movement result, supplied capture outcome, supplied
random result or gameplay choice. The notice is explicitly acknowledged as a
headless display service; its caller ignores the return value. A display-refresh
service is omitted at
the presentation boundary; other inherited presentation/runtime services are
listed in the private receipt.

| State | Before request | At notice and subsequent Base-screen entry |
| --- | --- | --- |
| Actor position | (19,15) | (20,16), inside captured Base |
| Actor movement spent / allowance | 0 /3 | 3 /3 |
| Actor damage / stored morale | 0 /1 | 0 /1 |
| Actor home | Attacker's original Base0 | Base0 |
| Selected / secondary unit | Actor1 /none | Actor1 /none |
| Captured Base identity | Base1 at (20,16) | Same identity and coordinates |
| Current / former owner | Defender /defender | Attacker /defender |
| Population / assimilation timer | 2 /0 | 1 /50 |
| Stored nutrients | 5 | 5 |
| Minerals / mineral baseline | 7 /7 | 0 /0 |
| Current production | Former | Scout |
| Previous production baseline | Former | Stockpile Energy |
| Additional production queue | Scout, Former | Empty |
| Ordinary facilities | Tanks, Perimeter, Bank, Node | Perimeter, Bank, Node |
| Faction Base counts, attacker /defender | 1 /2 | 2 /1 |
| Treasuries, attacker /defender | 100 /200 | 200 /100 |
| Remote defender Former home | Captured Base1 | Remaining defender Base2 |
| Remote defender Former owner /position | Defender /(30,16) | Unchanged |
| Owned technologies | Ecology /Ecology | Unchanged after native technology comparison |

Both independent control Base records remain byte-for-byte unchanged. The
independent Scout at the attacker's original Base is unchanged in all captured
semantic fields. No unit is created, destroyed or renumbered before this notice.
The captured Base receives the governor settings of the attacker's existing
Base. This is an observed copy in this fixture, not a universal governor default.
The [donor-selector follow-up](capture-governor-donor.md) establishes the exact
population/HQ ranking and excludes the captured Base itself from eligibility.

The actor has already moved and spent its allowance when the capture body is
entered. The remote defender Former keeps its owner and position while its
home changes to the remaining defender Base. All three cached support amounts
remain0 at both boundaries. The faction population aggregates still contain the
pre-capture totals1 /3, even after the actual captured-Base recalculation. Do not
replace that aggregate with a freshly summed total earlier in the booty formula.

## Treasury and mandatory randomness

The denominator's [cached population lifetime](faction-population-aggregate.md)
is now traced: ordinary owner summary rebuilds it after Base upkeep, while
the covered founding/removal/capture actions preserve it until a refresh.

The inspected ordinary treasury branch computes the booty before population
loss: positive defender credits times the captured population, divided with
integer truncation by the defender's stored population aggregate plus1. Here
that is `200 * 2 / (3 + 1) = 100`. It credits the attacker and subtracts from
the defender. This one execution does not cover negative credits, insurance,
Headquarters relocation, or overflow boundaries.

The facility-loss loop consumes **37 CRT draws**, in ascending logical facility
ID order1..38, skipping Pressure Dome26. It draws even for absent facilities.
A remainder0 modulo3 requests removal of that facility. It then explicitly
removes Headquarters and, on this non-recapture branch, Recreation Commons
and Recycling Tanks. The forced Tanks removal is not evidence of a land-only
rule. With the final seed3 input, the loop randomly removes Tanks3, then the
forced removal requests it again. Perimeter, Bank and Node survive this seed.

With CRT seed3, the CRT state becomes1951609278. The shared RNG remains3 with
zero calls. The literal case retains the ordered random outputs. These draw
counts remain unchanged through Base-screen entry; they are not a full-command
RNG budget.

The attacker's two tracked directed encounter counters each increase0â†’4,
using twice the original population. The reverse counters remain0. Mutual
primary relationship words become2147524888 from16. These are logical values
for comparison with the existing relationship contracts, not new field-layout
instructions or a complete diplomatic interpretation.

## Required continuation and limits

Follow-up qualification: [native screen continuation](captured-base-screen-close.md)
executes the screen's recalculation, then finds a missing constructed window
object. Static inspection shows mode0 skips this function's modal wait loop.
The entry below therefore identifies a construction/show boundary; it does
not prove the movement caller must wait for a Base-management choice or close.

The notice entry is retained, then acknowledged with a display-service return
that its caller ignores. Original notification-history posting/view logic
executes using the established absent-window callback. Original technology
ownership comparisons execute; equal technology ownership produces no new
technology or research-lab increment in this case. The original elimination
helper returns with the defender still owning Base2. The original captured-Base
selection and recalculation execute, followed by the original map selection
query. That query takes the branch which opens Base1's interactive screen in
mode0.

The stopping point of this receipt is entry to that Base screen. Its show path
must complete before the caller continues; the separate interaction/close path
is not established. No Base-screen input is synthesized. The screen body
performs another recalculation and can accept gameplay edits, so it is not
replaced by a generic display-only acknowledgement. The capture
routine returns after the screen call in the inspected caller, but that return
and the outer movement-wrapper return have not executed. Selection and RNG
after Base-screen interaction remain unproved.

This receipt establishes an authentic Base-management continuation, including
the post-notice capture tail through its preceding recalculation.
Population1 destruction, recapture, defended capture, occupied homed units,
alternative facilities/seeds/factions, elimination/restart, naval capture,
networking, and rendered live play remain outside this case.
