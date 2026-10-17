# Ordinary Former terrain rendering

Completed Farm, Solar, Mine, Forest and Road work now stages an owned CPU render
payload before the Unit, terrain and knowledge transaction commits. The frontend
applies small atlas patches and sprite instance changes on its own thread.
`Texture::AddFrom` records dirty areas for the existing GPU upload path. This is
GLSMAC presentation behavior, not an original-game pixel comparison.

The existing `LandSurface` painter is reused with an explicit candidate
terraforming accessor, a private tile-sized canvas and a coordinate-seeded
presentation RNG. Neither gameplay stream is advanced. Connected Forest and
Road neighbors are repainted in coordinate order. The admitted changed tile and
affected connection neighbors are flat, feature-free land with only these five
ordinary improvements. Other terrain combinations reject before authority
writes. Solar/Mine/Farm actor descriptors use the existing source coordinates,
placement and z-order; unrelated bonus/pod/landmark instances are retained.

Coastlines copy land into water-layer atlas regions. Initial map painting now
retains those ordered water composition operations, owning their cropped source
pixels and keeping references only to Perlin objects owned by the same Map's
render modules. Before atlas transfer, each operation records its actual mask,
rotation, alpha and RNG state. Completion replays affected water canvases from
transparent pixels, including unchanged intervening operations and previously
committed neighboring land patches. This preserves existing coastal masks and
geometry rather than overwriting the water layer with an unmasked land tile.
The frontend payload contains only owned pixels and copied sprite values; it
does not reference recipes, Map state, source assets or Perlin objects.

Normal generated and restored worlds call `Map::Initialize`, regenerating this
presentation capability. No saved gameplay schema changes. Legacy debug render
dumps loaded directly through `Map::Deserialize` do not contain recipes and
explicitly reject incremental updates. Headless backend fixtures without render
resources continue to issue semantic tile updates only. No completion invokes
`LoadTiles`, `Initialize`, mesh rebuilding or a full-map atlas copy. The backend
never reads or writes the transferred atlas or its const MapState alias.

Patch/cache textures carry `TF_CPU_ONLY`, so disposal never schedules a graphics
unload. Sprite nodes and queue payloads allocate during preparation; committing
them uses existing-node transfers, erasure and swaps. Cancelling preparation or
disposing a queued payload cannot retire live sprites or mutate the atlas.

Three native CPU groups exercise the actual painter and request-copy path with
synthetic assets: all five improvements and opposite/neighbor connections;
interior plus ordered corner/neighbor coast copies and a second change; and
discarded preparation, retained bonus instances and late invalid-patch rejection.
They make no interactive/GPU or installed-asset pixel claim. Production and test
syntax checks pass; targeted production analysis is recorded in the handoff.
All three groups pass in the shared cached Linux Debug build with allocation
tracking at `4e99da48`, with normal exit. The first linked attempt exposed an
invalid expected-image fixture that passed no RNG to a Perlin operation; the
corrected fixture uses independently reset captured RNG state. The runtime
receipt is `first-session-render-tests.log` under
`/home/jhurliman/src/glsmac-rover-runtime/`. This does not yet validate GPU upload
or installed-asset pixels in a live session.
