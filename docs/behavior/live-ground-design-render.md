# Live ground designs on the map and console

Replace the fixed Scout/Spartan role with a **live Unit → retained raw
design → component scene** recipe. The ordinary renderer already supports
custom tuples; its map and console callers share this machinery. The
Workshop strip's fixed-facing scratch Unit is a different caller.
[Nine bounded output controls](live-ground-design-render-cases.json)
distinguish Scout, armor, weapon, utility payloads, Rover, console views,
and owner material. Original executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

## Resolve identity on every draw

`4d7860` is the ordinary flat/slope-aware body caller; it invokes
**`4d8400(unitId,scene)` at`4d7c1c`**. That composer clears the scene's
frame count at`4d8410`, loads **signed raw design ID from Unit+0x0a** at
`4d8420`, and resolves the retained52-byte Design record. It reads actual
chassis/weapon/armor/reactor bytes`+0x24..27` and ability mask`+0x20` at
`4d84c8..4d84ff`. Actual **Unit owner+0x0e**, not the design's bank or a
fixed faction role, supplies the ordinary material owner at`4d84ff`.

For the supported ordinary designs, assemble in this exact source order:

```text
4d88e0(owner,chassis,weapon,armor,reactor,scene)   body
4d8e40(owner,chassis,weapon,scene)                 weapon/package
4d8700(abilities,scene,owner)                     ability decorations
4d9130(chassis,weapon,armor,0,scene)               armor
ordinary reactor branch                         reactor/additional pieces
```

The addresses are`4d8538/4d8541/4d854d/4d855c`, with the reactor branch
starting`4d8561`. Its results for the tested tuples agree with the existing
source-derived [stock composition reference](workshop_stock_composition_reference.py).
The ordinary path does not call the Workshop main composer`4466c0` or
its reactor wrapper`4d95f0`. In particular, ability decorations precede
armor here. Reuse the component helpers and their per-part frame transforms,
not the Workshop caller as a whole.

Infantry attachments retain the decoded mounts: body/armor/reactor use
`(-400,200,0)`; the Infantry gun mount comes from owned AVGMTGUN.CVR via
the existing [mount contract](design-workshop-mount-and-bounds-inputs.md).
Direct copies preserve their source transforms and receive no attachment
offset. Animation counter`8f3c60` drives the existing per-part frame
selection. All nine new output controls deliberately use phase0.
The [ability-art contract](design-workshop-ability-art.md) owns individual
decorator-bit operations; the new controls use mask0 rather than claiming
a new ability matrix.

## Distinguishing compositions at phase0

Names below are owned installed asset identities; payloads remain private.
All examples use reactor1 and abilities0.

| Tuple | Ordered composition operations | Expanded objects |
| --- | --- | --- |
| Infantry/Gun/No Armor | attach VI; attach viptr00 |59|
| Infantry/Gun/Synthmetal | attach VI; attach vipta00; attach viptr00 |60|
| Infantry/Laser/Synthmetal | attach VI; attach VW01; copy VGMT; owner-tint-copy VGMTP; attach vipta00; attach viptr00 |64|
| Infantry/Former/No Armor | VB; owner-tinted VBP; VCL; VCLT00; VLIGHTS; vt; vwntu; VR00 |21|
| Infantry/Colony/No Armor | VB; owner-tinted VBP; VCL; VCLT00; VLIGHTS; drop; droplet; VR00 |8|
| Speeder/Gun/No Armor | VRCP00; VGMC; owner-tinted VGMCP; VB; owner-tinted VBP; VSP; vsptb; vsptf; VLIGHTS; VR00 |10|

VI contains58 records, so two composition operations are not two objects.
Armor and Laser do not merely recolor the Scout image. Former/Colony are
vehicle bodies with distinct payloads, despite stored Infantry chassis0;
they must not reuse the VI soldier silhouette. Complete helper tables,
utility weapon modes, armor tiers, reactor pieces and mounts remain the
[existing composition-family contract](design-workshop-stock-composition-families.md).

## Camera and scale: ordinary view, not strip view

For the supported flat idle route, keep the proven
[ordinary view math](scout-idle-view.md), replacing its Scout-only
multiplier with the [stored component predicate](ordinary-actor-scale-predicate.md).
With R32 denoting a binary32 store and positive override denominator:

```text
h = binary32(0.07) = 0.07000000029802322
base = R32(h*n/d)                    when scale-override n>0
base = R32(h*(zoom+16)/16)            otherwise
scale = R32(base*componentMultiplier)
delta = trunc(50*n/d)                when n>0
delta = trunc(50*(zoom+16)/16)        otherwise
wrapperAnchor = callerAnchor + (delta,delta)
copyOffset = wrapperAnchor - (128,128)
```

