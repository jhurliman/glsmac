# Retained-input climate and terrain recalculation

2026-10-18. RE-SMAC-CLIMATE-SEMANTIC-RULES-001. Implementable [named-field reference](climate_recalculation_reference.py) and [literal cases](climate-recalculation-rules-cases.json). This supplements the warming scheduler; the preceding32-control freeze remains unchanged. Broad RE remains paused.

**The reference now independently reproduces altitude reconstruction, height shaping, the admitted ecology volcano geometry, temperature, rivers and rainfall.** Thirty-four native comparisons check every byte of all3,200 tiles and the actual random requests, outputs and seeds. Eleven separate returned live-record controls establish flooding, Pressure Dome/population changes, actual Base removal and actual Unit compaction. Three complete recalculation calls also compare the composed first four passes before the established region/coastal passes.

## Caller order and integration

When the deferred scheduler applies its signed sea step, or after the separate high-history volcanic geometry, execute:

1. Reconstruct altitude classes from retained contours/applied sea offset, then ordered height shaping.
2. Recompute temperature codes using retained temperature category, climate-extra and world-builder divisors.
3. Rebuild rivers, with their contour changes and source-specific shared seeds.
4. Recompute rainfall and retained local climate flags.
5. Rebuild regions, then coastal metadata and site-rating invalidation; reuse region/Freshwater and [coastal/area](coastal-metadata-and-region-area.md) contracts. Preserve stored-area quirks rather than recounting final labels.
6. Run the admitted territory refresh, then reseed the shared generator from the supplied OS millisecond-clock authority and publish views.

Do this against the evolving staged world. Neighbors read earlier mutations in the stated scan order. Entity destruction can compact live arrays while a terrain routine is still traversing them. The reference's `entity_change` and `invalidate_landmark_cache` hooks identify real engine responsibilities; they are not permission to omit them. Occupied cross-category changes without a coupling hook raise explicitly. Region capacity/nonempty auxiliary summaries and the final frontend publication transaction retain their previously documented limits.

The reference models normal gameplay terrain mode and the already admitted valid-coordinate ecology volcano arm. It does not model the random placement/name-generation branches of general landmark generation or the separate generic volcanic disaster's81-tile blast.

## Geometry and numeric authorities

Coordinates use doubled X. Visit Y ascending, then each row's parity-valid X ascending. Horizontal wrapping and vertical clipping match the existing map contract. The literal fixture includes the exact ordered225 shaping offsets and ring ends `[1,9,25,49,81,121,169,225]`; this is a square/ring footprint, different from the21/49-square site/landmark footprints. Preserve order for RNG and repeated writes.

Two height tables serve different purposes:

| Meaning | Values |
| --- | --- |
| Contour class thresholds, including upper endpoint | `[0,20,40,60,80,100,120,140]` |
| River uphill-adjustment lower bounds by class | `[0,20,40,60,80,100,120]` |

Classify `contour - applied_sea_offset` by the greatest threshold index0..6 whose threshold is <= that value, defaulting to0 below all thresholds. The first seven entries coincide in the literal controls; the contour
resampling helper also requires the explicit upper endpoint140. Keep each
consumer's measured table and indexing rather than deriving another height curve.

Installed world-builder values used here: latitude divisor14, temperature-band divisor14, jitter divisor8, warming divisor8, mountain shadow5, hill shadow3, shadow step1. Standard retained temperature and rainfall categories are both1; the discriminators also use categories0/2 and climate-extra0/1/3. Pattern offset is27067. These are caller inputs, not constants to hardcode for every map.

Named tile fields in the reference are independent semantic inputs. `items` and `landmark_word` use the established original feature identities; this follow-up does not rename uncertain flags by guessing. Exact masks used by these routines remain explicit. `landmark_word` includes the signed detail/suppression byte; positive detail and complemented/suppressed detail differ. `remembered_items` contains seven positive-faction retained snapshots. No executable addresses or binary record offsets appear in the public reference.

## Altitude and height shaping

The first altitude pass assigns every tile's classified contour class through the ordinary class-change coupling. The second pass visits classes6,5,4,3,2, each in row order; for each matching tile, invoke height shaping at that class.

Height shaping clears its eight flood-count scratch entries to0 and eight affected-Base scratch entries to−1 **on every invocation**. It ensures the center class/contour, then processes outward rings. For radius1..class−1, raise cells below `class-radius`; stop this outward raising scan after the first ring with no changes. Independently, for radius1..7−class, lower cells above `class+radius`, again stopping after the first unchanged ring. The separate contour helper first performs ordinary class assignment, then checks whether the retained contour already classifies to the requested class. Only on mismatch, resample:

```text
span = threshold[class+1] - threshold[class]
residue = CRT_raw % span       # one draw when span > 1
contour = (threshold[class] + applied_sea_offset + residue) mod 256
```

