# Workshop whole-parent visual target and skin recipe

2026-10-21, original terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The existing [whole-parent reference](design-workshop-native-parent.md) already
has a separately reproduced1024x768 indexed frame. This packet connects its
decoration to the actual asset/callers and adds bounded Morgan-material views.
It does not repeat the model, cost or picker scalar matrices.

## First integration step: load DESWIN, then restore its clips

**The Workshop skin is `deswin.pcx`, not a newly drawn generic dialog frame.**
Installed dimensions are **623x368**, file SHA256
`ad1eebc6ac5de170a18c4aaf2fee86e7bfc9938d9257a12e66cf9c9b272e6ab9`.
It contains the parent panel borders, circular preview decoration, scanline
backing and shell details. Keep original pixels, including its transparent
corners/areas. Do not stretch a38x38 interface frame over this parent.

Constructor4469a0 at446d90..446de5 loads it through
`5f0be0("deswin.pcx",0,10,236)` into **Workshop+13c6c**, a cached bitmap.
The native decoder relocates PCX indices by10; source255 becomes key9.
Use the active PALETTE.PCX-derived game palette and its reserved entries,
rather than expanding the faction PCX palette or leaving raw indices unchanged.

`446e05..446e7d` creates the parent Window at the cached bitmap's width623
and height **600-consoleHeight**. In the recorded1024 profile consoleHeight213,
so parent H387. The bitmap is still368 rows: copy it at(0,0), clip to its
actual source bounds, and preserve the remaining Window/background behavior.
**Do not vertically scale368 rows to387.** The constructor copies the cached
bitmap through5f23b0. Component painters restore their own regions through
5f23e0 before drawing models and text:
43de60 Chassis,43e2b0 Weapon,43e850 Shielding,43ed10 Reactor,
43f120/43f4f0 ability cards,43f8c0 Main.

These restores are essential to avoid retaining prior model/text pixels when
changing designs. The full painter441ba0 also performs the existing cleanup,
card/preview/status/strip paints and final lower-button outline; it is not
equivalent to painting only the main model.

## Window coordinates and layers

At1024x768, parent Window is **[216,168,839,555)**, canvas623x387, and console
Window is **[0,511,1024,768)**, canvas1024x257. They overlap on screen.
Constructor position is `left=trunc((screenWidth-623+32)/2)` and
`top=screenHeight-consoleHeight-H`, thus top168 here. The parent and console
have independent canvases; add their own screen origins to local coordinates.

| Parent-canvas region | RECT |
| --- | --- |
| Main preview/title/data | `[140,9,451,320)` |
| Chassis | `[30,7,129,92)` |
| Weapon | `[30,101,129,186)` |
| Shielding | `[30,195,129,280)` |
| Reactor | `[462,7,561,92)` |
| Ability1 | `[462,101,561,186)` |
| Ability2 | `[462,195,561,280)` |
| Cost/status allocation, actor+b2c | `[140,57,451,73)` |
| Main stat allocation, actor+b3c | `[140,256,451,272)` |

The borders immediately surrounding those inset card clips come from DESWIN.
The main ring also belongs to that backing. Text/models overlay them using
the separately checked per-card clip/anchor/scale contracts.

| Console-canvas region | RECT / recipe |
| --- | --- |
| Main Workshop status panel | `[266,70,759,168)`; style3 outer, style0 inset3 |
| Vehicle Presets frame | `[783,60,1016,248)`; style3 |
| Vehicle Presets caption | `[786,63,1013,84)`; style6 |
| Preset button allocation | `[786,87,1013,245)` |
| Saved strip | `[8,192,764,248)`; style3 around tiled backing |
| Left/right navigation | `[12,199,24,241)` / `[748,199,760,241)` |

Console shell art comes from the original console2.pcx/console_x2.pcx loaders
for this profile. Keep the native console Window composite beneath the parent;
do not relocate the status/preset/strip panels into the623-wide parent canvas.
The [strip art/centering contract](design-workshop-strip-art-and-centering.md)
supplies arrow crops, theme tile, card masks and browse offsets. Generic dialog
rail geometry is not a replacement for DESWIN's own parent shell.

## Child action buttons and preset buttons

Actual parent-owned button RECTs, before screen translation:

| Button | RECT | Action |
| --- | --- | --- |
| Apply | `[27,321,132,341)` | 1 |
| Retire | `[459,321,564,341)` | 0 |
| Done | `[27,347,132,367)` | 4 |
| Rename | `[135,347,240,367)` | 5 |
| Obsolete | `[243,347,348,367)` | 6 |
| Upgrade | `[351,347,456,367)` | 7 |
| Cancel | `[459,347,564,367)` | 8 |

