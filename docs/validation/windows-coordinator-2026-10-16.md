# Windows startup coordinator integration check — 2026-10-16

The combined checkout at
[`12a58670f277c442a850b256c84902f242ef0b09`](https://github.com/jhurliman/glsmac/commit/12a58670f277c442a850b256c84902f242ef0b09)
passed a rendered Windows startup, base-screen, turn, new-game and close check.
It is retained at `codex/validation-20261016-initialization-coordinator`.
This is an integration observation, not an original-game fidelity comparison.

The checkpoint includes the initialization coordinator source
`1676caf69a2a995af431022f6139a973fd5ac1e5`, earlier runtime/reset/snapshot work,
the owned ability parser, and the original CVR pixel evidence. The subsequently
implemented indexed rasterizer and native collector-retirement guard are absent.
It is distinct from the smaller publication branches and the earlier Mac build.

## Build and launch

- Host: aleph0, native Windows Debug, `x64-debug-win-clang` CMake preset.
- Executable SHA-256:
  `f04baf5028f8b4eef1ae3cac09724a9fccd4cbb126f62f564213efd4b6231b32`.
- Assets: the local purchased GOG Planetary Pack installation, explicitly using
  the SMAC ruleset. This does not establish SMAX behavior.
- Window: 1280 × 800; quickstart map: 32 × 32; supplied seed: `1:2:3:4`.
- Isolated prefix: `out/smoke-win-coordinator-12a58670-visible`.
- Process 4784 ran from 16:22:20 UTC through 16:28:53 UTC on October 16.
- The Windows title displayed `v0.3-???????-debug`. Revision provenance here
  comes from the pinned checkout/build and binary hash, not that title.

From the repository root, supply the installation to `--smacpath`, then:

```text
--smactype gog --ruleset smac --prefix <isolated-prefix> --windowed --window-size 1280x800 --quickstart --quickstart-mapsize 32x32 --quickstart-seed 1:2:3:4
```

The first attempt used a hidden window and was absent from the UI inventory.
Its disposable process, 6584, was stopped explicitly and reported exit -1.
That setup attempt is not a successful shutdown or an engine crash result.
The visible run above used a separate prefix and separate logs.

## Observed interactions

1. Morgan Industries rendered with terrain, minimap, unit selection and the
   bottom interface at mission year 2101.
2. Double-clicking the base opened its screen; closing it returned to the map.
3. Turn Complete and confirmation of unmoved units advanced the display to 2102.
4. Menu → Game → Start New Game and confirmation replaced the world in the same
   process. University Base rendered at mission year 2101.
5. Double-clicking University Base opened its base screen too; closing it
   returned to the replacement map.
6. Clicking the title-bar close button completed with process exit code **0**.
   Logs record game/graphics cleanup, window destruction and the main thread
   stopping. The allocation tracker was not enabled for this rendered run.

After the new-game confirmation, screenshots observed old terrain without the
HUD at 5.613 seconds, the loading screen at 13.259 seconds, and the replacement
map with its HUD at 22.088 seconds. These are observation times including input
injection and capture overhead, not an exact generation duration or benchmark.
The two maps differ; a supplied quickstart seed alone is not evidence that the
entire repeated-start sequence reproduces deterministically.

The replacement map had a jagged black border along its top edge and a partially
clipped base badge. These remain visible rendering/camera follow-ups. Numeric
resource displays were inspected only for interaction, not rule correctness.
Save/reload, multiplayer, long-run stability, audio and performance remain open.

## Automated checks at the same source checkpoint

| Check | Result |
| --- | --- |
| Native Windows CTest | 8/8, 49.14 seconds |
| WSL Linux ASan/UBSan/LSan CTest | 8/8, 52.36 seconds; leak detection enabled |
| Full LLVM 15 correctness analysis | 299 production translation units, 66.69 seconds, zero failures; Windows-only network implementation excluded |
| Actual Engine shutdown harness | Default and single-thread modes pass with allocation tracking and ASan/UBSan/LSan |

The sanitizer build retains its existing local `-no-pie` startup workaround;
no repository compiler policy or diagnostic suppression changed. These combined
results do not replace current-head GitHub review or extend the native Mac
validation beyond its previously pinned `5cd78418` build.

Local evidence: `out/initialization-coordinator-windows-{build,ctest}.log`,
`out/coordinator-visible-{process,exit}.json`, and
`out/coordinator-visible-{stdout,stderr}.log`. The two map-initialization log
entries are at lines 647 and 1282; minimap receipts are at 783 and 1445.
Linux logs and analyzer output use the `initialization-coordinator-` prefix under
`/home/jhurliman/src/glsmac-gc-audit/`; the Engine harness manifest is under its
`build-asan/engine-shutdown/` directory. Preserve those local artifacts when
reusing build directories. No original assets or rendered screenshots are
committed in this report.
