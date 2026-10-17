# Startup designs and supplied combat — 2026-10-17

Integration `445eba1d2c8b5a8cb1a7021d8ddf08dc509b4476` is retained as
`codex/validation-20261017-startup-combat-445eba1d`. It combines owned startup
designs, independent gameplay RNG state, supplied neutral Scout combat and
conventional stat queries with the existing live simulation and Scout preview
paths. It also rejects unselectable world-size catalogs and overflowing Scout
anchors at admission. Publication PRs retain their own exact revisions and gates.

| Environment | Configuration | Result |
| --- | --- | --- |
| aleph0 native Windows | Clang Debug, full CTest | 8/8, 83.08 seconds |
| aleph0 Ubuntu/WSL | ASan/UBSan/LSan, leak detection and halt-on-error | 8/8, 148.14 seconds |
| honey native macOS arm64 | Apple Clang Debug, full CTest | 8/8, 61.39 seconds |

The full LLVM 15 correctness scan covers 328 production translation units with
two workers, zero failures in 254.55 seconds. The Windows-only network source
is explicitly excluded from this Linux scan. No diagnostic suppressions or
analysis-policy changes were introduced.

## Implemented boundaries

- Startup now owns per-faction generated designs, prototype flags, cached fields
  and selected Scout identity. Canonical global difficulty is bound explicitly.
  Simulation V2/world V10 restore exact saved state without regeneration.
- Two explicitly supplied gameplay RNG words have a strict independent codec.
  This does not install them into Game or establish original new-game seeds.
- Pure neutral Gun/Scout combat retains stable identities, separate victory and
  original wrapper-return metadata, exact draw order and promotion behavior.
  Pure morale and full-allowance queries require resolved inputs. Neither slice
  creates a conventional live unit or drives the existing event handlers.
- Real Unit/GetSprite and registered preview checks retain textures through
  teardown. The optional installed-original-CVR test passes with allocation
  tracking on Windows and honey. It uses Null graphics and synthetic badges.

Source contracts and separate original comparisons are published in
[#138](https://github.com/jhurliman/glsmac/pull/138),
[#139](https://github.com/jhurliman/glsmac/pull/139),
[#140](https://github.com/jhurliman/glsmac/pull/140),
[#141](https://github.com/jhurliman/glsmac/pull/141) and
[#142](https://github.com/jhurliman/glsmac/pull/142). Their source-specific
comparisons are not repeated or relabeled as complete gameplay in this receipt.

## Native Mac and retained evidence

The isolated Mac checkout is
`/Users/jhurliman/.codex-validation/glsmac-20261017-445eba1d/src`.
Configure/build/test ran from 11:00:58 to 11:02:55 UTC on October17. The final
checkout was clean; existing interactive sessions were not interrupted.

- Mac binary SHA-256: `b31d95525b98da2f0e7274a458d0f8499da5ae02823942503be6d346351db9d4`.
- Windows binary SHA-256: `e78efc602a80545107bd580d1b37ee6e74a29899fcaeb9c39b84343d123183b3`.
- Transfer bundle SHA-256: `6830ca39650ab2a8026d8238e808efca84c4f987539f0fa4e522a936345eea30`.

Logs remain in `out/startup-combat-windows-*`,
`out/validation-honey-445eba1d/logs/`, and
`/home/jhurliman/src/glsmac-gc-audit/startup-combat-*`.
The manifest `out/startup-combat-integration-manifest.json` pins source revisions,
test/analysis logs, binary and transfer hashes. Its SHA-256 is
`8b4951525ebf11b13dfbc131fe652e6a4e6f3a900c71366deb1e8cb4612b898b`.

The later script-only lobby fix `77687a96` separately passes both actual SMAC
and SMAX setup regressions with allocation tracking on Windows and Linux with
ASan/UBSan/LSan. It prevents readiness without a size and rejects startup before
hiding the lobby. It is not included in the full-build revision above.

There is no new interactive GPU, final-pixel, socket synchronization, disk-save,
replay or complete conquest result here. The user's successful interactive Mac
check remains at `5cd78418`. Live conventional unit state, startup package choice,
movement/combat wiring, founding and the complete economy remain unfinished.
No milestone or golden release is declared complete.
