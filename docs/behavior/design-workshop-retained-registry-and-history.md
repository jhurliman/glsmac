# Workshop retained design records, retirement history and intrinsic overlays

2026-10-20. Original `terran.exe`, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This supplements [occupied-slot editing](design-workshop-custom-edit.md),
[general construction](design-workshop-general-construction.md) and
[production-cache lifetime](production-reference-base-lifecycle.md).
No production edits or desktop interaction.

Additive private freeze:
`tmp/reference/independent-workshop-registry-authority-evidence-20261020/`;
manifest SHA-256
`ce73e8db32e0eeeb0e420d10b644ddd70be4603947e74edce61718d62d98d7ee`.
It contains **18 new native controls**, full input/output regions, harness
sources, and private disassembly with instruction addresses and opcode bytes.
`reused-evidence.json` pins the existing immutable construction, allocation,
installed-loader and turn-zero prefix evidence. Native outputs are expectations;
poisoned storage and constructor arguments are declared test inputs.

## Retained raw-record authority

Keep all 512 records of 52 bytes at `0x9b01e8`, including inactive records and
the shared intrinsic bank `0..63`. Owner custom banks begin at `64 * owner`.
Three fields need distinct producer rules:

| Offset, decimal | Authority and writes |
| --- | --- |
| 43, unsigned byte | Automatic category from `5bd4f0`; `597390` subsequently replaces it when its category argument is nonnegative. Argument 10 additionally forces plan byte 42 to 3. |
| 47, opaque byte | Preserved by the checked builder, prototype constructor and installed loader. Preserve the destination slot's value; zero-origin storage naturally supplies zero. No meaning is assigned here. |
| 50, signed 16-bit prerequisite | `59dfc0` writes the installed row's resolved prerequisite before building it. Ordinary builder/prototype construction preserves the destination value. |

The installed loader initializes active flags to 1, exclusion byte 44 to zero
and claim byte 45 to 255. Its configured plan override affects byte 42 without
overriding automatic category 43. It preserves byte 47 and unused records.
See [installed designs](independent-startup-installed-designs.md): two complete
poisoned/zero pool controls and ordered stores already establish this.

The [fresh constructor](independent-startup-fresh-design.md) has 36 prior
complete controls, including destination byte 47 `A5` and prerequisite `1234`.
Startup callers can override category to 2 or 4 after automatic construction.
These retained fields must not be reconstructed solely from the component tuple.

Three new complete `597390` controls supply Infantry/Laser/Synthmetal/Fission,
mask zero, with deliberately different source and destination fields:

| Owner | Category argument | Returned slot | Output category | Output byte 47 | Output prerequisite |
| --- | --- | --- | --- | --- | --- |
| 2 | -2 | 128 | 3, computed | A5 | 1234 |
| 2 | 21 | 128 | 21, overridden | A5 | 1234 |
| 7 | -2 | 448 | 3, computed | A5 | 1234 |

Source record 65 has category `31`, byte 47 `B6`, prerequisite -7 and name
`Source Laser`. Its name and component tuple are supplied to the constructor;
its other fields are not copied. Full Design/Faction/Base/Unit/history regions
are retained in `native-prototypes-and-overlays.json.z`.

### Riot-copy caller

At `50a927..50a96b`, the riot transfer path calls
`597390(recipient, chassis, weapon, armor, mask, reactor, -2, sourceName)`
only for source IDs **greater than 64**. ID 64 bypasses this constructor.
The call receives no source category, opaque byte or prerequisite.
Argument -2 enables prototype-complete flag `0x04`; it does not override
category. Category is recomputed for a newly constructed destination, while
byte 47 and prerequisite retain the destination slot's prior values.
Existing-match and allocation/retirement behavior remains the earlier contract.