Debug controls also have constructed bounds in the gaps at135/351,y321, but
construction does not establish that they are visible gameplay buttons.
Retain the existing debug gating rather than filling those gaps with controls.
441ba0@441d59..441dc0 draws the lower-row union expanded3 using style3:
**`[24,344,567,370)`**. This overlay explains the small pixels that differ
from a bare DESWIN copy at the lower side margins.

Normal button painting in the captured constructor uses **style27,7b0490**
through6222a0@62239d..6223b0. Its hover/pressed alternatives are28/29 under
the generic state selector. These are interface.pcx small-frame styles:
`x=1+39*(i%3)`, `y=229+39*trunc(i/3)`; normal27 begins(1,580).
Use the eight6-pixel corner/edge slices and24x24 center pattern, with key9,
as specified in the [generic frame recipe](generic-dialog-fonts-and-frame.md).
The blue scanline center is asset texture, not a solid blue fill.

Preset controls are **console-owned**, not parent-canvas buttons. Constructor
446e92..446f84 lays out two columns by seven rows in column-major action order
9..22. First x786, second x901, width112, y`87+23*row`, height21. Captions
come from SCRIPT **DESIGNAUTO**, not the normal-open proposal role names.
See the [AutoDesign caller map](design-workshop-action-contract-audit.md)
for the action/filter distinction.

## Fonts, caption sources and cost flags

Actual glyph/LOGFONT records are retained with each reference. These are
Arial requests at1024, independent of the ability modal's Arial Narrow body.

| Role | LOGFONT height / weight | Representative baseline |
| --- | --- | --- |
| Main design name | -16 /700 |53 parent-local, centered in main clip |
| Main component headings | -12 /700 |18,112,206 for left column |
| Component names | -10 /0 | See per-card glyph journal; wrapping preserved |
| Component numeric values | -10 /0 | Actual side-card glyph runs; separate alignment from names |
| Main DATA heading/rows and ordinary COST | -12 /700 | DATA117; COST glyph journal in the57..73 allocation |
| Main stat string | -16 /700 |270 parent-local |
| Action button labels | -14 /700 |15 button-local |
| DESIGN WORKSHOP status title | -18 /700 |94 console-local |

Normal heading/title foreground in these receipts is active palette224
(`110000e0` color operand); normal action-button text is index65
(`11000041`). Preserve the stored palette/color mode instead of interpreting
these integers as packed RGB. The current palette and all per-glyph color,
clip, font and destination identities accompany the native reference.
Vehicle Presets is the separate caption at LABELS offseta70, one-based row669
(physical679), painted by441ba0 with foreground index240. DESIGN WORKSHOP is
LABELS e4c, row916 (physical926), painted by43d730. These titles do not come
from DESIGNAUTO's button-name rows.

Installed SCRIPT **DESIGNLABELS**, section at physical line1223, supplies the
cached moves/type/power/prototype-cost/cost/data/preset/weapon/armor/chassis/
reactor/ability/equipment strings. `4497c0..449823` reads16 rows into45-byte
slots beginning actor+14270. DATA and W/A/C/R/SP1/SP2 are therefore original
presentation labels, not a port-invented debug overlay. The main design name
comes from the existing stored-name/automatic-name branch, uppercased for draw.

Use the existing [main presentation](design-workshop-main-presentation.md)
for cached mineral factor, recomputed draft cost, Min Cost threshold, fast-land
`!` marker and separate prototype surcharge warning. This parent capture adds
placement and pixels; it does not alter that arithmetic or merge those lines.

## Reference scope and comparison boundary

The additional private output directory is
`tmp/reference/workshop-morgan-parent-visuals-v3-private-20261021/`.
Targets are intrinsic Scout2, Colony0 and **Unity Rover11**. ID1 is Formers.
Each target has a1024x768 native indexed buffer, active palette and PNG
palette expansion, plus selected52-byte record, glyph/font journal, Window
children, source frame/style calls, phase and yaw bits. These are original
Window-tree composites, not host-assembled UI mockups.

