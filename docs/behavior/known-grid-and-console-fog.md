# Known terrain grid and console fog admission

For the same pinned GOG `terran.exe` as the
[main-map contract](main-map-exploration-and-startup-zoom.md).
[Cases](known-grid-and-console-fog-cases.json) and an
[independent geometry reference](known_grid_reference.py) cover13 full map paints,
21 tile-admission/grid-continuation controls, and8 complete console selection
repaints. No foreground or natural-startup work is involved.

**Correction: the known terrain grid switch is global preference
`959f98 & 0x100`, not view flag bit2.** Handler `529500` XORs that preference,
updates its menu state and requests a map repaint. View flag2 affects two spacing
fields in `46f3b0`; changing it does not alter any of the four paired rendered
frames here. The earlier stock-flags partial21 receipts contained **no known-grid
line calls**: their only line calls were the unknown continuation. Their flags
`0x60010ee3` do not alone enable the known grid.

## Known-grid admission, geometry and ordering

The grid continuation is `47147f..47160b`, within each known terrain tile's
feature continuation. It uses the four already constructed ordinary terrain
vertices W/N/E/S. It does not issue additional height/shade queries.

- Ordinary admission requires preference `0x100` and a clear special renderer
  local (`flags & 0x80000000 == 0`). Explicit renderer flag `0x4000000` forces
  this continuation even when the preference is clear or special mode is set.
  That forced mode still does not bypass the earlier ordinary unknown admission.
- Each cell emits only **W→N, then N→E**. It does not draw all four sides. These
  two edges correspond to neighbors `(x-1,y-1)` and `(x+1,y-1)`.
- Wrap X once for the wrapping map; flat maps clip X. Always clip Y. Invalid
  neighbors produce no line. Thus a north-row cell emits no known-grid lines;
  the tested west-edge flat cell emits only N→E, while wrapping emits both.
- In ordinary mode with display option `959f9c & 0x800000` clear, a sea current
  tile emits no known grid, and a land cell omits an edge whose neighbor is sea.
  Setting that option admits those edges. Forced special mode has its own sea
  bypass as checked in the bounded continuation controls.
- If both current tile and neighbor have altitude band>=3, color is37 (`0x25`).
  If either is sea, admitted color is190 (`0xbe`). Neighbor exploration is not
  tested by this edge-color/neighbor loop; the current tile's earlier admission
  decides whether its normal terrain/grid continuation runs.

| Line family | Index | Captured RGB |
| --- | --- | --- |
|Known land/land grid|37|21,41,24|
|Admitted sea/coast grid|190|14,37,75|
|Separate unexplored outline|194|13,28,48|

The known grid reads ordinary rendered vertices: land displacement uses the
height return captured while each triangle is constructed; sea has flat ordinary
vertices. It does **not** use the unknown path's signed cache-byte/FF displacement
quirk. The reference generates the known vertices at the correct point in the
query schedule from owned map/profile/cache inputs, then derives northern edges.

Known-grid lines occur after base terrain and landmark material overlays, before
later improvements, resources and subsequent tiles' rendering. They are not a
final overlay applied to the completed map. Applying all23 first-frame grid lines
to the already completed grid-off frame disagrees with native output at40 pixels.
The public line records retain their positions within the primitive-call stream;
all are in the per-tile feature phase after that tile's terrain primitives.

The map inputs are deliberately bounded: the13 controls are three
four-frame partial sequences plus one unknown first paint. Each partial sequence
supplies21 exploration bits, viewer/display faction3, the same80×80 installed
Planet map and1024×511 viewport, zoom0/relief25, rules `0x1a0e`, camera22,38 →24,38
→22,38, and empty rosters. The sequences are preference0/stock flags,
preference100/stock flags, and preference100/stock flags with view bit2 cleared.
The unknown first paint sets preference100 with no exploration.

Grid-enabled first and repeat paints each emit23 known lines; neither narrow
east/back dirty strip contains known cells, so their zero new known lines rely
on retained grid pixels. Grid-off paints emit none. All four view-bit2-clear
frames match the enabled counterparts byte-for-byte. The unknown control matches
the prior unknown first frame exactly. Grid-off partial frames match the previous
four partial21 native frame hashes exactly.

