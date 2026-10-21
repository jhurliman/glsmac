# Workshop generic WEAPON picker: source completion

2026-10-21. Pinned original terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This is source tracing plus reuse of existing composition, renderer, picker
transition and installed-definition evidence. No new raster/model matrix or
desktop interaction is claimed. The additive scalar reference is
[workshop_weapon_picker_reference.py](workshop_weapon_picker_reference.py).
The [compact card plans](design-workshop-weapon-picker-cases.json) are explicitly
source-derived, with four CVR pointer identities checked against the retained
native model-loader receipt; they are not 21 new native controls.
Shared layout/fonts remain in
[the picker layout contract](design-workshop-preference-and-picker-layout.md);
do not use the different [WEAPON side-card contract](design-workshop-weapon-side-card-empty-scenes.md).

## Independent scene composition

`43bc40` maps admitted ordinal to weapon definition ID, checks owner technology,
and clears scene count at picker+`3f68` (scene+`13cc`). It then selects:

| Definition ID / condition | Original scene operation | Scale bits / approximate value |
| --- | --- | --- |
| 0, Hand Weapons | `632060(7af880,0)`, `vw00.cvr` | `3e4ccccd`, .20 |
| 13, Planet Buster | `632060(7af350,0)`, `vw13.cvr` | `3d75c28f`, .06 |
| 20, Conventional Payload | `632060(7af360,0)`, `VM.CVR` | `3d75c28f`, .06 |
| Other, mode<3 | `4d8e40(owner,-1,weapon,scene)` | `3e4ccccd`, .20 |
| Other, mode9 | Same independent-part call | `3da3d70a`, .08 |
| Other, mode11 | Same independent-part call | `3e4ccccd`, .20 |
| Other modes | Same independent-part call | `3dcccccd`, .10 |

The three direct-ID branches precede mode dispatch (`43bcaf..43bd59`). They
remain direct even if configured fields differ. In particular, a missile
draft chassis cannot suppress the two payload scenes here. This callback
never passes the draft chassis to the weapon composer.

For stock ordinary combat IDs1..12 the independent scene copies, respectively,
`VW01`, `VW02`, `VW04`, `VW03`, `VW05`, `VW06`, `VW07`, `VW08`, `VW09`,
`VW10`, `VW11`, `VW12` (all `.CVR`). The existing
[stock composition reference](workshop_stock_composition_reference.py) retains
the power-based lookup, signed -1/99 cases and clamp to0..24. A power<=1
ordinary fallback can be empty; direct Hand Weapons ID0 still has its scene.

With chassis **-1**, package composition is:

| Mode | Scene operations |
| --- | --- |
| 7, Transport | Copy `VWNTT.CVR` |
| 8, Colony | Attach `droplet.cvr` using marker `adroper.cvr`, frame0 |
| 9, Former | Copy `vwntu.cvr` only |
| 10, Supply | Copy `VWNST.CVR` |
| 11, Probe | Copy `ptmod.cvr` |
| 12, Artifact | Copy `VWNAA.CVR` |

Thus the independent Colony part retains its marker displacement. Former does
not add the Infantry-only `vt.cvr`; Probe does not add the Infantry body VPT.
Ordinary independent combat parts omit the Infantry mounting/gun-base assets,
the owner-tinted base and the Hover mounting. No new faction tint is selected
by this callback. Preserve the loaded CVR materials and existing renderer
shade/palette rules. Scene orientation/light initialization is shared
`43d128..43d1ac`, with fixed yaw `3f05f92d`, rather than the main preview timer.

## Underlay source, crop, key and clip

After composing/scaling and **before** normal/silhouette drawing,
`43bd70..43bdf8` checks mode8/9/11. Modes8/9 use signed weapon record byte+9
(`9b6fb1+16*ID`). Mode11 instead uses signed **absolute byte9b034e**, the
intrinsic Probe Team design6 record's byte+46 decimal. It does not use the
currently selected/draft design graphic byte. A negative selector skips the
underlay; there is no fallback sprite. A nonnegative selector j selects
**`780740+352*j`**, frame0, passed to `5fd9e0`.

**Installed stock selectors are -1 in all three cases.** ALPHA's Colony/Former
weapon icon fields are -1; the existing native initialized-design receipt has
9b034e=-1. Consequently stock mode8/9/11 cards have their independent CVR
scene and no sprite underlay. Do not invent a mandatory Colony/Former/Probe
picture behind those scenes merely because the conditional branch exists.

The conditional asset recipe is nevertheless defined. `45f550` loads
**UNITS.PCX** (`6a8228`) and extracts 100x75 sprites into the bank rooted at
`77fae0`. The weapon underlay bank starts nine rows into it. For j0..9:

