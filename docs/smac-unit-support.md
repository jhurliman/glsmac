# Detached SMAC unit support

`SmacUnitSupport::Recalculate` computes mineral support from an explicitly
ordered, owned vector of semantic unit records. It supplies the support demand
that a later mineral/worker consumer can use; it does not change tile yields,
units, bases, orders, RNG, snapshots or default gameplay.

Only same-faction units homed to the evaluating base enter the calculation.
Supply, Probe and Artifact weapon packages, Clean Reactor, and built-in psi
designs on land/shelf fungus are exempt before counting. Player-designed psi
does not acquire the native exemption. Package means the weapon mode, not the
AI plan. Location ownership is not an input to the support predicate.

The caller supplies the actual unit-array order. IDs identify records; they do
not establish age or sort order. Exempt units consume no free place. The free
allowance is 0/0/1/1/2/3/4/max(4,population) at SUPPORT -4 through +3, with the
lookup clamped at either end. Each later eligible unit costs one mineral, or
two at SUPPORT <= -4. Recalculation returns fresh owned results, so changing
eligibility, population or SUPPORT can redistribute which units pay.

Each returned record is `Ignored`, `Exempt`, `Free` or `Paid`. `Ignored` means
this base must not write that unit's existing paid-support flag. The other
three dispositions replace that flag, and only `Paid` sets it. This distinction
follows the original owner/home filter preceding flag clearing; it does not
implement a live mutation protocol. The output also owns eligible and paid
counts, free allowance, charge per paid unit and total mineral support.

The admitted population is 0..127. The original performs a signed32 SUPPORT+4
lookup addition before clamping; the port rejects the four overflowing positive
inputs rather than claiming original overflow equivalence. Other signed32
ratings are accepted. A record-count ceiling of INT32_MAX/2 bounds every count
and the worst-case two-mineral sum. This is an arithmetic admission limit, not
a claim about the original game's allocation capacity. Invalid enum values
and duplicate identities reject before a result is returned. Prior results
and caller records stay unchanged on failure. IDs are opaque uint64 values,
including zero; no original-to-port identity mapping is invented here.

## Original evidence and owned projection

The human-owned [support and convoy handoff](behavior/unit-support-and-convoys.md)
is RE-SMAC-SUP-001. The retained harness executes original routine `0x4feb20`
and its Clean ability helper. It supplies a same-faction territory result for
the separate pacifism calculation. It does not substitute the support
predicates or arithmetic. No original execution was rerun for this change.

`tools/reference/project_smac_unit_support.py` reads the public semantic
inputs and literal retained outputs. It verifies exact ordered case IDs and
report equality before emitting the compact owned fixture. It imports neither
the original emulator nor a support oracle. Local ordinal IDs retain input
correspondence only. The pins are:

| Input | SHA-256 |
| --- | --- |
| Pinned SMAC executable identity | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| `unit-support-and-convoys-cases.json` | `7b14abc88b0c41393111cb41b5ee6a1da6da18306feae416d11c4ff70087ddec` |
| `tmp/reference/support-original-x86-results-20261016.json` | `b96988121be0a122537ee6d70d972986fc9afe75b6bec04ae65a14a4ac4f440b` |
| `tmp/reference/emulate_support.py` | `06ab5db01e00f6644e264dd90b8cd975d6919635fa32b05512e2edf875a6b3ce` |

The projection retains 28 support outputs: mineral total, eligible count and
ordered paid flags. It also retains the 23 separately reported transitions
that add Clean Reactor to a paid unit: changed total and that unit's cleared
flag. The harness repeated each original support calculation and asserted the
total did not accumulate; it did not record a second complete result object.
All seven convoy and two cancellation cases are excluded.

The native fixture checks those literal outputs and transitions, repeated
port calls, the independently captured native-leaving-fungus pair, and explicit
port controls for reordered IDs, ownership/home changes, changing population,
malformed records and arithmetic boundaries. Trench and extreme integer
controls are static-predicate/admission tests, not extra captured game cases.
The public native-fungus case likewise does not prove legal placement in deep
ocean. No full-game upkeep, convoy orders, capture/homing lifecycle, support
deficit/disband selection or original construction/deletion ordering is claimed.

## Validation

On the intended source based on `dacac744`, all six native groups passed with
allocation tracking in Linux Debug (1.027 s) and ASan/UBSan/LSan with leak
detection and halt-on-error enabled (1.040 s). An optimized `-O3 -DNDEBUG`
production TU linked into the same native harness also passed all six groups.
LLVM 15 analysis of the one new production TU reported zero diagnostics and
zero failures (0.36 s). This is focused validation, not a new full-platform or
whole-suite checkpoint.

Seven separately compiled copied-production controls failed their intended
assertions: changing the -4 cost boundary, counting exemptions, treating custom
psi as native, exempting deep fungus, dropping owner or home filtering, and
sorting records by identity. The intended checkout remained unchanged. The
projector rechecked all 28 outputs and 23 transitions; the generated fixture
hash is `c0909726903cd784447703af67b4b879d63c614803363969a780c45a74d0d1ac`.
The first builds exposed an unqualified `size_t` in the new fixture declaration;
it was corrected to `std::size_t`, with failed logs retained and no production
change. Original reference execution remains the separate receipt above.
