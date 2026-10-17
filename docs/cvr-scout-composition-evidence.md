# Scout frame assembly: original SMAC evidence

The bounded probe executes the original scene-assembly routine and its vector
addition helper, including twelve cases reached through the original Scout
selectors. **36 cases and six negative controls pass.** At this stage the body
and armor contribute a flat ordered list of frame records: flag byte, translated
location, unchanged matrix and opaque resource references. This is evidence for
frame assembly, not a rendered Scout, vertex transformation or pixel fidelity.

## Pinned reference and prior evidence

The installed GOG `terran.exe` is 3,190,840 bytes, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
All addresses below are virtual addresses in that PE32 image. The experiment
does not apply automatically to `terranx.exe` or another patched executable.

The earlier [selector experiment](cvr-scout-selection-evidence.md) established
the Infantry/Gun/Scout body selection and a defense-1 positive control. This
experiment uses the same explicit faction 1, chassis 0, Gun attack/mode 1/0,
abilities 0 and **reactor 0 supplied by the fixture**, without inferring a
default reactor. Defense 1 is an armor control, not the Scout Patrol definition.

| Input file | SHA-256 | Objects / frames |
| --- | --- | --- |
| `VI.cvr` | `d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1` | 58 / 17 |
| `Vipta00.cvr` | `b54fff2c0096e1545004c15fb2fd546883a45aa0af96ffe58d9daa5054ededbd` | 1 / 32 |

The probe reads those files independently using the already checked
[scene-record layout](cvr-scene-records.md). It retains exact flag/location/
matrix bytes. No file loader or allocator from the original executable runs.
No original file, array payload or community implementation is committed.

Existing local research `tmp/reference/re-terran-20261015/index.json` supplied
candidate function boundaries; the executable bytes were independently decoded
with Capstone 5.0.7. The prior scene/geometry/selector notes and rendering plan
were read before this experiment. Human research files, installed binaries and
the Binary Ninja session were not modified or operated.

## Static loader-to-array correspondence

The following is static evidence, not an executed original-loader result:

- `0x652f00` stores scene frame count at loaded-state offset `0x20`, object
  count at `0x24`, and allocates a three-pointer array per object at `0x30`.
  It dispatches each object to `0x653110` in source order.
- `0x653110` dispatches the object's animation container to `0x653420`, passing
  the frame count and that object's three-pointer record.
- `0x653420` recognizes the raw little-endian words `0x04040301`, `0x04040302`
  and `0x04040303`, corresponding to file tags `01030404`, `02030404` and
  `03030404`. It allocates and reads exactly `frames`, `frames * 12` and
  `frames * 36` bytes into the three pointers, respectively. No matrix transpose
  or location transformation occurs along those reads.

That correspondence supports constructing a **supplied loaded-scene ABI** from
the installed arrays for the composition experiment. It does not prove the
complete loader's allocation, validation or error behavior. Geometry descriptor
pairs and the loaded-state field at `0x14` are supplied as distinguishable,
unmapped opaque handles: this stage copies or offsets those values without
reading their targets. Their downstream interpretation remains untested.

## Original routines executed

`0x631a60` receives the output object in ECX and three stack arguments: loaded
model handle, pointer to three binary32 offset components, and signed frame
number. It returns with `ret 12`. The tested nonnegative frame number is reduced
by the scene's positive frame count using signed division; negative-frame
behavior is outside the supplied input domain.

For each object in source order, it:

1. Copies the selected one-byte frame flag to the next output record. It does
   not filter objects on that byte in this routine.
2. Calls original `0x641ee0` on the selected three-component location and the
   supplied offset. That helper performs three x87 additions, each stored to
   binary32. No matrix participates in this step.
3. Copies nine selected matrix words, in their existing order, without arithmetic.
4. Copies the object's two opaque descriptor words and records the loaded-state
   field at `0x14` plus `0x302` as another opaque reference, then increments the
   output count.

The actual selector argument at `0x6aa028` contains words `c3c80000`, `43480000`,
`00000000`: binary32 **(-400, 200, 0)**. Both the inspected body and defense-1
paths use that pointer. It must not be replaced with a guessed zero offset.
The experiment establishes addition in array-component order, without naming
those components world X/Y/Z or deciding how later vertex transforms use them.

The output has capacity 200 in this routine. A count of exactly 200 returns 1
before another record is appended. Starting at 199 with two objects appends
one, then returns 1: original assembly is partial on this capacity condition.
That internal limit is not a CVR file-validity rule. Inputs above capacity,
zero frame counts and malformed runtime pointers were not modeled as valid.

## Executed cases and controls