`50a973..50a977` replaces the Unit design ID only for a nonnegative result.
A negative result leaves the prior design ID while the transfer path has
already changed the Unit owner. The new controls execute the complete
constructor with this caller tuple/mode; the riot argument assembly itself is
source-traced, rather than a new complete riot-body run.

## Retirement tuple history

History base `0x9af1e8` contains eight owners × 64 slots × eight bytes.
For owner `f` and slot `s`, address is `base + (64*f+s)*8`.
Each slot is exactly a little-endian ability-mask DWORD followed by chassis,
weapon, armor and reactor bytes. Names, design IDs, category, opaque byte and
prerequisite are absent.

The next-write cursor is a signed DWORD at `0x95f224 + owner*0x20bc`,
relative offset `0x207c` within that owner's Faction record at `0x95d1a8`.

`594210(owner, design)` reads the old cursor, advances it using signed
remainder `(old+1)%64`, and writes the design's old component tuple into the
slot addressed by the **old, unmasked cursor**. For valid cursors `0..63`,
63 wraps to zero and only the addressed eight bytes change. There is no
separate valid-count or full flag. Invalid cursors are outside the supported
state: the native writer does not normalize the index before writing.

Retirement `595ac0` calls this writer before Unit/queue surgery and rebuilding.
The earlier full allocation matrices distinguish retirement from silent no-Unit
in-place rebuilds, which do not append history.

`5942a0(owner,chassis,weapon,armor,mask,reactor)` scans **all 64 slots** for an
exact tuple, without consulting the cursor. It returns 1 on a match only when
the owner's bit in human mask `0x959ff0` is set. AI returns zero even if the
same tuple is present. `597390` checks it before allocation and returns -1
when blocked. Thus untouched old slots remain behaviorally significant.

New native helper controls cover clearing owners 1/7 with cursors 17/63;
appending at cursors 0/63 for owners 1/7; and finding slot 3 with cursor 42,
AI bypass, and reactor mismatch. Every control compares all 4,096 history
bytes and all eight cursors, including unchanged regions.

### Initialization, restart and explicit reset

`5941e0(owner)` fills that owner's 512 history bytes with `FF`. It **does not
reset the cursor**. Turn-zero `5c8650` faction initialization separately does
both: call `5941e0` and zero the ten DWORDs beginning at faction-relative
address `0x95f214`, a range including the cursor.

At `5c89be`, turn comes from `0x959fdc`. Nonzero turn takes the branch at
`5c89ef` directly to `5c8da5`, skipping both history initialization and cursor
zeroing. **A nonzero-turn faction restart retains untouched history and its
next-write cursor.** Later production-cache clearing is a separate operation.
Two new bounded gate controls, owners 1/7 and turn 42, execute precisely this
branch and preserve every history byte and cursor. They do not claim a new
whole faction-initializer run. The existing 35 full turn-zero prefix controls
cover initialization from poisoned faction/history regions.

The separate 512-byte `5941e0` scratch region in the earlier
[first-site document](independent-startup-first-site.md) is this retirement
tuple history. Its scratch/file naming does not imply technology ownership.
That reference's zero initialization is admitted for turn zero only.

The Workshop debug reset caller `442600`, call at `44287a`, invokes the same
history clear helper for its selected owner(s). It does not reset the cursor.
Legacy load below version 6 clears all eight histories with the same helper;
that block also has no cursor reset of its own.

## Mutable intrinsic overlays

`441e20` can match an active intrinsic design even while that owner's bit is
set in exclusion byte 44. Keep its flags and per-owner exclusion bits as
mutable global-record state, alongside the intrinsic definitions.

For an exact tuple match whose ID differs from the currently selected ID:

1. If byte 44 contains `1 << owner`, OR low flags byte 48 with `0x08`.
2. Clear only that owner's bit in byte 44, preserving every other owner bit.
3. Preserve all other flag bits and intrinsic record fields, including name,
   category, opaque byte and prerequisite.

