# General conventional Workshop model previews

2026-10-20. Original terran.exe fingerprint is unchanged from
[the main renderer contract](design-workshop-renderer.md). Additive private bundle:
`tmp/reference/independent-workshop-general-visuals-evidence-20261020/`.
Manifest SHA-256:
`b655640abdfa715169e8395d0ca170d30ed43c71fde108d2a372d078afa97cf4`.

**All 15 complete native indexed/depth frames match independent staged replay:**
three full previews and all four side cards for each design, totaling **326
object projections and 400 raster calls**. Models, constructors, builders,
selection and painters execute original x86 code against owned assets. There
is no port output, desktop capture or modal input. Geometry/view/raster checks
consume declared native stage-entry operands; this is not independent
whole-asset-to-camera regeneration.

## Fixtures and materials

Each fixture runs the complete original `5bd4f0` builder for saved custom slot
127, then selects that record through `4490b0`. The three tuples are Infantry /
Laser / Synthmetal / Fission, Infantry / Laser / Plasma / Fusion, and Rover /
Impact / Synthmetal / Fission, all owner 1 and no abilities. A self-authored
diagnostic record name has no geometry role. The native builder-produced
52-byte record is a declared stage boundary, captured separately in fixture
inputs. Record correctness is covered by
[the independent construction controls](design-workshop-general-construction.md).

All paints restore the same initialized Workshop scene, set animation counter
`8f3c60=0`, stored main yaw=0 and x87 control word `037f`. They use the
existing software voxel path, fixed original camera/light setup, 256 by 256
indexed surface, background 9 and depth 65535.

Faction-colored panels require an **initialized faction material row**. These
fixtures explicitly use Spartan art for owner 1. Original PCX loading/sampling
reads `spartans.pcx` at `(435,745)` and produces loaded tint index 80, RGB
`(64,76,76)`. The declared material input at `791298` is independently derived
from that tint, VBP.CVR's 24 by 256 shade rows and the active Gamma100 palette,
using the established [faction material algorithm](faction-actor-material-colors.md).
Its full 6,144-byte hash is
`a8416d9624088092b893b4fe4a49214ef6422b0c56748c1f8f1a47cf9e0d74dc`.

This row is supplied at the material-stage boundary; this supplement does not
rerun the complete faction graphics/cache lifecycle or claim every owner 1 is
Spartan. The producer algebra, original sample, palette/source bytes and full
independent derivation are recorded. The palette is the native Gamma100 profile
`c7129c7433e5d3028e4e3d7990e696fd9a5b321ebf24694cee6316e5d9a32ba6`.
Use the actual loaded faction's row in production. Zero-filled rows cannot
serve as the default for the mount/body panels introduced by these designs.

## Full preview composition, in source order

`4466c0` clears the scene, then calls body `4d88e0`, weapon `4d8e40`, armor
`4d9130`, reactor `4d95f0`, and ability decoration `4d8700`. Global `7528f8`
is 1 only around the weapon call. The three no-ability controls add no ability
decorations. Preserve the order, source frame locations/matrices, per-object
shade tables, geometry scalars and grouped-object subdivisions.

The attachment APIs differ:

- `631a60(model,offset,frame)`: ordinary attachment using an explicit offset.
- `631be0(model,tint,offset,frame)`: attachment with an override shade table.
- `632060(model,frame)`: copy the model's stored frame transforms directly.
- `6321c0(model,tint,frame)`: direct frame copy with an override shade table.

Direct-copy assets do not receive the Infantry attachment offset. All frame
arguments observed here are 0. The complete native frame-transform records and
each selected material table are frozen; do not collapse every asset into a
single rigid transform or shade table.

### Infantry Laser / Synthmetal / Fission

| Order | Asset | Operation / material |
| --- | --- | --- |
| 1 | `VI.CVR` | Attach at `(-400,200,0)`; own shades |
| 2 | `VW01.CVR` | Attach at Infantry weapon mount; own shades |
| 3 | `VGMT.CVR` | Direct copy; own shades |
| 4 | `VGMTP.CVR` | Direct copy; owner material row |
| 5 | `vipta00.cvr` | Attach at `(-400,200,0)`; own shades |
| 6 | `viptr00.cvr` | Attach at `(-400,200,0)`; own shades |

The weapon mount vector is read from `7af698`; this initialized fixture has
binary32 values `(0.000003069639205932617,70.45306396484375,136.67886352539062)`.
`4d9003..4d9014` supplies that vector to `631a60`. It is distinct from the
body attachment offset. The tiny nonzero X is an observed arithmetic result,
not permission to quantize loaded frame locations generally.
The [asset-derived mount contract](design-workshop-mount-and-bounds-inputs.md)
derives this vector from AVGMTGUN.CVR's first two frame positions and specifies
hidden-object/root-LOD bounds behavior.

### Infantry Laser / Plasma / Fusion

