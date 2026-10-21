# Workshop yaw beyond64 radians

2026-10-21. Original executable SHA-256 remains
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
**Remove an adapter's arbitrary abs(yaw)<=64 condition for the native
accumulation path.** The native main painter has no such bound and no explicit
modulo reduction. Keep finite-input validation and the original binary32
accumulation contract; do not wrap stored yaw each turn.

One new complete main frame at65.0f passes independent owned-asset frame,
bounds, centering, view and all64 draw-transform checks, plus full indexed
and depth replay of71 raster calls. As in the
[nonzero supplement](design-workshop-nonzero-animation.md), the latter is
kernel-stage replay with recorded kernel entry operands, not a newly independent
asset-to-kernel renderer. Camera/material initialization retains the same
declared stage inputs. No port output or desktop screenshot is an input.

[Scalar cases and hashes](design-workshop-yaw-range-cases.json).
Private additive freeze:
`tmp/reference/independent-workshop-yaw65-evidence-20261021/`.
Manifest SHA-256:
`ea67de608a9dfb9f8a23f4fbc3f46d36bbb368722aa18e57b1b7c7fabfa0cb2e`.
There are152 payloads.

## Actual native path

`43fac2..43faee` passes the old stored yaw, then0,0 to6324b0. That wrapper
passes the three-angle vector directly to641930. The latter executes FSIN
and FCOS without a software yaw bound, clamp, remainder helper or C2 test.
After handing off the old angle, the painter adds constant3d4ccccd and stores
to its binary32 yaw field. The rendering function itself does not mutate that
field or substitute a reduced angle.

The full-frame control uses Infantry/Laser/Synthmetal/Fission, owner1,
selected custom127, no abilities, shared phase1 and x87 control word037f.
Input yaw42820000 is65.0f. After drawing, yaw is4282199a, approximately
65.05000305175781; shared phase remains1. Indexed SHA-256:
`898589b479879c3ad830a1e11256c94895a523977850d23bbfe19de2a13e9752`.
Depth SHA-256:
`4057549681e69ed5e39930ebde0ffeedcf57a1f07aef9cae04745d78c59ad697`.

## Reachable interval and instruction admission

Starting at0, nearest-even binary32 stores of the positive0.05f increment
increase yaw until1048576 (2^20). Below that endpoint, binary32 spacing is
at most0.0625, so half a spacing is less than the increment. At the endpoint
spacing becomes0.125 and0.05f is less than half a spacing, so the addition
stores1048576 unchanged. The immediately preceding representable value
1048575.9375 advances to1048576. This arithmetic argument covers the interval;
it is not a claim of an exhaustive multi-million-frame raster run. The existing
native yaw-producer controls separately verify the stalled endpoint.

The entire reachable interval0..1048576 lies inside the accepted finite
argument range of x87 FSIN/FCOS, strictly between-2^63 and+2^63; C2 reports
arguments outside that range. This is instruction admission, not a guarantee
that hardware transcendental approximation equals a host libm result for every
argument. Intel documents the distinction and its internal range-reduction
precision limits. See [Intel SDM Volume2A, FSIN/FCOS](https://cdrdv2-public.intel.com/868140/253666-089-sdm-vol-2a.pdf)
and [Volume1, transcendental instructions](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf).

Eight complete original641930 helper controls use0,1,64,65,128,65536,
1048575.9375 and1048576 radians with the other two angles0. All48 individual
FSIN/FCOS instruction results have C2 clear, and all stored matrix components
are finite. Independently computed binary32 yaw matrices match every output
word, including the helper's signed-zero schedule. These execute original x86
under the evidence harness; they do not certify all physical x87
implementations or every yaw's raster bits. There is no evidence supporting
the port's64 bound. Preserve the original accumulated input angle rather than
changing it to a binary32 modulo2π state.
