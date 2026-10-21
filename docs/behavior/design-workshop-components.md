# Workshop component cards and armor chooser renders

2026-10-20. Original executable fingerprint and camera conventions are in
[the main-preview contract](design-workshop-renderer.md). This is additive
evidence; previous bundles are unchanged and no production source changed.

Private freeze:
`tmp/reference/independent-workshop-components-evidence-20261020/`.
Manifest SHA-256:
`c626d0ebc17af24f218254a390aff7a75c4fa2e5cb031ad00a76eb4378b8dafa`.

It contains separate fixture inputs/native observations, eight component-card
paint receipts, three bounded armor-picker first pages, **34 complete model-only
indexed/depth frames**, exact per-frame/object/kernel inputs and selected shade
tables. Independent stage checks match all **148 object projections** and
**169 raster calls**, including the two reactor Square2 calls. Private client
rasters retain the actual labels, selected borders and unavailable presentation.
These are offscreen native painters, not desktop captures or a modal-loop run.

## Component cards: compose the actual component

Fixture: select built-in Scout 2, owner 1, Infantry/Hand Weapons/Fission,
no abilities, phase global `8f3c60 = 0`; draft armor is 0 or Synthmetal 1.
Only Industrial Base is owned by owner 1. Each design starts with the same
constructor scene snapshot; its four painters execute in native order below,
sharing that scene. The observed x87 control word at every draw is `037f`.

Coordinates are Workshop-client-local. Rectangles are canvas clips; the
anchors are the actual `631370` X/Y arguments, not an inferred center of
the whole card. The native internal framebuffer remains 256 by 256.

| Card / painter | Composition | Scale bits / approximate scale | Clip | Anchor | Draw flags |
| --- | --- | --- | --- | --- | --- |
| CHASSIS `43de60` | `VI.CVR` only, 58 objects | `3d4ccccd` / .05 | `(30,7)-(129,92)` | `(79,47)` | 2 |
| WEAPON `43e2b0` | `vw00.cvr` only, 1 object | `3e19999a` / .15 | `(30,101)-(129,186)` | `(79,143)` | 6 |
| SHIELDING `43e850`, armor 1 | `vipta00.cvr` only, 1 object | `3d8f5c29` / .07 | `(30,195)-(129,280)` | `(79,235)` | 6 |
| REACTOR `43ed10` | `viptr00.cvr` only, 1 object | `3e4ccccd` / .20 | `(462,7)-(561,92)` | `(511,45)` | 6 |

Armor 0 intentionally **does not call the model renderer** in SHIELDING.
Its labels/stats still paint. There is no invented empty model framebuffer
for that card. These observations explain blank No Armor without treating
the card's renderer as broken.

VI, Synthmetal and Fission parts attach via `631a60`, phase/frame 0, offset
`(-400,200,0)`. The Hand Weapons card calls `632060(7af880,0)`; that copies
`vw00.cvr` frame locations/matrices directly, without the Infantry attachment
offset. The body is not included in its weapon, shielding or reactor card.

All four cards set yaw bits `3f05f92d` (about .52333 radians), not the main
preview's rotating yaw. Its observed scene rotation, row-major:

```text
 .8661580681800842, -.4997701346874237, 0
 .4997701346874237,  .8661580681800842, 0
-0,                  0,                  1
```

The camera auxiliary matrix/position, near/far and initialized model light
are the same kinds of operands captured for the main preview: near/far
`-65535/+65535`, depth endpoints `0/65535`, zero perspective, depth scale .5,
camera matrix rows `(1,0,0)`, `(0,.7073882222,-.7068251967)`,
`(0,-.7068251967,-.7073882222)`, camera position
`(-0,7068.251953125,7073.88232421875)` and normalized model light from `(6,4,15)`.
Use each card's actual scale and scene rotation with these inputs.

