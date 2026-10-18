# RE-SMAC-TERRAFORM-ECO-001: terraforming input to ecology

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Thirty-five named cases and 4,096 additional combinations match the
complete original radius-scan fragment, its original helper routines, and the
following mineral routine's initial divide-by-eight step. Full-game observation
remains pending. These are **not** final eco-damage values or bloom probabilities.

## Which squares contribute

Scan the base's 21-square radius, including the center. Count improvements on
both worked and unworked squares. The scan does not require the tile to be
owned by the base's faction, nor does it filter by land/sea, resource bonus,
visibility or worker assignment before including a valid radius square.
An improvement outside this radius contributes nothing to this input.

In doubled-x coordinates, the relative positions are:

`(0,0), (1,-1), (2,0), (1,1), (0,2), (-1,1), (-2,0), (-1,-1), (0,-2),`
`(2,-2), (2,2), (-2,2), (-2,-2), (1,-3), (3,-1), (3,1), (1,3),`
`(-1,3), (-3,1), (-3,-1), (-1,-3)`.

Wrap x across the horizontal seam on wrapping maps. Flat maps reject
out-of-range x. Both reject out-of-range y; poles do not wrap.
Very narrow maps that alias radius positions remain outside accepted coverage.

The worked flag used below is the square's explicit flag in the base's worked
set. Do not infer it merely because the square lies in the radius. The center
is included in the scan, but its flag is still tested explicitly; its implicit
resource intake does not itself force the ecology worked multiplier.
How the original assigns or maintains these flags is a separate worker rule.

## Points per square, in order

1. Add 1 for each present Road, Mag Tube, Mine/Mining Platform, Solar Collector/
   Tidal Harness, Farm/Kelp Farm, Soil Enricher, Condenser, Echelon Mirror and
   Thermal Borehole. Combined flags count independently, including Road and
   Mag Tube together.
2. Double **only that count** if the square's worked flag is set.
3. Add 8 more for a Borehole, 6 more for an Echelon Mirror and 4 more for a
   Condenser. These extra weights are not doubled for working the square.
4. Subtract 1 for Forest, regardless of whether the square is worked.
5. If both Tree Farm and Hybrid Forest are actually present at the base,
   replace this square's points with zero. Otherwise, if either is present,
   divide this square's points by two, truncating toward zero.

Fungus, river, sensor, bunker, airbase and the base marker do not directly add
a point in this scan. This does not specify their other ecological or gameplay
effects. The rules count stored feature combinations even when an editor is
needed to create them; legal construction/removal behavior remains separate.

With neither mitigating facility:

| Feature alone | Unworked points | Worked points |
| --- | --- | --- |
| Road, Mag Tube, Mine, Solar Collector, Farm or Soil Enricher | 1 | 2 |
| Condenser | 5 | 6 |
| Echelon Mirror | 7 | 8 |
| Borehole | 9 | 10 |
| Forest | -1 | -1 |

A Tree Farm alone and a Hybrid Forest alone have the same halving effect in
this scan. Both together remove the entire terraforming contribution; they do
not merely quarter it. The two facilities' other ecology effects or contribution
to clean-mineral allowances are not established by this scan.

Round the halving separately for each square, then sum the results. Two
unworked roads with a Tree Farm give zero plus zero, not one point from halving
their combined total. A Forest's -1 becomes zero under either single facility.
Without a mitigating facility, negative forest contributions remain in the sum.

## Initial scaling and remaining ecology calculation

The next mineral/ecology routine divides the summed points by eight using
rounding toward negative infinity. For nonnegative totals this is the usual
integer floor. A total of 7 becomes 0; 8 becomes 1; -1 becomes -1 rather than 0.
This rounding differs from the per-square facility halving.

This scaled value next enters the separately verified
[clean allowance and mineral burden](clean-minerals-and-ecology-burden.md)
calculation. Later faction statistics, PLANET, human/AI settings and global
scaling are not yet fully specified. Do not expose these points as final eco-damage,
apply a guessed bloom chance, or substitute an unverified community formula.

The [35 acceptance cases](terraforming-ecology-input-cases.json) compare raw
points and the scaled contribution. The additional grid covers all 512
combinations of the nine counted feature flags, both worked states, and all
Tree Farm/Hybrid Forest combinations on a radius square. Named cases cover
forest subtraction, uncounted features, foreign tiles, radius boundaries,
center flags, seams and pole behavior.

Worker-state updates, final eco-damage, clean-mineral history, fungal-pop/bloom
selection and consequences, climate changes, extreme overflow, full-game
observations and SMAX remain open.
