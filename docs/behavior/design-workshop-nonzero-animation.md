# Nonzero Workshop yaw and Copter raster receipts

2026-10-21. Original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This extends the [animation controller](design-workshop-animation-controller.md)
with four complete original main-painter executions. No port rendering or
desktop capture is an expectation input. [Scalar cases and hashes](design-workshop-nonzero-animation-cases.json)
are public; owned asset bytes and rendered images remain private.

Private additive freeze:
`tmp/reference/independent-workshop-nonzero-visuals-evidence-20261021/`.
Manifest SHA-256:
`52a360088d8c333fdbc2373ffb5028131add72d5d3d66afe4781b52fe9da767b`.
There are224 payloads, four complete256x256 indexed/depth frames,
154 object projections and214 raster calls.

## Inputs and independence boundaries

Each control executes the original builder for selected saved custom slot127,
then `4490b0(127,0)`, followed by main painter `43f8c0(0)`. Owner1 uses
the same declared initialized Spartan material row as the existing general
visual receipt. Every tuple is Laser / Synthmetal / Fission, no abilities.
The initialized constructor scene is restored before each control. The x87
control word is037f. For the visibility predicate the Workshop is explicitly
a visible root (`+9c` bit1 set, `+c4=0`); this is a bounded composition fixture,
not a desktop/window-focus claim.

The independent checker derives composer operations from the source recipe,
decodes selected flags/locations/matrices directly from each owned CVR, and
derives the Infantry gun attachment from the two AVGMTGUN marker locations.
It checks all assembled objects, including hidden descriptors. It regenerates
root-geometry bounds, midpoint subtraction, yaw rotation, camera projection,
and every visible object's transformed location/matrix. Native bounds,
centered scene positions, assembled transforms and final projection are
verification outputs, never regeneration inputs. The camera's initialized
operands remain the explicitly declared established constructor/camera stage
from the renderer contract; this does not rerun an independent camera loader.
Geometry scalar selected for each draw remains a declared draw-stage operand.

A separate independently implemented raster loop consumes the recorded native
kernel-entry streams, origins, bases and lookup tables. It matches every
indexed byte and depth word. This is **staged raster verification**, not a
claim that an independent renderer generated all kernel operands from assets.
The new asset/bounds/view checks close the captured-final-transform gap for
these four cases; lighting/basis/kernel generation retains that stated boundary.

## Consecutive yaw with phase held

The two Infantry controls hold shared phase at1. Their input stored yaw words
are3d4ccccd and3dcccccd, the first control's output and the next input matching
exactly. Current drawing uses the input yaw; output words are3dcccccd and
3e19999a. The two frames differ at2,351 indexed pixels and2,415 depth cells.
They share the same asset-derived bounds and centered scene position:
`(-1.104278564453125,-164.7764892578125,-260.8135681152344)`.
These are check results, not production camera constants.

VI and ordinary Infantry armor both select frame1. The weapon and reactor
select0. This makes a yaw-only comparison: phase does not change between the
two independently reset controls. It is not a timing or elapsed-clock test.

## Visible Copter entering14 and15

Both Copter controls hold input yaw at3d4ccccd. VCTB selects frame14 for entry
phase14, and frame0 for entry phase15. The body call increments the shared
counter once after that copy, yielding15 and16 respectively. Subsequent
weapon/armor/reactor stages execute after this increment; these particular
Copter armor assets use frame0 in both cases.

The frames differ at3,364 indexed pixels and4,115 depth cells. Bounds and
centering also change with the rotor frame:

| Entry phase | Rotor frame | Centered scene position |
| --- | --- | --- |
|14|14|`(-0.90966796875,99.54336547851562,-197.83815002441406)`|
|15|0|`(-1.4998779296875,100.13116455078125,-197.83815002441406)`|

Recompute bounds for the actual composed frame before projection. Reusing a
phase-zero center or rotating a previously rendered bitmap does not satisfy
these receipts. The existing controller source/operand controls still govern
chassis-card-before-main ordering; this new four-frame raster set paints main
only. Speeder and additional Infantry phases remain bounded follow-ups if
needed, rather than an assumed universal modulo15 rule.

## Cached branch: authoritative byte inputs

Main painter `43f932..43f99f` first tests signed byte46 of the selected stored
52-byte design (`9b01e8+52*selected+46`, equivalently9b0216+52*selected).
If it is nonnegative, compare the complete stored tuple against the draft:
unsigned chassis36, weapon37, armor38, reactor39, and DWORD abilities32.
All five must match. Name, owner, cost and status flags are not part of this
tuple comparison. A match takes cached draw `56fd50`.

If that admission fails, `43f9a1..43f9c0` tests signed weapon icon byte
`9b6fb1+16*draftWeapon` (weapon record base9b6fa8, offset9).
Nonnegative plus draft chassis0 also takes cached draw. Otherwise use the3D
scene path and its per-paint yaw addition. Interpret the low byte as signed:
0..127 admit,128..255 do not. Zero is a real icon; testing greater-than-zero
is incorrect. A design icon alone does not bypass the draft-tuple check.

The cached call uses flags44 hex, without action bit20. Its idle-render path
does not make the3D main yaw addition. Do not infer a rotation from a Scout
name or a timer tick. Use the actual loaded selected record and draft bytes.
All21 installed stock weapon icon fields are-1, as established in the existing
[graphics-row contract](unit-graphics-rows-and-y-anchors.md). The original
custom builder sets design icon46 to255; use the saved record's actual field
for predefined or otherwise loaded records.

The current rules parser already exposes both source columns:
`UnitRulesTXTLoader::ParseUnits` stores ninth-column `UnitDesignRecord.icon`,
and `ParseWeapons` stores sixth-column `WeaponComponent.icon`, both int32.
`SmacPredefinedUnitCatalog::Build` narrows the design icon to its low byte,
retained as `SmacPredefinedUnit.icon`; its weapon table retains source icon
metadata. Narrow the weapon icon to a byte before this native signed-byte
predicate too. `SmacCustomDesign` in `SmacStartupDesignState.h` currently has
no icon member; its ordinary newly built custom record's native default is255.
Parser availability does not establish that the live Workshop caller already
passes these fields. That connection remains port work.
