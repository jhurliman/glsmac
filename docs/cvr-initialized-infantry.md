# Explicit initialized Infantry Scout rendering

The `InitializedScout` descriptor selects the initialized, reactor-1 Infantry /
Gun / Scout-armor idle composition. It loads `vi.cvr`, then `viptr00.cvr`, plus
`PALETTE.PCX` and `system.txt` through the existing installed/mod resolver. The
descriptor contains a fixed logical composition, view and placement values;
it cannot choose file paths. It does not change default unit definitions or
spawn a starting Unit.

The existing baseline Scout remains composition 0 and the Spartan Rover remains
composition 1. Initialized Scout is composition 2 with its own appended outer
render tag and script type `cvr_initialized_scout`. The self-contained descriptor
version remains 1; the outer tag and inner composition must agree. Existing
serialized values and their behavior are preserved. A reader predating this
variant rejects its new tag. The existing 256-byte outer frame cap and strict
descriptor validation still apply; no world/settings version changes occur.

## Geometry, lighting and palette ownership

The explicit initialized profile retains the Infantry attachment offset,
scene frame selection, per-part shade tables and Infantry scale multiplier
1.25. Palette presence never chooses geometry. The installed map-light source
must contain exactly the already-supported `6,4,15` tuple; the renderer derives
its normalized direction using the existing checked view producer. Unsupported
light sources reject rather than silently selecting another illumination model.
The bounded parser is shared with the unchanged Rover loader.

Final RGB comes from the software main palette: its first 236 PCX colors are
installed at indices 10 through 245, with the observed constructor's 20 reserved
colors retained. This reuses the existing bounded PCX decoder and software
palette construction, now exposed separately from faction-material generation.
No faction PCX or actor tint table is required for initialized Infantry. Both
Infantry scene shade tables remain unchanged. RGB is copied exactly, without
quantization, low-bit expansion or an inferred correction from the scene RGB.
Index 9 controls the existing host RGBA transparency conversion; neither depth
nor palette flags serve as alpha.

The loader owns all decoded scene, geometry, shade, palette and lighting data
before returning. Each render produces independent full 256x256 indexed, depth
and RGBA buffers. The scene cache stores this renderer separately and keys every
admitted pixel-changing view field together with the composition. All variants
share the existing 64-view budget; placement remains a separate key. Retained
sprites and registered previews retain the generated texture owner across cache,
manager and presenter teardown. Failed replacement loads leave old owners intact.

The conventional Unit admission accepts the exact initialized-tag/composition
pair for the existing supported Infantry context. It does not widen Speeder or
faction/design eligibility. Real Unit creation still uses the existing checked
simulation and social context. Facing, scale, center, size and morale mode remain
explicit caller inputs; this change does not infer startup or UI defaults.

## Original evidence and comparison boundary

Human-owned `RE-SMAC-INFANTRY-MATERIAL-001` starts a fresh original process,
runs constructors, binds the main palette, executes the graphics palette
initializer prefix using actual `PALETTE.PCX`, loads 107 models, performs full
dataset setup `(0,1)`, creates actual design 2, and reaches its internal idle
draw. This is a stronger initialized caller boundary than the earlier baseline
renderer receipt. It does not establish final surface blitting or a complete
interactive game.

The nine retained drawings include the six ordinary Infantry faction owners,
a facing-0 case, zoom 8, and an actor-remap table overwritten with constant 252.
All six owners have identical indexed/depth output; the remap control leaves
the drawing unchanged. This establishes a faction-independent initialized
Infantry path for this receipt, not six guessed faction materials. The active
main RGB differs from the scene asset palette in the low two bits on 224 of
403 visible baseline pixels; the production conversion uses the actual main
palette without modifying either source.

Pinned inputs (research reports and proprietary assets are not committed):

- Executable:
  `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- `docs/behavior/initialized-infantry-materials-cases.json`:
  `9d24d7949babf7c92c67737ac0b4e5c9ff0d9f19770bad47e3b0dc2a02f2d1b8`.
- `tmp/reference/initialized-infantry-materials-original-x86-results-20261017.json`:
  `a9eaf014607bf7aaf338ae4d30d8f52de49a7145eea01a967667025eea5318c1`.
- `vi.cvr`:
  `d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1`.
- `viptr00.cvr`:
  `572ca9d55ad49c2ab32a90ae8d3e1b1bc9a78e7901cf4387d26d0dde205a4cee`.
- `PALETTE.PCX`:
  `54341b1bee31736997950e432ff3312a620f7cf722016c154098a4c78d28f866`.
- `system.txt`:
  `cadbe5153d3e36fbb95638f6aec54ba929b6f60af56e782b38042fb835cb146f`.

`tools/reference/compare_initialized_infantry.py` verifies these pins, original
result status/order and literal expected/actual equality before invoking the
optimized production probe. Only the four installed assets and public facing,
zoom, ratio and anchor enter that probe. It compares all nine complete indexed,
depth and main-palette RGBA images plus anchors, and independently reconstructs
key-9 RGBA from the generated software RGB palette. The full generated RGB hash is
`d40184d13f25209e08bd29a4d627eae51ee52085078b044fb5009a4a14db76ef`.

The original submits 59 scene-frame records and executes 65 raster kernels per
drawing. The port emits 65 grouped/simple raster draws, processes 47,483 voxel
records and records its write count separately. These counters are not treated
as interchangeable: complete image hashes are the comparison boundary. For
facing 2 / zoom 0, the owned RGBA hash is
`ba57ad59fa682ed8a73eb35a07bfc72f3c587a5ce0e2633087db0e6c26e793fe`.
The retained PNG is a lossless encoding of that buffer, with no crop, filtering
or other pixel transformation, and stays outside Git.

## Validation

Source parent: `b7f6aad37f55aa5cf73073f0e18e4b1bc97d5b7c`, the separately reviewed
explicit geometry/profile policy split. No default script changes are included.

Four added native groups cover owned initialized pixels/view policy, malformed
palette/light rejection, separate wire/cache identity with failed replacement,
and actual GSE definition -> backend conventional Unit -> frontend refresh ->
registered retained preview teardown. The latter uses real UnitManager and
preview callbacks with Null graphics and narrow synthetic badge resources;
it is not an interactive rendered-scene test. An opt-in run with
`GLSMAC_INITIALIZED_SCOUT_ASSETS` uses the installed CVR/palette/system assets
through the same actual Unit/preview path.

- Full normal CTest: 8/8, 85.83 seconds.
- Full leak-enabled ASan/UBSan/LSan CTest: 8/8, 161.16 seconds.
- All 27 focused CVR groups pass with allocation tracking in both builds.
- Installed-asset actual Unit/registered-preview tracking passes in normal and
  leak-enabled ASan/UBSan/LSan builds.
- Optimized UBSan production probes match all nine initialized drawings and
  their metadata exactly. Existing comparisons remain exact: baseline Scout
  19 drawings / 20 metadata; Rover 15 materials / 19 drawings / 20 metadata /
  68 detail queries. No original executable was rerun for this port validation.
- Three copied production controls fail the intended native assertions:
  substitute scene RGB, retain baseline lighting, or alias the baseline cache
  key. The intended checkout and application binaries are never mutated.
- Full header-impact LLVM15 analysis: 337 production translation units,
  zero failures, 252.75 seconds. The Windows-only network implementation is
  outside that Linux scan.

Private logs, controls, hashes and comparison results are retained under
`/home/jhurliman/src/glsmac-initialized-infantry/`. Existing arithmetic admission,
host transcendental/cross-CRT limits, clipping and target-copy boundaries remain
those documented by the [Scout renderer](scout-idle-renderer.md).
