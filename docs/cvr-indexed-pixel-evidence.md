# CVR records to indexed pixels: original SMAC evidence

The bounded experiment executes four original routines with **42 passing cases
and eight negative controls**. It establishes a cumulative 26-direction sample
walk, two-stage byte lookup, unsigned depth comparison and one-pixel or four-pixel
coverage. Complete record streams from an infantry component and the armor control
also pass. These records feed an indexed software raster kernel; treating them as
triangle vertices would require different evidence.

The projection basis, initial position, lookup tables and surface are explicit
synthetic inputs. This is **not an original camera, original lighting/RGB palette,
complete Scout render, screenshot comparison or SMAX result**. It does not justify
choosing triangle topology or applying GLSMAC's current lighting/gamma to CVR data.

## Reference and provenance

All addresses refer to the installed GOG PE32 `terran.exe`, 3,190,840 bytes,
SHA-256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Capstone 5.0.7 decodes its disk bytes; Unicorn 2.1.4 executes the allowed routines.
The probe refuses a different executable or selected asset hash.

| Local input | SHA-256 | Selected object |
| --- | --- | --- |
| `VI.cvr` | `d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1` | `EH_M1-Mask05` |
| `Vipta00.cvr` | `b54fff2c0096e1545004c15fb2fd546883a45aa0af96ffe58d9daa5054ededbd` | `VIPTA00` |

The [selector](cvr-scout-selection-evidence.md) and
[composition](cvr-scout-composition-evidence.md) experiments establish why these
files are useful body/armor inputs. Armor remains a defense-1 control; it is not
silently added to Scout Patrol. No reactor default is inferred.
The [geometry reader](cvr-geometry-records.md) previously preserved the raw
records without interpreting them. This experiment provides a bounded
interpretation for the simple non-block draw path.

Existing local `tmp/reference/re-terran-20261015/index.json` supplied candidate
routine boundaries. Those candidates were independently decoded from the pinned
executable. No human research file/session was changed and no community renderer
implementation or table was copied. Installed files and raw reports stay local.

## Executed routines and supplied boundary

| Routine | Executed behavior |
| --- | --- |
| `0x6558d0..0x655982` | With fixture context flags 0 and surface format byte 1, selects draw `0x6867f0`, clipped draw `0x686000` and initializer wrapper `0x655a70`. Only the first draw target is exercised. |
| `0x65b3a0..0x65b8f2` | Builds all 26 step vectors from six supplied signed-16 values and three supplied signed-32 values. Its packed/scratch outputs execute too; the selected kernel uses the ordinary step table. |
| `0x686d51..0x686e43` | Patches exactly 38 existing four-byte operands with supplied table, color, depth, row-offset and pitch values. |
| `0x6867f0` | Executes only entry, non-block loop and common return ranges. Reads three-byte records, performs table lookup and depth tests, writes color/depth samples. |

The initializer wrapper's static call to `0x686d51` was inspected; the experiment
invokes this leaf initializer directly, leaving its other kernel initializers
unexecuted. Likewise the original renderer's geometry routine `0x659800` was
inspected but **not executed**. Its simple branch supplies the header count at
`+38`, stream at `+42`, and maps geometry flag 8 to draw-mode bit `0x20` at
`0x659e2b..0x659e73`. That connection is static evidence, separate from execution
of the exact raw record streams below.

No routines are replaced with returning stubs. Supplied state comprises:

- A minimal context/surface sufficient for the original selector.
- A projected basis and initial integer accumulators, chosen for bounded output.
- An explicit row-offset table, pitches, color bytes and uint16 depth bytes.
- A 2,048-byte first lookup and 24 by 256-byte second lookup, both synthetic.
- Raw fixture records, or independently extracted flags-8 installed records.
- A bounded cdecl stack and x87 control word `0x037f` for the step helper.

The synthetic lookup functions are recorded in the source and every supplied
byte is retained in the local report. The selected second-table size matches a
statically inspected context allocation; **the original table builder does not
run**, so this does not establish its values, lighting model or color meaning.

## Executed dataflow

For bytes `(b0, b1, b2)`, the tested kernel uses:

- `code = b0 >> 3`. Codes 0 through 25 select the lexicographic triples in
  `{-1, 0, 1}³`, omitting `(0, 0, 0)`. The original step builder produces the
  corresponding linear combination of each supplied basis row.
- The three running accumulators advance **before** the current sample is drawn.
  The first two address components use arithmetic division by 256, rounding
  toward negative infinity. The depth path shifts its input and steps by eight
  bits, then compares the high word as an unsigned uint16.
- `lookup_index = ((b0 & 7) << 8) | b1`.
- `indexed_color = table[(first_lookup[lookup_index] << 8) | b2]`.
  Calling these indices a particular normal encoding, material, or RGB color
  would require the upstream table builders. Variable names in the probe do not
  expand that claim.
- A sample writes only when its uint16 depth is **strictly less** than the stored
  depth. Equal depth preserves the previous color. Negative supplied depth also
  follows the observed unsigned comparison after word extraction.
