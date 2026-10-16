# Faithful GLSMAC 1.0

This is the fork's execution plan. The upstream README roadmap remains useful
historical context, but completion here is determined by reproducible acceptance
results, not by the presence of a screen or a version number.

## Release contract

Deliver complete, playable **Sid Meier's Alpha Centauri (SMAC)** and **Alien
Crossfire (SMAX, included in the Planetary Pack)** campaigns using the player's
installed original assets. Select the ruleset explicitly; installing Planetary
Pack must not silently apply SMAX rules to a SMAC campaign.

The target is the original game's rules, content, interface, audiovisual
presentation, and player-visible behavior. Resolve uncertain mechanics with the
installed rules, manuals, and controlled original-game observations. Track
contradictions and intentional fixes individually. Do not turn convenient
approximations into undocumented rules. AI implementation and map generation
need not reproduce the original program's internal algorithms, but their
observable behavior and outcomes need separate fidelity evaluations.

Windows, Linux, and macOS are release targets. Preserve upstream FreeBSD
portability and verify it before release. Native GLSMAC multiplayer and robust,
versioned GLSMAC saves are mandatory. Original executable multiplayer
interoperability is outside the existing architecture. Original save/map import
is a separate compatibility investigation, not a substitute for working saves.

No original assets or binaries belong in this repository. The engine continues
to load them from a user-supplied installation. See
[reference validation](reference-validation.md) for provenance and evidence.

## Starting evidence — 2026-10-15

Audit baseline: upstream `c602cef43a99b984a01f801ae40d254bc9362ff2`, with the
five-file macOS port at `9e959c64a0e7f7b8b6116cb6d33f5b71d8b92cef` as the
proposed platform baseline. The work is not yet a playable strategy game.

| System | Evidence in the source | Implication |
| --- | --- | --- |
| Renderer, map, UI, network | `src/graphics`, `src/game/frontend`, `src/game/backend/map`, `src/network`, scripted UI | Substantial existing infrastructure; retain it and test it |
| Units and starting world | `GLSMAC_data/default/units/defs.gls.js`, `game/world/default.gls.js` | Four native unit types; starting scout is a native unit, not a conventional Scout Patrol |
| Base economy | `GLSMAC_data/default/game/bases.gls.js` | Tile intake, workers, consumption, and growth exist; growth has approximations and support is unfinished |
| Production and resource UI | `GLSMAC_data/default/ui/parts/game/popup/base_screen/bottom_bar.gls.js`, `base_screen.gls.js` | Several displays contain fabricated values; display presence is not implemented production/research |
| Combat and turn events | `GLSMAC_data/default/game/event/attack_unit.gls.js`, `complete_turn.gls.js` | Native combat and direct turn progression exist; animation callbacks still affect authoritative combat state |
| Player progression | `src/game/backend/Player.h` | No complete faction research/economy/diplomacy/AI state; human type is hardcoded |
| Factions and rules | `GLSMAC_data/default/factions.gls.js`, `src/game/backend/faction/Faction.h`, `src/game/backend/rules/default/Default.cpp` | All 14 faction entries exist, but bonuses/social agendas/personalities are not implemented; default rules initialization is empty |
| Snapshots | `src/game/backend/base/{Base,Pop,BaseManager}.cpp`, `Game.cpp` | Population definition name/ID mismatch, missing facility definitions and custom/worker state; transport snapshots do not establish save/reload correctness |
| Synchronization | `src/game/backend/turn/Turn.cpp`, `Game.cpp` | Finalize/checksum paths contain TODO throws; do not infer that the separate direct turn event always exercises them |
| Automation and visibility | Gameplay and frontend audit | No complete strategic AI, diplomacy, fog/exploration, or victory loop found |

Path abbreviations in the table are relative to their named directory; confirm
the current source before implementing each item. This is an audit, not an
assertion that every unlisted feature is absent.

