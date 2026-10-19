# XF index252: initialized stock behavior and limits

Stock SMAC/SMAX, 2026-10-18. Follow-up to [indexed compositing](scout-effect-indexed-compositing.md). [Numeric cases](scout-effect-index252-cases.json); [private execution/source map](../re-smac-scout-effect-index252.md).

**Index252 is not transparent in the measured original renderer.** It reads beyond the generated explosion CLUT into a global UI bitmap's header/palette storage. The measured SMAC software startup produces a repeatable mapping, rather than arbitrary allocator bytes. It is an accidental memory alias, not an intended extra opacity row.

For the measured SMAC initialized state, source252 over destination index `d` outputs:

```
1                       if d == 13
(d - 20) / 4            if 20 <= d <= 252 and d % 4 == 0
0                       otherwise
```

The division is exact in that branch. These are output palette indices, not alpha values. For example:

| Background index | SMAC output | SMAX constructor-state output |
| --- | --- | --- |
| 0 | 0 | 122 |
| 13 | 1 | 0 |
| 24 | 1 | 128 |
| 44 | 6 | 133 |
| 100 | 20 | 147 |
| 200 | 45 | 172 |
| 245 | 0 | 0 |
| 252 | 58 | 185 |
| 255 | 0 | 0 |

The SMAX column is the actual owning bitmap constructor plus original one-pixel compositor in a fresh image, **not a completed SMAX game startup**. Its row lies farther into another bitmap's default palette: output `122 + d/4` when `d % 4 == 0`, otherwise0. The two executables therefore must not share one assumed escape-row mapping.

## What repeatability is established

Three fresh SMAC emulation contexts execute the complete original C++ constructor batch, software palette initialization, all107 model loads and full asset/data setup. Only constructor writes touch the escaped row; later setup preserves it. One run moves the supplied heap start by64KiB. Row bytes match across all three contexts. Two of those contexts additionally execute32 original XF-frame blits each, covering every first-family frame over backgrounds44 and200; all420 escaped pixel reads per context return6 or45 respectively, with identical outputs across the heap variation. These64 frame controls isolate the escaped lookup; normal explosion banks have not yet been initialized at that setup boundary, so their whole-frame hashes are not full-color combat acceptance evidence.

A separate original initialization control executes the palette decoder, Gamma100 path and complete explosion-lookup initializer. Its ordinary death bank matches the previously verified generator/cache hash, and it leaves the escaped row unchanged. The original compositor then confirms the mapping for **all256 background indices**. No lookup result is stubbed. Software backing, file/GDI services and quiet event handling remain supplied.

This establishes deterministic initialized **software-harness** behavior and independence from the tested heap displacement. It does not establish repeated real OS launches, full WinMain/world/command lifetime, hardware/DirectDraw mode, other display configurations or all later uses of the aliased UI bitmap. No stock game process was running for direct live-memory sampling. The broad research goal remains paused.

## Why this cannot be a universal hardcoded CLUT

The escaped SMAC row begins in bitmap header fields and continues into default palette bytes. Actual allocation of a100×150 backing for that owning bitmap changes the first four escaped bytes to the little-endian size15000. Executed diagnostic controls consequently change background0's lookup0→152 and background1's0→58. This is an original allocation method called diagnostically; normal gameplay admission of that allocation was not established. The tested relocation of unrelated heap allocations does not change the row.

SMAX's relevant constructor completes in isolation and its one-pixel compositor matches the different default row in two fresh storage layouts. An attempted full SMAX constructor batch stopped on an uninitialized UI path before completion. Later SMAX asset/display state remains unresolved; do not promote the constructor column to full runtime parity.

Treating252 as transparent is therefore a deliberate port fallback, not exact measured stock behavior. A profile-specific compatibility mapping can reproduce the measured default SMAC state without reading unrelated memory. Any claim of universal stock fidelity still needs a live startup/display-and-command receipt showing the owning bitmap's lifetime. No production changes were made.