The [opt-in probe](../tools/reference/probe_scout_composition.py) uses Unicorn
2.1.4. Direct assembly executes only `[0x631a60, 0x631bdf)` and
`[0x641ee0, 0x641f05)`. Selector integration adds the exact previously audited
wrapper and four selector ranges. **No function body is stubbed.** The harness
supplies memory state, not simulated composition return values.

| Cases | Coverage |
| ---: | --- |
| 18 | VI alone and VI followed by Vipta00, each at explicit frames 0, 1, 14, 15, 16, 17, 31, 32 and 33 |
| 12 | Original selector wrapper through actual assembly, defense 0/1, at counters 0, 14, 15, 16, 29 and 30; explicit reactor 0 |
| 4 | Two synthetic objects with three distinguishable frames, flags including 0/128/255, nonsymmetric matrices and offset (1.25, -2.5, 7.75); append after two existing records |
| 2 | Synthetic capacity controls starting at 199 and 200 |

Direct VI+Vipta00 assembly applies each scene's own frame count: for example,
supplied frame 17 selects VI frame 0 and armor frame 17. The actual investigated
selectors first reduce their counter modulo 15, so their tested composed frames
are only 0–14 even though the files contain more. The other authored frames'
purpose and animation timing remain unresolved.

Each successful case compares the **entire** output descriptor and destination
buffers, including untouched padding and existing records, against an independent
field-level expectation. It checks the complete composition call order, exact
flag/matrix bytes, binary32 addition results, count, descriptor propagation,
stack cleanup and callee-saved register canaries. VI contributes 58 records and
the armor control contributes one more; the source names are not lookup keys.
Observed original-instruction counts are 37, 121, 200, 4,904, 5,020, 5,122 or
5,260 per case.

Each case has a 20,000-instruction budget, and each invocation has a two-second
emulator timeout. A code hook permits only decoded instruction starts within
the declared ranges. Read/write hooks permit only the listed synthetic stack,
specific supplied state and declared output fields. Source arrays and opaque
targets have no write access; descriptor/table targets have no read access.
The return sentinel, cleanup and canaries must all match. Unexpected execution
or access fails immediately. Explicit x87 control word `0x037f` supplies
nearest-even rounding and extended precision; it is a fixture assumption, not
a measurement of the original process's live control word.

All six controls fail at the intended boundary: budget 1, denied helper code,
denied offset read, denied destination write, an oracle that omits the offset,
and an oracle that transposes the synthetic matrix. The last two are checks of
comparison sensitivity, not patched-original execution. Complete instruction
counts/accesses, supplied bytes, allowed ranges and outputs are retained locally.

## Reproduction and retained evidence

Install Capstone 5.0.7 and Unicorn 2.1.4 in an isolated reference environment:

```sh
python tools/reference/probe_scout_composition.py /path/to/terran.exe \
  --output out/reference/scout-composition-report.json
```

`--assets` selects a separate CVR directory; the default is beside the executable.
All three file hashes are checked before original instructions run. This command
uses installed files read-only and writes only its requested local JSON report.
It is opt-in and does not add original assets or a full-suite CI job.

The validating worktree retains `out/validation-cvr-composition/` with the report,
run log, and independently generated static disassembly. The earlier selector
and scene probes remain separate evidence. The October 16, 2026 run produced:

- Probe source SHA-256:
  `5c97f8ebda930909d78896c48b764f5aea8ae5322cbdcf2a8dd2b9df1b59f201`.
- `composition-report.json` SHA-256:
  `6adbab51313665ec1e2041627159556f1e4ae562d71c2df449ccfca627805e61`.

The large JSON report remains local because it contains supplied original frame
bytes. These hashes identify this run; they do not expand its scope.

## Smallest actionable implementation boundary

A subsequent pure, owned frame-assembly API can consume the existing decoded
CVR scenes and explicit scene/frame/offset selections. Its output should retain
the scene/object identity and flag, copy the selected matrix without changing
order, and compute the tested three additions. Own the scenes plus stable
indices, or copy the required values; do not publish borrowed pointers into
temporary source containers. Keep opaque geometry/table identity separate from
its eventual rendering interpretation. Validate a complete request before
publishing a usable GLSMAC result rather than exposing a half-built object.

Synthetic tests should exercise distinct scene frame counts, source ordering,
nonidentity matrices, nonzero offsets, append/limit policy and source lifetime.
Installed comparisons should continue through this original probe's actual
selector/assembly route. Explicit limits and invalid-input policy belong to the
new API; the original internal memory layout need not become GLSMAC's layout.

This would provide verified frame records to the next geometry stage. It would
still not establish record-to-vertex decoding, matrix application order, axis
orientation, world scale, projection, normal/color interpretation, pixel coverage,
reactor defaults or SMAX behavior. No live render mapping or conventional-unit
activation is justified by this experiment alone.
