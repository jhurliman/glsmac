# Workshop main-preview renderer contract

2026-10-20. Original `terran.exe`, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This supplements [the preview assembly evidence](design-workshop-preview-parent.md).
No production renderer or game source was changed.

Private additive freeze:
`tmp/reference/independent-workshop-renderer-evidence-20261020/`.
Manifest SHA-256:
`ee06051a5e77b093928d8f0680f2d317a315a916a9aab06da56fdf2b1bc41cd9`.
All five earlier Workshop bundle manifests and their listed files verified
unchanged. This bundle includes separate `fixture-inputs.json` and
`native-observations.json`, complete native frame/location/matrix records,
per-object lighting arguments, selected lookup bytes, per-kernel streams,
complete indexed/depth buffers, original source traces and probe/check scripts.
Owned art/geometry and disassembly remain private.

## Four bounded original-code renders

The original initializer, Workshop constructor, selection and main painter
execute. Each case starts from the same constructor scene snapshot, selects
Scout ID 2, supplies owner 1 and Hand Weapons/Fission/zero abilities, and sets
the draft armor to either 0 or 1. Animation global `8f3c60` is explicitly zero.
Stored yaw is either zero or binary32 `3d4ccccd` (approximately 0.05). The
painter uses that yaw, then increments the stored value by approximately 0.05.
The latter increment does not affect the current draw.

| Draft | Object draws | Indexed raster calls | Stream records, excluding terminators |
| --- | ---: | ---: | ---: |
| Stock Scout, each yaw | 59 | 65 | 47,483 |
| Infantry/Synthmetal, each yaw | 60 | 66 | 56,644 |

The 238 per-object projections and locations match an independent staged
arithmetic calculation from native **prepass entry operands** and native frame
operands. An independent scalar raster replay of all 262 native kernel-entry
operand sets matches every byte of all four color and depth buffers. These
are stage checks, not an independent whole-asset-to-camera implementation.
The original execution does not call the port or consume port-rendered output.

## Composition and per-object inputs

The relevant call interval starts at `4466c0` returning to `43faf3` and ends
at the main `631370` returning to `43fb3a`. This excludes component cards and
the console design strip.

Scout attaches `VI.CVR`, then `viptr00.cvr`; Synthmetal inserts
`vipta00.cvr` between those two. Every attachment in these four controls uses
frame 0 and offset **(-400, 200, 0)**. Preserve source object order, individual
frame matrices/locations, geometry scalar and grouped-object subdivision.
The armor contributes one object/kernel, not a replacement Infantry body.

`native-observations.json` records each frame index, descriptor, frame flag,
location and matrix at `654856`; each actual `659800` argument set includes
the selected geometry header, transformed location/matrix, both light operands,
geometry scalar and footprint flag. Each raster record identifies its object
and group plus the exact first/second lookup digests. Do not merge all models
under one shade table. The first table is 2,048 bytes; the scene palette's
selected second table is 6,144 bytes.

## Native centering before projection

The main painter calls `631370(clientCanvas,295,164,0x17)` using the
client-local center of `(140,9)-(451,320)`. The renderer uses its own
**256 by 256** internal surface, centered independently of that client anchor.
Do not stretch that internal image to the entire preview rectangle.

Flags include `4` and `0x10`. At `631532..6315f2`, save the scene rotation,
temporarily replace it with identity, and execute `655210(scene+18,0,min,max)`.
Restore the rotation, compute each midpoint with the original binary32 store,
then subtract it from the scene position. `655210` uses each object's frame
location/matrix, geometry scalar and geometry bounding vectors; `655540`
accumulates the transformed box corners. It does not measure raster pixels.
The additive [bounds and attachment schedule](design-workshop-bounds.md)
now gives its exact operations/spills and independently checked native entry
operands/results, including the two complete model bounds.

In this constructor fixture the resulting scene position is:

| Draft | X | Y | Z |
| --- | ---: | ---: | ---: |
| Scout | 6.574676513671875 | 55.81300354003906 | -252.00990295410156 |
| Synthmetal | 15.566165924072266 | 55.81300354003906 | -252.00990295410156 |

Both tested yaws produce the same centering position because bounds use
identity rotation. These numbers are verification outputs, not runtime
constants to substitute for deriving bounds from the supplied models.

## Camera and lighting: distinct operands

