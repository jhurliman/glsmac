# Saved GLSMAC map resource policy

`GlsmacResourceMapPolicy::DisplayedTerrainV1` gives newly generated SMAC maps an explicit, owned interpretation of existing tile fields for the forthcoming canonical resource consumer. This is a GLSMAC generator policy. It does not reconstruct original SMAC height thresholds, map-generation probabilities, hidden resource patterns, landmark regions, or original random seeds.

This checkpoint produces and restores the map input. It does not yet replace `Tile::GetResources` or `Base::GetIntake`; the next consumer must also require the owned Base context, current faction state, frozen rules, explicit facility roles and supported no-project capability. An absent map policy means unavailable, not neutral. SMAX has no policy.

## Displayed terrain and bonuses

The policy uses `Tile::Update`'s actual five-sample water classification. A center's sign alone does not determine whether the tile is water. Water depth uses the strict comparisons already displayed by `tile_preview`: below -2000 is trench, below -1000 is deep ocean, and the remaining admitted water is shelf. Land uses `3 + max(center, 0) / 1000`; the existing admitted elevation range -3500 through 3500 gives land bands 3 through 6. A majority-land coast with a negative center remains land band 3. Out-of-range elevations reject before conversion.

Visible nutrient, mineral and energy bonuses map explicitly to their corresponding resource identities; their numeric enum orders differ between the port tile and the resource calculator. The existing bonus resolver receives an explicit forced/type-overridden bonus, so no undisplayed automatic coordinate pattern is introduced. Unknown bonus values, invalid coordinate parity, wide coordinates and visible bonuses below shelf reject. A rejected restored bonus is never rewritten.

For new policy-controlled generation only, `SimplePerlin::GenerateDetails` consumes the existing resource-candidate draw and keeps the candidate only on land or shelf. Rejected deep-water candidates still consume the same draw. Generation without the policy retains its previous behavior. The actual generator normalizes/scales elevations before details; its later fungus/moisture and impossible-feature passes do not change elevations. The regression validates every bonus against the final terrain returned by actual `Map::Generate`, in addition to comparing the detail generator's candidate order and final JSF state.

## Ownership and world format

`Map` owns an optional policy tag. Generation publishes it only after the existing cancellation check succeeds. Cancellation or a failed raw/imported tile load cannot imply a policy. `Map::LoadFromBuffer` clears the tag; a raw tile file does not contain this metadata and cannot reconstruct it.

World V18 writes an explicit presence Boolean and, when present, a framed policy after the simulation state and before the map body. The policy V1 payload is exactly two typed int64 frames, 28 bytes. The outer `ReadString(28)` checks the length before allocating/copying the string or advancing its cursor. Format and policy IDs must both be 1; no trailing fields are accepted. SMAX rejects presence before reading the string. World versions V1 through V17 remain unsupported; GlobalSettings V11 and Simulation V6 do not change in this checkpoint.

The actual download path decodes the policy before reading/allocating a Map. After the map payload loads, it installs the previously decoded owned tag. The world writer/body/`InitComplete` regression preserves the tag, both owned gameplay RNG words, the map JSF state and the complete fixture snapshot. This byte comparison uses a fixture with no unordered populated UnitManager tables; it does not claim arbitrary world wire ordering. Raw map import subsequently clears the tag.

Malformed policy checks cover unknown IDs, bad types, missing/truncated frames, exact-cap nested checksum/version errors, and complete 29-byte and 65536-byte outer payloads. The real immediate and queued Client final-chunk paths preserve the prior map/turn/managers/RNG and produce no initialization callback when the prefix rejects. The fixture preloads earlier chunks for larger payloads and dispatches the actual final Packet directly; it is not a socket throughput or rendered-world test. Existing partial-body failure/rollback limitations are unchanged.

## Validation

Six native groups cover visible depth/land boundaries, actual mixed-sign `Tile::Update` coasts, explicit bonus identities, the codec/cursor boundary, actual detail-generator draw order, actual generation/cancellation, and world continuation. The focused run also includes both actual Client rejection modes and the existing Ingress-to-Failure collection checkpoint. The latter hand-built SMAX snapshot needed an explicit absent policy field to keep reaching its intended malformed map-body checkpoint.

The final intended source passed the full normal CTest suite, 8/8 in 92.33 seconds, and the full leak-enabled ASan/UBSan/LSan suite, 8/8 in 204.22 seconds. Both binaries passed the same nine affected native groups with allocation tracking. Full shared-header correctness analysis covered 340 production translation units with LLVM 15, zero failures in 245.60 seconds.

Four copied-production controls each failed the intended assertion: retaining deep-water candidates, skipping rejected candidate draws, removing the outer length cap, and omitting restored policy publication. Intended source files were not mutated for these controls. The first copied Game compilation used a general include-directory flag and accidentally resolved the top-level ResourceManager header; the corrected control uses the original source directory as a quote-include path and reaches the actual restore assertion. The initial full-suite receipt with the omitted SMAX fixture field and these diagnostic logs remain retained separately from the final passes.

Validation logs and a source/log digest manifest are retained outside Git in `/home/jhurliman/src/glsmac-unit-weapon-parser/map-policy-final-manifest.json`. All original proprietary assets remain outside this repository. These checks cover an asset-free map/checkpoint fixture and actual packet handlers, not a rendered canonical base or complete first-worker turn.
