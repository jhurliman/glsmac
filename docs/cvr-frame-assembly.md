# Owned CVR frame assembly

The production assembler converts explicit scene/frame/offset selections into
owned frame records. It follows the bounded [original SMAC assembly
experiment](cvr-scout-composition-evidence.md): source order, per-scene frame
modulo, unchanged flag and matrix bits, and component-wise location addition.
This is a frame-data operation, with no renderer or live-unit activation.

## Ownership and input contract

Create a `CvrFrameSource` once from a vector of decoded `CvrScene` values.
It owns the scenes, animation frames and optional opaque geometry bytes. Each
assembly retains a shared pointer to that immutable source, so animation frames
do not repeatedly copy geometry. Copying/moving an assembly preserves the same
source identity and the meaning of its scene/object indices, even after the
caller's source pointer is released. Object names may be empty or duplicated;
they are never identity keys.

Source creation takes the vector by value: an lvalue is copied and an rvalue
is moved. A caller moving a vector must relinquish any retained mutable pointers
to its elements. The source exposes only const access, but this does not police
pre-existing aliases or hostile casts by a caller.

`CvrFrameSelection` explicitly names a source scene index, a nonnegative signed
32-bit frame number and three binary32 offset components. There is no implicit
zero/Scout offset, selector `% 15`, axis conversion, reactor or faction choice.
Each selected scene independently reduces that supplied frame number by its
own positive frame count. For example, frame 17 selects frame 0 from a 17-frame
body and frame 17 from a 32-frame armor component.

Each `CvrAssembledFrame` owns its copied flag, translated location and unchanged
nine matrix values, alongside selection/scene/object/frame indices. Geometry is
the optional `voxel_data` at those indices in the retained source. The assembler
does not decode geometry, filter flag bits, resolve attachment names, transpose
or multiply matrices, or reproduce the original's opaque runtime descriptor and
lookup-table pointers. Output structs are values; callers that manually change
their indices or replace their source are responsible for that new state.

## Validation and capacity

The source factory validates all supplied scene metadata and every frame before
publishing the source: scene names and object/frame counts are positive, frame
counts fit the supported signed domain, object frame lengths agree, present
geometry is nonempty, and all location/matrix components are finite binary32.
The empty source collection is valid for an empty assembly request. These are
GLSMAC ingestion rules, not claims about original malformed-input behavior.

Assembly validates **every selection and evaluated location**, including entries
that would exceed capacity, before allocating result records. Invalid scene
indices, negative frames, null sources and unsupported arithmetic throw
`std::invalid_argument` prefixed with `CVR assembly:`. Allocation exceptions
propagate through standard owned containers; no partial result escapes them.
Inputs and the calling thread's floating-point environment must remain unchanged
during the call.

The valid output retains at most the first **200** records. The optional
`first_omitted` position identifies the first selection/object not included.
An exact 200-record request is complete; 199 records followed by a two-object
scene retains its first object and reports the second as omitted. This preserves
the observed original prefix behavior without treating the original's internal
capacity as a file-format validity limit. Consumers must check this status before
treating an assembly as complete. GLSMAC deliberately validates even the omitted
suffix; the original probe establishes no equivalent malformed-input guarantee.

## Supported arithmetic

The API requires IEEE binary32 storage, at least binary64 intermediate precision,
and the calling thread's standard rounding mode to be `FE_TONEAREST`. On SSE
targets, its independently configurable MXCSR rounding mode must also be nearest.
Checking only `fegetround()` is insufficient on platforms where it reports x87
state while the evaluated additions/conversions use SSE. Neither control is
changed by assembly. Location and offset
operands must be finite normal values or signed zero. The computed binary64
intermediate must be zero or within the finite normal binary32 range **before**
conversion. This checks the rounded intermediate, not an infinitely precise
mathematical sum: adding 1 to maximum finite binary32 is absorbed in binary64
and returns maximum finite binary32. Subnormal operands
or results, overflow, NaNs, infinities and other rounding modes are explicitly
unsupported. These conservative boundaries avoid guessing at the original
process's exceptional floating-point behavior.

SSE flush-to-zero (FTZ) and denormals-are-zero (DAZ) may be enabled: all admitted
binary32 operands are normal or zero, and their smallest possible nonzero
difference, 2^-149, is normal in binary64. Unsupported subnormal operands are
rejected by their stored bits; subnormal binary32 results are rejected using the
normal binary64 intermediate before conversion. Thus neither mode changes
accepted arithmetic or conceals these domain errors. Matrix bits are copied
without arithmetic, including subnormal values.

Each addition uses binary64 intermediates and one binary32 conversion. For two
binary32 operands, this produces nearest-even binary32 rounding within the
supported domain. No multiply/add contraction occurs. Tests pin even/odd halfway
rounding and negative-zero addition. Matrix values undergo no arithmetic, and
their finite subnormal, signed-zero and endpoint bits remain unchanged.

The original experiment supplies x87 control word `0x037f`; it does not measure
a live game's control word. Broader exceptional arithmetic compatibility is not
claimed by this implementation or the installed-file comparisons.

## Production caller and original comparison

The opt-in `cvr_frame_probe` runs the real container reader, scene reader and
assembler. It accepts one or more explicit groups of file/frame/offset arguments,
emits JSON with stable indices and exact transform bit patterns, and returns
0 for complete output, 3 for a capacity-limited prefix, 1 for rejected input/I/O,
or 2 for command shape errors. Offset words are eight hexadecimal digits without
`0x`, in file component order. No assets are bundled.

