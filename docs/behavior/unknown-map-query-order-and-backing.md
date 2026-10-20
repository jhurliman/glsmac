# Unknown main-map query order, cache lifetime and backing

This extends [exploration/startup zoom](main-map-exploration-and-startup-zoom.md)
for the same pinned GOG `terran.exe`. The new
[independent scheduler](unknown_view_geometry_reference.py) constructs query
order, returns, terrain anchors and unknown outlines from the installed map,
owned profile tables, camera, exploration and retained cache. Captured lines,
query arguments/returns and cache bytes are **not** inputs to that scheduler.
[Cases](unknown-map-query-order-and-backing-cases.json) bind two four-frame
sequences: unknown and supplied21 explored, each first/repeat/east/back.

**Port action:** choose the unknown continuation before scheduling material
height/shade work. Unknown cells still have geometry queries, but never execute
the normal land-material shade schedule. Preserve the view cache across pans
and repaints. Do not warm it with hidden terrain's material pass or a whole-map
preparation pass.

## Exact ordinary schedule

Vertex numbers are `0=center, 1=W, 2=N, 3=E, 4=S`. For each selected terrain tile:

1. Call its projector. Internally it requests heights **4,3,2,1** and returns
   the projected anchor including the arithmetic-shifted average displacement.
2. The terrain routine requests heights **4,3,2,1 again**, before deciding the
   ordinary unknown/known raster branch.
3. With standard rules and an unknown tile, request heights in this order:
   **0,1,2, 0,2,3, 0,3,4, 0,4,1**. There are **no shade calls**. This is the
   four-triangle table order, not merely one W/N/E/S query per outline.
4. For a known land tile, the same12-vertex table instead schedules
   **height(k), shade(k)** for each k in order. Cold shade calls may themselves
   query neighboring face corners and warm geometry caches. For known sea in
   these controls, the material stage has no such height/shade calls; the
   terrain-entry corner pass still occurs.

After all terrain submissions, the separate object pass calls the projector for
each of its admitted candidate tiles, even if the eventual object disclosure
reader finds nothing to draw. It does not share one projector result with the
earlier terrain submission. Full passes use the same candidate tile order;
incremental object regions expand the dirty terrain regions by2 on each side.
The rosters are empty here. Object disclosure remains governed by the prior
unit/base contract, not exploration alone.

Before a small horizontal incremental pan's bitmap operations, the painter does:
`projector(new first), heights4,3,2,1(new first), projector(old first),
heights4,3,2,1(old first)`. The projector's own four internal queries are additional.
Then it copies/clears the retained image and performs the selected terrain and
object work. These preliminary queries belong in cache history.

The source sites are `47665e/476697` terrain dispatch, `46fa0c..46fa3c` projector
corners, `46fe7a..46feaa` terrain-entry corners, `473142` unknown table loop,
`470a0d/470a44` known material height/shade, and `476865` object projection.
39 literal instruction guards and all three12-entry vertex tables are frozen.
Only the outer schedule is independently generated; every recursive native
instruction/call count is not independently synthesized. The35,458 recorded
height calls, including nested calls, have a separate captured-argument return
replay and final height-cache check.

## Outline geometry comes from signed cache bytes

Let `u=12` at zoom0, `P=(px,py)` be the independently calculated projector anchor,
and `a=sum(entry heights4,3,2,1)>>2`. The terrain routine reconstructs its flat
center as `(px+4*u, py+a+2*u)`.

For each of the12 triangle vertices it calls height(k), **then reads the cache
byte for k and sign-extends it**. The function return alone is insufficient.
The four outline points are the second vertex of each triangle, W/N/E/S, with
flat offsets `(-4u,0),(0,-2u),(4u,0),(0,2u)`, subtracting that signed cached height
from Y. Draw W→N→E→S→W in palette194. The reference computes these coordinates
while executing the query schedule, rather than reading final cache snapshots.

Important sea edge: the height function can return0 for sea without changing its
cache byte. A remaining FF byte is read as−1 by the outline, shifting that vertex
down1 pixel. Neighboring land queries may have populated some of those shared
corner bytes already. For example, first-paint shelf tile `(16,26)` has projected
anchor `(240,-24)` and outline `(240,0),(288,-23),(336,1),(288,25)`.
Replacing the signed-byte read with the returned height0 disagrees.

This also explains an observable cold/warm difference. In the unknown sequence,
40 shelf tiles have different outlines on the warm repeat:94 line records and
1849 final pixels differ from first paint. The repeat changes **zero** cache
bytes; it reads the cache left by the entire first traversal, while first paint
read each tile's then-current cache. In the partial sequence,83 line records
and1495 pixels differ. Do not require the cold first frame to equal a warm repaint.

