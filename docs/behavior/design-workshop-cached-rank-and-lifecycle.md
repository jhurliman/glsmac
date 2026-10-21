# Cached row12 rank and Workshop reconstruction lifecycle

2026-10-21. Original executable SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
**Row12 variant0 is not a constructor guarantee.** Return the resolved native
scratch-preview rank from the backend. Do not take a real selected Unit's rank,
copy stored morale directly, or default the variant to0.

[Thirteen new native cases and reused reset evidence](design-workshop-cached-rank-lifecycle-cases.json)
execute complete original5d9ca0/5d8420 scratch initialization,5d8f50 query,
and56fd50 row/rank/sprite-address selection. Sprite painting is supplied.
No new full open/close desktop run is claimed. Open lifecycle is source-traced;
the actual reset fragment/native timer reset is reused from the prior24-case
controller freeze.

Private additive freeze:
`tmp/reference/independent-workshop-cached-rank-lifecycle-evidence-20261021/`.
Manifest SHA-256:
`4935d1897cb2eafe3e13c1cda26ad073ce9dec155baff6846fd6858038795ebb`.
There are17 payloads.

## Stored morale and effective rank

The [cached main recipe](design-workshop-cached-preview.md) constructs scratch
Unit2048 using design63 and the Workshop owner.5d8420@5d84df..5d84f5 reads
the low byte of the direct faction MORALE scalar at95a9a0+owner*59c, increments
that byte, and stores Unit+36 decimal. Thus stored morale is
`uint8(directFactionMorale+1)`. Neutral scalar0 gives stored1. Direct-1 gives0;
direct-2 wraps to255. The constructor does not apply production training or
first-prototype morale. Home Base word+46 is-1.

56fd50 calls `5d8f50(2048,0,0)` for positive owners. Row12 uses that effective
rank as variant0..6; other rows use variant0 even though morale is still
queried. Sprite address is `77fae0+44*(12*8+rank)`. Owner0 instead uses515d50;
that wild/turn-based path is outside these positive-owner controls.

For scratch plan byte42 other than11, the native query has two distinct paths:

* Negative weapon offense plus design ID<64 returns stored morale clamped0..6
  at5d9059..5d9096. Scratch ID63 meets that test even when the selected real
  design was custom. Later social/direct/immunity modifiers are bypassed.
* Conventional weapons use retained social MORALE at95d430+owner*20bc, direct
  MORALE at95a9a0+owner*59c, and negative-morale immunity bit8000 in
  95a9d0+owner*59c. The scratch is homeless and mode0, so there is no home
  Creche or riot adjustment.

