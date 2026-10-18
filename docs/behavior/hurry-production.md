# RE-SMAC-HURRY-001: hurry quotes, payments and reset

Status: original-x86 checked for the SMAC fingerprint in the
[research ledger](README.md). All 1,781 scenarios pass, including 37 named
fixtures. Actual cost helpers, quote arithmetic, admitted payment writes,
selected guards and a post-faction-upkeep reset/settlement fragment execute.
The network synchronization callback is supplied. Full dialog admission,
numeric control behavior, locking and faction upkeep are not executed.
These are synthetic routine checks, not live-game observations.

This extends [production accounting](production-accounting.md). Hurry updates
minerals/payment state; it does not create a unit, grant a facility or complete
a project within the checked path.

## Total cost and quote

Obtain the total production cost `C` using the actual
[unit/prototype factor](unit-design-and-prototype-costs.md) or a negative item's
rules cost, multiplied by the rounded [mineral factor](mineral-cost-factors.md).
Let current stored minerals be `P` and `D=C-P`. Calculate in this order:

1. Unit: `price=2*D+trunc(D*D/20)`. Negative item: `price=2*D`.
2. For a Secret Project, double the price. Double again if `P` is strictly less
   than four times the mineral factor used for that faction/world/INDUSTRY.
3. Double if `P` is strictly below the active retool exemption.
4. Double if the Voice of Planet has a nonnegative recorded location, including
   a location owned by another faction. Ownership is not tested by this step.
5. Apply the faction HURRY percentage last: `trunc(price * percentage / 100)`.
6. A nonpositive result takes the no-purchase route.

Here `trunc` means toward zero. For the usual nonnegative terms, it is floor.
Do not recombine the unit quadratic term, multipliers and percentage into a
single fraction. Default ordinary HURRY is 100%. Local facilities and orbitals
have the same linear pricing step; the project-only multiplier starts with
Secret Project items.

For example, at mineral factor 10 and no other multiplier, unit cost 40 with
20 stored has quote 60. Recycling Tanks at total cost 40 and 20 stored has
quote 40. A project at cost 200 with 39 stored receives the early-project
doubling; at exactly 40 stored it does not. World/INDUSTRY/AI factor changes
move that threshold; it is not universally 40.

## Selected admission checks

The tested riot/menu guard blocks unit and orbital/Stockpile/project targets
during Drone Riots, while local facility targets are allowed through that guard.
This is one guard inside the dialog, not a full list of construction eligibility.

After the main dialog returns a positive purchase choice, a separate transient
turn-advance blocking bit can take the no-purchase route. The
[ordinary-local authority follow-up](hurry-ordinary-local-authorities.md)
identifies its server NEXT TURN producer and native clear boundaries: it is
false for the admitted fresh ordinary-local profile. It is not a scenario
option. That follow-up also resolves the active stock retool exemption to10
through the original RULES loader. Next, an
existing partial-payment marker blocks both full and partial purchase choices.
The marker check occurs before the full-versus-partial branch.

The earlier menu setup also compares available credits with the quote before
offering full purchase. That entire interface is not executed here; full-payment
fixtures use adequate available credits. The admitted full commit does not
recheck affordability on its own. Earlier ownership/recalculation, locks,
negative-budget menu handling and complete input admission remain separate work.

## Full and partial purchases

Available budget is treasury minus previously reserved spending.

A full admitted purchase spends the quote and sets current minerals to `C`.
It does not set the partial-payment marker.

A partial purchase uses admitted entered credits `E` and computes:

```text
paid = min(max(E, 0), max(treasury - reserved, 0))
new_minerals = P + trunc((C - P) * paid / full_quote)
```

It prices the partial purchase as a fraction of the original quote; it does
not invert the nonlinear unit formula for a smaller mineral amount. There is
no minimum-one-mineral credit. With `C=40`, `P=20`, quote 60:

| Admitted paid credits | Mineral gain | New stored minerals |
| --- | --- | --- |
| 30 | 10 | 30 |
| 5 | 1 | 21 |
| 2 | 0 | 20 |
| 0 | 0 | 20 |

Even a zero-credit admitted partial payment sets the partial-payment marker.
Negative entered credits or a negative available budget clamp payment to zero
in this fragment, then still follow that marker/baseline commit.

