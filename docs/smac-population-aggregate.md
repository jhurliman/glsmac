# Saved faction population summary

The native owner-upkeep controller now refreshes a persistent population
aggregate after that owner's Base pass. It includes completed growth and
starvation. Individual Base recalculation, founding, removal and faction
reinitialization do not refresh it. Fresh-world initialization sets it to zero.
The [original field-lifetime receipt](behavior/faction-population-aggregate.md)
distinguishes these phases and proves that ordinary save/load preserves stale
totals even when there are no Bases from which to reconstruct them.

This is the denominator input needed for capture's looted-credit calculation.
Capture must consume the saved defender total before ownership/population
changes; it must not sum current Bases instead. This change supplies the live
producer and save ownership, without activating Base capture.

The inner faction economy record is V2. It preserves the signed32 aggregate
independently of credits, pending debit, rates and research carry. Port V1
records had no such authority: migration marks the aggregate absent rather
than inventing a stale value from current entities. The next completed native
owner-upkeep pass establishes it. Capture will need an explicit admission check
for that migration state. Original-game save import is a separate feature.

Native integration exercises growth while a canary remains stale, explicit
owner refresh, subsequent starvation retaining that summary, actual Start
restore of the stale value, and refresh by the real end-turn controller.
Independent foreign-owner values and nonzero reinitialization are checked too.
The codec checks exact signed counters, old-format migration and bounded input.
The original receipt dynamically covers growth, founding and ordinary removal;
its starvation conclusion is static composition with earlier caller evidence.
