# Hab completion lifts housing before the same population step

2026-10-18, stock SMAC. **A Hab Complex or Habitation Dome completed during
Base upkeep is already present when that pass's population step checks housing.**
With a full existing nutrient box and nonnegative surplus, the newly completed
facility permits growth immediately in that pass.
[Literal four-case receipt](hab-completion-before-population-cases.json).

| Construction and initial housing | Start population / food | Facility present at population entry | After population / food |
| --- | --- | --- | --- |
| Complex completes; neither Hab initially present | 7 /80 | Complex | **8 /2** |
| Complex misses completion by one mineral after posting | 7 /80 | Neither | **7 /42** |
| Dome completes; Complex initially present | 14 /150 | Complex and Dome | **15 /2** |
| Dome misses completion by one mineral after posting | 14 /150 | Complex only | **14 /77** |

Every population entry still has the original population and full nutrient box,
with surplus2. In the completed cases the grant is observed while population
is still7 or14, before queue advancement and before population processing.
The ordinary growth branch then empties the box and collects surplus2. In the
matched blocked cases housing reduces the box to40 or75 and collects2.

The four contexts are unmanaged human owner1, turn1, Standard size, neutral
GROWTH and no housing-limit modifiers/projects. Active rules set caps7/14,
nutrient/mineral factors10 and nutrient requirement2. Ordinary moist-land
workers produce surplus2; mineral surplus is1. Declared installed-cost rows
are Complex8 and Dome16, yielding actual costs80/160. Completed cases start
with minerals80/160; comparison cases start78/158, so posting leaves79/159.
Each successful completion leaves mineral store1 and selects Stockpile from
the supplied queue. Dome controls already have Complex; neither control has
the facility being built at entry.

This executes the actual Base-upkeep caller through its resource/citizen
calculation, production gate, cost, grant, queue, baseline commit, complete
population step and second resource pass. The resulting population persists
through that second pass. It reuses the
[admitted upkeep-production harness](upkeep-production-loop.md): presentation,
history/storage services are supplied, and reached production cleanup tails
continue at the original epilogue. No grant, housing, allocation or population
result is substituted. Execution stops after the second resource pass; it
does not claim a complete Base/faction upkeep return or live Inspector test.

Preserve production grant before population evaluation. Do not cache the
pre-construction housing restriction for the later population step. These
four cases add no broader housing/facility eligibility matrix. Standing
research remains paused.
