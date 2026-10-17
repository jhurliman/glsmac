# Owned flat-idle Scout renderer

`CvrScoutRenderer` connects the existing CVR container, scene, geometry, frame,
basis and indexed-raster components into a complete owned image producer. Its
inputs are explicit VI and viptr00 asset bytes plus facing, zoom, integer scale
override and screen anchor. It generates view transforms and lighting, decodes
each scene's shades/RGB, processes simple and grouped geometry in composition
order, and returns indexed color, unsigned depth and keyed RGBA. It never reads
the original executable or accepts captured per-object transforms/lookups as
runtime defaults.

This is the reactor1 Infantry/Gun/Scout-armor design2 path: frame0, flat ground,
zero world position/default actor offsets, no shadows or palette overrides,
and a256-square internal surface. It does not select gameplay designs, replace
Mind Worms, spawn units, implement combat, or attach a cached texture to a live
actor. Target clipping, software blitting and dirty-area reuse remain separate.
Changing an input anchor changes returned placement metadata, not internal
pixels; an off-target request still produces its owned image.

## Ownership and admission

`Load` decodes two owned immutable scenes and separate geometry/palette values.
Temporary caller buffers may be destroyed immediately. Frame records refer to
stable scene/object indices. Each render has private surfaces; no mutable global
cache or borrowed input survives the call. Copying the renderer shares immutable
assets; returned images and textures do not share mutable pixels.

The explicit budgets are32MiB per input file,200 total objects,200 total groups
and1,048,576 records including terminals. These are port resource limits, not
original malformed-input behavior. The existing readers validate full framing
before owned copies. The adapter admits geometry flags8/10, idle frame flag0,
group flag0 and terminal streams. Every processed record must remain inside the surface;
unsupported flags, nonfinite/singular arithmetic, absent RGB entries, width
overflow and out-of-surface samples reject. Partial private work is discarded.
There is no missing-model fallback or inferred definition identity.

`CreateTexture` makes a fresh tracked256×256 `Texture`, copies the RGBA bytes,
and returns a `unique_ptr` with the matching tracked deleter. An eventual actor
cache must retain this owner for as long as any actor borrows the texture. This
factory does not establish that later lifetime or render a window by itself.

## Connected arithmetic and image semantics

The body and reactor each receive the original attachment offset(−400,200,0).
The real assets produce58 body records plus one reactor record. The three
grouped objects expand to nine groups;56 simple streams plus those groups give
65 ordered raster calls. Simple origins use the geometry's third header vector.
Grouped origins use first+fourth group vectors, transformed by the already
scaled object matrix and added to the transformed frame location.

The flat view uses facing−1 as0 and accepts ordinary facings0..7. Set
`k=(facing+5)%8`, store `phi=k*0x3f490e56` as binary32, then store its sine/cosine.
This constant is the original approximate facing step, not exact pi/4. The flat
angle is binary32 `0xbf85f92d`; its sine/cosine construct the full auxiliary
matrix `[1,0,0; 0,u,v; -0,v,-u]`.

The port admits zoom−8..8 and a positive override denominator. Positive
numerator selects `R32(h*n/d)`; otherwise the base is
`R32(h*(zoom+16)*0.0625)`, where h is binary32 `0x3d8f5c29`. The stored base is
multiplied by1.25 and stored again for this Scout. XY projection uses that
scale; depth uses0.5. Translation is(128.5,128.5,37767.5), from the fixed256
surface, half-pixel offset and original default depth mapping
`(10000+65535)*0.5`. Target anchor adds the signed integer truncation of
`50*n/d` or `50*(zoom+16)/16`, with explicit signed32 overflow rejection.
Unobserved invalid zoom/ratio policies are not inferred from the original.

`ScoutArithmetic` is a renderer-internal finite binary implementation, not a
general x87 emulator. Normalized uint64 significands and portable two-word
intermediates round each multiply/add/divide to64 significand bits, nearest-even.
Binary32 stores retain finite subnormals and signed zero. Matrix cells use
their original term order:

| Row | Column0 | Column1 | Column2 |
| --- | --- | --- | --- |
|0| `(t1+t2)+t0` | `(t0+t1)+t2` | `(t0+t2)+t1` |
|1| `(t0+t2)+t1` | `(t0+t2)+t1` | `(t2+t1)+t0` |
|2| `(t0+t1)+t2` | `(t2+t1)+t0` | `(t0+t1)+t2` |

