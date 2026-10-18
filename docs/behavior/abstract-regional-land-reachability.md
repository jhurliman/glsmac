# Abstract regional land reachability

RE-SMAC-ABSTRACT-REGIONAL-PATHS-001, 2026-10-18. Pinned stock SMAC.
[Literal maps and expected results](abstract-regional-land-reachability-cases.json).
This closes the current ordinary land profile's **boolean path predicate**,
extending the [existing caller/strategy cases](regional-strategy-overrides.md).
It does not specify a Unit's movement costs or the returned first-step direction.

## Inputs and graph

Let A be the owner whose summary is being calculated and B the peer owner.
The caller asks whether an actual Base of A can reach an actual Base of B.
It passes both coordinates and both owners to an abstract search, with
mode 0 selecting one option combination and any nonzero mode selecting the
other. The original substitutes predefined prototype 0 for the abstract
calculation; the admitted installed profile is its ordinary land design.
Do not pass the selected Scout's movement points, morale or weapon stats.

The graph uses the eight parity-map neighbors listed in
[Base-stage flags](regional-base-stage-flags.md). X wraps only on cylindrical
maps; Y never wraps. Every accepted edge has equal cost in this profile.
An intermediate tile must have elevation class at least 3. Either requested
endpoint is exempt from this terrain test, so a controlled water endpoint
can join neighboring land, but a water corridor cannot bridge two land areas.
Rockiness, fungus, roads, occupancy/Base item bits and intermediate region
tags do not change the measured abstract predicate. This does not admit
ordinary ground Unit movement through occupied tiles or across water.

The search expands **backward from B's coordinate**, which is seeded without
applying the territory admission test. A's coordinate and other entered tiles
must pass that test. This matters for supplied inconsistent endpoint territory:
do not model both endpoints as unconditionally admitted. Normal end-to-end
Base cases use consistent A/B ownership and land endpoints.

Territory means the tile's retained signed territorial-owner value, not its
Base affiliation, exploration state, Base owner, or a newly computed nearest
Base. Negative unowned territory and owner slot 0 are distinct inputs.

## The modes are not supersets

In both modes, a tile belonging to A or B is admitted. Other territory follows
these exact rules:

| Territory | Mode 0 | Nonzero mode |
| --- | --- | --- |
| Negative/unowned | Admit | Reject |
| Third-party owner C, including slot 0 | Reject | Apply the directional checks below |

For the nonzero mode, using retained primary diplomatic words:

1. If `relation[B][C]` contains `0x100000` and contains neither bit in `0x11`, reject.
2. Otherwise, if `relation[C][B] & 0x11` is nonzero, admit.
3. Otherwise, admit only if C's human-mask bit is clear.

The forward veto precedes the reverse-direction permission. A Pact-like
`0x20` alone does not satisfy `0x11`. These are literal primary-word tests;
do not substitute a symmetric war/treaty enum. A third-party AI tile with zero
relations is admitted in the nonzero mode; an otherwise identical human tile
is rejected. The mode that permits AI territory simultaneously rejects unowned
territory, so “broad route includes every strict route” is incorrect.

## Base caller and retained state

The caller's existing gates still apply before the graph query. A nonzero
requested region requires positive retained regional Base counts for A and B.
It scans actual A-owned Base records in order, filters their stored regions,
and pairs them with B-owned Bases in the same stored region. No live eligible
pair means false, even if retained counts are positive.

It clears map scratch marks, marks same-owner land territory components and
suppresses already-marked Base representatives. Those marks are temporary
query work, separate from persistent ownership/region arrays. The existing
33 policy/caller cases already cover component suppression and stale counts;
they are reused rather than reclassified as new tests. The new 66 complete
Base-caller cases execute the real component marker and real pathfinder on
supplied maps with consistent owned endpoints. Their literal path arguments
match the A-coordinate, B-coordinate, A-owner, B-owner and requested mode.

The native pathfinder retains a distance workspace keyed by destination,
abstract plan, mode flags and B. It can reuse a populated workspace; it is not
a saved faction authority. These new graph cases explicitly invalidate that
workspace before each independent map and before the full Base-caller check.
They establish current-map reachability with a fresh workspace. They do not
close every upstream workspace invalidation after terrain/diplomacy changes,
nor authorize pretending that arbitrary stale workspace contents were measured.

## Validation and scope

An independent graph oracle agrees with **386 complete native abstract queries**
and **66 additional complete Base-to-Base caller executions**. Controls cover
signed owner boundaries, both relation directions, human classification, sea
levels, endpoint exceptions, X wrap/flat edges, disconnected vertical bounds,
varied region tags and item flags. One hundred supplied mixed terrain/territory
maps are exercised across modes and human masks. Wrap-only and unowned-corridor
controls distinguish cases that an all-land smoke would miss. No path helper
result is supplied; all calls return normally without gameplay RNG calls.

Use the case file's boolean expectations for the current no-project land-world
consumer. Its uniform-cost reachability does not close sea/air prototypes,
projects changing the admitted design profile, ordinary Unit movement/ZOC,
weighted paths, every arbitrary option combination, first-step tie breaking,
or a fresh integrated campaign smoke. Those remain separate contracts.
