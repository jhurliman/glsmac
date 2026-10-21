# RE-SMAC-SCOUT-IDLE-VIEW-001: bounded map-idle view producer

The [Spartan Recon extension](spartan-recon-composition-and-view.md) now checks
the actual generated Speeder composition in20 views. Its ten-part scene omits
this Infantry design's1.25 multiplier; apply that class distinction before
reusing the common view math below. Its initializer-produced map light and
declared initial-zero actor remaps have separate fixture provenance.

For general designs, use the [exact ordinary scale predicate](ordinary-actor-scale-predicate.md).
The1.25 selection reads stored chassis/weapon IDs before any Probe body remap;
chassis0 noncombat and several combat weapons instead use0.8.

Status: **original-x86 checked** in20 complete internal-draw cases, with
independent generation of the view, auxiliary matrix, scales and target anchor.
All1,180 projected object matrices/locations and lighting-input sets match.
The [acceptance cases](scout-idle-view-cases.json) supply owned caller inputs,
independently calculated outputs and original indexed/depth fingerprints.

This is the reactor1 Infantry/Gun/Scout-armor design2, faction1, VI plus
viptr00, frame0/animation counter0 path. It uses flat orientation with the
slope switch off, draw flags0, world/model position0, default actor offsets,
a256×256 internal surface and supplied target clip(0,0,512,512). Palette
override bits are clear and active shadows are excluded. These conditions
bound the adapter; do not generalize its formulas to other designs or slopes.

## Facing and the flat auxiliary transform

`R32(x)` means nearest-even binary32 storage. Start with the signed unit-facing
input. Facing−1 is changed to0. The checked ordinary facings are0..7. Set:

```text
k = (facing - 3) & 7
phi = R32(k * 0.7853749990463257)
c = R32(cos(phi))
s = R32(sin(phi))
R = [ c, -s, 0;
      s,  c, 0;
      0,  0, 1 ]
```

The facing step is the stated binary32 constant, **not exact pi/4**. Substituting
pi/4 changes19 of the20 checked view matrices; the unchanged case has k0.
Facing0 therefore uses phi3.926875114440918. Facing−1 matches facing0.

The flat helper starts from a fixed angle `t` whose binary32 bits are
`0xbf85f92d`. Define `u=R32(cos(t))`, `v=R32(sin(t))`. It produces the full
auxiliary matrix:

```text
B = [ 1, 0,  0;
      0, u,  v;
     -0, v, -u ]
```

Thus `u=0.500459611415863`, `v=−0.8657598495483398`. These values are derived
from the flat helper's angle, not unexplained captured-matrix defaults. Preserve
the indicated zero sign where the implementation supports it; later lighting
atan2 has demonstrated signed-zero sensitivity at poles.

The caller constructs the auxiliary position from this matrix's third row:
`P[j]=R32(−10000*B[6+j])`, giving
`(0,8657.5986328125,5004.59619140625)`. It also supplies light
`(−L,+L,+L)`, where L has binary32 bits`0x3f13cd36`
(0.5773500204086304). The full matrix B and this light feed the
[complete lighting selector](scout-lighting-generation.md).

## Scale and screen-anchor calculation

Caller inputs include zoom `z` and an optional integer scale override `n,d`.
The checked denominators are positive. If `n>0`, the override takes priority
over zoom. With binary32 constant `h=0.07000000029802322`, calculate:

```text
base = R32(h*n/d)                 if n>0
base = R32(h*(z+16)*0.0625)       otherwise
scale = R32(base*1.25)            for this Scout

delta = trunc(50*n/d)             if n>0
delta = trunc(50*(z+16)/16)       otherwise
target_anchor = input_anchor + (delta,delta)
```

Intermediate base is stored before the Scout's1.25 multiplication. The1.25
factor comes from the actual design/chassis selection path after the map
caller sets its base scale; omitting it changes all20 matrices. It does not
change the integer anchor delta. Preserve integer truncation toward zero for
the delta, rather than rounding the floating-point scale into pixels.

