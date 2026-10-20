# Main-map exploration, disclosure and startup zoom

For the pinned GOG `terran.exe` SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Numeric cases](main-map-exploration-and-startup-zoom-cases.json) contain ordered
terrain tiles/anchors, unknown outlines, admission records, pixel hashes,
constructor/geometry controls and the six-field view codec.

**Port action:** apply the viewing faction's exploration mask to the main map.
Standard rules do not expose the whole geography. Unknown cells have a black
background and dark-blue outlines, even when the ordinary known-terrain grid
toggle is off. At a 1024-wide fresh main view, use zoom0 and nominal96×48 tiles.

## State owners and readers

| Authority | Original layout | Reader / meaning |
| --- | --- | --- |
| Tile exploration | byte `tile+4`, bit `1<<viewer`;44-byte map record | `46fd00` terrain and `473290` / `4737b0` object tile admission |
| Remembered improvements | DWORD `tile+12+4*viewer` | Normal positive-viewer terrain item selection at `46ff66` |
| Live improvements | DWORD `tile+8` | Negative viewer or faction reveal `0x200` selects live items at `46ff6c` |
| Unit stored disclosure | byte `unit+0x27`;52-byte record at `9951b0` | Foreign/exceptional-own unit check at `4736f7`, against global display faction `9430d4` |
| Base stored disclosure | byte `base+0x0a`;304-byte record at `96f1b0` | Foreign base check at `473875`, against global display faction `9430d4` |
| Faction reveal | DWORD `95d1a8 + viewer*0x20bc`, bit `0x200` | Bypasses tile exploration and selects live terrain items; does **not** bypass foreign object disclosure |
| Editor reveal | byte `959fc8`, bit `0x80` | Bypasses the tested terrain and object disclosure gates |

The terrain viewer argument and the global display faction are distinct inputs.
The captures use3 for both. Different-input behavior is source-traced, not newly
executed here. Viewer0 aliases another tile field in the remembered-item expression;
do not use earlier viewer0 geometry fixtures as a human knowledge-mask contract.

Exploration and remembered improvements have their existing producer/lifetime
contracts in [ordinary exploration](ordinary-land-exploration.md) and
[initial startup reveal](first-worker-map-producers.md). Previously explored
tiles remain explored; ordinary sight refreshes remembered improvements.
The terrain readers above do not consult a per-frame current-observer-radius mask.
This does not establish that every terrain feature reads only remembered state.
Individual feature selectors retain their separate contracts.

Unit stored disclosure is event-updated and can survive an observer leaving.
Use [native visibility lifetime](native-visibility-lifetime.md) and
[foreign land visibility](foreign-land-visibility.md); do not rebuild all object
disclosure masks from current observer radii every frame. This receipt adds
consumer checks, not new detection, diplomacy, base-discovery, or save/load lifetime
proofs. Minimap admission remains a separate reader.

## Standard unknown terrain and survey override

`5b5760` returns stock standard rules `0x1a0e`. Bit `0x100` is **clear**.
The custom-rules checkbox "No Unity Survey" is inverted: when its bit is clear,
`5a13e4..5a13f2` sets rules `0x100`, enabling survey geography. Do not interpret
rules `0x100` as "No Survey enabled."

Without editor/reveal, when survey is disabled and a tile is unexplored,
`47072f..47077b` jumps to `4730be`, bypassing the normal terrain/features pass.
The full ordinary repaint clears its destination to palette65 (`0x41`) at
`47651e..476525`, subject to its documented special/incremental mode guards.
The unknown continuation draws four lines per admitted tile in palette194
(`0xc2`). With the captured palette these are:

| Index | RGB |
| --- | --- |
|65|0,0,0|
|194|13,28,48|

This special outline does not depend on normal grid bit2. Flags1 and3 produce
identical complete images in the paired control. The stock-flags control uses
`0x60010ee3`, rules `0x1a0e`, and also produces only these two colors.

**Keep relief in unknown outlines.** The continuation queries `47ca10` and
subtracts signed cached corner heights before drawing. For example, the first
fixture outline is `(-48,0)→(0,-24)→(48,-6)→(0,18)→(-48,0)`.
Its halves span48 pixels horizontally; vertical deltas need not all be24.
Use the existing [viewport geometry/cache rules](viewport-geometry-lifecycle.md).
These captures replay measured line geometry through the independent indexed-line
reference; they do not newly derive all scheduled geometry independently.

With survey bit `0x100` set, unknown cells instead enter the terrain raster with
the native blank/survey local set1. The traced setup zeros moisture/rockiness,
clears selected item bit `0x20`, and modifies render metadata. It is a specific
survey style, not permission to use every live improvement. The eight controls
do not enumerate every possible survey feature/terrain combination.

## Object admission

With an active game, ordinary display flags `0xc0`, no special selected/blinking
actors, a valid supplied lookup and separate draw sinks:

| Foreign tile/object state | Base and unit admitted? |
| --- | --- |
| Unknown; stored object bit clear or set | No |
| Explored; stored object bit clear | No |
| Explored; stored object bit set | Yes |
| Unknown + faction reveal `0x200`; stored object bit clear | No |
| Unknown + editor `0x80`; stored object bit clear | Yes |

