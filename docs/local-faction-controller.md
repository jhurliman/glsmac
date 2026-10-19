# Local faction controller integration

The next playable milestone requires one human and six actual computer factions,
not seven human participants. The [original seven-faction Planet receipt](behavior/seven-faction-planet-start.md)
establishes setup and the first human/computer controller boundaries. It does
not establish AI decisions, a completed human turn, or arbitrary-map balancing.

## Verified sequence

For the pinned Librarian, Standard Rules, Gaia-human Planet start, the original
initializes human1 first, then wildlife0 and computer2..7. Human membership is
0x02 independently of active membership 0xff. All seven positive factions get
an immediate population1 Headquarters and their starting Scout; Sparta uses its
generated Recon Speeder. Separate site balancing grants six additional Pods in
this particular world. Its deficit producer must be implemented, not replaced
by those observed counts or coordinates.

The controller advances the global year once, then runs wildlife0 and human1
upkeep before human1 command entry. University3 has not received its extra TECH
award. A separately admitted continuation through the native next-slot increment
runs Hive2 upkeep before computer2 command entry, with the same global year and
human focus. Upkeep itself changes the Hive Scout order before the AI body.
The receipt's site-dependent nutrient/mineral/treasury postings are comparisons,
not hardcoded constants for the port.

## Current integration boundaries

Inspected at integration 84336af1 (through PR375):

- `GameSimulation.cpp::PrepareNewSimulation` assigns human membership to every
  occupied participant. `Player`/`RosterRecords` transport roles do not encode
  human versus computer control. Preserve roster identity and the independent
  saved human-membership authority.
- `GameStartingBase.cpp::HandleCanonicalStartupEvent` and downstream starting
  Base/Unit producers require human authority. Add a private admitted startup
  authority for bound computer participants; do not relax public local commands.
- `GameCanonicalTurn.cpp::RequireCanonicalTurn` checks all active participants
  with `RequireSmacLocalHumanOwner`. `AdvanceCanonicalTurn` stages every owner's
  Base records, refreshes first-turn social state for all, and resets every
  positive-owner Unit's movement. `ContinueCanonicalTurn` runs all owner upkeep
  before publishing the command-ready turn. This is an explicit existing local
  human policy, not the original computer scheduler.
- `SmacSocialState::ValidateTurn` requires every faction to be Refreshed once
  global turn is positive. A true first round can contain both Early and
  Refreshed factions; persisted scheduler progress must make that distinction
  verifiable rather than removing validation globally.
- `Game::CanonicalTurnContinuation` retains Base/research/colony-screen progress
  while upkeep is suspended. Extend that ownership model to faction command
  boundaries. Existing query tokens must not become computer UI prompts or
  permit a human to issue commands for another faction.
- Saved `SmacSimulationState::human_membership` already survives capture and
  restoration independently of the bound roster. A computer victim in a fixture
  does not itself implement computer admission, scheduling, or decisions.

## Implementation and acceptance order

1. Admit the local controller roster explicitly, preserving transport identity,
   local human focus, native faction IDs, and separate human/computer ownership.
   Initialize stock factions with their existing rule producers. Connect site
   selection and balancing to actual terrain inputs; independently verify the
   deficit calculation before expecting the pinned seven-Base/thirteen-Unit
   startup. Never fabricate the observed Pod package in production.
2. Introduce an owned faction cursor with global-advance, owner-upkeep and
   owner-command phases. Persist enough state to resume without replaying any
   year advance, awards, random draw, resource posting or movement reset. Refresh
   social/technology/Unit state at its measured owner boundary. Preserve the
   existing suspended research and colony-screen transactions.
3. Route a completed local human turn to the next scheduled owner. Computer
   commands use private owner-bound authority, while frontend commands retain
   local human authentication. Decide research/production/population queries
   through an explicit computer policy, not supplied UI answers. Keep human
   focus separate from the scheduled faction.
4. Connect computer decisions to legal existing movement, founding,
   terraforming, production and combat transactions. Implement purposeful
   expansion, defense and attack before claiming a playable opponent. Native
   controller behavior and full strategic/diplomatic fidelity remain separate
   required work; an empty computer turn is not an AI milestone.
5. Verify a complete local conquest match with one human, actual computer
   opponents, save/reload across owner boundaries, elimination/restart and an
   ending. Then exercise network authority separately before the M2 gate.

The first scheduler integration must test the actual startup/turn callers: at
human1 command entry, Hive2 and University3 must still retain pre-upkeep state;
at computer2 entry only its upkeep has advanced; global year stays unchanged.
Exercise repeated/stale completion input, forbidden foreign human commands,
restoration at supported command boundaries and suspended choices. Compare
original setup/controller fields only at the exact boundaries audited above.

Use cached affected tests and targeted correctness analysis during development.
Scheduler persistence, shared lifetime and command authority warrant focused
sanitizers and a broader integration gate, not a full three-platform matrix for
each arithmetic change. A playable match is a meaningful platform smoke gate.
