# RE-SMAC-REGIONAL-LIFECYCLE-002

The nonzero restart preserves the regional arrays. Keep the port's existing
preserve/decrement behavior. The earlier [restart input lifetime](restart-input-lifetimes.md)
and the newer fresh-initialization prefixes describe different branches and
agree. [Numeric cases](regional-lifecycle-and-territory-cases.json) include two
actual nonzero caller sequences, twelve complete controlled territory passes,
and eight isolated global-upkeep gate tails.

## Nonzero initialization and later owner summary

In the actual turn3 starvation sequence, last-Base removal decrements its
regional Base byte before invoking faction disposition and initialization.
Poisoning every entry with Base77, units777, attack777, flags777 and strategy77
therefore gives Base76 in the removed Base's region and77 elsewhere at initializer
entry. These exact arrays survive initialization, native site search, native
replacement-Unit creation and return to the disposition caller. The fresh-only
regional-clear loop is never entered.

The subsequent owner summary is a separate operation. It clears/recounts the
owner's Base/unit/attack arrays and retains only flag8, subject to the existing
sixteen-phase expiry. In this no-Base replacement package, the final counts are
zero Bases, one ordinary land combat unit in its new region and zero attack.
Entry0 strategy77 survives; the later strategy pass rebuilds entries1..127.
All128 entries of the peer's five poisoned arrays remain unchanged throughout.

A second complete actual disposition call on an empty-Base faction likewise
preserves all the poisoned regional arrays through native search/package return.
There is no later owner summary inside that call. Do not use the full outer
upkeep's eventual recounted values as evidence that initialization cleared them.

Fresh turn-zero initialization does clear its regional Base/unit/attack/flag
arrays while preserving the strategy canary, as established by the earlier
prefixes. This receipt adds no unconditional strategy reset. Capability clears
have their own branch/lifetime contract and are not evidence for clearing regions.

## Separate global territory producer

At entry, clear these arrays for **all faction slots0..7 and all128 regions**:

| Summary | Storage | Recount |
| --- | --- | --- |
| Territorial tiles | Unsigned16 | Every tile, assigned to resolved territory owner |
| Known tiles | Unsigned16 | Separate per-faction exploration/override gate below |
| Suitable tiles G | Unsigned16 | Moisture, rockiness and fungus gate below |
| Covered suitable tiles F | Unsigned16 | Suitable and Base-radius-excluded |
| Maximum effective site rating R | Unsigned8 | Positive territorial owners only |
| Nonzero resolved site signals | Unsigned8 | Territorial owner, including neutral0 |

Also clear the128 global region maximum-rating and resolved-site-count values
(signed32 storage). Preserve the global region area and regional terrain-fertility
count, and every faction's Base/unit/attack/strategy/flag arrays. The territory
pass is not the owner summary or terrain-fertility producer.

Visit every parity-valid tile in ascending row order, including polar rows.
Run the existing land (reference receipt `land-territory-producer.md`) or
sea (reference receipt `sea-territory-producer.md`) territorial resolver, writing its raw claim.
An unclaimed result contributes to aggregate slot0. These are raw claims, not
the diplomacy-aware recognized-owner query used by workers.

G counts tiles with nonzero decoded moisture, rockiness below2 and no fungus.
There is **no separate land-only gate** in this aggregate. A controlled water
tile with qualifying moisture bits counts. Use stored tile classification,
not a fresh global fertility count or total region area.

**Correction to the earlier producer receipt:** F is covered suitable tiles,
not farmed suitable tiles. It tests the same Base-radius exclusion used by the
site-cache caller. Actual Farm alone gives F0; exclusion without Farm gives
F=G in the all-suitable controls. The native Base-marking helper supplies this
flag on its inner21 footprint. The old fixture key `owned_farmed_suitable` and
case ID `good4_farmed1` remain compatibility names; their numbers are unchanged
and mean covered suitable. Research pressure uses G−F with that interpretation.

For ratings, reuse the full cache contract (reference receipt `territory-cache-refill-and-radius-repair.md`):
excluded tile → effective0 without erasing its cache; otherwise reuse nonzero
cache, or query/store on zero. Maximize the effective value globally by region
and for its positive territorial owner. **Neutral R remains0**, even when the
global region maximum is13. Do not derive a neutral rating from neutral tiles.
Land/sea query formulas and exceptional turns are already supplied by
land site quality (reference receipt `site-quality.md`), sea site quality (reference receipt `sea-site-quality.md`) and
late/startup turns (reference receipt `site-quality-late-turn-and-startup.md`).

The separate site-signal count increments once for any **nonzero resolved
site/pod query**, not by that query's returned magnitude. It is not a count of
Monolith markers. Global counts include neutral tiles; faction-slot0 site
counts also increment. The controlled forced-signal case distinguishes this
from Monoliths and from the special known-counter marker below. Goal creation
that can follow this count is outside the new controls.

## Known-tile count and override priority

For each visited tile, independently for each faction1..7:

1. If its retained **special-marker-only known-count override** is set,
   count exactly tiles carrying the special known-counter marker. Ignore both
   the all-tiles override and ordinary exploration in this branch.
2. Otherwise, count when the all-tiles known-count override is set.
3. Otherwise, count only if the tile's exploration bit for this faction is set.

Slot0 known count stays0. No active, alive, human, territorial-owner or Unit
visibility filter is applied. This reads persistent map exploration, not a
foreign Unit's visible-to mask. The semantic override/marker names identify
these measured counter predicates; this task does not establish their editor,
scenario or gameplay producer. Do not guess their values from human status.

The later [authority receipt](known-count-authorities.md) identifies the
all-tiles flag as map reveal and the marker-only flag as Search objectives,
with actual reset/editor/technology/load controls. The original bounded
regional controls alone did not identify those producers; bit20 marker
production remains separate. Use that follow-up for current admission.

The80-tile control has60 tiles in region1 and20 in region65. Explored/all-tiles
gives60/20. The special-marker override gives9/3 even when all tiles are
explored and the all-tiles override is also set. Other factions retain their
own exploration-based60/20 result. A Monolith alone does not satisfy that marker.

## Scheduling and cache lifetime

Ordinary owner upkeep does not refresh the territory summaries on demand.
Reuse the existing executed founding/removal/input-lifetime sequences:
founding marks then refreshes; removal clears its footprint, re-marks all
surviving Bases, then refreshes. Terrain postprocessing clears ratings but
does not unconditionally refill them. Ordinary Farm/Road/Solar/Mine/Forest
completion preserves nonzero cache as covered by the existing former receipt.

The end-of-global-upkeep gate requests territory processing when
**turn modulo4 equals1**. Eight isolated native tails, with the full caller's
carried constant, test turns0..7: refresh at1 and5 only. This is not a claim of
eight full global turn runs. Advanced-start processing calls territory before
its owner summary; that caller order is static corroboration, not a newly
executed full advanced start. These are additional callers, not replacements
for founding/removal refreshes or a mandate to refresh on every research query.

Validation uses the pinned SMAC executable and declared controlled map/retained
inputs. No search, site, territory or route result is replaced. Polling and
inherited presentation/runtime services are declared privately. No rendered
live-game run, arbitrary scenario override lifecycle or general AI planning
claim is made; no production code or standing-goal state changes.
