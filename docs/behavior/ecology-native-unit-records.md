# Ecology-born Mind Worms: native creation, complete state and persistence

RE-SMAC-ECOLOGY-NATIVE-RECORDS-001, 2026-10-18. Pinned stock SMAC.
[Literal creation records and returned queries](ecology-native-unit-records-cases.json).
Two complete original H6→7 human land handlers create ten Units across two
explicit native-runtime profiles. One original ordinary save/load pair restores
all five produced Units in the initialized profile. No native AI or combat runs.

## Identity and produced design

Each birth uses shared **predefined design8, Mind Worms**, and **owner0**. Owner0
is the game's native-life identity, not an absent owner, a playable human faction
or the human whose ecology caused the event. The human-control mask contains
only faction1 in these controls; owner0 still owns real Units. Global Unit ID0
is also valid. Do not use zero as an absent owner/Unit sentinel.

The installed produced design is Infantry chassis0, Psi weapon12 with power−1,
Psi armor9 with power−1, reactor rating1, no abilities, stock plan1, carry0,
icon1 and cached cost5. Negative component powers designate Psi components;
they are not zero/missing conventional Gun/Scout data. Reactor rating belongs
to the **design**, not a new per-Unit reactor byte. The existing original-loader
numeric component packet matches the carried context exactly; all required
component records are asserted before use.

The ecology owner attribution is a separate byte. The initializer/factory return
it as0. After a successful factory return the ecology caller writes1. **Owner
stays0.** Admission and saving must preserve both identities independently.

## Complete returned gameplay record

The first profile's five IDs/coordinates are0=(63,19),1=(65,19),2=(64,20),
3=(63,21),4=(62,20). The actual initialized-native profile produces identical
records and coordinates. Every allocated slot starts poisoned; the original
initializer writes all52 bytes, before placement. The portable semantic fields
below account for every initialized field; the private receipt holds layout.

| Field | Returned state in this fixture |
| --- | --- |
| Design / owner / ecology attribution | 8 / 0 / 1 |
| Position | Literal five coordinates above |
| State / flags | unsigned32 zero / unsigned16 zero |
| Damage taken | unsigned byte0: healthy |
| Order | byte0, no active order |
| Automation control / waypoint count | 0 / 0 |
| Four waypoint X and four waypoint Y slots | All signed16−1 |
| Stored morale | byte1 |
| Terraforming work progress | byte0 |
| Observation mask | byte0 in this supplied sight context |
| Movement spent / facing | 0 / 2 |
| Movement iteration control / status character | 0 / NUL0 |
| Probe action / probe target controls | 0 / 0 |
| Home Base | signed16−1, no home |
| Next / previous stack links | signed16−1 / −1: singleton |
| Auxiliary word / auxiliary owner-control byte | unsigned16 zero / unsigned byte0 |

The last two fields are explicitly initialized carry-through controls; their full
future AI/visibility meanings are not assigned by this receipt. Other auxiliary
controls also retain their measured values rather than being inferred from a
faction-owned conventional Unit constructor. Do not drop them from a portable
saved representation merely because this birth initializes them to0.

The unplaced initializer coordinates are−4/−4; placement replaces them with the
actual tile. The caller's attribution write follows factory return. Publish the
completed staged Unit, not that intermediate unplaced/tag0 state.

Visibility is conditional state. Actual factory placement and visibility helpers
execute, but this fixture's supplied tile exploration is cleared and there are
no human observer Units. Its returned observation mask0 is **not a universal
visible/hidden rule for every native birth**. Reuse the existing
[native observation contract](native-visibility-lifetime.md) for sight updates.
Owner visibility and stored observer masks remain separate authorities.

## Health and movement queries

With produced reactor1 and damage0, the existing native reactor-based health
contract yields **10/10 hit points**. Damage is a taken-HP count, not a percent;
there is no newly initialized per-Unit maximum-HP field. This value reuses the
[original health/creation evidence](scout-creation-morale-and-movement.md) and
[component-complete native combat](native-component-combat.md); no fresh battle
or broad reactor/Psi health grid was executed here.

Every created Unit's actual basic and complete movement allowance queries return
**3 engine quanta**, with the installed movement quantum3 and spent0. Elite
suppression on/off both return3. The existing Unit queried at turn330 reaches
native level6 but still receives allowance3: predefined negative-Psi weapon
handling suppresses the ordinary elite movement increment. This does not claim
movement cost through fungus, command admission or autonomous movement timing.
Those remain separate from an available allowance.

## Stored morale versus dynamic lifecycle

Native owner0 dispatches to the native-level query, bypassing ordinary faction
retained-MORALE arithmetic. At birth turn90, stored morale1 coexists with neutral
effective level**2**, whose installed native label is **Pre-Boil**. Do not render
or freeze the level by reading stored morale1 as Larval Mass.

