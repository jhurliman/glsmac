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

The focused LLVM 15 correctness gate has landed on main through PR #16;
readability and modernization checks remain disabled. The explicit single-file
runner has landed in #55. The analyzer still honors explicit NOLINT
directives, so those require review; there is no blanket suppression baseline.
Shared-header changes continue to trigger a full production scan rather than
using the single-file option to narrow the required check.

## Current status — 2026-10-17

M0 and M1 remain in progress. There is no complete conquest match, and no
milestone is declared complete. The integration branch combines tested slices
before their smaller PRs land; its contents must not be confused with main.

The latest [production-profile receipt](validation/production-profile-2026-10-17.md)
records Windows and leak-enabled Linux CTest 8/8 at `23dcaacf`, plus native
macOS arm64 CTest 8/8 at `8923374f`. Focused Windows/Linux tracked tests cover
the latter revision's effective SSE rounding guard. On honey, the user confirmed
map interaction, base inspection, a turn, a new world and close at `5cd78418`.
A later read-only check confirmed exit 0 and no tracked memory/OpenGL leaks.
The screenshots show Morgan Industries before and University Base after restart.
The delay remains untimed; see the [Mac receipt](validation/macos-native-2026-10-16.md).

The [preceding receipt](validation/faction-transform-2026-10-17.md) retains an
unresolved Windows native-filter child timeout followed by passing unchanged
runs. Later successes do not identify its cause. The separate aggregate test
allowance correction does not explain it. Hardware Linux interaction, long runs,
complete saves/replay and multiplayer convergence remain open.

### Implemented foundations

Worker claims and population/event rollback have real backend regressions.
Independent production item/mineral balances and switching baselines landed in
[PR #103](https://github.com/jhurliman/glsmac/pull/103). New bases remain
unconfigured. The mineral-cost calculator landed in
[#98](https://github.com/jhurliman/glsmac/pull/98), and merged
[#108](https://github.com/jhurliman/glsmac/pull/108) calculates switching against
independent baselines and matches 4,594 retained original scenarios. Neither
calculator chooses eligible items or schedules a turn.

[#111](https://github.com/jhurliman/glsmac/pull/111) freezes four SMAC production
rule values and all 71 facility records in host settings. A separate V4
world-header change binds that profile before map allocation; its source
validation is recorded in [world snapshot rules](world-snapshot-rules.md).
It is being prepared as a focused PR. SMAX has no SMAC numeric profile.
Technology definitions and actual faction/design startup producers precede
production activation.

The wider lifetime stack preserves entity state, RNG continuation and turn
metadata, stages map publication, and guards specific native replacement/cleanup
paths from collection. Terminal-connection cleanup is
[#110](https://github.com/jhurliman/glsmac/pull/110). These bounded fixes do not
settle canonical Player/Slot/Connection ownership. Concurrent roster and
failure/reset findings holding #77 remain open; sequential round-trips do not
prove exclusion. Follow the
[pinned roster plan](https://github.com/jhurliman/glsmac/blob/fa939625bc87de69d193c8e732e883807e8d3c93/docs/player-roster-ownership-plan.md).
World downloads are still not disk savegames or deterministic replay.

Conventional units have parsed component/design metadata and verified bounded
CVR decoding, composition, projection and raster calculations. Frame assembly
matches 30 installed-input cases and 1,755 records. The corrected explicit
transform calculator matches 44 cases, 660 binary32 words and 528 integers;
indexed rasterization matches 42 cases. Original lighting selection has
separate evidence in [#109](https://github.com/jhurliman/glsmac/pull/109).
Actual conventional-unit rendering, caller inputs, gameplay definitions and
commands remain unfinished. Component results do not establish visual fidelity.

### Next three implementation slices

1. Finish publication of the production profile and V4 world binding. Preserve
   strict early client rejection and actual body-restoration checks. Every
   changed PR head needs its own GitHub review and relevant checks.
2. Import the real SMAC technology catalog and classify facility prerequisites
   against it. Preserve identities and raw priority positions. Distinguish no
   prerequisite, disabled definitions, known technology and unavailable
   references; this is not a buildability decision. Synchronize host settings
   without inventing technology ownership or social ratings.
3. Establish actual faction and predefined-design startup producers, then connect
   founding to base creation. Inputs include owned technologies, selected social
   models/faction modifiers, current ratings, separately retained founding
   SUPPORT, active designs and cached costs. Retain original difficulty and
   world-size identities, and process initial creation before FirstTurn advances
   the counter. Connect support/intake, posting, completion and queues afterward.

Reverse engineering is prioritized around those consumers. New bounded founding
and upkeep compositions execute original target selection, resource/citizen
processing, completion and baseline writers with explicit supplied boundaries.
They establish ordering, including resource recalculation after a facility
grant, but do not prove normal startup selections or a whole game turn. The
first base → Scout/Former/Colony → second base loop still needs real producers,
legal commands and persistence checks. Native lifetime repairs and conventional
rendering continue in parallel. SMAC evidence must never silently become SMAX rules.

### Review and evidence discipline

GitHub Codex review became available again on October 17 after the retained
October 16 quota failures. New reviews identified x87-only rounding validation,
classification before binary32 underflow rounding, and case-sensitive asset
lookup. Each was fixed, checked against a reproducing regression, and received
a clean review on its corrected head. Dependency merges that change the head
receive another review. Each PR owns its current review/check evidence; a
request or local success is not a completed GitHub review.

Production, research, terraforming, conventional combat, AI, diplomacy, victories
and the expansion remain substantial work. Original executable evidence is
tracked separately from interactive PRACX observations. Original assets,
binaries and the researcher's uncommitted files are excluded from implementation
PRs. Golden 1.0.0 still requires every release gate above.
