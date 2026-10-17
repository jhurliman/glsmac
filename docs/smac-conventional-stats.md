# Supplied SMAC conventional morale and movement allowance

`SmacConventionalStats` provides two pure queries for the next live Unit
consumer. It owns no game state and supplies no defaults. It does not create a
unit, change stored morale/damage/spent movement, train or promote, draw RNG,
resolve a design, or admit a keyboard/movement command. SMAX, native/owner-zero,
Psi and Probe morale branches and the Artifact-plan movement branch are outside
this contract.

The implementation follows RE-SMAC-SCOUT-STATS-001's conventional paths at
original `0x5d8f50` (morale) and `0x5d9530` (full allowance). The separately
observed basic-design query `0x5d9400` is an explicit input boundary; passing a
raw chassis speed in its place would be incorrect. No current or retained
social rating, faction bonus, project, facility or movement quantum is inferred.

## Query contract

Morale takes the raw stored byte, retained MORALE, direct faction MORALE scalar,
negative-morale immunity, an optional home-base record and mode 0 or 1. An absent
home is distinct from a supplied home with neither Children's Creche nor riots.
The current social rating is deliberately absent from the interface.

1. Clamp retained rating to -4..4 and map it to `-3,-2,-1,-1,0,1,1,2,3`.
2. Add a negative direct faction scalar; halve a negative modifier toward zero
   if the home has Children's Creche; then add a positive direct scalar.
3. Immunity replaces a negative modifier with zero.
4. Mode 1 with a rioting home and no immunity subtracts one and clamps the
   **modifier** to 0..6. Mode 0 skips this step.
5. Add stored morale, then clamp the effective result to 0..6.

Full allowance takes the resolved basic allowance, positive movement quantum Q,
land/sea/air domain, reactor and damage bytes, effective mode-1 morale, resolved
Maritime Control Center ownership and the caller's elite-suppression flag.
Sea Maritime adds 2Q, then effective morale 6 adds Q unless suppressed. Zero
damage and all air-domain requests return that result directly. Otherwise:

```
H = 10 * clamp(reactor, 1, 100)
L = clamp(H - damage, 0, 9999)
K = clamp(H, 1, 99)
U = allowance / Q                  # nonnegative integer division
result = Q * clamp(ceil(L * U / K), sea ? 2 : 1, 999)
```

Elite and Maritime apply before damage. The denominator's separate cap of 99
is intentional. The nonnegative ceiling uses quotient and remainder, avoiding
an unnecessary overflowing `numerator + denominator - 1`. Spent movement does
not affect this query. In particular, diagnostic damage at or above H still
returns the ground/sea minimum; this is not a survival or command-admission test.

## Port admission and errors

The following limits are the public input contract, separate from claims about
which values ordinary gameplay currently produces:

| Input | Admitted domain |
| --- | --- |
| Stored morale | Integer 0..255, including original diagnostic 7/255 cases |
| Retained and direct faction MORALE | Signed32; retained is clamped as above |
| Query mode/domain | Only the named enum members |
| Basic allowance | Nonnegative signed32 |
| Movement quantum | Positive signed32 |
| Reactor/damage | Unsigned byte 0..255; diagnostic reactor tiers remain explicit |
| Effective morale | Integer 0..6 |
| Home/project/immunity/suppression | Explicit booleans/optional home |

Invalid metadata throws `std::invalid_argument`. Every evaluated original
integer sum/product must fit signed32 before a later clamp or cancellation;
otherwise the port throws `std::overflow_error`. This conservative rejection
policy does not reproduce original machine overflow. Products are widened
before evaluation. Unused branches perform no arithmetic: for example Maritime
does not multiply Q on land, and air skips damage scaling. Effective-morale
metadata is still validated even if elite suppression makes its value unused.
The stored byte domain is query compatibility, not permission for a live
training or combat producer to emit morale outside its own admitted range.

## Original comparison and provenance

