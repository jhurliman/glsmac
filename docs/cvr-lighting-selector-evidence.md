# CVR lighting lookup selection: original SMAC evidence

This experiment executes the final table-selection part of the original lighting
routine with **two explicitly supplied angular values**. A separate composition
then passes its captured output pointer to the original single-record indexed
kernel. That kernel reads the installed first lookup table and a synthetic second
table. The result establishes selection and consumption of an original lookup
window; it does not establish camera setup, vector-to-angle conversion, RGB
palette construction or a complete rendered Scout.

The earlier [caller/origin experiment](cvr-caller-origin-evidence.md) stopped
before lighting, and the [indexed-pixel experiment](cvr-indexed-pixel-evidence.md)
supplied both lookup tables. This evidence narrows that gap without silently
substituting a camera, inverse, trigonometric library or palette.

## Reference and provenance

All addresses refer to the installed GOG PE32 `terran.exe`, 3,190,840 bytes,
SHA-256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The probe verifies that fingerprint before decoding original disk bytes with
Capstone 5.0.7 and executing with Unicorn 2.1.4. It reads the prior repository
evidence and uses the existing pinned-image reader and pixel instruction decoder.
The local research index supplied candidate routine boundaries; no human
research file, executable, installed asset or interactive analysis session was
modified or operated. No original table or execution report is committed.

Static inspection places the suffix within `0x65ab50..0x65aca6`. Earlier code
calls inverse/matrix helpers, applies supplied scaling, reflects a negative Z,
and obtains two angles through an acos wrapper and `FPATAN`, including adjustment
of a negative azimuth. **None of that earlier code executes here.** Those static
connections do not supply an implemented or verified camera/light model. The
experiment begins at `0x65ac62` with ST0 holding the azimuth input and ST1 the
inclination input. These names describe the upstream dataflow, not inferred
world axes or default values.

## Executed suffix and arithmetic domain

Allowed original code is exactly `0x65ac62..0x65aca6` and the signed64 conversion
helper `0x660678..0x66069f` (half-open ranges). No function is stubbed. The two
nonnegative binary32 inputs are widened exactly into raw 80-bit x87 registers by
the fixture. No synthetic setup instructions execute. The input control word is
`0x037f`: extended precision, nearest-even arithmetic and masked exceptions.

The suffix uses these exact original constant bits:

| Address | Binary32 bits | Value |
| --- | --- | --- |
| `0x695064` | `0x3f000000` | `0.5` |
| `0x695070` | `0xc16a46ad` | `-14.64225482940673828125` |
| `0x695074` | `0xc162a2fa` | `-14.1647891998291015625` |

Let `Kr` and `Ka` be the positive magnitudes of the last two constants, and `r`
and `a` the explicit inclination/azimuth inputs. The observed integer results
are `trunc(0.5 + r*Kr)` and `trunc(0.5 + a*Ka)`. The helper temporarily sets
rounding toward zero for `FISTP`, writes a signed64 result and restores its saved
control word. The probe checks both actual qword writes and their order. It does
not replace either constant with a mathematical expression involving pi.

The admitted domain requires positive-normal binary32 values or positive zero,
exact products and half-shift sums representable with at most 64 significant
binary digits, and resulting indices in row `0..23` and pre-wrap azimuth `0..89`.
The oracle uses exact rationals for these checks, so admitted operations do not
depend on host floating-point rounding. Negative values including negative zero,
subnormals, nonfinite values, inexact extended intermediates, unknown control
words and out-of-range results are rejected **before execution by the harness**.
These are conservative experiment limits, not validation attributed to the
original routine. Neighbor fixtures are generated near exact-rational bin
thresholds, then their exact outcomes and straddling are checked; no claim that
each supplied binary32 value lands exactly on a half-integer tie is needed.

The helper may set x87 precision/status bits. The checks require the original
control word restored, two inputs popped, all tags empty, TOP advanced from zero
to two, and no invalid/denormal/divide/overflow/underflow status. They do not claim
that all status flags are restored.

## Original table identity and selected window

The 24 original pointers at `0x6c2e88..0x6c2ee8` each address a 2,144-byte row:
`row_pointer(i) = 0x6b6588 + i*2144`. The contiguous 51,456-byte installed region
`0x6b6588..0x6c2e88` has SHA-256
`81102740e2d482516c9d1b42fec5972c2ba8364cb597fd7492e82d1e287eec60`.
The probe verifies the entire region and pointer array before each experiment
set; all original bytes in that region are in `0..23`.

The executed suffix changes azimuth result `89` to `0`, reads the selected row
pointer, and stores:

```text
selected_pointer = row_pointer(row) + 89 - wrapped_azimuth
```

The stored pointer is read back from guest address `0x9c6b94` after the independent
oracle assertion. That captured value, rather than a recomputed oracle pointer,
is the input to the separate kernel composition. The window offset is `1..89`;
an entire 2,048-byte lookup window fits within its source row. The window length
comes from the kernel's established 11-bit record lookup, not a length read or
computed by the selector itself. The selector reads only one pointer-array entry
and the rounding constants; it does not read the table window or construct it.

This is evidence about installed table bytes. It does not explain their
generation, identify a portable replacement formula, or justify distributing
them as new GLSMAC source data.

