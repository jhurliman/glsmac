# Named landmark art in the ordinary tile prefix

Implementation checkpoint: the C++ quad selector is validated against the 358
original caller contexts. Live quad composition passes land/sea and Forest/sea
Fungus ordering controls. Uranium/Geothermal/Nexus marker selection, live
composition and separate legacy-instance replacement are connected and tested.
Jungle adjacency and Dunes/Farm precedence match all 76 additional original
material contexts and are connected to live composition. All 443 captured indexed frames replay exactly through the production rasterizer
under ASan/UBSan, using owned external assets. This captured-command replay does
not prove the complete live scene assembler. Live capture and original-scene
parity remain pending. The contract below describes the
original executable; it is not a production-completion claim.

This closes the normal named-art continuation at zoom0 with known viewer3,
default relief and gamma100. It composes the established
[height/shade profiles](landmark-height-shade-profiles.md),
[developed overlays](ordinary-developed-overlays.md), and
[resource/Pod/Monolith display](resource-pod-monolith-art.md).

The [quad/marker cases](named-landmark-quad-marker-art-cases.json) cover358
producer contexts and363 indexed frames. The
[material cases](landmark-material-cases.json) add76 contexts and80 frames.
Together434 contexts produce1,948 ordered commands;443 complete indexed prefix
frames match50,031 polygon spans,1,494,244 attempts and1,222,075 stores. The
[reference](named_landmark_art_reference.py) checks every span, source attempt,
store and frame hash using external owned PCX assets. This is offscreen native
execution, not a claim of live screenshot or full UI parity.

## Authority and draw order

Named membership and detail come from the **live shared tile word**, even when
ordinary improvements use remembered items. Under the measured normal rendering
path, order is:

1. Ordinary land fan or sea-sector base polygons.
2. Mount Planet, then Sunny Mesa, then Garland Crater art quads.
3. Uranium/Geothermal marker sprite, when enabled.
4. Jungle, Forest, Fungus, territory, coast, River, Kelp and Road/Tube in their
   established order and respective gates.
5. Resource icon, common developed sprites, Nexus sprite, Pod and Monolith.

Keep a single ordered polygon/sprite command list. Moving all sprites after all
terrain polygons changes mixed marker/Jungle/coast composition. Memberships can
overlap: the three quad branches are independently admitted and all eligible
ones draw in the order above. The height and extra-shade selectors still use
their separate priority orders; art is not an exclusive choice of one profile.
The native alternate-style/fog override suppresses these named quads, marker
sprites and Jungle. That alternate mode remains outside the measured profile.

## Crater, Mount Planet and Mesa quads

For normal detail0..48, read the corresponding signed `(artIndex,rotation)`
pair from the49-row table in the cases. An artIndex of-1 skips that quad. Do not
replace these tables with a radial or coordinate heuristic. The mapping is
independent of the113-entry height/shade mapping.

Rotate the outer terrain vertices W,N,E,S and their associated scalar values
by `rotation & 3`. Each source crop is56x56 in TEXTURE.PCX:

| Live bit | Name | Crop |
| --- | --- | --- |
| `2` | Mount Planet | `(1+57*artIndex,628,56,56)`, indices0..2 |
| `0x100` | Sunny Mesa | `(768+57*(artIndex%4),15+57*(artIndex//4),56,56)`, indices0..7 |
| `1` | Garland Crater | `(1+57*artIndex,685,56,56)`, indices0..2 |

Mount UVs are `(0,0),(55,0),(55,55),(0,55)`. Mesa and Crater use
`(0,55),(0,0),(55,0),(55,55)`. Geometry is terrain-deformed, not a sprite
rectangle. All use key9 transparency before lighting. Mount detail0 uses the
**unlit** keyed polygon kernel; other admitted Mount rows, Mesa and Crater use
the keyed/shaded kernel.

On land, the scalar values are the deformed tile's established raw vertex shade
values, including extra landmark shade, rotated with the vertices. Over sea,
the caller supplies **sector ordinals `[0,1,2,3]`**, rotated with those vertices.
It does not use zero land lighting, extra profile shade, water depth, or a mesh
normal for these art quads. The scalar limits are the current renderer limits:
land[-4,4], sea[-256,256] in this profile. Mount detail0 ignores these scalars
through its unlit kernel. Sea heights remain0 despite these possible art draws.

