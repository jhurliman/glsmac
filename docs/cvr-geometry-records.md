# CVR geometry records

`loader::cvr::CvrGeometryReader::Read(const std::vector<uint8_t>&)` decodes the
observed simple/grouped records in `CvrSceneObject::voxel_data`. It returns
owned header fields, ordered groups and exact three-byte records. It does not
reconstruct positions, interpret normals/colors, apply transforms, generate
meshes, choose unit assets or activate rendering. The
[rendering plan](https://github.com/jhurliman/glsmac/blob/e022b688e4206ee9848050d816329dd125ade4ca/docs/conventional-unit-rendering-plan.md) describes those separate
dependencies.

## Supported profile

All integer fields below are little-endian. Offsets are relative to the opaque
geometry payload, excluding its enclosing nested chunk header.

| Offset | Width | Field / accepted profile |
| --- | ---: | --- |
| 0 | 4 | Flags: exactly 0, 2, 8 or 10 |
| 4 | 4 | Exact payload byte size, uint32 |
| 8 | 4 | Finite positive IEEE binary32 scalar, retained as `scalar_bits` |
| 12 | 4 | End field, equal to payload size |
| 16 | 4 | End-minus-one field, equal to payload size minus one |
| 20 | 18 | Three vectors of three signed 16-bit components, in file order |
| 38 | 4 | Positive record count, with the conventions below |
| 42 | variable | One simple record sequence, or ordered groups |
| final 5 bytes | 5 | Exactly five zero trailer bytes |

The scalar is classified directly from its sign/exponent/fraction bits and
stored as the original uint32. This preserves positive subnormals and every
accepted mantissa without a floating-point conversion, rounding or flush to
zero. Zero, negative values, infinities and NaNs are rejected. Header and group
vectors retain the complete signed-16 range; no geometric bound is inferred.
The neutral field names do not establish scale, pivot or coordinate semantics.

Flags 0/8 have a single group with no group header. Its record count is the
geometry header count. Flags 2/10 have repeated groups, each beginning with:

| Group offset | Width | Field |
| --- | ---: | --- |
| 0 | 24 | Four vectors of three signed 16-bit components |
| 24 | 4 | Positive count of following three-byte records |
| 28 | 4 | Group flags, currently exactly zero |
| 32 | count × 3 | Records |

Each record preserves all three bytes. `Code()` exposes only the first byte's
upper five bits. Codes 0 through 25 are accepted as uninterpreted record codes.
When geometry bit 8 is set, each group must end in exactly one code-26 record;
its other bits and bytes are preserved too. Code 26 anywhere else, or any
code greater than 26, is rejected. No direction table, terminal vertex,
normal angle or palette index is inferred.

Simple geometry header counts **include** the terminal record. Grouped
geometry header counts equal the sum of group counts **excluding** one
terminal per group when bit 8 is set. Each group count itself includes the
terminal. The sum and final extent must match exactly. A terminal-only simple
group is structurally allowed; a grouped terminal-only group contributes zero
to the aggregate, whose overall header count must still be positive. These
rules impose no claim about a useful drawable object.

Flags 1/3, other geometry bits, nonzero group flags, nonpositive/nonfinite
scalars, zero counts, extra bytes and unknown trailers are deliberately
unsupported. Rejection is an explicit GLSMAC ingestion policy for this
observed profile, not a claim that other CVR producers or the original game
must reject them. There is no partial/salvage rendering mode.

## Bounds and lifetime

The fixed header and trailer are checked before field access. Payload-size
equality bounds the input by uint32. Group headers use remaining-length
checks; a record count must fit remaining bytes divided by three before any
offset multiplication or vector resize. These bounds also limit aggregate
count arithmetic. Signed components are decoded through a representable
int32 value before narrowing, with no unaligned loads or type-punning.

The complete payload, including late groups, is validated in a first pass
without allocating result storage. A second pass builds the owned value.
Malformed/unsupported input throws `std::invalid_argument` prefixed with
`CVR geometry:`. Allocation exceptions propagate normally; standard containers
release any storage already created. The caller must keep input unchanged
throughout the call. This is not a global memory quota.

Results can outlive the source scene and be copied or moved independently.
The simple group's header is `std::nullopt`; grouped headers retain all four
vectors, count and flag. No record is deduplicated or removed, and terminal
bytes remain available for future evidence-based interpretation. The decoder
owns no scene actor, texture, GC object or file handle.

## Evidence and validation

The metadata survey in the rendering plan and a separately compiled Release
probe calling the production envelope, scene and geometry readers agree:

- All **140** installed files pass envelope and scene validation, with **536**
  objects, **440** geometry payloads and **7,672** object-frame records.
- **438** geometry payloads pass: 340 simple and 98 grouped. Their decoded
  results contain **897** groups and **3,620,629** three-byte records,
  including terminal records.
- Two payloads explicitly report `unsupported geometry flags`: `VHTTp` in
  `VHTTp.cvr` (flags 3) and `VLights` in `VLIGHTS.cvr` (flags 1).
- The installed inventory is the same 140-file manifest documented by the
  [container audit](cvr-container.md), SHA-256
  `96f95e27c1677834d7b8e4c4e2c17d48095106dcbcc09827ce9849d137653f50`.

These are file/record compatibility measurements, not original-renderer
comparisons. No original payload is committed or needed by tests. No unit-to-
asset mapping, normal/color interpretation or rendering equivalence is claimed.

Seven synthetic native groups cover all four profiles, signed endpoints,
exact scalar bits including subnormals, all accepted codes, terminal rules,
repeated records, every truncated prefix with original/repaired size fields,
invalid counts/extents, unsupported flags, late bad groups and independent
source/copy/move lifetimes. They run in the existing `gse` CTest. The focused
command requires the native test filter from PR57 or its integration; the
ordinary `--gse-tests` command includes these cases on the focused publication:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^CVR geometry' \
  --memorydebug --prefix /tmp/glsmac-cvr-geometry-tests
```

The opt-in probe uses the build command in [cvr-container.md](cvr-container.md):

```sh
/tmp/cvr-container-probe --geometry '/path/to/installed/Alpha Centauri'
```

`--geometry` implies scene validation; default envelope and `--scene` modes
retain their behavior. The probe prints separate file and geometry totals.
Any rejection returns nonzero: the inspected complete installed set returns
**1**, with the two unsupported geometry variants above, rather than hiding
them as a successful full-geometry result.

Local Linux/WSL results with Clang 15:

- Full normal CTest: **8/8**, 38.45 seconds.
- All seven focused groups pass with the application memory tracker and under
  ASan + UBSan + LSan (`detect_leaks=1:halt_on_error=1`), with no reported leaks.
- A copied-source negative control removes only the aggregate-count equality
  check. Its actual native run exits 1, **5 passed / 2 failed**, on a false
  positive header count for terminal-only groups and malformed aggregate counts.
  The intended production source and normal binary remain unchanged.
- Explicit one-TU analysis is clean in Debug and Release: zero failures in
  1.28 and 1.17 seconds. The standalone `-O2 -DNDEBUG -Wall -Wextra -pedantic`
  production-reader probe compiles clean. Its default envelope and `--scene`
  modes still accept all 140 files with exit 0.

Local evidence is retained under `/home/jhurliman/src/glsmac-unit-weapon-parser/`
as `geometry-*.log`; copied-source controls and metadata are also retained in
the audit checkout's ignored `out/validation-cvr-geometry/` and
`out/validation-cvr-render-plan/`. Native Windows/macOS execution is left to
integration validation.
