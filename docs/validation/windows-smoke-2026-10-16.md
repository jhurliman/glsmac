# Windows integration smoke test — 2026-10-16

This is a GLSMAC integration observation, not an original-game comparison or a
claim of SMAC/SMAX fidelity.

- Checkout: combined commit
  [`69e72f06`](https://github.com/jhurliman/glsmac/commit/69e72f069b5f4161c36eadf4468fdd291f844c19),
  published as an ancestor of the immutable evidence branch
  [`codex/validation-20261016`](https://github.com/jhurliman/glsmac/tree/codex/validation-20261016).
- Build: native Windows Debug, `x64-debug-win-clang` CMake preset.
- Assets: installed GOG Planetary Pack on aleph0, using `--smactype gog`.
- Window: 1280 × 800; quickstart world: 32 × 32; random seed was not pinned.
- Isolated configuration prefix: `out/smoke-win-20261016`.
- Log files retained locally: `out/windows-smoke-stdout.log` and
  `out/windows-smoke-stderr.log`; no original assets are included here.

Launch the built executable from the repository root with the local installation
path supplied to `--smacpath`, followed by:

```text
--smactype gog --prefix <isolated-prefix> --windowed --window-size 1280x800 --quickstart --quickstart-mapsize 32x32
```

`--windowed` must precede `--window-size`. The initial launch omitted it and
exited with code 1 and the expected argument error; that was not a runtime crash.
On Windows, use an explicitly waited process to capture the GUI executable's
exit code and redirected output.

## Observed sequence

1. A world rendered with Gaia's Landing, a native unit, terrain, resource icons,
   minimap and the bottom interface. The display showed mission year 2101.
2. Double-clicking Gaia's Landing opened its base screen. Worker/resource icons,
   Headquarters, intake/consumption/surplus and energy allocation were visible.
   Closing the screen returned to the map.
3. Turn Complete displayed the unmoved-unit prompt. Confirming advanced the
   display to mission year 2102; the log recorded the complete/advance events.
4. Menu → Game → Start New Game, followed by confirmation, initialized another
   quickstart world in the same process. University Base rendered at year 2101.
   Quickstart re-enters world creation here rather than stopping at the main menu.
5. Clicking the window's close button completed with exit code 0. The log
   recorded game cleanup, OpenGL/window cleanup and the main thread stopping.

This exercises real startup, base UI, one turn, world replacement and shutdown
with the combined camera/light lifetime fixes. It does not validate numeric
yields against the original, worker reassignment, save/reload, network play,
audio, performance, or sustained stability. The optional allocation tracker was
not enabled for this interactive run.

## Headless checks

The following separate runs on October 16, 2026 used combined commit
[`dc546b5b`](https://github.com/jhurliman/glsmac/commit/dc546b5bdca12a727fe75ce39932dbc6b12ebfe5),
the tip of the evidence branch above. This adds the framed-buffer read fix to
the interactive smoke checkout. The published commit makes the complete tested
source tree, including the camera/light changes, available for reconstruction;
the supporting logs remain local.

| Check | Command after build | Observed result |
| --- | --- | --- |
| Linux Debug, WSL Ubuntu 22.04 | `ctest --test-dir build --output-on-failure` | 4/4 passed, 18.48 s |
| Native Windows Debug | `ctest --test-dir out/build/x64-debug-win-clang --output-on-failure` | 4/4 passed, 20.51 s |
| Selected correctness policy, LLVM 15.0.7 | `python3 tools/analyze_cpp.py --all --jobs 8` | 284 production translation units, 52.75 s, zero failures; Windows-only network implementation explicitly skipped |
| Linux ASan/UBSan | `ctest --test-dir <asan-build> --output-on-failure` | Three exit probes passed; the main suite process exited with a segfault before output at 0.03 s |
| Targeted sanitizer retry | `ctest --test-dir <asan-build> -R '^gse$' --output-on-failure` | Main native/script suite passed, 18.04 s |

The sanitizer environment used `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`. The initial empty-output failure resembles
the separately encountered sanitizer startup issue, but this run did not
capture its stack. Retain both attempts; this is not a clean first-pass 4/4
sanitizer result or a leak-check result.

Local evidence files use the `buffer-integrated-` prefix under
`/home/jhurliman/src/glsmac-gc-audit/` (Linux build, CTest, analysis, sanitizer
CTest and targeted retry) and `out/` (Windows build and CTest). Later integration
commits require their own relevant validation.

## Follow-ups

- A jagged black strip was visible between the map's lower edge and the bottom
  interface in both worlds. Determine whether terrain coverage or clipping is
  responsible and add a reproducible rendering case.
- One injected Alt+F4 action did not close the window. The title-bar close button
  did. Reproduce with direct keyboard input before attributing this to the game.
- Repeat with a pinned seed, additional window sizes, save/reload and repeated
  world replacement; this single run is not the release soak gate.
