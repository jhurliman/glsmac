# Climate initialization and live occupying affiliation

The fresh-world schedule resets pending climate steps, increment, divisor and
accumulator to **zero**. Divisor 20 belongs to event scheduling, not fresh-world
initialization. The pending-zero gate prevents the ordinary consumer from using
the initial zero divisor. The blank-map initializer separately resets climate-extra
and applied sea offset to zero. Existing warming-count initialization evidence
remains separate: the new prefix stops before that later lifetime is established.

The climate model's temperature category is a retained world header value,
separate from each tile's low-three-bit temperature code. Installed standard
Planet stores category **1**, at header offset 2624 (file offset 2639). The original
map loader reads that header directly. Blank-map initialization does not replace
the category. Reuse the [map import contract](standard-planet-map-import.md) and
[climate model](climate-recalculation-rules.md), rather than deriving the category
from rendered tiles or climate-extra.

Cached occupying affiliation is a retained four-bit map field. It shares its byte
with a separate high-nibble site rating. It is neither territorial ownership nor
a continuously derived view of all Units on the tile.

| Live operation | Presence flags and cached affiliation |
| --- | --- |
| Place first Unit on a valid tile without a Base marker | Set affiliation to Unit owner, preserving high site-rating nibble; set Unit presence |
| Add Unit to existing stack | Preserve existing affiliation, including a different owner's affiliation |
| Place Unit on Base-marked tile | Preserve affiliation; set Unit presence |
| Remove/move Unit out | Unlink; clear Unit presence only if last Unit; preserve affiliation |
| Found Base | Set Base presence and affiliation to Base owner, preserving site rating |
| Remove Base | Clear Base presence; retain affiliation in the checked complete removal |

Two complete ordinary Scout moves confirm source-tile affiliation survives both
departure from a Base and departure from an ordinary tile. The second movement
uses an explicitly renewed movement allowance, not a newly executed global turn.
Human and native factories/deletions confirm owners 1 and 0 respectively. A mixed
stack control creates human then native Units, removes the human, and leaves a
native Unit with cached affiliation 1 and Unit presence still set. Removing that
last native Unit clears presence but still leaves affiliation 1. These mixed
factory admissions are producer controls, not a claim that ordinary hostile
movement admits mixed stacks.

This history affects ecology. The native sweep rejects physical presence with
cached affiliation 1..7; volcano occupancy rejects cached affiliation 0..7.
Keep the presence flags and cached field together. Recomputing affiliation from
the remaining Unit would change the mixed-stack result before a repair occurs.
An empty tile's stale nibble alone is not physical occupancy.

There are explicit repair boundaries. Ordinary load runs a complete Unit map
repair, then a Base map repair. Unit repair clears/rebuilds Unit presence by
scanning dense Unit IDs in ascending order; for a matching Unit it writes the
first matching owner's nibble unless the tile currently has a Base marker.
Base repair clears/rebuilds Base presence and writes the first matching Base
owner. Both preserve the high site-rating nibble. Tiles without matching records
retain their cached affiliation. The order matters: Unit repair tests the incoming
Base marker, before Base repair reconstructs it.

Executed repair controls change the mixed survivor's stale affiliation 1 to native
0. A diagnostic real Base/Unit tile with affiliation 7 preserves 7 through Unit
repair, then becomes Base owner 1 through Base repair. A complete ordinary
memory-backed save/load with zero entity registries retains four deliberately
different affiliation bytes, while clearing their diagnostic presence flags.
Import also retains raw affiliation bytes; the map wrapper clears presence bits.
Initial blank tiles use affiliation 15, as established by the existing map reset
receipt. A stack query's inconsistent-presence recovery can also call Unit repair;
that additional caller is source traced, not a new executed recovery control.

The [cases](climate-initialization-and-occupant-cache-cases.json) contain 15 complete
factory/movement/founding/removal returns, three complete repair-helper returns,
the poisoned newgame prefix, blank-map reset and actual ordinary codec control.
Execution is pinned stock SMAC with existing headless presentation services. No
full GUI campaign or exhaustive editor/combat/transport ownership audit is claimed.
