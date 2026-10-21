# Workshop timer, redraw, yaw and model phase

2026-10-20. Original `terran.exe`, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Private freeze `tmp/reference/independent-workshop-animation-evidence-20261020/`,
manifest `a6c8dba7ba04f4fdd4c97cd837bdd379b72cb0031917ccc1dfdb5338dc7d0634`.
**24 focused native controls pass**, covering original timer registration,
notification/dispatch/callback, visibility recursion, yaw producer, body/armor
phase selection, repeated paint effects and reset. OS, painter and model-copy
boundaries are supplied where stated. No desktop, previous raster matrix or
new nonzero-phase pixel controls were run.

The Workshop requests a **40 ms redraw timer**. It does not derive model phase
or yaw from elapsed time. Main 3D yaw advances per paint. Model phase is shared
with other rendering; the Workshop-specific phase writer is a visibility-gated
Copter body composition. These are separate state transitions.

## Timer registration and delivery

Timer object is `Workshop + 12774`, normally `7509ac`, for main object
`73e238`. Open `449630@449851..449861` calls
`62fe60(callback=4468a0, argument=0, period=40, resolution=5)`.
The exact object address is computed from the object; do not use the numeric
address as an engine identity.

`62fe60` uses the multimedia registration import for periods below 50. Native
arguments are `(40,5,630330,timerObject,1)`: callback adapter, object pointer
and periodic flag 1 are retained. It stores callback at timer `+8`, period
at `+18`, resolution at `+20`, handle at `+4`, and clears pending `+1c`.
The native registration control receives supplied handle 17 and verifies all
five API arguments. This establishes the requested schedule, not measured OS
precision or a promise of 25 displayed frames per second.

`630330` notification checks a nonnull timer pointer and optional exclusive
timer global `9c15dc`. If another timer owns exclusivity, notification posts
nothing. Otherwise, if pending `+1c` is zero, it posts application message
`401` with the timer pointer and sets pending to 1. Further notifications
coalesce while pending. Timestamp arguments are unused.

Application dispatcher `6094d0@609b39..609b8e` first clears pending, then drops
the callback if inhibit global `9c0084` is nonzero or another timer owns
`9c15dc`. For this periodic registration it calls timer `+8` with argument
`+14`. It does not replay missed intervals or inspect the notification time.
Four notifications with timestamps 0,40,4000,FFFFFFFF yield one post and one
callback in the new Workshop-specific control. Inhibited/excluded dispatches
clear pending without painting.

Actual callback `4468a0`:

1. Read current Workshop pointer `7528f4`; call main painter `43f8c0(0)`.
2. Read the pointer again; call `5ee450(&Workshop.a9c,0)` for that preview
   rectangle.
3. Tail-call `4d4380` on the shared UI object `8d0b88`.

It does not repaint every component card. The controller control executes this
callback with those presentation boundaries supplied and verifies their order.
No elapsed-clock query occurs in any of the 24 controls.

## Visibility, focus and lifecycle

Neither this notification adapter, dispatch branch nor Workshop callback
explicitly queries Windows focus. An admitted callback still reaches its
painter when the Workshop's visible bit is clear; that case is verified with
painting supplied. Global inhibit/exclusive-timer behavior remains applicable.
This is not a claim about live minimized-window delivery or compositor policy.

The predicate used by the Copter composer is `610d10`, **visibility through the
widget hierarchy**. It requires `object+9c & 1`, then recursively requires the
same from the parent pointer at `+c4`, until a null parent. It does not call
GetFocus or GetForegroundWindow. Five complete predicate controls cover hidden
self, visible root, hidden parent, visible parent and hidden grandparent.

Normal open has an existing-window path when state `7528f0==2`: refresh through
the window virtual method and skip new timer registration. Thus an already
open refresh is not a fresh timing epoch. On the constructed/open path the
timer is registered after window setup. Actual intervening UI paints determine
how many yaw steps have already occurred before its first delivery.

Reset `4468d0@4468fe..446917` zeros stored yaw `+12770` and local main-paint
counter `+ce8`, and calls timer reset `630290`. This reset runs during Workshop
construction and specified cleanup paths; it is not part of every selection.
The new bounded reset control executes the actual timer reset and preserves
shared model phase 1000.

Timer stop `630240` clears the handle, but retains callback/configuration and
pending. Timer reset `630290` also clears callback/configuration/pending and
restores default resolution 5. Both preserve Workshop yaw and shared phase.
Controls check these separately. Close paths `4495f0@449607..44961c` and
ordinary open teardown `449630@449922..449933` clear current Workshop pointer
and stop the timer, then call `630380`, which clears exclusive timer global
`9c15dc`. This is not a pending-message flush. Do not infer a queued-message
lifetime guarantee from stopping the timer helper alone.

## Main paint counter and yaw

Each main painter entry increments local DWORD `+ce8` at
`43f922..43f92c`. Four bounded entries produce 0→4 without changing shared
phase. This is a paint count, not a wall-clock frame index.

Only the 3D scene branch at `43fac2..43faee` performs the yaw step:

```text
renderSceneRotation(storedYaw, 0, 0)
storedYaw = StoreBinary32(storedYaw + binary32(0x3d4ccccd))
composeBodyWeaponArmorReactor()
```

The constant is `0.05000000074505806` radians. Current drawing uses the **old**
yaw; the addition prepares the next paint. No elapsed-time multiplication,
2π reduction, clamp or explicit wrap occurs in this producer. Native x87
controls use word `037f`. Cases at yaw zero, approximately 2π, and 1048576
verify exact stored binary32 results. At the last value, float spacing causes
the 0.05 addition to store the unchanged value.

