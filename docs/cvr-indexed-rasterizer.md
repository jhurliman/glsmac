# Explicit-input CVR indexed rasterizer

`loader::cvr::CvrIndexedRasterizer::Draw(initial, lookups, draws)` implements the
bounded sample primitive established by the
[original executable experiment](cvr-indexed-pixel-evidence.md). It returns owned
indexed color/depth buffers and record/write counts. **All 42 original cases
match byte for byte**, including complete infantry-mask and armor-control streams
read through the production CVR readers.

No live unit renderer uses this primitive yet. Projection basis, initial sample
position, lookup tables, footprint and record selection are explicit inputs.
There is no camera, matrix/scalar application, RGB palette, lighting-table
construction, clipping, default reactor or SMAC/SMAX equivalence assumption.

## API and lifetime

- `CvrIndexedSurface` owns width, height, pitch, indexed color bytes and uint16
  depth samples. Both vectors contain exactly `pitch * height` elements, including
  padding. The row offset is exactly `y * pitch`: **fixed stride is the tested
  subset** of the original caller-provided row-offset table.
- `CvrIndexedLookups` owns a 2,048-byte first table and complete 256-byte pages of
  a second table. These are supplied lookup values, not an RGB palette.
- `CvrIndexedDraw` is a small synchronous view of an existing vector of
  `CvrGeometryRecord`, plus explicit basis, initial accumulators, terminal mode
  and footprint. No geometry, scene or per-frame asset copy is made.
- The complete ordered batch is validated before the result's surface copy is
  allocated. Drawing then applies to that single copy, preserving component and
  record order. Inputs must remain unchanged for the synchronous call. No input
  pointer/reference escapes into `CvrIndexedResult`.

An empty batch returns an owned copy with zero counts after surface/table
validation. A draw with zero effective records is rejected. Invalid later draws
cannot return partial output or change the initial surface. Standard container
ownership also handles allocation failure; no renderer, GC or tracked engine
object is allocated by this API.

## Arithmetic and sampling contract

The first two basis rows must fit signed16, as in the executed step builder.
The third contains signed32 values. The 26 direction codes select lexicographic
triples in `{-1,0,1}³` with zero omitted. All 26 combinations are checked, even if
an individual draw uses only some codes.

Validation uses int64 arithmetic and rejects any basis-derived step or running
accumulator outside the original signed32 domain. Depth is checked **after its
eight-bit shift**, including the initial depth before any record is processed.
The implementation deliberately excludes original overflow/wrap behavior. All
int64 intermediate bounds follow from three int32 inputs and previously checked
int32 accumulators; no signed overflow or implementation-defined signed shift is
used.

Each record advances all three accumulators before sampling. The first two use
explicit floor division by 256. Depth uses floor division by 256 followed by
conversion to uint16, reproducing unsigned high-word extraction in the supported
domain, including negative values. The record's low three bits and second byte
form the 11-bit first-table index. That table's byte becomes the high byte of
the second-table index; the record's third byte supplies its low byte.

Only a depth strictly less than the current uint16 depth writes. Equal depth
retains the earlier record/component. A write updates depth then color. A single
footprint visits one sample; a square footprint visits top-left, top-right,
bottom-left, bottom-right, testing each depth separately. These are output-buffer
directions, not inferred world axes.

Terminal mode excludes the final triple. GLSMAC additionally requires that its
code is 26 and every processed code is below 26. This is a validation policy;
the executed original loop merely subtracts one from the supplied count and does
not validate the ignored terminal bytes.

The API rejects out-of-bounds samples rather than clipping them. It also rejects
invalid modes, missing records, malformed buffer/table extents, and used second
lookup indices outside the supplied table. No defaults repair invalid input.

Resource limits are explicit GLSMAC policy: at most 200 draws, 1,048,576 total
records including terminals, 16,777,216 surface samples, and 65,536 second-table
bytes. The surface limit permits at most 48 MiB of returned color/depth storage;
the caller owns its separate initial buffers. These limits are not claims about
the original engine's malformed-input handling or maximum supported asset size.

## Production caller and original comparison

`tools/tests/cvr_indexed_probe.cpp` is an opt-in standalone production caller.
It accepts a bounded explicit binary fixture and writes the resulting color/depth
bytes. Optional `CVR OBJECT_INDEX` arguments run the container, scene and geometry
readers, verify the fixture's simple flags-0/8 stream agrees with the installed
object, then draw the production-decoded records. This adds no runtime activation
or asset search/default selection.

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -Wall -Wextra -pedantic -Isrc \
  src/loader/cvr/CvrContainerReader.cpp src/loader/cvr/CvrSceneReader.cpp \
  src/loader/cvr/CvrGeometryReader.cpp src/loader/cvr/CvrIndexedRasterizer.cpp \
  tools/tests/cvr_indexed_probe.cpp -o /tmp/cvr-indexed-probe