## Executed sequences and independent checks

Inputs are the installed80×80 Planet map,1024×511 viewport, viewer/display
faction3, zoom0, relief25, rules `0x1a0e`, flags `0x60010ee3`, no editor/reveal,
and empty rosters. Camera22,38 pans east to24,38 and back. Each sequence starts
with a new cold native cache; its four frames retain that cache. The partial
sequence supplies the existing21-offset startup footprint around22,38; this is
not a naturally admitted faction start or another startup-producer execution.

| Sequence/frame | Terrain / object candidates | Outer P / H / shade | Native cache bytes changed | Unknown lines |
| --- | --- | --- | --- | --- |
|Unknown first|288 /288|576 /4608 /0|1964|1152|
|Unknown repeat|288 /288|576 /4608 /0|0|1152|
|Unknown east|63 /88|153 /1016 /0|170|252|
|Unknown back|50 /75|127 /808 /0|0|200|
|Partial21 first|288 /288|576 /4524 /168|2092|1068|
|Partial21 repeat|288 /288|576 /4524 /168|0|1068|
|Partial21 east|63 /88|153 /1016 /0|170|252|
|Partial21 back|50 /75|127 /808 /0|0|200|

The21 known cells include14 land and7 sea, explaining the168 shade calls.
The dirty strips in these two small pans contain no known cells. Terrain regions
are `[30,26,36,50]` east and `[9,26,14,50]` back; the second region is empty.
X is half-open, Y inclusive. Ignored region fields can retain values during a
full repaint; the incremental flag controls whether to use them.

The checker independently matches **25,112 outer queries/returns**, all5344
unknown lines, all terrain anchors/admissions, and **230,400 semantic cache
bytes**: shade bytes0..3 and height bytes4..8 on all3200 tiles after eight frames.
Full12-byte native cache hashes are retained, but bytes9..11 are not independently
modeled here. The four fully unknown frames additionally match **2,093,056
indexed pixels** from independently generated outline geometry and independently
derived copy/fill operations, with zero differences. Partial frame pixels are
hashed, not independently reconstructed by a complete terrain-raster model.

## Backing opacity and retained history

Full ordinary repaint clears the destination to index65 before unknown outlines.
It does not paint an opaque black polygon inside every unknown tile. The unknown
continuation primarily writes its four outlines, leaving interior backing intact.

For the checked east pan, the native same-bitmap copy moves source `(96,0)` to
destination `(0,0)` and clips to928 retained columns. It then fills the exposed
rectangle `[928,0,1024,511)` with **index0**. The return pan copies `(0,0)` to
`(96,0)` and fills `[0,0,96,511)` with0. There is also a zero-height fill request,
which writes nothing. Native copy and fill execute; SetRect is supplied as an
ordinary memory-only Win32 service. No presentation/raster sink replaces them.

Indices0 and65 are both RGB(0,0,0) in this palette;194 is RGB(13,28,48).
Consequently the back frame differs from the warm repeat at47002 indexed pixels
but matches it at every pixel after treating black0/65 equivalently. The model
matches the exact indexed results before that equivalence check.

Requirements for the port's composition follow from this backing behavior:

- Start a full map repaint with **opaque black**, including target alpha when
  compositing. Skipping unknown tiles on an alpha-zero texture allows an
  underneath whole-map terrain layer to show through.
- Gate terrain/material features and object/name emission using their own
  knowledge/disclosure readers. An opaque cover alone does not reproduce the
  query schedule or prevent hidden geometry from warming caches.
- Reuse incremental history only when that history belongs to the same current
  view/knowledge/display mode. Copy valid retained pixels and black-fill exposed
  strips before drawing outlines. Unknown outlines do not erase arbitrary stale
  terrain or sprite interiors.
- When a backing contains revealed/editor/another-viewer content that is no
  longer admissible, discard/fully repaint it or explicitly clear the affected
  layers. Separately retained sprite/feature layers must also remove rejected
  prior draws; omitting the next draw leaves old pixels in those layers.

These are composition requirements inferred from the executed opaque/retained
contract. The batch does not prove every original faction/editor toggle's
invalidation caller, knowledge-loss scheduling or a GPU alpha implementation.
Ordinary exploration is permanent; do not simulate unexploration every frame.
Survey bit `0x100` is excluded throughout: its prior bounded blank/survey raster
controls remain separate, without this new scheduler proof generalized to them.

Verification: `tmp/reference/check_unknown_map_schedule.py`. Frozen evidence is
`tmp/reference/unknown-map-query-order-backing-evidence-20261019/`, including the
raw trace/cache/pixel receipts and a hash manifest. No foreground desktop,
natural startup, production port code or full UI input loop is involved.
