# SMAC road quantum and ordinary ground basic allowance

`SmacMovementRules` retains the signed32 value from the first numeric `#RULES`
row. `Quantum()` derives `clamp(raw,1,100)` from that sole authority. Distinct
raw values remain distinct in equality and persistence even when they produce
the same Q. No default or independently writable Q exists.

`OrdinaryGroundAllowance` accepts an explicit Infantry/Speeder identity and the
existing parsed `ChassisComponent::speed`. For the admitted conventional Gun,
no movement-changing ability/package context it converts Moves to an unsigned
byte modulo256, clamps that byte to1..99, then multiplies by this profile's Q.
The result is1..9900 and every intermediate fits signed32. Chassis Cost is a
different field and does not participate. Unknown enum values reject.

The caller must establish that design context. This API does not inspect a
design, determine buildability, calculate morale/damage effects or grant live
movement. The current chassis parser is unchanged: it preserves the signed32
source value; the original byte store belongs to this calculation. Naval/air,
transport and ability modifiers are outside this contract.

## Ingestion and persistence

`SmacRulesTXTLoader::ParseMovementRules` reads one unique canonical SMAC `RULES`
section with exactly70 positional rows. It consumes only row1, allowing its
optional trailing comma and semicolon annotation. Existing port signed32
parsing requires the complete integer, permits one leading plus, and rejects
overflow, embedded NUL, fractional/hex/junk suffixes and malformed framing.
Other rule rows remain uninterpreted. This strict text policy is a port choice;
the original evidence supplies parsed integers rather than executing the full
original text lexer. No file lookup or SMAX inference occurs.

The self-contained movement profile V1 uses the existing typed Buffer envelope:
integer version1, then the raw signed32 road value carried by an integer field.
Decode rejects wrong types/version, values outside signed32, truncation and
trailing data before returning an owned value. It stores no derived Q, so raw
and derived values cannot diverge. This codec is not yet part of GlobalSettings
or a world snapshot. Host freeze, packet/world binding and live unit consumers
are separate follow-ups; merely parsing this profile does not activate it.

## Original evidence

The read-only human handoff is RE-SMAC-MOVE-Q-001. Actual original paths are
rules loader0x59c2e0, first-row clamp/store0x59c2ff..0x59c31b, chassis numeric
stores0x59e4f8..0x59e528, and complete basic query0x5d9400. The rules clamp uses
1..100. The basic query reads Moves unsigned and clamps1..99 before multiplying
by Q. The original token integer reader and row advancement were supplied;
clamp, byte stores, ability predicate and basic arithmetic executed. No original
arithmetic result was substituted.

| Evidence | SHA256 |
| --- | --- |
| Original SMAC executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Installed alpha.txt | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
| Original probe source | `b48fb64a0f62f214cbe90f6c6f4fe4ede7a3c8b262bcee4041eb35179f3e91d3` |
| Behavior case JSON | `c5a1be815524453228356ac7257e0a596737b632b268ae61031e1f6d79462359` |
| Original summary report | `142357caba479c41eea0dcee9d7a7f5bbd10b6fd876222216180fb547f8e53ba` |

The cases cross11 signed road inputs, two chassis identities and ten Moves
inputs. Cost17 is supplied independently. The original report records220/220
passing and installed Infantry/Speeder Q3, Moves1/2, allowance3/6. This port
comparison pins that report and compares the literal case expectations; the
report is an aggregate receipt, not a second per-row output dump. No original
executable or research script was rerun or modified for this implementation.

The optional `tools/tests/smac_movement_rules_probe.cpp` builds actual port
parsers, profile codec and basic calculation. Its stream records are four
signed32 integers: raw road rate, chassis0/1, Moves, Cost. It constructs a
synthetic complete rules section and chassis row, then prints raw rate, Q,
parsed Moves/Cost and allowance. Malformed input exits2; an empty stream is
valid. Passing one explicit alpha.txt path instead reads that bounded file and
checks positional installed Infantry/Speeder. It does not search installations.

`tools/reference/compare_smac_movement_rules.py` takes explicit `--probe`,
`--fixture`, `--report`, `--output`, and optional `--alpha` paths. It checks all
evidence fingerprints and ordered case IDs before comparing220 rows. Optional
installed comparison verifies the alpha fingerprint and the two recorded
chassis results. Source data and proprietary assets are not committed here.

## Validation

Source parent is main `ccb80244e1acfeb26338c0eb3745cb6b3384fd61`. Four focused
native groups exercise real parser ingestion, raw/derived/equality and codec
boundaries, plus actual chassis Moves versus independent Cost17. They are
registered in the existing full suite; no new default test invocation is added.

Linux Clang15 normal CTest passes8/8 in58.75seconds. All four groups pass with
allocation tracking in both normal and ASan/UBSan/LSan builds, with leak
detection and halt-on-error enabled. The sanitizer build retains the existing
local non-PIE workaround. The Release `-O2 -DNDEBUG` probe with UBSan matches
all220 original fixture rows and both installed rows. Ten malformed CLI records
reject, including embedded NUL and an overlong line; final valid records with
and without a newline both work.

Three separately compiled copied-production controls fail the intended native
assertions: Q upper limit99, omitting unsigned-byte Moves storage, and accepting
trailing profile bytes. The intended source and its normal objects remain
unchanged during these controls. Full LLVM15 production analysis (the required
header/CMake scope) passes all 315 translation units with zero failures in
306.80 seconds; the Windows-only network implementation is outside that Linux
scan. Logs and receipts are retained locally under
`/home/jhurliman/src/glsmac-movement-rules/`. Native Windows/macOS and live
gameplay are not claimed here.