The latest upstream submission is [PR #69](https://github.com/afwbkbc/glsmac/pull/69).
Its reported Linux/WSL Debug suite passes and WSLg software-rendered smoke test
are useful baseline evidence. Its macOS Debug build passes, but the full GSE
suite crashes in the benchmark/GC phase on both patched and untouched upstream.
Interactive macOS checks and Linux hardware performance validation remain open.
The fork integration is [PR #1](https://github.com/jhurliman/glsmac/pull/1).

## Milestones and exit gates

Work in dependency order, with independent reference capture, UI, and platform
tasks in parallel. Milestone labels are planning labels, not promised dates.

| Milestone | Work | Required evidence to exit |
| --- | --- | --- |
| M0 — Trustworthy foundation | Cross-platform Debug builds and tests; reliable failing exit codes; reproduce/fix GC crash; original-install manifests; fidelity case registry; deterministic scenario harness | Failing tests fail CI; full GSE suite passes on release platforms; baseline interactive/rendering checks recorded; at least one original-game behavior fixture with provenance |
| M1 — Persistent simulation | Stable object IDs; explicit serializable game state separate from callbacks/UI; complete definitions and state snapshots; RNG state; deterministic turn ordering, checksums/replay; validated commands, rollback, save versioning and bounds checks | Semantic round-trips preserve base population/worker links, facilities, nutrients, units, ownership and RNG; resume/replay matches uninterrupted run; late join matches host; corrupt/unsupported saves fail safely |
| M2 — Complete small conquest match | Conventional scout/former/colony pod; base founding; mineral production, support and maintenance; Centauri Ecology and research; farms/mines/solar arrays; movement/roads; conventional combat, capture, elimination and conquest; exploration/fog; usable commands and real UI values | Two human factions play the scenario below from setup to result, locally and over network, with saves at every checkpoint; all included mechanics have reference cases |
| M3 — Full SMAC simulation | All base-game technologies/prerequisites, components/design workshop, units, terrain/jobs, facilities/projects, economy/social engineering, drones/talents/specialists, ecology/native life, commerce/probe actions, transport/air/naval/orbital rules and world events | Inventory reconciled against the pinned SMAC reference; each rule family has normal/boundary/interaction cases; complete campaigns and long-run simulation |
| M4 — Single-player and politics | Legal AI actions, expansion/economy/research/combat; diplomacy, treaties/pacts/vendettas, council, trading and negotiations; governors, former/unit automation; all SMAC victories and defeat paths | Seeded AI campaigns finish without stalls; no hidden-information cheating outside documented difficulty rules; original-behavior comparisons; conquest, diplomatic, economic, transcendence and time/score outcomes tested |
| M5 — Alien Crossfire | Expansion factions including alien rules; added tech/components/abilities/facilities/projects; expansion terrain/native life and faction/victory interactions; Planetary Pack scenarios/content inventory | Separate SMAX campaigns and reference cases pass; SMAC mode remains unaffected; expansion data/content checklist reconciled |
| M6 — Presentation and compatibility | Every functional screen/dialog and preference; keyboard/mouse behavior; readable/resizable UI; tutorials/help/datalinks; original movies, interludes, sound and music; accessibility and platform integration | Screen/interaction inventory reviewed against originals; no placeholder values; assets resolve on supported installations; audio/video/input tests across platforms |
| M7 — Golden 1.0.0 | Release candidates, packaging, upgrade/save migration, performance/memory/network soak, fresh-install testing, documentation and license/asset separation verification | All release gates below signed off against a specific commit and packaged binaries |

AI work starts with a small legal deterministic opponent during M2, so
single-player architecture is exercised early. M4 remains the full AI quality
gate. Presentation verification runs throughout; M6 must not become a place to
hide broken gameplay. Multiplayer and save/reload stay green as each system is
added rather than being retrofitted at the end.

### M2 acceptance scenario

Use a small fixed world and two human factions with recorded options/difficulty.

1. Start with real starting units and found the first base through gameplay.
2. Assign workers, grow and exercise starvation without leaving stale tile claims.
3. Produce a Scout Patrol, Former and Colony Pod with actual mineral accounting.
4. Research Centauri Ecology through the research system, including prerequisite
   and availability changes.
5. Improve terrain with a farm, mine and solar array; verify work time, yields,
   cancellation and technology constraints. Exercise movement on roads too.
6. Found a second base; verify pod consumption, population, ownership and support.
7. Fight conventional units, capture the opponent's bases, resolve remaining
   units/pods according to the pinned rule, and trigger conquest only when valid.
8. Save/reload at each checkpoint. Match the uninterrupted semantic state and
   subsequent RNG-dependent results. Repeat with a remote client and late join.

An internal milestone may use scripted opponents. That does not constitute
finished single-player or satisfy the 1.0 AI gate.

## Immediate PR queue

| ID | Slice | Acceptance / dependency |
| --- | --- | --- |
| F01 | Integrate the narrow macOS baseline | Fork PR #1; Codex review, upstream recorded limitations, interactive/GPU checks before completion |
| F02 | Make GSE failures fail CI; register CTest; platform regression workflow | A passing script, deliberately failing script, missing script and complete suite report correctly; no proprietary assets needed |
| F03 | Repair population removal and rollback bookkeeping | Real event handlers tested for nutrient restoration and both worker/tile links; guard undefined rollback base; growth/starvation and specialist cases |
| F04 | Pin reference installations and approve this release plan | Reproducible metadata-only inventory, synthetic tool tests, original-versus-patched executable distinctions |
| F05 | Reproduce and repair GSE GC lifetime/concurrency failures | Minimized failing regression and full suite on macOS/Linux/Windows; sanitizer evidence where available; do not disable GC to pass |
| F06 | Introduce semantic backend scenario tests | Real backend events/state, no render dependency; deterministic seed and normalized state output; distinguish mocked rule tests from integration |
| F07 | Repair and version base/population/facility snapshots | F06; definitions restored before instances; stable IDs and all authoritative links/state preserved; malformed/older format policy explicit |
| F08 | Finish deterministic turn/snapshot/replay contract | F07; RNG, faction and entity state preserved; checksum divergence diagnosed; simulations independent of animation timing |
| F09 | Implement conventional unit definitions and colony founding | F08 plus referenced costs/stats/start setup; real rendering and command feedback |
| F10 | Complete production, research and terraforming slices | F09; separate PRs per mechanic with reference and replay tests |

Keep the next three actionable slices specified in enough detail to implement.
Expand later milestones into PRs when their prerequisites and reference cases
exist. A speculative list of hundreds of tickets is not an implementation plan.

## PR and review workflow

- Use small `codex/` branches targeting the fork. Keep upstream-facing portability
  contributions separate from the larger gameplay effort.
- Each PR describes the observable problem, the resulting behavior, the reference
  evidence when applicable, and tests actually run. Mark unrun checks explicitly.
- Request **Codex code review on GitHub** for every PR. Record the reviewed head
  commit; address findings and request another review when the fix changes it.
  A posted request is not a completed review. If integration is unavailable,
  leave the PR pending and record the blocker.
- Keep changes independent where possible. For stacked PRs, name the dependency
  and use the preceding branch as base so reviewers see only the new slice.
  Retarget after the dependency merges; rerun affected validation.
- Merge only when relevant checks and review findings are resolved. Never merge
  merely to move the queue or to make a milestone appear complete.
- `.gls.js` is GSE, not JavaScript: follow `AGENTS.md` and
  `tools/js-vs-glsjs-cheatsheet.md`, then run the actual engine's tests.
- Preserve the existing architecture until evidence justifies a focused change.
  Performance changes need measurements; rules changes need reference cases.

## Golden release gate

- Every milestone above has evidence links, with no open critical correctness,
  data-loss, crash, progression, or multiplayer divergence defect.
- Both rulesets support complete campaigns, every victory/defeat path, all
  factions, and all difficulty levels; representative option combinations and
  edge cases are covered in a documented test matrix.
- Save/reload, autosave, reconnect and supported save migrations pass on all
  release platforms. Fixed scenarios and replay hashes agree across platforms.
- AI finishes campaigns across a published seed/difficulty matrix, uses the
  implemented systems, and passes a documented human playtest evaluation.
- At least 1,000-turn deterministic soak scenarios and repeated new-game/load/
  exit cycles run without leaks, crashes, unbounded growth or deadlocks; include
  late-game maps and network interruption/rejoin scenarios.
- Establish measured frame-time, turn-time and memory budgets on named hardware
  at M0/M2, then enforce them on representative late-game workloads. Software
  rendering results do not establish hardware GPU performance.
- Packaged binaries are installed and exercised on clean Windows/Linux/macOS
  systems using supported original installations. Record FreeBSD build/runtime
  status. Build success alone does not satisfy interactive validation.
- All screens/actions and audiovisual sequences in the content inventory are
  implemented and reviewed. Every accepted difference from the reference is
  documented; no invented fidelity percentage.
- Tag `1.0.0` only on the tested release commit. Publish the evidence, known
  differences, save compatibility policy and installation instructions alongside
  the release artifacts.

## Analysis and development-loop policy

Prioritize short edit/build/test cycles. Static analysis should eliminate
classes of correctness defects, not turn each gameplay PR into a style rewrite.
Keep local checks explicit and fast, with a small reviewed set of blocking
correctness checks in PR CI. Analyze changed translation units without hiding
diagnostics by changed-line filtering; widen analysis when shared headers or
build configuration change. Offer a separate full analysis command for broader
investigation. Measure runtime and inspect actual findings before promoting a
rule into the gate.

Do not block on readability, naming, formatting, or wholesale modernization
rules. Record existing findings and fix them in focused PRs; avoid suppressions
that hide newly introduced defects. Any necessary suppression needs a narrow
scope and an explanation. Compiler diagnostics, static analysis, sanitizer
tests, and deterministic gameplay regressions cover different failure modes;
none substitutes for the others. Keep platform-specific checks visible and
do not describe an unsupported analyzer invocation as coverage.

## Current status

M0 and the prerequisites for M1 are in progress. No milestone is complete.
The reviewed SDL/main-thread foundation landed in fork PR #1 at
[`428cf01f7a9a13b70dd29ff505e9e5e7aec1f496`](https://github.com/jhurliman/glsmac/commit/428cf01f7a9a13b70dd29ff505e9e5e7aec1f496)
as a development baseline; its interactive macOS and hardware Linux checks
remain open. Subsequent implementation PRs are still being reviewed and
integrated. The integration checkout now exercises
real backend entities, event serialization, base snapshots and their script
bindings without the original assets. Linux and Windows CTest pass; selected
correctness analysis scans the complete production codebase without findings.
These checks establish infrastructure behavior, not gameplay fidelity.

The [Windows smoke test on October 16](validation/windows-smoke-2026-10-16.md)
rendered two quickstart worlds in one process, opened a base screen, advanced a
turn and exited normally. It also records visible rendering and input follow-ups.
Interactive macOS validation, hardware Linux validation, long-running sessions,
complete save/reload and multiplayer convergence remain open.

The subsequent integration contains authoritative worker coordinates and reverse
claims, optimistic-event wire preservation and acknowledgement bookkeeping,
exact population rollback, and explicit pre-initialization ruleset selection.
These remain separate reviewed PRs, not merged release functionality. The
verified SMAC base-square calculator and strict RESOURCEINFO parser are pure
components; the live resource callback has not been switched to them.

The [follow-up validation record](validation/rulesets-yields-2026-10-16.md)
pins the combined source, successful ruleset and signed-yield Windows smoke
tests, later headless checks, and the tracked GUI failures and subsequent
functional passes. The tracked two-world run at `705cc939` passed worker
removal/restoration, map replacement and minimap rendering, then exited 0 with
empty stderr and no tracked memory/OpenGL leaks. Earlier retained allocations
and failures remain recorded. At `68081411`, Windows CTest passed 8/8 and Linux
ASan/UBSan/LSan CTest passed 8/8 with leak detection enabled after repairing
normal switch-scope ownership. These are bounded checks, not a soak or full
save/reload result. The async-cancellation defect matching an older PR head's
Windows timeout now has a deterministic regression and a focused repair in
PR #69. Integration `0d98cde8` passes Windows and leak-enabled Linux CTest 8/8;
the original hosted timeout and lack of its exact interleaving remain recorded.
Focused UUID and mesh allocation checks add approximately one second each
locally to CTest. Full allocation-tracked suites and interactive debugging
remain separate from the ordinary development loop.

The next implementation slices are:

1. Complete the focused scheduler review and insert its repair before the
   affected weapon-table PR, preserving that feature's narrow diff. Eight
   deterministic cases cover same-deadline repeat and sibling cancellation,
   self-cancel, nested timers, stopping all timers and exception bookkeeping.
   Separately preserve the backend RNG continuation through snapshot rebuilds:
   validate a versioned algorithm/state header before map allocation, stage
   state during reconstruction and install it before resumed gameplay. Ordinary
   initialization must retain its existing RNG ordering. Turn restoration and
   future host/client lockstep are separate contracts.
2. Resolve conventional-unit content from explicit component tables. Weapon,
   defense, reactor, chassis and counted predefined-design parsers are queued
   in PRs #59, #60, #62 and #65. PR #67 creates owned metadata by exact
   component keys, preserving row identity and rejecting ambiguous or mismatched
   ruleset references. Reactor selection, ability-bit mapping, conventional
   design state, CVR rendering and gameplay commands remain separate work.
   Do not turn the current native-unit movement assumptions into standard-unit
   behavior or invent reactor/cost defaults.
3. Connect resource requests to their actual evaluating base. Worker intake and
   base-screen previews must pass the same canonical base and owner, preserving
   legacy context-free calls. Exercise real work/unwork events and destination-
   world snapshot identities with a fully specified synthetic yield fixture.
   Resource bonus, nutrient, mineral and energy kernels now have original-routine
   comparisons; the energy kernel matches 3,122 direct checks. A native bridge,
   complete frozen rules profile and authoritative terrain, technology, social,
   capability and phase-specific event producers precede normal-game activation.

World downloads are now bound to the frozen host profile before map allocation,
and faction snapshots preserve flags and ordered land/water names. Those
changes and canonical unit snapshots are queued in PRs #47, #48 and #53;
they do not complete disk savegames, whole-state rollback or network convergence.
The native test exit-code gate landed in PR #3 after current-head cross-platform
checks and Codex review. Population-removal bookkeeping, OpenGL context checks
and boolean operand evaluation have also landed through that gate. The remaining
stack continues through the same review and validation process.

The checkpoint work still needs complete RNG state, side-effect-free turn
restoration, canonical player/faction links and turn checksums. The current
world-download path is not a disk-save or deterministic replay implementation.
Production still lacks authoritative current-item/mineral/baseline state, and
its interface contains placeholder progress. Complete these dependencies in
small slices alongside conventional units and founding before claiming the M2
conquest loop.

Live center yields follow only after the adapter can supply authoritative
economy, facilities, terrain/bonus, faction and phase-specific event inputs.
The calculator's resolved event flags are not a replacement for persistent
event state. Original SMAC evidence must not silently become SMAX behavior.

The original installation was found on aleph0 and SMAX PRACX reached setup, but
full-game rule observations remain blocked by the reproduced startup/base-creation
crash in the reference notes. Original executable research proceeds separately;
keep its evidence level distinct from full-game observations. See each PR's
reviewed commit and checks for current validation, rather than treating this
plan as proof that queued work has landed.
