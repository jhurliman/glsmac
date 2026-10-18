# RE-SMAC-COASTAL-AREA-PRODUCERS-001: coastal metadata and stored area

Status: ten complete native terrain postpasses and one additional region fill
match independent coastal and region models. Two postpasses run through the
actual rebuild caller, including its four earlier terrain stages, on the same
world. [Compiled native cases](../../src/gse/tests/fixtures/SmacCoastalGeography.h) provide decoded
terrain, initial stored areas, final region IDs, areas and coastal flags.

## Coastal predicates

The terrain postpass clears both flags on each tile before recalculating them.
Water tiles retain both flags clear. For land, use the ordered twenty neighbors
from [ordered terrain footprint](../../src/game/backend/map/GlsmacTerritoryTopology.cpp), omitting
the center. The first eight form the inner scan; the remaining twelve form
the outer scan. Apply horizontal wrapping when enabled and skip invalid tiles.
Land means elevation band at least3; lower bands are water.

For `coastal_complexity`, count land/water runs in each ordered scan:

- Start the previous classification as unset. The first valid inner tile
  counts as one run; each subsequent classification change adds one.
- Carry the previous classification into the outer scan. Its first *offset*
  always starts an outer run if valid. Later valid offsets add a run only on
  a classification change. If that first outer offset is clipped, the next
  valid offset receives no forced initial run.
- Skipped tiles neither count nor reset classification. There is no closing
  comparison from the last tile back to the first.

Set the flag exactly when the center's **stored region area is at least80**,
inner runs are at least4, and outer runs are at least4. Internally it can be
set during the inner scan and cleared again after the outer scan.

Set `nearby_distinct_land` when any valid outer-scan tile satisfies all of:

- Its row is strictly greater than4 and strictly less than `height−4`.
  These are the neighbor's coordinates, not the center's.
- Its region is land-designated, including the polar land region, and differs
  from the center's region.
- Both regions have stored areas at least40.

This second flag does not require either run threshold. Failing the outer-run
threshold clears only the first flag. Neither rule performs a fresh area count.
Other tile flags are preserved by these coastal writes. After the terrain
postpass, all cached site ratings are cleared; occupying affiliation is preserved.

## Stored area through the real rebuild caller

The checked caller does **not** reset the whole area table before region fill.
The [region-fill rules](water-region-rebuild-and-freshwater.md) still apply:
each discovered ordinary component resets its own counter and counts its
members. Entries for absent components remain unchanged. The final polar-row
pass adds to the existing polar counters, without subtracting those tiles from
the ordinary component counts accumulated earlier.

The same-world sequence below includes the caller's preceding terrain stages
on both rebuilds. The final row is a direct fill on the resulting world.

| Counter | Initial | First rebuild | Second rebuild | Further direct fill |
| --- | ---: | ---: | ---: | ---: |
| Discovered ordinary land | 901 | 251 | 256 | 256 |
| Polar land | 60 | 92 | 124 | 156 |
| Ordinary sea, absent after terrain edit | 965 | 5 | 5 | 5 |
| Unused polar sea | 70 | 70 | 70 | 70 |
| Unused ordinary entry | 4242 | 4242 | 4242 | 4242 |

Before the second rebuild, the ordinary land counter was deliberately changed
to77777; discovery reset it to256. Five water tiles were changed to land.
The current polar land population is32 in both rebuilds, despite the stored
counter increasing. Do not replace these stored values with final-label counts.

## Discriminators and connection to site quality

The cases distinguish area80 from79, nearby region40 from39, neighbor row4
from5, and the same seam terrain with wrapping enabled versus disabled.
The polar case has5 inner runs and4 outer runs: counting a fresh initial run
at the first *valid* outer tile would incorrectly produce5 outer runs.

The first full rebuild sets coastal complexity on a polar land tile using its
stored area92. The second rebuild clears it after the coast becomes all land,
even though the stored area increases to124. The site-quality H term
reads the union of these flags on its **last valid visited tile**. For example,
the query centered at(17,3) ends at(16,0), so that polar metadata is relevant;
H is not a center-only coastal test. The query behavior is established by the
linked receipt; this task tests its metadata producers, not new resource/site
resolution or search behavior.

Scope: supplied32×16 terrain, installed world-builder rules, region counts
below capacity, empty auxiliary summary grid, and generation-mode rebuild
through the end of terrain postprocessing. Logging, polling and numeric text
reading are supplied services; terrain, climate, fill and postpass calculations
execute natively. These are staged terrain edits, not proof of terraforming
command admission. Full world generation, nonempty auxiliary grids, capacity
merges, other rebuild modes, later territory/UI work, save/load initialization
and live-game scheduling remain open.
