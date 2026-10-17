# Explicit CVR composition and origin calculator

`loader::cvr::CvrTransformCalculator::Calculate` implements the bounded arithmetic
from the [original caller/origin experiment](cvr-caller-origin-evidence.md).
Its Release production probe matches all 44 retained original cases: **660
binary32 output bit patterns and 528 integer results**. The calculator has an
explicit-input CLI caller; no game, scene renderer or runtime unit is activated.

## Inputs and owned output

`CvrTransformInput` contains row-major binary32 **bit patterns**, not host floats:
the supplied transform `C` (nine), translation `T` (three), selected frame matrix
`F` (nine), selected location `L` (three), positive geometry scalar `s` (one), and
the geometry header's signed16 vector `H` (three). There are no camera, axis,
projection, attachment, frame-selection, reactor or palette defaults. These
inputs must be selected by a caller with evidence for that selection.

The returned value owns nine composed-matrix words, three translated-location
words, three origin words, a 3×3 fixed basis and three signed32 initial
coordinates. It contains no pointers into the request, scene or geometry. All
work is local; input remains unchanged and a rejected request returns no partial
result. The successful path needs no dynamic storage; constructing an exception
may allocate.

With `S32` denoting one nearest-even binary32 store, the staged calculation is:

```
P = S32(S32(C × L) + T)
M = S32(S32(C × F) × s)
Q = S32(P + S32(M × H))
basis  = existing CvrBasisConverter(M)
origin = round_half_away_from_zero(Q × 256), checked signed32
```

Each dot product stores once after its extended-precision sum. Each later
addition or scalar multiplication stores again. In particular, `(1 + 2^-24)`
stores as 1 before a scalar multiplication by 3; combining the two operations
would produce a different binary32 result. No host `float`, `double`, fused
operation or floating-point rounding mode evaluates these expressions.

## Arithmetic and rejection domain

Private helpers represent exact signed dyadics as an odd unsigned64
significand, exponent and sign. Products are checked before multiplication.
Addition aligns into two unsigned words only after checking the exponent gap.
For unequal exponents the low term is odd: a high term wider than 65 bits cannot
be canceled to 64 significant bits by the at-most-64-bit low term. Such requests
reject before a shift. The remaining 65-bit carry/borrow is normalized and must
fit 64 significant bits. Zero/cancellation and shift counts 0 and 64 have
explicit paths; no `__int128`, signed shift or arbitrary-precision library is
required.

Every product, all three possible pair sums in each dot product and the final
sum must be exact in the original x87 64-bit significand. Store conversion uses
integer nearest-even rounding to 24 significand bits, including an exponent
carry. Below minimum normal it first rounds on the `2^-149` lattice, then
classifies the result. Positive values at/below `2^-150` may round to positive
zero, and either sign of `2^-126 - 2^-150` rounds to minimum normal. Exact
nonzero subnormal lattice values, and values rounding to a nonzero subnormal,
remain rejected. Right shifts below, equal to and greater than 64 have separate
bounded paths; no oversized shift is evaluated.

The original origin fragment's `abs(Q*256)+0.5` must also be exact before
truncation. For example, both signs of `Q=2^-72` are admitted and round to zero;
both signs of `2^-73` reject because that half-shift requires 65 significant
bits. Reusing the existing converter's signed32 row is valid after this check:
the power-of-two scaling is exact in the accepted range, even though the
original origin fragment does not store the scaled value as binary32.

The calculator rejects with `std::invalid_argument`:

- NaN, infinity, supplied negative zero, and a nonpositive geometry scalar.
- Generated negative-zero stores. Zero signs are retained through private
  products/sums so these cannot silently become positive zero. This is an
  explicit narrower restriction where the rational evidence oracle does not
  generalize original signed-zero propagation.
- An intermediate requiring more than 64 significant bits, or a rounded
  nonzero subnormal/overflow store. Subnormal **inputs** are allowed
  when their eventual products/stores satisfy this rule.
- A basis result outside signed16 in its first two rows or signed32 in its last
  row; an origin result outside signed32.

These are GLSMAC supported-domain restrictions, not original malformed-input
behavior or a model of all x87 precision modes, exceptions or status flags. The
calculator neither reads nor changes the host FP environment. Positive zero and
exact cancellation are supported.

