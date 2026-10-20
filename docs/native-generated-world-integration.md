# Connect the native generated world to gameplay

This is the implementation sequence after the native generation tail closes.
The current generator reaches the original prelandmark boundary; its raw plane
must not be published as a finished world. The original-game reference now
matches seed 1 through final preparation and is checking discriminating controls.
That reference result is not yet port acceptance.

## Current coupling found in the live code

| Boundary | Current implementation | Required change |
| --- | --- | --- |
| Generation | `map/Map.cpp::Generate` uses `SimplePerlin` and `DisplayedTerrainV1` for generated SMAC maps | Generate owned native state, preserve the selected settings and explicit RNG inputs, then publish only completed state |
| Raw authority | `GlsmacResourceMapPolicy` stores `shared_ptr<const SmacPlanetMap>` | Retain a common native raw-map authority with explicit generated/installed provenance; keep installed-container decoding separate |
| Snapshot | `map/MapNativeTerrain.cpp` takes geometry, contours, landmarks and resource flags from `OriginalPlanet()` | Read those fields from the common native authority; combine them with current worker knowledge and live improvements as today |
| Rendering | `frontend/NativeTerrainSession.h::Admits` requires wrapped 80x80 retained geometry | Admit completed generated authority through the same proven geometry/material path, with explicit profile checks |
| Campaign restore | `GameCampaign.cpp` equates an original-plane policy with `MT_INSTALLED_PLANET` | Validate origin, geometry and selected categories against the authority's actual provenance |
| Save/network snapshot | `Game.cpp` repeats the installed-origin invariant | Save and restore generated raw authority together with gameplay RNG, topology, knowledge and simulation state |
| Native consumers | Resource policy, climate/global-spread publication, starting-base logic, minimap, territory and legacy mesh finalization use the installed source | Route the existing rules through the common raw authority; retain original bits through live mutations |

Paths above are relative to `src/game/backend/` except the frontend session.
`SmacPlanetMap` has fixed installed-Planet dimensions, category-4/header admission
and original container encoding. Do not manufacture a Planet header or relabel
generated games as installed maps to pass those guards. `OriginalPlanet()` is
currently both a raw-data accessor and a provenance test; those responsibilities
must be separated at its callers.

## Ordered implementation

1. **Close the producer.** Import the frozen landmark/final-preparation algorithms
   and controls. Compose them after `GenerateInitialClimate`. Verify final raw
   tiles, region storage, summary, landmark labels/centers, RNG and external
   clock inputs against actual original setup before faction startup. Reuse the
   existing climate algorithms where their caller thresholds/globals match.
   Initial source placement is not the later river-routing operation.
2. **Introduce the common owner.** Own dimensions/topology, every 44-byte record,
   resolved generation categories, resource pattern, sea/climate offsets and
   relevant rule flags independently of file format. Keep the complete raw
   authority rather than reconstructing it from display elevations. Installed
   imports retain their original container bytes and round-trip behavior.
   Immutable snapshots must outlive queued rendering/worker consumers.
3. **Connect resource and terrain consumers.** Use that owner for altitude,
   landmark membership/detail, resource conditions, geometry and native snapshot
   export. Preserve current live-terrain/observer filtering and cache invalidation.
   Audit every `OriginalPlanet()` use, including production cases listed above;
   changing the renderer's admission boolean alone is insufficient.
4. **Connect startup and settings.** Transfer selected size, ocean, erosion,
   temperature, rainfall, life and Standard Rules without drawing replacement
   settings from JSF32. Compose native seven-faction starting placement and its
   balance/ecology initialization with the exact completed generated plane.
   Keep the explicit generation CRT/shared states through startup. No captured
   start sites, fabricated reveal radius or wholesale map reveal.
5. **Persist one consistent world.** Introduce a versioned generated-authority
   payload and validate its origin/options at every save, load and network
   boundary. Preserve existing saved policy versions. Keep parse/validation
   detached until the owning worker can commit a complete world. Cancellation
   or failure must not publish partial authority, stale knowledge or a mix of
   Perlin and native records.
6. **Switch the live caller.** Replace the generated SMAC branch in
   `Map::Generate` once the above works through actual startup and restore.
   Preserve the other ruleset's behavior. Verify the original generated 80x80
   scene first and extend admitted dimensions/profiles from source evidence;
   80x80 is an integration checkpoint, not the release's world-size limit.

## Evidence required for the live milestone

- An actual new game follows the selected setup choices through completed native
  generation and seven-faction startup; deterministic raw-state acceptance is
  produced from initial inputs, not captured intermediate outputs.
- The human initially sees the original explored footprint, black unknown grid,
  coastline, terrain color and relief at the original logical resolution/zoom.
  Compare native-size terrain crops and full runtime screenshots separately so
  actor/font/console differences do not hide terrain errors.
- Select and move a unit, found a base, advance a turn, then save/reload. Verify
  raw geography, resources, current terrain mutations, exploration, selected
  options and RNG continuation survive; queue-owned copies remain independent.
- Installed Planet saves and startup still work. Generated provenance is
  distinct and checked rather than inferred from whether raw records exist.
- Use focused cached development checks while implementing. This live
  generation/startup/save milestone warrants platform CI and native interactive
  validation on honey plus the supported Linux/Windows paths. Include immutable
  original/runtime comparison images in the visual PR.

Full 1.0 still includes the remaining game rules, content, AI, diplomacy,
victories, audio/animation, performance and release validation. Completing this
generator integration does not prove those requirements complete.
