# Canonical first Base during claimed startup

`BaseManager::SpawnStartingSmacBase` and its GSE entry
`spawn_starting_smac_base` create the first canonical Base for an authenticated
human owner during the actual claimed new-world Start at turn zero. The local
campaign has a Single local player, no Connection or network mode, and admits
the existing Single/Player human roster roles. The existing initial resource
context producer retains its first-owner-Base and global-difficulty-3 boundary.
This entry does not replace the default world script yet.

The producer consumes the frozen faction, resource, specialist, technology,
production and territory rules. It grants HQ and the owner's admitted FACILITY
records, chooses predefined Former 1 when its actual prerequisite is available
and Scout 2 otherwise, and stores half the calculated mineral cost in both
current and baseline storage with an explicit empty queue. The supplied worker
and seven specialist IDs must resolve to registered positional roles. The
default definitions now provide those roles and the stock Hive/University
starting facilities.

The actual first worker allocation uses the real terrain/resource queries and
the existing complete allocator. It reads Early **current** social ratings;
retained ratings remain absent. Existing startup Units must authenticate as
pending and homeless, establishing zero initial support without inventing a
retained SUPPORT value. Selection and active specialist count begin at zero.
GLSMAC explicitly initializes unused packed specialist storage to zero; this
does not claim that the original clears all padding in a reused Base record.
Initial food remains zero, or the existing deficit marker -1 when the first
calculation is short. No turn of food or minerals is posted here.

Fresh knowledge is an explicit local campaign policy before any entities:
unexplored tiles, no diplomatic disclosure or grants, and unknown remembered
improvements. The first Base reveals the actual ordered 21-square radius before
its center marker and any following Scout sight. Terrain topology and claims
are rebuilt from actual terrain, frozen border distance and saved Base order.
Later owners preserve existing knowledge and authenticate the earlier pending
Units and Bases. This is not inferred from movement history or resource-map
policy alone.

The detached candidate, population projection, knowledge, topology and all
allocating registry copies are prepared before identity/center publication.
Registry, placement and knowledge commits have no callback or allocation gap;
the normal `base_spawn` callback runs once afterward. Generic spawn/restore and
ordinary resource-query phase requirements remain unchanged. This entry does
not implement a Colony Pod command, general founding legality, energy posting,
growth, production completion, or the two-pass upkeep transaction.

The source boundary follows the retained first-base order, first-worker map,
founding facilities/population/production and first-upkeep receipts under
`docs/behavior/`. The new four native groups execute the actual claimed Start
and default definition handlers with synthetic owned rules/terrain. They cover
all seven stock faction packages, arid specialist versus productive worker
allocation, queued startup, prepublication rejection, callback completeness,
and a second Role::Player Base following a real pending Scout. They check saved
authority and unchanged random streams; they do not claim installed-asset UI
rendering or a completed upkeep.

Validation at this source checkpoint: changed production and fixture sources
pass syntax checks; five changed production translation units pass targeted
Clang correctness analysis. The four linked native groups are pending the
shared cached integration build. No new full-suite or platform run is claimed.
