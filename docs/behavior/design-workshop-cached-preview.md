# Workshop cached main preview call and sprite recipe

2026-10-21. Source from original `terran.exe`, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Two complete native Scout controls](design-workshop-cached-preview-cases.json)
execute original art initialization and main painting, with explicit design
icon0. Their complete output surfaces are identical; yaw and shared phase
remain unchanged. The installed Scout icon is255, signed-1. Icon0 is an
explicit override fixture, not installed Scout metadata.

Private additive freeze:
`tmp/reference/independent-workshop-cached-scout-evidence-20261021/`.
Manifest SHA-256:
`c7be2bb02bfd23df7e229cf5ae0e261f5df29b093f8e6a7ab4ee0446c3357bce`.
There are30 payloads. The independently decoded sprite source and trimming
match all5,829 native source bytes. Surface hashes are native observations;
this packet does not claim an independent scaled/shadow blitter replay.

## Entry, clip and placement

Use the [exact branch predicate](design-workshop-nonzero-animation.md#cached-branch-authoritative-byte-inputs).
At main painter entry `43f8cc..43f917`, set the Workshop client canvas clip
to `Workshop+a9c`, then restore that rectangle's saved background from
`Workshop+13c6c` with `5f23e0`. Increment local paint counter+ce8 once.
The tested clip is `[140,9,451,320)`, on a623x387 client canvas with pitch624.

The cached branch `43f9c6..43fa1a` copies the draft chassis/weapon/armor/reactor
bytes and abilities DWORD into scratch design63, then copies the selected
stored design's icon46 into scratch icon46. It does not use the selected
record's component tuple as the rendered tuple. `5d9ca0(63,owner)` calls
`5d8420(2048,63,owner)` and returns scratch Unit ID2048. Scratch Unit facing
byte41 is2; coordinates are(-4,-4), damage is0. These are original scratch
identities, not a requirement to allocate corresponding simulation entities
in the port. The scratch design icon is set back to255 later in main painting
at43fe18, so inspecting its value only after the call misses the icon used
for this preview.

Let `[L,T,R,B)` be the preview clip. With positive global scale override
numeratorN at6b0114 and denominatorD at6b0118:

```text
nominalWidth  = truncTowardZero(100*N/D)
nominalHeight = truncTowardZero(75*N/D)
```

Without a positive override, nominalWidth=187 and nominalHeight=140.
Then `43fa41..43fab5` computes:

```text
x = L + truncTowardZero((R-L-nominalWidth)/2)
y = T + truncTowardZero((B-T-nominalHeight)/2)
56fd50(canvas, scratchUnit, x, y, 14, 0x44, 0)
```

Here canvas is `Workshop+444`; the seven arguments are canvas, Unit ID,
x, y, zoom, flags, terrain-orientation request. They are not a model-facing
angle. The bounded native call is exactly
`(Workshop+444,2048,202,94,14,0x44,0)` twice.

## Main cached admission normally implies the sprite path

Inside56fd50, graphics row starts at scratch design chassis. If chassis0 and
signed weapon icon is nonnegative, row=9+weaponIcon. A nonnegative copied
design icon overrides that with row=9+designIcon. The rank-dependent variant
is used only for row12; all other rows use variant0.

The [scratch-rank follow-up](design-workshop-cached-rank-and-lifecycle.md)
checks13 complete native initializer/query/dispatch controls. Neutral
positive-owner scratch inputs yield row12 variant1, with distinct Psi and
conventional faction-modifier paths. Return the resolved scratch-context rank
from the backend. That follow-up also specifies fresh-construction versus
already-open-refresh yaw reset boundaries.

For ordinary selected designs other than reserved scratch ID63, the main
branch admission proves row>=9: either its copied design icon is nonnegative,
or draft Infantry's weapon icon is nonnegative. In both cases `570131..57015a`
draws a sprite. At57017c..570181 the row>=9 test then skips all CVR rendering.
Thus the general helper's idle4d7860 branch is not the normal cached-main
route. Connecting a fixed-facing CVR renderer for icon0 would be incorrect.
The selected-ID63 alias is a source edge:43fa00 clears its icon before
43fa13 rereads it, so that reserved scratch ID is excluded from this ordinary
admission implication. No new native control claims it is normally selectable.

Sprite address is `77fae0 + 44*(8*row+variant)`. For row9/variant0 it is
780740. With the Workshop flags,56fd50 calls:

`sprite.5fe680(canvas, sprite.key, x, y, 71db18)`.

Flags44 do not request the silhouette or action paths, and bit40 suppresses
later flag/health/order overlays. The separately calculated graphics anchor
bytes in56fd50 feed those overlay coordinates; they do **not** shift this
sprite call. For the explicit icon0 controls, native arguments to5fe680 are
`(Workshop+444,9,202,94,71db18)` exactly. Do not add row9's(1,2) overlay anchor
to(202,94).

Before drawing,56fd50 sets global blit scale toN/D under a positive override,
otherwise `(zoom+16)/16`, hence30/16 here. It temporarily resets scale to1/1
after the sprite stage; this is the observed scale after the complete call.
The original sprite primitive applies its stored trim offset and clip, treats
key9 as transparent, maps source0 through the supplied71db18 destination
lookup, and copies other indices. Source0 is not ordinary opaque black in
this primitive.

The [isolated blit receipt](design-workshop-cached-blit.md) now provides exact
311x311 pre/post destination, source sprite and initialized shadow-lookup
bytes before later caption/data painting. Its independent replay also
distinguishes replication-block shadow sampling on the textured background.

## Original sprite source and trim

Art initializer45f550 loads UNITS.PCX with index offset10, changes loaded
indices7 and6 to9, and extracts icon0's100x75 cell at(2,2). Its sprite
constructor trims key9 borders. The independent owned-asset decode gives
trim bounds `[8,0,95,67)` within that cell: width87, height67, pitch87,
stored offset(8,0). Full trimmed index SHA-256 is
`9c784a21e0e3f0d902d93dcfc39eaee66238e2cb70127a33570736992bd55c42`.

Scale the trimmed raster and its stored offset using the sprite primitive;
do not recenter the87x67 opaque extent. At30/16 the primitive computes width163,
height125 and origin(217,94), before clipping. Source `5fe993..5fe9ea` uses
binary32 scale and truncation for these quantities. The preview's nominal
100x75 footprint remains the placement input, including its transparent area.
This differs from stretching the sprite to the entire311x311 preview clip.

Both native paints preserve stored yaw3f800000 and shared phase14. Paint count
advances0→1→2. Their full623x387, pitch624 surface SHA-256 is
`ae002cf1f175713a45028d4f124d6601af6b18cb2ed9ca419e826c801d8c1a79`.
No4d7860,4d74d0 or4d6080 entry occurs in either paint.

## General helper's idle path, for distinguishing callers

For a caller that reaches56fd50 with row<9, flags44 route through
`5701b9..5701cc` to
`4d7860(canvas,unit,originalX,originalY,flags,zoom,terrainRequest)`.
It receives the original caller anchor, not the adjusted overlay anchor.
TerrainRequest0 skips map/slope sampling and supplies normal(0,0,1).
The idle renderer derives orientation through4d5ad0 and initializes shared
scene8f2790; it composes through4d8400, using the scratch Unit's stored design.
Facing-1 becomes0; the yaw argument is
`StoreBinary32(((facing-3)&7)*binary32(3f490e56)-terrainYawOffset)`,
where3f490e56 is0.7853749990463257, not a substituted exactπ/4.
For scratch facing2 the direction index is7. It does not read Workshop+12770.

Its initial camera scale is `StoreBinary32((zoom+16)*0.07*0.0625)` without
a positive override, or `StoreBinary32(N*0.07/D)` with one, before4d8400's
model-family scale adjustment. Drawing anchors are originalX/Y plus
trunc(50*(zoom+16)/16), or trunc(50*N/D). It passes renderer flags1 for
Workshop flags44, not main3D flags17.4d8400 calls shared body/weapon/ability/
armor/reactor composers, so their phase contracts still apply to such callers.
The idle path has no action-renderer's unconditional phase increment. This
paragraph is source-only; the new native receipt covers the actually reached
icon0 sprite path.
