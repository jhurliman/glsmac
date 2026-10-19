# Saved faction-turn continuation

`SmacFactionTurn` records the ordered continuation needed by the native faction
controller: global entry, wildlife upkeep/command, each positive owner's
upkeep/command, then the round tail. Human focus remains separate from the
scheduled owner. The original dispatch and first-refresh ordering are documented
in [the native authority handoff](behavior/seven-first-upkeep-authority.md).

This is port-owned metadata, not a translation of the original save layout.
The existing live all-human controller does not create or advance this cursor
yet. A cursor transition records completed work; it does not perform economy,
movement, RNG, wildlife or computer decisions.

Each transition requires the exact saved turn, sequence, phase and owner ticket.
Rejected stale or out-of-phase transitions leave the cursor unchanged. Inactive
owners can be retired without inventing upkeep, and reactivating an earlier slot
does not rewind the same round. The per-round upkeep mask resets at global
entry; the social-refresh history survives subsequent rounds.

Simulation V34 optionally stores the cursor. Restore binds its actors to the
simulation roster, its focus to saved human membership and its year to the world
turn. Each owner's social phase must agree with saved refresh history, with a
known retained snapshot even for future owners. This allows a first-round save
after one owner's upkeep without refreshing the remaining factions on load.
V11–V33 records keep the cursor absent and retain the existing strict world-level
social phase guard. Decode never reconstructs missing progress from entity state.

Validation includes original first-round dispatch fields and social timing,
inactive owners, stale tickets, bounded malformed records, partial-round codec
round trips and an actual backend world-body save/restore with mixed Early and
Refreshed owners. That backend test exercises serialization and restoration,
not an executed computer turn.

Live activation still needs explicit computer roster/startup authority,
owner-scoped movement resets and upkeep, suspended command continuations,
real global/wildlife work, and computer policies that issue legal commands.
Successful faction reinitialization must also reset the appropriate history at
its actual native boundary; retirement alone does not establish those semantics.
