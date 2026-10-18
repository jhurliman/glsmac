# Undefended Base capture integration

Native `move_unit` now transfers an admitted undefended foreign Base through a
prepared transaction. It retains Base and surviving Pop identities, retires the
lost citizen only after ownership is coherent, refreshes ownership presentation,
and saves the conquered Base's former owner and assimilation timer.

The implemented slice is local human Librarian play, adjacent clear flat land,
full ordinary Infantry/Gun/Scout/Fission movement, mutual Vendetta, first capture
of an ordinary Base with population at least2 and no technology gain. Native
references cover population2-to1 and population3-to2; population1 razing and
recapture remain separate rules. Last-Base cleanup/restart is joined as described
in the [disposition plan](smac-last-base-capture-plan.md). Defended cities and
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

Presentation posts the capture message, refreshes the Base's owner and population,
then emits the existing Base selection event to open its management screen.
The selection travels with the prepared Base update queue, is delivered once
after the ownership refresh, and is not saved or replayed by Start. It does not
suspend backend movement for a screen-close continuation. The original constructor
is a [documented research boundary](behavior/captured-base-screen-close.md), with
[static return-tail evidence](behavior/capture-return-tail-static.md) distinguished
from a successful dynamic return. Assimilation history is stored and its upkeep
tail is connected as described below; its citizen contribution awaits the live
citizen-processing slice. No full original
capture-screen-close or SMAX fidelity is claimed by these tests.

Validation uses live native movement, exact RNG/treasury/history expectations,
stable and retired Pop wrappers, remote home reassignment, unchanged control Base
records, actual saved-world Start, and failures during future resource queries
and late role publication. Context tests cover V2 migration, V3 history bounds,
truncation and round trips. Relevant consumer tests, changed-production analysis
and focused ownership sanitizers accompany this slice.

The cached Linux Debug run passed 201 affected native groups. LLVM15 correctness
analysis passed 11 changed/related production translation units in24.69 seconds.
Six focused groups passed ASan/UBSan/LSan with leak detection and halt-on-error,
including future-query rollback, late publication failure, actual Start, citizen
retirement and the subsequent hostile relation value. No per-tweak platform
matrix was added.

## Headquarters and candidate population

[Original HQ capture](behavior/headquarters-capture.md) establishes that HQ
presence does not refuse capture. The same random facility scan and mandatory
HQ removal run. Workers, support, resource calculations and citizen publication
use the population after ordinary capture loss, rather than hardcoding1. Food
and the retained population aggregate keep their separate lifetimes.

[Paid relocation](behavior/headquarters-paid-relocation.md) checks the original
HQ and pre-loot defender treasury. It scans
remaining defender Bases in ascending ID order, scores retained regional counts,
population and wrapped distances, and retains the first strict minimum below9999.
A final selected ID0 is rejected without retry. In the admitted local caller,
focus is the captor, so no losing-focus confirmation is entered. Successful
relocation installs HQ immediately and pays1000 before transferring the booty
already calculated from the original treasury. It does not queue HQ or recompute
booty. Both facility changes and treasury changes publish with the captured Base
before citizen retirement or notifications. Resource history is not reposted.

The live checks include ordinary and paid HQ capture, population3-to2, late
publication rollback, control-Base records, exact treasury/RNG, screen ordering,
and actual save/Start followed by assimilation upkeep. Existing population2 and
last-Base capture checks remain in the affected group.

The original paid case ends at820/0 credits after saving720 booty and paying1000,
with37 CRT draws and shared RNG unchanged. The live composition checks those
results and retains the supplied regional count3 rather than deriving2 from the
Base list. A separate static-rule extension selects a larger farther HQ Base
while home repair still chooses the nearest Base. The focused Linux checks cover
17 affected native groups; two production/consumer translation units pass LLVM15
analysis in4.46s. Four ownership groups pass leak-enabled ASan/UBSan, including
ordinary/paid capture, rollback and save/Start; unchanged production evidence is
reused after the final test-only selector extension. The original timed escape
notice and full captured-Base screen/return rendering remain presentation work.

## Assimilation upkeep

Completed native Base upkeep now decrements positive conquest history once,
after both resource passes, without another recalculation. Matching former and
current owner decrements by up to two; expiration sets former owner to current
owner. An incoming zero preserves stale former-owner identity. Production-only
processing and ordinary resource recalculation do not age it. The prepared Base
transaction joins this tail step to population/production publication, so a
failed upkeep cannot consume it before a retry.

The [complete original owner-upkeep receipt](behavior/assimilation-owner-upkeep.md)
establishes the ordinary phase and distinguishes the skipped modal FINISHED view
branch. The [timer receipt](behavior/assimilation-timer.md) establishes matching
owners and zero handling separately. The port tests use eight supplied-history
combinations and actual Start, plus real capture → save/Start → next upkeep.
They cover failed publication and read-only recalculation. The 34 affected upkeep,
Colony, starvation and capture groups pass; final next-upkeep checks pass too.
Correctness analysis passed five production translation units in6.40 seconds.
The capture ownership sanitizer evidence above is reused; this following timer
arithmetic/publication slice does not add another ownership or platform matrix.

The screen-entry follow-up passes five focused native groups, including capture
queue ordering, no duplicate selection on later flush, actual Start/upkeep and
the existing synthetic rendered ownership/navigation check. It reuses the current
`base_select` UI listener; no GSE behavior is rewritten. Native interactive capture
on the original installation and in the port remains a milestone validation.
Correctness analysis passed four affected production translation units in10.92s.
