# Retained territory summaries

The live territory producer now records territorial tiles, known tiles, suitable
terrain, covered suitable terrain, maximum effective site rating and nonzero
resolved site signals for all eight faction slots and all128 regions. Global
region maximum ratings and site counts are retained separately. This supplies
territory inputs needed by the remaining regional strategy/research integration;
it does not enable Blind Research by itself.

The [original lifecycle/counter receipt](behavior/regional-lifecycle-and-territory.md)
provides twelve complete native aggregate passes and eight isolated global-upkeep
gate controls. `SmacTerritorySummary` reproduces those aggregate outputs from
literal native post-resolver/cache tile inputs. Existing complete land/sea claim
and site-quality/cache tests cover the preceding producers independently.

## Live callers and authority

Founding, removal and the existing capture territory caller prepare the summary
alongside their replacement raw claims and rating history, then publish through
the existing transaction. Startup founding consumes its prepared knowledge rather
than the previously published map. The canonical global tail refreshes after all
owners only when turn modulo4 equals1. Owner upkeep, ordinary resource/research
queries, snapshots and restore do not refresh on demand. Terrain postprocessing
and region rebuilding preserve stale summaries until an actual territory caller.

Suitable G requires nonzero decoded moisture, rockiness below2 and no fungus;
there is no land-only test. Covered suitable F additionally requires the retained
Base-radius exclusion. A Farm alone never supplies F. Excluded sites contribute
rating0 without erasing their cached rating. Neutral territorial owner0 receives
tiles and site counts, while its maximum rating and known counts stay0. Site
classes1 and2 each add one, rather than adding their magnitude.

All positive faction slots independently read persistent exploration and the
saved map-reveal mask, without an active/human/ownership filter. The measured
Search-objectives override takes priority and counts only the raw special marker.
Its fresh-local value is explicitly zero, owned by the new topology history and
preserved in saves. The AI/editor writer remains unimplemented; this does not
claim arbitrary original scenario/faction-flag import. InstalledPlanetV1 reads
saved original water climate and bit20 markers. DisplayedTerrainV1 retains its
explicit zero projection for water climate and original-only special markers.
Original climate mutation and marker production are separate unfinished work.

## Save and validation boundaries

GLSTERR5 appends the owned counters and Search-objectives authority. V1 through
V4 frames retain absence; loading cannot infer historical counters or flags from
today's terrain. Existing area, terrain-fertility, military/Base and strategy
histories keep their separate lifetimes. Byte/word increments preserve their
native widths; global counts remain signed32 values.

Validation includes the twelve literal native projections, all eight scheduling
controls, focused address/undefined-behavior sanitizers, counter-width limits and
all truncated summary frames. Live backend tests exercise two real Base insertions,
removal, unchanged RNG, stale read-only/snapshot behavior and actual Start restore.
The existing Tanks production/save/reload scenario carries a summary canary through
real queued turns2..6: it remains through4, refreshes at5 and remains at6. Targeted
C++ correctness analysis covers the new producer, codec and affected live consumers.
The normal installed-Planet/startup/capture/restart groups remain integration gates.

Remaining work includes per-owner regional military/strategy producers, actual
route predicates, leading-human/endgame pressure, ecology input integration and
the live Blind Research priority/selection continuation. No fixed strategy or
placeholder zero is substituted for those missing authorities.
