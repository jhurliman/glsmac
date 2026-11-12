# Native Base targeting reads stored gross resources and eco-damage

2026-11-11 static producer mapping for `57cc80`, pinned stock SMAC. Reuses
the executed [native policy](native-wildlife-upkeep-and-command-policy.md),
[complete mineral/ecology pass](independent-startup-mineral-accounting.md),
[energy pass](independent-startup-energy.md), and commerce receipts.
No new runtime matrix or pathfinder equivalence claim.

## Bind these three fields

| Base DWORD | Ordinary completed resource-pass meaning | Bind to |
| --- | --- | --- |
| +0xc0 | Gross mineral intake, including incoming mineral convoys and mineral production multipliers; includes any final Space Elevator orbital-production doubling | Stored gross minerals, before support/outgoing demand subtraction |
| +0xc4 | Gross energy intake, including incoming energy convoys and incoming commerce | Stored gross energy, before outgoing energy and inefficiency subtraction, allocation, or category multipliers |
| +0x48 | Final calculated eco-damage from the mineral/ecology pass | Stored per-Base final eco-damage, not faction ecology aggregate, event probability after perihelion adjustment, or raw terraforming points |

The consumer's ESI is **Base+2**, starting at `96f1b2`. Its reads ESI+be
at `57cd78` and ESI+c2 at `57cd84` therefore mean **Base+c0 / Base+c4**.
It computes integer `32*distance / (c0+c4+32)` at `57cd8a..57cd93`.
Its ESI+46 read at `57ce0a` means **Base+48**, tested for exact zero.
There is no population substitution and no independent normalization to
population, net surplus, worker-only yield, or commerce-ranking snapshots.

In the existing doubling predicate, retained owner PLANET is still
`95d43c + owner*20bc`, and the Unit byte38 comparison still applies.
The zero test reads the stored final damage even though that damage's own
producer used **current** PLANET. Do not replace the predicate by retained
PLANET alone or by the faction aggregate being zero.

## Exact producer boundaries

1. **Intake `4fd650`** initializes bc/c0/c4 from the capped/possibly halved
   orbital contributions (`4fd6e1..4fd7b7`) and adds the actual selected
   worker yield-dispatch results. Its snapshot copy at `4fe8f8..4fe913`
   publishes ac/b0/b4/b8. At this stage c0 and c4 are intake values, before
   the mineral and energy transformations below. The separate b0/b4 snapshots
   do not automatically follow later gross writes; see
   [snapshot lifetime](commerce-ranking-snapshot-static.md).
2. The **tail of that same intake function** first sets +48=0 (`4fe91c`),
   scans the 21 Base-radius offsets, computes the existing terraforming burden
   including worked-square weighting and Tree Farm/Hybrid Forest effects,
   and accumulates raw points into +48 (`4feaf9..4feafe`). At intake return
   this is a temporary raw input, **not yet final eco-damage**.
3. **Minerals/ecology `4ff1e0`** adds incoming convoy scratch918684 into c0
   (`4ff201`), applies the production multiplier with one final division
   (`4ff2c0..4ff2cc`), and retains gross c0 while writing support/outgoing
   demand f0 and surplus d0 separately. It transforms +48 in place through
   terraforming division, clean-mineral allowance, divisor, mineral burden,
   atrocities, rule/perihelion multipliers, difficulty, current PLANET and
   technology-score factors. The final rounded +48 store is **4ff637**.
   Reuse the complete existing producer, including its signed rounding.
4. **Space Elevator comes after eco-damage** in that same mineral function:
   for the four orbital facility targets it doubles c0 (`4ff684`) and
   recalculates surplus. It does not recalculate +48. Thus the native picker
   can see doubled c0 alongside damage calculated from the earlier c0.
5. **Energy `500880`** adds incoming convoy scratch918688 into c4 (`50089e`)
   and then adds computed incoming commerce into c4 (`500aef`). Outgoing
   demand f4, waste e4 and net d4 are separate; the later allocation,
   economy/psych/labs multipliers, imbalance penalties, specialists and
   Stockpile Energy do not turn c4 into those category outputs. The citizen
   tail does not replace these three fields.

The ordinary full sequence is support `4feb20` -> intake `4fd650` -> nutrients
`4ff0a0` -> minerals/ecology `4ff1e0` -> energy/citizens `500880`.
`501570(force)` owns the selected-Base cache guard: run the sequence if forced
or the signed remembered Base ID differs; otherwise retain the stored fields.
Base upkeep has its two ordinary passes and conditional post-research third
pass as documented in [the ecology cache receipt](ecology-handler-research-cache.md).
Collection/production posting and ecological-event processing are separate
consumers, not alternative writers of raw worker intake.

## Lifetime at ChooseBase

`57cc80` reads each candidate's actual Base record directly. Its complete
function has **no resource-refresh call** and does not call `501570` to create
a fresh generation for each candidate. Bind the latest retained posted
calculation outputs already associated with that Base. A real preview/forced
resource calculation may publish a newer generation; do not automatically
refresh every candidate during native planning or reconstruct one from current
population/terrain/nearest distance. Mixed generations follow actual caller
order. Do not feed an interrupted intake-stage +48 into a model intended to
represent the ordinary completed calculation.

A completed ordinary Base creator explicitly runs support at `4faa9e`, intake
at `4faaa3`, nutrients at `4faaa8`, minerals/ecology at `4faaad`, and energy
at `4faab2`. Its returned Base already has the first calculated outputs.
This gives a producer-backed fresh Base value, rather than an assumed zero
from a new or reused roster slot. The earlier creator +48=0 store at `4fa709`
is superseded by that completed sequence. Legacy absent port cache values
cannot be inferred from a bare roster allocation or silently replaced by
population; retain absence or establish values through the corresponding
existing calculation boundary.

The retained home ordinal is a separate Unit input already covered by the
native policy receipt. This mapping adds no route-selection requirement:
the reported distant order24 target and the port's policy for choosing a
legal adjacent sea step remain separate from exact original Base scoring.