Across13 controls, the model matches **43,104 outer geometry queries/returns**,
all **92 known-grid lines and their full source quads**, unknown outlines and
terrain anchors, and **374,400 semantic cache bytes**. Grid does not change the
height/shade query schedule or geometry cache. Full native indexed frames have
hashes and retained image files; this supplement does not independently implement
the full known-terrain raster. Sea/coast color190 and special/context admission
are original continuation/line-argument controls, not additional full pixel scenes.

## Viewer and context boundaries

The21 context controls execute the original terrain admission prefix and, when
admitted, the original grid continuation with an explicitly supplied flat quad.
Material/raster execution is omitted there, and line calls are observation sinks.

Positive viewer3 requires exploration bit8; viewer4 requires16. Negative viewer
bypasses that ordinary tile-knowledge test. Faction reveal `0x200` also bypasses
it for its viewer. The bounded survey-bit control enters the blank/survey terrain
path and can reach this grid loop; that does not generalize the survey raster or
its whole feature schedule.

**Editor bit80 is a caller boundary for terrain:** an unknown tile passed directly
to `46fd00` with a positive viewer still takes its unknown path in the new control.
The normal main painter translates editor mode to viewer−1 before calling terrain
(`4773b0..4773c3`). The console similarly passes−1 for editor preview. Earlier
full-editor frame results describe this complete caller behavior, not a positive
viewer terrain-reader exception. Object disclosure readers also have their own
direct editor bypasses, as previously documented. Viewer0 is a diagnostic: its
remembered-items expression aliases the map's landmark word and is not a normal
positive human faction-memory fixture.

## Console selected unexplored tile

The original normal console selection repaint is `4cb9f0`. Its terrain-card
admission at `4cbf41..4cbfa7` is:

```text
editor bit80 OR viewing faction reveal bit200 OR tile explored by display faction
```

When false, it obtains the actual "Unexplored" label from the loaded string table,
centers it in the card, and jumps from `4cc01e` to `4cda3d`. This skips the terrain
preview call and ordinary elevation/moisture/feature text. The normal tail still
prints the selected tile's coordinates. It does not choose a fallback tile or
render an unknown terrain preview. **Unity Survey alone does not admit the card.**

The eight complete original console repaints select `(8,8)` in an explicit high
land / fungus / nutrient-resource fixture with no units or bases:

| Selection state | Preview calls | Actual text |
| --- | --- | --- |
|Viewer3, bit8 set|1|Elev:2590m; Flat & Moist; Xenofungus; Nutrient bonus; coordinates|
|Viewer3, unknown|0|Unexplored; coordinates|
|Viewer3, only viewer4's bit16 set|0|Unexplored; coordinates|
|Viewer4, bit16 set|1|Same terrain details; coordinates|
|Unknown + faction reveal200|1|Same terrain details; coordinates|
|Unknown + editor80|1|Same terrain details; coordinates|
|Unknown + Unity Survey100|0|Unexplored; coordinates|
|Unknown + alternate-detail field1|0|Unexplored; coordinates|

The unknown label's captured console-local text origin is `(159,129)`, and
coordinates `(8 , 8)` are at `(202,179)`, using the current-host font metrics.
The admitted preview uses renderer view0, zoom0, flags `0x621`, viewer3/4 normally
and−1 for editor. Its scale overrides reset to−1 afterwards. The
[existing preview contract](lower-console-terrain-preview.md) specifies its flat
85/100 geometry; that raster proof applies only after console admission.

Native label selection, formatting, layout and the complete console routine
execute. Terrain, frame/body/presentation rendering boundaries are sinks; final
console pixels and historical font parity are not newly proved. The source calls
the terrain-card outer/inner frame painters before branching on knowledge. In the
port, clear/replace the old preview and detail text when selection becomes unknown;
returning without removing the prior contents would leave an information leak.
Do not let a fallback terrain renderer or elevation formatter bypass this admission.
Foreign object cards and selected-actor special paths retain their separate rules.

Run `tmp/reference/check_known_grid_console_admission.py`. It checks37 source
literals and all three evidence groups. Frozen evidence lives in
`tmp/reference/known-grid-console-fog-evidence-20261020/`, with its manifest,
native traces/images/cache files, the public cases and independent reference.
