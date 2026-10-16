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
positive. Each ordered arithmetic intermediate must remain representable as a
signed 32-bit integer. Calculations use signed 64-bit intermediates, and reject
out-of-domain operations with `std::overflow_error` before any C++ overflow can
occur. An invalid resource enum or zero base count raises
`std::invalid_argument`. These checks also run in release builds.

This explicitly excludes unverified original overflow/wrapping behavior rather
than inventing a compatibility rule. Representable endpoints are permitted;
the wider signed output also fits the backend resource-value contract. The
function has no state changes or external dependencies.

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

The existing native GSE/CTest harness calls the production calculator for each
fixture. Additional tests check invalid contexts, rejected arithmetic overflow,
and representable boundaries; these extra safety cases are port-domain tests,
not new original-game observations. CI needs no original executable or assets.

Live integration still requires active rules ingestion/synchronization and an
adapter supplying authoritative owner, economy, facilities, eligible terrain
bonuses and event state. Passing this suite establishes the bounded calculation
contract, not complete base income, original map behavior or SMAX parity.