- Draw-mode bit 0 selects one pixel. With that bit clear, the routine tests and
  potentially writes `(x,y)`, `(x+1,y)`, `(x,y+1)`, `(x+1,y+1)` in that order.
  Each accepted sample writes depth first, then its color byte.
- Draw-mode bit `0x20` subtracts one from the supplied record count. The last
  triple is not read in this path. The loop does not validate a terminal code;
  the fixture and existing reader enforce the supported terminal convention.

The mode that chooses a footprint is an explicit argument here. This experiment
does not establish when the live game selects either mode or infer a world axis
from a supplied basis row. It demonstrates samples with a square footprint, not
triangle connectivity, solid cube faces or a complete geometric surface rule.

The fixture excludes accumulator overflow, clipping, zero effective counts,
block-compressed mode bit `0x10`, unsupported record codes and out-of-bounds
samples. The original un-clipped loop contains no safe count/extent validation
for those inputs. Grouped objects, group origins, header-vector transformations,
scalar application, camera construction and the other raster kernels remain
outside this executed boundary.

## Coverage and controls

The 38 synthetic cases cover all 26 directions individually; cumulative record
order; all four combinations of footprint/last-record modes; near, equal and far
depth; repeated equal-depth samples with different lookup inputs; fractional
stepping; and negative depth's unsigned tie behavior. The terminal bytes are
deliberately nonzero so ignoring them is observable in the permitted read set.

Four further cases run the entire selected installed streams in both footprint
modes. Their initial position is fitted to a **synthetic orthogonal basis** to
keep every sample within the supplied surface. Header vectors/scalar are retained
as provenance. No frame matrix is selected or applied in this experiment.

| Object | Raw triples | Processed triples per run | Supplied width / height / pitch |
| --- | ---: | ---: | --- |
| `EH_M1-Mask05` | 1,386 | 1,385 | 27 / 39 / 34 |
| `VIPTA00` | 9,162 | 9,161 | 61 / 45 / 68 |

Across all 42 cases, the original routines process **21,231 sample records**,
perform **11,414 accepted color/depth write pairs**, and execute 673,323 allowed
instructions. A scalar oracle computes results without reading emulator state.
Every color/depth byte, padding byte, canary and ordered depth/color write matches.
Every processed record byte is read exactly once in order; the terminal triple
is outside the permitted draw-read range when mode `0x20` is set.
The original 26-step table is checked independently against coefficient sums.

All eight controls fail at their intended boundaries:

1. Remove a required draw instruction from the execution allowance.
2. Deny the first lookup read.
3. Deny color-buffer writes.
4. Deny one required initializer operand write.
5. Reverse one step component in the scalar oracle.
6. Ignore the first lookup in the scalar oracle.
7. Replace strict depth comparison with less-than-or-equal in the oracle.
8. Reduce the total instruction budget to one.

Every execution checks decoded instruction starts and phase-specific code,
read and write ranges. Initializer code writes are allowed only at the 38
prechecked operand fields, each initially `0x01020304`; all are written exactly
once. Whole kernel-byte comparison rejects any non-operand change. **Draw-phase
code writes are forbidden.** Each case has a 500,000-instruction total budget;
each call has a two-second emulator timeout and must return through the sentinel with correct
cdecl stack cleanup and callee-saved registers. Ordered memory traces retain
instruction address, access address, width and value.

## Reproduction and retained evidence

From a checkout with Python, Capstone 5.0.7 and Unicorn 2.1.4:

```text
python tools/reference/probe_cvr_pixels.py --exe /private/terran.exe --assets /private/AlphaCentauri --report /private/pixel-report.json
```

The script imports only the local PE/chunk helpers in
`tools/reference/probe_scout_composition.py`. It neither searches for nor
downloads original files. Retain the report privately; it includes supplied raw
record bytes derived from installed assets. No image, binary, asset, palette or
raw executable bytes are committed by this change.

The completed local run used
`out/validation-cvr-pixels/pixel-report.json` (75,660,207 bytes):

- Probe SHA-256: `dee511e774ffa42cce36c8e63c4ff4820b42b3b2cc2831dc5bbf80d8de55c167`.
- Imported helper SHA-256: `5c97f8ebda930909d78896c48b764f5aea8ae5322cbdcf2a8dd2b9df1b59f201`.
- Report SHA-256: `f87e27980b2c0407e1c8f9218b70dabb1ee550c920c9b9c11ac31b2ce829f4db`.

## Next bounded implementation and reference gaps

A justified production slice is an explicit-input indexed sample rasterizer for
this non-block, un-clipped path: validated raw records, integer projected basis
and initial position, caller-provided lookup/row tables and owned color/depth
output. A command-line probe can exercise it against these original traces.
Reject unsupported arithmetic/count/extent inputs before writing; that would be
GLSMAC's bounded input policy, not a claim about original malformed-input handling.
Do not activate it for live units or select camera/palette defaults in that PR.

The next original experiment should connect the real geometry header, composed
frame matrix/location and scalar to the projected basis/origin, then execute the
lighting/index-table and RGB-palette preparation separately. Only after those
boundaries are supported can a composed Scout's indexed output become a faithful
texture in the existing renderer. A generic triangle mesh is not required by
the observed primitive and would introduce unsupported topology assumptions.