| Selector | Source x,y,width,height in UNITS.PCX |
| --- | --- |
| j0..8 | `[2,2+77*j,100,75]` |
| j9 | `[716,233,100,75]`, special loader branch `45f7c0..45f7dc` |

Selector3 has seven loaded animation frames, separated by102 source pixels
in x, but this caller chooses the first sprite only. Other selectors here
use one loaded frame. The callback only checks j>=0; it has no upper-bound
clamp. Values beyond the initialized bank are not supported asset identities.

Before extraction, `45f7e0..45f803` maps indexed marker colors7 and6 to9
through `5f7380`; that routine compares only low bytes (`5f74a6..5f74be`).
`5fc7d0` receives key109 hex and stores its low byte **9** as transparency.
The resulting underlay is a keyed indexed-color copy, **without a faction
tint, grayscale pass, or silhouette conversion**. `5fd9e0` copies non-key
bytes and retains destination pixels for key9. It applies normal sprite
transparent trimming: retain the trim offset relative to the logical100x75
crop instead of centering the trimmed image afresh.

Given callback inset RECT C, anchor the logical sprite at
`(C.left+trunc((C.width-100)/2), C.top+trunc((C.height-75)/2))`.
Signed division truncates toward zero. First cell C=`[30,7,129,92)` therefore
gives **(30,12)**, even though C.width99 is one pixel narrower than the sprite.
`5fd9e0@5fdb56..5fdb6b` adds the sprite trim offsets to this anchor.

The active canvas clip comes from **outer cell RECT**, not C:
`43cda0@43ce3e..43ce4b` sets `[27,4,132,95)` for the first cell, then insets
by3 only for callback arguments. The sprite can paint that extra pixel at
x129. The same outer cell clip applies to model rendering and text. The cell
wrapper restores the full backing clip after each callback (`43ceed..43cef3`).
Later cells translate by108*column and94*row.

Model anchor is the truncated midpoint of C, **(79,49)** for the first cell.
An available card calls `631370(canvas,x,y,6)`. Unknown technology still
composes the scene, scale and conditional underlay first, then calls
`631760(canvas,x,y,185,6)` and returns0 **before header/stat text**. The
unavailable underlay is not included in that model silhouette operation.

`5fd9e0` also reads the existing sprite ratio `6b4fb8/6b4fbc`; this callback
does not set those globals. Equal numerator/denominator uses the direct keyed
copy/trim path above. If a caller admits a different ratio, reuse the checked
[indexed sprite sampler](native-zoom-known-terrain-sprite-scaling.md), including
its scaled trim/clip rules. The logical100x75 anchor calculation still occurs
in `43bc40` before that helper; `6324f0` changes CVR scene scale only.

## Weapon names and mode/stat text

Available cards resolve their header from `9b6fa8+16*ID`, uppercase the
localized name and center/wrap it in the header through `5f68b0`. Use shared
Arial10 normal small font and Arial12 bold numeric font; header/bottom
alignment and baselines follow the existing metric-derived picker contract.
First-card bottom row is `[30,79,129,92)` with small-font H13 in that profile;
caption baseline89 and numeric baseline90.

| Mode | Bottom-left LABELS offset / one-based row | Bottom-right |
| --- | --- | --- |
| <3 | 204 hex, row130, **Weapon** (physical line140) | Unsigned record byte+8 as decimal, except ff -> localized Psi |
| >=3 | 200 hex, row129, **Equipment** (physical line139) | Unsigned record byte+11 as decimal, except byte+8 ff -> localized Psi first |

Psi uses LABELS310, row197 (physical207). Its test precedes the mode test.
Thus Planet Buster prints Weapon99, not `*`; Colony/Former/Transport/Supply/
Probe/Artifact print Equipment10/6/4/8/4/36 under installed definitions.
Byte+11 is the rules record's raw cost field here, not a computed prototype
cost. Configured combat strength -2 prints254. There is no colon, multiplier
suffix, or additional mode-specific label in this callback.

## 21 definitions, stock20 admission and overlapping scroll windows

`444400@444430..44445a` converts the current weapon ID to its admitted ordinal
before calling `43cfc0`; `4444b2..4444d0` converts the returned ordinal back.
Admission includes signed prerequisite>=-1, independent of owner technology.
The original table has21 definitions IDs0..20, but stock Alien Artifact ID19
has prerequisite Disable (<-1), so **stock admitted count is20**, not21.
Stock ordinal19 is Conventional Payload ID20. Unknown technology does not
remove a card or make payload start a second initial window.

