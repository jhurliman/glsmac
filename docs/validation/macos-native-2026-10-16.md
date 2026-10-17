# Native macOS validation, 2026-10-16

The pinned revision built natively on honey and passed all eight CTest cases.
A separate hidden-window diagnostic also created and used an Apple OpenGL 4.1
core context. The user subsequently confirmed the map/base/turn/new-game/close
checklist worked and supplied before/after screenshots. Screenshots and logs
corroborate a rendered game and successful restart into a different world.
The final follow-up on October 17 confirmed exit status 0 and a clean tracked
memory/OpenGL shutdown. This is one native interactive smoke run, not evidence
of pixel fidelity or general race freedom.

## Revision and isolation

- Tested revision: `5cd78418271231121fb9a8e1d136faa82bbc5dd0`, retained as
  `codex/validation-20261016-macos`.
- Detached validation checkout:
  `/Users/jhurliman/.codex-validation/glsmac-20261016-5cd78418/src`.
  Its sibling `build-debug`, `logs`, and application prefix are isolated.
- The transfer used a self-contained bundle containing only that reference.
  Bundle SHA-256:
  `828fc783b2b574f8ff2d91e82649dea05ce8eae48f864587b4b0dad52cb0ddd6`.
- The user's existing checkout at
  `/Users/jhurliman/Documents/Code/jhurliman/glsmac` remained clean at
  `34f45f54958b9aa8c5c3799ea6803e218f2b99dc`; it was not built or modified.
- Installed dependencies were reused. No system, Homebrew, SSH, accessibility,
  or screen-recording settings were changed.

## Native build and tests

Environment: macOS 26.5 (25F71), arm64, Apple M2 Max, 32 GiB RAM; Apple Clang
21.0.0 (`clang-2100.1.1.101`), CMake 4.3.0 and Ninja 1.13.2. Installed dependencies
include SDL2 through sdl2-compat 2.32.72, SDL2_image 2.8.12, GLEW 2.3.1,
OSSP UUID 1.6.2, yaml-cpp 0.9.0 and FreeType 2.14.3 (whose pkg-config version is
26.6.20).

With `ROOT=/Users/jhurliman/.codex-validation/glsmac-20261016-5cd78418`:

```sh
env PATH=/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin \
  PKG_CONFIG_PATH=/opt/homebrew/opt/ossp-uuid/lib/pkgconfig \
  LDFLAGS=-L/opt/homebrew/lib \
  cmake -S "$ROOT/src" -B "$ROOT/build-debug" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cmake --build "$ROOT/build-debug" --parallel 4
ctest --test-dir "$ROOT/build-debug" --output-on-failure
```

Configure and compilation succeeded. Compilation emitted existing warning
families including nontrivial-type `memcpy`, variable-length array extensions,
duplicate linked libraries and a null string initializer in `Event.h`; this was
not a warning-clean build.

CTest passed **8/8 in 28.55 seconds**, including the full native/script GSE suite,
the UUID and mesh memory-tracking cases, native-filter CLI tests, script exit
status cases and ruleset CLI tests. This was an ordinary Debug build, without
ASan, UBSan or TSan. The binary is a Mach-O arm64 executable with SHA-256
`f3667105c66432ffa8804d0e116046b4a872049df5e2134e415d39a2e3369640`.

## Hidden context diagnostic

An isolated C++ diagnostic used the installed SDL2/GLEW and the production Mac
context/pixel attributes. It created a 64x64 `SDL_WINDOW_HIDDEN` window, verified
that it remained hidden, created a forward-compatible 4.1 core context,
initialized GLEW, queried the context, and completed `glClear`/`glFinish` without
an OpenGL error. It then destroyed the context and window. It loaded no game
assets and did not exercise GLSMAC shaders, scene rendering or gameplay.

The diagnostic exited 0 and reported:

```text
SDL_runtime=2.32.72
SDL_video_driver=cocoa
GL_vendor=Apple
GL_renderer=Apple M2 Max
GL_version=4.1 Metal - 90.5
GLSL_version=4.10
GL_core_profile=1
GL_context_flags=1
GLEW_version=2.3.1
hidden_window_verified=true
context_diagnostic=passed
```

Its retained source SHA-256 is
`189cddc69a1eaed5aee4b8ed4177a5424a831ed1f3d6b0f83d4236c7392ebae2`.
This establishes native driver/context availability, not a rendered-game pass.

## Original installation and visible session

The read-only installation is
`/Users/jhurliman/Documents/Games/Alpha Centauri` (140 CVR files). Its `alpha.txt`
is 53,073 bytes with SHA-256
`38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`, matching
the SMAC rules reference on aleph0. Its `terran.exe` and `terranx.exe` also match
the reference executable hashes recorded in the retained metadata.

The installed **`alphax.txt` differs**: honey has 63,766 bytes and SHA-256
`26f48a5b0bfcf75b4d5c0bfc244084c6335d3aec77d3afc2f007faf049cb6897`, while
aleph0's reference is 62,778 bytes. This session deliberately selects SMAC;
no matching SMAX rules or SMAX fidelity result is claimed.

The visible command uses the pinned binary, its own data and prefix, and the
matched installed SMAC assets:

```sh
"$ROOT/build-debug/bin/GLSMAC" \
  --datapath "$ROOT/src/GLSMAC_data" \
  --prefix "$ROOT/smac-windowed-prefix" \
  --smacpath "/Users/jhurliman/Documents/Games/Alpha Centauri" \
  --smactype gog --ruleset smac \
  --windowed --window-size 1280x800 \
  --quickstart --quickstart-mapsize 32x32 --memorydebug
```

