# Supplied CVR matrix to fixed basis: original SMAC evidence

The bounded experiment executes the original nine-value conversion and its
original x87 integer-conversion helper. **41 cases pass**, covering 369 supplied
binary32 entries; eleven unsupported requests reject before emulation and seven
negative controls fail at their intended boundaries. This establishes a useful
arithmetic boundary for the [indexed rasterizer](cvr-indexed-rasterizer.md).

The input is an explicitly supplied sequence of nine binary32 values. It is
**not a claim that an unmodified CVR frame matrix is the renderer's input**, or
that identity is an appropriate camera/default. No frame composition, matrix
multiplication, axis conversion, geometry, lighting, RGB palette, pixel draw or
SMAX code executes in this experiment.

## Reference and exact execution scope

All addresses refer to the installed GOG PE32 `terran.exe`, 3,190,840 bytes,
SHA-256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The probe requires Capstone 5.0.7 and Unicorn 2.1.4 and rejects a different binary.
Existing local `tmp/reference/re-terran-20261015/index.json` supplied candidate
routine boundaries; the probe independently decodes the original disk bytes.
No human research file, executable or interactive analysis session is changed.

| Executed half-open range | Behavior | Original-byte SHA-256 |
| --- | --- | --- |
| `0x65a930..0x65ab42` | Scales nine supplied values, converts each, writes fixed basis and clears two scratch words. | `84b19e20729742160eb7edae9127496b568941e75df7c3152bed482cbbd8791c` |
| `0x660678..0x66069f` | Saves the x87 control word, selects truncation, converts to signed64, restores control and returns the result in EDX:EAX. | `d0a2aca68648a0ae9d5cbfae1b276d4a64d4f797fc0c60f28b64096cdfa3148c` |

There are no substituted functions. The second routine executes nine times per
case. Original constants at `0x695058..0x695068` have exact binary32 bits
`43800000 00000000 bf000000 3f000000` (256, zero, -0.5, +0.5).

The supplied environment is a cdecl argument pointer, nine binary32 words, a
bounded stack/return sentinel, initialized output/canary bytes and x87 control
word `0x037f` with an empty x87 stack. The original process, loader, allocator,
OS and renderer caller are absent.

## Observed dataflow and arithmetic domain

1. Read the nine input words in increasing address order. Multiply each by 256
   and store it as binary32 at `0x9c8f7c + 4*i`, preserving signed-zero bits.
2. Compare each stored value with zero. Add -0.5 for a negative value; otherwise
   add +0.5. Both positive and negative zero take the nonnegative branch.
3. Invoke the original helper. It saves `0x037f`, selects `0x0f7f`, converts the
   x87 value to a signed64 integer by truncation, then restores `0x037f`.
4. Store the low word for entries 0..5 at `0x9c8f40 + 2*i`, and the low dword
   for entries 6..8 at `0x9c8f4c + 4*(i-6)`. The admitted results fit those
   signed widths, so this experiment does not rely on narrowing wrap.
5. Write zero to `0x9c8fc4` and `0x9c8fc8`.

For the supported finite domain this is nearest-integer rounding with halfway
values away from zero after the binary32 scaling store. The independent oracle
uses exact rational values for the stored binary32 numbers; it does not replace
the original conversion helper during execution. Power-of-two scaling is exact
for admitted values, including subnormals. The selected extended-precision
addition and truncation agree with the rational result in these cases; there is
no intermediate binary32 rounding of the +/-0.5 addition.

The probe admits only requests whose first six rounded results fit signed16 and
whose last three fit signed32. For example, the boundary case returns
`32767, -32768, 32767, -32768, 0, 0, 2147483520, -2147483648, 2147483392`.
The next upper signed16 half threshold and the next representable positive
signed32 input are rejected by the probe before execution. NaN, infinity,
scaling overflow, wrong input count and different x87 precision/rounding controls
are also outside the experiment. **These are probe admission restrictions, not
claims that the original performs equivalent validation.** Original narrowing,
overflow, invalid conversion and other FP-control behavior remain unestablished.

This conversion's admitted signed32 outputs are not all accepted by the later
rasterizer: step construction, shifted depth and cumulative coordinates have
their own checked arithmetic domains. A caller must satisfy both contracts.

Control restoration is separate from status preservation. Every case returns
with control word `0x037f`, empty x87 tags/TOP, restored callee-saved registers
and expected cdecl stack position. The status word is recorded, **not restored**:
the retained run observes `0x0000` in 24 cases, `0x0100` in 16 and `0x4000` in
one, reflecting condition-code differences. No admitted case reports invalid,
divide-by-zero, overflow or underflow status in Unicorn. This does not establish
all hardware exception/inexact-flag behavior or promise a future port will
reproduce x87 status flags; the arithmetic output and original control-word
save/restore are the tested contracts.

## Cases and fail-closed checks

The 41 executed cases comprise identity, mixed signed zeros, a distinct
nonidentity matrix, four matrices around adjacent binary32 values at positive
and negative halfway thresholds, a small/subnormal matrix, a destination-width
boundary matrix and 32 deterministic mixed matrices. No installed frame is
silently treated as a camera-composed input.

For every case the probe checks:

- Execution only at independently decoded instruction starts in the two ranges,
  with at most 2,000 instructions and a two-second timeout.
- Reads confined to the input, original constants, scaled scratch and bounded
  stack; writes confined to basis/scaled/counter outputs and that stack.
- Exact nine input reads, all nine scaled binary32 bit patterns, all nine
  original helper signed64 results, ordered output writes and complete output
  bytes including untouched gaps/canaries.
- Nine ordered control-word save/truncation writes, final control/stack state,
  immutable input/code/constants and stack canaries.

The positive cases execute 12,679 original instructions. Eleven domain controls
reject before starting emulation. Seven further controls withhold helper code,
input reads or basis writes, replace the independent rounding oracle with
truncation or ties-to-even, transpose its expected output, or reduce the budget
to one instruction. Each must fail for its expected reason; an unrelated
emulator failure does not count as a successful control.

## Reproduction and retained artifacts

```sh
python tools/reference/probe_cvr_basis.py \
  --exe '/installed/Alpha Centauri/terran.exe' \
  --report /private/cvr-basis-report.json
```

The probe imports only the existing PE image reader/validation utilities from
`probe_scout_composition.py`; it does not run that helper's experiment entry point.
Reports contain ordered memory traces and original-derived bytes and must stay
outside Git. No proprietary bytes or generated images are committed.

| Artifact | SHA-256 |
| --- | --- |
| `tools/reference/probe_cvr_basis.py` | `fae91916c120d3518dcb2ce6d776d04e7d29833695a871b5efc887845fd6a46a` |
| Imported `probe_scout_composition.py` | `5c97f8ebda930909d78896c48b764f5aea8ae5322cbdcf2a8dd2b9df1b59f201` |
| Local `out/validation-cvr-basis/basis-report.json` | `4da27525e353d93085a608a1f6243a8664e86bb0a985be0d9335196d2fc1a7cb` |

The next small implementation can expose an explicit supplied-binary32-to-basis
conversion with this bounded arithmetic domain and original-report comparison.
It must not install a default camera or treat raw frame matrices as already
projected. Connecting the renderer caller's frame/scalar/camera operations, the
initial sample position and the lighting/RGB builders remains separate evidence
work before a faithful conventional-unit visual.
