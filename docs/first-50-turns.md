# First 50 turns: integrated playable milestone

## Strategy

The user changed the development strategy on 2026-11-11: build a substantial
playable milestone in parallel, then concentrate cleanup, simplification,
regression tests and repeated QA around the integrated result. Work proceeds on
`codex/first-50-turns`, initially based on accepted integration
`b719f052d7ea2e8fdbde5943dd39146c6b8b3b49`, with the validated passive High
Morale combat continuation preserved. No PR is required for every construction
step. Lane commits are integrated in logical batches; the milestone gets a
reviewable PR when the complete loop is ready for hardening.

This changes sequencing, not the release objective. Faithful SMAC and SMAX /
Planetary Pack, original presentation, multiplayer, all content and outcomes,
and native-platform release gates remain requirements of golden 1.0.0.

## Playable target

Play from ordinary SMAC setup through 50 completed human turns with one human
and six actual computer factions. Start with the source-backed Planet scenario
and then repeat on a generated map. Librarian and Standard Rules are the initial
comparison profile, not a permanent difficulty or map restriction.

Every mechanic encountered in that interval must execute real game rules and
update visible, saved state. A disabled control, empty AI turn, artificial
resource grant, scripted victory or test-only implementation does not satisfy
the corresponding requirement. Differences in AI algorithms are allowed by the
release contract; legal, purposeful and non-cheating play is still required.

- Correct faction packages and human/computer identities; independent global
  year, per-owner upkeep and command phases, including wildlife.
- Workers, nutrients, minerals, energy, support, maintenance, growth/starvation,
  research and available production change coherently as turns advance.
- Manufacture useful military units, Formers and Colony Pods; improve terrain,
  move, establish additional Bases and use researched components/facilities.
- Meet opponents and native life; handle ordinary diplomacy, attacks, defense,
  casualties and capture through the normal command paths.
- Computer factions expand, develop, research, produce, defend and attack
  through legal transactions rather than injected results or hidden UI answers.
- Normal UI exposes the required decisions and accurate state; exploration,
  map interaction, suspended choices and turn completion continue to function.
- Save/reload at several checkpoints preserves continuation and RNG-dependent
  outcomes; a saved owner boundary must not replay upkeep or a computer turn.

An unexercised rule family is still unverified. The milestone report records
which actions, technologies, facilities, events and encounters actually ran;
it must not infer complete campaign fidelity merely from reaching year 2151.

## Parallel ownership

| Lane | Main responsibility |
| --- | --- |
| Faction controller | Local roster admission, independent human mask, per-owner scheduler/cursor, AI decisions and private command authority |
| Gameplay | Useful manufactured designs, ordinary/native combat, movement, founding, terraforming and connected Base economy/production |
| Live UI | Ordinary setup/callers, generated native rendering/visibility, decisions and command feedback |
| Integration | Shared world/diplomacy inputs, dependency coordination, 50-turn harness, integration, cleanup and QA |

Controller owns shared Game declarations and scheduling; gameplay owns rule
transactions; UI owns frontend/scripts. Lanes coordinate boundary changes rather
than editing conflicting implementations independently. Each uses an isolated
worktree; the reverse-engineering checkout remains read-only to implementers.

## Construction and hardening

1. Close the actual startup-to-next-human-turn loop; drive it repeatedly and
   fix the first real blocker. Integrate substantial working lane commits.
2. Exercise development, expansion, research and combat through turn 50. Reuse
   established source contracts; request original-game research only for blocking
   unknown mechanics. Cosmetic pixel polishing is deferred.
3. Simplify the integrated architecture, remove obsolete gates/duplicate paths,
   and reconcile shared ownership and serialization. Preserve failure evidence.
4. Run multiple distinct QA passes: deterministic headless progression and
   checkpoint replay; normal interactive play; adversarial/edge-state regression;
   native platform and performance/audio checks at the milestone boundary.
5. Lock reproduced bugs and important cross-system invariants into meaningful
   regression tests, review the complete milestone diff and publish the build.

Use cached incremental builds and focused affected checks while constructing.
Do not require full OS matrices, new sanitizer matrices, large manifests or
separate PRs for every arithmetic or layout change. Ownership, scheduling and
serialization risks receive focused analysis/sanitizers during hardening.
Tests and reference evidence remain mandatory for the accepted milestone;
their timing and scope serve the integrated result.