General window refresh, component edits and accepted Rename can also call this
painter. They can add a yaw step between timer deliveries. Do not implement
rotation solely as `elapsedSeconds * angularVelocity` or advance a second
independent yaw in the timer callback.

The painter also has a cached-unit/sprite branch through `56fd50`, which skips
the 3D yaw addition. Its source guard first accepts a selected record with
nonnegative signed icon byte 46 whose complete component tuple matches the
draft. Failing that, it still takes this branch if weapon byte `9b6fb1 + we*16`
is nonnegative and draft chassis is Infantry. Otherwise it takes the 3D branch.
This is a byte-level source contract; those weapon bytes are not inferred from
display names.

The Workshop cached draw supplies flags `44`, without action bit `20`.
`56fd50@57019e..5701cc` therefore uses ordinary idle renderer `4d7860`, rather
than action renderer `4d74d0`. A timer tick must not automatically advance the
action-render phase just because it paints a cached Unit.

## Shared phase and per-model frame selection

Shared DWORD `8f3c60` is not Workshop-local. The indexed literal reference
inventory finds these named producers:

| Producer | Behavior |
| --- | --- |
| Graphics/model initialization `4d6210@4d64dc` | Store 1 |
| Action renderer `4d74d0@4d774c..4d775c` | Increment after assembly |
| Copter body `4d88e0@4d8d89..4d8d97` | Increment if `610d10(73e238)` succeeds |

This inventory is source evidence for named direct references, not a universal
proof about arbitrary bulk memory writers. Previously checked action-render
controls are reused; that matrix was not repeated. Workshop initialization,
timer registration, callback and reset do not reset the shared phase.

For Copter, rotor `VCTB.CVR` is copied using **entry phase%15**, then the
visibility predicate and possible increment occur. Armor and later composition
stages see the updated global. With entry phase 14, visible Workshop, two
complete body calls produce rotor arguments 14 then 0 and final global phase
16. With visibility clear, a body call at 14 leaves phase 14. A separate
control verifies the raw 32-bit increment `7FFFFFFF -> 80000000`.

There is no global rollover at 15. Native instructions use signed remainder
for frame selection and raw 32-bit increment for phase. Negative/overflowed
phase can yield negative frame indices; the overflow control observes the
producer with model assembly supplied and does not certify safe rasterization
from negative phase. Normal nonnegative phase is the admitted frame domain.

Both main composition and the chassis card call the same body helper. Hence a
Copter chassis-card paint can advance phase, and a following main paint can
advance it again. Full refresh `441ba0` paints the chassis card before the main
preview. The timer callback paints only main. A fixed “one phase increment per
40 ms” rule would miss this paint-dependent behavior.

| Composer/model | Frame argument | Owned scene frame count | Effective nonnegative frame |
| --- | --- | --- | --- |
| Body VI | phase%15 | 17 | phase%15 |
| Body Probe VPT | phase%15 | 32 | phase%15 |
| Speeder `vsptb`, `vsptf` | raw phase | 17 each | phase%17 |
| Copter VCTB | phase%15 | 16 | phase%15, then possible phase increment |
| Ordinary Infantry `vipta00` armor | phase%15 | 32 | phase%15 |
| Infantry `viptawal`, `viptasgn`, `viptapsi` | raw phase | 32 each | phase%32 |
| Weapon/reactor stages in this pass | 0 | Asset-specific | 0 |

The last reduction comes from copy/attachment methods themselves:
`632060@632090..632098` and `631a60@631a97..631a9f`, with equivalent tinted
variants, divide the signed frame argument by loaded header frame count.
Counts above are independently decoded from the owned CVRs and hashed in
`owned-frame-counts.json`. **Do not assume every asset has 15 frames**, or use
all its frames when the caller explicitly restricts it to phase%15.

Complete body/armor operand controls cover phase 14/15 on Infantry, raw phase
1000 on Speeder, and Copter visibility/increment/overflow. They execute the
actual composers with scene-copy boundaries supplied; they establish selected
arguments and state changes, not nonzero-frame pixels.

## Remaining independent raster controls

Existing [general](design-workshop-general-visuals.md) and
[stock-family](design-workshop-stock-composition-families.md) full color/depth
receipts remain phase-zero evidence. The following are the focused next checks;
none should take captured camera centers or final transforms as runtime inputs.

| Priority | Nonzero control | What it must distinguish |
| --- | --- | --- |
| 1 | Visible Copter, two main paints entering at phase 14 then 15, plus chassis-card→main ordering | Rotor 14→0; one increment per body composition; next stages see updated phase |
| 2 | Infantry combat with ordinary armor at phases 1,14,15; Probe at 1 and 15 | Body/ordinary armor phase%15, independently decoded frame transforms/flags |
| 3 | Speeder main or chassis card at phases 16 and 17 | Wheel frame 16→0 from 17-frame asset, not a universal modulo15 |
| 4 | Infantry structural armor at phases 31 and 32 | Armor frame 31→0 while body uses its separate modulo15 |
| 5 | Main 3D preview from two consecutive stored yaw values with phase held | Drawing uses old yaw; exact binary32 stored update and bounds/view regeneration |

These controls should hold unrelated yaw/phase inputs explicitly and exercise
the real controller ordering where relevant. They are a bounded nonzero-frame
gate; they do not require rerunning all component combinations or the desktop.

Private disassembly includes opcode bytes and complete timer/callback/painter/
composer/visibility/copy functions, plus open/reset/close and cached/action idle
dispatch. No recommendation about live OS focus, minimization, scheduling
precision or renderer frame rate is inferred from the offscreen tests.
