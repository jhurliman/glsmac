# Unity Survey through the complete main viewport painter

Eight full native frames now cover all-unknown and partial21 Planet, each with first/repeat/east/back. The independent reference derives every outer projector/height/shade query, projected tile anchor, object-pass enumeration, nine semantic cache bytes per record, pan regions and retained-backing operations. The raster verifier independently generates all polygon spans and source sampling from the actual primitive inputs and owned art. This is not an independent material-command producer: production should derive commands from the supplied map/context and use the receipt as expected output.

[Compact production controls](unity-survey-production-cases.json), [query/cache reference](unity_survey_viewport_reference.py). Full primitive traces and pinned disassembly remain in the separately verified frozen archive rather than being duplicated in source Git. Separate frozen evidence: `tmp/reference/unity-survey-viewport-evidence-20261020`. Earlier partial21, grid and console bundles remain unchanged.

## Inputs

Use the exact [partial21 material fixture](partial21-material-survey.md), with rules changed from `0x1a0e` to **`0x1b0e`**. The imported resource pattern remains27067; map address remains`0x4085bf60`, variation base low16`0xbf60`. Viewer/display3, status0, flags`0x60010ee3`, zoom0,1024×511, empty entity lists, grid preference0, display options0, editor0 and inactive scale override are unchanged. Global lighting at **`6a9324` is explicitly captured as2 before/after every frame**; the memory observer records no writes to it.

Each sequence starts with cold native view cache, camera`(22,38)` and backing73. The all-unknown input clears every exploration byte. Partial21 grants bit8 on the same21 radius cells while preserving Planet's authored memory, including the three nonzero remembered3 words. It does not execute a natural startup sight producer. Each sequence retains its cache, map-render metadata and backing across repeat/east`(24,38)`/back`(22,38)`.

## Native style and query order

Survey admits every geographical tile to the terrain painter. Unknown survey tile policy is blank1, full-light flag0, moisture0, rockiness0, selected remembered items with Fungus cleared, and global lighting2. Known tile policy is blank0/full-light1 with its original moisture, rockiness and selected remembered items. Geography and live-neighbor authority stay unchanged. The main painter does not silently replace unknown survey terrain with ordinary known terrain.

The terrain walk has288 cells in first/repeat,63 in east and50 in back. Full first/repeat all-unknown policy is288 survey cells. Partial21 has267 survey and21 known cells (14land/7sea). Both pan strips contain only survey cells; their repaint store streams are identical across the two knowledge sequences. Known terrain remains in the retained overlap.

For each terrain tile in existing enumeration order:

1. Call projector `46f930(x,y)`. Its internal corner queries remain part of the real native execution, but are not double-counted as outer calls.
2. On terrain entry, call height `47ca10` in order4,3,2,1 and use their arithmetic average for the anchor adjustment.
3. For land, follow fan vertices **`0,1,2,0,2,3,0,3,4,0,4,1`**. Each vertex calls height. A known land vertex then calls shade `47d330`; an unknown survey vertex instead loads lighting2 directly and makes no shade call.
4. Sea uses its ordinary sea diamond/depth path after the initial corner average; it does not enter the land fan height/shade schedule.

After terrain, the object pass projects its separately selected288/88/75 cells, even though the rosters are empty. For pans, prepend the native wrapper's projector plus4,3,2,1 heights for **new first tile, then old first tile**, before the ordinary partial walk. The independent scheduler regenerates this sequence and actual return values from Planet and cache state.

| Frame position | Projector | Height | Shade, all unknown | Shade, partial21 |
|---|---:|---:|---:|---:|
| first |576|3492|0|168|
| repeat |576|3492|0|168|
| east |153|956|0|0|
| back |127|712|0|0|

No unknown blue boundary lines are submitted in any control. Survey admission is distinct from ordinary exploration, and does not authorize the lower-console terrain card; the existing console-fog gate remains applicable.

## Cache and retained pixels