The existing rasterizer must still validate direction steps, shifted depth,
cumulative coordinates, surface bounds and the draw request. In particular the
calculator deliberately accepts the four conversion-valid original cases whose
later raster arithmetic classification rejects. It performs no clipping and
does not turn those cases into valid draws.

## CLI and comparison with original execution

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Werror -pedantic -Isrc \
  src/loader/cvr/CvrTransformCalculator.cpp src/loader/cvr/CvrBasisConverter.cpp \
  tools/tests/cvr_transform_probe.cpp -o /tmp/cvr-transform-probe
python3 tools/reference/compare_cvr_transform.py \
  --report /private/caller-origin-report.json --probe /tmp/cvr-transform-probe
```

The comparator requires exact report SHA-256
`9e10d7d6f3a0ab8b919b5bafc130fe4dbe85aa00fb312a6da5f61f270cd24662`, executable
hash and original-probe source hash. It supplies the recorded inputs to the
actual C++ calculator, then compares matrix/location/origin words with captured
original stage-memory bytes and the integer results with the original helper's
recorded signed64 outputs. It imports no arithmetic oracle and does not execute
the original. The proprietary report remains ignored and uncommitted.

The opt-in CLI accepts one line containing 25 uint32 hexadecimal words, in
`C9 T3 F9 L3 scalar` order, followed by three signed16 decimal header values.
Lowercase `0x` prefixes are optional. A fixed 2,048-byte line buffer bounds the
request. It prints `ok`, then 15 hex words (`M9 P3 Q3`), nine decimal basis values
and three decimal origin values; arithmetic rejection prints `invalid`.
Malformed syntax/stream input exits 2. Accepted request streams exit 0,
including explicitly reported arithmetic rejection. This is a test interface,
not a file, save or network protocol.

## Validation

Seven native groups exercise noncommuting composition, all staged stores, even
and odd halfway values with signs, rounding carry, exact `2^64-1` borrowing and
three-term `UINT64_MAX+1` carry, excessive exponent separation, exceptional
inputs in all 25 fields, generated negative zero, normal products from
subnormal inputs, signed header/origin endpoints, the origin half-shift
threshold, owned output and all four standard host rounding modes. The new
group checks positive zero and signed minimum-normal boundaries in every matrix
cell, scalar and location stores, including exact 64-bit rounding shifts and
larger shifts. It rejects the neighboring nonzero subnormal and negative-zero
results. These cases run under all four host rounding modes without changing
the caller's mode.

- Full normal CTest after the rounded-store correction: 4/4, 17.17 seconds. This isolated publication ancestry
  predates the later primary suite; it is not a claim about that suite.
- The same seven groups pass focused native execution with `--memorydebug` and
  ASan/UBSan/LSan `detect_leaks=1`, with no reported memory leaks. Ignored copied
  registration TUs select the groups while linking the actual unchanged
  production/application objects. The local sanitizer non-PIE cache workaround
  does not change repository build policy.
- Six earlier copied-production controls transpose output, round ties away, omit the
  scalar, omit the header contribution, omit the origin half-shift exactness
  check, or incorrectly reject the 65-bit borrow temporary. All return exit 1
  from the native assertions; intended checkout source is never replaced.
  For the rounded-store correction, the original production reader with the
  new native group returns exit 1: six groups pass and the new group fails at
  `rounded positive zero or minimum-normal product rejected`, with no tracked
  leaks. The fixed implementation passes. The final original-code comparison
  includes the 29 scale-corrected caller cases and 15 new store cases.
- Explicit production-TU correctness analysis passes in Debug/Release with
  zero findings (1.28/1.28 seconds). The warning-clean Release CLI matches
  all 44 original cases after the final arithmetic changes.
- Previously, eleven malformed CLI requests returned exit 2 without output; four arithmetic
  rejection requests print `invalid` and exit 0. Python syntax and
  `git diff --check` pass.

Evidence is retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/transform-store-*` (prior checks:
`transform-*`) and each build's
`transform-validation/`. Independent read-only arithmetic review found no
actionable issue; that review did not rerun the original executable.

The calculator adds no camera builder, supplied-state selection, lighting,
palette, clipping, draw dispatch or conventional-unit hookup. Those remain
separate prerequisites for a faithful rendered Scout and do not follow from
this explicit arithmetic boundary alone.
