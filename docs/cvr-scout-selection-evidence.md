# Scout component selection: original-renderer evidence

Read-only audit, 2026-10-16. This note separates static observations from
**executed original selectors with stubbed composition**. The bounded probe
below passed 28 executions. There is no live renderer integration.
The existing [geometry reader](cvr-geometry-records.md) preserves owned raw
records; it does not yet establish their geometric or shading meaning.

## Reference identity and provenance

The inspected installed GOG executable is `terran.exe`, 3,190,840 bytes,
SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Addresses below are virtual addresses in that exact PE32 image, whose image
base is `0x400000`. They do not apply automatically to `terranx.exe`, PRACX
variants or other releases.

The audit read the existing local `tmp/reference/re-terran-20261015/index.json`
for candidate function boundaries and independently decoded the installed
executable's bytes with Capstone 5.0.7. No Binary Ninja process, project or GUI
was accessed. The local original files, human research notes and existing
research scripts were not changed. No community implementation or asset
payload is included here.

Own ignored evidence is under `out/validation-cvr-selection/` in the audit
checkout: `static_inspect.py`, `static-references.json`, `selector-callers.txt`
and the `disasm-0x*.txt` files. The script verifies the executable hash before
reading it. Linear disassembly can include jump-table data; table entries
below were read as little-endian words, not interpreted as instructions.

The installed `alpha.txt` is 53,073 bytes, SHA-256
`38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
Its Scout Patrol references are `Infantry`, `Gun`, `Scout`: respectively
chassis row 0, weapon row 0 (Hand Weapons, attack 1, mode 0), and defense row
0 (No Armor). The [component resolver](https://github.com/jhurliman/glsmac/blob/e022b688e4206ee9848050d816329dd125ade4ca/docs/unit-component-resolution.md) preserves
these explicit table identities. `#UNITS` supplies no reactor field; this
audit does not fill that gap with a presumed default.

## Static file-to-object binding

The initialization routine at `0x45cb80` passes literal filenames and distinct
global object addresses to `0x630d10`. That helper replaces a stored loaded
pointer using a file handle, then closes the handle. These are static call
relationships, not an observed successful load.

| Literal filename | Model object address | Call instruction |
| --- | --- | --- |
| `VI.CVR` | `0x7af6c8` | `0x45ce3a` |
| `vw00.cvr` | `0x7af880` | `0x45cc69` |
| `VW01.CVR` | `0x7af370` | `0x45ce67` |
| `viptr00.cvr` | `0x7af820` | `0x45cca5` |
| `vipta00.cvr` | `0x7af808` | `0x45cce1` |
| `vt.cvr` | `0x7af668` | `0x45d005` |
| `vwntu.cvr` | `0x7af678` | `0x45cff6` |

