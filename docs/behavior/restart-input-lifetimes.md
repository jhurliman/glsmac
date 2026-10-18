# RE-SMAC-RESTART-INPUT-LIFETIMES-001: native restart input producers

Four focused [cases](restart-input-lifetimes-cases.json) connect
[restart search](restart-search.md) to native producers. One world executes
region fill, the world-postpass caller, complete owner upkeep, founding/removal
and local cache invalidation/refill. A second capture prefix first rebuilds
regional counts natively, then observes capture. Two small setup fragments
identify ocean coverage. No search grid or replacement calculator is used.

| Input | Native producer and lifetime |
| --- | --- |
| Regional fertility F | Terrain postpass clears all regional counters, then counts qualifying tiles by current region label. Region flood fill alone does not refresh F. |
| Regional Base counts | Fresh faction initialization clears128 entries; founding/removal apply deltas. The post-Base owner summary clears/rebuilds only that owner's array. Capture does not transfer these counts immediately. |
| K | Retained ocean-coverage setting0..2. Setup selects it; generation resolves a negative random choice. Checked upkeep, founding/removal and terrain/cache passes preserve it. |
| Site-rating cache | Native site-quality query and its callers produce ratings. World postpass and local/Base mutation helpers invalidate entries; territory processing can refill them. |

## F is a terrain-postpass result

A qualifying tile is land, has nonzero moisture, is at most rolling and lacks
land fungus. Count its current region label. F is distinct from stored region
area A, territory fertility totals and regional Base counts.

The world rebuild runs region fill, then the terrain postpass. In the checked
world, fill produces main-region A=1024 but preserves stale F=777. The actual
postpass replaces that canary with F=960 in the main region and F=64 in the
polar region; all other F counters become zero. Main-region A remains1024.
See [region rebuilding](water-region-rebuild-and-freshwater.md) for the stored
area versus final-label distinction.

The postpass also clears **every cached site rating**, preserving occupying
affiliation. Broader rebuild callers include world generation, terrain/editor
changes and sea-level changes. This is not an unconditional owner-upkeep
refresh: the checked complete owner upkeep, founding/removal and capture leave
F unchanged. Do not make those operations silently repair an old F value.

## Regional Base counts have incremental and deferred writers

The rebuilding producer is the owner summary used by the
[population aggregate](faction-population-aggregate.md), after ordinary Base
and Unit upkeep. In this run owner1's stale77 becomes1; owner2's88 canary
preserves. Founding changes the stored count1→2 immediately; native removal
changes2→1. These are deltas, not full recounts or peer-owner refreshes.

The capture discriminator executes both native summaries first, producing
regional counts1/2 matching live ownership1/2. Actual undefended capture changes
live ownership to2/1 while saved regional counts remain1/2, with no count write
during capture. It stops at captured-Base screen entry without an answer. Thus
capture can make a previously accurate count stale before restart reads it.
Do not replace those stored inputs with current entity counts.

Fresh turn-zero initialization clears the array; nonzero restart bypasses that
fresh reset. Later owner summaries rebuild their own arrays. No extra global
recount at capture, every Base calculation or every region rebuild is established.

## K is ocean coverage

The setup indices mean ocean coverage0=30–50%,1=50–70%,2=70–90%. K is a retained
map choice, not a count reconstructed from terrain or factions. The admitted
native selection writer stores selected index2 as K=2. A separate generation
prefix resolves K=−1 with an actual CRT draw: seed3→3173050, raw48, K=0.
Other nonnegative choices preserve. The selection writer restores K=2 for
the lifecycle run; all subsequent checked stages retain2. Setup/new-map/editor
configuration paths are the identified direct writers. This is not a full
setup-UI, terrain-generation or new save/load replay.

## Rating function and invalidation callers

The missing implementation is the native **uncached site-quality query(x,y)**:
it evaluates the21-square neighborhood, nearby Bases and resource/site/terrain
inputs. Its complete formula remains the next extraction target; do not replace
its output with default1. The storage wrapper preserves affiliation.

- World postpass clears all ratings. The checked human owner upkeep leaves all
  1024 empty; a later producer must calculate any needed entry.
- Local terrain invalidation clears21 squares around the changed site.
  Static completion callers include fungus changes and Condenser/Borehole;
  the latter also request world rebuilding. Their broader admission belongs
  to the prior completion receipts.
- Founding marks its inner21 squares with the Base-radius exclusion and clears
  the following28 ratings. Removal clears49 ratings around the lost Base,
  repairs surviving Base-radius markings, then processes territory/ratings.
  Both complete native operations execute here.
- Native territory processing reads nonzero caches and calculates empty,
  nonexcluded entries. It computes989 ratings during founding and52 during
  removal in this world, storing1 as the actual result.
- A local clear at(40,16) clears exactly21 entries. Complete native territory
  processing then calculates and stores21 ratings of1, with RNG unchanged.
  These are observed outputs on this controlled map, not universal ratings.

Fresh map initialization also clears ratings. AI planning, owner-summary and
view routines have additional query-on-miss consumers. Direct-reference scans
are not complete alias/indirect-call analysis. Arbitrary terrain, full cache
history and the query formula remain outside this receipt; keep a missing
uncached calculation explicit until that function is implemented.