Same first four operations as above. Armor is still `vipta00.cvr`, but uses
`631be0` and Plasma's table at `7a7a98`, zero-based armor cache table 0, hash
`faf65eb5141db7ca6fe896a71020787d2eb274470117012b85bd6c0ba1c474b9`.
The last attachment is **`viptr01.cvr`**, Fusion, with own shades and the
same `(-400,200,0)` offset. A reactor change must select the reactor asset;
it is not solely an HP/stat update.

### Rover Impact / Synthmetal / Fission

All operations below are direct frame copies, in order:

| Order | Asset | Material |
| --- | --- | --- |
| 1 | `VRCP00.cvr` | Own shades |
| 2 | `VGMC.CVR` | Own shades |
| 3 | `VGMCP.CVR` | Owner material row |
| 4 | `VB.CVR` | Own shades |
| 5 | `VBP.CVR` | Owner material row |
| 6 | `VSP.cvr` | Own shades |
| 7 | `vsptb.cvr` | Own shades |
| 8 | `vsptf.cvr` | Own shades |
| 9 | `VLIGHTS.CVR` | Own shades |
| 10 | `VW02.CVR` | Own shades |
| 11 | `VA01.cvr` | Own shades |
| 12 | `VSPA01.CVR` | Own shades |
| 13 | `VR00.cvr` | Own shades |

The first nine are the chassis/body stage; then weapon, two armor assets, and
reactor. The scene contains 41 frame descriptors but only **35 object draws**:
six Impact objects have frame flag `80` and are skipped by the original draw
dispatch. The weapon side card similarly retains 29 descriptors and draws 23.
Do not render hidden descriptors or remove all geometry from that model.

## Side cards are separate compositions

Each card starts from the same scene snapshot and executes its own native
painter. All use fixed yaw bits `3f05f92d`, approximately .52333 radians,
with matrix rows `(.8661580682,-.4997701347,0)`,
`(.4997701347,.8661580682,0)`, `(-0,0,1)`.

| Card | Native painter | Scale | Draw flags | Composition |
| --- | --- | --- | --- | --- |
| Chassis | `43de60` | `3d4ccccd`, .05 | 2 | Body stage only: VI, or Rover's first nine assets |
| Weapon | `43e2b0` | `3e19999a`, .15 | 6 | Infantry VW01 + VGMT + tinted VGMTP; Rover VW02 only |
| Shielding | `43e850` | `3d8f5c29`, .07 | 6 | Infantry vipta00, Plasma override if ID 2; Rover VA01 + VSPA01 |
| Reactor | `43ed10` | `3e4ccccd`, .20 | 6 | Infantry viptr00/viptr01; Rover VR00 |

Native client clips/anchors remain the established component-card rectangles
in [the component contract](design-workshop-components.md). Flags 6 center
using the actual fixed card rotation; flags 2 on chassis do not request bounds
centering. Both Infantry reactor cards select Square2 at this scale; the Rover
Fission reactor selects Single. Do not infer footprint solely from the displayed
reactor category.

Full previews use scale bits `3e0f5c29` (.14), anchor `(295,164)` and flags
`17` hex. At stored yaw 0 their scene rotation is identity. Main flags include
the identity-rotation bounds override; each design's own model bounds determine
its centering. Side-card centering does not use that override. Apply the
[binary32 store/attachment schedule](design-workshop-bounds.md), including
the scaled-translation spill before adding 128.5.

## Raster details newly exercised

The recorded geometry flags include grouped objects (`2`); their group vectors,
counts, flags and individual kernel streams are frozen. Four grouped objects
in each Infantry main draw yield 3/2/4/2 kernels; Rover main includes ten grouped
objects. Keep original traversal and strict unsigned depth comparison.

Across the 400 kernel calls: 394 are ordinary Single with terminal flag 32,
two are ordinary Square2 with terminal flag 32, two are **Square2 without that
flag**, and two are clipped Single with terminal flag 32. Stream processing
must follow that flag: with 32, the last direction-26 record is a terminator;
without it, all count records are real voxels. Rover lights exercise the latter.
Unconditionally dropping every stream's last record loses valid pixels.

Laser weapon cards each reach clipped entry `686000` once. Its signature is
`(x,y,z,clipPointer,count,stream,flags)`, unlike six-argument `6867f0`.
The internal clip is half-open `(79,86)-(178,171)` in these controls. Advance
the stream's coordinates for every voxel, then reject outside the clip before
testing/writing depth. The independent replay covers every byte of both Laser
card outputs after this correction. The original clipped-kernel patcher and
entry were also isolated against recorded stage inputs during validation.

## Using the freeze

`fixture-inputs.json` declares builder-stage records, constructor scenes, phase,
camera-angle inputs and material inputs. `native-observations.json.z` contains
the composition calls, loaded filename map, all frame/light/group/kernel entry
operands and exact output hashes. `checked-contract.json` identifies the 15
successful color/depth comparisons. Hash-named binaries hold full lookup tables,
streams, indexed buffers and depth buffers; case-named PNGs are private inspection
views. Filenames/material choices above are the runtime contract; framebuffer
hashes and observed centers are expectations, not runtime constants.
