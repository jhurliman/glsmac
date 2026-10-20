# Connect the native generated world to gameplay

The [owned native terrain producer](native-complete-terrain-generation.md) now
matches original seed 1 through final preparation, before faction startup.
Its terrain/region/profile/name buffers and RNG are verified in the port.
The live caller still needs the authority and startup integration below;
pre-startup terrain must not be presented as a completed playable world.

## Current coupling found in the live code

| Boundary | Current implementation | Required change |
| --- | --- | --- |
| Generation | `map/Map.cpp::Generate` uses `SimplePerlin` and `DisplayedTerrainV1` for generated SMAC maps | Generate owned native state, preserve the selected settings and explicit RNG inputs, then publish only completed state |
| Raw authority | `SmacTerrainAuthority` now owns installed/generated read views; installed resource queries use it | Connect the remaining live consumers and generated publication while keeping container decoding/provenance separate |
| Snapshot | `map/MapNativeTerrain.cpp` now reads geometry, contours, landmarks, resource fields and cache identity from the common terrain owner | Preserve observer/live-terrain composition when generated policy/publication is connected; the current render snapshot remains80x80 |
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

1. **Producer acceptance is established before startup.** `GenerateTerrain`
   composes the frozen rules and verifies complete original seed-1 output,
   profiles/names, clocks and RNG. Retain the discriminating controls and
   distinguish initial source placement from later river routing when
   extending admitted world profiles.
2. **Connect the common owner.** The [shared terrain authority](native-terrain-authority.md)
   now retains installed/generated read views and serves installed resource
   queries. Connect dimensions/topology, every 44-byte record,
   resolved generation categories, resource pattern, sea/climate offsets and
   relevant rule flags independently of file format. Keep the complete raw
   authority rather than reconstructing it from display elevations. Installed
   imports retain their original container bytes and round-trip behavior.
   Immutable snapshots must outlive queued rendering/worker consumers.
3. **Connect resource and terrain consumers.** Use that owner for altitude,
   landmark membership/detail, resource conditions, geometry and native snapshot
   export. Preserve current live-terrain/observer filtering and cache invalidation.
   Snapshot export and geometry cache now use the common immutable owner,
   including flora preview restoration. Installed-origin negative detail remains
   explicit; generated publication and codec admission remain separate.
   Audit every remaining `OriginalPlanet()` use, including production cases listed above;
   changing the renderer's admission boolean alone is insufficient.
4. **Connect startup and settings.** Transfer selected size, ocean, erosion,
   temperature, rainfall, life and Standard Rules without drawing replacement
   settings from JSF32. Compose native seven-faction starting placement and its
   balance/ecology initialization with the exact completed generated plane.
   Keep the explicit generation CRT/shared states through startup. No captured
   start sites, fabricated reveal radius or wholesale map reveal.
   [Faction prefix and initial exploration](native-startup-faction.md) now match
   complete original record/store/RNG controls; terrain generation composes with
   the first prefix. The [generated search/cache adapter](native-generated-first-site.md)
   now derives the original first site and initial exploration.
   [Named technology acquisition](native-startup-technology.md) now extends that
   first-faction composition through its parsed initial award. Base construction,
   automatic designs, full Unit factory and controller stages still follow it.
   [Capability refresh](native-startup-capability.md) now matches complete
   original faction buffers and ordered stores; the generated first faction
   composes through the capability boundary.
   [Opening design construction and caller Scout selection](native-opening-designs.md)
   now match the stock named-grant controls and run in the Gaia composition.
   Installed predefined catalog initialization, actual localized names and later
   workshop branches remain required.
   [Base seed and radius preparation](native-startup-base.md) now match complete
   original controls and the generated first-Base/tile projection.
   [Territory/cache rebuilding](native-startup-territory.md) now extends that
   projection through `4faa88`, including all generated uncached ratings and
   ordered tile writes. Resource/citizen passes and automatic designs still follow.
   [Ordinary ground factory and sight](native-startup-ground-unit.md) now extend
   the first-faction Unit/map projection through the initial Scout at `5c9fb5`,
   including the caller's home reset. This does not substitute for full Base
   resources, automatic designs, other factory profiles or controller equality.
   [Nutrient accounting and opening support](native-base-nutrient-accounting.md)
   now match complete raw-record/store controls. Their upstream worker/intake
   inputs and the remaining resource/citizen stages still need composition.
   [Mineral accounting and ecology stores](native-base-mineral-accounting.md)
   now match complete raw-record and ordered-store controls, sharing the live
   arithmetic core. Worker intake, energy/citizens and controller composition
   remain required before publishing a generated game.
   [First-HQ energy and settled citizens](native-opening-energy.md) now match
   complete native records, commerce/citizen scratch and ordered stores. The
   actual calculated psych feeds the citizen tail.
   [Worker intake and complete first-Base composition](native-first-worker.md) now
   derive all304 original Base bytes from the independently generated Gaia world,
   including exact worker query/score/store journals and unchanged map/regions/RNG.
   [Automatic Base naming and cursor progression](native-base-naming.md) now
   close the complete first Gaia faction comparison as well. Complete installed
   design initialization, other Base contexts and all-seven controller
   composition still need closing.
   [Canonical Unit initialization and stack placement](native-startup-units.md)
   now match original complete-record/pool controls; Base founding, complete
   factory visibility and seven-faction starting-package composition remain
   separate required stages.
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