For the conventional case, clamp retained social MORALE to-4..4, then map
`[-4,-3,-2,-1,0,1,2,3,4]` to`[-3,-2,-1,-1,0,1,1,2,3]`.
LetM be this modifier plus the signed direct scalar. If the direct immunity
flag is set, replace negativeM by0. Return clamp(unsignedStoredMorale+M,0,6).
The direct scalar participates both in initialization and this later query.
Direct `MORALE,0` sets the immunity flag even though its scalar is0;
`IMMUNITY,MORALE` is a separate social-axis mechanism. See
[direct faction MORALE](direct-faction-morale.md) and the
[complete conventional morale rule](scout-creation-morale-and-movement.md#retained-rating-and-effective-morale).

Scratch plan11 enters the Probe path first, using its own PROBE/project/tech
modifiers and minimum2. Main copies components, abilities and icon into63,
**not the selected design's plan42**. Scratch plan is persistent slot data.
The common new controls seed plan0; a separate diagnostic seeds11. This is
not a new universal initialization claim or exhaustive prior-writer inventory.
Do not substitute a selected real design's plan/ID when resolving this rank.

The [complete Probe follow-up](design-workshop-probe-rank.md) now specifies
all scalar/project/technology inputs and25 new complete native controls,
including current parser availability and seven stock direct-MORALE audits.

| Weapon/path | Direct MORALE | Retained social MORALE | Immunity | Stored | Variant |
| --- | ---: | ---: | --- | ---: | ---: |
| Psi |0|0|off|1|1|
| Psi |0|-4|off|1|1|
| Psi |0|4|off|1|1|
| Psi |2|4|off|3|3|
| Psi |-1|0|off|0|0|
| Psi |-2|0|off|255|6|
| Psi |256|0|off|1|1|
| Conventional |0|0|off|1|1|
| Conventional |0|-4|off|1|0|
| Conventional |0|2|off|1|2|
| Conventional |2|0|off|3|5|
| Conventional |0|-4|on|1|1|
| Probe plan11, neutral Probe inputs |0|0|off|1|2|

These are supplied faction inputs, not a rerun of parsing/social refresh.
All cases use owner1, icon3, scratch63, no home and query0,0. Plan0 applies
except the last diagnostic. The new controls verify actual selected sprite
addresses after native rank resolution; they do not render those sprites.

## Fresh open and already-open refresh

For a persistent DOM Surface, use a native reconstruction/reset epoch. Do not
reset yaw on every selection/edit or merely because the same surface becomes
visible again.

| Path | Local yaw/count reset | Timer behavior |
| --- | --- | --- |
|449630 entry,7528f0!=2|Yes, via4469a0→4468d0|Reset during reconstruction, then register on constructed/show path|
|449630 entry,7528f0==2|No constructor reset|Virtual+f8 refresh; skip new registration|
|Selection4490b0, edits, redraw|No lifecycle reset|Actual3D paints can advance yaw|
|Ordinary44990d..449938 close cleanup|No reset here|Stop timer; clear current pointer/exclusivity|
|Explicit4468d0 or destructor4499b0|Yes|Timer reset630290|

449659..449670 skips constructor4469a0 only for state2. Constructor first
calls4468d0 at4469d5. The later state2 branch449828..449844 calls virtual+f8
and jumps past62fe60 registration. It still performs intervening selection/
draft setup. Refresh may cause real paints and advance yaw; retaining the
lifecycle state does not mean freezing the angle across those paints.

4468d0 zeros yaw+12770 and paint count+ce8 and resets timer state. The reused
native control starts with yaw1.0, count99, a registered timer and shared phase
1000; yaw/count/handle become0 while shared phase remains1000. Fresh
construction therefore resets local yaw while preserving shared model phase.
Construction/show can cause paints before the first timer delivery, so the
first visible frame is not required to have yaw0.

The research-review/cached-window states can retain a constructed window
between calls.4495f0 has a state1 special path and ultimately sets7528f0 to0;
a later ordinary open reconstructs and resets. Normal state0 close-and-reopen
starts a new epoch. An already-constructed state2 refresh preserves it. Apply
the [existing timer contract](design-workshop-animation-controller.md) for
coalescing, stopping versus resetting, and shared-phase ownership.

## Fresh show paints before setting visibility

Source-only follow-up for the normal fresh, initially hidden Workshop with
the panel switch admitted. [Eighteen literal guards and three vtable slots](design-workshop-fresh-show-order-cases.json)
are checked; no new desktop or raster run was needed. Private freeze
`tmp/reference/independent-workshop-fresh-show-order-evidence-20261021/`,
manifest SHA-256
`f53df7e196fcffbbc11b70e36bbaa979a3b65c519852b569112a39c5bf10680d`.

The initial full refresh precedes setting Workshop+9c visibility bit0:

1. 449630@44984e invokes main virtual+4, resolved through68d328 to4478e0.
2. 4478e0 tests610d10; when hidden, it calls469fe0 on the+a14 panel object.
3. 469fe0 dispatches that panel's virtual0 with1. Vtable68d2d4 resolves it to
   447940. The active-panel switch helpers do not set the Workshop visibility
   bit before this callback's initial paint.
4. 447940@447960 calls main virtual+30, resolved to441ba0. This paints the
   chassis card at441cfe and main at441d34 while the fresh Workshop is hidden.
5. Only afterward,447940@4479ed..4479ef calls606850(main,3).6068a4 sets the
   visibility bit.60694b..606954 skips its synchronous virtual+30 repaint
   because flags3 includes mask1. The helper still invalidates the window.
6. After show returns,449630 registers the40ms timer at449861.

Consequently the initial chassis/main Copter body compositions both fail
their610d10 visibility gate and do not increment shared phase. For example,
considering this identified refresh alone with entry phase14, both use rotor
14 and the phase remains14. The3D main painter still uses old local yaw and
stores its next value; its yaw addition is not visibility-gated. Setting DOM
visibility only after those first compositions therefore matches this order.

An already-visible full refresh is different: chassis then main can advance
phase twice, as the existing visible controls prove. The first admitted
visible timer callback paints main only and can advance it once. Further
OS/expose/edit refreshes retain their ordinary paint-dependent effects; this
source trace does not prescribe exactly how many such paints occur before
the user sees the first frame.469fe0 also has active-panel early exits, so do
not apply this normal fresh panel-switch sequence to every possible call.