Owner1's vehicle material is a declared **Morgan** stage input: original
morgan.pcx sampling at(435,745) yields active index251/RGB255,255,0. The row
is independently derived from the checked VBP shade/palette algorithm and
stored at791298; SHA256
`d4830e6cfad32f510d2b06322065665ac85d36e1cf5c7ab8ac5dc0a7c4d8254d`.
The remaining initialized pools/rules are the bounded existing fixture.
Its inherited renderer initialization grants owner1 all89 technology ownership
bits; the retained prior before-open ownership snapshot confirms89/89.
These frames therefore do not establish Morgan's stock starting-technology
availability or research state. Scout/Colony/Rover selection and theme geometry
are the explicit visual targets.
**This is not a booted Morgan campaign or a complete faction-cache lifecycle.**

The primary map background is explicitly blank. Before **each** composite,
original5f8950 clears front9bf978 to index0, then original6101a0 composes the
visible tree. This declares opaque black uncovered-map pixels and prevents
previous-frame remnants in the blank-map harness. Timer registration is inert;
phase is reset0 and yaw is reset before the full painter. Host GDI supplies
font metrics/pixels; memory Win32/file services and the earlier initialization
boundaries remain explicit. No real desktop pointer, OS timer delivery or
normally rendered campaign map is claimed.

For the first port pixel gate, compare the parent623x387 crop at(216,168)
and console panels at their stated screen coordinates. Keep the blank map
outside those regions out of a normal-campaign comparison. Compare the exact
indexed buffer only under the same palette/font/phase/state inputs; the PNG
is its deterministic RGB expansion. Use the retained staged renderer controls
to diagnose model differences rather than treating this one-screen target as
a new proof of the complete asset-to-raster algorithm.

[workshop_parent_pixel_gate.py](workshop_parent_pixel_gate.py) compares matching
1024x768 reference/candidate PNGs, reports differing-pixel count and screen
bounds, and fails unless the selected parent/console/full region is exact.
It requires explicit matching state and scale; it performs no alignment,
resizing or tolerance that could hide a decoration/placement error. The
[compact cases](design-workshop-parent-visual-target-cases.json) retain source
asset hashes, native buffer/crop hashes, active state and verification scope.
No comparison against a port screenshot is claimed by the native capture.

Final native results: Scout Cost10, Colony Cost30, Unity Rover Cost20, with
the captured ordinary COST labels at baseline69. Each reference independently
matches **3,552 non-key DESWIN decoration pixels** in stable shell/top regions;
all PNGs exactly expand their native indexed buffers through their saved palette.
The comparator self-checks exact parent crops and rejects the different real
Scout/Colony images (16,913 differing parent pixels), establishing its mismatch
reporting without claiming a port/native match.

Additive frozen bundle:
`tmp/reference/independent-workshop-morgan-parent-visual-evidence-20261021/`.
All18 payload files and manifest hashes verify. Manifest SHA256
`4af1724882f41f4d40f0200dcc5f50030e160c88b8cccfdbfece97adeea8661d`.
This final results/provenance paragraph was appended after freezing the recipe.

## Live Tiny/Morgan comparison: keep economic inputs distinct

The native renderer fixture explicitly writes resolved size category
**9b9528=2 (Standard cost category)** and mineral rule **9ba444=10** in
probe_workshop_preview_parent.py before the reused renderer initialization.
Human owner1's current INDUSTRY is0 in the retained faction snapshot;
4f9d60 caches mineral factor**10**. It is a blank-map UI fixture, so the
category is a declared economic input rather than a newly generated Standard
campaign map. The Morgan identity in this packet is the material-stage input
described above; full starting faction/game setup is not claimed.

The implementation thread reports its live1024x768 capture as **Tiny/Morgan**,
default current INDUSTRY, cached mineral factor**8**. Therefore:

| Design | Native category2/factor10 | Reported live Tiny/factor8 |
| --- | --- | --- |
| Scout |10 |8 |
| Colony |30 |24 |

These cost glyph differences are expected economic output, **not pixel defects**.
Use matching map category, current social state, rule multiplier, cached factor,
technology, prototype and UI hover state before asserting a whole-parent match.
Do not scale the whole image or change the live factor to disguise the difference.

The comparator now supports **`--region decoration --skin PATH_TO_DESWIN.PCX`**.
This checks exactly the3,552 opaque skin pixels in the verified shell/top ROIs,
excluding economic text, model content and transparent uncovered-map pixels.
It is appropriate for the first parent-decoration gate across those two economic
states. It does not claim to test all panel contents or replace the later
matching-state whole-parent/console gates. This comparator option and economic
clarification were added after the initial18-file freeze; frozen payloads remain
unchanged.
