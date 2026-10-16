# Evaluating-base context for tile yields

Tile resource callbacks can now distinguish the base evaluating a square from
its faction and from the square's current worker or base center. This supplies
missing context to the existing gameplay path; it does not activate the verified
SMAC calculators or supply technology, social, event, capability, or mirror data.
No SMAX equivalence is inferred.

## Contract

`tile.get_resources()` still selects the active player.
`tile.get_resources(player)` still selects the explicit player. Both emit exactly
`{ tile, player }` to `get_tile_resources`.

`tile.get_resources(player, base)` emits `{ tile, player, base }`. The base must be
registered in this tile's current world, and the player must be its current
owner. An absent optional argument preserves legacy behavior; an explicit null,
undefined, stale, or non-Base argument is rejected. The C++ resource methods have
the same optional Base parameter. A null C++ slot with a valid Base selects that
base's owner. Registration is checked by pointer equality before reading the
supplied Base pointer, and ownership is checked against current world slots.

This is evaluation context, not a worker-assignment or territory authorization
check. A prospective square may be evaluated without assigning it, changing a
claim, or making it the base center. Assignment validation remains in the worker
events and Base APIs. Callers must use the current world's canonical objects;
this API does not transfer raw pointers or identities between worlds.

## Production routes

- Base intake supplies the evaluating base for its center and worked squares.
- Worker scoring supplies the base whose candidates are being ranked.
- The base-screen resource panel includes its base in each tile-resources widget.
  The widget query selects that base's owner instead of the active player.
- Generic tile previews keep their existing no-base behavior.

The widget retains the wrapper until query time and checks its current type,
class, and link before unwrapping. A despawn invalidates retained wrappers; a
later redraw rejects them before a resource callback. The drawing code consumes
the queried signed values using its existing bounded presentation policy.

## Regression scope

Asset-free native tests use real Base/Pop/Tile objects, actual work/unwork Event
processing and production handlers, production base intake, and BaseManager
snapshot restore into another world. They check signed resource sums and the
exact destination Base, Tile, and owner identities. Other cases run the actual
base-screen resource layout with a recording widget factory and the widget's
production query method, plus the production worker-scoring helper. They assert
legacy callback shape, owner versus active player, base-dependent values for the
same square, preview nonmutation, and rejection before callbacks for foreign,
unregistered, mismatched-owner, and stale contexts. Rendering pixels and complete
session save/reload are outside this test's scope.

Run the focused cases in a Debug build with:

```sh
./build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^test evaluating base' --prefix build/test-data-evaluating-base
```

The native tests are also registered in the ordinary GSE suite. No original-game
assets are required.

Validation on 2026-10-16: the regression against the old production route failed
with `GSEInvalidCall: Property 'base' is expected but not found` during actual
base intake (0 passed, 1 failed, process exit 1). The completed change passes
all eight Linux Debug CTests (36.57 seconds), the three focused cases with
`--memorydebug`, and those same cases under ASan/UBSan with leak detection
enabled. Explicit analysis of Tile.cpp and TileResources.cpp reports no failures
in either Debug or Release (2.74 and 2.64 seconds). Native Windows and interactive
rendering were not rerun for this isolated slice. Local evidence is retained in
`/home/jhurliman/src/glsmac-base-yield-context/`.
