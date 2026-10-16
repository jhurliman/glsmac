# C++ correctness checks

Run analysis when preparing a C++ change for review. It is separate from the
normal edit/build/test loop and does not require the original game assets.
The `C++ correctness` pull-request workflow runs the same command as local use.

## Local use

Install clang-tidy **15** and the usual Linux build dependencies. On Ubuntu:

```sh
sudo apt-get install clang-tidy-15 cmake ninja-build libfreetype-dev libsdl2-dev libsdl2-image-dev libglu-dev libglew-dev libossp-uuid-dev libyaml-cpp-dev
cmake -S . -B build/analysis -G Ninja -DCMAKE_CXX_COMPILER=clang++-15 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=OFF
cmake --build build/analysis --target glsmac_version
```

After initial configuration, the fast command checks local edits, including
staged and untracked C++ files:

```sh
python3 tools/analyze_cpp.py
```

To check a branch against the merge base with its target, including local edits:

```sh
python3 tools/analyze_cpp.py --base origin/main
```

Use `--all` to scan every production translation unit in the compilation database
and `--jobs N` to set parallelism. This mode does not require Git, so it cannot
detect newly added sources omitted from a stale database; reconfigure first.
The manually dispatched GitHub workflow always configures a fresh database and
runs a full scan. `--clang-tidy PATH` supports
version 15 installed under another name. The runner rejects other versions so
an unreviewed toolchain upgrade cannot silently change the blocking rules.
Reconfigure CMake after adding source files or changing build settings.

WSL users should keep the checkout and build directory in the Linux filesystem;
Windows-managed Git worktree metadata contains Windows paths. Native Windows
compilation databases and MSVC arguments are not part of this Linux CI check.

## Scope and diagnostics

The runner analyzes complete changed production translation units under `src/`.
Generated `src/tmp/` files and `src/gse/tests/` test fixtures are excluded. A
header, CMake file, analysis configuration, runner, or analysis workflow change
widens the run to all production translation units. Deleted headers also widen
the scan. This avoids incomplete include-dependency guesses; source deletion
alone needs no analysis. A changed source missing from the compilation database
fails with a reconfiguration request.

Two known `.cpp` files are not independent Linux translation units.
`src/network/impl/Windows.cpp` is explicitly excluded from this Linux check;
relevant runs report it in the console and `summary.json` as skipped.
`src/ui/dom/Object.colormap.cpp` is included by `Object.cpp`, so its changes
trigger a full scan just like a header. These are exact paths; other sources
missing from the database still fail instead of being silently skipped.

Diagnostics from project headers remain enabled. There is no changed-line
filter, warning baseline, or `NOLINT` suppression. Every enabled finding fails
the check, including findings outside the edited lines. Compiler errors,
analyzer failures, and per-file timeouts fail too. Failed-file diagnostics print
in the job log; complete per-file output and `summary.json` are saved under
`build/analysis/analysis-results/` and uploaded by CI. Compiler warnings outside
the selected policy remain available in those logs.

The explicit policy in `.clang-tidy` enables:

- `clang-analyzer-core.*`: null dereferences, undefined values and operations,
  divide-by-zero, invalid stack escapes, and the other core path checks.
- `clang-analyzer-cplusplus.NewDelete`: use after delete and double deletion.
- `clang-analyzer-unix.Malloc`: use after free, double free, invalid frees, and
  other modeled `malloc`/`free` errors.
- `bugprone-sizeof-expression`: incorrect `sizeof` operands or arithmetic.
- `bugprone-suspicious-memset-usage`: likely swapped or invalid fill arguments.
- `bugprone-use-after-move`: moved-from objects used without reinitialization.

No style, naming, formatting, or modernization checks are enabled. Categories
must demonstrate useful findings before becoming blocking rules. In the initial
trial, `bugprone-string-constructor` mostly identified an overridden member
initializer, and `bugprone-undefined-memory-manipulation` flagged the existing
numeric vector/color buffer copies. Those entire categories are omitted rather
than accumulating local suppressions or requiring unrelated representation
changes. The older broad `scan-build` artifact job remains separate.
Leak-only checks such as `clang-analyzer-cplusplus.NewDeleteLeaks` have not been
validated and remain disabled; the manual GC ownership model needs particular
care when evaluating those categories.

## Measured cost and limitations

The initial trial analyzed 284 production translation units in **52.23 seconds**
at eight jobs. Ten representative source files took **8.51 seconds** at four
jobs. After fixing three genuine core findings, the core policy took **47.59
seconds**. The separate `NewDelete`/`Malloc` trial took **43.78 seconds**, with no
findings. The final combined policy scanned all 284 in **51.06 seconds**, with no
findings, at eight jobs. These are local
measurements with clang-tidy 15.0.7 on WSL Ubuntu 22.04, 32 logical CPUs, and
sources on the mounted Windows filesystem; they are not hosted CI estimates.
CI uses four jobs, and dependency installation/configuration adds time.

The [hosted PR #16 run](https://github.com/jhurliman/glsmac/actions/runs/38867153319/job/116862507393)
at commit `e635bbc1` on October 16, 2026 scanned all 284 production translation
units in **151.05 seconds** at four jobs, with zero failures. The Ubuntu 24.04
job took about **3 minutes 26 seconds** including installation, all 14 runner
tests, configuration, and artifact upload. This is an observed run, not a CI
latency guarantee; queue time is excluded.

The initial findings produced separate fixes and regression tests for missing
native event-handler returns, clearing a scene camera, and embedded NUL bytes
in hexadecimal color strings. Keep gameplay rules, concurrency, ownership and
platform behavior covered by tests and code review: this Linux Debug scan does
not prove their correctness or exercise platform-specific branches.

Scope-selection and failure-handling tests need only Python and Git. The real
diagnostic fixtures additionally use clang-tidy 15 (and skip when it is absent).
They verify that null dereference, use after delete and double free actually
fail, including diagnostics in project headers, while valid ownership passes.
CI runs both sets after installing LLVM:

```sh
python3 -m unittest discover -s tools/tests -p 'test_analyze_cpp*.py'
```
