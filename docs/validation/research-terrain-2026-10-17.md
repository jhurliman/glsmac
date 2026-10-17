# Research and terrain integration — 2026-10-17

The local campaign now connects final Base labs to directed research, required
choices, discovery and Secrets. A required answer suspends the owning native
turn before credit; answering resumes once through credit, remaining Bases and
faction debit settlement. Technology map revelation retains saved terrain and
Base memory. The frozen TERRAIN catalog supplies ordinary Former action metadata.

| Checked source | Scope | Result |
| --- | --- | --- |
| `d7a52bb3` | Cached Linux Clang Debug, allocation tracking, affected native filter | 82 groups pass; normal exit |
| Same binary/source | Actual required-choice GSE module with UI/API doubles | Script passes; normal exit |
| `d894d286` | Incremental build, repeated tile preview and specialist instruction-boundary groups | Both pass with allocation tracking; normal exit |
| `d894d286` | Focused LLVM15 analysis of frontend Game and Tile | 2 translation units, zero failures, 8.13 seconds |

The 82-group filter covers research cost/pool/reader literals, two actual claimed
Start pause/answer/reset scenarios, discovery and revelation, frozen profile
transport, technology/knowledge/simulation restoration, and immediate/queued
real Client rejection. The research scenarios exercise duplicate answers,
Secrets, exact-once resumption and stable restore. The script check separately
exercises mandatory selection, stale callbacks, duplicate submission and reset.
It does not establish interactive native modal rendering.

The preview fix retires replaced meshes after the staged update is ready; an
existing widget's copied mesh remains valid. Selected tile refresh retains the
selected Unit. Twenty-four alternating land/water updates exercise owned copies
and current improvement/preview data under allocation tracking. This repairs
metadata previews; updating the world atlas and terrain actors remains separate.

Component receipts remain scoped: nine research production units and four
TERRAIN units had clean focused analysis before integration. The installed
TERRAIN probe matches all twenty retained projections under UBSan. These checks
are not new full-suite, Windows/macOS or sanitizer-suite runs. Local work follows
the [cached-build policy](../ci-validation.md); full platforms are reserved for
platform-sensitive changes and playable integration/release checkpoints.

The later Former feature `5b2eb65a` has its own 20-group tracked component receipt
and eight-unit analysis in [the work contract](../smac-former-work.md). The combined
controller/rendering batch is still in progress. This checkpoint does not claim
a complete conquest match, disk-save UI, network convergence, AI or SMAX parity.

The resulting research/catalog formats are World30, Global16, Simulation10,
ResourceRules4, Technology2 and Knowledge4. BaseManager10 and UnitManager4 remain
unchanged here; the subsequent Former feature raises individual Unit records to4.

Logs are retained under `/home/jhurliman/src/glsmac-rover-runtime/` on aleph0:
`research-terrain-build.log`, `research-terrain-tests.log`,
`research-ui-tests.log`, `preview-research-build.log`,
`preview-research-tests.log` and `tile-preview-analysis/summary.json`.
