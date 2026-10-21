# Workshop weapon mount and bounds inputs

2026-10-20. Static original-source checks and independent owned-asset decoding;
no new native painter, desktop or production edit. Additive private bundle:
`tmp/reference/independent-workshop-mount-bounds-evidence-20261020/`.
Manifest `c58bf20e3aafa85ef2fb57bac34d72550dd8553cb62419fa403e1d66a440564b`.
The 15-frame visual freeze remains unchanged.

## Infantry mount comes from AVGMTGUN.CVR

Original initializer `45cb80` loads **`AVGMTGUN.CVR`** into a temporary model
at `45cf11..45cf1a`. Installed asset SHA-256:
`42a68bfc39934fe2ad240facd50578815e4071e31f5c1f544e2a73bfe557e816`.
It contains **one object and two animation frames**. The initializer checks
loaded model frame count at header `+20` equals 2, obtains descriptor array
`+30`, then the first object's frame-location array at descriptor `+4`.

At `45cf34..45cf41`, it calls `641f10(frameLocations+12,frameLocations,7af698)`:

```text
mount[j] = StoreBinary32(frame1.location[j] - frame0.location[j])
```

Use the original x87 subtraction followed by a binary32 store per component.
There is no division, midpoint, matrix rotation, body-bound calculation or
additional Infantry offset in this producer. It is the displacement between
the marker object's first two frames. Runtime can derive it directly from
the player's decoded marker asset.

Independent CVR decoding reads outer scene chunk 4, frame-count chunk `304`,
first object `404`, its frame container `30404`, and the two positions in
`2030404`. This produces exactly
`(0.000003069639205932617,70.45306396484375,136.67886352539062)`, matching all
four existing VW01 attachment calls in the 15-frame receipt. The literal source
positions stay private. The tiny X follows from the actual frame values;
do not replace this asset-derived vector with a captured runtime constant.

The missing/unloaded model branch zeroes all three mount components. A loaded
model with frame count other than 2 takes the skip branch and leaves the prior
global vector unchanged; the valid installed asset takes the two-frame branch.
After extraction the temporary model is released. Weapon composer
`4d8e40@4d900c` later passes the retained vector to `631a60` for the Infantry
weapon. Main and weapon-card painters use the same mount vector.

## Bounds includes hidden frame descriptors

Common bounds routine **`655210` does not test frame flag `80`**. It walks
every assembled object in scene order and uses the requested frame's location
and matrix. Therefore the six hidden Impact descriptors remain in the bounds
calculation. Main and side-card centering both use this routine when requested.

Raster dispatch is separate: **`654770@654858`** tests that flag and skips
the object before choosing detail and drawing. This explains the Rover main's
41 assembled descriptors versus 35 draws, and Impact card's 29 versus 23.
Filtering the scene down to visible descriptors before measuring bounds changes
the original algorithm. Keep all assembled objects for bounds, and apply the
visibility flag at draw dispatch.

The existing [bounds arithmetic](design-workshop-bounds.md) remains unchanged.
Main flags `17` hex temporarily use identity scene rotation for bounds, while
side-card flags 6 use their actual fixed rotation. The chassis card's flags 2
do not request automatic bounds centering.

## VLIGHTS uses root geometry for bounds, selected LOD for drawing

`655210@65525c` reads the **root geometry pointer** from the assembled object's
geometry pair. It takes scalar at header `+8`, dimensions at `+14`, and corner
offset at `+1a`, and applies the existing bounds formula. It never calls detail
selector `652a50` and never follows the geometry's next-level offset.

For installed `VLIGHTS.CVR`, bounds therefore uses **level 0**, scalar
`4.800000190734863`, and that root header's vectors. It does not substitute
the geometry level selected for the current draw scale. The private check
records root vectors directly from the owned asset.

Draw dispatch instead calls `652a50(rootGeometry,drawScale)` at `6548a9`.
Under the existing [linked-detail contract](cvr-linked-detail-levels.md), the
Workshop main scale .14 selects VLIGHTS level 2, scalar `8.112000465393066`
with 565 records; the chassis-card scale .05 selects level 4, scalar
`13.70927906036377` with 184 records. Both match the existing native kernel
operands. Selection returns an interior geometry pointer without changing the
root pointer or source chain.

Preserve both representations in the port: root geometry for bounds and the
scale-selected detail pointer for drawing. Selecting an LOD while loading and
replacing the model's root header would lose the bounds inputs required here.
