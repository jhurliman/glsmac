# Workshop WEAPON side card: Missile and Probe empty-scene boundaries

2026-10-21. Source-only urgent caller clarification for original terran.exe
SHA-256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
No new model matrix or pixels. Existing
[composition families](design-workshop-stock-composition-families.md) identify
the reused model objects; this supplement traces their live side-card caller.

Main side-card callback is **43e2b0**, not generic picker43bc40. At43e40e it
zeros scene's model count (`Workshop+13b68`) before choosing the part:

| Draft / context | Native side-card call | Intended result |
| --- | --- | --- |
| Weapon0, any chassis | direct632060(7af880,0) | Hand Weapons scene; this branch precedes chassis handling |
| Missile chassis8, payload weapon13 or20 | 4d8e40(owner,8,weapon,scene) | Empty scene, deliberately |
| Infantry0, Probe weapon18,7528f8==0 | 4d8e40(owner,-1,18,scene) | Independent **ptmod.cvr**, object7af7f8 |
| Infantry0, Probe18,7528f8!=0 | 4d8e40(owner,0,18,scene) | Empty gun part; full-body composition owns the Probe body |

The exact Probe exception is `43e444..43e464`: it tests **weapon ID18**, then
global7528f8. Only when the global is zero does it substitute chassis-1.
All other nonzero-weapon side-card calls use the draft's actual chassis
(`43e466..43e47c`). Do not globally pass-1 for every weapon card.

`4d8e40@4d8e80/4d8ee6` checks chassis==8 and exits at4d9111 **before** its
weapon13/20 payload branches at4d8eec..4d8f2b. Thus the ordinary Missile side
card does not show an independent payload model. A request for the independent
payload via chassis-1 reaches those branches, but is a different caller input.
The generic weapon picker also has its own direct payload branches; that does
not make it the intended main side-card scene.

For Probe mode11, the mode table4d9118 selects4d90c6. Chassis0 exits without
adding anything; any nonzero chassis, including-1, copies object7af7f8 into
the scene with phase/frame0 through632060. That object is the existing
ptmod.cvr independent equipment model. It is not the complete VPT.CVR Probe
body used by whole-unit assembly. No new tint/attachment recipe is introduced.

The side card still calls631370 on the resulting scene (`43e4f9..43e502`)
and then draws its ordinary localized name/stats, even when model count is0.
An empty part must therefore produce a safe empty render, not an exception
or an invented whole-unit/payload fallback. The existing card background
copy/clear happens before composition.

There are **no5fd990/5fd9e0 sprite-underlay calls anywhere in43e2b0**. Those
generic-picker underlays belong to43bc40. The main side card sets scale.15
for combat mode<3, .06 for terraformer mode9, and .08 for other modes. Thus
Probe mode11 and payload13's mode0 select.08 and.15 respectively even if the
model count is0. Yaw remains3f05f92d; draw flags6. These scale selections do
not authorize populating an intentionally empty scene.
