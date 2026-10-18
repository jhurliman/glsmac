# Ecology terrain presentation batch

`Map::PrepareTerrainAppearanceUpdate` prepares CPU atlas patches and sprite
changes for a set of candidate tile appearances. The caller keeps live tile
fields unchanged during preparation, then publishes its complete gameplay
transaction and commits the prepared presentation. The existing owned
`TerrainRenderUpdate` payload crosses to the frontend for atlas/actor updates.
The existing ordinary Former update path remains supported.

Land fungus and rockiness use the existing LandSurface painter, with candidate
feature and terraforming lookups for neighboring connections. Changed fungus,
forest and road/tube connections rebuild their neighboring tiles in coordinate
order. Sprite replacement preserves bonus/pod identities and unchanged
improvement markers; removed farms, solar collectors, mines and soil enrichers
and invalidated Uranium/Geothermal markers are handled explicitly. This does
not implement missing original terrain geometry or improve the existing art.

Water fungus is a replaceable final paint in the retained coast recipe, after
coast and river-exit composition. The initial map generation uses the same
WaterSurfacePP painter and records that distinction. Open ocean without coast
paint gets a shared transparent base recipe, so its first bloom is supported.
Later coast refreshes retain the committed fungus overlay rather than restoring
its import-time absence. Replacement or removal can also be prepared privately.

Presentation uses a separate deterministic per-tile random stream. It never
advances shared gameplay or CRT state. Preparation retains live TileState colors
and cached coastal river textures, and clears candidate context on failure.
Coastal rivers reuse their already transformed exit texture while painting
staged land. Multiple tile sprite identities are assigned in coordinate order.
Contours, shared corners, water geometry and mesh normals remain unchanged.
Changes to unsupported feature/improvement producers reject rather than silently
publishing an inconsistent appearance. Render dumps without retained recipes
remain outside this update path; headless maps return no render preparation.

Validation: eight focused native groups cover land fungus edges and indirect
rockiness, stable repeated preparation, sprite identity preservation, water
fungus replacement and later coast replay, open ocean, discarded preparations,
late failure/context recovery, unchanged cached river pixels, the existing
ordinary Former path and updated saved Planet authority. The six terrain render
groups also pass ASan/UBSan with all five changed production TUs instrumented;
other objects reuse the normal cached build and leak detection is disabled.
Targeted correctness analysis covers Map.cpp, MapTerrainUpdate.cpp,
TerrainRenderUpdate.cpp, LandSurface.cpp and WaterSurfacePP.cpp.

This is the actual map renderer's prepared update API and initial recipe
capture. Joining it to the live ecology handler, history/CRT publication,
resource authority and native Unit creation remains the next integration step.
The CPU fixture tests do not establish original-game pixel equality, hardware
vsync performance, or native macOS/Windows rendering certification.