For stored chassis0, weapon IDs0..2 or18 use1.25; other weapons use
binary32 **0.8**, bits`0x3f4ccccd`. Stored chassis1/weapon18 also uses1.25;
other chassis/weapon combinations use1.0. Therefore Gun/Laser/Impact
Infantry scale differently from Former/Colony, and ordinary Rover uses
no Infantry multiplier. The multiplier does not alter integer delta.
Reset base before each composition so repeated scene reuse cannot apply
the multiplier twice.

Use the live signed Unit facing byte`+0x29`, loaded`4d791f`; -1 maps to0.
`k=(facing-3)&7`, angle `R32(k*0.7853749990463257)` at`4d7c28..4d7c4f`,
not exact pi/4, minus the admitted view yaw offset`8d5fc8`. The existing
flat auxiliary camera is derived from angle bits`0xbf85f92d`, with
third-row depth scale0.5 and translation`(128.5,128.5,37767.5)` for the
default256-square work surface. Preserve staged matrix/float stores.
Do not copy the strip's facing2/zoom-10 into a live Unit render.

## Map versus selected and stack cards

The general dispatcher is`56fd50`; the supported ordinary ground body
route reaches`4d7860`. Existing dispatcher controls establish this route;
the new draws enter that body helper with the caller's admitted arguments.
Map zoom and projected anchor remain live viewport inputs. Flat/slope
selection is a separate argument: all new controls explicitly supply flat0.
There is no new general terrain-slope or moving/attacking view claim.

At1024x768 the console's origin is`(0,511)`. Its actual callers use:

| Caller | Body anchor, console-local | Zoom / flags | Separate overlay pass |
| --- | --- | --- | --- |
| Selected card`4c9880@4c9a05` | `(34,66)` | -4 /`0x48` | `(14,66)`, zoom0, flags`0x1800` at`4c9a25` |
| First stack/roster cell`4cb9f0@4ce236` | `(156,197)` | -8 /`0x400` | `(145,197)`, zoom0, flags`0x1800` at`4ce256` |

Body and overlay passes are separate. Keep the existing selection/order/
health presentation and clip rectangles from
[lower-console cards](lower-console-unit-card.md). **Do not resize the
entire transparent256x256 work surface to fit a card.** Translate it by
the computed copy offset and clip. The Guard selected control gives
scale0.06562500447034836, wrapper anchor`(71,103)`, copy offset`(-57,-25)`;
the roster control gives0.04374999925494194,`(181,222)`, `(53,94)`.

## Palette, faction art, light and shadow reuse

Load the actual faction identity's PCX, not a fixed Spartan file for owner1.
The original sample `(435,745)` is its vehicle tint index in the active
PALETTE.PCX-derived software palette. Use the already checked
[6,144-byte material-row producer](faction-actor-material-colors.md), sourced
from VBP's24x256 shade table. Owner row address is`78fa98+owner*6144`.
Only explicitly tinted copy/attach operations use it. For example ordinary
VI, vipta00 and viptr00 attachments are untinted; do not recolor the whole
Infantry scene. The Laser mount, utility chassis and Rover include
explicit owner-tinted pieces.

The new owner2/Gaian Laser Guard has the **same complete depth buffer**
as owner1/Spartan and a different complete color buffer. This is a declared
draw-owner control, not a newly traced gameplay ownership-conversion event.

Use initialized map color light from SYSTEM`VEH_LIGHT_DIR_MAP=(6,4,15)`,
normalized to bits`3eb8941e/3e761ad3/3f66b926`. Workshop main-light globals
are not a substitute. Ordinary shadow direction remains the separate
fixed`(-L,+L,+L)`, L bits`3f13cd36`, as in
[initialized light](scout-initialized-light.md). Run the existing
[shadow projection](scout-shadow-producer.md) over the **new composed
scene**, not the Scout's frozen222-pixel mask. New shadow goldens are not
claimed here.

Keep indexed9 clear/key transparency and unsigned depthFFFF; final color
copy tests index9, not depth or RGB magenta. Reuse
[target copy/cleanup](scout-target-copy.md) and the existing shadow
compositing/precision contracts. New complete internal color/depth
controls stop at`631613`, before final2D copy; that final copy is not
newly revalidated for each tuple.

## Names, ownership and invalidation

