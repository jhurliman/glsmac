# Workshop shared position and painter ordering

2026-10-21. Pinned original terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Scalar native receipts](design-workshop-shared-position-cases.json).

**Position belongs to the Workshop scene and persists across component calls.**
A zero-initialized position for every card matches the very first chassis
paint only. The later Morgan Scout target includes an additional full paint
and therefore a different, retained chassis position. This is engine scene
state, not a sprite-placement offset.

## Actual owner, initializer and producer

The scene is Workshop+1279c; its position is Scene+2c/30/34, equivalently
Workshop+127c8/127cc/127d0. Constructor4469a0@446cea calls630f20, whose
630fea/630fed/630ff0 zero those three words.

Chassis43de60@43df9f clears **Scene+13cc**, the assembled-object count.
It does not clear position. It assembles the chassis, sets scale0.05 and
rotation, then calls631370 flags2 at43e027. **Flags2 omit centering bit4**,
so this draw consumes and retains whatever position the scene already has.

In631370, flags4 enter631532..6315f2. Save rotation, optionally use identity
for bounds when flags10 is set, call655210(Scene+18,0,min,max), restore
rotation, compute each midpoint and store it as binary32. Then subtract the
stored midpoint from the current position and store binary32 again. Actual
position stores are6315de,6315e8,6315f2. Bounds include the current position;
do not replace this with a hardcoded model offset or a fresh-zero centering
calculation for every draw. The existing [bounds operation contract](design-workshop-bounds.md)
pins the internal x87 spills and object inputs.

Renderer cleanup6325b0 clears shared image/depth state; it does not reset
scene position. Rebuilding model descriptors also does not create a new scene.

## Native full-paint order and exact Scout values

441ba0@441cfa..441d49 paints **Chassis → Weapon → Shielding → Reactor →
Ability1 → Ability2 → Main → status → saved strip → data**. An independent
shielding pre-pass changes the state history. Carry the same scene position
through every actual draw in this order; any centering draw can update it.

For the declared stock Scout fixture, no armor or ability model is drawn.
The scene draw calls and positions at entry are:

| Painter | Flags | Fresh first full paint | Explicit second full paint |
| --- | --- | --- | --- |
| Chassis | 2 | `(0,0,0)` | `(6.5746765,55.8130035,-252.009903)` |
| Weapon | 6 | `(0,0,0)` | same retained main position |
| Reactor | 6 | Weapon's newly centered position | Weapon's newly centered position |
| Main | 17 | Reactor's newly centered position | Reactor's newly centered position |

The JSON records every component's exact incoming binary32 words, including
the small rounding differences caused by previous position. Both main draws
leave position **`(6.574676513671875,55.81300354003906,-252.00990295410156)`**,
words **`40d263c0,425f4084,c37c0289`**. These are outputs for this assembled
model and fixture, not constants to use for arbitrary designs.

The initialized first-open baseline has a zero-position chassis. The older
`morgan_scout` capture script first executes modal_open(0), then explicitly
runs another441ba0 with phase/main-angle reset to0. It does **not** reset
scene position. That image's chassis consequently uses the previous main's
center. Main yaw/phase reset alone does not make these histories equivalent.

## Position-only causal controls

Using a complete initialized native guest, restore the same entire scene
snapshot for each test and alter only its12 position bytes. Execute native
43de60. Retained-main versus zero position changes exactly **197 pixels**
within card-local `(47,18)-(56,54)`. Restore the retained position and all
**8415 pixels** of the99×85 chassis crop match again. All three chassis
calls have **zero writes to Scene.position**.

A native full-paint memory-write journal records exactly nine writes to this
position: the three centering stores for Weapon, Reactor and Main. Thus the
previously unexplained197-pixel repaint difference is causally resolved;
other retained scene/pointer fields need not be changed to reproduce it.

The declared Morgan-material variant also passes: retained position matches
**all8415 pixels** of the previous Morgan Scout chassis target crop; zero
position differs at197 pixels. The port report's196-pixel count is therefore
close but is not used as the native expected count. Native model ink bounds
against DESWIN move from `(47,18)-(56,43)` at zero to
`(47,29)-(56,54)` with retained position; use projection/state, not a fixed
11- or14-pixel patch.

## Lifetime, timer and cached-main boundaries

Use one value per Workshop scene/window, separate from picker scenes and
from the general-unit scene8f2790 used by console/cache rendering. A complete native4490b0(2,0) control preserves an explicit `(11,22,33)`
position marker. Draft selection does not reconstruct the scene. Ordinary full paints retain
position for the next paint.

Timer callback4468a0 calls **43f8c0 only**, then5ee450 with the main-preview
rectangle and4d4380 to resume audio. It does not call441ba0 or the component
painters. Its main scene draw can update the position that a later full paint
will inherit, while existing card pixels remain unchanged. OS events can
independently request a full paint; that is not the direct timer schedule.

A native4468a0 control confirms one WorkshopScene631370 draw, from the main
painter, with flags17. Presentation5ee450 and audio-resume4d4380 are supplied
service boundaries in that control; it does not claim OS message delivery.
A complete native630f20 initializer control zeros an independent scene
previously marked `(11,22,33)`.

The hide/close helpers447910,4495f0 and ordinary44990d cleanup stop/hide the
UI without writing position. They set mode7528f0 to0 on the ordinary close
path. The **next ordinary449630(mode!=2)** invokes4469a0 and resets position.
The special cached mode2 skips that constructor and preserves it. Three
complete4495f0 controls, with downstream hide/flush/timer helpers supplied,
retain a `(11,22,33)` position marker and set mode0. Mode1 intentionally
skips those cleanup calls; that exception does not zero the position. Bare
4468d0 clears yaw, controls, timer/fonts/backing; the actual scene reinitialize
is the later630f20 call inside4469a0. Do not use the yaw reset as a substitute.

When Main takes its cached/general-unit branch43fab5→56fd50, it bypasses
WorkshopScene631370 centering. Keep the position left by the preceding
component; do not inject the general renderer's separate position. The
installed Scout's icon255 takes the scene path used by these baselines.
A complete43f8c0 caller control with explicit synthetic icon0 and the
general Unit renderer supplied preserves both a `(11,22,33)` position marker
and yaw1.0, issuing no WorkshopScene631370 draw. This is a state/dispatch
control, not a new cached-sprite visual replay.
See the [cached branch contract](design-workshop-cached-preview.md) for its
exact predicate and explicitly overridden icon0 controls.

Private pixels, scene snapshots and write traces remain under
`tmp/reference/independent-workshop-shared-position-evidence-20261021/`.
Earlier frozen packets remain unchanged. No production code is modified.

## Port integration contract

A window-owned position array is consistent with the native ownership. Reset
it when reconstructing the Workshop scene; retain it across draft selection,
main-only timer callbacks, and ordinary repaints of that window. Pass it
through **every** ordered component/main call and return each actual
centering result, not only the main result. Chassis consumes without updating;
empty models leave it unchanged. Keep picker/general-unit scenes separate.
The full-frame renderer should preserve original card ordering, including
Shielding after Weapon. A timer repaint calls Main alone and carries its
updated position into the next full paint.
