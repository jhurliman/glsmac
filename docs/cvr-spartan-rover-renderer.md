# Owned Spartan Rover idle renderer

`CvrSpartanRoverRenderer` produces an owned 256x256 indexed/depth image and an
owned RGBA conversion for the admitted starting Spartan Recon design: Speeder,
Gun, Scout armor, reactor 1, no abilities, animation 0. It consumes installed
assets through a logical-name reader. It does not create a Unit, choose a design,
replace Mind Worm gameplay, or assign an Infantry sprite to a Speeder. A live
render descriptor/cache/actor binding remains a separate consumer.

The fixed ordered composition is `VRCP00.cvr`, `VGMC.CVR`, `VGMCP.CVR`, `VB.CVR`,
`VBP.CVR`, `VSP.cvr`, `vsptb.cvr`, `vsptf.cvr`, `VLIGHTS.CVR`, `VR00.cvr`, each at
frame 0. Their asset locations are used directly. The Infantry composition's
(-400,200,0) attachment offset does not apply. Existing Scout frame, matrix,
lighting and checked raster machinery is shared; the original Scout entry point
retains its separate assets, scale, light and offset.

The reader requests `system.txt`, those ten files, `PALETTE.PCX` and
`spartans.pcx`. Application callers resolve these names through the existing
installed-resource resolver. No file paths are serialized. All scenes, frame
values, geometry levels, palettes and generated lookup tables are owned before
loading returns; the reader and its temporary strings need not survive. Copies
share immutable decoded composition data. Each render returns independent
buffers and makes no source mutations. `CvrScoutImage` is the existing common
owned image value and can use its existing texture-copy adapter.

## Material and view inputs

The software palette copies the first 236 RGB entries from `PALETTE.PCX` to
indices 10..245. The other 20 entries are the verified constructor constants,
including 8=(8,0,0), 9=(9,0,0), 246=(245,0,0), 247=(246,0,0). PCX entry flags are
not alpha. Faction artwork uses an unsigned-byte +10 palette offset; the tint
sample is column 435, row 745. Its embedded palette does not replace the software
palette. Installed Spartan artwork gives index 80, RGB (64,76,76).

Each channel of each software color is multiplied by the tint channel and the
exact binary32 1/255 constant, then truncated. The nearest of all 256 software
colors is selected by unweighted squared RGB distance, with the lowest index
winning ties. These products fit exactly in binary64 and the distances fit
signed32. This mapping transforms the owned 24x256 shade bytes from `VBP.CVR`;
only `VGMCP.CVR` and `VBP.CVR` use the resulting actor table. Other parts retain
their own decoded shade tables. Runtime material generation uses no captured
lookup bytes, zero remap fallback or approximate faction color.

The admitted `system.txt` section is exactly one `#VEH_LIGHT_DIR_MAP` row with
integers 6,4,15. The port's strict text parser admits whitespace/comments but
rejects duplicate sections, extra rows, partial integers, NUL and other vectors.
It is deliberately not a general original text-loader emulator. Those supplied
integers are normalized using the observed binary32 inverse-length staging.
The common idle camera/facing/anchor policy is retained. Speeder uses the stored
base scale without Infantry's 1.25 multiplier. Facing -1/0..7, zoom -8..8 and the
existing positive-denominator ratio domain remain explicit inputs. Transcendental
operations retain the existing host-library/cross-CRT validation limitation.

The internal surface starts at index 9 and unsigned depth 65535. RGBA uses the
decoded software RGB and alpha 0 precisely for index 9, otherwise 255. This is
the host texture conversion of the established original color-key policy; it
is not an original alpha plane or native window screenshot. Target clipping,
overlays, animation and other designs/reactors/factions are outside this adapter.

## Preserved linked detail levels

`CvrSimpleDetailChain` is the separate simple flags 0/1 profile required by
VLIGHTS. `CvrGeometryReader::Read` retains its previous standalone 0/2/8/10
profile. At each level, header +4 is the remaining chain extent, +12 is the local
extent/next relative offset, and +16 is local extent minus one. The local body
contains its positive count of three-byte nonterminal records followed by five
zero trailer bytes. Every link, scalar, extent, record and final end is checked
in a first pass before result storage allocation. Flags, vectors, scalar bits,
remaining extents and records are preserved without masking or flattening.

Selection starts at level 0. For threshold binary32 bits 1068827777
(1.414199948310852), stop if current scalar times draw scale is >= threshold;
otherwise stop if the candidate next scalar times scale is > threshold;
otherwise advance. Products of finite binary32 inputs are exact in binary64,
without a binary32 spill. Scalars are positive and finite. Finite zero/negative
scales are accepted by the isolated selector for diagnostic comparisons; they
do not broaden renderer zoom admission. The selector returns a borrowed index
into its own immutable levels, used only during the synchronous draw.

VLIGHTS contains five levels at offsets 0,5237,8359,10101,11090, with counts
1730,1025,565,314,184. The normal .07 scale selects level 4; .105 selects level 3;
.14 selects level 2. The latter two retain continuation flag 1 and still render
as nonterminal streams. The renderer uses the selected scalar for projection,
lighting and footprint, and the selected local records for rasterization.

