# Windows integration smoke test — 2026-10-16

This is a GLSMAC integration observation, not an original-game comparison or a
claim of SMAC/SMAX fidelity.

- Checkout: combined local commit `69e72f069b5f4161c36eadf4468fdd291f844c19`.
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

## Follow-ups

- A jagged black strip was visible between the map's lower edge and the bottom
  interface in both worlds. Determine whether terrain coverage or clipping is
  responsible and add a reproducible rendering case.
- One injected Alt+F4 action did not close the window. The title-bar close button
  did. Reproduce with direct keyboard input before attributing this to the game.
- Repeat with a pinned seed, additional window sizes, save/reload and repeated
  world replacement; this single run is not the release soak gate.
