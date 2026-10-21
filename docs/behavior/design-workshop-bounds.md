# Workshop attachment and bounds store schedule

2026-10-20. Additive supplement to [the frozen renderer contract](design-workshop-renderer.md).
Executable SHA-256 remains
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The executable and port execute independently. No production source changed.

Notation: `E(op)` rounds each arithmetic operation to the x87 64-bit
significand, nearest-even; `S(x)` stores to binary32, nearest-even. Control
word is `037f`. `MV` and `MM` return binary32 values using the documented
[vector and cell-specific matrix schedules](cvr-matrix-arithmetic.md).
Each vector formula below applies componentwise unless a matrix is shown.
The executable's finite tested arithmetic is implemented in the independent
[bounds reference](workshop_bounds_reference.py).

## 631a60: attach before bounds or camera projection

Arguments are `(modelObject, attachmentOffsetPointer, requestedFrame)` with
the destination scene in ECX. Select the frame using signed remainder of
`requestedFrame / modelFrameCount`; tested requests 0, 1 and 4 with three
frames select 0, 1 and 1.

In source object order, append each record:

```text
assembledFlag       = sourceFlag[selectedFrame]       // byte copy
assembledLocation[j]= S(E(sourceLocation[j] + attachmentOffset[j]))
assembledMatrix     = sourceMatrix[selectedFrame]     // nine word copies
selectedShadeTable  = loadedModelPalette + 0x302
geometryPair        = source object's two geometry words
```

No rotation or scale applies to the attachment offset here. Do not add it
again in bounds or the indexed prepass. The six synthetic records include a
`16777216 + 1` translation to establish the binary32 store boundary.

The destination count increments after each complete record. Count exactly
200 stops further appends with result 1, retaining earlier appended records;
ordinary completion returns 0. Null model/offset returns `0x10`, and an
unloaded model returns 7. These guards are source-traced; the new native
controls exercise successful attachment and positive frame wrapping.

## 655210: build the origin and three edge vectors

Inputs are `(scene + 0x18, frame, minOut, maxOut)`. For each assembled object,
let `R` be scene rotation, `p` scene position, `s` scene/object scale,
`L` assembled frame location, `F` frame matrix and `g` geometry scalar.
Geometry `vectors[0]` at header `+14` is the three signed16 dimensions `d`;
`vectors[1]` at `+1a` is the signed16 corner offset `b`.
`vectors[2]` does not participate in this bounds routine.

```text
q    = S(E(g * s))
u    = MV(R, L)                         // store each component
v[j] = S(E(u[j] * s))
t[j] = S(E(v[j] + p[j]))
D    = MM(R, F)                         // nine binary32 stores
bq[j]= S(E(b[j] * q))
w    = MV(D, bq)                        // store each component
C[j] = S(E(w[j] + t[j]))

ax   = E(d[0] * q)                      // retain extended value
by   = E(d[1] * q)                      // retain extended value
cz   = S(E(d[2] * q))                   // explicit binary32 spill
A[j] = S(E(D[j,0] * ax))
B[j] = S(E(D[j,1] * by))
Z[j] = S(E(D[j,2] * cz))
```

The asymmetry in `cz` is observable. Do not pre-store `ax` or `by` to
binary32. Native `655540(C,A,B,Z,localMin,localMax)` receives these four
stored vectors. The first object copies its local bounds to the aggregate;
later objects update aggregate coordinates only on strict `<` / `>`.
An empty object list leaves the caller's output buffers untouched.

For Workshop draw flags `0x17`, `631370` temporarily supplies identity `R`
to this routine, then restores the actual yaw rotation. Grouped geometry
still uses its object header bounding vectors here, not per-group voxel
stream origins or measured rendered pixels.

## 655540: corner walk and rounding points

Initialize both min and max by copying `C`. Keep a working vector `V`, then
visit the following seven further states in order:

| State | Update from previous state | Forced binary32 stores before comparison |
| --- | --- | --- |
| 1 | `V = C + A` through vector-add helper | X, Y, Z |
| 2 | `V = E(V + B)` | none |
| 3 | `V = E(V - A)` | Z only |
| 4 | `V = E(V + Z)` | none |
| 5 | `V = E(V + A)` | none |
| 6 | `V = E(V - B)` | none |
| 7 | `V = E(V - A)` | Y and Z |

At each state, compare each working coordinate with the current stored
min/max. On a strict improvement, store `S(V[j])` into that bound. That
conditional store does **not** replace the retained working value, except
for the forced spills in the table. State 3's spilled Z is also the input
to state 4; state 7's spilled Y/Z are used in its comparisons. Working X
at state 7 remains extended through its comparison.

Thus the corner sequence is `C`, `C+A`, `C+A+B`, `C+B`, `C+B+Z`,
`C+A+B+Z`, `C+A+Z`, `C+Z`, with the exact chained operations and spills
above. Recomputing those expressions independently, or replacing the walk
with `C + min/max(0,A) + min/max(0,B) + min/max(0,Z)`, changes its rounding.

## Center subtraction and following indexed prepass

After restoring scene rotation, `631370` calculates each midpoint as
`mid[j] = S(E(E(min[j]+max[j]) * 0.5))`, then separately stores
`p[j] = S(E(p[j]-mid[j]))`. It does not assign `p = -mid` unless the input
position happens to be zero. The frozen constructor controls produce the
positions and final camera translations in the renderer contract.

The following `653fb0` camera prepass receives the centered scene position,
assembled locations and copied frame matrices. For the admitted no-perspective,
scene-scale-1 Workshop fixture, its stages are:

```text
baseProjection = MM(cameraMatrix, sceneRotation)
projection rows X/Y *= cameraScale      // separate binary32 stores
projection row Z   *= depthScale        // separate binary32 stores
relativeScenePos = S(scenePosition - cameraPosition)
cameraPositioned = MV(cameraMatrix, relativeScenePos)
translation.X/Y = S(E(E(cameraPositioned.X/Y * cameraScale) + 128.5))
translation.Z   = S(E(E(S(cameraPositioned.Z - near) * depthScale) + depthFirst))

drawLocation = Add32(MV(projection, assembledLocation), translation)
drawMatrix   = S(E(MM(projection, assembledMatrix) * geometryScalar))
```

Use the existing renderer contract for the actual camera matrix, scale,
depth endpoints, light operands and frame records. This simplified prepass
formula is scoped to scene scale 1 and zero shifts/offsets in those four
controls; the `655210` bounds formula above also checks non-unit scene scale.

## Native receipts

Fourteen self-authored complete `655210 -> 655540` controls check origin,
all three edge vectors and literal binary32 min/max words. A further two
receipts supply the frozen Scout/Synthmetal assembled frame and geometry-header
operands to the original bounds routine. The independent reference matches
all 119 per-object origin/edge sets and both complete aggregate bounds,
which also equal the earlier full-render capture's min/max.

Three complete `631a60` calls check six synthetic attached records, positive
frame wrapping, binary32 translated locations, unchanged matrices, flag copies,
geometry pairs and selected shade-table pointer. The synthetic controls do
not consume native expected outputs as algorithm inputs. The real-operand
replay consumes explicitly declared native boundary operands, not port output
and not prior native min/max expectations.

Private additive freeze:
`tmp/reference/independent-workshop-bounds-evidence-20261020/`.
Manifest SHA-256:
`6db4d85873d1d2976134c0f30f592ca83365bda7b5b71ab98074d3cdceca4d8f`.
Inputs and observations are separate. The renderer bundle `ee06051a...`
and every listed file were verified unchanged. No desktop capture was needed.