The scene's own scale at the prepass is **1**. `6324f0(0.14)` sets the
camera scale, bits `3e0f5c29`. The prepass auxiliary/camera matrix, row-major:

```text
1, 0, 0
0, 0.7073882222175598, -0.7068251967430115
0, -0.7068251967430115, -0.7073882222175598
```

Its position is `(-0,7068.251953125,7073.88232421875)`, perspective is zero,
near/far are `-65535/+65535`, and output depth endpoints are `0/65535`.
The final global scales are `(0.14000000059604645,0.5)`.

This camera matrix is distinct from the **model illumination vector**.
In these initialized controls the actual first vector passed to `65ab50` is
`(0.36050504446029663,0.24033670127391815,0.901262640953064)`, from the
initialized direction `(6,4,15)`. The second argument vector reported at the
draw boundary points into the camera's auxiliary matrix; preserve that matrix
as a matrix for lighting selection. Its first row alone is not a replacement
light direction. The `9c902c` shadow-projection globals are zero in these
controls and must not be mistaken for the lighting auxiliary matrix.

The earlier preview document calls `6323e0` the constructor light setter;
its copied vector/matrix are the camera/auxiliary operands above. The new
`65ab50` receipts now distinguish those from the model illumination input.
Reusing the existing map renderer's tilt, translation or default light policy
would bypass the observed Workshop caller.

For yaw zero, final projection words, row-major:

```text
3e0f5c29 00000000 00000000
00000000 3dcad286 bdcaa932
00000000 beb4f27f beb51765
```

For yaw bits `3d4ccccd`:

```text
3e0f2e4b bbe547cb 00000000
3ba230b6 3dca91a2 bdcaa932
bc90b28f beb4b89b beb51765
```

Final translation is `(129.4204559326172,158.96517944335938,37836.91015625)`
for Scout, and `(130.67926025390625,158.96517944335938,37836.91015625)`
for Synthmetal, for both tested yaws. Derive it from the centered scene
position and camera position/matrix. The internal XY anchor is **128.5**, not
the external client center `(295,164)`.

Use the existing [x87 arithmetic schedule](cvr-matrix-arithmetic.md):
binary32 operands/stores, extended 64-bit significand intermediates under
control word `037f`, the cell-specific matrix addition order, then a separate
geometry-scalar store. Native per-object operands and origin/basis values are
included for direct comparison with the existing indexed kernel.

## Complete model-only outputs and limits

Color starts at index 9 throughout; depth starts at unsigned `65535`.
All calls here dispatch `6867f0` with flags 33 and the single-sample footprint.
The allocated auxiliary byte plane stays zero. It is not native RGBA alpha.
Private PNGs are derived from the native indexed frame and loaded software
palette, with host alpha zero for index 9. The original internal buffers,
rather than those PNGs, are the equality expectations.

| Case | Indexed SHA-256 | Depth SHA-256 |
| --- | --- | --- |
| Scout, yaw 0 | `0e75d4f02c26fd845e7120c405af7518d8184ff3a16de8a089a34dae4463c670` | `fdead0e225fd61882635dddfde47b452dd1bdd979bf98fb15246cc2ab9857282` |
| Synthmetal, yaw 0 | `00091bcc2b41aaa0358434fbb6b3090916ac5e4c05d7e96dd48cc37d9481284a` | `d3248d7369e591a12a6e7afb146153e3becbea7410f6b5b62ae44ffb90ea3fb8` |
| Scout, yaw ~0.05 | `d00bfab447f7500ca5c07423db7348090b546a47dc09c86830d7e5d3312763cd` | `aed810b8e59c218c0e8b06af46fbb7172ec36124f9afe7f0b8879ea6807127e3` |
| Synthmetal, yaw ~0.05 | `0e10b15d76d3fa516a91b57b8a3955ace9de758084d218473249967c40f9c62d` | `47991c347ffc13ecad756c0ddb5be769a54b90df089564bd3f100642448f3b55` |

This proves the original Workshop-to-indexed draw and complete internal
color/depth outputs under declared inputs. Memory-backed OS/file/GDI services
remain the inherited offscreen fixture boundaries. It does not establish a
booted desktop screenshot, every animation phase, other components, final
window clipping/compositing or an independently decoded bounds implementation.
Those limits do not require map-camera defaults for the renderer integration.