If the matched ID is **already selected**, `441fd6` jumps over both overlay
writes. Even an excluded currently selected intrinsic match remains excluded.
If its exclusion bit was already clear, switching clears no additional bits
and does not add flag `0x08`.

Four complete `441e20` controls verify the entire pool and unchanged
Faction/Base/Unit/history regions: owners 1/2; exclusion masks `86` and `84`;
flags `0205`; and already-selected versus switching to intrinsic Scout ID 2.
For example owner 1 switching from a custom design with exclusion `86` yields
exclusion `84` and flags `020D`. Already-selected Scout retains `86`/`0205`.
The intrinsic-name warning/preservation branch follows these overlay writes.

## Serialization authority

The ordinary serializer `5be250` reads/writes all `0x6800` design bytes for
version >=3 and all `0x1000` history bytes for version >=6. All cursor DWORDs
are inside the ordinary 67,040-byte Faction block. Preserve raw records,
intrinsic overlays, all ring slots and cursors on save/load; do not rerun the
installed loader or rebuild these fields from active designs.

One new complete save `5c0340` / read `5c05e0(name,0)` control uses the
established memory-backed FILE/allocator/TLS services, no real save file.
It fills the complete pool and history with declared patterns, assigns all
eight cursors `0,9,18,27,36,45,54,63`, saves, poisons all those bytes, then
loads. Every byte and cursor is restored, both functions return zero and
there are no RNG calls. Full frontend resume/owner upkeep is outside this test.

For version <3, the legacy path transfers 32 records per owner and explicitly
clears only name byte 0, flags word 48 and category byte 43 in upper slots
32..63. It does not clear byte 47/prerequisite there. For version <6, history
is cleared to `FF`; cursor authority remains in the Faction load/migration
path. These legacy branches are source-traced, not new full legacy-file runs.

## Instruction provenance

The private freeze includes complete indexed function disassembly with opcode
bytes for every function below, plus save/read entries. This table identifies
the decisive instruction ranges; all addresses are from the pinned SMAC image.

| Behavior | Function / decisive addresses |
| --- | --- |
| Automatic category and preserved fields | `5bd4f0`, reused 40 whole-pool builder controls |
| Installed prerequisite writer | `59dfc0`, store at `59e09e`; reused two full loader controls |
| Constructor mode/category | `597390`: builder call `597aee`, -2 flag `597afd..597b07`, difficulty flag `597b0e..597b17`, fresh/constructed flags `597b1e..597b29`, category/plan override `597b30..597b3f` |
| Riot constructor arguments / failure retention | `50a210`: `50a927..50a977` |
| Clear exactly owner history | `5941e0`, complete function |
| Cursor and old-slot append | `594210`: cursor read `59422b`, advance/store `594232..594242`, tuple stores `59425a..594290` |
| Human gate / whole-ring lookup | `5942a0`: human gate `5942b7..5942b9`, compare/scan `5942d1..594308` |
| Retirement producer call | `595ac0`, complete function |
| Turn-zero versus restart | `5c8650`: `5c89bc..5c8da5`, clear call `5c89fc`, cursor-containing zero range `5c8d5e..5c8d6e` |
| Workshop debug history clear | `442600`, call `44287a` |
| Intrinsic reactivation | `441e20`: selected-ID skip `441fd0..441fd6`, flag OR `441ff8`, exclusion store `442013`, intrinsic branch `442019..442022` |
| Full design pool serialization | `5be250`: version gate preceding `5bebb4`, size/pointer and read/write `5bebbb..5bebf2` |
| Legacy upper-slot fields | `5be250`: `5bec5c..5bec86` |
| Full history / legacy clear | `5be250`: version gate `5bec88`, transfer `5beca4..5becdb`, legacy clear loop `5becef..5becfe` |

The opcode/source inventory establishes concrete named producers and bulk
serialization. It is not a claim that opaque byte 47 can never change through
any whole-record copy or other unexamined path.
