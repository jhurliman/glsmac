# Complete generated startup records

`CompleteSmacGeneratedStartup` composes the native gameplay producers from
completed generated geography through ordinary setup return. Its source-backed
acceptance fixture is seed 1, standard 80x80, Gaia as the human in slot 1,
difficulty 3, categories(2,1,1,1,1,1,1), all seven stock identities in their original slots, Standard Rules,
secondary rules 0, local play and immediate first HQs. This is a detached world
producer, not a live-game switch or a complete campaign implementation.

## Composition

The function copies the caller's generated terrain/regions and initializes the
predefined catalog from supplied decoded rules. For each faction, it initializes
the faction prefix, searches/refines a site against the cumulative world, prepares
strategy/exploration, grants named technology, refreshes capabilities and creates
early designs. It then names and seeds the Base, cleans its radius, rebuilds
territory, runs support/worker/resources/citizens, creates the Scout and clears
its home at the separate startup caller.

The first-Base accounting composition now accepts a selected Base in a shared
pool, with one owned Base and no owned Units. Foreign Units are passed to the
support producer; worker-radius and factory visibility admission remain enforced.
Worker Base stores are rebased to the actual record index. The actual human mask
and definition DRONE/TALENT values feed each owner's accounting.

After each Scout, the definition's intrinsic SOCIAL vector is installed. Applying
it before that faction's first resource calculation would change the fixture.
Neutral slot 0 is initialized immediately after the human faction, consuming its
eight draws before Hive begins. The local pairing loop retains all 403 rejection
draws, followed by original stock-identity rivalries. Existing startup balancing
then calculates the extra Pods and terrain markers from the cumulative world.
Each owner's Pod is created before that owner's marker writes, preserving Unit
stack links and remembered item words. No captured start sites, worker choices,
yield results, design records, balancing decisions or random values are inputs.

Search and balance now expose optional observers for their actual CRT draws.
These observe the draw where it occurs; they do not reconstruct a trace from a
final seed or advance a second gameplay stream.

## Original comparison

The fully verified 297-file frozen bundle
`independent-startup-complete-controller-evidence-20261020` has manifest SHA256
`295eee84b55f4653e2ab36930f5f72fbe778baf3aab743cbc25dbeb740761ecf`.
The original executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The original-x86 harness executes gameplay helpers with documented substituted
platform/file/UI services; this is not a new live interactive game capture.

The port's test starts with zero storage and explicit generation RNG/clocks,
runs `GenerateTerrain`, then the complete startup composition. It compares all
seven post-Scout boundaries and both controller/setup-return boundaries. At
completion the buffers contain:

| Buffer | Compared bytes |
| --- | ---: |
| Seven Bases | 2,128 |
| Eleven Units | 572 |
| Eight factions | 67,040 |
| 512 designs | 26,624 |
| Terrain | 140,800 |
| Regions | 3,584 |
| Technology ownership | 89 |

Counts, dirty flags, shared RNG 110 and final CRT 2566230518 also match.
All 8,339 ordered CRT draws match caller/before/after/result. Each of the seven
worker calls separately matches its complete journal: 721 Base/core-scratch
stores, 476 yield queries and 140 candidate scores total. Captured buffers and
journals enter only comparison fixtures. Numeric input tables and faction
definitions are declared source data.

Asset-free CI substitutes symbolic Base/design labels. It normalizes only the
25-byte Base-name fields, predefined name payloads including their terminator,
and 32-byte names of active custom designs. Other name tails and record fields
remain checked. It does not claim original text hashes under synthetic labels.

For a complete unnormalized comparison, set `GLSMAC_STARTUP_ASSETS` to the
purchased installation and run the same native group:

```sh
GLSMAC_STARTUP_ASSETS='/path/to/purchased/game' ./build/bin/GLSMAC \
  --gse-tests --gse-tests-native-filter 'SMAC complete generated seven faction' --quiet
```

This verifies original source file hashes before reading Base lists, predefined
names and localized early-design labels through the existing component parsers.
The same nine boundaries then compare full Base/design bytes, including names,
without normalization. Purchased text and captures remain outside Git; the test
writes no copies. Both asset-free and actual owned-text comparisons pass.

Cached incremental build and 26 affected native groups pass, including existing
search, balance, ground factory, design, first-worker and accounting controls.
Seven affected production TUs/consumers pass targeted C++ correctness analysis.
Focused eight-TU ASan/UBSan instrumentation passes the same groups plus the
complete owned-text comparison; other objects are normal and leak detection is
disabled. No duplicate platform matrix or new rendered-game image is claimed.

## Remaining live integration

The returned world is complete for this record-state fixture. Its terrain member
retains the geographic producer's `Prestartup` stage; callers must consume the
complete startup result rather than treating that geographic tag alone as proof
that gameplay initialization occurred. No live registries, UI objects or world
snapshots are published here.

The ordinary generated-game path, mutable raw authority, provenance/options
validation, saves/network snapshots, live faction scheduling and first full turn
remain required. Additional human factions, difficulty/settings, map sizes,
Look First, network play, configured UNITs, alternate identities and SMAX need
their own source-backed admission and comparisons. Successful seed 1 record
equality does not establish those branches or their rendering/UI behavior.
