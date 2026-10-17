# Supplied SMAC gameplay random states

`game::backend::SmacGameplayRandomState` is an owned, inactive value component.
Its constructor requires both uint32 states. It does not choose a seed, read a
clock, select a stream for a caller, or change the existing map JSF32 generator.
There is no Game, Unit, settings, network, or world-snapshot integration.

The two primitive advances are independent:

| Stream | Next state, modulo 2^32 | Returned primitive |
| --- | --- | --- |
| Shared | `1664525 * state + 1013904223` | low 16 bits |
| CRT | `214013 * state + 2531011` | `(state >> 16) & 32767` |

The multiplication and addition use uint64 before defined unsigned narrowing.
Both full-width uint32 domains are admitted, including zero and UINT32_MAX.
`Shared()` and `Crt()` snapshot without advancing. `ExchangeShared()` writes the
supplied word and returns the prior shared word; `SetCrt()` writes only CRT.
Neither operation draws from either stream. Interval reduction, movement-stream
selection, and suppression of combat draws for width <= 1 belong to callers.

## Port record

`Serialize()` returns an owned, fixed 16-byte record:

| Byte offset | Field |
| --- | --- |
| 0 | ASCII `SMGR` |
| 4 | uint32 little-endian version 1 |
| 8 | uint32 little-endian shared state |
| 12 | uint32 little-endian CRT state |

`Deserialize()` rejects a wrong extent, marker, or version with
`std::invalid_argument`, before reading state fields. Every state bit is
preserved. It returns a new value and neither seeds nor advances it. This is a
port format, not an original save-file format or proof of original load timing.

## Original evidence and fixture scope

The human-owned `docs/behavior/gameplay-rng-state-seams.md` handoff is
RE-SMAC-RNG-SEAMS-001. Its case JSON SHA256 is
`ea87ebf6d67d6936eadbd5abae8b448db8b923b352f845a8a8349e3a336556f6`.
That investigation records 28 original-x86 cases with supplied thread-data,
logging, registration, and clock services. This implementation did not rerun
the original executable or change those research files.

The test-owned semantic records in
`src/gse/tests/fixtures/SmacGameplayRandomState.h` compare all 24 direct
set/exchange/draw/restore/replay cases. Shared draws use the fixture caller's
interval 0..8 conversion; CRT draws compare raw results. The selector recorded
in each fixture is provenance only: direct primitives do not select a stream.
The remaining four records compare only the effects of explicitly supplied
setter calls. They do not execute module registration, shutdown registration,
the clock service, or a new-game initialization controller.

No complete new-game seed producer, multiplayer reseeding, original save/load
lifecycle, or SMAX equivalence is established. A later combat model must receive
both states explicitly and retain its own caller draw ordering.

## Validation

Source base: main `98dc39438621d62a3178067502953ca928dd7b87`. The existing native
harness registers five small groups covering original replay records, explicit
writes, independent wire bytes, full-width words, copy/restore ownership,
interleaved continuation, every truncated prefix, trailing bytes, and invalid
markers/versions. They run in the existing full GSE test; no extra default
CTest invocation was added.

```sh
build/bin/GLSMAC --gse-tests --memorydebug \
  --gse-tests-native-filter '^SMAC gameplay RNG ' --prefix build/rng-test-data
```

Local Ubuntu/Clang 15 Debug validation passed all eight existing CTests in
49.28 seconds and all five focused tracked groups in 1.11 seconds. The same
five groups passed with ASan, UBSan, LSan, and allocation tracking in 1.09 seconds
(`detect_leaks=1`, halt on sanitizer error). The sanitizer executable used the
existing local non-PIE linker workaround; no build policy was changed. Focused
LLVM 15 correctness analysis of the production TU reported zero failures.

A separate `-O3 -DNDEBUG -Wall -Wextra -Werror` build of the real production TU
was called through a temporary native bridge. It compared the research JSON
directly: all 24 replay records and four reduced setter-effect records matched.
An independent Python integer oracle also checked 65,536 primitive advances
from 4,096 supplied pairs, exact wire bytes for those pairs, and 25 malformed
records. This is arithmetic/record coverage, not additional original execution.

Five copied-production-TU controls, linked into the real native test executable,
each exited 1: shared high bits instead of low bits (one failed group), CRT
16-bit instead of 15-bit output (one), CRT setter changing shared state (one),
swapped decoded state words (two), and omitted version admission (one). The
intended production source was never temporarily changed for these controls.

Local commands, copied controls, bridge source, logs, and results are retained
under `/home/jhurliman/src/glsmac-gameplay-rng-state/`. These receipts cover Linux
source validation; they do not claim hosted CI or native Windows/macOS runs.
