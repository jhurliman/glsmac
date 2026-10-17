# CVR caller composition and initial position: original SMAC evidence

This experiment connects supplied frame/camera state to the existing
[fixed basis](cvr-basis-conversion-evidence.md) and
[indexed raster](cvr-indexed-pixel-evidence.md) arithmetic contracts. It executes
two original fragments with a deliberate gap: **A stops before lighting; B is
started separately using the actual matrix and location bytes captured from A.**
It does not execute a complete renderer or produce a Scout image.

The current retained run passes **44 paired cases, 23 pre-execution domain rejections
and 16 negative controls**, executing 36,893 original instructions. Every case
checks intermediate binary32 stores, all nine basis results and all three
initial-position results. Of those 44, 40 also satisfy the separately checked
raster arithmetic domain. No surface, clip bounds or palette is supplied or
validated by that classification.

## Reference and supplied state

The reference is the installed GOG PE32 `terran.exe`, 3,190,840 bytes, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
All addresses below belong to that image. `VI.cvr` is pinned to SHA-256
`d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1`.
Its selected first object, `EH_M1-Mask05`, has 17 frames and a simple flags-8
geometry header. Five cases use its frames 0, 1, 8, 14 and 16. The earlier
[Scout selection](cvr-scout-selection-evidence.md) established this file's place
in the explicitly parameterized Infantry/Gun/Scout selection; this experiment
does not rerun that selector or establish a default reactor or SMAX mapping.

The prior scene, composition, geometry, pixel and basis notes were read first.
The local human research index at `tmp/reference/re-terran-20261015/index.json`
provided candidate routine boundaries, including `0x654770`, `0x659800` and
the matrix helpers. Original disk bytes were independently decoded with
Capstone 5.0.7 and executed with Unicorn 2.1.4. Human research files, original
binaries and the interactive analysis session were not modified or operated.
No original asset, executable, array payload or third-party implementation is
committed.

Both fragments use supplied loaded-data structures, not the original loader.
The fixture supplies frame flag/location/matrix arrays, the geometry header,
one selected-object descriptor, a context with the optional pre-draw callback
set to zero, an explicit 3×3 transform `C`, translation `T`, positive admission
scale `k` and permissive admission bounds. These are experiment inputs, **not
an inferred camera, axis convention, projection or original default**. The
normal caller's camera setup, prepass and raster initializer are skipped.

One installed case supplies the previously observed component attachment
`(-400, 200, 0)` by adding it to the supplied location array with binary32 stores.
The other installed cases use raw locations. The attachment operation is not
executed as original code in this experiment; the earlier composition evidence
owns that claim.

## Fragment A: actual caller through basis conversion

Execution begins at `0x654849`, inside the object loop of `0x654770`, with an
explicitly seeded loop register/stack ABI. It reaches the actual call at
`0x6549e9` into `0x659800`, executes that routine's prefix and its original
`0x65a930` conversion, and **halts at `0x659827` before the lighting call**.

| Executed half-open range | Tested role |
| --- | --- |
| `0x654849..0x6549ee` | Select frame arrays; transform location; choose simple geometry; test admission; compose/scale matrix; call draw entry. |
| `0x641e70..0x641edb` | Row-major matrix times column vector. |
| `0x641ee0..0x641f05` | Vector addition with three binary32 stores. |
| `0x652a50..0x652a91` | With supplied flags 8, returns the same geometry pointer; hierarchy/LOD traversal is not exercised. |
| `0x6420a0..0x642137` | Original admission predicate using the supplied bounds and scale. |
| `0x641bb0..0x641cdf` | Row-major matrix multiplication. |
| `0x659800..0x659827` | Actual draw-entry prefix and original basis-conversion call. |
| `0x65a930..0x65ab42` | Original nine-value basis conversion. |
| `0x660678..0x66069f` | Original x87 signed64 conversion helper. |

For admitted inputs, let `S32` mean one nearest-even binary32 store. `L` and
`F` are the supplied location and matrix for the chosen frame; `s` is the
geometry header's binary32 scalar at offset 8. The observed staged results are:

