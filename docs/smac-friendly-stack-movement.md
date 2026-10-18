# Live friendly stack movement

Conventional directional movement now admits an occupied, nonfungal destination
when every resident is a current registered conventional Unit belonging to the
moving Unit's exact owner and simulation identity. This enables returning to an
occupied friendly Base. Hostile entry still requires combat; foreign or malformed
membership is rejected before movement, queue, sight or RNG publication.

The existing ordinary edge-cost, allowance, ZOC and sight callers remain in
control. The move transfers only the actor's preallocated map node. Resident
records, movement spending, homes and identities remain unchanged. Departing a
stack leaves its other members in place. Existing sight processing retains the
occupied marker at a source with surviving residents and records the destination
as occupied. There is no new stack representation or save format.

Original evidence is RE-SMAC-MOVE-FUNGUS-EXEMPTIONS-001: 81 complete requests and
nine reset sequences execute the original movement/insertion code with a real
friendly occupant. Successful entry preserves both Units and clears the source
marker; failure preserves both placements. The original friendly occupant also
has a distinct fungus lottery exemption. This change connects the ordinary
nonfungal movement path; occupied fungus remains explicitly unavailable until
that exemption and the encounter/nearest-history continuation are joined. The
existing unoccupied-fungus behavior is unchanged. The original receipt is
`docs/behavior/movement-fungus-and-failure-marker.md` in the research checkout;
its case file is not a claim of new rendered or network validation here.

Native checks use the actual GSE directional event, not direct tile insertion
as the action under test. They enter/leave/re-enter an occupied HQ, preserve a
stationary homed Scout, inspect saved sight occupancy, and restore both Units
through actual Start. The previous occupied-destination rejection fixture now
uses a foreign Unit and verifies that rejection reaches the ownership check.

Validation: 66 affected native groups pass with allocation tracking, retaining
65 unchanged successes and rerunning the corrected rejection group. Scoped
correctness analysis passes ConventionalMovement.cpp, GameWorkerKnowledge.cpp
and UnitManager.cpp in 12.17 seconds. Interactive graphics, hotseat handover,
combat and network play remain independent integration requirements.
