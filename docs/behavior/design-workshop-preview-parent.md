# Workshop preview and parent layout supplement

2026-10-20, same original SMAC executable as
[the first path](design-workshop-first-path.md).
The first frozen bundle is unchanged. This additive private bundle is
`tmp/reference/independent-workshop-preview-parent-evidence-20261020/`, manifest
SHA-256 `7f75516cebacf88ecf410c93fc66fcbf1112b1703db5e7207eaf506d30d2c49c`.

This supplies a genuine original-x86 edited Infantry / Hand Weapons /
Synthmetal / Fission / zero-ability main preview at a supplied 1024×768 host
profile, plus a Synthmetal Rover comparison. Original PCX/CVR assets, original
scene construction and original rendering execute. These are offscreen
emulator client rasters. They are not a booted whole-game screenshot.

## Correction: identify the caller before choosing a renderer

The first handoff incorrectly attributed ordinary-unit wrapper calls to its
Scout main preview. Its receipt actually shows `5d9ca0` returning to `4481b0`
in the console design strip; the main preview's scale call returns to
`43faff`, proving that the stock Scout **already used the Workshop scene**.
The first-path document now carries this correction; frozen bytes are intact.

`43f8c0` still has both source branches. The ordinary-unit branch requires the
nonnegative design-icon/equal-component case or the alternate qualifying
Infantry weapon case. Do not translate that into “unchanged Scout uses map
renderer.” Stock values must satisfy the actual branch. In both new edited
controls the main call enters `4466c0` from `43faf3`, then draws `631370` from
`43fb3a`. Ordinary-unit calls returning to `4481e4` during the same complete
paint belong to the separate design strip.

## Edited Infantry scene, exactly observed

Scene object: `73e238 + 1279c = 7509d4`.
The main composition segment is bounded by `4466c0@43faf3` and the subsequent
`6324f0@43faff`; small component panels also use this scene, so collecting all
model calls for that object without the call interval mixes different views.

The original composition calls execute in this order:

| Routine | Arguments relevant to this design |
| --- | --- |
| `4d88e0` | owner 1, chassis 0, weapon 0, armor 1, reactor 1, scene |
| `4d8e40` | owner 1, chassis 0, weapon 0, scene; `7528f8` is 1 only around this call |
| `4d9130` | chassis 0, weapon 0, armor 1, extra flag 0, scene |
| `4d95f0` | chassis 0, weapon 0, reactor 1, ability mask 0, scene |
| `4d8700` | ability mask 0, scene, owner 1 |

Loaded model-object provenance is independently recorded by `630d10` calls.
Within that main composition segment, `631a60` attaches:

| Object | Owned asset | Observed frame |
| --- | --- | --- |
| `7af6c8` | `VI.CVR` | 0 |
| `7af808` | `vipta00.cvr` | 0 |
| `7af820` | `viptr00.cvr` | 0 |

The separate Hand Weapons panel attaches `vw00.cvr` from `43e43d`; do not add
that panel model again to the main Infantry composition. Frame zero is an
observed fixture phase, not a promise that all in-game animation phases are
zero. `4d88e0` includes phase selection from global `8f3c60`.

Before composing, the main painter calls `6324b0(angle,0,0)`. It advances stored
angle by float bits `3d4ccccd` (~0.05) per invocation, sets scale bits
`3e0f5c29` (~0.14), then calls:

`631370(clientCanvas, 295, 164, 0x17)`.

These centers are client-relative, from integer averaging of the unchanged
preview rectangle `(140,9)-(451,320)`. Constructor scale ~0.18 is not the
actual final main-draw scale. In the two sequential controls, the stored angle
bits change `3d4ccccd -> 3dcccccd` and `3dcccccd -> 3e19999a` respectively.
Previous bounded paints affect phase; screenshot comparisons must declare it.

Constructor light `6323e0` for this scene receives:

* Vector bits `80000000 45dce204 45dd0f0f`, approximately
  `(-0, 7068.251953125, 7073.88232421875)`.
* Row-major matrix bits
  `3f800000 00000000 00000000 / 00000000 3f351765 bf34f27f /
  00000000 bf34f27f bf351765`.

The matrix starts from the original `(0,0,~0.785)` angle setup; its final
vector is negated and scaled by `-10000.0` in the original operation order.
The ordinary-unit scene is a different object, `8f2790`, with a different
recorded light matrix/vector. Reusing that scene's light is incorrect.
`6324b0` is rotation; `6323e0` copies light vector/matrix. Keep them distinct.

Private `edited_synth.png` contains a visible model, Synthmetal panel, 1-2-1
stats and COST 20. This is raw 2-row Workshop cost × mineral multiplier 10;
it does not override the first handoff's 3-row unprototyped production price.
Private `edited_synth_rover.png` exercises the composition with chassis 1;
its rendering is not evidence of Speeder picker eligibility without its tech.

## 1024×768 geometry: two parents, not a stretched client

The original bounded console constructor has now also loaded its native rails
through `466f98`. Its window is `(0,511)-(1024,768)`. The Workshop client
window is `(216,168)-(839,555)`, still 623×387, with the owned 623×368 artwork.
Neither image should be resized to 1024×768.