```
P0 = S32(C × L)                  # one store per vector entry
P  = S32(P0 + T)
M0 = S32(C × F)                  # one store per matrix entry
M  = S32(M0 × s)
```

The probe compares bytes at the returns from both location helpers, the matrix
helper and the draw-entry boundary. It captures the actual eight draw arguments,
including pointers to `P` and `M`, the unchanged geometry pointer and the
footprint argument. It then compares original basis globals, scaled binary32
scratch values, two cleared scratch words, all nine signed64 helper results
and their ordered writes with the independent oracle.

The tested footprint argument is one when `s*k <= 0.5`, otherwise zero.
Here `k` is the exact value of the binary32 bytes supplied to the guest. The
probe serializes the requested Python scale once, then uses those bytes for both
domain validation and the footprint oracle. For example, `0.50000001` becomes
exactly `0.5`; with scalar one the original argument is one. The positive value
`1e-100` becomes zero and is rejected before execution. The report retains both
the requested scale and the supplied four bytes.
Varying `k` across that boundary changes this argument while leaving `P`, `M`
and the basis unchanged. This is an argument/dataflow observation, not execution
of the footprint's eventual pixel-writing behavior. An out-of-bounds supplied
location is rejected by the original admission routine; the harness then stops
fail-closed at the unapproved continuation `0x654a03`.

## Fragment B: separately seeded origin calculation

A fresh emulator starts at `0x659c75` and halts before `0x659d65`. The matrix
and location storage is initialized from **A's captured bytes**, never from the
oracle's recomputed bytes. The same geometry header is supplied. The fragment
executes the original matrix-vector, vector-addition and signed64 conversion
helpers listed above; there are no substituted functions.

This branch reads the three signed16 fields at header offsets 32, 34 and 36,
converts them exactly to binary32, and computes:

```
H  = signed16 vector from geometry header offsets 32, 34, 36
Q0 = S32(M × H)
Q  = S32(P + Q0)
O  = round_half_away_from_zero(Q × 256)   # per coordinate
```

The `Q × 256` rounding sequence uses the x87 value directly, adds +/-0.5 and
calls the original truncation helper. Unlike the basis converter, this fragment
does not store that scaled value as binary32 before conversion. For the admitted
finite domain, multiplication by 256 is exact and the difference does not alter
the oracle result. All three original qwords must fit signed32 before their
observed dword destinations are accepted. At the boundary, X is in the stack
local at `ESP+0x7c`, Y in EBP and Z in EAX.

The connection from flags 8 to this branch is static caller evidence: the
simple-path dispatch tests flags masks `0x2` and `0x4` before this fragment. **The
intervening lighting, bounds/prepass and dispatch instructions are not executed.**
In particular, `0x65ab50` and its lighting-table access at `0x6c2e88` remain a
separate dependency, as do the later `0x65b060` bounds helper, clipping, palette
construction, raster initialization and drawing. B's supplied ABI is not a
claim that these skipped operations have no other side effects.

## Arithmetic and execution boundaries

The independent oracle uses exact dyadic rationals. Every product, each possible
pair sum in a dot product and its final sum must fit the x87 64-bit significand
exactly; this makes the helper's varied instruction scheduling immaterial within
the admitted subset. The oracle rounds to binary32 only at the observed store
boundaries, using integer nearest-even rounding. It does not use host `float`
matrix multiplication or assume a sequence of binary32 additions matches x87.

Store classification follows rounding: a positive exact value at or below the
`2^-150` midpoint can store as positive zero. The exact midpoint immediately
below minimum normal, `2^-126 - 2^-150`, stores as minimum normal by the same
nearest-even rule, for either sign. Nonzero subnormal **results** remain outside
the admitted domain, as do generated negative-zero results. This extends the
earlier pre-rounding normal-exponent restriction; it does not model all original
subnormal or signed-zero behavior.

