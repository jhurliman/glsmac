# SMAC base-square yield calculator

`resource::smac::CalculateBaseSquareYield` implements the arithmetic contract
of RE-SMAC-BASE-001 for a valid founded center. Its output is signed nutrients,
minerals and energy before consumption, support, commerce or allocation.
This component is compiled into the backend but is **not yet activated by the
normal game resource callback**. It does not change SMAX behavior.

Every caller supplies all inputs. There are no default rules triples, neutral
economy assumptions, inferred facilities, or implicit event state. The three
rules rows are Base Square, Bonus Square and Recycling Tanks. Resource type,
one landmark point per resource and solar-event activation are already-resolved
semantic inputs. The calculator does not generate maps, resolve resource
patterns or landmark geometry, count workers, or schedule events.

The operation order includes nutrient-resource doubling before other additions,
one recycling bonus for either or both recycling facilities, the one-base HQ
exception at economy -1, and the original +3/+4 economy thresholds. The latter
differ from installed economy descriptions and still need full-game observation.
Energy is clamped to 0..99 before local events, atmospheric dust and solar
tripling. Nutrients and minerals have no corresponding clamp. Local decreases
test nonzero, including negative custom yields.

## Arithmetic domain

Rules and economy are decoded signed 32-bit integers; base count must be
positive. RE-SMAC-BASE-INTEGER-001 establishes signed32 wrapping for the direct
center returns. Each addition, doubling, increment and decrement wraps before
later sign tests or clamps. Golden Age wraps the effective economy before
branch selection; energy clamps the wrapped total to 0..99 before local events,
dust and solar multiplication. Nutrients and minerals retain signed results.

The implementation uses signed64 intermediates and defined unsigned modulo
conversion, then explicitly reconstructs the signed32 result. It invokes no
C++ signed overflow or out-of-range unsigned-to-signed cast. The final values
widen into the backend resource-value type. An invalid resource enum or zero
base count still raises `std::invalid_argument`, including in release builds.
The function has no state changes or external dependencies.

## Evidence and tests

The test-owned table `src/gse/tests/fixtures/SmacBaseSquareYields.h` retains all
57 semantic `BASE-X86-*` cases from the behavioral handoff, its content hash,
and these reference identities:

- SMAC `terran.exe`: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- `alpha.txt`: `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.

Research status: **original-x86 checked using synthetic inputs; not full-game
observed**. The installed rules file was locally hash-checked and its relevant
default rows were verified as `(2,1,1)`, `(2,2,2)` and `(1,1,1)`. These values
appear in fixtures; the production calculator requires supplied rules.

The native GSE/CTest harness calls the production calculator for all 57 cases
and the 25 named signed32 boundary cases in `SmacBaseSquareInteger.h`. Additional
tests retain invalid-context rejection and representable endpoints. The new
boundary expectations come from complete original center routines with supplied
synthetic inputs, not full-game observations. Extreme text parsing, production
of extreme social ratings, whole-base overflow and ordinary-play reachability
remain outside this contract. CI needs no original executable or assets.

The [host-rules integration](smac-host-rules.md) now ingests the SMAC triples
and synchronizes them through global settings. Live calculation still requires
an adapter supplying authoritative owner, economy, facilities, eligible terrain
bonuses and event state. Passing this suite establishes the bounded calculation
contract, not complete base income, original map behavior or SMAX parity.

### Signed32 validation — 2026-10-17

The opt-in batch probe accepts all 31 semantic input fields explicitly. The
comparison tool verifies fixture SHA-256
`c92671c1dbd7fe16d403109bf78d801d07e34363284ee684de212ecea032c059`,
merges its shared defaults with each case's overrides (arrays replace), and
compares literal expected triples:

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -fsanitize=undefined \
  -fno-sanitize-recover=all -Isrc tools/tests/smac_base_square_probe.cpp \
  src/game/backend/resource/SmacBaseSquareYield.cpp -o /tmp/center-probe
python3 tools/reference/compare_base_square_integer.py --probe /tmp/center-probe \
  --cases /private/base-square-integer-boundaries-cases.json \
  --output /tmp/center-comparison.json
```

At implementation `ecb625b8bd70ab1d4c61d3d7ae027012346f7f18`, the Release/UBSan
probe matched all 1,389 scenarios (4,167 returned values). A separately compiled
copy returning unbounded intermediates instead of wrapping disagreed on exactly
422 scenarios and exited nonzero; the intended source/binary was unchanged.
Six malformed probe field/framing controls rejected without emitting a result.
The actual publication suite passed 8/8 in 48.50 seconds. Full LLVM15 analysis
checked all 300 Linux production translation units with zero failures in
223.54 seconds at two workers; Windows-only networking is outside that scan.
Logs and comparison results are retained under
`/home/jhurliman/src/glsmac-base-square-integer`, with publication CTest logs in
`/home/jhurliman/src/glsmac-pr95-refreshed/base-square-integer-ctest.log`.

The broader integration at `c85982ad923847854a1ca3f1ca85ed07ca3d5eb2` passed
Windows CTest 8/8 in 79.85 seconds and Linux ASan/UBSan/LSan CTest 8/8 in
129.93 seconds with leak detection and halt-on-error enabled. These are separate
integration results, not additional runs of the smaller publication branch.
