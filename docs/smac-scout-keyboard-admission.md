# Supplied Scout keyboard admission

`SmacScoutKeyboardAdmission` is a pure, owned implementation of the measured
RE-SMAC-SCOUT-KEY-001 ordinary local keyboard gate. It consumes a supplied selected
unit, fresh full allowance, spent movement, focused simulation faction, map
extent, supported keyboard context and pending-action latch. Its result reports
the decoded direction, queried stable identity, notification count, resulting
latch and optional request. It does not borrow a live unit or execute a request.

The two measured key codes are 99 and 105, producing original direction codes 2
and 0 respectively. These names preserve the captured numeric interface without
inventing a geographic direction mapping. An emitted request carries the exact
selected stable identity and original third argument 0. Identity 0 and high-bit
identities are valid; absence is represented by `optional`, never a sentinel ID.

## Effects and admitted domain

| Supplied selection | Result | Actionability query | Notification | Latch |
| --- | --- | --- | --- | --- |
| Absent | `NoSelection` | None | None | Preserved |
| Foreign, out of map, or spent >= fresh allowance | `CannotAct` | Selected identity | Once | Cleared |
| Owned, in map, spent < fresh allowance | `EmitRequest` | Selected identity | None | Preserved |

The comparison is direct and cannot overflow through subtraction. One or two
remaining thirds admit a request; the gate does not require a complete movement
point. After combat, callers must supply the survivor's freshly resolved full
allowance, including changed damage and morale. Admission itself neither spends
movement nor mutates either random stream.

The supported context is normal local unit control with no queued-direction
modifier, network-busy state or scenario editor. Focused and selected owners are
explicit simulation IDs 1..7, not transport slot indices. Selected spent movement
is 0..255 and supplied full allowance is a nonnegative signed32 value. Width is
even and at least 2; height is positive. Out-of-map coordinates produce the
observed `CannotAct` effect, while unproved in-map parity is rejected explicitly.
Only `Status::Ordinary` is admitted. The other enum values describe unsupported
categories; they are not a mapping of original status-word bits.

Unsupported context, status, key or numeric domains throw `invalid_argument`
before producing effects, even when no unit is selected. These are deliberate
port admission restrictions, not claims that the original rejects every such
input. The wider nonnegative allowance and latch ranges have arithmetic/state
boundary tests; original keyboard captures use the ordinary Scout inputs below.

This component contains no RNG input, clock seed, edge-cost producer, keyboard
event hookup, live Unit/Game state or serialization. It does not admit mouse,
goto, network, transport, special-order or general movement commands. Request
emission is distinct from movement success, combat outcome and original combat
wrapper-return evidence.

## Retained original comparison

The human-owned behavior document and case JSON remain read-only. The native
fixture retains all 300 original command cases: 288 combinations of owned
selected indices 0/2, morale 1/6, damage 0/4/5/9, spent 0..8 and keys 99/105,
plus 12 absent, foreign and out-of-map controls. It compares emitted arguments,
query identity/predicate, decoded direction, notification and latch effects.

The captured three complete battle-to-next-key histories are also retained.
Tests call the existing pure combat model, verify the recorded survivor and
selection, resolve fresh morale/allowance with the existing stats producer, then
run this gate. The ordinary damaged survivor is exhausted; the elite undamaged
survivor has another full point; the initially partial ordinary attacker remains
overspent. Both RNG words are unchanged by the keyboard stage. These are port
composition checks against retained original observations, not another execution
of the original command or full game.

Original mutable array indices map through an explicit fixture table to stable
IDs (`UINT64_MAX`, 0 and a high-bit ID). The keyboard stage does not compact
arrays, replace a cleared selection or infer continuation selection.

| Input provenance | SHA256 |
| --- | --- |
| `scout-keyboard-command-admission-cases.json` | `c2d7c06214d95d012377ea5e9012b20125b48667dea109551bc9a124339a04f3` |
| Retained original result report | `a4681e3b0c8a750a84f76647e13c46af3ee6317c12d2d5012a7137c80e25a3f1` |
| Original private harness | `f1ce288af2fc4c41238d0a7f2c4ea084c9d669b2f4301593fd6fcdfa8e259600` |
| Original executable identity | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |

The local converter verifies those fixture/report/harness hashes, all 300 exact
case outputs and the three sequence records before emitting semantic literals.
No original executable or asset bytes are committed. The original executable
was not rerun for this port comparison.

## Validation

Source base is main `0b8cad8cc93d9ddd54d77c715ac3b78f4474a116`. Production is
independent of the edge-cost and empty-movement implementations; the sequence
tests reuse the already-landed combat, conventional stats and RNG components.
Five groups join the existing native suite:

```sh
build/bin/GLSMAC --gse-tests --memorydebug \
  --gse-tests-native-filter '^SMAC Scout key admission ' \
  --prefix build/scout-key-test-data
```

Ubuntu/Clang15 Debug passes all eight existing CTests in 56.28 seconds, five
focused tracked groups in 1.03 seconds and one-TU LLVM15 correctness analysis
with zero failures. The same five groups pass ASan/UBSan/LSan with tracking in
1.07 seconds, leak detection and halt-on-error enabled. The local sanitizer link
uses the established non-PIE workaround; repository policy is unchanged.

An optimized `-O3 -DNDEBUG` build of the actual production TU, linked into the
native harness, passes all five groups. This is a Release-TU check, not a claim
that the complete executable was rebuilt in Release. Seven separately copied
production mutations fail at their intended assertions: admitting exhaustion,
refusing partial points, clearing the latch without denial, admitting a foreign
owner, losing the selected identity, reversing direction and accepting unproved
status. They fail 3/2/3/2/2/1/1 groups respectively; intended source is never
mutated. Additional direct tests cover identity lifetime, supplied-state
nonmutation, extreme allowance/latch values and 18 unsupported-domain cases.

Commands, conversion, manifests and logs are retained locally under
`/home/jhurliman/src/glsmac-scout-keyboard-admission/`. These are source/Linux
receipts; publication, hosted and other-platform checks are separate.