Requests with nonfinite inputs, negative-zero inputs, rounded nonzero subnormal/overflow stores,
inexact extended intermediates, invalid array extents, unsupported header flags,
nonpositive scalar/admission scale, or out-of-width basis/origin results are
outside this probe's admitted domain. These restrictions are experiment policy,
not original input validation or a complete original floating-point model.
Ordinary positive zero and cancellation-to-zero cases are covered; signed-zero
propagation and exceptional/overflow behavior are not generalized.

Both fragments begin with x87 control word `0x037f` and an empty x87 stack.
Each returns to its specified boundary with the same control word and empty
tags/TOP. Every conversion checks the original saved/truncation control-word
writes (`037f`, `0f7f`). Status is recorded separately and may change; this run
observes `0000`, `0100` and `4000`, with no invalid/divide/overflow/underflow flags in
Unicorn. Hardware FP status equivalence is not claimed. Returned helpers check
their cdecl stack and callee-saved registers; the two incomplete containing
functions are checked at their explicit fragment boundaries, not presented as
normal function returns.

Only decoded instruction starts in the declared ranges may execute. Read/write
allowlists distinguish selected frame fields, supplied global state, original
constants, basis outputs and bounded stack storage. Code writes are prohibited;
the lighting call cannot execute. The report records complete ordered
instruction and memory traces, supplied byte ranges, stage captures, original
range hashes, final register/status values and oracle results. Whole arena,
global and stack snapshots are reconciled with observed writes; canaries and
code/constants must remain unchanged. Each phase has a 4,000-instruction budget
and two-second emulator timeout.

## Cases, controls and separate raster admissibility

The first 29 cases cover five installed frames, three noncommuting synthetic frames,
four scalar values, three header vectors including signed16 endpoints, four
translations, one large depth-basis case, eight admission/footprint scales and
one explicitly supplied attachment case. The controls independently reverse
matrix multiplication, transpose the matrix and omit scalar, translation or
header-vector contribution. All five fail at a stage-byte assertion. Both phases
also independently deny code, reads, writes and instruction budget; each fails
at the intended boundary. The final control leaves the admitted original-caller
branch through its real admission predicate. Four scale cases straddle the
binary32 rounding boundary around `0.5` with scalar one, including requested
values on both sides that serialize to exactly `0.5`. A fifteenth control uses
the unrounded requested scale for the oracle and fails specifically at the
actual renderer-argument comparison.

Fifteen further synthetic cases execute the actual matrix, scalar and location
stores at the new rounding boundaries. They include `2^-200` and `2^-250`, the
zero midpoint and its lower binary32-operand neighbor, minimum-normal midpoint
and exact minimum normal for both signs, scalar-store equivalents, and location
stores followed by exact cancellation before the independent origin check.
`2^-150 - 2^-213` exercises a 64-bit rounding shift. Six additional domain
rejections cover its positive perturbation, the upper zero-midpoint neighbor,
negative underflow to zero and both signs below the minimum-normal midpoint.
A sixteenth control flushes the minimum-normal oracle result to zero and fails
at the actual matrix-product stage comparison. The original code is unchanged.

Twenty-three malformed/unsupported requests reject before original execution. The
report retains the precise reason for each. No control is counted merely for
throwing an arbitrary exception.

Signed-width conversion validity is checked first. The separate raster policy
then computes all 26 direction steps, checks signed32 sums and shifted depth,
and walks the selected installed stream's 1,385 nonterminal records in order.
It classifies 40 cases as arithmetically admissible. Two more fail the initial
shifted-depth bound, one fails a cumulative depth bound and one fails a shifted
step bound. This classification executes **no original raster code** and checks
no surface extent, color/lighting lookup or draw footprint. Even its admitted
negative X/Y values are not a statement that a particular surface can draw them.

## Retained evidence and next implementation boundary

Run with the pinned Python dependencies and user-owned installed files:

```powershell
python tools/reference/probe_cvr_caller_origin.py `
  --exe 'C:/Program Files (x86)/GOG Galaxy/Games/Alpha Centauri/terran.exe' `
  --assets 'C:/Program Files (x86)/GOG Galaxy/Games/Alpha Centauri' `
  --report out/validation-cvr-caller-origin/caller-origin-report.json