The traced numeric prompt is initialized with the smaller of quote and available
budget. Its wrapper converts this argument to initial input text; this is not
an additional quote ceiling in the payment arithmetic. The admitted payment
clamps only to available budget. Entering 100 with the 60-credit quote and ample
treasury produces 33 minerals, increasing the store from 20 to 53. Whether the
complete numeric control admits that input remains unverified; this fixture
specifies the post-entry arithmetic, not normal user-facing purchase limits.

Every successful full or partial commit replaces the current stored minerals,
switching-baseline minerals and baseline item with the purchased state. It also
updates the dialog's saved mineral baseline. Tiny/zero partial payments therefore
have state effects even when no mineral is gained.

## Immediate payment versus reserved spending

| Payment context | Treasury mutation | Reserved spending mutation |
| --- | --- | --- |
| Local, or network with reservation submode inactive | Subtract paid credits | Preserve existing reservation |
| Network with reservation submode active | Preserve treasury now | Add paid credits |

The network immediate-deduction branch requests faction credit synchronization;
the reservation branch does not request that callback here. Its body and network
delivery/atomicity are not executed. The reservation-submode producer remains
unresolved; these are resolved mode inputs, not a claim about a named network
configuration.

The immediate branch additionally subtracts paid credits from a separate global
credit counter. Fixtures expose that input/output as `credit_cache`; its meaning
and wider lifecycle are unresolved. Do not invent another treasury account or
port persistence requirement from this native bookkeeping field.

## Repeat requests, target changes and faction reset

After an admitted partial commit, the actual post-dialog guard rejects another
positive purchase choice. This includes a zero-credit partial commit. An actual
[automatic target change](production-switching.md) does not clear the marker.

Full payment leaves the marker clear. A subsequent full mineral store normally
makes the next quote nonpositive. If the target changes, the partial-marker
guard still permits a positive choice; that does not itself prove all other
purchase conditions or a later transaction succeeds.

The traced faction routine calls faction upkeep and later executes this
reset/settlement passage:

1. Clear partial-payment markers on every base owned by the processed faction.
   Preserve other factions' markers.
2. Subtract that faction's reserved spending from its treasury.
3. Set its reservation to zero and floor its resulting treasury at zero.

The actual reset fragment is composed with an earlier reserved partial payment
in a named fixture. The guard allows another positive choice afterward. The
fragment also handles reservations exceeding treasury, yielding zero. The
processed faction's other bases participate even if they were not the hurrying
base; foreign treasury/reservation state is unchanged.

This locates the reset relative to the containing faction-upkeep call. It does
not establish every turn/network entry into that routine or execute full upkeep.
Other status housekeeping in the passage remains separately named/researched.

## Overfilled-unit boundary

The admitted quote fragment has no independent `P>=C` exit before its formula.
For a unit, an overfill of 40 produces quote zero; an overfill of 41 produces
quote 2 before other modifiers. For cost 30, store 75, ordinary percentage and
no multipliers, `D=-45` gives quote 11. An admitted five-credit partial commit
then reduces minerals to 55 because the mineral difference is negative.

These are original arithmetic boundary results, not proof that the full dialog
offers such a purchase in a reachable normal game. Keep them separate from the
ordinary positive-missing-mineral contract; do not silently clamp the difference
inside the traced calculation. Extreme signed overflow is not verified.

## Fixtures and remaining scope

The [37 named fixtures](hurry-production-cases.json) cover quote thresholds,
foreign Voice location, percentage/zero price, tiny/zero/negative/over-quote
admitted payments, budgets, reservations, repeat guards, riots, target changes,
reset/settlement and the overfilled-unit boundary. Additional grids cover 768
quotes, 280 payment combinations, 48 guards and 648 human/AI cost compositions.
The AI compositions have a nonhuman leader, so both adaptive cost deductions
are inactive; their implementation is covered by the mineral-factor contract.

Unit factors and runtime negative-item rules costs are explicit resolved inputs;
some are custom values, not claims about installed item prices. `choice=none`
collects quote/gate evidence without a payment; its gate field probes the positive
partial-choice path. All quote/cost/payment arithmetic is original execution.
Full dialog/input bounds/cancellation, lock and synchronization bodies, mode
producers, whole turn ordering, full production completion, saves and live-game
confirmation remain open.
