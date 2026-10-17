# Live simulation identity and technology — 2026-10-17

Integration `39172a7d4df8b79d46224cafb45b1d3970469859` is retained as
`codex/validation-20261017-simulation-39172a7d`. It combines the live simulation
binding from source `7eb1fef3577ac781689db945126c70f423110850` with the earlier
canonical roster, frozen unit profiles, claimed startup/failure correction,
owned Scout image producer and signed32 base-center correction. Small
publication PRs have their own revisions, prerequisites and merge gates.

| Environment | Configuration | Result |
| --- | --- | --- |
| aleph0 native Windows | Clang Debug, full CTest | 8/8, 80.65 seconds |
| aleph0 Ubuntu/WSL | ASan/UBSan/LSan, leak detection and halt-on-error | 8/8, 138.94 seconds |
| honey native macOS arm64 | Apple Clang Debug, full CTest | 8/8, 57.99 seconds |

Linux uses the established local non-PIE sanitizer configuration. No leak or
diagnostic suppressions were added. The full LLVM 15 correctness scan covered
322 production translation units with two workers in 244.59 seconds, with zero
failures. The Windows-only network implementation remains explicitly outside
that Linux analysis.

## Implemented boundary

The [live binding contract](https://github.com/jhurliman/glsmac/blob/39172a7d4df8b79d46224cafb45b1d3970469859/docs/smac-live-simulation-state.md)
records the exact admitted policy. New SMAC worlds allocate simulation IDs once
from occupied canonical slots before create-world callbacks. Explicit saved IDs
and exact mutable technology state survive V9 world transfer without replaying
startup grants. Roster epoch, Game session and Player identity tokens reject
stale or changed controllers; same-occupant refresh and retained reconnect
preserve state. Script queries return detached copies.

The source-specific validation adds 21 tracked groups and five copied-source
negative controls. Actual claimed Start and direct Client packet-dispatch tests
exercise startup, saved non-dense IDs, publication ordering, cancellation,
replacement and malformed-prefix rejection. The map fixture is asset-free;
these are not socket, rendered-network-start or disk-save tests.

## Native Mac provenance

The isolated source checkout is
`/Users/jhurliman/.codex-validation/glsmac-20261017-39172a7d/src`.
Configure, build and tests ran from 10:04:31 to 10:06:24 UTC on October 17,
macOS 26.5 (25F71), native arm64, Ninja and four build workers. The final checkout
status was clean. Existing interactive sessions were not interrupted.

- Binary SHA-256: `3f95f693324b40b377e02ee59f1310b711ebbff9cd09df24263613b8b0f8ea0c`.
- Transfer bundle SHA-256: `629ea21118efda8a105117ecff49b26e7f7b4e07116a76805851eba8ef91871e`.

## Retained evidence and limits

- Windows: `out/simulation-live-windows-{build,ctest}.log`; CTest SHA-256
  `cc604cf4d7911969af6ba869012e4010e0ffe4b0c2b0b4c48df16436bfa24c88`.
- Linux: `/home/jhurliman/src/glsmac-gc-audit/simulation-live-asan-{build,ctest}.log`;
  CTest SHA-256 `834abd3e0c849bdec8fc4c92bdafdce63bf635c212dd54344a5c6314c81b34c5`.
- Mac: the isolated checkout's sibling `logs/`, copied to
  `out/validation-honey-39172a7d/logs/`; CTest SHA-256
  `abfaa894a64cc09e9ffd47b2f66ea7465f69e616d637284d2bed16876afaa747`.
- Analysis: `/home/jhurliman/src/glsmac-gc-audit/simulation-live-analysis/summary.json`;
  SHA-256 `1b8ec679e77c0acb066e8748a4018cad5dd0510bc1492e466b8aa04212b594dd`.
- Evidence manifest: `out/simulation-live-integration-manifest.json`; SHA-256
  `366ec60eee85a1c3e5229ae034eb2121fa68622fc56d2e5ada0c9d7bcea41521`.

The separate `a4dfd992` image checkpoint matches 19 original Scout drawing views
on native Windows Release and macOS Release with UBSan. This receipt does not
claim new pixel comparisons or live Scout actors at `39172a7d`. The user's
successful Mac map/base/turn/restart/close check remains at `5cd78418`, with
untimed restart latency. Conventional startup designs, units, production,
research upkeep, complete saves/replay and multiplayer convergence remain open.
M0, M1 and the complete conquest scenario are not declared complete.