After a changed ring cell, clear its cached occupying affiliation to15 only if it lacks a real cached Base/Unit occupant and the existing natural-resource resolver returns0. Reuse the measured resource resolver; the callback is not a random draw. Matching center classes can still lead to contour resampling, so class equality does not imply no CRT work.

Same-land class changes retain the previously measured rockiness branches. Raising can increment rockiness only when below2 and `rockiness+1 <= new_class-3`, on CRT remainder0 modulo `rockiness+4`. Lowering can decrement it only when positive and `rockiness > new_class-3`, on a nonzero remainder modulo `rockiness+1`. Sea-to-sea changes do not use those draws. Turn0 skips the ordinary live class-change side effects; contour resampling remains separate. On the same rising reconstruction, turn90 uses329 CRT draws while turn0 uses306.

The ecology volcano's admitted geometry makes ordered height requests0 then6 at its supplied center. It then marks the first nine ordered offsets with Mount Planet bit2 and their original detail indices0..8, applies item mask`ee377e83` (also`f3ffffff` at the center), adds item`100000`, sets rockiness2 and the corresponding dirty marks, and records Mount Planet's center. No new placement RNG runs on this valid-coordinate arm. Interior/west-wrap/west-flat/pole controls match exact maps and consume59/64/37/44 CRT draws respectively. The ordinary handler's separate candidate/occupancy admission remains in the warming handoff.

## Temperature

Reseed shared RNG to `pattern_offset+17`. Each visited tile, except the active landmark-bit1 detail<21 skip, draws `jitter_draw = shared(0,2*trunc(height/jitter_divisor))` then `height_draw = shared(0,temperature_category+1)`. The upper bound is exclusive; even an empty/single-valued interval advances the generator.

Let `J=trunc(height/jitter_divisor)` and `S=trunc(height/band_divisor)`:

```text
L = abs(trunc(height/2) - jitter_draw - y + J)
    - (temperature_category-1)*trunc(height/latitude_divisor)
    - climate_extra*trunc(height/warming_divisor)
index = trunc((trunc(S/2) + 2*L) / S)
value = 3 if index<=2 else 2 if index<=9 else 1
```

The nine-cell feature scan can add1 when an item`1000000` feature is present and the current class<=4 or that feature's class is at least the current class. If value>1, class6 subtracts1 when category<2, otherwise draws `shared(0,2)` and subtracts1 on0. Class5 subtracts1 when the earlier height draw is0. Store `clamp(value-1,0,2)` as temperature; preserve class/moisture. This optional third draw must remain conditional and ordered after the neighbor scan.

## Rivers

The reference gives the complete deterministic routing procedure. Clear item mask`280` everywhere, preserving source bit`100`. Visit source cells in row order, requiring land class>=3. Clear the temporary landmark visited bit`800000` for each new source; reseed shared RNG to `47*y+31*x`, draw initial direction in0..4, and mark the source visited. Directions are NE,SE,SW,NW in that order.

Each routing iteration draws one binary preferred-direction bias, excludes the reverse direction, and scans the remaining directions in numeric order. Candidate base weight is `32*clamp(previous_contour-candidate_contour+2,1,9999)`, plus31 for the preferred direction. Uphill candidates use their separate ordered minimum-contour/weight8,9,… path. Neighbor tests can halve the weight at an early land/sea boundary, divide by8 for an admitted prior-visited neighbor, force weight1 with three higher neighbors, or reject a candidate/terminate the whole source. The distinction between rejecting one candidate and ending the source is material; an already-river candidate ends the source immediately.

Draw the weighted choice only after a nonempty candidate list. On uphill movement, lower the new contour to `max(previous_contour-1, river_bound[new_class])`; this does not reclassify altitude immediately. Preserve same-class run length, visited-loop state and the1000-step bound. Source termination may add item`200` on a valid unoccupied land endpoint. Finally convert visited tiles to river bit`80`, and clear all temporary visited bits after all sources. These details are implemented directly in the semantic reference rather than replaced with shortest-path routing.

The seam discriminator produces68 changed tiles/136 shared draws with wrapping versus11/22 on the same flat map. A source already carrying item`1000000` still reseeds and draws its initial direction, then stops. Do not drop that draw.

## Rainfall and its retained scan state

Rainfall consumes neither random stream. It first clears the local auxiliary byte, then builds influence flags from item`400000` nine-cell overlap, sea proximity, item`200000` local influence, and hill/mountain shadows. Flags1/4 distinguish one versus overlapping influence;32/16 are west/east shadows;128 carries the sea influence;64 is the local influence. The first pass's source scan and clipped/wrapped neighbors determine the later pass's inputs.

Mountain/hill shadow extents are `base_extent ± (rainfall_category-1)*shadow_step`, with different signs west/east. Rays stop at sea or terrain at least as high as the source. The item`200000` footprint includes raw intermediate coordinates at X+1 on three rows; normalize through the original half-X storage addressing. Its middle coordinate can alias the center/east cell according to row parity. Rejecting that intermediate merely for parity mismatch changes the result.

