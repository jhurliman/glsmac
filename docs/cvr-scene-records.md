# CVR scene records

`loader::cvr::CvrSceneReader::Read(const CvrContainer&)` decodes the observed
nested records in container chunk 4 into owned scene, object and frame values.
It is independent of file I/O and the game renderer. It preserves opaque voxel
payloads, object order, duplicate names and unapplied transform components.
It does not activate conventional units or interpret geometry, materials,
attachment relationships, coordinate axes, animation timing or visibility.

## Supported profile

The outer container must match the [four-chunk envelope](cvr-container.md).
Because `CvrContainer` is public and directly constructible, this reader checks
all four ordered tags, nonempty outer payloads, the four-byte tag-1 payload,
and a total file-size budget that fits the uint32 envelope. It does not
interpret the contents of outer chunks 1, 2 or 3.

Nested headers use the same mixed byte order as the envelope: a big-endian
uint32 tag followed by a little-endian uint32 size **including the eight-byte
header**. Fields below must appear exactly in order, with no extra fields or
trailing bytes. Tags are written as their big-endian numeric values.

| Parent | Tag | Decoded field |
| --- | --- | --- |
| Scene | `00000104` | Nonempty scene-name bytes |
| Scene | `00000204` | Positive little-endian uint32 object count, exactly four bytes |
| Scene | `00000304` | Positive little-endian uint32 frame count, exactly four bytes |
| Scene | `00000404`, repeated object count times | Object record |
| Object | `00010404` | Object-name bytes, possibly empty |
| Object | `00020404`, optional | Opaque geometry container |
| Geometry | `01020404` | One nonempty opaque voxel payload |
| Object | `00030404` | Animation container |
| Animation | `01030404` | One raw flag byte per frame |
| Animation | `02030404` | Three little-endian binary32 location components per frame |
| Animation | `03030404` | Nine little-endian binary32 matrix components per frame |

Names are length-delimited byte strings, not identity keys or filenames. The
API preserves embedded zero bytes and accepts duplicate object names,
including duplicate empty names. An object-name header may therefore have
size eight. Other nested payloads must be nonempty. Scene and object lists
remain in source order; no name-based map or parent/child relationship is
inferred. Geometry may be absent, including in every object in a scene.

Every object's three animation arrays must match the scene frame count
exactly. All transform components must be finite; signed zero, subnormal and
finite endpoint bits are retained. Flags retain all eight bits; observed
values do not limit the public API. Matrix components remain in file order,
with no transpose, axis conversion, scaling or matrix application. The
`location` and `matrix` field names describe the record layout rather than a
verified original-renderer transformation convention.

These are explicit restrictions of the currently supported profile. Empty
scene names, zero counts, nonfinite transforms, unknown or reordered fields
and other variants require separate evidence before adding support. Rejection
does not establish that a file is invalid for every possible CVR producer.

## Bounds and ownership

Each nested header is checked against remaining bytes before reading it. Its
length must fit those bytes before creating a view or advancing the cursor.
Counts never determine an unchecked allocation: the object count is bounded
by remaining chunk-header space, and each frame-array length is divided by
its fixed stride (1, 12 or 36), then compared with the declared count. This
avoids multiplying an untrusted count. The validated array extents bound all
later component offsets.

A first pass validates the entire scene, including late records and every
transform component, without allocating result storage. A second pass creates
owned strings, optional byte vectors and frame vectors. Integers are assembled
from widened bytes; float bits are transferred with `memcpy`, with no unaligned
load or type-punning dereference. Supported builds require IEEE binary32
`float` at compile time.

Unsupported data throws `std::invalid_argument` with a `CVR scene:` prefix.
No partially decoded scene escapes. Allocation exceptions propagate normally,
and standard containers clean up any result storage already allocated. This
is not a global allocation quota: a large valid input still requires memory
for its decoded result. The caller must keep the container unchanged during
the call. Returned scenes can outlive the input and be copied or moved without
aliasing its payloads or each other.

## Evidence and limits, 2026-10-16

The production decoder was run read-only over all 140 case-insensitive `.cvr`
files in the installed GOG Alpha Centauri directory:

- **140 accepted, 0 rejected**, totaling **12,807,702 bytes**.
- **536 objects**, of which **440** have opaque geometry and **96** are
  animation-only; **7,672 object-frame records**.
- Every declared object count matches the nested records. Every frame array
  has the required length, and all stored transform components are finite.
- Observed per-scene frame counts are 1, 2, 12, 15, 16, 17, 32 and 36.
  Object counts span 1 through 58. These observations are not hardcoded limits.
- `VCUW.cvr` has ten objects and nine distinct names; `W3L` occurs twice.
  `ACP03.cvr` has one empty object name. The initial decoder rejected the latter
  (139 accepted / 1 rejected), leading to the explicit empty-name support and
  synthetic regression. That initial probe log is retained.

The inventory manifest SHA-256 is
`96f95e27c1677834d7b8e4c4e2c17d48095106dcbcc09827ce9849d137653f50`;
its canonical format and selected file hashes are recorded in
[cvr-container.md](cvr-container.md). No original payloads are committed or
required by the automated tests.

The record investigation also consulted the reverse-engineering author's
[CVR-Colorizer source at commit 1174833e](https://github.com/uaEquals42/CVR-Colorizer/blob/1174833e288ad39f5cf03654344a23abb8b6fc3b/src/CVR.py)
and the contributors' [Caviar format notes](https://alphacentauri2.info/wiki/Caviar.html).
The supported profile above was checked against the local files; these
sources are not a specification from the original game's publisher.
No original-renderer comparison was performed. Envelope and record decoding
do not prove voxel topology, normals, palette/shading interpretation,
attachment/model composition, unit-to-asset mapping or rendered equivalence.
Those remain separate decoder and rendering work.

## Reproduction and validation

Eight synthetic native groups are registered in the existing `gse` CTest.
They cover directly constructed containers, byte order, distinguishable
objects and frames, duplicate/empty names, independent lifetimes and
copy/move, optional geometry, every truncated prefix, malformed lengths and
counts, nested schema/order, nonfinite values and finite endpoint bits.
No extra full-suite invocation is added to default CTest. A Debug build can
run the full suite with `build/bin/GLSMAC --gse-tests`. The focused command
below requires the native-test filter from
[PR #57](https://github.com/jhurliman/glsmac/pull/57); omit that option and its
argument on branches without the separate filter feature.

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^CVR scene' \
  --prefix /tmp/glsmac-cvr-scene-tests
```

The optional installed-file probe adds `--scene` to the existing envelope
probe. It reports only filenames, sizes and counts. Build it using the command
in [cvr-container.md](cvr-container.md), then run:

```sh
/tmp/cvr-container-probe --scene '/path/to/installed/Alpha Centauri'
```

Local Linux/WSL validation with Clang 15:

- Full normal CTest: **8/8**, 39.63 seconds.
- All eight focused scene groups pass ASan + UBSan + LSan with
  `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The existing local non-PIE
  sanitizer workaround is retained; no build-policy change is included.
- Explicit one-TU static analysis in Debug and Release: zero failures,
  0.55 and 0.45 seconds respectively.
- The standalone `-O2 -DNDEBUG -Wall -Wextra -pedantic` probe compiled clean
  and accepted the installed set using the production container and scene
  readers. Its default envelope-only mode also accepted all 140 files.
  Missing/invalid probe arguments, empty inventory and a synthetic valid
  envelope with an invalid scene all return the expected nonzero status; the
  latter still passes the envelope-only mode.
- Separate copied-source negative controls disable finite checks, reuse the
  first frame's location for every frame, and reject empty object names.
  The actual native runs exit 1 with **7 passed / 1 failed**, **4 passed /
  4 failed**, and **7 passed / 1 failed**, respectively. Intended source and
  normal application objects stay unchanged during these controls.

Evidence is retained locally under `/home/jhurliman/src/glsmac-cvr-scene/`,
including `normal-ctest.log`, `asan-focused.log`, `debug-analysis.log`,
`release-analysis.log`, `installed-scene-probe.log`,
`installed-scene-probe-initial139.log`, and `*-negative.log`.
Native Windows/macOS execution and rendering fidelity are not claimed here.