```sh
clang++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra -pedantic -Isrc \
  src/loader/cvr/CvrContainerReader.cpp src/loader/cvr/CvrSceneReader.cpp \
  src/loader/cvr/CvrFrameAssembler.cpp tools/tests/cvr_frame_probe.cpp \
  -o /tmp/cvr-frame-probe
/tmp/cvr-frame-probe '/installed/VI.cvr' 17 c3c80000 43480000 00000000 \
  '/installed/Vipta00.cvr' 17 c3c80000 43480000 00000000
```

The comparison tool consumes the retained report from the original executable
probe. It verifies installed-file hashes, supplies each observed assembly call's
explicit frame and offset to the production caller, and compares count/order,
selected frames, flags and all location/matrix bits:

```sh
python3 tools/reference/compare_cvr_assembly.py \
  --reference-report out/reference/scout-composition-report.json \
  --probe /tmp/cvr-frame-probe --assets '/installed/Alpha Centauri' \
  --output out/reference/cvr-assembly-comparison.json
```

The Release production reader/assembler matches **30/30 original-executed
installed-file cases and all 1,755 records**. These include 18 direct cases and
12 reached through the original selector wrapper. This compares supplied loaded
scene ABI assembly; the original loader, vertex transforms and rasterizer remain
outside the experiment. Original synthetic append/capacity evidence remains in
the separate probe, while native tests exercise the new API's equivalent prefix
policy and stricter validation.

The original report has 36 positive cases in total. The differential comparison
excludes its four synthetic pre-existing-prefix cases and two synthetic capacity
cases: they operate on a caller-supplied original output buffer, whereas this API
assembles a fresh sequence of selections. Native tests cover the corresponding
ordering, arithmetic and capacity policies; they do not constitute 36/36
original differential matches. The original probe's six guard/oracle negative
controls are also separate from the three implementation mutations below.

Six native regression groups cover multi-scene modulo, distinct per-component
offsets, nonidentity matrices, raw flag bits, immutable shared lifetime,
duplicate/empty names, complete/prefix capacity, malformed public scenes,
late invalid selections and arithmetic boundaries. They run in the existing
full native suite (`build/bin/GLSMAC --gse-tests`).

Initial implementation checks on Linux/WSL:

- Full normal CTest: **4/4**, 17.58 seconds, including all six new native groups.
- The six groups pass with `--memorydebug` and under ASan + UBSan + LSan
  (`detect_leaks=1:halt_on_error=1`), with no reported leaks. This main-based
  checkout predates the native-filter CLI. The focused runs select the same
  tests using a copied registration translation unit, linked to the unchanged
  application objects; no production source is replaced in the checkout.
- Three separate copied-production controls omit offsets, transpose matrices,
  or use a fixed divisor instead of each scene's frame count. They fail with
  **3, 2 and 1** intended assertions respectively, all returning exit 1. The
  normal source/binary remain unchanged, and the controls clean up their owners.
- Curated Clang 15 analysis passes for the new production translation unit and
  header in both Debug and Release, zero findings (2.19 / 2.32 seconds).
- The standalone Release probe compiles with `-Wall -Wextra -pedantic` and
  matches the 30 original cases above. Explicit CLI controls check capacity
  status/first-omitted position and rejection of negative frames, subnormal
  offsets and malformed command shape.

Logs remain under `/home/jhurliman/src/glsmac-unit-weapon-parser/assembly-*`,
with focused and copied-source logs in `build/assembly-validation/` and
`build-asan/assembly-validation/`. The validating checkout's ignored
`out/validation-cvr-composition/assembly-comparison.json` pins the reference
report and Release probe hashes. Original frame payloads remain local.

## Independent SSE rounding regression

The follow-up adds a seventh native group and a shared, asset-free standalone
caller. It changes MXCSR directly while leaving x87 nearest, then checks all
three non-nearest SSE modes are rejected. It also checks nearest-even rounding,
all four FTZ/DAZ combinations, unsupported denormal operands/results and unchanged
opaque matrix bits. A scoped environment saves/restores the standard environment
and exact MXCSR on normal, assertion and exception exits.

The same checks can run against an optimized production TU without the Debug-only
GSE harness:

```sh
clang++ -std=c++17 -O3 -DNDEBUG -Wall -Wextra -pedantic -Isrc \
  src/loader/cvr/CvrFrameAssembler.cpp tools/tests/cvr_frame_arithmetic.cpp \
  -o /tmp/cvr-frame-arithmetic
/tmp/cvr-frame-arithmetic
```

Follow-up validation:

- Copied original production code fails the real mixed-control check in both
  Debug (`-O0 -g`) and Release (`-O3 -DNDEBUG`), exit 1. With x87 nearest and
  MXCSR downward, `0x3f800001 + 0x33800000` incorrectly returns `0x3f800001`
  instead of nearest-even `0x3f800002`. The intended checkout was not reverted.
- Fixed Clang 15 and GCC standalone Debug, Release and ASan/UBSan builds pass.
  Native Windows x64 Clang Debug and Release standalone builds also pass.
  The architecture guard uses standard SSE intrinsics and omits the independent
  control check on non-SSE targets; no new native macOS execution is claimed.
- Full Linux/WSL normal CTest passes **4/4**, 17.41 seconds, including all seven
  assembly groups. The standalone checks take under 0.01 seconds each.
- Debug and Release production-TU correctness analysis passes with zero findings
  (2.07 / 2.06 seconds).
- The newly compiled Release production reader/assembler again matches the
  retained original report's **30 cases / 1,755 records**. This reruns the C++
  comparison, not the original executable experiment.

Follow-up commands, binaries, logs and comparison hashes remain under
`/home/jhurliman/src/glsmac-cvr-effective-rounding/`; native Windows standalone
logs are in the validating checkout's ignored `out/validation-cvr-rounding/`.

This work does not establish axis orientation, matrix application,
geometry-to-vertex mapping, colors, projection, animation timing, default reactor
selection, SMAX behavior or a rendered conventional unit.