The second pass carries wet/highland/lowland counters across the row-major scan, including across row boundaries. These are not reset per tile or per row. Polar rows receive moisture0 in this pass without resetting that carry. Rain category0 has a distinct highland-to-lowland carry transition: reaching zero need not re-enable wet carry. Rivers reset wet carry and can remove a negative local adjustment, conditional on half-X modulo3. Landmark/influence branches then adjust or force moisture, clamped0..2. The reference specifies each ordered branch and counter update.

The third pass changes a dry tile to moisture1 if any of its eight neighbors is rainy land (class>=3, moisture2). This includes sea/polar target cells. Its setter also sets landmark dirty bit`400000` and dirty-state bit1; simply writing moisture loses a measured side effect.

The scalar terrain outputs are independently modeled. The live rainfall routine additionally records nearby Base/viewer notification caches on a moisture change at nonzero turn. Preserve its source-mapped query/callback boundary and saved viewer authority; no full original UI announcement receipt is claimed here.

## Live class conversion: Base and Unit effects

On a changed class at nonzero turn, invalidate the landmark/detail caches first. Crossing land/sea categories masks actual items with`7e1377a3`, then copies that masked item word into all seven positive-faction remembered item snapshots. This happens before Base/Unit coupling. Final class storage occurs after the coupling.

For an existing Base, a new land class or an existing Pressure Dome skips the flooding damage/Unit cleanup branch. Otherwise draw one CRT value and compute:

```text
loss = clamp(trunc(population/2), 2 + CRT_raw%3, 10)
population -= loss
install Pressure Dome (facility26)
owner_status |= 80000
flood_scratch_count[owner] += 1
```

An objective Base is protected to population>=1. A surviving positive population returns without removing mismatched Units. If population is nonpositive, the last-Base/network-client guards can likewise retain it at1. An admitted ordinary two-Base deletion instead runs the actual Base-removal producer before continuing Unit cleanup. The local/network and objective branches are source mapped; the new live controls execute the ordinary local nonobjective families.

Without a surviving Base exception, inspect the live stack: air domain2 and Unit exemption flag32 survive; otherwise a Unit survives only when `(domain==sea)` matches the new sea/land category. Removal uses the ordinary dense compaction/stack/reference repair, not a swap with the last Unit. Cached affected-Base queries happen in the original source order.

| Literal control | Returned result |
| --- | --- |
| Empty-tile land→sea stack [Mind Worm, Isle, Locust] | [Isle, Locust] |
| Empty-tile sea→land same stack | [Mind Worm, Locust] |
| Land→sea, Mind Worm exemption flag32 | All three survive |
| Flood Base population12, no Dome, CRT seed0 | Population6, Dome installed, all three survive |
| Flood population4, CRT seed2 | Population2, Dome installed |
| Flood population40, seed0 | Population30: loss cap10 |
| Flood population3, last Base | Retained at1, Dome installed, all three survive |
| Flood population3, two live Bases | Flooded Base deleted; other Base compacts to0; only Locust survives |
| Existing Dome, population12 | Population12, no new flooding CRT draw |
| Complete later sea step with population12 | Population6/Dome/all three survive through complete recalculation |

In the two-Base case, ordinary Base removal first destroys the Isle while the tile still has its old land class; subsequent class-conversion cleanup destroys the Mind Worm. Do not keep the Isle by evaluating only the final sea class. The final Locust's nonlink record bytes are unchanged. The other controls likewise verify surviving nonlink bytes, actual deleted indices and stable compaction.

Faction population-summary canaries777/888 remain unchanged in all11 live controls, including Base deletion. Do not subtract live population loss from those retained summaries early; their existing owner-summary producer owns that lifetime. The complete sea-step control also observes flood scratch count1 immediately after the Base mutation, then0 before temperature because a later height-shaping call reset it. Owner status`80000` persists.

## Randomness and acceptance limits

The shared generator advances `seed=(1664525*seed+1013904223) mod 2^32` and scales its low16 bits into the requested exclusive interval. The CRT generator separately advances `seed=(214013*seed+2531011) mod 2^32`, returning bits16..30. Preserve every seed request and optional draw in caller order. The composed rising/falling cases match6612/6666 shared draws plus329/323 CRT draws before region rebuilding; after volcanic geometry the composed four passes match6600 shared draws and0 additional CRT. These are literal contexts, not universal budgets. The complete caller later resets the shared seed to clock2000 in these controls.

This closes the previously untranslated normal terrain passes and admitted ecology volcano geometry for the measured retained-input domain. Existing region/coastal rules are reused; their overflow/capacity and nonempty-summary limits remain. Original renderer/cache publication, every transport/cargo/ownership deletion family, objective/network UI behavior and an entire natural high-history campaign turn are not certified by these bounded controls. The new models use bounded integer inputs; corrupt values, zero divisors, arithmetic overflow and tiny-map repeated-alias families are not generalized. No high-history suppression or production change was made.
