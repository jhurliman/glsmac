# Live ordinary sight

This records the World24/KnowledgeV2 sight slice. The later
[ordinary ZOC integration](smac-live-ordinary-zoc.md) replaces the foreign-neighbor
command fence described below and adds KnowledgeV3/World26 directed Pact state.

The existing Map-owned `SmacWorkerKnowledge` now receives ordinary exploration
and visibility updates from actual conventional Unit creation and successful
native movement. It remains an explicitly supplied, authenticated world value;
this change does not initialize campaign knowledge, diplomacy or territory.
Raw Early startup creation is unchanged.

## Authority and publication

The actual current Tile sea fields and saved explored masks feed
`SmacOrdinaryLandExploration::Explore`. Its ordered direct-nine refresh and
single ordered hole-fill sweep remain the measured kernel. The returned refresh
coordinates copy current GLSMAC feature/terraforming bits and Base/Unit presence
into optional per-observer remembered records. These are GLSMAC semantic copies,
not a claim that port flags reproduce the original packed map-word layout.
Missing observer memory and unknown individual squares remain distinct. No
terrain values, foreign visibility, claims or relationships are inferred from
an absent record.

Creation prepares sight, the Unit, registry/tile maps, placement history and
presentation queue before publishing identity or membership. Movement prepares
sight alongside its existing copied RNG and destination/update storage, then
commits through nonthrowing swaps and scalar writes. No callbacks run within
either commit. The existing spawn hook runs afterward. A failed movement lottery
retains its actual RNG/spent/failure result without changing sight. Restore
reuses saved knowledge after both entity managers bind; it does not replay sight.

Local canonical `move_unit` events are checked at enqueue and dispatch and use
the native movement path. Other event and terrain/turn mutation fences remain.

## Visibility domain

The ordinary positive-owner branch clears the moving Unit's stored visibility
before event-local detection. Adjacent foreign ordinary Units reveal each
other; an adjacent foreign Base grants its owner's bit. Older stationary flags
persist. Worker diplomatic exemption remains a separate saved predicate.

This uses the accepted `foreign-land-visibility` receipt's singleton,
unobscured flat interior land domain. Sensors, detection across map edges,
concealed/modified detection tiles, and foreign mixed stacks reject before
publication. The existing foreign-neighbor movement/ZOC guard remains; this
change does not claim the complete captured approach/departure command history
is playable yet. Native actors, contact effects, global Pact sharing, stationary
sight commands and general upkeep visibility are not activated.

## Persistence and checks

Knowledge V2 retains its fixed tile/Unit prefix and appends optional remembered
rows. Each known square contains two uint16 fields and two presence bits. The
4 MiB aggregate bound and complete suffix framing are checked before array
materialization. World24 rejects older headers before body loading; Unit3,
Base-manager8 and Simulation8 remain unchanged in this source branch. Integration
retains the separate World23 frozen-priority profile prerequisite.

The final focused normal run passed 24 memory-tracked groups in 1.066 seconds:
the existing 194 factory and 14 transition exploration expectations; owned
knowledge/binding/restore tests; and four new live sight groups. Those groups
cover unknown/copied memory, malformed suffixes, actual neutral/Pact/Treaty
factory-to-worker outcomes, mutual visibility, real GSE failed and successful
movement, stationary retention, exact actual restore and prepublication
rejection. The movement fixture supplies an already-retained foreign flag at a
remote position to respect the unchanged ZOC guard. It does not rerun the full
original 80-state visibility corpus. The identical 24 groups passed with
ASan/UBSan/LSan and memory tracking in 3.122 seconds. LLVM15 analyzed the six
changed production translation units with zero failures (18.553 seconds including
the runner). No full suite or platform run is claimed.
