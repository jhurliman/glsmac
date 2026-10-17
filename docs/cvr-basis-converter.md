# Explicit binary32 to CVR fixed basis

`loader::cvr::CvrBasisConverter::Convert` turns nine explicitly supplied IEEE
binary32 bit patterns into a three-by-three signed integer basis. It implements
the supported arithmetic output of the
[original conversion experiment](cvr-basis-conversion-evidence.md). All **41
original cases and 369 values match** through a standalone production caller.

The API takes `std::array<uint32_t, 9>` and returns an owned
`std::array<std::array<int32_t, 3>, 3>`. It preserves flat component order: entry
`i` becomes `[i / 3][i % 3]`. Successful conversion needs no heap storage and
retains no input references. Invalid input throws `std::invalid_argument` with a `CVR basis:`
prefix; no partially filled result is returned and supplied bits remain unchanged.

These are already supplied matrix bits, not an automatic interpretation of
`CvrFrame::matrix`. No camera, axis conversion, frame/header/scalar composition,
lighting, default matrix or live unit hook is introduced. The returned type can
be passed to the [explicit-input indexed rasterizer](cvr-indexed-rasterizer.md),
whose independent step/depth/coordinate validation must still succeed.

## Arithmetic contract

The original scales each value by 256, stores binary32, adds a sign-selected
half, and truncates through its x87 helper. In the admitted domain the result is
nearest-integer rounding, with halfway values away from zero. First-row and
second-row results must fit signed16; third-row results must fit signed32.
Positive/negative zero both return integer zero. Subnormals are accepted and
return zero after scaling and rounding.

The implementation performs no floating-point operations. It extracts sign,
biased exponent and significand, then evaluates the scaled value as a power of
two using unsigned64 arithmetic. For a normal input the power is `exponent-142`;
for a subnormal it is -141. Scaling by 256 is exact within the accepted output
domain. For right shifts up to 24, adding the half-bit before shifting performs
the required magnitude rounding. Larger right shifts necessarily round to zero.
Exponents above 150 reject before any left shift, so admitted left shifts are
at most eight bits. Destination bounds are checked before signed conversion.

This avoids signed overflow, excessive shifts, implementation-defined signed
right shifts, host floating-point rounding, and flush-to-zero settings. The
converter neither reads nor changes the caller's FP environment. It reproduces
the tested numeric output under the original experiment's x87 `0x037f` profile;
it does not reproduce x87 scratch storage, control/status flags or other original
control-word modes.

Nonfinite values and rounded results outside the destination widths reject.
In particular, positive 32768 is invalid in the first six positions, while
negative -32768 is supported; the third row supports -2147483648 but rejects
positive 2147483648. The largest representable admitted positive binary32 input
there produces 2147483520. These are GLSMAC domain checks, not assertions about
the original's malformed-input or narrowing-wrap behavior.

## Production probe and original comparison

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -Wall -Wextra -pedantic -Isrc \
  src/loader/cvr/CvrBasisConverter.cpp tools/tests/cvr_basis_probe.cpp \
  -o /tmp/cvr-basis-probe
python3 tools/reference/compare_cvr_basis.py \
  --report /private/basis-report.json --probe /tmp/cvr-basis-probe
```

The comparison requires the exact retained report hash from the evidence note:
`4da27525e353d93085a608a1f6243a8664e86bb0a985be0d9335196d2fc1a7cb`.
It supplies the recorded input bits to the actual C++ converter and compares
every returned value to the original helper's recorded signed64 results. The
tool creates no replacement rounding oracle and does not run the original.

The standalone probe accepts one line of nine hexadecimal uint32 words, with
optional lowercase `0x` prefixes. Lines use a fixed 1,024-byte buffer. It prints
`ok` followed by nine decimal values, or `invalid` for a rejected arithmetic
request. Malformed syntax/stream input returns exit 2; accepted request streams
return exit 0, including explicitly reported arithmetic rejection. This is an
opt-in test interface, not a save/network format. Private original reports and
generated evidence are not committed.

## Validation

Five native groups exercise original identity/nonidentity outputs, halfway
neighbors with both signs in every position, signed zeros/subnormals, all four
standard host rounding modes, exact destination boundaries and nonfinite/large
inputs in every position. FP-mode tests restore the prior mode on every exit.

- Full normal CTest: 4/4, 17.26 seconds, including the new native cases. This
  isolated publication base predates the later primary eight-test suite.
- The same five groups pass focused `--memorydebug` and ASan/UBSan/LSan with
  `detect_leaks=1`. Ignored copied registration TUs select the new groups while
  linking unchanged production/application objects. The existing local
  sanitizer `-no-pie` cache workaround does not change repository policy.
- Four copied-production controls truncate instead of rounding, transpose the
  output, relax signed16 bounds, or ignore signs. Each returns exit 1 from the
  real native assertions; intended checkout source remains unchanged.
- Explicit production-TU Debug/Release correctness analysis passes in
  0.43/0.42 seconds. The standalone Release probe compiles cleanly with the
  warning flags shown above and matches all 41 original cases.
- Seven malformed Release-probe inputs return exit 2 with no output; two
  arithmetic rejection inputs return `invalid`. Python syntax checks and
  `git diff --check` pass.

Logs/comparisons are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/basis-*`; focused binaries/logs
are under each build's `basis-validation/`. The renderer caller's actual
matrix composition, initial sample position and lighting/RGB preparation remain
separate reference boundaries before faithful conventional-unit rendering.