Port resource/error policy is separate from original malformed-input behavior:
64 levels and 1,048,576 total detail records; 32 MiB per CVR and 64 MiB aggregate;
200 aggregate objects/groups (all retained levels count), and 1,048,576 aggregate
records. PCX is bounded to version-5 RLE, one 8-bit plane, origin zero, dimensions
and even stride <=2048, <=16 MiB, exact rows and exact palette trailer. `system.txt`
is capped at 64 KiB. Unsupported/malformed inputs throw before returned
publication; allocation/I/O failures propagate. Source-reader callbacks may
materialize bytes before these adapter checks. Grouped detail chains, other
geometry continuation combinations and general PCX variants are not admitted.

## Reproducible evidence boundary

The opt-in production probe is `tools/tests/cvr_spartan_rover_probe.cpp`;
`tools/reference/compare_spartan_rover.py` compares its outputs with retained
original results. It accepts installed assets and explicit view fields, never
captured camera matrices, lighting windows, material rows or geometry records.
Case-insensitive asset lookup rejects missing files, directories and collisions.
The proprietary input/output buffers and PNG previews stay outside Git.

The evidence is versioned here so the implementation does not depend on private
human research files being included in a publication branch:

| Contract | Fixture SHA-256 |
| --- | --- |
| RE-SMAC-SPARTAN-RECON-001, composition/view | `24131603b688b75a0c467bb9cba59a5e098ecab9628f2b48e1a9cc725c3e1329` |
| RE-SMAC-ACTOR-COLOR-001, materials/colored views | `eacd4d9d32e6e32768fa4cc44b6b58eec81158c876d4598a402973a0a119b7b0` |
| Reserved palette/source-shade inputs | `c3af2e46ead2bd1b73d37b7dd066c0095c13c01f8540d56c5fce5dea953db418` |
| RE-SMAC-CVR-LOD-001, linked levels | `ad74cb962164657691e6f8310c5cebb8c8b7cf8332ffd354f32847139cac60d1` |

Original executable SHA-256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Retained composition report SHA is
`81615c245dd57b2da520c104400d12f6535edb8dfd99cf81491abbf381f838ab`;
colored report SHA is
`9d5619b3e4b0fadf59c7efd6d5d0398acb008f1c8229c78999a3740d62c39a6f`.
No original executable is rerun by this implementation's validation.

The actual optimized C++ probe matches 15 material projections, all 20 anchor
results and 19 visible indexed/depth images. The one original target-culled
view is metadata-only: this adapter has no target-window clip. RGBA is checked
against the complete independently reconstructed software palette and key-9
alpha. The detail probe checks 32 installed scalar queries, 16 owned synthetic
boundary queries and 20 prior-view scale queries, and reconstructs every owned
header/record byte: all 11,689 bytes and each of the five block/record hashes
match. This is 68 selector queries, not 68 additional full original draws.

VLIGHTS asset SHA is
`7b90c70683e0afe1a6ac2677e7a3d008e6714ae98d575db69176a1100f9570f1`;
its complete geometry SHA is
`a2132d0f984086b5e09068a3c45b58a7bd1885d84d8f07de832688f19f8a91aa`.
VBP source shade SHA is
`7ec7220b339f81a78ddb0ff5570f97cd8dc25f7a0e9eb88ca610326b98125e1c`;
the generated Spartan 6144-byte material SHA is
`cf8960d9612c082f18e5411cc03809523c4641361f7ea5a366cebddc9165932b`.

Seven native groups cover owned PCX/material lifetimes, distinct sample versus
neighbors/padding, malformed framing, preserved linked data, exact/neighbor
selector boundaries, malformed late levels, fixed composition and view-specific
selection after source-reader destruction. Existing Scout groups run alongside
them: all 21 selected groups pass with allocation tracking in normal and
ASan/UBSan/LSan builds. Full normal CTest passes 8/8 (80.60s); full leak-enabled
ASan/UBSan/LSan CTest passes 8/8 (145.02s). A final test-only current-level-equality
assertion was rebuilt and passed in both tracked selections after those suites.
The optimized production probe passes the complete original comparison in both
Release and UBSan builds. Full LLVM15 correctness analysis passes all 331
production translation units, zero failures, in 246.41s with two workers;
the Windows-only network implementation is outside that Linux scan.

Five copied PCX/material translation-unit controls fail their intended native
assertions (offset, sample column, reserved color, tie order, trailing bytes).
Six copied detail controls fail for candidate equality, current equality,
sqrt(2) threshold, premature binary32 product spill, last-level-only selection,
and ignored late trailer. A separately copied renderer using the Infantry
attachment offset completes but fails both original indexed and depth image
comparison. Intended sources are never temporarily mutated. The initial image
diagnostic that exposed this offset mistake is not counted as a passing result.
No live Unit/actor or native Windows/macOS result is claimed by this source
checkpoint.

The focused publication reapplies only the Rover feature to pinned main
`868b08bc1eed3c2fd93e134adf0e90cf40a1ab51`. The sole conflict is the native-test
registration union; production, native fixture and probe/comparator files are
byte-identical to source commit `92e4cc0c091b8fb37b5e3203089014b685173a6a`.
On that publication stack, all 21 selected groups pass with allocation tracking,
full normal CTest passes 8/8 (71.80s), and explicit LLVM15 analysis of the five
affected production translation units has zero failures (2.85s). The broader
source sanitizer and 331-TU receipts above remain distinct from this publication
validation. No frozen movement-rule prerequisite is imported into the branch.
