# RE-SMAC-BUREAU-DRONE-001: bureaucracy input to citizen processing

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Eighteen named cases and 3,456 additional combinations execute the
original citizen bureaucracy-component fragment and its later addition to raw
drone demand. Intervening citizen effects and later conversion/calming are not
executed. The output is not a final visible unhappy-citizen or riot count.

## Component calculation

Use the same interval as the [bureaucracy warning](bureaucracy-warning.md):

`T = floor((8 - D) * (4 + max(EFFICIENCY, 0)) * floor(sqrt(tile_count)) / 112)`

For humans, `D` is that faction's difficulty; for AI it is 3. Tile count is
`floor(map_width / 2) * map_height` in the game's doubled-horizontal coordinates.
The checked domain uses normal difficulties, nonnegative counts/population and
products fitting signed 32-bit arithmetic.

Let `N` be the faction's current number of bases, `P` this base's population,
and `I` its **current position in the global base list**. For `T > 0`:

1. Compute `r = I mod T`.
2. Compute `q = trunc_toward_zero((N + r - T) / T)`.
3. Set bureaucracy input to `clamp(q, 0, P)`.

For `T == 0`, set this component to zero and skip division. This differs from
the separate manual warning caller, whose zero-interval positive-count case
faults in the scoped synthetic test.

## Distribution depends on global order

The distribution key is not this base's rank among its faction's bases. Other
factions' entries affect positions in the same global list. Do not substitute
a per-faction counter, an arbitrary stable unit/base identifier or a presumed
fixed redistribution of total drones. A port can preserve equivalent ordering
metadata without reproducing the original's storage layout. How deletion and
capture update that ordering remains a separate research dependency.

For interval 4, population at least 2 and five faction bases:

| Global base-list position | Bureaucracy input |
| --- | --- |
| 0 | 0 |
| 3 | 1 |
| 4 | 0 |
| 7 | 1 |
| 8 | 0 |

At eight faction bases the component is 1 for every remainder. At nine bases,
positions with remainder 3 receive 2 and the other remainders receive 1,
subject to the population cap. Consequently, changing the faction base count
can increase a previously existing base's component; the newly founded base
is not necessarily the one that receives the extra point.

## Entry into drone processing

The calculated component is added to raw drone demand after other sources
have been computed. This addition has no immediate population clamp: the raw
sum can exceed population. Later citizen conversion, specialists, talents,
psych, facilities and other calming effects must process it in their actual
order. Do not interpret this contribution alone as final drones, superdrones,
riots or a complete social-engineering result.

The cases supply other raw demand explicitly at this addition boundary; they
do not claim to execute all intervening sources. The component itself is capped
at population even when the faction base count is very large.

## Acceptance scope

The [18 named cases](bureaucracy-drone-input-cases.json) cover interval boundaries,
global-position residues, multiple layers, population cap, negative efficiency,
human/AI difficulty, raw addition and zero interval. The grid combines human/AI,
three difficulties, three efficiencies, three map sizes, four faction counts,
four global positions, two populations and two other-demand values, cycling
all seven faction slots. Zero population and oversized raw demand are supplied
arithmetic boundaries.

Full citizen-pipeline effects, global-order changes, complete refresh timing,
actual drone distribution after all effects, riots and original-game observations
remain open.
