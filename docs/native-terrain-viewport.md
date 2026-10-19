# Live native terrain painter integration

`--native-terrain-inspector` connects the measured SMAC ordinary land pipeline to
an actual loaded standard Planet campaign. Use a1024x768 viewport; the inspector
starts and holds native zoom0 (96x48 diamonds). Camera panning, minimap recentering,
object selection and ordinary game commands still use their existing live paths.
Other viewport sizes hide the native layer until1024x768 is restored. This is an
explicit rendering inspector which can reveal terrain, not a completed observer
fog implementation or the default production renderer.

The backend publishes a detached3200-record rendering snapshot through a
world-ticketed MT request. It copies current Former/Fungus state and retained
altitude bands/topology. Cancelled, stale and retired requests cannot publish
another world's terrain. No mutable Tile/Map pointers, GSE objects or gameplay
RNG cross into the native painter. Terrain/tile updates coalesce refresh requests;
camera redraws use the owned frontend snapshot. Destruction retires the actor
before releasing its texture; Mesh owns its quad.

The immutable indexed atlas comes from owned `texture.pcx` and `palette.pcx`,
with source index+10, common gamma100 palette and the measured software reserved
colors. The painter connects ordinary height fans, signed lighting, Base/Rock/
Forest/Fungus material selection and the original integer indexed raster. A
separate actual-store mask preserves opacity when a non-key source shades to9;
keyed holes remain transparent. The GPU layer sits above old terrain and below
existing improvements, Bases, Units and labels. Painting runs only when the
camera or snapshot changes, not on idle/vsync frames.

`--native-terrain-base-low16 N` supplies decimal0..65535 for matching a measured
original tile allocation base. Default0 explicitly corresponds to the controlled
reference base0x40100000. Natural Windows allocation low16 remains unmeasured;
this convention is not a claim of identical variation across arbitrary original
runs. It does not consume either random stream.

## Boundaries still to close

- Roads and further land continuations.
- Named landmark deformation/shading and neighboring affected stencils.
- Original observer/fog admission, complete object occlusion and animated water.
- Full native view/camera-origin/scrolling behavior and other zooms/viewports.

Unsupported centers retain the existing GPU terrain through transparent holes;
this mixed scene is not whole-map pixel parity. The independent `View` helper
matches1597 original ordered tile/F/record-offset submissions at six fixed
camera contexts. The live integration currently uses the existing camera matrix
and row order so terrain aligns with current live objects; that does not prove
arbitrary original scrolling phase or dirty-region redraw behavior.

## Relevant validation

- Twelve focused native groups pass with allocation tracking, covering the owned
  snapshot after Map destruction/current changes; stale/retired/cancelled tickets;
  loader-progress exclusion;1597 view submissions; key/shade coverage; connected
  frame ownership; prior indexed, camera, material and palette contracts.
- The27 original indexed polygon frame hashes still match production C++ using
  the owned atlas. Those prove the raster, not this entire mixed scene.
- Focused ASan/UBSan exercises ten complete painter views across flat/wrapped
  horizontal and polar boundaries plus malformed record offset rejection.
- Targeted correctness analysis covers changed production translation units.
- Actual1024x768 software-rendered owned developed campaign: Load Game,
  minimap recenter to two areas, native atlas admission and ordinary menu Quit.
  Quit returns zero with no tracked OpenGL leaks. Audio is disabled; this is not
  hardware-vsync/audio performance acceptance. Captures remain external.

## Water/coast and sea Fungus continuation

The live inspector also copies the retained contour byte and paints ordinary
water/depth sectors, land coast masks and shelf Fungus. Coast masks are generated
from the owned indexed RAINFALL.PCX; no original pixels are embedded in this repo.
Sea Fungus uses its separate atlas bank and keyed unlit shader after the water
sectors; existing land Fungus remains before coast. The snapshot owns its fields
and the atlas cache owns its mask, with no retained backend tile pointers.