```

The original 25-case report remains retained in the `unit-weapon-table-parser`
managed tree:

| Artifact | SHA-256 |
| --- | --- |
| Probe source | `293b1b4a8dbf0725e8a55104f09f897af31079a26c2e4e35a336b68699b89c03` |
| `out/validation-cvr-caller-origin/caller-origin-report.json` (3,131,131 bytes) | `e087d08630fa8ac9b7ad2b200c183bbc385bd53848c9f3c1488f512544a0fb2a` |
| Imported scene/PE helper | `5c97f8ebda930909d78896c48b764f5aea8ae5322cbdcf2a8dd2b9df1b59f201` |
| Imported geometry-stream helper | `dee511e774ffa42cce36c8e63c4ff4820b42b3b2cc2831dc5bbf80d8de55c167` |
| Imported basis oracle | `fae91916c120d3518dcb2ce6d776d04e7d29833695a871b5efc887845fd6a46a` |

A second complete run reproduced those original report bytes in 2.37 seconds.
GitHub review then identified the pre-serialization scale error described above.
The corrected run and expanded boundary/control cases are retained separately in
the `camera-review-fix` managed tree:

| Historical scale-correction artifact | SHA-256 |
| --- | --- |
| Probe source | `a0a9e485b90a8b2229d86a59f88bfbf8c80204899be764db6c0a08b73f21c163` |
| `out/validation-cvr-caller-origin-scale/caller-origin-report.json` (3,631,414 bytes) | `a431e72dce2b475b90b0c715685774e7e6c53e39e8c8323909d88cd0fbc7f822` |

After the dependency refresh, the complete 29-case publication probe was rerun
with the current geometry-stream helper, whose SHA-256 is
`502db5ce7cdd9e1538d1920992f31cab1293cb77beb8dbf2a872ba809209e750`.
It passed all 29 paired cases, 17 domain rejections and 15 negative controls
(24,351 original instructions). The other helper hashes and probe source are
unchanged. The new 3,631,414-byte report is retained in this same managed tree at
`out/validation-cvr-caller-origin-current/caller-origin-report.json`, SHA-256
`84f50774da8c57ed5ec2380ee2e1e9e60ab8229149369aeed9ccecb1147c7b21`.
The earlier report used the earlier helper and remains historical evidence;
its hash is not presented as the output of the current source stack.

The rounded-store extension and complete combined run are retained in the
`unit-weapon-table-parser` managed tree:

| Current artifact | SHA-256 |
| --- | --- |
| Probe source | `e9696bf1cfc6a9b88b458ca66866ceea9b3a0ea12a5ded97351fba0866c70db2` |
| `out/validation-cvr-transform-store/caller-origin-report.json` (5,498,649 bytes) | `9e10d7d6f3a0ab8b919b5bafc130fe4dbe85aa00fb312a6da5f61f270cd24662` |

This combined receipt includes the case-sensitive-filesystem correction to
the imported geometry-stream helper, now
`502db5ce7cdd9e1538d1920992f31cab1293cb77beb8dbf2a872ba809209e750`.
The other imported fingerprints are unchanged. The preceding 44-case receipt
remains at `out/validation-cvr-transform-store/caller-origin-report-before-path-fix.json`;
the refreshed report differs only in that imported-helper fingerprint.
Python bytecode compilation and
`git diff --check` pass. The separate
[calculator validation](cvr-transform-calculator.md) covers the production
implementation. Neither check claims a rendered-game smoke.

The next supported implementation boundary is an explicit-input composition
and origin calculator preserving these staged binary32 stores and checked
arithmetic limits, feeding the existing basis converter and indexed rasterizer.
An exact integer/dyadic implementation or another proven rounding model is
needed; this evidence does not authorize silently replacing x87 dot products
with ordinary host `float` operations. Inputs must continue to supply transform,
translation, selected frame, scalar and header vector. The original camera
builder, lighting/palette dataflow, clipping and a complete conventional-unit
render path remain independent work; no live-unit or default-camera hookup is
supported by this experiment alone.
