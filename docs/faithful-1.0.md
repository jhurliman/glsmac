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

## Active presentation follow-up

The [Design Workshop implementation plan](design-workshop.md) tracks the user's
request for the complete functional, pixel-faithful unit customization screen.
A live editor now opens from Base production and saves retained designs through
authenticated commands. Its complete playable and pixel-faithful acceptance is
still open; see the current dashboard and the dated implementation receipts.

The user's honey playthrough exposed startup/menu, logical-resolution, terrain
seam and main-map visibility gaps. They are tracked with original asset sections
and measurable acceptance gates in [the playthrough audit](smac-playthrough-ui-audit.md).
The original interactive guided tour is explicitly excluded from the 1.0 release
requirement at the user's request (2026-10-19). Help and Datalinks remain required.

## Next gameplay integration

The [local faction controller plan](local-faction-controller.md) connects the
verified seven-faction Planet startup and per-owner upkeep boundaries to the
current engine. Computer admission, owner scheduling and actual decisions are
required before the local conquest milestone can be claimed.

## Current acceptance dashboard — 2026-10-21

Accepted integration: **`8865b021652cc8d0434812262d798be007b0e09c`**, through
[#533](https://github.com/jhurliman/glsmac/pull/533), on `codex/faithful-1.0`.
Its selected Linux regression/lifecycle and correctness-analysis checks passed
at head `1d7b78e993054988b6f707c2af31cb19375b3243`. This is an integration
checkpoint, not a release candidate or a statement that all platforms were rerun.

**Current focus: the complete Design Workshop (M3/M6). M0–M7 remain open.**
The next implementation tasks below preserve that focus. The local faction
controller and complete conquest scenario remain dependencies of M2; a growing
set of verified rules is not a completed match.

Evidence classes are deliberately separate:

- **Bounded reference match:** declared original inputs produce matching native
  fields or complete buffers. For example, the Workshop's29 general/stock
  static model frames match; this does not prove arbitrary drafts or animation.
- **Live caller test:** an authenticated queued operation changes a real backend
  fixture, including failure/restore checks. #502 verifies confirmed global
  design retirement and queue surgery; it is not a human-played match.
- **Rendered preview:** an actual rendered UI/scene, with harness overrides
  disclosed. The #502 example confirmation capture proves layout, not an
  interactive full-bank retirement or whole-screen original pixel parity.
- **End-to-end playable acceptance:** the complete named scenario through normal
  setup, UI/gameplay, save/reload and result. The Workshop receipt and M2
  local/network match have **not run to completion** at this checkpoint.

Workshop selection, saved-design browsing, component/ability pickers, Rename,
Obsolete, Retire, Upgrade, redesign/intrinsic confirmations and the live preview
controller now have implementation receipts in the [Workshop journal](design-workshop.md).
The ordinary action buttons match all 14,700 compared native pixels. Live
rotation, stop and reopen pass an actual instrumented SDL session; five declared
model frames and the bounded cached-sprite output match complete native buffers.
These receipts do not establish whole-screen parity or the complete human-played
manufactured Unit path. First custom prototype completion is now integrated
in#531: a queued saved design is manufactured with its one-time morale bonus
and native design-history changes, and checkpoint replay matches uninterrupted
repeat production. #533 connects admitted High Morale and the successful
newborn-state stores against15 native controls. The allocation-limit aftermath
is the current follow-up; complete original Unit/Base equality and the complete
human-played/remote manufacturing receipt remain open.

| Acceptance | Status at the accepted head | Evidence and remaining exit requirement |
| --- | --- | --- |
| M0: foundation/platform gate | Partial | Focused CI and earlier native Mac interaction passed; [development policy](#analysis-and-development-loop-policy) applies. Fresh release-platform full suites, hardware graphics/audio pacing and packaging receipts remain required. |
| M1: persistent simulation gate | Partial | [Campaign saves](local-campaign-save-load.md), [UI cursor persistence](campaign-ui-cursor.md), [faction continuation](faction-turn-continuation.md), and live Workshop whole-world restore tests exist. Complete cross-platform uninterrupted/replay comparison, remote late join/reconnect and migration acceptance are not established by these bounded tests. |
| M2.1: normal setup, starting roster and first Base | Partial | Local-human conventional startup/founding has live callers. [Faction-controller admission/scheduling](local-faction-controller.md) is still required; independent seven-faction startup records do not create a working opponent. |
| M2.2: workers, growth and starvation | Partial | Worker/upkeep and shortage fixtures exercise live state. Repeat them inside the complete two-faction scenario without stale claims; no complete scenario receipt exists. |
| M2.3: Scout/Former/Pod production and accounting | Partial | [Canonical production selection](canonical-production-selection.md) and [Colony production](smac-colony-production.md) are bounded live paths. Complete the scenario's support/maintenance/production checkpoints and usable manufactured Units. |
| M2.4: research Centauri Ecology | Partial | [Live research](smac-live-research.md) and [choice continuation](research-choice-ui.md) exist. The full scenario must demonstrate the resulting availability changes through gameplay. |
| M2.5: farm/mine/solar/roads | Partial | The [terraforming catalog](smac-terraforming-catalog.md) and live Former paths have focused evidence. Finish the complete scenario's job times, yields, cancellation, legality and road movement checks. |
| M2.6: second Base and Pod consumption | Partial | Founding and [colony-choice continuation](smac-colony-choice-continuation.md) have live fixtures. Verify population/ownership/support together in the complete scenario. |
| M2.7: combat, capture, elimination and valid conquest | Partial | [Hostile movement](smac-live-scout-combat.md), [defended capture](smac-defended-base-combat.md) and the [conquest-controller contract](behavior/conquest-controller.md) cover bounded paths. A legal opponent, full remaining-Unit/Pod disposition and actual match ending remain required. |
| M2.8: every checkpoint reload, remote repeat and late join | Not run as the complete acceptance | Local restoration fixtures are not the numbered uninterrupted-vs-restored match or its network repeat. Retain this as an explicit M2 exit gate. |
| WS1: open, browse/New, rename, reselect, Apply/Done/Cancel | Partial | [Custom-edit](behavior/design-workshop-custom-edit.md) and [fresh-draft](behavior/design-workshop-fresh-draft.md) reference contracts exist; queued save/retirement paths pass. Normal-open/browse and fenced reselection/Rename paths are connected; Apply/Done/Cancel and original warnings have live receipts. Complete the combined named-design gameplay receipt, preference persistence and original input/screen inventory. |
| WS2: all legal components, previews, costs and abilities | Partial | [Workshop journal](design-workshop.md) separates exact static compositions, card controls, general record construction and live integration. Stock component/ability pickers, cleanup, PSI formatting, DATA/COST and live animation are connected. Ability-dependent model composition, advanced/SMAX cases, preferences and complete pixel/interaction comparisons remain open. |
| WS3: new named design to usable manufactured Unit | Not run as the complete acceptance | Follow the journal's minimum receipt through Apply, close/reopen, save/reload, Base production, first prototype and a playable Unit. Price/registry tests alone do not close general production admission. |
| WS4: original full Workshop screen and interaction diff | Not run as the complete acceptance | Exact declared CVR buffers and UI layout previews are supporting evidence only. Match the original parent/console, artwork, glyph advances, all picker/confirmation states and fixed animation phase at logical4:3 before claiming screen parity. |
| M3: complete SMAC rule/content inventory | Partial | Many bounded rule families are implemented; Workshop WS1–WS3 and the full inventory/interaction reconciliation remain open. No full campaign receipt. |
| M4: AI, diplomacy and all SMAC outcomes | Not run as complete campaigns | [Local controller plan](local-faction-controller.md) still requires live per-owner dispatch and legal decisions. Seeded AI campaign/hidden-information/automation/victory quality gates remain open. |
| M5: complete Planetary Pack/SMAX | Not run as complete campaigns | A distinct ruleset route is not full expansion gameplay. Expansion content inventory, alien rules, scenarios, complete campaigns and SMAC nonregression remain required. |
| M6: complete presentation/input/AV | Partial | [Playthrough audit](smac-playthrough-ui-audit.md), [ordinary exploration](ordinary-native-exploration.md) and [generated-terrain acceptance](generated-terrain-appearance.md) record supported profiles and remaining boundaries. General generated/flat/SMAX live authority, populated actor/font/UI comparisons and native hardware AV receipts remain open. The guided tour is excluded. |
| M7: packaged golden1.0.0 | Not run | All [golden release gates](#golden-release-gate) remain open: campaign/platform/soak/network/AI/content evidence must be pinned to actual packaged binaries and a release commit. |

There is no current external blocker that justifies marking the project blocked.
“Partial” means supporting bounded evidence exists; it never means the listed
exit test passed. Historical checkpoints below retain their original scope and
dates, and must not be read as current implementation inventories.

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
| M6 — Presentation and compatibility | Every functional screen/dialog and preference; keyboard/mouse behavior; readable/resizable UI; help/datalinks (guided tour excluded); original movies, interludes, sound and music; accessibility and platform integration | Screen/interaction inventory reviewed against originals; no placeholder values; assets resolve on supported installations; audio/video/input tests across platforms |
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

## Next three tasks and their acceptance targets

1. **Close manufactured Unit-state gaps — WS3/M2.3.** Connect the admitted
   High Morale bonus with training/current MORALE/prototype ordering and clamp,
   retain the original produced-state flags, and establish allocation-limit
   aftermath. The successful first/repeat/history/checkpoint path is integrated
   in#531. The captured allocation-failure prefix still does not prove a refund
   or atomic rollback. Extend actual gameplay package admission independently.
2. **Complete Workshop rendering and interactions — WS2/WS4.** Connect the
   source-confirmed ability models and Artillery composition ordering in main
   and saved-strip scenes. Match selected/hovered picker/modal states, parent
   ability decoration, preference persistence and remaining Help/input behavior.
   Compare complete native frames at fixed phase and logical 4:3 resolution.
3. **Close the playable Workshop receipt — WS3, contributing to M1/M6.** Take
   a newly named edited design through Apply, close/reopen, save/reload,
   production selection, first prototype and an actually usable manufactured
   Unit. Repeat the relevant network path. Targeted tests do not substitute
   for this normal gameplay/UI receipt.

The complete M2 local/network conquest scenario and the M0–M7 release gates
remain required after this focused Workshop work. Do not restart obsolete
integration queues or infer promotion of the integration branch to main.

Presentation discrepancy tracking: [4:3 viewport and title-menu comparison](four-three-presentation.md).
Correct aspect and successful rendering do not establish original UI parity.

## PR and review workflow

Development uses cached incremental builds, focused affected tests and targeted
correctness analysis. Ownership/serialization changes get focused sanitizers.
Full Linux/Windows/macOS runs belong at platform-sensitive changes, meaningful
integration or playable milestones, and release gates; they are not a gate on
every gameplay tweak. Reuse existing CI and unchanged-source evidence instead
of duplicating runs. Spend implementation effort on connecting complete gameplay
loops; keep validation notes proportionate to the change.

- Use small `codex/` branches targeting the fork. Keep upstream-facing portability
  contributions separate from the larger gameplay effort.
- Each PR describes the observable problem, the resulting behavior, the reference
  evidence when applicable, and tests actually run. Mark unrun checks explicitly.
- Use **Codex code review on GitHub** when credit is available. Record the
  reviewed head and resolve findings. While credit is exhausted, the user's
  explicit waiver removes review as a merge gate: continue focused validation
  and relevant CI without repeatedly requesting quota-failed reviews or waiting
  for credit. Never report an unavailable review as approval.
- Keep changes independent where possible. For stacked PRs, name the dependency
  and use the preceding branch as base so reviewers see only the new slice.
  Retarget after the dependency merges; rerun affected validation.
- Merge only when relevant checks and any actual review findings are resolved.
  Guard the merge against the validated head commit. Never merge
  merely to move the queue or to make a milestone appear complete.
- `.gls.js` is GSE, not JavaScript: follow `AGENTS.md` and
  `tools/js-vs-glsjs-cheatsheet.md`, then run the actual engine's tests.
- Preserve the existing architecture until evidence justifies a focused change.
  Performance changes need measurements; rules changes need reference cases.

### Visual progress and original comparisons

Share actual engine screenshots at visible gameplay/UI milestones, labeled with
platform and build. Backend-only changes need their focused checks, not a fresh
interactive platform matrix. Preserve the supplied Mac menu/map observations as
historical baselines. Compare against the original executable with matched
resolution, UI scale, camera, map and game state; otherwise image differences
cannot establish fidelity. Begin with menus and Base screens, then compare
matched terrain, unit and capture states. Keep original and PRACX references
identified separately and exclude proprietary assets from published fixtures.

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
Shared-header changes include known affected consumers in the focused scan;
widen to the full production scan when the dependency boundary is unclear.
Routine gameplay work uses cached incremental builds and affected tests. Full
platform suites are reserved for platform changes, playable integration
milestones and releases. The scoped CI policy is in
[PR #168](https://github.com/jhurliman/glsmac/pull/168).

## Historical checkpoint — 2026-10-17

M0 and M1 remain in progress. There is no complete conquest match, and no
milestone is declared complete. The integration branch combines tested slices
before their smaller PRs land; its contents must not be confused with main.

The accepted integration now connects [default conventional startup](default-conventional-startup.md),
editable workers, [production selection](canonical-production-selection.md),
Scout/Former/Colony manufacture, second-base founding, SUPPORT and shortage
losses, starvation and home reassignment. [Research and discovery](smac-live-research.md)
have a [player choice continuation](research-choice-ui.md); saved Former orders
drive terrain work. [Local campaign saves](local-campaign-save-load.md) have a
[save/load UI](campaign-save-load-ui.md). These are bounded live systems with
their own admission limits, not proof of complete campaign or network acceptance.

Recent reviewed merges add HQ relocation and Energy Bank/Network Node
construction ([#185](https://github.com/jhurliman/glsmac/pull/185)), individual
facility fees and insolvency sales ([#186](https://github.com/jhurliman/glsmac/pull/186)),
and friendly occupied movement ([#188](https://github.com/jhurliman/glsmac/pull/188)).
[Hostile Scout movement](smac-live-scout-combat.md) merged in
[#189](https://github.com/jhurliman/glsmac/pull/189) after current-head review
and the relevant hosted checks passed. It joins actual casualties, saved hostile history,
mode1 morale and subsequent movement; it does not implement Base capture.

Frontend ownership refresh merged in
[#190](https://github.com/jhurliman/glsmac/pull/190), retaining stable Base identity,
owner badges/labels and navigation membership. The saved post-upkeep faction
population aggregate merged in [#191](https://github.com/jhurliman/glsmac/pull/191).
[Ordinary undefended capture](smac-base-capture.md) merged in
[#192](https://github.com/jhurliman/glsmac/pull/192); its live transfer, loot,
facility draws, remote home repair and actual Start tests pass. It remains a
bounded population2 first-capture slice. Assimilation upkeep and ordered Base
screen entry merged in [#193](https://github.com/jhurliman/glsmac/pull/193) and
[#194](https://github.com/jhurliman/glsmac/pull/194), after current-head review and
relevant CI. [Human-defeat ending requests](smac-human-defeat-request.md) merged
in [#195](https://github.com/jhurliman/glsmac/pull/195). The retained geography,
site-query and original candidate-search dependencies merged through
[#206](https://github.com/jhurliman/glsmac/pull/206).
[Last-Base capture disposition](smac-last-base-capture-plan.md) merged in
[#207](https://github.com/jhurliman/glsmac/pull/207), joining post-loot
initialization, Unit cleanup/replacement, future worker/resource queries and
successful restart tracking. Its current-head Codex review and relevant CI pass;
94 affected native groups and focused leak-enabled sanitizers support the joined
transaction. This does not establish final conquest or a full match.

Headquarters capture and paid relocation merged in
[#208](https://github.com/jhurliman/glsmac/pull/208), including population3-to2,
future resource/support queries and atomic facility publication. Full Planet
rendering has also been captured with seven provisional faction Bases; this
exercises the current renderer, not original contour/shading parity or a
complete seven-faction campaign.

The [defended-Base combat integration](smac-defended-base-combat.md) connects
garrison combat to the separate capture caller, frozen source combat percentages,
saved defender-loss aftermath and Base upkeep cleanup. Its original17-round
receipt and live save/Start sequence are separate from global ending semantics.

The next three actionable slices are:

1. Extend the admitted combat/capture interaction to the remaining M2 terrain,
   garrison, population1 and recapture cases, using original-game evidence.
2. Connect the original global ending controller and survivor/conquest branch.
   Retain the distinction between a human-loss request, the active roster,
   retained ranking reference and local focus. Establish the necessary owned
   ranking/role inputs; zero Bases alone does not establish a conquest winner.
3. Compose defended capture, disposition and the appropriate ending through the
   M2 save checkpoints. Remove the remaining command/roster restrictions needed
   by an actual match, then exercise it through network authority; local human
   tests alone do not satisfy M2.

The earlier [campaign-upkeep checkpoint](validation/campaign-upkeep-2026-10-17.md)
retains its 112-group affected Linux evidence. The historical checkpoints below
describe their recorded revisions, not the current completion state. Full SMAC,
SMAX, strategic AI, presentation and release gates above remain required.

The earlier live-worker and movement integrations at `50d43f9f` and `fa159ff8`
retain their 16-group and 59-group focused Linux results. Their later production,
founding and upkeep consumers are covered by the campaign-upkeep receipt above.

The latest [unit, Rover and resource receipt](validation/unit-rover-resources-2026-10-17.md)
records CTest 8/8 on Windows, leak-enabled Linux and native macOS arm64 at
`d6afde1a`, plus zero failures across 336 production analysis units. Conventional
Unit state and paired gameplay RNG are owned and restored with checked context;
fixed Rover presentation reaches real unit previews, and resource text follows
measured physical-row semantics. The subsequent movement implementation in
[the bounded command receipt](conventional-empty-ground-movement.md) connects the
real event to Unit/tile/RNG authority and exact continuation. Its source passes
full normal and leak-enabled sanitizer suites. The subsequent
[combined movement receipt](validation/conventional-movement-2026-10-17.md)
records Windows and Linux CTest 8/8 and 18 tracked groups at `5bc62766`.
The subsequent [startup/HQ checkpoint](validation/startup-hq-2026-10-17.md)
at `5ed286fd` adds the restricted Infantry/HQ route and actual raw-startup
readiness lifecycle. Windows, leak-enabled Linux and native macOS pass CTest
8/8; 50 tracked groups pass and the full 337-unit analysis has zero failures.
Default conventional startup remains a follow-up.
The [Rover/HQ follow-up](validation/rover-hq-2026-10-17.md) at `b2deeaca`
adds the measured selected Spartan Rover route; 53 tracked groups pass on
Windows and leak-enabled Linux, with zero failures in the changed-source scan.
It does not claim a new three-platform full-suite checkpoint.

The preceding [social and movement receipt](validation/social-movement-2026-10-17.md)
records CTest 8/8 on Windows, leak-enabled Linux and native macOS arm64 at
`f0b1ee2e`, plus zero failures across 332 production analysis units. Owned
social continuation refreshes before the first authoritative turn hooks;
settings and world snapshots retain frozen movement Q. These values are ready
for conventional Unit consumers; they do not yet drive complete live commands.

The preceding [startup and combat receipt](validation/startup-combat-2026-10-17.md)
records CTest 8/8 on native Windows, leak-enabled Linux and native macOS arm64
at `445eba1d`, with zero failures across 328 production analysis units. Owned
startup designs now accompany simulation identities and technology through
actual startup and exact restore (simulation V2/world V10). Scout render
descriptors reach real Unit and registered preview consumers. Separate pure
components reproduce the supplied gameplay RNG, neutral Scout combat, and
conventional morale/allowance queries; these do not yet drive live commands.
Each publication still has its own dependency, review and CI gates.

The preceding [live simulation receipt](validation/live-simulation-2026-10-17.md)
records CTest 8/8 on native Windows, leak-enabled Linux and native macOS arm64
at `39172a7d`. The actual claimed Game startup and restore paths now own explicit
simulation faction identities and mutable technology state; callbacks receive
copies, and world V9 preserves exact saved IDs and progress. This checkpoint
also contains the owned Scout image producer and the original signed32
base-center correction. Live Scout actors/previews and conventional gameplay
remain separate work, and these integrated changes have their own PR gates.

The preceding [unit-profile/screen receipt](validation/world-unit-profile-2026-10-17.md)
records CTest 8/8 on native Windows, leak-enabled Linux and native macOS arm64
at `58197e64`, plus zero failures across 313 Linux analysis translation units.
It adds frozen predefined-unit profiles in GlobalSettings V7/world header V8,
the canonical Client packet-budget guard, ready-lobby packet ordering, and the
complete base-screen signed-endpoint repair. The subsequent admitted snapshot
Failure/Start correction has its own validation and publication gate.
The preceding [live-roster/startup receipt](validation/live-roster-startup-2026-10-17.md)
records CTest 8/8 on native Windows, leak-enabled Linux and native macOS arm64
at the same integrated commit, `75c280c8`. It includes live canonical roster
publication, frozen faction profiles in the V7 world header, the owned predefined
unit producer and production resource-overflow regressions. The preceding
[world-size receipt](validation/world-size-startup-2026-10-17.md) retains the
accepted preset/custom identity checkpoint. The
[technology/startup receipt](validation/technology-startup-2026-10-17.md)
retains the earlier catalog, initial-turn and Server retirement checkpoint.
The [preceding production-profile receipt](validation/production-profile-2026-10-17.md)
retains the effective SSE rounding guard's focused Windows/Linux tests. On honey, the user confirmed
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
rule values and all 71 facility records in host settings;
[#112](https://github.com/jhurliman/glsmac/pull/112) binds that profile in the V4
world header before map allocation. [#114](https://github.com/jhurliman/glsmac/pull/114)
adds all 89 positional technology definitions and their original key lookup and
prerequisite classification. [#116](https://github.com/jhurliman/glsmac/pull/116)
binds the technology catalog in V5. These are published, reviewed slices awaiting
their dependency merges; see the pinned
[world snapshot rules](https://github.com/jhurliman/glsmac/blob/445ee2094331babdab7e82528eb00f028a65ff26/docs/world-snapshot-rules.md).
SMAX has no SMAC numeric profile. Definitions alone grant no discoveries or
buildable items. Actual faction/design startup producers precede production
activation. [#113](https://github.com/jhurliman/glsmac/pull/113) runs initial
world events at turn zero, before the first turn advances.

[#121](https://github.com/jhurliman/glsmac/pull/121) adds the five frozen SMAC
world-size rows and explicit preset/custom acceptance to the actual setup and
download paths. It preserves custom category 99 at preset geometry and checks
the actual tile dimensions before map allocation. [#120](https://github.com/jhurliman/glsmac/pull/120)
parses the seven canonical faction files against the technology catalog,
preserving ordered bonuses and the original eight-record projection. It does
not apply faction effects. [#123](https://github.com/jhurliman/glsmac/pull/123)
freezes these profiles in host settings, and
[#124](https://github.com/jhurliman/glsmac/pull/124) binds them to downloaded
worlds. Both are integrated and published for dependency review.
The positional RESOURCEINFO correction landed in
[#118](https://github.com/jhurliman/glsmac/pull/118): row position selects the
numeric rule, while labels may be renamed or repeated.

The wider lifetime stack preserves entity state, RNG continuation and turn
metadata, stages map publication, and guards specific native replacement/cleanup
paths from collection. Terminal-connection cleanup is
[#110](https://github.com/jhurliman/glsmac/pull/110), followed by Server departure
retirement in [#115](https://github.com/jhurliman/glsmac/pull/115). The integrated
live roster now keeps State-owned Players stable through metadata refreshes,
resolves selected factions canonically and copies bounded notifications with
captured epochs. Twenty real packet/notification/collector tests exercise that
boundary. The combined canonical-roster/startup correction is published in
[#126](https://github.com/jhurliman/glsmac/pull/126), including real claimed
Failure/Start and pending/running cancellation checks. Its refreshed publication
passes normal and leak-enabled sanitizer suites after the separate
[#66](https://github.com/jhurliman/glsmac/pull/66) switch-scope ownership fix.
Those PRs still require their current-head review and CI gates; #77 and #79
remain held individually. Follow the
[pinned roster plan](https://github.com/jhurliman/glsmac/blob/fa939625bc87de69d193c8e732e883807e8d3c93/docs/player-roster-ownership-plan.md).
The owned value codecs landed in [#119](https://github.com/jhurliman/glsmac/pull/119);
both live writers and decoders now switch together in the integration. Its
[pinned publication contract](https://github.com/jhurliman/glsmac/blob/75c280c857d16a40c3651c8a06b5cd8d89480012/docs/live-roster-publication.md)
records exact identity, lifetime, protocol and concurrency limits.
World downloads are still not disk savegames or deterministic replay.

[#125](https://github.com/jhurliman/glsmac/pull/125) constructs owned predefined
SMAC designs from source components, abilities and technology references. It
matches the retained original final records and calculated cost/capacity outputs;
it does not create live unit definitions or faction custom designs.
Conventional units also have verified bounded
CVR decoding, composition, projection and raster calculations. Frame assembly
matches 30 installed-input cases and 1,755 records. The corrected explicit
transform calculator matches 44 cases, 660 binary32 words and 528 integers;
indexed rasterization matches 42 cases. Original lighting selection has
separate evidence in [#109](https://github.com/jhurliman/glsmac/pull/109).
The reverse-engineering handoff now captures a complete original internal
reactor-1 Scout draw, including grouped geometry and indexed/depth buffers.
Twelve complete original draw/copy/cleanup cases also establish palette index 9
as the transparency key. Independent generation now reproduces all 51,456
first-lookup bytes, and bounded original cases establish palette decoding,
view construction and the per-cell extended-precision matrix schedule. The
owned Scout image producer is implemented in
[#133](https://github.com/jhurliman/glsmac/pull/133). Native Windows Release and
macOS Release with UBSan each match all 19 original drawing views, including
indexed/depth buffers, generated lighting and the baseline keyed RGBA image.
The twentieth original case is target-culled and excluded from pixel matching.
Live actor/cache/preview integration is implemented in
[#138](https://github.com/jhurliman/glsmac/pull/138), including retained texture
and presenter callback lifetimes. The integrated original-asset Unit/preview
smoke passes on Windows and honey with Null graphics and synthetic badges;
this is not a new interactive GPU or pixel-comparison receipt.
Actual conventional-unit gameplay definitions and commands are unfinished.
Component results do not establish visual fidelity.

Owned technology state is published in
[#131](https://github.com/jhurliman/glsmac/pull/131). Its
[live binding](https://github.com/jhurliman/glsmac/blob/39172a7d4df8b79d46224cafb45b1d3970469859/docs/smac-live-simulation-state.md)
is now integrated: authoritative startup allocates IDs once before create-world
callbacks; restores preserve explicit saved IDs, ownership and progress without
replaying grants. Canonical Player tokens, the roster epoch and Game session
guard access. Tests cover sparse slots, permuted/non-dense saved IDs, retained
reconnect, replacement rejection, cancellation and real Client prefix failures.
This does not activate research upkeep or create conventional units.

The base-center signed32 correction landed in
[#134](https://github.com/jhurliman/glsmac/pull/134): wrap each original ordered
operation before its sign checks/clamps. The Release/UBSan probe matches all
4,167 values in 1,389 retained original scenarios; removing wrapping differs in
422 scenarios. This calculator result does not claim reachability of synthetic
extreme social/resource inputs or a complete base economy.

### Historical gameplay batch and implementation slices

The [Former/controller integration](validation/former-controller-2026-10-17.md)
now passes its affected cached Linux checks, including ordinary work, owned
terrain patches and original selected-unit continuation. Faction selection,
visible unit commands, energy presentation and campaign save/load are the next
first-session changes; their source handoffs are not yet interactive receipts.

The [research integration checkpoint](validation/research-terrain-2026-10-17.md)
connects actual Base labs, required manual choice, completion, Secrets and map
revelation to suspended native upkeep. An answer resumes credit and eventual
faction debit settlement exactly once. The frozen TERRAIN catalog is also
integrated. The cached Linux build passes 82 affected native groups and the
research UI script; the subsequent preview-retirement and arithmetic-boundary
changes pass their two affected groups. These are focused checks, not a new
platform matrix or interactive playthrough.

Current update after PR #183 (accepted commit `3f76f742`): faction/difficulty
selection, Former work and controls, required research choice, campaign disk
save/load, Colony choices, last-base restart/elimination and refounding are
connected. PR #183 passes completed GitHub Codex review, regression/lifecycle/
correctness CI, 72 affected native groups, two GSE scripts and ten focused
ASan/UBSan groups. A restart → founding → world save/Start → next upkeep
composition matches its measured treasury/research and production boundaries.
These are engine/script checks, not a completed human playthrough or M2 exit.

1. Continue playable production/economy slices using existing rule evidence.
   [Recycling Tanks construction](smac-recycling-tanks-production.md) is merged
   in PR #184 after completed Codex review and relevant CI, with real picker →
   saved ordinary turns → completion coverage. PR #185 joins HQ relocation and
   Bank/Node production to their current energy consumers; PR #186 joins ordered
   maintenance insolvency. Both have completed review and relevant CI.
   Node/Artifact link state, citizen-status facilities and conventional defense
   effects remain separate dependencies. The subsequent maintenance continuation
   now joins per-facility charges, ordered insolvency sales and retained research
   to actual upkeep, with five composed scenarios and saved inventory/treasury.
   [The original contract](behavior/facility-maintenance-insolvency.md) records
   the measured human branch and static-only exceptions. Hurry spending and
   the charged-maintenance statistics consumer remain separate dependencies.
   [Friendly stack movement](smac-friendly-stack-movement.md) now connects normal
   directional entry/departure at occupied friendly Bases, including saved sight
   occupancy and actual Start restoration. Occupied fungus and hostile combat
   still need their own caller continuations.
2. Keep the [visually reviewed first session](first-session-checkpoint.md) as an
   integration acceptance task while gameplay development continues. It covers
   normal setup, units/commands, Base/production/research screens and disk
   save/reload. Current rendered validation remains incomplete; older Mac smoke
   receipts apply only to their recorded revisions. Use platform builds at the
   combined presentation checkpoint, not for every gameplay tweak. Load Game is
   implemented through PR #175; its rendered menu workflow still needs review.
3. Close M2 with actual opposing-faction control, conventional attacks, capture,
   conquest and network replay. Current live attack entry points explicitly
   reject conventional Units. The saved roster represents human network roles;
   it has no AI-controller identity. The retained neutral Scout calculator is
   limited to its measured Human/AI, Vendetta, flat/no-base context. Connect
   authenticated controller/diplomacy inputs and the real command/retirement
   callers rather than treating calculator success as a playable battle.
   Base capture and victory need their own original caller evidence. M2's
   two-human local/network acceptance and the initial legal AI opponent remain
   required; full SMAC, SMAX and presentation gates are unchanged.

The [worker-authority checkpoint](validation/worker-authority-2026-10-17.md) at
`dacac744` passes all eight tests on Windows, Linux Debug, Linux sanitizers and
native macOS, with the same 154 affected allocation-tracked groups on Windows and
both Linux builds. Targeted analysis of both new production units has zero
failures in 1.82 seconds. Lossless Base storage preserves 49 captured records and
six original allocator continuations; the owned worker frame matches 343 Base
queries. These values are detached. Optional live Base authority, atomic
population publication and actual entity-bound map knowledge are next in the
[live integration plan](smac-live-worker-integration-plan.md).

The [complete detached allocator checkpoint](validation/automatic-worker-2026-10-17.md)
at `df61f015` passes all eight tests on Windows, Linux Debug, Linux sanitizers and
native macOS, with the same 144 affected allocation-tracked groups on Windows and
both Linux builds. The full 350-unit correctness scan has zero failures. The
allocator matches 51 complete original returns; availability matches 736 masks.
The later worker-authority checkpoint supplies the detached storage values;
live knowledge and allocation publication remain pending. This does not activate
a default canonical campaign.

The [live-intake and worker-stage checkpoint](validation/worker-intake-2026-10-17.md)
at `ce47b2b1` passes all eight tests on Windows, Linux sanitizer and native macOS,
with the same 111 affected allocation-tracked groups on Windows and Linux and
zero failures in the full 346-unit correctness scan. It connects canonical gross
intake and actual-owner specialist queries, and adds detached automatic worker
scoring/assignment stages. Full allocation and default campaign activation remain
pending. Existing collection is validated separately for the next integration.

The [base-intake foundation checkpoint](validation/base-intake-foundation-2026-10-17.md)
at `990cc59a` passes all eight tests on Windows, Linux sanitizer and native macOS,
with the same 87 affected allocation-tracked groups on Windows and Linux and zero
failures in the full 342-unit correctness scan. It adds Base context, facility
identity, generated-map resource semantics and movement history. Default
canonical base economy and native encounter activation remain pending. The
[preceding resource-context checkpoint](validation/resource-context-2026-10-17.md)
retains its separate 146-group regression receipt.

The host/world profile and startup-design foundations are implemented; focused
publication and review remain separate from integration. Review quota delays
must not broaden PRs or turn old reviewed heads into approval for new commits.

Reverse engineering is prioritized around those consumers. Bounded original
compositions now cover all seven human factions through first-base creation,
late intrinsic SOCIAL, and the first upkeep. The first base selects Formers for
Gaians and Scout Patrol for the other six at the tested difficulty; the Spartan
placed starting Rover is distinct from its base's production target. Two
resource passes bracket production, baseline commit and population processing;
the second pass does not post minerals again. Explicit supplied terrain, naming,
UI and regional services limit these results. They do not establish full
generated-world setup, rendered first-input parity or a complete campaign. The
first base → Scout/Former/Colony → second base loop still needs real producers,
legal commands and persistence checks. Native lifetime repairs and conventional
rendering continue in parallel. SMAC evidence must never silently become SMAX rules.

The fresh original data-setup receipt now covers constructors, software palette
initialization, 107 model loads and data setup through normal return. Its one
supplied clock/cache fixture records seven shared draws after reseeding; later
cache time reads do not reseed. Palette-ID retries prevent treating seven as a
universal count. Complete first-command and successive-game RNG origins remain
open, so the port retains its explicit new-world entropy policy.

The pure Scout combat implementation in
[#142](https://github.com/jhurliman/glsmac/pull/142) matches 187 supplied original
core/normal-return cases, 3,073 rounds, 6,194 CRT draws and three subsequent
command histories. The independent two-word gameplay RNG codec is in
[#139](https://github.com/jhurliman/glsmac/pull/139). Combat retains the attacker
at its source after victory; original wrapper return zero is distinct from
defeat. The ordinary keyboard gate rejects exhausted Scouts even though a
direct diagnostic battle call can reach combat. Original new-game/save RNG
provenance and live command integration remain open.

Owned startup design state in [#141](https://github.com/jhurliman/glsmac/pull/141)
matches 1,008 human difficulty/owner cases and nine baseline projections, with
exact codec roundtrips. Conventional stat queries in
[#140](https://github.com/jhurliman/glsmac/pull/140) match 9,484 selected original
outputs; basic allowance remains an explicit input in that slice. Controlled
original startup/controller evidence establishes the first-command morale
refresh, without proving or inventing an earlier retained-zero reset. These
bounded components are not a complete conventional unit or campaign.

### Historical review and evidence discipline

The October 19 review-credit waiver above supersedes the waiting requirement
in these historical notes. Actual unresolved findings still require fixes.

GitHub Codex review became available again on October 17 after the retained
October 16 quota failures. New reviews identified x87-only rounding validation,
classification before binary32 underflow rounding, and case-sensitive asset
lookup. Each was fixed, checked against a reproducing regression, and received
a clean review on its corrected head. Dependency merges that change the head
receive another review. Each PR owns its current review/check evidence; a
request or local success is not a completed GitHub review.

The review usage quota was reached again earlier on October 17. Those failures
are historical evidence, not a claim about every later PR: review subsequently
completed without findings for PRs #180–183. A current head still waits for its
own completed review and relevant CI; repeated quota requests never substitute
for review. Implementation continues independently while reviews run.

Production, research, terraforming, conventional combat, AI, diplomacy, victories
and the expansion remain substantial work. Original executable evidence is
tracked separately from interactive PRACX observations. Original assets,
binaries and the researcher's uncommitted files are excluded from implementation
PRs. Golden 1.0.0 still requires every release gate above.

## Historical checkpoint — 2026-10-19 (through #369)

Integration checkpoint: `bd931ed7` (through [#369](https://github.com/jhurliman/glsmac/pull/369)).
These notes describe that historical head, not current feature absence. The
current dashboard above supersedes their status and queue claims. No milestone
or release gate was declared complete at this head.

- Bounded conventional movement, founding, production, research, terraforming,
  combat and capture paths exist. Individual rule/caller tests do not establish
  the complete M2 local/network conquest scenario. Ordinary startup still has
  one local human; network/SMAX startup retains a placeholder path. A legal
  opponent and complete match remain required.
- Native terrain now retains view-specific geometry/cache history, uses measured
  painter order and anchors objects to their own projector calls
  ([#356](https://github.com/jhurliman/glsmac/pull/356),
  [#358](https://github.com/jhurliman/glsmac/pull/358)).
  [#359](https://github.com/jhurliman/glsmac/pull/359) matches all eight complete
  original indexed viewport frames and paint-store counts under ASan/UBSan:
  first/repeat/east/back for a diagnostic observer and a valid faction control.
  See [comparison scope and commands](behavior/viewport-pixel-comparison.md).
  These controls have empty rosters and explicit allocation/visibility inputs;
  they do not establish complete game screens, fog, arbitrary cameras or GPU parity.
- The Detail minimap has observer-aware terrain geometry and navigation (#357),
  with Base body, defense/HQ and population controls (#360). Main-map Base
  metadata (#362), captured Base art from its former owner (#364), native
  population geometry (#366), and full-canvas CVR body placement (#367) are
  integrated. Complete label/font parity, unit shadows and a matched populated
  original viewport remain open. Native terrain replacement remains behind the
  inspector flag; the ordinary map is not yet a proven original viewport match.
- The live dominance graph uses saved native faction identities/history (#345).
  Whole movement no longer prints a decimal suffix (#361). The lower console
  now uses the native flat-tile preview painter and owned texture cache (#365):
  seven original controls pass, and an actual bare-tile upload matches all
  1,600 opaque framebuffer pixels. Optional inspector geometry is excluded from
  ordinary preview snapshots. Actual selected/roster Scout body dimensions are
  15 by 33 and 11 by 23 pixels; full console composition, aircraft state and
  remaining console actions are not established by those measurements.
- Retained conquest distinguishes liberation to the former owner (#368).
  Ordinary population-one capture removes a Base rather than retaining it at
  zero population (#369), including homed Unit repair, territory, loot, history,
  rollback and actual Start reload. Last population-one Base disposition is in
  [#370](https://github.com/jhurliman/glsmac/pull/370), not integrated at this
  checkpoint. Its original reference ends at a window-cleanup boundary; full
  ending UI and executed movement-return parity remain unproven.
- Earlier Mac interactive checks passed. Focused checks and selected CI do not
  replace release hardware, frame pacing, audio or complete campaign acceptance.
- GitHub Codex review credit is exhausted; the user waived that merge requirement.
  Focused correctness checks and relevant CI continue.
