# Technology, world binding and initial-turn checkpoint — 2026-10-17

The integrated source is `445ee2094331babdab7e82528eb00f028a65ff26`, retained as
`codex/validation-20261017-world-technology-445ee209`. It includes the frozen
89-record SMAC technology catalog, V5 world-header binding, the first-turn
event-order correction and bounded Server departure retirement. This is an
integration checkpoint; its smaller PRs have separate publication trees and
review/check requirements.

| Environment | Configuration | Result |
| --- | --- | --- |
| aleph0 native Windows | Clang Debug, full CTest | 8/8, 41.07 seconds |
| aleph0 Ubuntu/WSL | ASan/UBSan/LSan, leak detection and halt-on-error | 8/8, 64.74 seconds |
| honey native macOS arm64 | Apple Clang Debug, full CTest | 8/8, 32.52 seconds |

The Linux diagnostic build retains the previously documented local non-PIE
sanitizer workaround. No toolchain policy changed. The Mac run was automated
and asset-free; it does not replace the user's earlier interactive validation
at `5cd78418`, measure new-game latency, or establish conventional-unit rendering.
No milestone completion, complete turn simulation, disk-save compatibility or
SMAX gameplay parity is inferred.

## Windows portability correction

The first native integration build at `104f7487` failed because Windows
`wingdi.h` defines `TECHNOLOGY` as a macro. Both catalog enums were renamed to
`TECHNOLOGY_ID` in `77aa41be`, preserving their numeric values and all rule-file
tokens. That earlier integrated revision passed Windows CTest 8/8 in 39.94
seconds before the Server and V5 additions. The failure is retained separately
from the passing build and is not attributed to the final checkpoint.

## Mac run

An isolated checkout at
`/Users/jhurliman/.codex-validation/glsmac-20261017-445ee209/src`
was cloned from the preceding validation checkout and fetched from a local
bundle. Configure/build/tests ran from 07:01:47 through 07:03:14 UTC on October
17 on macOS 26.5 (25F71), native arm64, using Ninja and four build workers.
The source checkout was clean afterward. No interactive game was launched.

Binary SHA-256:
`7fc990950baead73f1c5667797f1d3803d378fe08ec3adab7a1506c46e520399`.
Transfer bundle SHA-256:
`82b55491f78a3e19ef26233f18993396d1de7611939f3ee2b3fa7bcdf5537b2c`.

## Retained evidence

Windows logs are under `out/` in the integration checkout:

- `world-technology-windows-{build,ctest}.log`; CTest SHA-256
  `88ae10580370910d115bac61e3980272909a2ece7ca6cbe80a69b87c5ba2dfca`.
- `technology-startup-windows-ctest.log`; earlier passing CTest SHA-256
  `e0e5da37ac73a12e5fac0e403fd760456e54afac6de5283c273c79f8ad2d9750`.
- `technology-startup-windows-build-macro-failure.log`; SHA-256
  `9652f4ad55debbca1bc29dca6f914cbfcde727971f96292a70e626e4d4f6833b`.

Linux logs are
`/home/jhurliman/src/glsmac-gc-audit/world-technology-asan-{build,ctest}.log`;
CTest SHA-256 is
`d34153a2a864984845018939ad3ec3a1e4b9a34db1eacfd590e09a5125885da5`.
Mac logs are under the isolated checkout's sibling `logs/` directory and copied
to `out/validation-honey-445ee209/`; CTest SHA-256 is
`61d398f235a5057b71d30cab0c6af0b408f85bd521a193ac6e96d527913c0a70`.

The V5 source branch separately checked all 307 Linux production translation
units with LLVM 15.0.7: zero failures in 115.05 seconds. That analysis included
the catalog's shared settings header; it preceded the additional first-turn and
Server changes in this integration checkpoint. Their changed production TUs
have separate focused analysis receipts. See the source-pinned
[V5 receipt](https://github.com/jhurliman/glsmac/blob/445ee2094331babdab7e82528eb00f028a65ff26/docs/world-snapshot-rules.md)
for the exact source and malformed-header controls. Do not describe that scan
as a full analysis of this later combined tree.
