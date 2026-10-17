# CLAUDE.md

## `.gls.js` is not JavaScript

Files matching `*.gls.js` (mostly under `GLSMAC_data/default/`) are GSE
(GLSMAC's embedded script language) — JS-*shaped* syntax, JS-incompatible
semantics. Do not pattern-match JS habits onto them.

Before editing or generating `.gls.js`, read `tools/js-vs-glsjs-cheatsheet.md`
for the mental-model diff. Grammar source of truth: `src/gse/parser/JS.h`.

Test loop: `./build/bin/GLSMAC --gse-tests --gse-tests-script PATH.gls.js --quiet`
(no script arg = full suite).

## Development speed and validation scope

Prioritize playable gameplay slices and connecting existing rule implementations
to their live callers. Do not make full Linux/Windows/macOS runs a prerequisite
for every gameplay change.

- During development, use a cached incremental build and the affected native
  test groups or GSE script. Reuse successful checks for unchanged source.
- Run targeted C++ correctness analysis on changed production translation units
  and known affected consumers. Broaden when shared lifetime, GC, threading,
  build infrastructure or an unclear dependency boundary warrants it.
- Use focused sanitizers for ownership, pointer, bounds and serialization risk;
  do not repeat sanitizer/full-suite matrices for every arithmetic rule tweak.
- Reserve full platform suites for platform-sensitive changes, meaningful
  integration/playable milestones and release gates. Prefer existing CI results
  over duplicate local platform runs.
- Keep focused PRs and GitHub Codex review. Record concise relevant validation;
  do not create fresh platform checkouts, elaborate manifests or redundant
  negative-control variants for every small change.

If a focused test or review exposes a broader problem, expand validation to
resolve that specific risk, then return to the tight loop.
