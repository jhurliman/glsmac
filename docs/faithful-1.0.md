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

The latest [live simulation receipt](validation/live-simulation-2026-10-17.md)
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
Live actor/cache/preview integration remains in progress, including retained
texture and presenter callback lifetimes.
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

### Next three implementation slices

1. Land the refreshed canonical-roster/startup publication with its separate
   switch-scope prerequisite after exact-head GitHub review and CI. Then land
   the already reviewed host/world profile chain and live simulation binding
   without dropping shared test registrations during dependency merges.
2. Connect the installed-asset Scout image to real Unit/GetSprite and preview
   consumers. Retain the image and copied badge textures through preview actor
   destruction; retire presenter callbacks safely and verify actual registration,
   teardown and reattachment. Keep the existing native-unit path operational.
3. Add owned faction custom-design blocks keyed by the admitted simulation IDs,
   with persistence and exact restore rather than regeneration. Extend original
   startup evidence across difficulty/network/owner contexts before wiring the
   generator into ordinary quickstart. Map the port's canonical difficulty
   identities explicitly to original indices, not its current -3..2 values.
   Then connect founding to base creation. The observed first-base current SUPPORT
   and INDUSTRY are zero; late faction modifiers precede first upkeep and do
   not retroactively recompute starting minerals. Preserve independent target,
   stored minerals and switching baseline, then connect support/intake,
   posting, completion and queues in their evidenced order.

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

The new conventional Scout reference covers complete battle-core returns in
187 supplied-world cases, including 3,073 rounds and promotion draws. Combat
uses a distinct CRT stream with ordered attacker/defender modulo draws and
ties against the attacker; ordinary movement uses a separate shared range
stream. Both must be persisted independently from map JSF32 state. Remaining
movement-tail position/selection, command admission and original new-game/save
RNG provenance are still being investigated. These are reference findings,
not implemented combat or a claim of full live-game fidelity.

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
