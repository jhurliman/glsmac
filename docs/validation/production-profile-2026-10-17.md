# Production profiles and arithmetic controls — 2026-10-17

This receipt distinguishes the integrated revisions from their narrower review
branches. These checks establish infrastructure behavior, not live production
or a playable strategy-game loop.

| Source | Check | Result |
| --- | --- | --- |
| `23dcaacfc4811803481817e8990ace8e8b7cdc14` | Native Windows Clang Debug CTest | 8/8, 37.93 seconds |
| Same | Linux ASan/UBSan/LSan CTest, leak detection enabled | 8/8, 48.95 seconds |
| `8923374f6fbb98a0a9029c6b0307b7e7a4e8d4bc` | Native macOS arm64 Debug CTest on honey | 8/8, 29.48 seconds |
| Same | Native Windows and Linux sanitizer focused frame-assembly tests | All seven groups pass with allocation tracking; normal shutdown and no reported leaks |

The first revision adds the complete frozen SMAC production profile to host
settings: four rule values and all 71 facility records. It also includes the
terminal-connection cleanup guard and corrected transform-store boundaries.
The second adds the effective SSE rounding-mode guard. The full Windows/Linux
results above precede that last change; the focused checks exercise it directly.
The corresponding retained branches are
`codex/validation-20261017-production-profile-23dcaacf` and
`codex/validation-20261017-profile-rounding-8923374f`.

## Native Mac run

An isolated checkout at
`/Users/jhurliman/.codex-validation/glsmac-20261017-8923374f/src`
was cloned from the preceding validation checkout and fetched from a transfer
bundle. Configure/build/tests ran from 06:23:14 through 06:24:38 UTC on October
17, with Apple Clang, native arm64, Debug, Ninja, `BUILD_TESTING=ON`, and four
build workers. The source checkout was clean afterward. Neither the user's
checkout nor the previous validation binary/logs changed.

The binary SHA-256 is
`4a5d88c385a79a9bec4b45ddb1b47e4374987a4f58be4533e12566c53c7c29c1`.
The bundle SHA-256 is
`2be87ddbc96f05f100f4ea0ccf39a6a0c0076d6e0d5a9a434a89f33a693b11b2`.
This was a headless suite without original assets, sanitizers or a visible game
launch. Native Mac interaction remains pinned to the user-tested `5cd78418`
described in the [interactive receipt](macos-native-2026-10-16.md).

## Retained logs

Windows integration logs are under ignored `out/`; Linux logs are under
`/home/jhurliman/src/glsmac-gc-audit/`. Native Mac logs remain beside its source
checkout and were copied to `out/validation-honey-8923374f/`.

| Log | SHA-256 |
| --- | --- |
| `production-profile-windows-ctest.log` | `0ace7409c212868fd9e643ddf6c6c769809d00657a424ab6cee1bdf60771a20c` |
| `production-profile-asan-ctest.log` | `a270ee585c123d02607a4be82ada05bc1f00a89fae547cf2283ab8acef7e7edc` |
| Mac `ctest.log` | `f5bc0fc1bc539f0107db02545763fbe0b01daca023e61d95c6d80231b59a81f9` |
| `effective-rounding-windows-test.log` | `12ead509153fade3a6b2fe7112965f9f0bd724e47f2aea85a056be3c38288cd2` |
| `effective-rounding-asan-test.log` | `bb02f958d17b21320cbf4de11c22deec50174ae89f4fef397214da051333231d` |

The earlier Windows native-filter child timeout remains unresolved in the
[previous receipt](faction-transform-2026-10-17.md). These passing runs do not
establish its cause. Hardware Linux interaction, complete save/replay, network
convergence, normal-game production activation, and SMAX rule fidelity remain
separate work.

## Subsequent V4 world binding

Integrated source `cfb5b4d55b5ea6d5ecd13dc81cc1e2106e6d89f5` adds the complete
production-profile comparison to the world header. Native Windows Clang Debug
CTest passes 8/8 in 38.91 seconds; Linux ASan/UBSan/LSan CTest passes 8/8 in
53.83 seconds with leak detection enabled. This later revision has not received
another native Mac run. The source change also has focused early-rejection,
restoration and negative-control evidence in
[world snapshot rules](https://github.com/jhurliman/glsmac/blob/cfb5b4d55b5ea6d5ecd13dc81cc1e2106e6d89f5/docs/world-snapshot-rules.md).

The Windows `out/world-production-windows-ctest.log` SHA-256 is
`f4d61ee99479ec78d7789d5193b554cb729ac6cac770e9feab2e93b533853489`.
The Linux `world-production-asan-ctest.log` SHA-256 is
`e62086f3646774629d4e756fb4418143ad214fc6c2ee4148049f8d19db516471`.
The source is retained at `codex/validation-20261017-world-production-cfb5b4d5`.
