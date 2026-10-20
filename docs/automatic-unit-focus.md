# Automatic Unit focus and camera margins

Selecting the next ready Unit no longer always centers it in the native map.
The original changes the independent cursor to the selected Unit, then keeps the
camera when the target lies in the current interior. A retained current selection
also keeps its camera. This avoids camera jumps between nearby ready Units.

The selection receiver records whether the controller kept its current ID before
`SelectUnit` updates that ID. Its new focus method reads the owned native world
and current view context. When recentering is required, it updates the native
camera, requests a repaint and prevents legacy camera adoption from overwriting
the result. Selection/cursor state remains independent of camera movement.

## Original decision

Original 0x524f40 decides whether to call camera setter 0x477a40. With native
`firstX`, `firstY`, `sx`, `sy`, `ox`, `oy`, its interior is:

```text
[firstX+3, firstX+sx+ox-3) × [firstY+3, firstY+sy+oy-3)
```

The margin is three map coordinates, not three screen pixels. For altitude band
greater than 3, a high-terrain top-edge check also requires
`unitY-band+3 >= firstY+3`. This uses the raw altitude band, independently of
contour elevation and the flattened-terrain display setting. Y is then checked
against the interior. Round-world X adds width if below the left edge and then
subtracts width if at/beyond the right edge; it is a sequential adjustment.
Flat worlds omit that adjustment.

At camera (22,38), zoom 0 and a 1024x511 terrain viewport, the captured interior
is X [13,30), Y [29,47). Tile (13,29) stays visible without centering; (30,30)
and (13,47) recenter. Tile (14,30) at band 4 stays inside, while band 5 triggers
the top-edge rule.

## Evidence and validation

The [25 compact original controls](behavior/automatic-unit-focus-cases.json) come
from frozen `signed-cursor-margin-evidence-20261020`, manifest SHA256
`9a2293d05887cae5477f042d8319ea8fcf5eaf6d283ac8c22c98239c517ce9d1`.
All eight files were verified during selector import. Original focus prefixes
execute actual view geometry, cursor setter and camera setter through 0x52519d;
presentation/notification services are supplied. These are not a complete GUI
event-loop trace.

Production tests match each captured layout, recenter decision and final camera
through the actual frontend focus method. They also verify that retaining the
current selection does not request repaint, and that camera policy leaves the
selected cursor unchanged. Seam, north boundary, inclusive/exclusive edges and
high-terrain cases are covered.

The cached incremental build and 12 affected native groups pass, including the
new controls, campaign reopen, signed selector, retained work, native zoom and
committed-frame picking. Five production TUs pass correctness analysis. All 12
groups also pass a focused three-TU ASan/UBSan build of the frontend selector,
native-terrain consumer and test fixture; other objects are uninstrumented and
leak detection is disabled. The final source edit only clarifies a comment.
No platform matrix or interactive graphics run was repeated.

## Remaining scope

The legacy renderer still centers a newly selected Unit through its existing
fallback. Flat profiles exercise the focus policy/context method in these tests;
a complete live native flat-world renderer is not yet admitted. Saved selected
Unit identity, later controller eligibility/continuation, full cursor history,
pending asynchronous camera transitions and wider nonzero-zoom interactive
acceptance remain separate requirements. These bounded controls do not establish
the complete original controller loop.
