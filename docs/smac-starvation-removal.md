# Non-last Base starvation

The live canonical upkeep caller now removes a population-one Base when the
population step returns removal and another same-owner Base exists. It uses the
accepted `starvation-base-removal` contract: distance is measured from the lost
Base, X wraps, and equal distances select the later Base record. Port entity IDs
remain stable while the saved Base record order closes the removed position.

Actual Units homed to the lost Base receive the selected surviving home. Unhomed
Units remain unhomed. Current canonical Unit admission requires same-owner homes;
foreign capture-created home links remain outside this slice rather than being
silently treated as an ordinary accepted state. Last-owned-Base removal still
rejects before effects pending the restart/disposition path.

The transaction prepares rehomed Unit states, territory, Base membership/order,
worker reverse indexes and update queues before committing without callbacks.
Exploration, remembered improvements and Unit observations are retained. Base
and Pop wrappers retire after the authority commit, including if the later
despawn notification throws. The controller consumes `removed_base` and skips
the retired Base's final calculation; its stable-ID traversal still processes
each surviving Base once. Earlier Bases' completed upkeep is not rolled back.

Focused native coverage runs the real turn event with the lost Base first,
middle and last in record order, checks the later-record tie, survivor posting,
home repair, unchanged knowledge/RNG, wrapper retirement and one despawn, then
restores each complete world and queries the repaired homes/support. A separate
last-owned-Base test checks unchanged snapshot and Unit ID allocation on rejection.
These are actual GLSMAC composition tests, not new original-game executions.