Selected-card paint rereads Unit.designId at`4c9a81`, then copies the
**current retained Design name** at`4c9a91..4c9ab4`. The tuple formatter
`5932d0` also resolves that raw design; its movement tuple is nominal
design movement, while the separate Moves row uses live Unit state.
Do not freeze names/stats in a frontend UnitDef keyed only by an old role.
The raw-design lookup also applies to intrinsic overlays and restored saves.

The ordinary composer resets/rebuilds its scene and rereads tuple/owner
every invocation. Its nearby location scratch reuse is not an immutable
per-Def mesh cache. The Guard127→LaserGuard127 controls keep Unit0 and raw
design127 while changing the declared stored weapon; the next draw changes
composition/color/depth without a new Unit or a different Def role.

For a port cache, include the resolved tuple/design graphic revision,
actual Unit owner and faction-material revision. Facing, phase, zoom,
view/lighting and render flags also affect raster caching. A name-only
revision can invalidate text without rebuilding geometry. These are port
cache choices implied by the source dependencies, not newly found original
cache structures.

Do not confuse draw-time resolution with Workshop mutation semantics:

- A stored rename leaves Units and their raw IDs intact; the next card
  paint reads the new stored name. Unsaved Rename draft is not yet that name.
- Accepted **live component overwrite** calls`595ac0` before rebuilding
  the slot and removes existing Units of that design. It is not an in-place
  transformation of those Units. See [custom edit lifecycle](design-workshop-custom-edit.md).
- Economic Upgrade has its separate conversion path and target raw ID;
  surviving Units must resolve that resulting ID on the next draw.
- Ownership/conversion/load paths must publish their actual Unit owner/raw
  design state and current faction art before drawing. This receipt does
  not replace those gameplay transition contracts.

## New controls and handoff boundary

Nine original full internal draws:406 expanded objects,482 raster kernels.
All nine ordered composition sequences, flat view matrices, auxiliary
matrices, scales, translations and anchors match independent existing
references. Color/depth guards hold. Literal indexed/depth hashes and
before-target-clip bounds are in the scalar fixture; native owned buffers
and material rows stay private. The new raster outputs are original
controls, not a new independent full-renderer implementation.

Complete107-model initialization and actual Spartan/Gaian material
generation execute using the existing harness. Startup records are
transferred from its earlier admitted startup control; custom127 tuples
are explicit renderer inputs, not new Workshop-save executions. Facing0,
phase0, flat terrain and reactor1/abilities0 bound the new outputs. Existing
facing/zoom, composition-family, decorator and raster/shadow contracts
provide the reusable extensions without another arbitrary matrix.

Private additive freeze:
`tmp/reference/independent-live-ground-design-render-evidence-20261021/`.

## Port integration receipt - 2026-10-21

The backend publishes an owned graphics snapshot on Unit spawn/refresh: actual
component tuple, installed weapon/armor semantics, abilities and current Unit
owner's faction PCX. No pointers into the simulation cross into rendering.
Absent faction-art authority retains the explicit legacy presentation. The
snapshot is not new save authority and does not change the world codec.

Map, selected-card and roster-card bodies consume the ordinary composer with
their existing live facing and independent zoom inputs. All nine complete
internal indexed/depth buffers, anchors and raster-kernel counts above match
the original controls. This is phase0, flat idle body evidence; it does not
establish animated/slope views, final target copies or whole-screen parity.

Decoded scenes and immutable textures share keys for graphics inputs, without
Unit IDs or display names. Actual owner/art, tuple, semantics, facing and view
separate results. Actor keys also isolate presentation definitions because
their Sprite wrappers allocate independent instance IDs. Shared texture data
does not imply a shared counter. Scene teardown preserves borrowed previews.
Geometry/pixel caches retain scene lifetime; long-campaign memory/performance
acceptance remains a separate release gate.

Actual frontend dispatch tests replace a same-ID Unit's body/card on component
and owner-material changes, preserve facing replacement and isolate two roles
sharing pixels. Live backend tests retain the foreign global design while
publishing the recipient's actual material, and preserve graphics on name-only
edits. Valid combat Rovers no longer require a Spartan startup-selection marker;
component, movement, presentation compatibility and retained riot-chain checks
still apply. Naval/air/PSI and unsupported ability physics remain unfinished.

An unmodified-data SDL run produces Morgan's Synthmetal Garrison after five
ordinary turns with the new recipe on map and both cards. Original map shadow
composition, broader animation/slope integration and full Workshop/campaign
acceptance remain open. The existing generic shadow producer is unchanged;
the body integration does not paint new shadows onto the map or console.