All nine semantic bytes per3200 records match the independent geometry/cache model after every frame. Total changed native cache bytes:

| Sequence | first | repeat | east | back |
|---|---:|---:|---:|---:|
| all unknown |1964|0|170|0|
| partial21 |2092|0|170|0|

Known land shading populates face-cache entries; survey land does not request face shading. Both sequences render identical first/repeat images and recover that image exactly after east/back. This differs from the earlier non-survey partial21 cold/warm control; do not reuse its expected difference as a rule for survey.

Full frames clear to index65 before painting. Horizontal pans self-copy96pixels, fill the exposed96×511 strip with index0 and execute the native zero-height second fill. They then repaint only the selected strip, retaining known and survey overlap. Geometry/cache history is retained rather than reconstructed from a fresh full repaint. Exact full-frame parity is verified with indexed values, including index0 versus65.

Terrain rendering also sets landmark metadata bit`0x200000` on survey tiles and clears it on known tiles. The checker derives these exact map mutations across all four frames. It verifies that exploration bytes, authored memory, live improvements and all other map bytes remain unchanged.

## Additional alternate-shader rules exposed by the viewport

The [earlier single-tile profile](partial21-material-survey.md) already pins absolute even-Y black index65, textured odd rows, keyed/coast transparency and the alternate opaque one-pixel source lag. Full coast/depth variation exposes two further rules:

- **Both coast modes ignore ordinary terrain clamp globals.** Alternate `63e880` and its full-color `6393d0` delegate interpolate the supplied values. The earlier frozen prose inferred a clamp from captured globals and was wrong. The [direct discriminator](survey-coast-clamp-and-clipped-opaque.md) supersedes that claim. Odd-row coast mapping still chooses preserve0, shaded source1 or literal mapped color>1; even rows write65 only for nonzero mask mappings.

- **Alternate water `63ef70` performs an initial whole-polygon rejection if every supplied vertex is>=threshold16.** Otherwise its odd rows retain the per-pixel signed shade<16 test, but even rows write65 for the entire covered span without that per-pixel threshold test (`63f4ab..63f50d`). Preserve the initial rejection; do not apply the odd-row threshold to black rows.

These are specific native alternate paths, not a global overlay applied after ordinary rasterization. The new checker composes these changes without modifying the previously frozen primitive-control checker. Both full and alternate rendering continue to use the common palette.

## Exact indexed frame hashes

| Sequence/frame | SHA256 |
|---|---|
| unknown first/repeat/back | `875933471fceafa73e872bc19e092c2ea53612878d40743c8da607a449bda3f7` |
| unknown east | `087787e972791ec2f7866fd4d0e3954c6d4c8318809555802621b831a0a7bdea` |
| partial21 first/repeat/back | `15ab35a7b3dfad20630006db5595b98e3aec4b671121f39d112d51ba0e8da60b` |
| partial21 east | `a9d31863346040c7998ab4a7444d69f7ec324d2d9dcaabd5a01b34a18e1b3c0a` |

## Validation and limits

From repository or frozen bundle root, with installed owned assets and Python dependencies:

```
python tmp/reference/check_unity_survey_viewports.py --verify-only
```

Checks pass for20504 independently derived outer queries,230400 semantic cache bytes,4186112 complete indexed pixels,6666162 native stores,6632 material primitives/2480050 material stores,217 owned-art atlas bindings and92 pinned instruction assertions. Native ordered streams are retained; independent polygon/sprite/line/fill stores match their exact order. Self-scroll compares the complete offset/value set and resulting pixels because native CPU word-copy order differs from the row-major reference.

This is bounded native main-painter execution using supplied exploration/rules and generic OS/window/memory services, with empty entities. It is not a foreground screenshot, natural launch, complete current-observation search or whole-engine independent material-producer claim. The source/query/cache reference and raw map/context are intended to connect the port's real renderer; captured command lists remain comparison data.
