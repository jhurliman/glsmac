# Lower-console Faction Dominance history and graph

**9 original history-writer contexts, 17 complete native graph builders and
6,860 exact line requests** match the portable
[reference](dominance_graph_reference.py) and [cases](dominance-graph-cases.json).
Native initialization and a16,000-byte uncompressed save/write/read branch also
execute. This closes the retained history source and graph coordinates; the
existing [faction-panel font contract](lower-console-faction-text.md) still owns
font/heading presentation. No new line-pixel or historical-font raster claim.

## Payload and writer

History is **1,000 rows ×8 signed16-bit values**, in turn-major order. Each
row has faction slots0..7. The ordinary score provider publishes only slots1..7;
slot0 is preserved. Whole-new-world initialization zeroes the complete16,000
bytes. Do not infer a reset of the separate ranking-score table from this clear.

The stored value is the low16 bits of the **fresh composite ranking score**,
read later as signed. It is neither the1..7 Might rating, rank-order position,
nor the working score buffer after destructive ranking consumes it.
Use the existing [ordinary score formula](ordinary-ranking-score.md), including
its retained input authorities and division order. Special/scenario score
branches remain owned by their existing scorer contract.

Both score-only and full-ranking calls publish the current-turn row **before**
the full-ranking pass replaces working score entries with−1. An inactive
faction publishes zero into its current slot; past samples remain intact.
Repeated calls in one turn overwrite that row, rather than appending samples.
Positive composite scores32768 and65536 store−32768 and0 in controlled native
inputs; do not clamp, saturate or store unsigned graph values.

For valid nonnegative turns, writes occur only while turn<1000. The turn1000
control recomputes working scores but preserves the entire history payload.
Native indexing does not contain a negative-turn guard; malformed negative
turns are outside this admitted storage domain.

## Sampling cadence and UI side effects

The global advance calls the full-ranking provider **before incrementing the
global turn**. Thus it publishes the turn being completed. A checked first
advance writes turn0 and then increments to1. The existing continuous
[ranking/upkeep ordering receipt](ranking-dominance-score-order.md) establishes
this call before faction upkeep/owner summary.

The lower-console **history builder and bar builder each call score-only
refresh**. A history rebuild therefore recomputes the current scores and writes
the current-turn history row from the inputs present at rebuild time. It is
not a read-only chart. Ordinary text painting afterward does not itself perform
this score refresh. The alternate global-controller branch also directly calls
the full-ranking provider.

Port integration must preserve these separate retained authorities: retain the
history, publish the pre-consumption scores at the native score-provider call,
and let the graph builder refresh its current row. Do not fill the series with
the consumed`−1` score sentinels, derive old points from current Might, or
silently replace old rows with a newly reconstructed history.

## Save/load and lifetime

The normal game serializer includes the entire **16,000-byte history block**,
including unused slot0 and unsampled future rows. The raw branch writes/reads
one item of16,000 bytes; each sample uses little-endian signed-short encoding.
Native write preserves the source payload, and native read restores all bits
from the memory-backed stream in the new controls.

The corresponding compressed branches route the same address, length and item
count through the existing compression wrappers; their routing is source-proved
here, not a new compression-format execution claim. A port-native save need not
copy the original container format, but must persist the same retained sample
values and current-turn lifetime. Fresh initialization clears history; opening
the UI does not clear it, resample old turns or manufacture missing rows.

## Reader and coordinates at1024×768

History mode reads **all seven series**, in faction order1..7. There is no
active/human/owner/viewer/contact/pact/knowledge filter on these history reads.
Native inactive and all-inactive controls still draw old samples for all seven
factions; only their freshly published current row becomes zero.

The existing column clips are`[269,73,495,165)` and`[498,73,724,165)` in
console-local coordinates. Inset each by3 for the graph primitive clip:
left`[272,76,492,162)`, right`[501,76,721,162)`. Add the console's screen Y
origin511 only at final screen placement.

Let graph rectangle be`[L,T,R,B)`, width`W=R-L`, and font advance13. Available
vertical scale height is`H=B-T-13-4`, which is69 in both measured columns.

Choose the smallest positive integer horizontal step`S` for which
`currentTurn // S <= W`. This is **not** generally`ceil(currentTurn/W)`.
At width220, turns220/221 use steps1/2, and turn441 still uses step2. That last
case can generate a final point at the exclusive right edge before clipping.

Let`M=max(1,currentCompositeScore[1..7])`, using the refreshed signed32-bit
scores. Vertical divisor`D=max(1,ceil(M/H))`. It uses **current scores, not the
maximum across history**. The native68/72-current-score controls give divisors
1/2 at H69. No score normalization to a percentage or leader's relative share
occurs.

For each faction, sample turn indices **1,1+S,1+2S,…≤currentTurn**. Turn0 is
stored but is never a plotted point. For point number`k` starting0:

`x = L+k`

`y = B-1-truncTowardZero(signedHistory[1+k*S][faction]/D)`.

Join successive points only. Turns0 and1 produce no line segment; there is no
initial baseline-to-first-point line or explicit point marker. Do not stretch
short histories across the full width. Sampling skips rows directly rather
than taking an average, minimum or maximum over each interval.

Historical peaks can project above the available scale, and negative signed
samples can project below the bottom. The builder sends those original
endpoints to the line primitive; clip to the graph rectangle. Do not clamp
sample values or change the scale to include the old peak. This receipt records
primitive endpoints/colors and the native clip, rather than newly auditing the
line primitive's pixel raster.

The source reader does not clamp the global turn to history capacity. Beyond
the last valid row, some sampled turn sequences can read adjoining storage;
the bounded portable reader admits0..999 and explicitly rejects higher turns.
That rejection identifies an unclosed fidelity edge, rather than claiming the
original stops its graph loop at999.

## Colors, heading, axes and legend quirk

Each series uses its resolved owner-slot graph color. If that faction's retained
filename equals case-sensitive`SPARTANS`, use the corresponding alternate graph
color table instead. This is not a human/selected-owner highlight. The fixtures
declare filenames and the actual native color-table inputs; custom faction
color production remains a separate authority.

`Faction Dominance` stays the ordinary regular-Arial heading specified in the
font handoff. The history branch issues no separate axes, grid, year labels,
tick labels or numeric scale labels in these controls. Those must not be
invented as native output.

There are seven ranked-name legend row buffers. In the measured columns,
native width fitting compares remaining relative width with the final absolute
plot X, repeatedly truncating every supplied`Faction N` name to empty. All17
contexts retain the heading plus seven empty legend rows. Captured native
measurement calls verify the names were supplied and then shortened; the
emptiness is not a missing-name input. This is a bounded1024-pixel behavior,
not a general legend rule for every possible layout.

## Validation and boundaries

The writer controls cover turn0, turn1, same-turn overwrite, inactive slots,
turn999, turn1000, full-ranking consumption and two short-width wrap cases.
The graph controls cover first points, exact width/division boundaries,
current-height boundaries, old peaks, negative samples, left/right columns,
inactive/all-inactive states and the last valid history row. Complete native
builders execute the real score-only provider; history/score results are not
substituted. Only final line requests are captured as observation services.

Run`python docs/behavior/dominance_graph_reference.py` for the asset-free
writer/serialization/coordinate checks. The original setup uses external owned
labels/theme art and current Windows font metrics; no original art is embedded
in these cases. Full compressed-save wrappers, arbitrary special-game scoring,
turns beyond999, alternate widths/fonts and live timer/rebuild scheduling are
separate from this finite graph contract.
