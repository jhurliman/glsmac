# Native world-generation bootstrap

`SmacGenerationBootstrap` implements the first missing original generation stage
on owned 44-byte records. It reuses the existing independent SMAC shared/CRT RNG
states and produces the settings, raw plane and continuation consumed by the
continent loop. It does not synthesize native records from Perlin elevations.

This is a staged implementation, not a new playable generator option. The live
`Map::Generate` still uses the legacy Perlin path while later generation and
preparation stages are ported. A partially reset plane is not published as a
gameplay map. Full generated-world equivalence remains required; the
[renderer comparison](generated-terrain-appearance.md) verifies drawing from
authoritative original records, not the port's generation algorithm.

## State and source boundary

Original 0x5e0100 through 0x5e01da resolves settings, calls complete reset 0x5a8450
and prepares the continent continuation. The module owns records, seven signed
categories, both RNG states, working land thresholds, generation flags, three
reset words, resource pattern, topology, budgets/progress and the two distinct
attempt counters. Geometry has already been allocated by the caller; resolving
the size category does not resize that storage.

`ResetRecords` implements the direct reset without changing caller categories,
budgets, topology, flags or working table. It clears the three original reset
words and consumes one shared draw even when there are no records. The pattern
is `1 + ((NextSharedLow16()*32767)>>16)`, producing 1 through 32767. Each record
becomes `20 14 0f 00 00 00` in its first six bytes; **bytes 6 and 7 survive**,
and bytes 8 through 43 become zero. Reusing storage is therefore distinct from
zeroing the whole plane. Fresh allocation policy is a separate caller concern.

`Bootstrap` copies the original eleven-entry land table, sets generation flags,
and scans categories in address order. Each negative value consumes one CRT draw
and becomes `draw % 3`; nonnegative values are retained. The independent shared
pattern draw follows those category draws. No JSF32, clock or implicit seed is
used. Optional draw observation is omitted by ordinary callers.

With record count N, ocean category K and explicit numeric rules B, M and I:

```text
land_target = trunc(i32(i32(B + i32((2-K)*M))*N) / 3200)
continent_minimum = trunc(i32(I*N) / 3200)
```

The implementation defines wrapping DWORD arithmetic with unsigned operations;
signed division truncates toward zero. Standard rules are B=384, M=256, I=36.
Topology becomes round, progress/iteration reset to zero, and the stage becomes
`ContinentLoop`. `Entry::First` resets the continent-attempt count; `Entry::Retry`
preserves it, matching the original jump back to 0x5e010d. The per-attempt
iteration counter is not that carried counter.

## Original evidence and verification

The frozen `generation-bootstrap-evidence-20261020` manifest has SHA256
`416e97fcdde43316dcf9bf1ab5f1cd9d6515de1595d000de154e9ea1351f720f`.
All 12 files were verified. Its independent original checker passes 34 controls,
70,921 records and 45 native RNG calls. The pinned executable hash, inputs,
complete-output hashes and draw/state results are retained in the compact
[numeric fixture](behavior/generation-bootstrap-cases.json); purchased executable
bytes and full original buffers are not committed.

Production tests compare every complete record-plane hash, the preserved-byte
stream hash, settings, working table, reset words, budgets, continuation counters,
both final RNG states and every observed draw in order. They cover empty/dirty
planes, pattern endpoints, negative/mixed settings, allocated sizes, fractional
budgets and retry entry. A composed prefix/retry test verifies patterns 11446
then 17133, independent CRT state and carried attempts without claiming that
continent growth has already executed between them.

The cached incremental build and ten selected bootstrap/RNG/Start groups pass.
The new production TU passes targeted correctness analysis. The same ten groups
pass with ASan/UBSan on the new module, RNG primitive implementation and bootstrap
test TU; other objects are uninstrumented and leak detection is disabled. Source
list/test registration uses the normal CMake build. No platform matrix or UI
launch was repeated for this detached generation stage.

## Next required stages

Continent seeding/growth must consume this exact state, followed by the original
hill/ocean shaping, regions and rejection/retry loop. Later terrain, landmarks,
climate, flora and startup must keep their original ordering and shared stream.
Existing climate/starting-site modules are later components, not replacements
for the missing initial plane. Complete generated startup's observed active mask
is 255, including native life; bootstrap itself neither writes that mask nor
creates factions. Live generated-map publication, actor/UI rendering and wider
platform acceptance remain separate requirements.
