# RE-SMAC-ENERGY-WASTE-001: headquarters distance and energy waste

Publication scope: this repository includes this handoff and its literal cases. Other research notes named as unpublished follow-ups below are outside this published evidence boundary and do not expand its claimed scope.

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 27 named and 1,600 deterministic sampled cases pass. They execute
the complete waste routine through the actual energy caller's pre-commerce
and post-commerce fragments. Commerce addition is supplied between fragments;
the commerce builder and whole-turn timing are not tested.

## Distance to headquarters

Scan all live bases owned by the current base's faction that have completed
Headquarters. Use the nearest qualifying base, including the current base
itself. A foreign headquarters does not count. Merely queued Headquarters
does not count. Multiple supplied headquarters are handled by taking the
nearest, not the first in base order.

Coordinates here use the original doubled-column map coordinate convention.
For each headquarters, take absolute horizontal and vertical differences dx
and dy. On a horizontally wrapping map, if dx exceeds half map width, replace
it with map_width-dx. Flat maps do not take that seam shortcut. There is no
vertical seam shortcut in this calculation.

Let M=max(dx,dy) and m=min(dx,dy). The integer distance is:

`distance = floor((3*M + m + 1)/4)`

Do not substitute Manhattan, Euclidean or plain maximum-axis distance. For
example, an axial difference of 16 gives distance 12; differences 8/8 give
distance 8. The seam calculation uses horizontal map width, not an area-based
world-size scale.

If no qualifying headquarters is found, use distance 16. The original search
also has a boundary at 999: candidates at distance 999 or greater cannot
replace its initial sentinel. If none is closer than 999, it uses 16 as if no
headquarters were found. Synthetic large flat-map cases distinguish distances
998, 999 and greater. This is a custom geometry boundary, not evidence that
ordinary generated maps reach it.

## Waste arithmetic

Let E be the energy amount supplied to the waste routine. If E is nonpositive,
waste is zero. If headquarters distance is zero, waste is also zero, even at
very low EFFICIENCY.

Otherwise let effective efficiency F equal current EFFICIENCY plus 2 if the
current base has completed Children's Crèche. A queued Crèche does not count.

- If F<=-4, all positive E is wasted.
- Otherwise `waste = clamp(floor(distance*E / (8*(4+F))), 0, E)`.

Division rounds down in this nonnegative scope. High efficiency is not capped
at +4 here: +8 or +12 can reduce waste further. This differs from the separate
[unequal-allocation penalty](energy-multipliers-and-allocation-penalty.md),
which does cap the rating. Crèche's adjustment belongs to this waste formula;
do not add it globally to every use of EFFICIENCY.

Without HQ, E=20 and EFFICIENCY 0 waste 10 energy. A Crèche changes that to 6.
At EFFICIENCY -4 a non-HQ base loses all positive energy without Crèche; with
Crèche it uses the ordinary formula at effective -2. At EFFICIENCY -6, even
Crèche leaves effective -4 and full loss. A zero-distance headquarters still
has no waste in each case.

## Caller ordering and commerce recomputation

The energy routine first adds incoming energy convoys to gross energy and
records outgoing energy convoy demand. It calls waste with:

`E_before = gross_energy + incoming_energy - outgoing_energy`

Store that waste and `net_before = E_before-waste`. After commerce adds to
gross energy, call the waste routine again on the updated amount:

`E_after = E_before + commerce_added`

Replace stored waste with the new result and store `net_after=E_after-waste`.
Do not reuse the first waste value, or independently calculate waste on only
the commerce addition. For the no-HQ default E_before=20, efficiency 0,
adding 9 commerce changes waste from 10 to 14 and net energy from 10 to 15.

If outgoing demand makes E negative, waste remains zero and the caller's net
energy remains negative. The subsequent [allocation stage](psych-allocation-and-specialist-output.md)
clamps its input to zero; this caller does not clamp that net field itself.
Commerce can turn a negative amount positive and activate waste on the
second call.

## Acceptance scope

The [27 named cases](energy-waste-and-headquarters-distance-cases.json) cover
HQ ownership/selection, zero-distance exception, cylindrical/flat geometry,
rounding, Crèche thresholds, high efficiency, nonpositive input, convoy order,
commerce recomputation, queues and the distance sentinel. Sampled cases vary
map widths 64/128/256, coordinates, ownership, ratings and energy inputs.
Both wasted amounts, both net amounts and distances actually used are checked.

Original facility-bit and waste helpers execute without patched instructions
or substituted returns. The caller fragments are separated by supplied
commerce addition; this does not establish commerce amount, diplomatic
eligibility, map-border creation, HQ relocation/capture, rating transitions,
alternative-rating score bookkeeping, full energy refresh or complete turns.
The intervening commerce producer (unpublished follow-up `commerce-pairing-and-income.md`) is now
separately checked with its actual rating/rank helpers and continuous energy
prefix; its status/input lifecycle remains open.