The first sum rounds before the second; each matrix result stores before the
separate geometry-scalar multiply/store. Vectors use `(t1+t2)+t0`. This accepts
the actual inexact Scout matrix intermediates and preserves zero signs. The
renderer rejects a store that would produce infinity, although the original
masked helper can produce infinity. That narrower downstream admission policy
is tested separately from the110 finite synthetic formula matches. Exact x87
status flags, exception delivery and extended subnormals are not emulated.

Lighting computes the staged cofactor inverse of each scaled object matrix,
multiplies the full auxiliary matrix by the supplied fixed light, applies the
stored geometry/XY/depth scales, then applies the inverse. If Z is negative,
all XYZ components are negated, including zeros. Inclination/azimuth select a
row and contiguous2048-byte window in the generated24×2144 table. Signed zero
at the reflected pole affects `atan2`; replacing the full auxiliary matrix with
a vector or normalizing this direction changes the result.

The first table is independently generated from spherical dot products. For
row i, pole entry is23−i; rings j1..23 and angles k0..88 use
`a=i*pi/46`, `b=j*pi/46`, `t=k*2*pi/89`, and
`floor(abs(23-acos(clamp(cos(a)*cos(b)+sin(a)*sin(b)*cos(t),-1,1))*46/pi)+0.5)`.
The last89 entries repeat, followed by seven zeros. Host sine/cosine/acos/atan2
are used under checked nearest rounding. The original comparisons establish
the stated cases; universal cross-CRT transcendental bit equivalence is not
claimed. The full table fingerprint is also a native regression.

Palette data comes from outer chunk3: required owned name0x101 and fixed
palette0x201, optional owned opaque0x301, with duplicate known fields rejected.
Unknown well-framed fields are skipped. Finite outer versions greater than7
use6914-byte palettes: two range bytes,768 RGB bytes and24×256 shades. Versions
at or below7 use4866 bytes and expand16 rows by `floor(2*row/3)`. Start/end are
retained as supplied; end is exclusive. Only installed scene ranges define
output RGB. No global/window palette or faction recoloring is assumed.

The caller selects a single sample if geometry scalar times the stored map
scale is≤0.5, otherwise a2×2 square. Depth uses the original uint32 modulo
shift/add accumulator and unsigned high16 comparison. The added
`DrawModuloDepth` entry point changes only depth arithmetic; existing `Draw`
keeps its strict signed-shift admission and all previous tests. X/Y widths,
surface bounds, lookup extents and complete draw prevalidation remain checked.

New surfaces clear to index9/depth65535. Host RGBA alpha is0 exactly when the
index is9, independently of depth, RGB or palette flags. Index0 remains opaque;
index9 stays transparent even with written depth. Transparent RGB is retained.
This matches the recovered software-copy key, not an original RGBA framebuffer.

## Pinned original comparison

Contracts consumed are RE-SMAC-SCOUT-IDLE-RENDER-001,
RE-SMAC-SCOUT-TARGET-COPY-001, RE-SMAC-SCOUT-LIGHTING-GENERATION-001,
RE-SMAC-SCOUT-IDLE-VIEW-001, RE-SMAC-MATRIX-SCHEDULE-001 and
RE-SMAC-CVR-PALETTE-DECODE-001. Existing checked assembly, origin,
basis and raster contracts remain documented alongside their components.

| Evidence/input | SHA-256 |
| --- | --- |
| Original SMAC executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| VI asset | `d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1` |
| viptr00 asset | `572ca9d55ad49c2ab32a90ae8d3e1b1bc9a78e7901cf4387d26d0dde205a4cee` |
|20-view fixture | `91d4a68b9cdc330741278d9a2ebc40b9d75706f7f9a2f1c0bd11378f2bdf7894` |
|111 synthetic matrix fixture | `7f0502fd9e801ed9c6d1feea5a49c4fe5d566f75237559ff0c913dd0f9ea8f72` |
| Original full internal draw report | `f53d0ee3a9aaeaefad7d4654763e4a1ad72903583658625ed9e1af7f9f6faee4` |
| Generated51,456-byte first lookup | `81102740e2d482516c9d1b42fec5972c2ba8364cb597fd7492e82d1e287eec60` |

The Release production probe with UBSan matches19/19 complete original drawing
views' indexed and depth hashes and all20 target anchors. The twentieth original
case is target-culled; its pixels are explicitly excluded because this API does
not apply target clipping. No captured transform/table is an input to these runs.
The baseline dispatches65 streams, processes47,483 records and performs2,627
accepted writes. Baseline indexed/depth/RGBA fingerprints are respectively:

