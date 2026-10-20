# Startup territory, regional statistics and rating caches

`RebuildSmacStartupTerritory` implements original `0x538e80`, the first complete
territory/statistics/cache pass after Base radius preparation. The generated
composition now reaches `0x4faa88` using owned terrain, region, faction and Base
records. Uncached ratings are computed from those records; captured ratings and
intermediate original buffers are never producer inputs.

## Territory and statistics

The pass traverses ascending Y and parity-valid X. Nearest Base selection filters
by the founding tile's region, with later record IDs winning ties. Claim distance
is strictly below the supplied border on land and its integer half on sea. Sea
claims are canceled by a foreign land Base at equal or smaller distance, without
a region restriction on that second scan. Population and diplomacy do not change
these selection rules. No claim is stored as signed byte -1.

Every territory byte is written. A changed signed owner first marks the landmark
word and dirty bit; exploration and remembered items remain separate. All eight
factions' four regional word tables and two byte tables are cleared and rebuilt.
Word/byte increments retain native wrapping. Knowledge flags preserve the original
precedence between live ordinary-Unit presence and survey/exploration. Productive
terrain and Base-radius statistics retain the source's literal climate, rockiness
and fungus predicates, including their application to water tiles.

Only region DWORDs +8 and +12 are cleared; remaining bytes survive. Base-radius
tiles contribute rating zero even with a stored rating. Other tiles reuse a
nonzero cache nibble or evaluate the shared site-quality rules and store a new
nibble without changing affiliation. Region and owner maxima are updated.

`RawQuality` is the common turn-zero raw-record decoder now used by both the
starting-site adapter and this pass. It reuses existing quality, resource-bonus
and site-signal arithmetic, rather than introducing a second scoring formula.
The previous complete first-site acceptance remains in the composed test.

## Pods and goals

Supply Pod presence uses the existing site-signal resolver with current pattern,
rules and coefficients. Region and territory-owner Pod counts include neutral
faction zero. Claimed Pods become priority-1 type-3 goals when known or admitted
by the original high-difficulty computer rule. Unclaimed Pods additionally require
regional Base presence, no matching persistent goal, and nearest owned Base
distance in the inclusive interval [border, 2*border].

Raw goal tables are decoded into the existing `SmacComputerGoals` implementation
and written back without dropping auxiliary fields. It preserves duplicate-goal
behavior, first-slot ties, separate active/persistent replacement scores and the
absence of an active refresh for persistent duplicates. No new goal-insertion
algorithm is introduced.

## Original evidence and validation

Frozen bundle `independent-startup-territory-evidence-20261020` has manifest
SHA256 `5d1a16353fd73efba98e59cd64e09f81f5ab415906d21fd42b229e04c37960cc`.
All 182 files were verified before importing numeric fixtures.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Purchased assets and complete capture receipts remain outside Git.

- 26 controls compare complete tile, eight-faction and region buffers, unchanged
  Bases, dirty state and final nearest-distance output. All 19,968 ordered tile
  stores, including instruction PCs, match. Cases discriminate tie ordering,
  both topologies, sea/foreign-land competition, odd borders, knowledge-flag
  precedence, Pod modes and full/duplicate goal tables.
- The generated test starts from zero terrain storage and initial RNG, composes
  generation, first-site search, exploration, named grant and Base/radius seed,
  then compares all 140,800 map bytes, all 128 region records and the complete
  Base against original `4faa88`. The faction statistics and goal fields owned
  by this pass match as a projection; full intervening automatic-design state
  remains open.
- Every one of the 3,155 uncached ratings and 9,555 ordered tile stores in that
  generated pass matches the original journal. CRT remains 2011860586 and shared
  state 110. This pass accepts no RNG stream.

Cached incremental build and ten selected native groups pass, including existing
quality and goal-insertion controls. Five production translation units pass
correctness analysis. A focused seven-TU ASan/UBSan build passes the same groups,
with other objects uninstrumented and leak detection disabled. No platform matrix
was repeated.

Callbacks are synchronous over detached startup storage. Failures can leave staged
writes; a caller must not publish a partial world. Remaining startup work includes
Base resource/citizen passes, automatic designs/capabilities, ordinary Unit
factory/sight and full seven-faction ordering. Live generated-world save/network
publication remains part of the
[integration plan](native-generated-world-integration.md).