The authorized launch started at **2026-10-16 14:48:54 UTC** through the retained
`run-windowed-smac.sh`; launcher PID 27144 and GLSMAC PID 27145 were still alive
at the first follow-up. GLSMAC logged `Creating window`, then its own 4.1 Metal
context. The log reached selected-unit and Morgan Industries sprite creation,
then `Received minimap texture`. Stderr emitted one Apple GL
diagnostic: `unit 0 GLD_TEXTURE_INDEX_2D is unloadable` and fallback to a zero
texture. That warning remains an observation requiring visual interpretation,
not a diagnosed renderer defect.

The logged-in user's GUI bootstrap domain and a WindowServer connection were
available through SSH. Passive preflight checks returned false for this helper's
accessibility and screen-capture access. No prompt, capture or synthetic input
was used. The game was left running for manual map/base/turn/restart/close checks.

## Manual results and follow-up

The user reported **“All steps worked”** for the requested map display, base
screen, turn, Start New Game and close checklist. They additionally said the new
game took a little while but worked. This is the user's interaction report;
the agent did not operate the game.

Two user-supplied 2784x1888 screenshots were inspected:

- Before restart, `codex-clipboard-6898d13d-52ba-4710-a690-3cbc6d40bb5f.png`
  shows the `GLSMAC v0.3-5cd78418-debug` window, a rendered Morgan Industries
  world, terrain, base/unit sprites, bottom interface and minimap. Mission Year
  2101 is visible. SHA-256:
  `ecc8bccd08e4578fa821c624eb50dd6bc8c04ae693681a4373f745d1116fe847`.
- After restart, `codex-clipboard-82cd32a7-491f-4f78-a758-68df9de188c4.png`
  shows the same revision with a different landscape, University Base, unit
  panel and minimap, again at Mission Year 2101. SHA-256:
  `bfd44d41c52ad790600dc7e705c35a79d998548070497e4ac66ffe97ba2f9759`.

These still images support visible world replacement and interface rendering.
They do not independently show the base-screen interaction, turn action or
close action, and are not an original-game image comparison. The base-screen
result therefore rests on the user's report.

The retained follow-up stdout adds measured event-order evidence:

| Log lines | Observed event |
| --- | --- |
| 866–870 | `complete_turn`, `advance_turn`, then `Turn started: 2` |
| 875 | `Resetting map` |
| 978, 999 | New 32x32 map generation, then map initialization |
| 1082 | The replacement world's `Turn started: 1` |
| 1178 | Replacement frontend received its minimap texture |

No application exception or assertion appears in that output. A movement event
was rejected because the source and destination were the same tile; subsequent
movement, turn and restart events continued. The earlier Apple GL texture
diagnostic remains the only stderr message and is not explained by these checks.

At the initial process follow-up, both launcher 27144 and GLSMAC 27145 were still alive
with reported elapsed time **06:58**, and `windowed-smac-exit-status.txt` did not
exist. The logs contained no shutdown sequence. Thus the user-reported checklist
success and the still-running process are recorded separately; no successful
exit status or clean memory-tracking shutdown is claimed. No kill, relaunch or
additional input was performed to resolve that discrepancy.

At approximately **2026-10-17 05:40 UTC**, a read-only follow-up found that
GLSMAC 27145 was no longer running and the launcher's exit-status file contained
**0**. Final stdout records OpenGL/window/SDL teardown, the main thread stopping,
`Shutting down`, and `No memory/opengl leaks detected.` The launch command used
`--memorydebug`; this completes the normal-exit and final tracker checks for
this one process. Its final stderr still contains only the earlier Apple texture
diagnostic. No process was stopped or relaunched to obtain this result.

Restart latency remains **untimed**. The log prefixes are relative values from
the unsynchronized shared `last_time` in `src/common/Common.cpp`, not a pair of
timestamped restart boundaries. Summing them would not give a dependable restart
measurement. The observed successful restart does not establish a bound on
latency or close the separate concurrency/lifetime investigations.

## Retained evidence

On honey, the isolation root's `logs/` contains `configure.log`, `build.log`,
`ctest.log`, `ctest-LastTest.log`, `native-validation-metadata.json`,
`hidden-context.log`, `hidden-context-metadata.json`, the capability preflight
logs, and `windowed-smac-*` launch/output files. The native metadata was captured
before the visible launch, so its `visible_glsmac_launched: false` describes
that earlier checkpoint; `windowed-smac-launch.json` records the later launch.
The script writes `windowed-smac-exit-status.txt` only after GLSMAC exits.

Copies and the small diagnostic sources are retained in the validating
worktree's ignored `out/validation-honey/` directory. The follow-up stdout/stderr
snapshots are preserved as `windowed-smac-manual-followup-{stdout,stderr}.log`;
the table above refers to that stdout snapshot. The supplied screenshots remain
in the task's attachments and are not copied into the repository. Original assets
are not redistributed.

The final stdout, stderr and exit-status files were copied without modification
into the integration checkout's ignored `out/validation-honey-exit-20261017/`.
Their SHA-256 values are, respectively:

- `3cb75dee8023ab0bfd83b0c5995ae6992e636596b87d43627e734d507d3aecd0`
- `72d8a1e4cadf6abfa1001e42f838538410be57addaa3136175e4579de73cb1f9`
- `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa`
