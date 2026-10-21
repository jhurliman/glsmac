# Native DESIGNNAME edit PNG targets

2026-10-21. Six private original-DIB PNG exports, not generated artwork or a
manual composite. Executable fingerprint matches the
[selection/caret contract](design-workshop-edit-selection-and-caret.md).
Original SCRIPT/font/palette/template loading and62eda0 painting execute;
on states use original62f920 callback from visibility0. No models, campaign,
map rendering, OS modal loop or measured wall-clock blink phase.
[Pixel/export receipts](design-workshop-edit-native-dibs-cases.json).
All four12200-pixel edit self-checks, both20-pixel caret differences, native
palette/PNG byte checks and retained-image identity pass. Additive private
freeze contains19 payloads at
`tmp/reference/independent-workshop-edit-dib-export-evidence-20261021/`;
manifest `b727b0c34e87c4ee3ad656e007042f0b43d41aadc697f09583fd7907ce71c8f0`.
This provenance paragraph was added after the immutable freeze.

Private source directory:
`tmp/reference/workshop-designname-dibs-v2-private-20261021/`.
All four edit images are **610x20**, text `Scout Patrol`, final inset0,
native MainFont16/Arial Narrow bold, original palette0/36/224:

| File | Selection | Caret |
| --- | --- | --- |
| selected-caret-off.png | Anchor0, active12 | Off |
| selected-caret-on.png | Anchor0, active12 | On, x84 |
| collapsed-caret-off.png | Anchor12, active12 | Off |
| collapsed-caret-on.png | Anchor12, active12 | On, x84 |

Each on/off pair differs in exactly20 pixels, bbox `[84,0,85,20)`.
Selected/off is byte-identical to the earlier retained native layout PNG,
SHA-256 `f36963a1f0778ff394f8d4e55dee436a9299eab3cbd8f8442cd7d2dc3bca1b6b`.
Palette-index buffers and the original1024-byte palette are retained alongside
PNGs. Their native pitch is612; the right two padding bytes per row are not
part of the610-pixel image or the comparison gate.

Parent-owned surfaces are also exported:

| File | Dimensions | Contents |
| --- | --- | --- |
| dialog-own.png | 760x300 | Dialog-owned caption and frame raster |
| name-row-own.png | 660x26 | Name-row-owned raster, including Name: label |

These are separate surfaces. The dialog-owned bitmap does **not** include
the separately owned row, edit and buttons; the Name-row bitmap does not
include its separately owned Edit bitmap. Do not compare either as a fully
composed modal screenshot or fill its blank areas with an invented map image.
No blank-map composite assumption is involved in the direct Edit DIB target.

For the1024x768 mode0 captured layout:

| Coordinate system | Rectangle |
| --- | --- |
| Dialog, screen | `[148,255,908,555)` |
| Name-row, dialog-local | `[33,38,693,64)` |
| Edit, row-local | `[53,1,663,21)` |
| Edit, arithmetic screen translation | `[234,294,844,314)` |

The Edit extends three pixels beyond its row's own rectangle. The arithmetic
screen translation establishes the origin, not full Window-tree ancestor
clipping. Prefer comparing the port's direct610x20 Edit surface. A screenshot
crop can additionally expose parent clipping/composition differences at the
right edge; those are separate from the owned-DIB pixel target.

Exact comparison helper, with no tolerance or asset pixels in public source:

```text
python docs/behavior/workshop_edit_pixel_gate.py NATIVE_EDIT.png PORT_EDIT.png
python docs/behavior/workshop_edit_pixel_gate.py NATIVE_EDIT.png SCREEN.png --candidate-crop 234 294 610 20
```

The helper checks dimensions/crop bounds and reports differing-pixel count,
bbox and first coordinates; mismatch exits1. A self-comparison covers12200
pixels exactly. Native off/on comparisons establish the20-pixel caret target.

The first development re-export ran after the inherited wrapper released the
private bundled font; glyph repaint then differed from the retained image.
Those failed development files are not reference targets. The final export
keeps that font registered throughout every paint, verifies the retained
selected/off image byte-for-byte, then releases the private font resource.
This fixes export lifetime without regenerating any models or altering the
original guest instructions. Existing key/state and draw-call receipts remain
separate from this verified raster export.

## Port integration result

The live1024x768 Rename dialog now accepts and applies `Scout Patrol`, then
reopens with that saved name selected. Normal and ASan/UBSan-instrumented SDL
sessions capture all four reference states and close cleanly. The screenshot
crop has41 differing pixels out of12,200 in each state:40 at the rightmost
three columns where the port's parent row clips the610-pixel Edit, plus one
glyph pixel at Edit-local(5,11). The original owned DIB does not establish
ancestor clipping, so those40 pixels are a composition boundary, not proof
that the original composed dialog displays them. The glyph difference remains
an unresolved font-raster difference; no tolerance or reference alteration
turns these captures into an exact pass. Both on/off pairs change exactly the
expected20-pixel caret column at x84. The private capture observer initially
sampled once per second and aliased the500 ms blink; shifting capture sampling
exposed both phases without changing application timing.