The window always paints20 cells,5 columns by4 rows. Let p be first ordinal.
Initial p=0; while `initial>=p+20`, add5. Store selection as the original
admitted ordinal; scroll position is p/5. Hence:

| Definition setup / current choice | Initial ordinal | First p | Position |
| --- | --- | --- | --- |
| Stock, Conventional Payload ID20 | 19 | 0 | Bottom-right cell |
| All21 admitted, Conventional Payload ID20 | 20 | 5 | Bottom-left cell |

For all21 admitted and p5, cells contain ordinals5..20 followed by four blanks;
the last valid ordinal20 uses outer `[27,286,132,377)` and inset
`[30,289,129,374)`. `43cf80` sets p=5*scrollPosition and repaints. These are
overlapping row windows, not disjoint 20-item pages. `43d31a..43d384` creates
the control when count>18, with range0..`trunc(count/5)-4+2` and field
`+1454=3`. For counts20/21 the supplied range is inclusive0..2
(`61ee70`, `61f1e0`), allowing p0/5/10. Out-of-range cells take the blank
backing-copy path (`43ce09..43ce2a`); they do not wrap around.
The later console attachment is separate and runs only for count>20.

Cancel/no selection or invalid hovered card returns the initial ordinal.
Hover stores painter availability at `+4648`. Accepted weapon changes retain
the existing caller cleanup and air-Probe rejection; this rendering contract
does not replace those checks. See the checked
[picker transition contract](design-workshop-interactive-editing.md).

Auxiliary scrollbar sprite chrome and held-button repeat timing remain a
separate follow-up; this packet supplies the picker range/callback operands,
not an unverified timing claim.

Source-only additive freeze:
`tmp/reference/independent-workshop-weapon-picker-source-evidence-20261021/`.
Its17 payload files and manifest were hash-verified; manifest SHA256
`e472343652354c8e6a09a84379760a826c319b5c3b0aa4c3f190b15c7209dda0`.
It retains source listings, scalar plans/reference, installed input hashes and
the identities of reused native bundles. This provenance footer was added
after freezing the behavioral document.

## Caller clarification: filtered current ID and initial scrollbar RECT

Source-only follow-up, 2026-10-21. `444400@444430..44445a` compares the
current ID **inside** the admitted-definition branch. Its exact scan is:

```text
ordinal = 0
for ID = 0..20:
    if prerequisite[ID] >= -1:
        if ID == currentID: break
        ordinal += 1
```

If currentID is itself filtered, the comparison never succeeds; the scan
returns **the total admitted count**, not the count preceding that ID and
not zero. Thus stock currentID19 (disabled Artifact) gives initial ordinal20,
first p5 and stored selection20. There are15 valid cards (ordinals5..19) and
five blanks in that window. Ordinal20 has no card frame/highlight;
constructor `43b7d0@43b854..43b85a` initializes availability `+4648` to0.
This invalid-current boundary has no automatic selection fallback.

The cancel/invalid return path preserves that initial ordinal20. The caller's
reverse scan can then exhaust all definitions and reach ID21
(`4444b2..4444d0`). This is a source-derived out-of-table boundary, not a new
executed end-to-end control or a supported way to select a valid component.
Keep the ordinary admitted-current path separate from this case.

Before the additional count>20 console attachment, the count>18 constructor
at `43d323..43d342` is exactly:

```text
61e960(this=picker+0xa14,
       x=pickerCanvasWidth-43, y=21,
       width=24, height=246, parent=picker,
       argument6=0, flags=1)
```

`61e960@61eab1..61eabd` passes those dimensions unchanged to Window creation
`5edd10`. Its initial dialog-local RECT is therefore
**`[W-43,21,W-19,267)`**. For the existing native Workshop width623 this is
**`[580,21,604,267)`**; at width700 it is `[657,21,681,267)`.
This is a fixed246-pixel control, not the full4x94 grid height or outer H.

With default style global `6b5264=-1`, the constructor creates24x24 end buttons
at control-local **`[0,0,24,24)`** and **`[0,222,24,246)`**
(`61eafc..61eb11`, `61eb93..61ec18`). The picker immediately invokes its
virtual+8, resolved to **606b50 (hide)**, at `43d347..43d34b`.
Creation alone does not establish visible chrome. The separate count>20
`46ac10` path subsequently resizes/repositions/shows this same control;
its recorded245-pixel attachment profile must not be substituted for the
constructor's246 pixels. The range/page operands remain as documented above.

## Attachment at623x387 and keyboard overflow admission

