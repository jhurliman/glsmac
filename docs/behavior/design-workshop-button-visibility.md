# Workshop action buttons in the native console overlap

2026-10-21, source/retained-frame clarification, no new render run.
Original indexed frame SHA256
`ecf6318a1d4a01e7f33389144c468f757bf9f600c5903e8d2c40452a423d9439`.
Private frame: `tmp/reference/workshop-parent-capture-private-20261021/native-parent.png`.

The checked scalar regions and source are also frozen in
`tmp/reference/independent-workshop-editor-actions-evidence-20261021/`;
manifest `a9811cfe7e4faf4aa81fa9258b35cfae56e01974933e626b2d8466c8aa1bb111`.
This provenance paragraph was added after freezing.

The ordinary lower action row **is visible at runtime**, without clipping
or relocation to avoid the console. The earlier construction/visibility
warning matters for the two editor-only controls, which447940 explicitly
hides in ordinary mode. The complete native post-show frame shows all seven
ordinary controls, including five in the second row.

Workshop Window: `[216,168,839,555)`; console: `[0,511,1024,768)`.
Final screen coordinates:

| Control | RECT |
| --- | --- |
| Apply | `[243,489,348,509)` |
| Retire | `[675,489,780,509)` |
| Done | `[243,515,348,535)` |
| Rename | `[351,515,456,535)` |
| Obsolete | `[459,515,564,535)` |
| Upgrade | `[567,515,672,535)` |
| Cancel | `[675,515,780,535)` |

Parent-local y347 therefore remains screen515.447940 shows ordinary child
controls before606850 shows the Workshop. Retained map root943294 owns the
Workshop; Workshop has nine visible children. The console is a separate
root with57 children, including presets/strip controls.

Native composition uses the window tree and keyed surfaces, not one opaque
console rectangle.60dd40 appends descendants before their owning background
to its flat list;6101a0 draws that list in reverse, so children paint after
their owning background. Window+124 is5eeb10 for all three retained roots.
Workshop and console have bit0x1000000 set, selecting its keyed-copy paths
at5eec94 /5eedff, which pass byte6b4fb4 (key9 here) to5f12c0.
The console's source255 transparent areas become key9 after loading.
Its bounding rectangle starting511 is not an opaque occlusion mask.

The final native frame is the overlap target: blue action buttons and labels
survive at515..535 with the console shell around them. Preserve owning
background/child ordering and source transparency in render layers; a dark
full-screen backdrop must not become an occluder between Workshop and console.
Native code has no GLSMAC floating-point z values, so none are invented here.

[Retained-frame receipts](design-workshop-editor-actions-cases.json) include
hashes for all seven105x20 screen regions and palette65 glyph counts, extracted
from the existing independently reproduced frame. No mockup or new screenshot.

## Main button caption integer centering

Source-only follow-up, 2026-10-21. The ordinary control painter is `6222a0`.
After painting the selected frame, `6228ab..6228c7` passes the caption to
`5f6720` with x=`rect.left+3` and available width=`rect.width-6`.
That wrapper delegates each line to `5f5e20`; `5f5e94..5f5ebc` measures its
advance with `5f55c0`, subtracts it from the available width, and computes
signed integer division by two using `cdq; sub eax,edx; sar edx,1`.
The resulting text origin is:

`rect.left + 3 + truncTowardZero((rect.width - 6 - measuredAdvance) / 2)`.

For the admitted short captions in the 105-pixel controls, this equals
`floor((105-measuredAdvance)/2)` relative to the control. An odd positive
spare pixel goes on the right; nearest-integer rounding moves the caption
one pixel too far right. The retained APPLY x=30 and DONE x=32 agree with
advances45 and41 respectively. This is advance-based placement, not centering
the visible glyph bounding box. The narrower/overlong-caption fallback at
`6228d4` is a separate path; no claim here changes that fallback or global
GLSMAC layout rules. No new native replay was needed.
