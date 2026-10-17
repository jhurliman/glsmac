# CI coverage and cost

Ordinary gameplay pull requests use Linux regression tests and the existing
scoped C++ correctness check. Marking a PR ready reuses its existing checks;
draft/ready transitions do not trigger another run. Local development should use cached builds and affected tests; CI keeps
the small asset-free CTest baseline to avoid guessing which tests a change needs.

| Change or event | Regression/lifecycle coverage |
| --- | --- |
| Ordinary PR, draft or ready | Linux Debug build and all CTests; Linux lifecycle sanitizers |
| Ordinary `main` push or merge queue | Same Linux coverage |
| Markdown-only or empty diff | Selection and gates report; engine and lifecycle jobs skip |
| Platform/build/threading-sensitive changes | Linux and macOS regression/lifecycle, Windows regression |
| PR labeled `ci:full` | Full platform coverage for an explicit integration checkpoint |
| Manual regression/lifecycle dispatch or `stable` release push | Full platform coverage |

Explicit full coverage overrides the Markdown-only skip.

Adding or removing `ci:full` reevaluates coverage, as does a new commit. The
selector checks the complete PR diff, including both sides of renames. Pushes
and merge queues compare their supplied base/head revisions. A Git/diff failure
fails the selection job; it never falls back to reduced coverage. All workflows
retain concurrency cancellation of superseded PR revisions.

`tools/ci_scope.py` lists platform paths explicitly: toolchains, dependency
manifests, CI, native graphics/audio/input/network/environment code, shared
threading/GC code, and platform assets such as icons and plists. Ordinary gameplay
headers and source-directory `CMakeLists.txt` files do not automatically force a
platform matrix. This path-only policy does not distinguish additive source
registrations from build-semantic changes inside those CMake files: the latter
require `ci:full` or a manual full run. Unlisted platform risk also needs that
override; update the selector when a new platform subsystem is added.

Linux uses a 2 GiB ccache compiler-output cache; compiler/configuration/input
changes still invalidate compiled results normally. On the unchanged baseline,
the [PR166 Linux job](https://github.com/jhurliman/glsmac/actions/runs/39052107831/job/117452618181)
spent 64.25 seconds in all eight CTests (48.76 seconds in GSE), compared with about
three minutes compiling 381 objects without a cache. This is one hosted
measurement, not a promised duration or a measurement of the new cache.

The old PR autobuild duplicated the Windows Debug compilation with Portable64
packaging and performed a full scan-build compilation alongside scoped clang-tidy.
Autobuild now runs only on `stable` pushes or explicit dispatch. A manual dispatch
builds both portable artifacts and the full scan-build report on the selected
ref. Dispatching it on `main`/`stable` retains its existing release publication
behavior; use another ref when only artifacts are wanted. Tiny `main` merges no
longer automatically replace the development release. Scoped C++ correctness and
the narrowly path-filtered reference inventory workflow are unchanged.

## Check results and branch protection

`Regression gate` and `Lifecycle gate` run even when a dependency fails. They
require successful scope selection and every selected job. A missing, cancelled
or failed required job fails the gate; an intentionally unselected Windows job
is accepted only for reduced scope. Engine/lifecycle skips are accepted only after
successful selection explicitly requested no tests. Summaries state these skips
without claiming that tests passed. These gates establish selected coverage, not an unconditional
claim that every platform passed. A manually cancelled whole workflow does not
produce a passing gate.

The branch-protection API reported `main` unprotected and the repository ruleset
list was empty when this change was prepared. No protection settings were changed.
If required checks are configured, prefer the two stable gate names over individual
conditional matrix jobs. GitHub treats an
[intentionally skipped job as successful](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-jobs-with-conditions),
so requiring Windows alone would not enforce full-platform coverage. The existing
path-filtered correctness/inventory workflows are not always-reporting gates;
do not make them unconditional requirements without addressing that separately.
GitHub Codex review remains a separate requirement, including when its quota is
unavailable. This change neither requests reviews repeatedly nor waives review.

Validate policy changes with `python3 -m unittest discover -s tools/tests -p
test_ci_scope.py` and actionlint. Selection tests exercise draft/ready/main/queue,
manual/release/label/platform/Markdown triggers, actual Git renames, emitted matrix outputs
and failed/cancelled/missing job results. They do not build the game or claim a
hosted workflow run.