Own base/unit controls on explored tiles admit without the stored bit.
Source edge: an own unit whose `word+8 & 0x440 == 0x440` falls through to the
stored-mask check. That status combination is source-only here. Selected,
blinking, animation, base-occupancy, special renderer flags, stacked selection
and name-label paths have additional branches; the simple table does not replace
those branches. The unit chosen by a roster lookup is supplied in these controls.
Factories, actual roster membership and final unit/base pixels are not executed.

## Startup scale and persistence boundaries

`46f2a0` initializes view zoom from global client width `91f3dc`:
width below1000 gives−6; width1000 or above gives0. Eight original controls
cover widths800/999/1000/1024 and types0/1. This is a constructor boundary.

The normal view-window initializer `47d630(mode0,...)` subsequently writes
`+1dd70=0x10ee3` and `+1dd98=(type==0 ? 0 : -8)` at `47da7f..47da96`.
Thus the fresh 1024-wide main view has zoom0 at both stages. Do not infer an
800-wide final main zoom−6 solely from the constructor experiment. Special
initializer modes write−7. The source restores the mode argument at `47da1e`;
its earlier temporary string-copy use of ESI does not change that decision.

`47dd60(reset)` calls the constructor only when reset is true, but calls the
normal window initializer for main slot0 and active auxiliary slots. The
cold/resume selector reaches it through `53f97d → 5a57e0 → 47dd60`.
This is static call-flow evidence plus an executed initializer store fragment,
not a complete natural fresh/resume display run.

The native `5be020` codec transfers these six DWORDs, in order:
`type(+1dd78), active(+1dd74), flags(+1dd70), zoom(+1dd98), cameraX(+1dd9c),
cameraY(+1dda0)`. A memory-FILE write/read control restores
`[0,1,0x60010ee3,-6,22,38]` exactly. Executing the normal main-view initializer
store fragment afterwards sets that zoom to0. **Ordering matters:** this proves
that zoom is serialized and that the initializer can overwrite it; it does not
prove the final visible zoom for every natural load path, whether a later codec
restores it, or every INI/preference source. No claim that saved zoom always wins
or always loses is made. No standalone persistent zoom preference is established.

`46f3b0` computes geometry using signed division truncated toward zero:

```text
n = trunc(50*(zoom+16)/16)
if scale_numerator > 0:
    n = trunc(50*scale_numerator/scale_denominator)
q = max(1, trunc((n+1)/4))
tileWidth=8*q; tileHeight=4*q
halfWidth=4*q; halfHeight=2*q; quarterHeight=q
```

The scale globals are `6b0114` / `6b0118`. Their positive-numerator override is
shared with other UI rendering contexts; it is not evidence of a saved startup
zoom setting. The nine extra prefix controls include zero/negative numerator,
ratios2/1 and1/2, zoom−16/−17 clamping, and zoom1. Denominator0, arithmetic
overflow and arbitrary out-of-range zoom are outside the safe tested profile.
At zoom0 the fields are96×48, half48×24 and quarter12; at−6 they are64×32.

For a 1024×768 screen, `5327e0` allocates the map/root rectangle with height
`768-consoleReservedHeight`. The ordinary console setup reserves213, giving a
nominal map canvas1024×555. The console artwork begins at y511 and can overlap
44 pixels of that nominal canvas. A1024×555 native-bitmap control is included;
no desktop occlusion/window-composition proof is implied.555/48≈11.56 nominal
vertical tile heights is consistent with roughly12 cells, not an exact screenshot
cell-count test. The1024×511 controls remain useful for the unobscured upper area.

## Finite evidence and reproduction

Eight raster controls use the installed80×80 Planet map, camera22,38, viewer3,
zoom0 and supplied knowledge/flags/rules. The21-tile replay supplies the already
proven startup footprint; it does not execute the startup producer again.

| Control | Size | Terrain calls | Normal terrain admissions | Unknown lines |
| --- | --- | --- | --- | --- |
|Unknown, flags1 / flags3|1024×511|288 each|0|1152 each|
|Unknown, survey `0x100`|1024×511|288|288, blank1|0|
|Unknown, faction reveal / editor|1024×511|288 each|288, blank0|0|
|All3200 explored|1024×511|288|288|0|
|Supplied startup21 explored|1024×511|288|21|1068|
|Unknown, standard rules/flags|1024×555|311|0|1244|

Fully unknown511-high frames contain500461 black and22803 blue pixels.
The555-high standard frame contains543573 black and24747 blue pixels.
Independent line replay matches all1614848 pixels of those three frames.
Other five frames have complete indexed hashes and admission checks, not an
independent full terrain-raster implementation. Original CPU raster executes;
generic presentation services and empty object-roster/draw boundaries are supplied.

Verification: `tmp/reference/check_unknown_map_visibility.py` checks84 literal
instructions,8 raster receipts,14 object controls,8 constructor controls,9
geometry/override controls and6 codec fields. It reads the pinned installed PE
and Planet asset, and never changes foreground state. Frozen evidence lives in
`tmp/reference/main-map-exploration-startup-zoom-evidence-20261019/`; its manifest
pins source scripts, receipts, private palette/pixels, public cases and dependencies.
No executable or installed game-asset payload is copied into the bundle.
