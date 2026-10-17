# Conventional units, Rover presentation and resources — 2026-10-17

Validated integration source: `d6afde1ae214b80f9a399952d3a511ec74020f22` on
`codex/faithful-1.0`. This branch contains independent pending work; it is not
main and must not be merged wholesale. Focused PRs retain their own gates.

| Check | Result | Elapsed |
| --- | --- | --- |
| Native Windows Debug CTest | 8/8 | 96.42 s |
| Linux ASan/UBSan/LSan CTest | 8/8 | 163.90 s |
| Native honey macOS arm64 Debug CTest | 8/8 | 64.58 s |
| LLVM 15 correctness analysis | 336 production units, zero failures | 259.20 s |

Linux keeps leak detection and halt-on-error, with the documented non-PIE
workaround. Analysis uses two jobs and excludes the Windows-only network TU;
Windows compilation and tests are separate coverage. Honey used a fresh clean
checkout at `/Users/jhurliman/.codex-validation/glsmac-20261017-d6afde1a-r1/`,
from 12:35:26 through 12:37:29 UTC. The first temporary clone lacked a bundle
prerequisite and stopped before compilation; the corrected run cloned the
previous validated source. No user's interactive checkout was reset.

## What changed

Conventional Unit state owns design/owner identity, morale, damage, spent
movement and failure marker. Current or incoming simulation context is checked
before creation and restoration. Frontend movement and morale are derived
views; the legacy float API cannot mutate a conventional Unit. This checkpoint
admits the bounded Infantry presentation, with explicit home-none and render
mode. It does not yet activate default conventional spawning or live commands.

World V14 / simulation V4 retain the exact shared and CRT gameplay RNG pair.
New-world entropy is explicitly a GLSMAC policy; restoration never resamples.
The original application's complete initialization/load-to-command lifecycle
remains open. The isolated original rules loader's no-draw result applies to
argument zero; normal nonzero setup performs additional faction work.

Fixed Spartan Rover definitions now use the real Unit/scene/preview path.
Distinct render tags and composition-aware cache keys separate Scout and Rover.
The source and actual publication also pass allocation-tracked preview tests
using locally installed GOG assets with Null graphics. This proves asset and
ownership plumbing, not a new interactive GPU comparison or Speeder admission.

RESOURCEINFO parsing now consumes nine physical rows, including blank/comment
rows, and the measured numeric-prefix conversions. Its separate optimized
comparison matches 38 original cases and 608 yield triples. Frozen profile
frame bounds and supplied keyboard admission are also present in this source.

## Publication and remaining work

Focused PRs are #149 (keyboard), #152 (Rover renderer), #153 (profile bounds),
#154 (RNG ownership), #155 (conventional Unit), #156 (resource text), and #157
(Rover presentation). Each publication has its own local receipt. GitHub Codex
review usage is exhausted again; unreviewed current heads remain unmerged.
Existing completed reviews are valid only for the exact heads they covered.

Next is synchronous conventional Unit/tile/RNG movement, then checked startup
creation. Empty-land movement evidence does not establish movement out of the
HQ base; that complete original request is a specific research priority.
Initialized Infantry faction materials also need a separate measured profile.
No complete conquest match, disk savegame, full SMAC/SMAX campaign or golden
release is claimed. Latest human Mac interaction remains `5cd78418`.

The private manifest is `out/unit-rover-resources-integration-manifest.json`,
SHA256 `2c8d3a9c767fa2dd298940eb3c6d4a7e02a2366c20f652dd961b835c15f74ad0`.
It records source components, logs, bundle and binary hashes. Original assets
and the researcher's working files remain outside implementation commits.