For the recorded1024 console asset bank, the >20 `46ac10` attachment finishes
at **`[580,21,604,334)`**, width24, height313 for picker W623/H387.
This follows the source's tile arithmetic rather than stretching to H:

1. Attachment scratch height is `H-13` (`46ae68..46b002`).
2. Loaded top/tile/bottom nominal heights are26/34/25. Constructor
   `4660d0@466eeb..466f57` extracts them from console_x2.pcx at
   `[1,279,32,26]`, `[1,306,32,34]`, `[1,341,32,25]` into
   `7eceb4`, `7ecee0`, `7ecf0c` respectively.
3. `46b012..46b03e` computes
   `skinHeight=26+25-34*trunc((-(H-13)+25+26+44)/34)`.
4. `46b29e..46b2c2` sets width24, height`skinHeight-10`, position`(W-43,21)`
   and calls show. At H387 the numerator is-279, quotient-8, skinHeight323,
   control height313. At H345 the same formula gives245, matching the
   retained ability-chooser attachment receipt. Default24-pixel end buttons
   therefore occupy `[0,0,24,24)` and `[0,289,24,313)` after resize.

This result is conditioned on that loaded asset bank and original attachment
inputs, not a newly executed387-pixel layout. The main narrowed frame/rails
remain separate from this attachment.

Stock20 has **no overflow to scroll**: all admitted cards fit p0. Its hidden
control's allocated range0..2 does not make all three windows user-visible.
Keyboard handler **43ca60** directly navigates selection; its key table is:

| Key | Source branch | Stock20 behavior |
| --- | --- | --- |
| Up38 / numpad8=104 | 43cab9 | Move up5; stop at first row |
| Down40 / numpad2=98 | 43cb0d | Move down5; stop at final row when count<=20 |
| Left37 / numpad4=100 | 43cb9d | Move left within current row, clamped |
| Right39 / numpad6=102 | 43cbde | Move right within current row, clamped |
| Home36 | 43cc4f | Select0 and set p0 |
| End35 | 43cc21 | Select count-1; change p only if count>20 |
| PageUp33 / PageDown34 | 43cc5f /43cc83 | Return immediately when count<=20 |

For an overflowing list, vertical boundary crossings update first ordinal
and scrollbar position via `61f1e0`; PageUp/Down use position minus/plus3 and
the inclusive scrollbar clamp. Home/End also move the window as their branches
specify. Keyboard handling does not require the scrollbar to be visible.
For stock20 valid-current input, these branches never require an alternate
window. This source packet does not establish an additional mouse-wheel route;
do not infer one from the hidden control's existence.

## Live implementation and validation

The live WEAPON card now opens the component picker and accepts through the
existing read-only edit adapter and queued Apply/Done path. Cards retain their
configured identity and owner technology availability. Available captions use
installed labels and raw byte values; unknown cards omit all glyphs. Conditional
UNITS sprites retain their logical crop origin and outer-cell clip, including
x129 in the first cell. Stock20 keeps the allocated scrollbar hidden; configured
all21 exposes the source-derived313-pixel attachment and overlapping p0/5/10.
Its current scrollbar widget chrome/repeat timing is not yet the original skin.

Renderer tests compare twenty independent single-asset recipes at both availability
states, plus the marker-based Colony scene and ID0's direct scale precedence.
These are source-derived composition checks, not new original-executable raster
controls. Prior complete native component and unit-frame controls still pass.
Synthetic and owned UNITS cases check crop/key/remapping, clipping and rejected
input atomicity. A wrapped Base query verifies stock-shaped20 admission, raw
signed model power versus unsigned caption, and retained Probe6 sprite authority.
GSE tests cover21/20 admission, overlapping/trailing windows, filtered current
selection, raw captions, unknown selection guards and accepted weapon routing.

49 selected native groups pass under ASan/UBSan (seven production and three test
translation units instrumented); six UI scripts pass. Targeted correctness
analysis is clean for the four changed production translation units. The actual
owner-backed Morgan capture opens the20-card picker, selects Colony Module,
Applies the resulting Colony Pod and closes with Done. It exits0 without
ASan/UBSan reports or tracked OpenGL leaks. The first capture's acceptance clicks
hit the wrong row and were excluded; the corrected second session establishes
that flow. Software GL/no-audio capture is not a frame-pacing or audio benchmark.

Whole-screen pixel parity, generic-picker keyboard/highlight behavior, native
scrollbar chrome/repeat timing, parent decorations and the remaining Workshop
actions/presentation are not established by this change. Filtered current IDs
retain their initial display boundary; no out-of-table component ID is admitted.