python3 tools/reference/compare_cvr_pixels.py \
  --original-report /private/pixel-report.json --probe /tmp/cvr-indexed-probe \
  --assets '/installed/Alpha Centauri' --output-dir /private/pixel-comparison
```

The comparison script pins the retained original report, executable and probe
hashes from the evidence note. It reconstructs explicitly supplied inputs; it
does not execute the original or substitute a new pixel oracle. It compares full
color/depth byte hashes, shapes and record/write counts. All **42 cases,
21,231 records and 11,414 accepted writes** match. Four cases use actual installed
input through the production readers. The other cases are synthetic. The new
batch API's ordering/lifetime behavior is covered by native tests; the retained
original cases each invoke one draw kernel, not a live multi-component renderer.

The probe fixture is a test format, not a game/network/save protocol. All words
are little-endian. `CVRIDX1\0` is followed by uint32 width, height, pitch and
second-table length; 2,048 first-table bytes; second-table bytes; initial color
bytes and uint16 depth samples; uint32 draw count; then each draw's uint32 terminal
flag, footprint (1 or 2), nine signed32 basis entries, three signed32 initial
values, uint32 record count and raw triples. Trailing bytes are rejected. Input
files are limited to 64 MiB before allocation. Output `CVROUT1\0` is followed by
uint32 width/height/pitch, uint64 processed/written counts, color bytes and uint16
depth samples. Exit codes are 0 success, 1 input/output/raster error, 2 argument
shape. Output is opened only after successful drawing.

Keep all fixture/report/output bytes private; installed record bytes are included
in those local artifacts. No proprietary file or generated image is committed.

## Validation

Seven native groups cover cumulative sample/lookup order, padded rows, multiple
components, equal/near/far depth, square footprints, ignored terminal bytes,
fractional/negative depth, exact shifted-depth endpoints, ownership/copy/move,
empty batches, invalid late records/draws and allocation/count limits.

- Full normal CTest: 4/4, 17.14 seconds. This isolated publication base predates
  the native-filter CLI; it is not the later primary tree's eight-test suite.
- The same seven groups pass focused `--memorydebug` and ASan/UBSan/LSan with
  leak detection. Focused executables use an ignored copied registration TU
  linked to unchanged production/application objects. The sanitizer cache's
  existing local `-no-pie` workaround does not alter repository build policy.
- Five separate copied-production controls reverse a step component, admit equal
  depth, skip an extra terminal-mode record, force one-pixel footprints or ignore
  the first lookup. They fail 4/2/1/2/4 native assertions respectively, return
  exit 1, and leave the intended source/build unchanged.
- Explicit Debug/Release correctness analysis of the new production TU and its
  header passes (0.78 / 0.76 seconds). The standalone Release probe compiles clean
  with `-Wall -Wextra -pedantic`.
- Nine malformed Release-probe fixtures reject before opening output; an empty
  batch succeeds and an invalid command shape returns exit 2.

Logs, private requests/outputs and comparison results are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/` in `raster-*` directories/logs;
focused variants are under each build's `raster-validation/` directory.

GitHub review found that the comparison consumer still used the report's
lowercase filename directly, despite the original probe's corrected lookup.
The consumer now requires exactly one case-insensitive regular-file match
before checking the retained asset hash. It rejects missing and ambiguous
matches, including collisions where one filename is an exact match.

On October 17, a Linux case-sensitive directory containing mixed-case symlinks
to the two installed assets reproduced `FileNotFoundError` with the previous
consumer (exit 1). The corrected consumer matched all 42 retained cases,
21,231 records and 11,414 writes using a freshly compiled Release production
probe. Three asset-free filesystem tests also passed, including the collision
case. This compares the existing pinned report; it does not rerun original
executable code. Evidence is under
`/home/jhurliman/src/glsmac-pr95-case-lookup/`; the fixed comparison JSON SHA-256
is `feda6e49b9193fa5a0ad1712ffbd08c9740e2cf62ddd53207bcd97c05a99de77`.

The next reference boundary is original supplied-matrix-to-integer-basis
conversion, followed by the connection to actual header/frame/scalar/camera and
lighting/RGB-table preparation.
