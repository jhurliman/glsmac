# Frozen unit profiles and complete base screens — 2026-10-17

Integration `58197e643a79f1342a693189f08625e94948fa32` is retained as
`codex/validation-20261017-world-units-58197e64`. It combines GlobalSettings V7
and world header V8 predefined-unit profiles, canonical live roster publication,
ready-lobby handshake ordering, complete base-screen endpoint presentation,
and the single-thread duplicate-registration guard. Publication PRs have their
own revisions, prerequisites and review gates.

| Environment | Configuration | Result |
| --- | --- | --- |
| aleph0 native Windows | Clang Debug, full CTest | 8/8, 79.23 seconds |
| aleph0 Ubuntu/WSL | ASan/UBSan/LSan, leak detection and halt-on-error | 8/8, 138.16 seconds |
| honey native macOS arm64 | Apple Clang Debug, full CTest | 8/8, 57.36 seconds |

The Linux sanitizer build uses the established local non-PIE workaround.
The full Linux correctness scan covered 313 production translation units with
two workers in 238.63 seconds and reported zero failures. The Windows-only
network implementation remains explicitly outside that analysis.

## Combined boundaries

The host freezes bounded raw and derived predefined-unit records. Both the
settings receiver and world-header validator rebuild and compare those records;
the world prefix is checked before replacement map allocation. Canonical roster
clients also enforce the complete settings envelope's packet budget. Four actual
Client cases cover initial settings and refresh at the exact cap and one byte
over it. Removing only this new admission check makes its regression fail.

The complete base-screen fixture exercises all ten sections with real Base,
Tile and default resource callbacks, substituting asset/DOM presentation only.
It covers nutrient and energy endpoint balances through display helpers while
leaving simulation overflow checked. This does not replace the existing
placeholder energy allocation with original-game economics.

The ready-lobby handshake now clears existing ready flags before adding the
joining slot, so a new Client does not receive slot flags before its initial
roster. Reset callbacks and running reconnects have separate real packet cases.
The Thread registration guard preserves one successful Start per Stop after
early-stop nomination. Source-specific tests and negative controls are recorded
in the corresponding implementation documents.

## Native Mac provenance

The isolated source checkout is
`/Users/jhurliman/.codex-validation/glsmac-20261017-58197e64/src`.
Configure, build and tests ran from 08:56:50 to 08:58:40 UTC on October 17 on
macOS 26.5 (25F71), native arm64, with Ninja and four build workers. A subsequent
status check was clean. Existing interactive sessions were not interrupted.

- Binary SHA-256: `1efdfb84b29c9cd9fe01c3b0406281c785420a21cc8155d59c0c04effd6d1f41`.
- Transfer bundle SHA-256: `413295d0a2bf0fef638b3f090b8548c1d2df25e72fb75c8fee1984ee776a805d`.

## Retained evidence and limits

- Windows: `out/world-units-windows-{build,ctest}.log`; CTest SHA-256
  `24e6ca08e51d5b5ad819924e6cab99dd5aa3161414908569e928c2e4f924c5d4`.
- Linux: `/home/jhurliman/src/glsmac-gc-audit/world-units-asan-{build,ctest}.log`;
  CTest SHA-256 `de8af682aef83584d6b735eeb131e76629bc8fb5475ecf6bb17a61007dee13c7`.
- Mac: the isolated checkout's sibling `logs/`, copied to
  `out/validation-honey-58197e64/`; CTest SHA-256
  `c1f1f14d15e85d7618f4749e77c5e6fc2f9b320c9b7cef3d6f42310e1dd39429`.
- Analysis: `/home/jhurliman/src/glsmac-gc-audit/world-units-analysis/summary.json`;
  SHA-256 `10f7e81aa6b0e0e82f8147e3590da6aab4ca366950cfcb024d3ef1848b777ecc`.
- Copied Client settings-admission control:
  `/home/jhurliman/src/glsmac-gc-audit/roster-settings-budget-control/`;
  manifest SHA-256 `9bc4d45312498033a81b0541753ef3ed6d387de56bdf230404cff9b86ceae0c2`.

These asset-free suites are not a new interactive gameplay run. The user's
successful Mac map/base/turn/restart/close check at `5cd78418` remains separate;
restart latency is still untimed. Frozen catalogs are not mutable faction
technology state or live conventional units. World downloads are not disk
saves or deterministic replay. This checkpoint predates the separate admitted
snapshot Failure/Start correction; it does not close the held PR77 findings.
M0, M1 and the complete conquest scenario remain open.
