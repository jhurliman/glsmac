# Regional flags at the Base-stage boundary

The actual post-Base owner summary now records the measured Base-stage regional
flags. At entry it retains only bit8, except when the low four bits of
`turn + owner + region` are zero. It scans current Base record order, and an
owned Base with an immediate sea neighbor adds bit128 to its stored region.
The current faction publishes this prepared snapshot together with its unit
counters; failures leave the prior state and peer snapshots unchanged.

The coastal query checks the original eight parity-coordinate neighbors in
order, wraps X only on cylindrical worlds, ignores off-map Y, and excludes the
Base center. Its result depends on actual sea classification, independently of
the neighbor's owner or region. The existing unit-phase flags remain separate.
Neither field claims the unfinished final peer/strategy flag word.

Simulation V20 adds an optional Base-stage snapshot for each positive faction
slot. Fresh admission explicitly initializes it to zero. V19 and older saves
retain missing bit8 history: a current coast query cannot reconstruct a prior
retained flag, so loading and later owner summaries preserve that absence.
Founding, removal, nonzero initialization and read-only queries do not eagerly
rebuild this snapshot. The codec preserves all128 regions and peer rows and
rejects malformed presence, truncation, trailing bytes or unrelated phase bits.

The [original Base-stage receipt](behavior/regional-base-stage-flags.md) supplies
51 native cases. The compiled fixture checks each complete reset array,
pre-Unit flag array, regional Base byte array and exact first sea-neighbor
direction using the published explicit profile/defaults. Public numeric arrays
are compacted without changing decoded values, and fixture comments identify
both hashes. These port projections are distinct from the original receipt's
broader watch of every faction array and initialization/storage controls.

Live native tests cover an actual coastal Base, peer canaries, staggered expiry,
late unit-scan failure rollback, removal without eager recount, real save/Start
restoration and the following owner summary. Codec/migration, existing territory
and affected Planet startup/upkeep/capture/restart groups pass. Targeted C++
correctness analysis passes on six changed/affected production translation units.
Nine focused groups also pass with the simulation codec and territory/coastal
translation units under ASan/UBSan, reusing the other normal objects and disabling
leak detection. A local sanitizer startup fault was traced before main to ASan
memory reservation; the final focused harness links without PIE.

Final regional strategy, the original reusable route-workspace lifetime,
leading-human pressure and ecology still need integration before enabling the
full automatic Blind Research dispatcher. Later flag writers must retain their
own measured stages and history; these snapshots do not supply invented zeros
for unfinished producers.
