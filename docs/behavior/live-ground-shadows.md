# Live ground shadows: map, selected card and roster

Ordinary live ground models draw **shadow first, body second in all three
views**. Selected and roster cards do not disable shadows. This extends the
[live design composition](live-ground-design-render.md),
[flat shadow camera](scout-shadow-camera.md), and
[Workshop strip](design-workshop-strip-raster.md) contracts.
[18 complete original controls](live-ground-shadows-cases.json) pair each of
Scout, Synthmetal Garrison and Rover in each view with an explicit body-only
control. Original executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

## Caller and enable rules

The ordinary dispatcher `56fd50` admits the supported chassis to `4d7860`
at `5701cc`. It skips that body block when caller flag `0x800` is set
(`570122..57012b`), or `0x80` is set (`570178..57017a`). The separate card
overlay pass uses `0x1800`, so it does not draw a second model or shadow.

`4d7860` derives the **different** surface-wrapper flags at
`4d7cb7..4d7cd8`:

```text
wrapperFlags = 1 | ((callerFlags & 0x2000) >> 12)
```

Thus ordinary map flags `3`, selected flags `0x48` and roster flags `0x400`
all become wrapper flags **1**. Adding caller `0x2000` gives wrapper flags
**3**, suppressing shadow generation and copy while preserving the body.
Do not pass the original card flags directly to `631370`.

`631370` requires initialized scene flag1, a non-null target and a nonempty
scene (`631377..6313b7`). Its argument bit2 controls scene shadow byte `+a4`
at `6313ed..631423`. The 3D traversal checks that byte through its admitted
scene descriptor at `654a03..654a0f` before submitting shadow geometry.
Shadow copy is gated by the same byte at `631658..6316ec`.

These conditions describe this ordinary renderer. A disabled whole body,
alternate chassis dispatcher, uninitialized/empty scene or separate overlay
pass must not acquire a synthetic shadow merely because the Unit exists.

## Placement and clipping

| View | Caller anchor | Zoom | Caller flags | Wrapper anchor | Copy offset | Target clip in new controls |
| --- | --- | ---: | --- | --- | --- | --- |
| Flat map control |128,128|0|`3`|178,178|50,50|`[0,0,768,256)`|
| Selected card |34,66|−4|`0x48`|71,103|−57,−25|`[14,66,130,242)`|
| First roster cell |156,197|−8|`0x400`|181,222|53,94|`[143,196,208,244)`|

The selected caller is `4c9a05`; roster is `4ce236`. Their clips are installed
at `4c99ab` and `4ce1e0`. The card coordinates are console-local; add511 to y
for the established 1024×768 screen layout. Later roster cells use their own
native cell rectangle and anchor; do not reuse the first cell's absolute x.

Map anchors and the viewport clip are runtime inputs. The new map target is
a declared software control, not a claim that every map viewport is768×256.
The earlier [populated map control](scout-shadow-producer.md)
has target clip `[0,0,1024,511)`, helper anchor624,288, wrapper674,338 and
copy offset546,210. Both follow the same rule.

Let `n/d` be the optional positive global scale override. The anchor delta is
`trunc(50*n/d)` when `n>0`, otherwise `trunc(50*(zoom+16)/16)`.
Add it to both caller-anchor coordinates. The internal surfaces are256×256;
both ordinary copies use `wrapperAnchor-(128,128)`, scale1/1. Clip to the
target's half-open clip rectangle and bitmap bounds. `631423..631529` also
maps the target intersection into the internal raster clip. There is no
extra fit of the transparent work surface into a card widget.

The source-confirmed scene flag `0x20` branch instead copies at
`wrapperAnchor-(64,64)`, scale1/2 (`6316a8..6316d2`, `631704..631724`).
The new controls use scene flag1; they do not newly validate that alternative.

## Blend, palette and backdrop

At `6316ec`, `5f1ce0` copies shadow bitmap `9c1ba8`, key0, using the lookup
at context `+54`. At `631734`, `5f12c0` copies body bitmap `9c1600`, key9.
For every admitted nonzero shadow pixel:

```text
target[index] = shadowLookup[target[index]]
```

Stamp value128 is a nonzero mask value, not alpha and not the lookup index.
Repeated geometry stamps set128; they do not repeatedly darken the target.
The completed mask is blended once. Body pixels other than9 then overwrite
the target, including any shadow beneath the body. Body transparency does
not consult depth.

The backdrop is the already-painted target. On the map that is the preceding
terrain/object composition. Cards use the shared console bitmap `7cbc04`:
the selected panel paints its inner frame/background style `7b0328` through
`5fbbd3` at `4c994a`; the lower roster region uses the same style at
`4cddf6`, with later selection-border handling. Preserve those target pixels
and blend their indices; do not composite against a new black or constant
background. The new controls deliberately use the declared diagnostic
pattern `(3*x+5*y+37) mod256`, including row padding. They are not full
console or terrain screenshots.

`4d7b7c` calls `632570(71db18)`, which copies256 bytes of the global shadow
lookup into the context. It is shared across model classes and factions;
it is not the faction material remap. The new controls execute original
`429e30/429710` through the warm cache prefix with read-only guest mappings
of the actual installed files. They supply the established active Gamma100
palette, SHA256
`c7129c7433e5d3028e4e3d7990e696fd9a5b321ebf24694cee6316e5d9a32ba6`.
The matching installed `shadow.tmp` produces lookup SHA256
`70b3ab988991323661c52a73dd8459d6b06202a9e8c6fec329e4f12b0fb7aa49`,
entry6=177, despite the explicit037f renderer profile.

The [precision/cache lifetime contract](design-workshop-shadow-precision-lifetime.md)
still applies: a newly generated Extended64 table differs at entry6=178.
Do not infer lookup contents from the current renderer control word or
regenerate on a facing, zoom or design change. The raw PCX palette used by
the earlier body-material controls differs from this active Gamma100 palette;
the new Spartan material row is regenerated against the supplied active
palette before rendering. Depth/geometry remain comparable, while colored
Rover hashes are specific to this palette/material profile.

## Composition and raster differences

Each mask uses the actual current scene geometry, frame transforms and
per-object scalar. Reuse the ordinary tuple/owner composer and flat shadow
camera, not a retained Scout silhouette. The shadow light remains separate
from the `(6,4,15)` body light. Facing0, phase0, reactor1 and abilities0 are
explicit in all new controls.

| Composition | Objects / color and shadow kernels | Map mask/body pixels | Selected mask/body pixels | Roster mask/body pixels |
| --- | --- | --- | --- | --- |
| Scout: Infantry/Gun/No Armor |59 /65|230 /341|148 /212|83 /113|
| Garrison: Infantry/Gun/Synthmetal |60 /66|266 /411|169 /248|88 /126|
| Rover: Speeder/Gun/No Armor |10 /24|689 /800|401 /473|197 /230|

Counts describe non-key internal pixels before body-over-shadow occlusion,
not final visible shadow area. Each body-only partner has zero shadow kernels
and pixels, with identical body color and depth to its shadow-enabled partner.
All nine complete final targets distinguish the two modes.

The Garrison adds its armor geometry to the shadow traversal. Rover changes
the scene composition and ordinary scale: map/selected/roster scales are
0.07,0.0525000021,0.035 in binary32. Scout and this Garrison use another
factor1.25, giving0.0874999985,0.0656250045,0.0437499993. Exact stored values
are in the fixture.

**Some Rover shadow kernels use2×2 stamps.** `6548b9..6548d9` selects the
small-footprint boolean from the selected geometry scalar times the current
ordinary scale: it remains1 when that product is at most0.5, becomes0 when
greater. This boolean reaches `659eb0`, then the shadow-kernel flags. It is
a per-object/scale decision, not a constant chassis property. Kernel flag1
selects one pixel; without it, the checked contained path stamps `(x,y)`,
`(x+1,y)`, `(x,y+1)`, `(x+1,y+1)`. Kernel flag`0x20` excludes the trailing
record. These are separate from caller flags.