Flags 2 on CHASSIS do not request automatic bounds centering. Its observed
scene position is zero and internal translation `(128.5,128.5,37767.5)`.
Flags 6 on the other cards request bounds centering using the **actual card
rotation**, without main-preview flag `0x10`'s identity-rotation override.
Apply [the exact bounds/store schedule](design-workshop-bounds.md) to the
component's own frame/header records. Raw pre/post-centering scene snapshots,
bounds, final projection/translation and selected tables are frozen per draw.
Do not substitute the main model's bounds or shrink its already rendered image.

The reactor's geometry scalar is 3, so `.20 * 3 > .5`: native footprint is
**2 by 2**. Both reactor cards dispatch flags 32 to `6867f0`; the other measured
draws use single samples/flags 33. Both modes retain the terminal-record rule,
strict unsigned depth comparison and modulo-depth arithmetic. Flags bit 0
distinguishes the single-sample versus Square2 kernel policy here.

## Important camera-store correction

`653fb0` spills the scaled X/Y scene-camera translation before adding the
internal anchor. The complete no-shift scale-1 schedule is:

```text
relative = Sub32(scenePosition, cameraPosition)
position = MV(cameraMatrix, relative)
scaledXY = Store32(position.XY * cameraScale)
translation.XY = Store32(scaledXY + 128.5)
```

Keep that intermediate `Store32`. The earlier main-view controls did not
distinguish omitting it. Photon Wall's chooser control does: one Y becomes
`165.764892578125`; skipping the store produces `165.76487731933594`.
The public bounds/prepass document now includes this correction. Existing
frozen main-view expectations and bytes remain unchanged.

## Armor picker: original caller and grid

The probe executes `444860` through its actual `43b7d0` constructor and
`43cfc0` setup/first-page painter, stopping at `43d433` **before** modal entry.
Scout and Synthmetal controls differ in the selected border. The third control
explicitly grants owner 1 every technology to expose available advanced models;
it is not a natural starting-faction technology claim.

Native picker fields: kind 2, 5 columns, 4 rows, 20 page slots; ten configured
armor entries in this fixture. The bounded client is 623 by 387. For ordinal
`i`, `col=i%5`, `row=i/5` (integer):

```text
outer = (27+108*col, 4+94*row)-(132+108*col, 95+94*row)
model clip = outer inset 3
model anchor = center(model clip), with integer truncation
```

The original grid visits ordinals 0..19. Ordinals 10..19 attempt the blank-sprite
path, whose zero-size source writes no pixels; they retain the picker background.
See [picker backing and borders](design-workshop-picker-backing.md). Ordinal 0,
No Armor, has text/stats and no model. Eligible definitions with prerequisite
at least -1 remain in the ordinal list even when their technology is unknown.
Definitions below -1 are excluded from ordinal enumeration. A separate
chassis-definition byte guard can limit this picker to No Armor; these
Infantry controls do not take that branch.

Every nonzero armor card uses scale bits `3dcccccd` (.10), the same fixed yaw
`3f05f92d` and ordinary flags 6. Each card gets its own model bounds centering.
`43cda0` draws the selected/unselected outer border before dispatching
`43c100`. The frozen glyphs include font IDs, baseline, color and GDI clip.
Example Synthmetal label lines are at `(150,14)` and `(168,27)`; its Shielding
caption/defense are `(138,89)` and `(230,90)`. These are native glyph baselines.

## Unavailable is a silhouette, not a hidden card

`43c100` maps the ordinal to the configured armor record and calls the actual
`5d17b0(prerequisite,owner)` predicate. In the starting-tech controls, No Armor
and Synthmetal are available; the other eight entries remain visible.

For an unavailable nonzero armor it composes the model, then calls
`631760(canvas,x,y,185,6)`. That fills the 6,144-byte override table at `9c2130`
with index **185**, temporarily sets scene flag 8, draws through `631370`, then
clears that flag. Complete resulting model frames contain only background 9
and silhouette index 185; their normal geometry/depth is still calculated.
The card returns before drawing its name and stats. It does not label the
silhouette with the otherwise available card's text.

