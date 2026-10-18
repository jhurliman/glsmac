# Undefended Base capture integration

Native `move_unit` now transfers an admitted undefended foreign Base through a
prepared transaction. It retains Base and surviving Pop identities, retires the
lost citizen only after ownership is coherent, refreshes ownership presentation,
and saves the conquered Base's former owner and assimilation timer.

The implemented slice is local human Librarian play, adjacent clear flat land,
full ordinary Infantry/Gun/Scout/Fission movement, mutual Vendetta, first capture
of a population2 non-HQ Base, no technology gain, and a remaining defender Base.
Defended cities, last-Base elimination, recapture, headquarters relocation and
other capture domains remain explicit refusals. This is not the complete conquest
milestone.

The [original capture receipt](behavior/undefended-base-entry.md) supplies the
pre/post transfer and facility draws: food is retained; population drops to1;
production stores and queue clear; ordinary founding selection chooses the next
target; the saved population aggregate supplies loot's denominator; 37 CRT draws
precede forced facility removals and faction grants. Remote former-owner units
homed here move their home to the nearest remaining Base, retaining Unit identity,
position and owner. The [governor donor receipt](behavior/capture-governor-donor.md)
establishes HQ weighting, first-record ties and exclusion of the captured Base.
Subsequent ordinary Scout battles preserve the capture diplomacy flag, matching
the [original follow-up](behavior/post-capture-hostile-relation.md).

Preparation runs existing worker/resource/territory queries inside a temporary
future ownership view under the owning GC accumulation, without callbacks,
yielding, RNG publication, queue changes, Pop retirement or ID allocation. A
scope guard restores that view on all exits. Application prepares the Pop,
assignment, Unit and Base queues before publishing the joined state.

Inner resource-context V3 adds optional conquest history; V2 migrates with that
history absent rather than inventing it. Complete saved-world Start restores the
transfer without replaying loot or facility rolls.

Presentation currently posts the capture message and refreshes the Base's owner.
Automatic captured-Base screen entry remains to connect; the original constructor
is a [documented research boundary](behavior/captured-base-screen-close.md), with
[static return-tail evidence](behavior/capture-return-tail-static.md) distinguished
from a successful dynamic return. Assimilation history is stored, but its citizen
contribution and upkeep decay remain a following gameplay slice. No full original
capture-screen-close or SMAX fidelity is claimed by these tests.

Validation uses live native movement, exact RNG/treasury/history expectations,
stable and retired Pop wrappers, remote home reassignment, unchanged control Base
records, actual saved-world Start, and failures during future resource queries
and late role publication. Context tests cover V2 migration, V3 history bounds,
truncation and round trips. Relevant consumer tests, changed-production analysis
and focused ownership sanitizers accompany this slice.