The source branches are `688ff0..68900a`, `689273..689282` and the one/four
store loops at `689288..689310`. Scout/Garrison use the checked one-pixel
path; Rover exercises both sizes. The additive
[bounded reference](live_ground_shadow_reference.py) extends the earlier
Scout raster with those count/stamp flags. Compressed block flag`0x10`,
clipped shadow-kernel variants and out-of-surface2×2 stamps remain outside
this extension.

## Verification and scope

`tmp/reference/probe_live_ground_shadows_20261021.py` executes18 complete
`4d7860` helpers through both original bitmap-copy bodies, dirty cleanup and
normal return. It uses the original107-model initializer and actual faction
material producer, startup-derived Unit/design backing, a declared retained
custom127 armor tuple, supplied software bitmap state and rectangle services.
Map flag3 executes the original single-Unit representative helper; stack
links are−1. This is not a newly saved Workshop design or whole dispatcher run.

`tmp/reference/finalize_live_ground_shadows_20261021.py` independently
regenerates all nine shadow cameras and full masks from decoded model/frame
and stream inputs, then all18 entire indexed targets. Every ordered native
target write was checked during capture; all guards survive and original
cleanup leaves color9, depthFFFF and shadow0. Source `6325b0` clears tracked
color, clears the shadow bitmap, and conditionally retains depth; the final
outer cleanup resets depth and dirty bounds. The two cleanup calls are retained
privately per case.

Private immutable packet:
`tmp/reference/independent-live-ground-shadows-evidence-20261021/`.
It contains original indexed/depth/mask/target buffers, stream payloads,
native receipts, source windows, probes and public snapshots. Public JSON
contains scalar expectations and hashes. Animation, slope, alternate renderer
modes, whole UI composition and live-window presentation remain outside this
bounded addition. Production source and previous evidence freezes are unchanged.

Manifest SHA256: `10905d0b4badaa714b4f0a8f80ef97ce0ff14ebf8c5d128a3eddbabbcbd561c1`. All 199 frozen payloads rehashed successfully.


## GLSMAC integration - 2026-10-21

The ordinary composer now returns separate body and shadow planes for a supplied
live view. Shadow footprint selection uses each geometry scalar and the ordinary
scale, including the Rover's contained2x2 kernels. All18 complete patterned
native targets match, retaining key-zero lookup before key-nine body copy.
The saved Workshop strip reuses this same producer.

SMAC selected/roster cards consume that producer through the live scene cache.
The original console2 backing and style21/18 frames remain indexed until the
completed mask/body copies finish. Widgets receive a compact transparent overlay
at the native116x176 or65x48 content clip, retaining their texture independently
of the bounded512-entry card cache. Geometry changes refresh the console origin;
re-registration replaces its geometry listener, and teardown removes it.

The actual cache overlay over the indexed backing matches all12 complete native
model-plus-panel RGBA crops in [the original goldens](console-unit-panel-goldens.md).
Eight whole panel targets and their crops match the native frame controls.
The live GSE console paints the same frames and console-local pattern phase,
advances roster cells66 pixels, and loads the SMAC shell with the active software
palette. These changes do not install SMAC frame policy in SMAX.

The final ASan/UBSan SDL session loads the saved Campaign Guard world, moves the
Scout into its stack, then selects Campaign Guard with both native card contexts
visible. It exits0 without sanitizer reports or tracked OpenGL leaks. Eleven
focused native groups pass with18 production/three test files instrumented;
four affected GSE scripts pass. Targeted correctness analysis is clean for ten
production translation units/consumers, with the final listener change rechecked
in its two affected consumers. Other relevant CVR/console groups pass normally.
The session uses software graphics and no audio, not a performance acceptance.

Live map shadow composition remains open: its indexed terrain/object destination
and draw ordering must be connected before those copies are displayed. Slopes,
walking/attack animation, complete decorated-card/whole-screen comparisons,
SMAX presentation and long-campaign cache acceptance remain release work.