The existing [native birth/lifecycle receipt](native-birth-producer.md) establishes
base levels by **global turn counter**, not per-Unit age: below45→0,45..89→1,
90..169→2,170..249→3,250..329→4,330+→6. The schedule skips5. Existing born Unit0
is newly queried at89/90,169/170 and329/330 here; its bytes and stored morale
remain unchanged while neutral levels are1/2,2/3 and4/6. These are controlled
counter/query checks, not simulated elapsed turns or native AI upkeeps.

Both queried morale modes agree for this owner0 branch. Supplying opposing
faction1 instead of neutral context0 gives an extra level because its retained
major-atrocity count is nonzero (3 here), with clamp6. Thus birth-turn90 queries
return2 for neutral context and3 for faction1 context. It is **+1 for nonzero**,
not +3 and not an automatic bonus from ecology attribution1. Source also reads
specific Unit state/flag markers and a design10 adjustment; those markers are0
for these design8 births, and their later producers are outside this handoff.

Installed native labels0..6 are Hatchling, Larval Mass, Pre-Boil, Boil, Mature
Boil, Great Boil, Demon Boil. They are sourced from the installed morale rows;
the counter/result is the query authority. Combat promotion, Psi strength,
retreat/capture, changed ownership and AI state transitions remain separate.

## Exact RNG and native-runtime distinction

Each handler consumes the established **twelve CRT draws**: one event roll and
eleven candidate draws. All five factories then consume **zero** CRT draws on
these two admitted paths. All returned-state and controlled-turn queries consume
zero RNG. No shared RNG entry occurs; the shared seed remains305419896 and CRT
ends2029258301 from initial seed65. There is no random birth-morale roll here.

The two zero-additional-draw reasons are deliberately distinguished:

- **Carried parent receipt context:** native reference coordinates0/0; each birth
  is in a different region from the reference tile, so the optional assignment
  tail exits at its region comparison. This is supplied retained context, not
  the native-faction initializer's default.
- **Actually initialized native runtime:** the complete original faction0
  initializer produces reference coordinates−1/−1. Each factory returns before
  the optional assignment tail. The actor's Base is preserved. Initialization
  itself consumes eight CRT draws and eight localized-text calls, no shared
  draw; an explicit ecology seed is supplied **after initialization**, as in
  earlier birth evidence. That setup consumption is separate from the twelve
  handler draws and is not a continuous process-start seed claim.

Other retained native assignment state may reach later factory branches and add
state flags/counters or RNG. Do not generalize these two paths to every native
creation. The source global factory capacity2048 returns failure before a new
record when full; capacity failure is source-backed here, not a new stress test.

The actual factory increments global Unit count0→5, its owner0 plan≤7 tally0→5,
and owner0 design8 count0→5. These are distinct counters. No human1 Unit count is
substituted for native ownership. The ordinary initializer/placement/factory
return records and post-ecology records are literal in the fixture.

## Original save/load preservation

The complete produced initialized-native world (one declared human Base and
five real neutral Units) is cloned byte-for-byte into a guest with separate
memory-backed FILE services. Original serialization/read bodies execute; Unit
records are not reconstructed by a host model. After save, all260 Unit bytes,
Unit count, native tallies, reference coordinates, ecology H and warming are
poisoned. Load returns0 and restores the five records **byte-for-byte**, including
owner0, attribution1, all sentinel waypoints/stack/home and auxiliary fields.
It restores native count5, design8 count5, reference−1/−1, H7 and warming1.

Fifteen actual loaded queries (neutral level, faction1-context level and allowance
for each Unit) still return2/3/3. Save, load and loaded queries consume no random
draw or seed operation in this ordinary non-Iron-Man control. This is preservation
of Unit state, **not evidence that ordinary file bodies restore RNG words**;
[the separate RNG save contract](gameplay-rng-save-load.md) remains authoritative.
Full frontend resume, native turn scheduling, filesystem durability and arbitrary
nonzero AI controls are not certified by this one file-body round trip.

## Port admission boundary

Admit native owner0 and predefined design8 explicitly; keep the ecology-source
faction independent. Persist the complete semantic record and its sentinel values,
query dynamic native level separately from stored morale, and obtain health and
allowance from the actual design. Resolve visibility from the existing sight
producer. Staging should include the owner's counters, tile occupancy and completed
record before publishing map/UI state. None of this admits ordinary human command
ownership for neutral Units or supplies native AI/combat policy.

Two full ecology returns, ten native factories, one complete native initializer,
106 returned-state/time queries and15 loaded queries are newly checked. Existing
lifecycle/combat/visibility grids are reused without recapture. The handler ends
at H7/warming1 as already measured; no broad climate work is added. No production
or installed asset files changed; the standing broad research goal stays paused.