```
5bd393374354f29e228c84156584aa92580ecd36d80b9b8236caf1f7f63f3584
ce09c6e31936e8918dbeeae07297c3ec761e93279463d44c58fa2c7a3a0927e8
8a48c86fc334faf5bb99fc59e543b10c8f5d08a09085b2b2cf20d520c5508884
```

The original caller chain is4d7860→4d8400→631370→654770/654849→659800;
matrix helper641bb0 supplies the recovered cell schedule. The retained original
internal draw stops before final2D copy; the separate target-copy evidence
establishes the key9 conversion. This is routine-level controlled original
execution, not a live screenshot comparison or a game-window test.

Build the opt-in probe from repository root (user-owned assets remain external):

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -fsanitize=undefined \
  -fno-sanitize-recover=all -Isrc tools/tests/cvr_scout_probe.cpp \
  src/loader/cvr/{CvrContainerReader,CvrSceneReader,CvrGeometryReader,CvrFrameAssembler,CvrIndexedRasterizer,CvrBasisConverter,CvrPaletteReader,ScoutLighting,ScoutArithmetic,ScoutView,CvrScoutRenderer}.cpp \
  -o /tmp/cvr-scout-probe
python3 tools/reference/compare_scout_renderer.py --probe /tmp/cvr-scout-probe \
  --assets "$SMAC_INSTALL" --cases "$PINNED_VIEW_CASES" \
  --output /tmp/scout-comparison.json
```

The tool resolves case-insensitive installed names uniquely, rejects mismatched
asset/fixture fingerprints, and compares literal original hashes. Its binary
output is65,536 indexed bytes,131,072 little-endian depth bytes, then262,144 RGBA
bytes. No asset payload or rendered proprietary image is committed. A local
PNG preview is encoded directly from the returned RGBA without resampling.

Source validation uses synthetic, asset-free native tests:

- Eleven new Scout groups plus seven existing indexed-raster groups pass with
  allocation tracking in both normal and ASan/UBSan/LSan builds. They include
  all111 synthetic matrix cases, complete two-scene assets, grouped origins,
  separate shade tables, terminal exclusion, key9/depth distinctions, texture
  ownership, malformed input, zero signs and nearest-mode rejection.
- Full normal CTest8/8 passes in73.39s; full ASan/UBSan/LSan CTest8/8 passes
  in132.16s with leak detection and halt-on-error enabled. The isolated sanitizer
  cache retains its existing local `-no-pie` startup workaround; no repository
  toolchain policy changed.
- Four copied-source controls fail as intended: omitting the fourth group vector
  changes all19 compared views plus baseline RGBA; forcing single samples changes
  the2/1-scale view; using depth for alpha fails the actual native written-key9
  case; changing the first matrix cell to uniform order fails its scheduled
  cancellation fixture. The checkout and validated binaries are never mutated.
- An initial test-only compile failure used the legacy assertion macro with an
  incompatible expression; the message was corrected before the final builds.
  That retained diagnostic is not counted as defect proof.

Full production correctness analysis is required because this change adds
headers/CMake registrations. LLVM15 checked all319 production translation
units with zero failures in233.44s. Local logs, probe outputs, comparisons and
the unaltered PNG remain outside Git under
`/home/jhurliman/src/glsmac-resource-callback-audit` and the owned checkout's
ignored `out/` directory. No Windows or macOS build result is claimed by this
source receipt.

## Main-based publication receipt

The publication branch is based on pinned main
`658d4bd5b27a01b44309e10329a88e1ad7994d64`. Its source commit
`d5cd6861012d42c3136611c302afe23adce2c64d` preserves the renderer, native tests,
fixtures and tools from reviewed source
`8365d135f3f25daaac8bfa40b94d114e1ec499cb` byte-for-byte. The sole cherry-pick
conflict was the test call's location: one Scout registration was added while
main's existing mineral-factor registration remained in place.

On that exact publication source, normal CTest passed 8/8 in 46.05 seconds and
all 18 Scout/indexed native groups passed with allocation tracking. The required
full-scope LLVM 15 analysis checked 306 production translation units with zero
failures in 118.33 seconds. A newly compiled Release/UBSan probe again matched
all 19 original drawing views, all 20 anchors, the full generated lighting
table and baseline keyed RGBA. This is separate from the larger source branch's
sanitizer and 319-unit analysis receipts above. Publication logs are retained
under the same audit directory with the `scout-publication-` prefix.