The independently hashed installed `VI.cvr` is
`d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1`;
`Vw00.cvr` is
`18395bc46eecb0988164c00365d4d7cbc9ca527e3e732067608fd366c62ae497`.
The earlier [asset audit](https://github.com/jhurliman/glsmac/blob/e022b688e4206ee9848050d816329dd125ade4ca/docs/conventional-unit-rendering-plan.md) established
58 objects and 17 frames in VI, and one object and 17 frames in Vw00. Those
counts alone do not establish composition or animation.

## Static selection path

`0x4d8400` is an instance-based composition path: it reads a design index from
the 52-byte unit record (`0x9951b0 + unit_index * 52`, index at offset 10),
then component bytes from the 52-byte design record
(`0x9b01e8 + design_index * 52`, chassis/weapon/defense/reactor at offsets
36/37/38/39 and ability bits at offset 32). It calls the body, weapon, ability
and armor helpers below. This links those helpers to actual unit records;
the audit has not executed this whole path or its other state dependencies.

`0x4d87e0` is a smaller composition wrapper with seven stack inputs inferred
from its call instructions: faction, chassis, weapon, defense, reactor,
abilities, output object. It clears output offset `0x13cc`, calls the same
four helpers, then conditionally adds reactor art. This is a useful bounded
execution seam with an explicitly supplied reactor value.

| Helper | Static observation for chassis 0 / weapon 0 / defense 0 |
| --- | --- |
| Body `0x4d88e0` | Reads weapon mode from `0x9b6fb2 + weapon * 16`; mode below 3 follows the Infantry combat branch. Calls `0x631a60` with VI object `0x7af6c8`, raw pointer `0x6aa028`, and signed global `0x8f3c60` modulo 15. |
| Weapon/package `0x4d8e40` | Reads signed attack from `0x9b6fb0 + weapon * 16`. For mode below 3 and attack 1, its local lookup yields -1 and it returns without a model call. Attack 2 selects `0x7af370` (vw01), with additional Infantry attachment calls. |
| Ability `0x4d8700` | Ability bits 0 return without a model call. This does not establish any nonzero ability mapping. |
| Armor `0x4d9130` | Defense 0 dispatches to `0x4d92cc`, leaving its attachment flags clear; chassis 0 then returns without a model call. Defense 1 is a useful positive control that reaches `vipta00`. |
| Reactor tail in `0x4d87e0` | Supplied reactor 0 skips the tail. Supplied reactor 1, chassis 0 and combat mode select `0x7af814 + reactor * 12`, hence `viptr00` at `0x7af820`, with raw pointer `0x6aa028` and frame 0. This is not evidence of the default reactor. |

For the investigated nonnegative frame values, the body branch uses frames
0 through 14 despite VI containing 17 frames. The purpose of the other two
frames, negative-counter behavior in actual play and animation timing remain
unestablished.

`Vw00` must not be attached merely because the design uses weapon row 0.
Direct references to its global object occur in other consumers at
`0x436810`, `0x43bc40` and `0x43e2b0`. For example, `0x436810` special-cases
selected weapon 0 to call `0x632060` with `0x7af880`, while other selections
call the weapon helper with chassis -1. This supports a separate component
preview interpretation; it does not prove the complete UI context or every
possible indirect consumer.

The low-level composition routines take the output object in ECX:
`0x631a60` consumes three stack arguments and returns with `ret 12`;
`0x632060` consumes two and returns with `ret 8`; `0x6321c0` is the related
three-argument variant. `0x631a60` iterates loaded scene objects, selects
per-frame arrays, calls `0x641ee0` with one of them and the supplied pointer,
and copies other data into the output. That suggests an attachment/transform
role for the pointer, but its coordinate semantics are not established here.

## Executed bounded selector experiment

`out/validation-cvr-selection/probe_selectors.py` maps the pinned original
PE headers and raw sections with Unicorn 2.1.4; remaining image bytes and
synthetic stack/output pages begin zero-filled. It enters `0x4d87e0` with
explicit component inputs. Executed code is restricted to that wrapper and
the four selector helpers. The three composition entries are intercepted
**before** their instructions execute. Hooks record the callsite, ECX and
raw arguments, then emulate the verified callee stack cleanup.

Data-read permissions cover only the exact static jump-table ranges,
synthetic stack, supplied weapon attack/mode bytes and four-byte frame
counter. Original writes may target only the stack and output's four-byte
count field at offset `0x13cc`. The report records these ranges, all observed
data accesses and every supplied byte. It also records PE initialization,
initial register canaries, and all substituted registers. An unexpected
instruction or access raises an error. Each invocation has a 10,000 original
instruction budget and a two-second emulator timeout. Reaching the return
sentinel, correct stack cleanup and unchanged callee-saved canaries are
required.

For the selected paths, static continuation inspection found no composition
output read or conditional branch on its return value. Each case was also
run under two different stub states: EAX 0 versus 7, with distinct poison
values for caller-clobbered ECX and EDX. Both produced the same expected
ordered selection trace. This tests independence from those substitutions
for these paths; it does not simulate composition success or failure effects
on a rendered result. The output count is reset by the original wrapper but
is not incremented by the stubs.

The installed weapon rows supply attack/mode 1/0 for Hand Weapons and 2/0
for Laser. Faction is explicitly 1, chassis 0, abilities 0. Reactor is an
input, never inferred. The 14 cases below each ran under both stub states:

| Inputs | Observed ordered model selection |
| --- | --- |
| Scout, reactor 0; counters 0, 14, 15, 16, 29, 30 | VI only; frames respectively 0, 14, 0, 1, 14, 0 |
| Scout, reactor 1; same six counters | VI at the corresponding frame, then viptr00 at frame 0 |
| Laser weapon 1, defense/reactor 0, counter 0 | VI, VW01, VGMT, VGMTP; all frame 0 |
| Hand Weapons, defense 1, reactor 0, counter 16 | VI, then vipta00; both frame 1 |

All **28/28 executions passed**, consuming 218, 231, 239 or 240 original
instructions each. Assertions compare the complete ordered routine,
callsite, model, raw pointer and frame argument sequence, not just the
presence of VI. The Laser control's additional models are independently
bound to `VGMT.CVR` at initializer call `0x45ce49` and `VGMTP.CVR` at
`0x45ce58`; its raw extra pointer is `0x791298` for supplied faction 1.
No pointer's geometric or color interpretation is inferred from this trace.

Three fail-closed controls also passed: budget 1 stops on instruction count;
withdrawing frame-counter read permission stops at `0x4d8966`; withdrawing
output-count write permission stops at `0x4d87f3`. No unexpected code or
access occurred in the successful cases. The experiment executed no original
file loader, allocator, OS import, composition body, geometry converter or
rasterizer, and wrote nothing to installed files.

Local evidence hashes:

- `probe_selectors.py`: SHA-256
  `acce5af40adaed485b37b535f90470d44548b50156b9d7015f09b8a59bb7e361`.
- `selector-probe-report.json`: SHA-256
  `d0a603597db02d4f288d7a902b4e11cde8f028d40366bc5639ed7d4bd23f235c`.

The exact local experiment above used the audit checkout command below:

```powershell
& 'C:/Users/jhurliman/Documents/Code/jhurliman/glsmac/tmp/reference/emulation-venv/Scripts/python.exe' out/validation-cvr-selection/probe_selectors.py
```

The retained [repository probe](../tools/reference/probe_scout_selectors.py)
adds explicit input/output paths and enforces Capstone 5.0.7 and Unicorn 2.1.4.
It includes no original executable, asset payload or community implementation.
Install those two Python dependencies in a reference-only virtual environment,
then run from the checkout root with your purchased matching installation:

```sh
python tools/reference/probe_scout_selectors.py /path/to/terran.exe --output out/reference/scout-selector-report.json
```

By default it reads `alpha.txt` beside the executable; `--rules` selects an
explicit path. Both hashes are checked before emulation. The generated report
is local evidence, not a repository asset. The original experiment hashes
above identify the earlier local script and report, not the parameterized
repository script.

The parameterized repository command was rerun on October 16, 2026: all 28
selector executions and three policy controls passed, and its report SHA-256
was exactly `d0a603597db02d4f288d7a902b4e11cde8f028d40366bc5639ed7d4bd23f235c`.

This is not an original-renderer screenshot or a geometry/attachment fidelity
test. The next reference gaps remain: default reactor construction, actual
loaded-scene attachment math, record-to-position/color decoding, projection,
pixel coverage, animation timing, and corresponding SMAX selection behavior.
