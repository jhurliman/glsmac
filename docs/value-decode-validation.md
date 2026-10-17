# Serialized GSE value validation

The value wire format is unchanged. `Value::Deserialize` now checks the full
signed wire type before dispatch, rejects negative or impossible collection
counts, and requires a registered custom class and a game before decoding a
game reference. These checks remain active in Release builds.

Counts use a conservative remaining-byte bound: each array element needs at
least one byte, and each object property needs a key and value. The decoder
does not reserve storage from the count; it grows collections as framed values
are successfully decoded. Buffer remains responsible for frame type, size,
checksum and truncation checks.

The decoder still reads one value from a stream, so following records remain
valid. A rejected value can consume its prefix. Successfully constructed child
values remain owned by the current GC space if a later child fails; callers
must use the existing accumulation contract. This change does not make a
whole stream transactional, define duplicate-property behavior, impose a depth
limit, or validate the IDs inside each custom class's own reference decoder.

## Validation

Five native tests run in the existing Debug GSE suite, without original game
assets. They cover full-width invalid tags, signed and impossible counts,
custom-class dispatch requirements, nested values including empty collections,
following records, and a failed second child after the first child allocated.

Before the fix, the full native/script run passed 187 tests and failed the
three new metadata tests. The fixed normal CTest suite passed 7/7 (29.37 s).
The full ASan/UBSan CTest suite passed 7/7 (37.81 s), with leak detection
disabled for the separately recorded parser-exception leak baseline.
All five decoder tests also passed with leak detection enabled. That local
run temporarily returned from `AddTests` immediately after `AddValueTests`;
the complete registration was restored and rebuilt afterward. No native-test
filter or additional default CTest invocation was introduced.
The existing correctness policy also passed for `src/gse/Value.cpp` against
both Debug and Release compilation databases; no warning categories changed.

A separate local Release probe linked the actual project objects with a tiny
replacement main. Value and Base were rebuilt with UBSan using the Release
compilation commands, with neither DEBUG nor FASTDEBUG defined. Original code:

- Accepted tag 256 as a null pointer after narrowing it to eight bits.
- Sent a negative array count to `vector::reserve` and a negative object count
  into child parsing instead of rejecting the count.
- Called through a null game when decoding a Base reference (UBSan).
- Crashed while dispatching an unknown custom class (UBSan SEGV).

All five fixed Release probes reject the input with the expected runtime
error and exit successfully. This is focused Release runtime evidence, not a
claim that the entire Release application was run under UBSan.

Local evidence, including the probe source and runner, is retained in
`/home/jhurliman/src/glsmac-value-decode-validation/`: `negative-tests.log`,
`fixed-ctest.log`, `normal-analysis.log`, `release-analysis.log`,
`original-release-*.log`, `fixed-release-*.log`, `asan-ctest.log`, and
`focused-value-leaks.log`.