The measured profile is documented in [water/coast](behavior/ordinary-water-coast.md)
and [sea Fungus](behavior/ordinary-sea-fungus.md). Production command checks cover
54 contexts,250 continuation commands and760 contour samples; raster receipts cover
60 complete indexed frames and7714 spans. To replay the production C++ against
owned original assets (loaded pixels and output stay in a temporary directory):

```
python tools/reference/verify_native_water_coast.py --texture-pcx PATH/TEXTURE.PCX --rainfall-pcx PATH/RAINFALL.PCX
```

Add `--sanitize --cxx clang++` for the focused ASan/UBSan replay. This is static
terrain material composition, not animated water or whole-map pixel parity.

### Retained landmark metadata admission

The live map capture exposed an overly broad landmark filter. The retained word
contains dirty bookkeeping and a shared upper detail byte, as well as membership
bits. Land geometry now checks the source-proved0xb03 profile mask without a
blanket negative-detail exemption. Metadata alone no longer rejects tiles or
their neighboring geometry stencil. Unsupported material memberships remain a
separate admission decision; sea0x10/Freshwater0x80 and the land-only0x40 base
branch do not alter this ordinary sea profile. Special-art1/2/0x100 stays excluded
on sea even though sea heights are zero. The original retained word is unchanged.

See [87 geometry controls](behavior/terrain-landmark-field-admission.md) and
[27 complete-renderer sea controls](behavior/sea-landmark-material-admission.md).
These clarify the existing flags1/zoom0 inspector; they do not implement other
landmark profiles, optional object overlays or dirty-only redraw scheduling.

## River continuation

The inspector now uses the explicit ordinary display profile0x401: base painting
plus River flag0x400. The River pass reads live item0x80, uses four diagonal live
neighbors (including River-bearing sea neighbors), paints the native shaded quad
after coast, and repeats mask0 for retained raw item0x200. Sea centers draw no
River and can keep their ordinary water prefix even when the River item is set.
Disabling0x400 suppresses River without erasing the underlying material. This
extension does not imply support for any other arbitrary display-flag combination.

[Original rules and cases](behavior/ordinary-river.md) cover33 contexts and37
composed indexed frames; the earlier alleged river/color-map branch is territory
coloration and remains separate. The production C++ replay uses owned assets:

```
python tools/reference/verify_native_rivers.py --texture-pcx PATH/TEXTURE.PCX --rainfall-pcx PATH/RAINFALL.PCX
```

Add `--sanitize --cxx clang++` for the focused indexed-raster replay.

## Territory-boundary continuation

The inspector's ordinary profile is now 0x10401: base materials, River 0x400,
and territory 0x10000. It copies current worker-knowledge claims and topology
regions, the actual local simulation viewer's directed relations, and the
accepted runtime faction artwork filenames into an owned snapshot. Bases and
turn advances invalidate the snapshot. It does not infer claims from occupancy
or substitute a stock faction order for the accepted roster.

The palette stage samples the owned PALETTE.PCX and SYSTEM.TXT boundary defaults,
then applies each accepted faction PCX's boundary color and opacity metadata.
The indexed renderer applies the original owner lookup to the already-painted
destination, before land coast and River. Identity-valued stores still count as
coverage. Source caches distinguish runtime faction artwork assignments.

[Territory reference](behavior/territory-boundary.md) covers 31 producer contexts,
154 complete commands, 43 composed indexed frames and 4096 lookup entries. Replay
with owned installed assets:

```
python tools/reference/verify_native_territory_boundary.py --asset-directory PATH
```

Add `--sanitize --cxx clang++` for focused ASan/UBSan. The live inspector deliberately
reveals terrain; it does not implement original fog admission, arbitrary display
flags, all diplomacy refresh events or original cache/hot-reload timing. Complete
native map/camera and unsupported landmark/road continuations remain separate.
