# Retained altitude class 7

**Retained World validation must allow classes 0–7.** Class7 is a supported
three-bit input to the original consumers; do not narrow import or clamp it
globally to6. Normal altitude reconstruction produces0–6, but that is an
operation-specific output contract, not the retained format's domain.

| Operation | Exact class7 handling |
| --- | --- |
| Temperature | Preserves altitude7 and uses the `class>5` high-altitude branch. Raw altitude still participates in feature-neighbor comparisons; clamping it to6 can change those comparisons. |
| Rivers | Accepts class7 as a land source and neighbor. It preserves altitude. When an uphill contour adjustment enters class7, the original reads lower-bound entry7: **140**. |
| Rainfall | Preserves altitude7; uses mountain shadow (`class>5`) and the raw class in neighbor-height comparisons. |
| AssignSurfaceClass | Accepts incoming7 and performs the ordinary old/new-class rules. It does not globally normalize other tiles. An explicit request7 writes7; no universal output clamp6 exists here. |
| ShapeHeight | Incoming neighbors at7 can remain7. A requested center6 does not lower a neighbor7: its first lower-ring ceiling is7. An explicit requested center7 also writes7. |
| RecalculateAltitudes | Reclassifies every tile from retained contour minus applied sea offset, choosing a threshold index0–6. Incoming7 is replaced by that computed class, which can be any0–6. Subsequent normal shaping requests are6 down to2. |
| Full recalculation | Runs altitude reconstruction first, so class7 is already absent when temperature, rivers and rainfall begin in the normal pipeline. |

The river-bound array must have **eight** usable entries for retained inputs:
`[0,20,40,60,80,100,120,140]` in these controls. The earlier seven-entry fixture
covered classes0–6; it was not a proof that the source table or retained domain
ended there. The new initial7 river control starts its source contour at100 and
other class7 contours at110. The original uphill read selects140, completes
normally, consumes194 shared draws, and retains class7 on all3,200 tiles. The
unchanged semantic reference agrees when supplied the eighth entry.

Explicit requested7 shaping needs an additional contour threshold endpoint:
the tested table through index8 is `[0,20,40,60,80,100,120,140,160]`. Its center
contour check still classifies only0–6, so requested7 resamples even when the old
contour is already150. The control produces center contour158 and class7,
with97 CRT draws including its shaped rings. This is separate from the normal
reconstruction producer, whose requests never reach7. A port may limit a
particular operation's admitted requests, but must not call that a universal
original class limit.

Nine complete native calls compare every tile byte and both RNG streams against
the unchanged semantic reference: temperature, rainfall, an uphill class7 river,
two direct assignments, two shaping calls, altitude reconstruction, and full
recalculation. The last two start with every altitude at7 while preserving the
prepared map's contours. Both end entirely in0–6; the full caller's temperature
entry is separately observed to contain no7. Synthetic maps are explicit
admissions, not a new natural campaign or editor audit.

[Literal controls](climate-retained-class7-cases.json). Existing frozen fixtures
and reference code are unchanged; broad RE remains paused.