## Available advanced armor composition and palette policy

For this Infantry/Hand Weapons fixture, all model frames are phase 0 and
attachments use `(-400,200,0)`:

| Armor ID | Component geometry | Material / extra presentation |
| --- | --- | --- |
| 0 No Armor | none | Text/stats only |
| 1 Synthmetal | `vipta00.cvr` | Model's own shade table |
| 2 Plasma Steel | `vipta00.cvr` | Override table at `7a7a98` |
| 3 Silksteel | `vipta00.cvr` | Override at `7a9298` |
| 4 Photon Wall | `viptawal.cvr` | Own shade table; grouped geometry |
| 5 Probability Sheath | `vipta00.cvr` | Identity shade rows, then indexed translucent composite |
| 6 Neutronium | `vipta00.cvr` | Override at `7aaa98` |
| 7 Antimatter Plate | `vipta00.cvr` | Override at `7ac298` |
| 8 Stasis Generator | `viptasgn.cvr` | Own shade table; grouped geometry |
| 9 Psi Defense | `viptapsi.cvr` | Own shade table; grouped geometry |

`631be0(model,shadeTable,offset,frame)` supplies explicit table bytes while
retaining the source model's frame and geometry. The four tint tables derive
from **VBP.CVR's** 24 by 256 shades and the common game palette, not from
the armor model's own palette. Installed `SYSTEM.TXT` ARMORCOLORING values
95/233/88/70 become game indices **105/243/98/80**. For every source shade index,
multiply each source RGB channel by tint RGB and float bits for
`.003921568859368563`, truncate, then choose nearest among all 256 palette
colors by squared distance, keeping the lowest index on ties. All 24,576
generated table bytes independently equal the installed `armorgrads.tmp`.

The harness supplies those declared installed cache bytes to original
`42abd6..42abe4`, and the installed 6,144-byte translucency cache to original
`42acbc..42acca`. Original `42abed..42ac1a` also fills the 24 identity shade
rows: every byte in row `s` is `s`. The cache inputs and exact palette/VBP/system
provenance are frozen; no port output feeds native execution.

Probability Sheath calls `6312e0(canvas,x,y,24,70e018,6)`. Its internal buffer
clears to **24**, receives shade-row indices 0..23, and uses flags `6|8` to
suppress the ordinary final copy. The following native indexed composite uses
source 24 as transparent; another source value `s` chooses lookup row `s`,
indexed by the destination pixel. Do not convert this internal frame directly
to opaque RGB or replace it with generic alpha blending.

Installed ARMORCOLORING_TRANS is relative color 62, beginning 30, ending 60.
For row `s=0..23`, `weight=trunc((24*30+(60-30)*s)/24)`. For destination indices
10..245, mix `((100-weight)*destinationRGB + weight*palette[72].RGB)/100`,
truncating each integer channel; choose nearest palette index 10..245 with
lowest-index ties. Reserved indices preserve identity. All 6,144 resulting
bytes independently equal the installed translucency cache. This uses the
same protected-range conventions as [effect indexed compositing](scout-effect-indexed-compositing.md);
the measured software palette has no configured protected ranges.

## Evidence boundaries and use

`checked-contract.json` has uniquely named side-card/chooser-frame cases,
native expected color/depth hashes, final camera operands and source tables.
The matching `native-observations.json` has full frame/geometry/lighting/kernel
entry operands; `fixture-inputs.json` declares selection, technology and cache
inputs. The three client images are painter outputs under those declared inputs,
not live parent-window visibility/input proofs. Probability Sheath's composite
executes in the client paint, while its frozen model-only frame is the buffer
before that composite. Derived PNG alpha is a host display convenience.

Port the separate component compositions, centering flags, fixed yaw, scales,
sample footprints and indexed material policies. Live chooser mouse/keyboard
selection, cancellation and other chassis/weapon/reactor combinations remain
outside this renderer receipt.
