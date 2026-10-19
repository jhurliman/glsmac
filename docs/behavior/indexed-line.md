# Native indexed software line primitive

**709 complete original calls, one separately bounded empty-clip hazard, and
3,362 observed palette-index stores** establish the focused kernel contract.
The [reference](indexed_line_reference.py) matches every ordered pixel write,
store-stream hash and full synthetic target hash in
[the cases](indexed-line-cases.json). No graph geometry or score writer was
rerun, and no owned assets are needed.

## Supported target and endpoints

Inputs are integer`(x0,y0,x1,y1,color)`, an8-bit software target, its byte pitch,
and a **nonempty clip rectangle contained in the bitmap**. Coordinates address
the bitmap directly; there is no implicit origin, console Y offset, transform,
line thickness or antialiasing. The tested target is32×24 with pitch40, including
eight untouched padding bytes per row, plus guard bytes around the allocation.

**Same-point requests draw nothing**, even inside the clip. The generic entry
delegates vertical lines first; its vertical helper rejects equal Y endpoints.
Do not replace this case with a one-pixel point primitive.

For every other admitted line, the endpoint coordinates are **inclusive**.
A segment clipped down to one visible pixel still stores that pixel. Horizontal
and vertical helpers sort their varying coordinate into ascending order;
diagonal/general lines normalize endpoints so X increases, swapping both X/Y
pairs when necessary. Reversing a segment therefore reproduces its normalized
ordered store sequence, not only its final pixel set.

## Exact stepping and clipping

For a general normalized line let`dx=x1-x0>0`, `dy=abs(y1-y0)>0`, and
`sy=sign(y1-y0)`. Choose X as major axis when`dx>=dy`, otherwise Y.
For major length`A` and minor length`B`, initialize
`error=2*B-A`. Emit the starting point. At each following major-axis step:

- if error **strictly>0**, advance the minor coordinate and add`2*(B-A)`;
- otherwise keep the minor coordinate and add`2*B`.

Then advance the major coordinate, continuing through the inclusive endpoint.
Thus ties at zero do **not** advance the minor axis. The reference uses this
integer rule for shallow, steep, rising and falling segments.

The clip is **left/top inclusive, right/bottom exclusive**. Preserve the error
phase accumulated from the original normalized endpoint while skipping the
clipped prefix. A valid equivalent is generating that original pixel sequence
and filtering it to the clip. Clipping geometric endpoints and restarting a
fresh error accumulator can select different pixels.

Native specialized loops stop at the applicable right/bottom/left/top limits;
the reference expresses the same bounded coverage by filtering. It does not
add an extra final clipped point or expand right/bottom edges. Negative and
wholly outside inputs participate in the original stepping/early-rejection
rules, with no coordinate clamp before establishing the phase.

The admitted arithmetic domain has signed32-bit differences/error terms that
do not overflow. Extreme integer-overflow cases and malformed bitmap metadata
are not established by these finite tests.

## Palette stores and state

Every selected pixel receives **`color &255`** as an unconditional byte store.
Color0 and color9 are opaque here; there is no color key, alpha blend, palette
lookup, shade or destination-dependent operation. Controlled colors include
0,9,65,79,241,255,−1→255 and`0x123`→35.

Native horizontal fills execute the original CRT memset, including wider
memory accesses. The receipts retain those raw accesses privately and normalize
their little-endian bytes into the same ordered per-pixel stream used by the
reference. Vertical and general lines execute their actual byte-store loops.
The starting-unlocked software target's cached pointer and lock count return
to their initial states. A target without software or hardware backing draws
nothing. DirectDraw/hardware lock behavior is outside this closure.

Independent segments include both endpoints, so consecutive graph segments
can store their shared endpoint repeatedly. Do not deduplicate those stores
when reproducing the primitive's requests. The routine's work-register return
is not assigned a new status-code meaning.

## Empty-clip diagnostic

The safe kernel profile requires a nonempty clip. Nine additional empty-clip
controls terminate with no stores, but one vertical segment crossing an
**empty-height** clip reaches its store loop with count0. Its assembly stores
before using`LOOP`, which would underflow that count and continue beyond the
intended span. The accepted hazard control stops **before the first store**;
it is not called a complete return or a valid empty-clip raster.

The portable reference rejects empty clips. A port wrapper should guard them
before entering this native-shaped kernel; silently treating the observed
unsafe native loop as a proved no-op would misstate the evidence.

## Coverage

The709 complete calls comprise700 nonempty-clip controls (including the
unbacked-target case) and nine terminating empty-clip diagnostics. They cover
625 ordered endpoint pairs, both endpoint directions, axis/diagonal/steep/
shallow and zero-error ties, negative/outside segments, all clip sides/corners,
single-row/column/pixel clips, same points and palette-byte truncation.
Guard and padding preservation are checked for all complete calls.

Run`python docs/behavior/indexed_line_reference.py` for the asset-free check.
Use the established graph line endpoints and clip directly with this kernel;
the graph's historical sampling, scales and series identity are separate
already closed contracts.