## Captured-pointer composition

The separate composition executes the existing original kernel initializer
`0x686d51..0x686e43`, followed by the previously bounded non-block single-pixel
path rooted at `0x6867f0`. Original selector output is installed in a fresh guest
at `0x9c6b94`; the original first table remains at its original mapped address.
The fixture supplies one raw three-byte record, one zero direction step,
initial position `(256,256,256)`, mode `1`, a 3-by-3 surface with stride 4,
row offsets, initial unsigned depth 65,535, and a synthetic 24-by-256 second
table. The step builder, dispatcher, caller, clipper and camera do not execute.

The synthetic second-table byte is `(shade*17 + material*7 + 3) mod 256`.
The actual draw must read exactly the selected first-table byte followed by the
corresponding second-table byte, and write depth `1` then the resulting indexed
color at surface coordinate `(1,1)`. Whole read-back color/depth buffers, row
padding, canaries, record reads and ordered output writes are checked. Thus the
result is a real original indexed lookup/write with synthetic surface and second
table inputs; it is not an RGB result or a complete original rendering pipeline.

## Bounds, controls and receipts

Every executed address must be a decoded original instruction start inside its
active phase. Selector reads are limited to declared stack storage, three
constant words and the pointer array; writes are limited to helper/call stack
scratch and the four-byte output. Stack/callee-saved registers, output canaries,
code and table immutability are checked. The suffix allows at most 150 original
instructions and two seconds per case.

Composition has separate initializer and draw permissions. The initializer may
write only the 38 preverified original operand fields from the earlier pixel
experiment, exactly once each; every nonoperand kernel byte must remain original.
The draw cannot write code or either lookup table. Instruction and read/write
bounds, return sentinels, cdecl cleanup and callee-saved registers are checked,
with at most 500 total instructions and two seconds per invocation.

The final retained run passes **2,503 selector cases, 19 domain rejections and
nine selector controls**, plus **100 captured-pointer compositions and five
composition controls**. It executes 122,674 original selector/helper instructions
and 10,500 original initializer/draw instructions in 43.21 seconds on the local
Windows Python environment.

Selector coverage consists of all 24-by-90 pre-wrap row/bin pairs, 336 neighboring
binary32 threshold cases, two small inputs, two exact64 small-value boundaries,
and three admitted upper-domain neighbors. The matching rejected neighbors
include the first out-of-range row/bin, tiny-normal exactness failures, and the
normal values immediately below the admitted small powers. Composition covers
every row at record indices `0`, `255`, `256` and `2047`, plus explicit endpoint
rows with azimuth results `88` and `89`. These cases consume 22 distinct original
first-table byte values; exhaustive table-value or complete record-stream
coverage is not claimed.

Selector controls deny the helper instruction, row-pointer read, constant read
and output write; reduce the budget; and test independently wrong omission of
the half shift, omission of wrapping, offset `88` and row selection. Composition
controls deny an initializer operand write, first/second lookup reads and the
pixel write, and compare against an intentionally wrong unshifted-row oracle.
Each control must fail at its intended boundary. Domain failures happen before
emulation, while controls exercise the monitored original instructions.

| Retained evidence | SHA-256 |
| --- | --- |
| Probe source | `a1630f15fd1dc9d68c7b42eddac52c1d6a2c462de9fb80140289e6ccaf19954f` |
| Image-reader helper | `5c97f8ebda930909d78896c48b764f5aea8ae5322cbdcf2a8dd2b9df1b59f201` |
| Existing pixel helper | `dee511e774ffa42cce36c8e63c4ff4820b42b3b2cc2831dc5bbf80d8de55c167` |
| Final report, 29,566,039 bytes | `8c09882c2e4296a363d6824f6081fa9c8a474dc747313d93e6497f4fd3167179` |

The private report is retained at
`out/validation-cvr-lighting-selector/lighting-selector-report.json`, with the
execution result in `final-run.log`. The earlier 2,498-case selector-only receipt
is separately preserved as `lighting-selector-core-report.json`; it is not the
final source/composition receipt. Python byte-compilation and whitespace checks
also pass. No C++ production code changed, so this evidence slice did not repeat
unaffected application builds or platform suites.

```sh
python tools/reference/probe_cvr_lighting_selector.py \
  --exe /private/Alpha\ Centauri/terran.exe \
  --report /private/lighting-selector-report.json
```

The report includes exact input words and x87 seeds, executed instruction counts,
ordered memory traces, actual qword/pointer results, original window hashes, and
the explicit synthetic composition inputs. It belongs outside Git.

## Next implementation boundary

The supported next calculation could accept explicit angular binary32 words and
return an owned row/window selection using the checked exact arithmetic above.
It would need explicitly supplied, owned lookup data with its own provenance;
this evidence does not authorize silently embedding the executable's table.
Connecting the existing indexed rasterizer to such a supplied window would
preserve the captured two-stage lookup contract.

Remaining reference work includes the upstream inverse/vector/trigonometric
path, camera/light initialization, original second-table and RGB palette
construction, clipping, complete component traversal and a rendered-image
comparison. No default reactor, SMAX equivalence, coordinate convention, live
unit activation or Scout visual-fidelity claim follows from this slice.