Constructor parent links explain the apparent out-of-bounds positions in the
first capture. Fourteen category buttons and navigation belong to console
`7cb7c0`; bottom action buttons belong to Workshop `73e238`. Bounds below are
the **native constructor geometry in this controlled fixture**, not a claim
that all controls are already visible/enabled in a booted modal lifecycle.

| Widget | Parent-relative rectangle | Resulting screen rectangle |
| --- | --- | --- |
| Apply (ID 1) | `(27,321)-(132,341)` in Workshop | `(243,489)-(348,509)` |
| Prototype (ID 2) | `(135,321)-(240,341)` in Workshop | `(351,489)-(456,509)` |
| Retire All (ID 3) | `(351,321)-(456,341)` in Workshop | `(567,489)-(672,509)` |
| Retire (ID 0) | `(459,321)-(564,341)` in Workshop | `(675,489)-(780,509)` |
| Done (ID 4) | `(27,347)-(132,367)` in Workshop | `(243,515)-(348,535)` |
| Rename (ID 5) | `(135,347)-(240,367)` in Workshop | `(351,515)-(456,535)` |
| Obsolete (ID 6) | `(243,347)-(348,367)` in Workshop | `(459,515)-(564,535)` |
| Upgrade (ID 7) | `(351,347)-(456,367)` in Workshop | `(567,515)-(672,535)` |
| Cancel (ID 8) | `(459,347)-(564,367)` in Workshop | `(675,515)-(780,535)` |

Category IDs 9..15 occupy the first console column:
`x=786..898`, `y=87+23*r ..108+23*r`, `r=0..6`.
IDs 16..22 occupy the second: `x=901..1013`, same y progression.
Add **511** to these y coordinates for this screen profile. For example,
ID 9 is `(786,598)-(898,619)`, not `(786,87)-(898,108)` on screen.
Navigation IDs 23/24 start with zero-size constructor rectangles and are
runtime-positioned; those constructor values are not useful hit targets.

The status/action explanation painter `43d730` draws into console canvas
`7cbc04`, using `+a4c` `(266,70)-(759,168)`, mapping to screen
`(266,581)-(759,679)`. It is not the saved-design strip. The saved strip uses
`447d40` and `+b0c` `(8,192)-(764,248)`. The category outer frame `+a8c` is
`(783,60)-(1016,248)`; its heading `+a6c` is `(786,63)-(1013,84)`.

The later [saved-strip controls](design-workshop-saved-strip.md) continue native
console initialization past this fixture's stop to establish nonzero arrow
bounds. They supersede any visible-thumbnail inference from this receipt's
`(30,207)` model call and zero arrow rectangles. Main component rectangles
remain client-local; add `(216,168)` only at composition time.

The Workshop parent pointer is a window-base subobject at `943294`, derived
from map object `91f500`. Reading the most-derived object's `+13c` as if it
were its WindowBase rectangle is incorrect. Both objects are included in the
private control snapshots; the full map parent is not initialized here.

## Fonts and clipping

Control snapshots now include the selected font pointer at canvas `+52c`,
guest HFONT and its GDI font-record ID. All 25 created Workshop buttons in
this fixture select `7ece2c`, record 149: **Arial, height -14, weight 700**;
host measured height 16, ascent 13, descent 3. This differs from the main
component caption font (Arial -12 bold) and main design-name font (Arial -16
bold). Do not infer every widget's font from a single title screenshot.

The edited design name uses record 154, baseline 53 at x195, clipped to
`(140,9)-(451,320)`. Its cost caption uses record 153, baseline 69 at x268,
with the same clip. A chassis caption uses record 153, baseline 18 at x54,
clipped to `(30,7)-(129,92)`. The full private glyph journal records every
rendered string's font, baseline, color, destination DIB and clip. These
coordinates are client-local; no host DPI conversion has been inferred.

`621620` obtains default control font/state from native theme globals and
`5f3a70`; the constructor's later styling loop sets `7ece2c`. Preserve native
style/clip selection and dynamic enablement, rather than drawing all labels
with the 16-pixel name font. A zero/unshown control is not a missing bitmap.

## What is still missing for a whole-frame claim

The separate full-open probe executes real `449630(1,2)` and attempts to stop
immediately before its modal-loop call. It fails earlier at `4d9a7e`, inside
`4d9a60`, when prior-window cleanup dereferences the runtime object rooted at
`7f0b58`. The fixture has not booted that window/overlay graph. The failed
receipt is retained as `full_open_failed.json`; no replacement success stub
or fabricated complete screenshot was used. Native constructor/painter
evidence above remains independently valid within its stated boundary.

Still required: an original 1024×768 full screen with (1) Scout selected,
(2) the edited Infantry/Synthmetal draft at a declared animation phase, and
(3) the shielding picker open and then cancelled, including console, frame,
action buttons, category strip and hover/focus states. These live states
would verify parent compositing, visibility, dynamic navigation and any
remaining frame offsets. The offscreen evidence already resolves component
assembly, scene light/scale/rotation, child parentage, constructor bounds and
selected fonts; those implementation steps do not depend on the live capture.
