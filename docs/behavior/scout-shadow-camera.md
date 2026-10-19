# Scout shadow camera, light and ground-plane producer

[Runtime reference](scout_shadow_camera_reference.py), [original numeric observations](scout-shadow-camera-cases.json). This supplement removes the measured projection/translation dependency from the earlier [shadow raster reference](scout-shadow-producer.md) for the flat, orthographic idle Scout path.

The producer takes actual scene inputs: auxiliary matrix `B`, auxiliary position `a`, auxiliary scale `s`, object orientation `R`, object position `p`, object scale `o`, shadow direction `D`, ground height `h`, XY scale, scene offsets, pixel shifts and surface dimensions. The convenience `flat_scout_scene(facing,zoom,ratio,surface)` constructs the original flat Scout inputs without reading a captured camera matrix. It reuses the established tilt bits `0xbf85f92d`, facing step bits `0x3f490e56` (not exact pi/4), base scale bits `0x3d8f5c29`, Scout multiplier1.25 and auxiliary distance10000. The complete runtime reference then generates the shadow matrix and translation.

## Light setup

Original light `L` has binary32 bits `[0xbf13cd36,0x3f13cd36,0x3f13cd36]`. Do not normalize it again. Native setup computes **`D = inverse(B) × L`**, with binary32 matrix/vector output stores. It does not substitute `B`, its transpose or an ideal orthonormal matrix for the inverse.

For the checked symmetric flat helper matrix, let `c=B[1,1]`, `n=B[1,2]=B[2,1]`, and `B[2,2]=-c`. The native inverse's stored cofactor is `q=float32(c*c+n*n)`; its reciprocal is stored again as `iq=float32(1/q)`. The inverse XY/Z block uses float32 products `c*iq`, `n*iq`, `-c*iq`; its first diagonal entry is float32(q*iq). This reproduces the observed original inverse and direction exactly. Here `iq` is1.0000001192092896. Using the uncorrected matrix produces a different direction despite its near-unit rows.

Both facing controls produce `D=[-0.5773500204086304,-0.21090613305568695,-0.7887869477272034]`. Facing changes `R`, not this light input. The reference's inverse specialization is for the flat helper matrix; arbitrary slope-matrix inversion remains outside this supplement.

## Projection formula

Clamp direction Z near zero using binary32 epsilon bits `0x358637bd`: `-epsilon < Dz < 0` becomes `-epsilon`; `0 <= Dz < epsilon` becomes `epsilon`. The native code writes the clamped scene field. These near-zero branches are source-confirmed; the three executed controls use ordinary negative Z.

Let `rx=Dx/Dz`, `ry=Dy/Dz`. Build this matrix with binary32 entries:

```text
P = [ 1  0  -rx
      0  1  -ry
      0  0    0 ]
```

The pre-scale matrix is `C0 = (B × P) × R`, storing binary32 output after each matrix multiplication. Compute the pre-scale translation in separate stored stages:

```text
u = float32(P × p)
u = float32(u + [float32(rx)*h, float32(ry)*h, h])
v = float32(u - a)
T0 = float32(B × v)
```

For the orthographic path, form `k=o*s`, `kx=k*xyScaleX`, `ky=k*xyScaleY`, retaining extended intermediate precision until the documented stores. Scale the first row of `C0` by `kx` and the second by `ky`, storing binary32; **leave the third row unchanged**. This differs from the normal color/depth camera mapping.

Store `tx=float32(T0.x*kx)` and `ty=float32(T0.y*ky)`, then:

```text
T.x = float32(offsetX*xyScaleX + pixelShiftX + (width//2 + .5) + tx)
T.y = float32(offsetY*xyScaleY + pixelShiftY + (height//2 + .5) + ty)
T.z = T0.z
```

The scene's separate normal-view shifts at+0x70/+0x74 do not participate in this shadow translation. The raised-ground control supplies them too, distinguishing these from shadow pixel shifts at+0x7c/+0x80. Width/height here are the internal surface dimensions, not the viewport or cropped mask dimensions. Native culling extents are `float32(hypot(C00,C02))` and `float32(hypot(C11,C12))`, each clamped to epsilon.

## Executed controls and precision boundary

Three complete original helper draws use the same owned Scout model/frame streams, zoom0, scene flags1 and256×256 internal surface:

| Control | Distinct mask pixels | Full mask SHA256 |
|---|---:|---|
| Facing2, ordinary flat scene |222|`3c26c4bbd927a4752e779a90f378c4ef9aade2b86755ca4d2aeb69f3aa4e9b20`|
| Facing3, ordinary flat scene |262|`e6fbd36a9089ca669acad7cc1aab9a0c141548242a01944aa33e05a6e32f0a06`|
| Facing2, translated/raised ground |233|`297f95a6a01232042e14e339f84268449402cc615b1acd2b5dbbb9a1fe4d0102`|

The third control changes position to(3,-2,4), ground height5, offsets(7,-3), normal shifts(2,1) and pixel shifts(4,-2) at the native prepass entry. It is an explicitly supplied discriminator, not a claimed natural gameplay position. Its matrix remains unchanged, while translation becomes(139.8265380859375,123.04536437988281,9998.9970703125).

The checker generates scene values, inverse light, plane, pre-scale matrix/translation, final matrix/translation/extents, then projects all59 decoded animation frames and rasters all65 owned geometry streams for each case. All three entire masks match. No measured projection, translation or captured mask is accepted as an input. Model frames/origins/streams remain owned-data inputs.

The reference models64-significant-bit x87 intermediate arithmetic and binary32 stores at the important producer boundaries. Host sin/cos matches the checked flat tilt and two facings; universal x87 transcendental identity, signed-zero bit identity, arbitrary slope inversion, perspective mode, animation and untested scale/clip ranges are not established here. Existing broader facing/zoom color-camera evidence is reused without claiming a new shadow matrix across that whole range.

`tmp/reference/check_scout_shadow_camera.py --verify-only` verifies this handoff. New immutable bundle: `tmp/reference/scout-shadow-camera-evidence-20261019`. Original pixels/streams stay private. Production files and previous frozen bundles are unchanged.