The opt-in `tools/tests/smac_conventional_stats_probe.cpp` links the actual
production implementation. `tools/reference/compare_smac_conventional_stats.py`
requires the external, hash-pinned public fixture and literal original report.
It does not execute or import the research harness and never regenerates its
outputs. The original evidence is supplied-world routine execution, not live
play. This port comparison does not claim to rerun the proprietary executable.

The comparison projects both modes of all 2,310 morale rows and the full result
of all 4,864 movement rows: **9,484 outputs**. Each movement request explicitly
supplies that row's separately captured basic-design allowance. That output is
not independently reproduced or counted as a comparison. Current rating is
omitted by design; the original fixture contrasts it with retained rating.
Movement fixtures supply neutral morale context, so their stored 1/6 is the
effective input. No allocation, startup composition, RNG or unit mutation from
the larger original receipt is compared.

| Evidence | SHA-256 |
| --- | --- |
| Original executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Original direct-query source | `a220c2dd13aea6086551d1b64e7f560cca0fc8bf83e4164c6a96e8ba75397dac` |
| Original report | `8a4374ff62e161718a637758acb5c587fdb81fca35dbf993b8fdfe73faf49e32` |
| Public input fixture | `bf6b3dffba9c7a031df4a865c1b1f392b3c1a636f8b201e6346d2e7c0b691dc7` |

Example local comparison, with the separately retained files supplied explicitly:

```sh
clang++ -std=c++17 -O2 -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all \
  -Isrc tools/tests/smac_conventional_stats_probe.cpp \
  src/game/backend/unit/SmacConventionalStats.cpp -o /tmp/stats-probe
python3 tools/reference/compare_smac_conventional_stats.py \
  --probe /tmp/stats-probe --fixture /private/scout-creation-morale-and-movement-cases.json \
  --report /private/scout-unit-stats-original-x86-results-20261017.json \
  --output /tmp/stats-comparison.json
```

The bounded text probe accepts nine signed32 tokens per line, at most 511 text
bytes, with exact byte/enum/boolean framing. Malformed framing exits 2; admitted
stream records print `ok N`, `invalid`, or `overflow`. It is an evidence adapter,
not a network or gameplay interface. No original assets or reports are committed.

## Validation

Four native groups independently cover retained mapping, ordering interactions,
diagnostic byte boundaries, domain/quantum errors and branch-sensitive overflow.
The Release+UBSan probe matches all 9,484 selected original outputs. On exact
source parent `5a556ff683f7601ca919c9537745bffdc1d61e94`, normal Linux CTest
passes 8/8 in 50.33 seconds. After a final test-only odd-negative rounding case,
all four focused groups pass with allocation tracking in both normal and
ASan/UBSan/LSan builds, with leak detection and halt-on-error enabled. Full
LLVM 15 correctness analysis covers 312 production translation units with zero
failures in 227.11 seconds; the Windows-only network TU is outside that Linux
scope. No full sanitizer-suite or native Windows/macOS result is claimed here.

Six separately compiled copied-production controls each exit 1 on their
intended assertion: positive scalar before Creche, missing riot-modifier clamp,
missing elite bonus, denominator cap 100, damage scaling of air, and unchecked
damage product. The air control selects the allowance group; applying it to
the unrelated valid-maximum air case also deliberately throws in the old native
test harness, so that initial abort is retained separately. The first positive-
scalar control exposed a missing odd-negative test boundary; adding that case
made the control fail without changing production. Intended source/object files
are never replaced by controls.

The evidence adapter rejects 12 malformed CLI streams (including embedded NUL)
with exit 2 and checks five explicit invalid/overflow stream results. Both
newline-terminated and final unterminated valid records pass. The initial NUL
framing gap was found in peer review and fixed in the adapter before this receipt.
Local logs, copied controls, compiled probe and original comparison remain at
`/home/jhurliman/src/glsmac-conventional-stats/`; original evidence remains outside
Git. The comparison receipt pins the final compiled probe hash.

The next live consumer must retain a single authoritative Unit record and bind
design and simulation-owner identities explicitly. It must obtain retained
social state, direct faction MORALE, home effects and Q from real producers,
then project these query results to the frontend. This change does not invent
those sources or turn the new-world package into an always-base/Infantry rule.
