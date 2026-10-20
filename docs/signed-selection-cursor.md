# Raw cursor origin in automatic Unit selection

The saved UI cursor and reopened camera are distinct. In the original 80x80
control, a saved cursor (80,38) reopens the camera at (40,40), but the later
selector still measures from (80,38). With ready Units at (20,40) and (24,40),
that selects the first record; substituting the camera selects the second.
This comes from the frozen primary-view handoff recorded in
[campaign cursor persistence](campaign-ui-cursor.md).

The frontend selection visit now copies a signed cursor pair, and the actual
backend selector accepts it without world-bound normalization. An owned valid
reference Unit still supplies the origin when present. Old callers that supply
a physical map tile retain their bounds checks. Old campaign files without a
cursor continue to derive focus from the existing camera projection until a
tile is selected. Simulation state and random streams do not change.

The actual queued path checks the (80,38) distinction, queue deduplication,
value ownership across a later UI cursor change, and weak retirement before a
deferred callback. The existing frontend command-panel fixture also exercises
`QueueAutomaticSelection` with both (80,38) and (-1,38), proving that its live
entry point passes the signed cursor rather than falling back to camera math.

Nine affected native test groups pass. Four production translation units pass
correctness analysis. The same nine groups pass with ASan/UBSan on the changed
backend/frontend selector and two test TUs; other objects are uninstrumented and
leak detection is disabled. This arithmetic/caller change does not repeat a
platform matrix or require another interactive graphics run.

## Signed arithmetic and cutoff

The independent supplement `signed-cursor-margin-evidence-20261020`, manifest
SHA256 `9a2293d05887cae5477f042d8319ea8fcf5eaf6d283ac8c22c98239c517ce9d1`,
was verified across all eight frozen files. Its standalone checker passes
40 selectors, 137 original metric returns and 25 separate recenter controls.
The compact [selector cases](behavior/signed-selection-cursor-cases.json) retain
only numeric inputs/results; full buffers and purchased assets stay outside Git.

The production metric now wraps every subtraction, addition and negation to
signed 32 bits, preserves `abs32(INT32_MIN)`, and rounds signed shifts downward.
Explicit unsigned arithmetic and widened conversion/division avoid C++ signed
overflow or implementation-defined negative shifts. A round-world seam correction
is applied once and followed by another abs; it is not coordinate normalization.
The original can return negative distances after overflow, and those remain
eligible. Initial best distance is 9999. Source524e63..524e71 confirms that an
eligible current selection wins an equal9999 distance even with no prior winner.

Both Unit metrics match all40 original round/flat controls (80 metric values),
and the actual backend selector matches all20 round controls. The current live
knowledge owner admits only periodic-X maps, so the flat cases exercise the
production metric without claiming a live flat-world implementation. A derived
origin additionally checks the source-proven equal9999 branch with/without a
current selection. Existing wrap, reference-Unit, tie/order and work-controller
regressions remain green. The backend TU was reanalyzed after the metric change;
all nine groups pass the final focused4-TU ASan/UBSan run.

Selected Unit persistence and the automatic recenter margin/high-terrain top-edge
rule remain separate implementation work. The captured recenter controls are
not counted as implemented by these selector tests.