Normal zoom0 without an override yields base approximately0.07,
scale0.08749999850988388 and delta50. Zoom−1 yields scale0.08203125 and delta46.
An override1/2 yields scale0.04374999925494194 and delta25 even when zoom5 is
supplied. The cases cover zoom−8,−1,0,4,8 and overrides1/1,1/2,3/2,2/1.
Other zoom ranges, invalid override denominators and negative scales are open.

## Projection, translation and per-object inputs

For the admitted defaults, form `Q=R32 componentwise(B*R)`. The projection
matrix C is Q with its first two rows multiplied by `scale`, and its third
row multiplied by0.5, storing every resulting component as binary32.

The0.5 depth scale comes from the default depth mapping: view near/far are
−65535/+65535 and output depth endpoints0/65535, so the ratio is
`65535/(65535−(−65535))`. The admitted perspective parameter is0 and its
factor is1. Global lighting scales are `(scale,0.5)`.

The flat helper places the auxiliary origin10000 units behind the model.
For the supplied zero world/model position and default offsets, the complete
prepass produces translation:

```text
T = (256/2 + 0.5,
     256/2 + 0.5,
     (10000 + 65535)*0.5)
  = (128.5,128.5,37767.5)
```

The half-pixel offset and depth translation follow the original default
surface/depth mapping. They are not target-screen coordinates. Changing only
the target anchor leaves C, B, the scales and T unchanged. This contract does
not establish arbitrary surface sizes, nonzero actor/world positions or
custom depth/perspective parameters.

For each visible frame, let its decoded asset matrix/location be F,p, and
its decoded geometry scalar be g. The caller supplies:

```text
A = R32 componentwise(R32 componentwise(C*F) * g)
location = R32 componentwise(R32 componentwise(C*p) + T)
```

The lighting selector gets A, the fixed light, geometry scalar g, full B,
and global scales `(scale,0.5)`. **Geometry scalar g is separate from map
scale**; obtain it from the supplying geometry metadata. Do not replace all
per-object matrices or geometry scalars with a single captured example.
All59 asset frame matrices/locations/scalars remain captured inputs in this
research comparison; they should come from the port's owned asset decoder.

Original matrix/vector arithmetic keeps extended64-significand precision
between binary32 stores. The independent projection oracle uses exact
rationals rounded at each extended operation/store. Its host-double sin/cos
matches all checked binary32 orientation/auxiliary outputs; universal x87
transcendental bit equality is not established. Intermediate zero signs compare
numerically equal in these checks; this is not exhaustive signed-zero testing.

## Controls and remaining boundaries

All20 cases execute the original idle caller, assembly, complete view prepass,
per-object bounding/projection/lighting and internal draw, stopping before the
final target copy. Nineteen dispatch65 kernels each. Anchor(500,500) produces
target anchor(550,550) and dispatches no raster kernels under the supplied
target clip; all59 projected object inputs still agree with the formula.
That case retains the supplied clear9/depth65535 buffers.

Anchors(0,0) and(10,20) produce target anchors(50,50) and(60,70), respectively,
and the same internal indexed/depth fingerprints as the baseline. Final
software copying/clipping/cleanup remains governed by the separate
[target-copy contract](scout-target-copy.md). The color sources are the
[generated first lookup](scout-lighting-generation.md) and
[decoded asset shade/RGB data](cvr-palette-and-shade-decode.md).

This establishes a bounded production adapter's view inputs without captured
camera defaults. It does not prove a completely independent end-to-end renderer:
asset frame/geometry decode, raster origins/bases and the full lighting path
retain their separately stated verification scopes. Slopes, animation, other
designs, nondefault draw flags, active shadows, faction palette overrides,
hardware/window palette lifecycle and a live original screenshot remain open.

The [exact matrix arithmetic follow-up](cvr-matrix-arithmetic.md) now specifies
the cell-specific product/addition schedule, rounded64-significand intermediates
and signed-zero stores. All59 baseline matrix products and following scaled
words match bit for bit, with111 synthetic matrix and118 vector diagnostics.
Use that schedule rather than a uniform dot product or exact-only rejection.