The quad branches are not controlled by resource flag `0x200` or improvement
flag `0x20`; a flag1 control still draws Mount detail0. Signed-negative detail
skips their art. The existing negative-height guard remains necessary because
the separate height consumer has no equivalent negative-detail guard.

## Uranium, Geothermal and Nexus sprites

These use TER1.PCX, native transparent trim, the terrain projector anchor P,
axis-aligned placement and the **plain** keyed sprite kernel. Source0 is a
literal index0 store, not a destination shadow. They receive no terrain lighting.

Under flag `0x200`, test live `(word & 0x80000008)==8` for Uranium; otherwise
test `(word & 0x80000400)==0x400` for Geothermal. Uranium wins if both are set.
There is no land-only gate in these calls. The high suppression bit suppresses
either marker. Crop size is100x62: Uranium `(822,253)`, Geothermal `(822,190)`.
These are the earlier marker sprites, not effective resource-bonus icons.

Nexus requires improvement flag `0x20` and
`(word & 0x80002000)==0x2000`. Detail2..4 draws no Nexus sprite; other normal
detail rows select:

| Detail | TER1 crop origin, size100x62 |
| --- | --- |
| 0 | `(579,695)` |
| 1 | `(680,695)` |
| 5 | `(478,695)` |
| 6 | `(478,632)` |
| 7 | `(579,632)` |
| 8 | `(680,632)` |

Nexus can draw on sea too. Its branch follows common improvements and precedes
Pod/Monolith. Neither these early markers nor Nexus has its own zoom>-8 test
in the traced caller. This work verifies their zoom0 pixels only, without
promoting scaled pixel behavior. Nexus details beyond the normal0..8 footprint
are not admitted by the reference.

## Jungle and Great Dunes materials

Jungle requires land and live `(word & 0x80000004)==4`. Build the same eight-bit
adjacency mask/rotation lookup family as the existing Fungus selector: bit k
examines native neighbor `(k-1)&7`; the neighbor must be land and have the same
unsuppressed live Jungle membership. A suppressed or sea Jungle neighbor does
not connect. The 15 crop origins form a4-column grid at
`(526+57*(index%4),259+57*(index//4))`, with56x56 crops and only the first15
entries. Rotate UVs by taking the outer corner of each rotated canonical fan
triangle, as the reference does. Draw a single unlit keyed quad on the deformed
outer terrain vertices, after early marker sprites and before Forest/Fungus/coast.
Jungle has no separate terrain flag1 admission test in this branch. All15 crops,
eight single-neighbor directions, high-bit suppression, sea rejection and mixed
Forest/Fungus/marker/coast cases are checked.

Great Dunes is a base-material override for live bit `0x40` on land. It selects
TEXTURE crop `(229,1,56,56)` for each ordinary base triangle, preserving the
existing triangle UVs, geometry and lighting. It tests the bit without a high-bit
suppression mask: `0x80000040` still selects the Dunes material. Selected Farm
under terrain flag1 takes its Farm base texture before the Dunes branch. With
flag1 absent, that Farm base-texture branch is skipped and Dunes wins. Farm's
later developed sprite remains governed by its own improvement flag and nutrient
selection. Dunes and Jungle may coexist: Dunes base triangles, then Jungle quad.

## Height-only and passive markers

Nessus `0x200` and Pholus `0x800` have no additional art branch here. Their
normal height/shade effects still alter the ordinary fan and other attached
art;20 indexed land/sea controls supplement the earlier complete geometry proof.
Raw `0x10`, `0x20`, `0x80`, `0x1000`, `0x4000` add no independent art in this
prefix when ordinary live items are absent. This includes checked nonnegative
and suppressed detail bytes. It does not erase the meaning of those landmarks:
Ruins may contain separately rendered Monolith items, Borehole Cluster affects
an existing live Borehole's variant, and later actor/model continuations remain
separate. Preserve all retained words and item fields.

Negative/out-of-range height detail, out-of-range art detail, arbitrary malformed
fields, alternate fog/style, diagnostic overlays, scaled pixels and subsequent
actor/fog scheduling are outside this closure. The whole accepted prefix ends
at a documented native instruction boundary before those later continuations.
