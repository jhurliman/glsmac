# Live ordinary N/M upkeep

`Base::ProcessSmacUpkeep` composes the actual Upkeep worker calculation and fresh
N/M processing with production, population and a forced second calculation.
The native turn dispatcher owns once-per-turn invocation; neither this method
nor the smaller `ProcessSmacProduction` stage has a GSE posting entry point.

The first pass supplies nutrient normalization, the stored-food gate,
nonnegative mineral posting, saved riots suppression, actual cached target cost,
human carry cap, queue advancement and target/mineral baselines. An empty human
queue repeats its current target. Unit eligibility uses actual active,
exclusion, prerequisite, owner and coastal inputs. Non-unit queue eligibility is not connected. The separate Colony continuation
is described in `smac-colony-production.md`.

The caller supplies a read-only registered presentation lookup, which must not
execute scripts or mutate the world. The manufactured-unit factory checks the
source design and prepares home, stack, sight and manager updates. The pending
Unit joins the actual support collector for subsequent calculations; the first
pass never retroactively includes its support.

Population processing consumes the owned current growth/housing rules and the
first nutrient result. Growth and boom run their required internal worker/N/M
calculation with the candidate population and pending newborn, then apply their
measured food normalization/collection order. Starvation retains its pre-loss
surplus until the unconditional outer pass. That outer pass always uses the
resulting population, target and newborn support. It normalizes stored food,
but never posts minerals or collects food a second time. Candidate calculations
never temporarily mutate the Base or its Pops.

Base preparation stages Pop assignments, additions/removals, reverse index,
accounting and both update queues. Surviving Pop nodes keep their addresses and
IDs; growth allocates fresh IDs with variant zero as an explicit GLSMAC
presentation policy. Unit and Base commit without an intervening callback or
allocation. Removed Pop wrappers retire after the complete authority commit
and trigger-lock release, before the spawn notification. A failed preparation
publishes nothing. This port transaction does not claim that the original
allocator's post-debit refusal refunded minerals.

The caller admits the existing local human roster, inactive governor and saved
no-project campaign. Generated random/custom maps use an explicit ordinary
non-scenario policy with no protected objectives; mapfile/scenario setup is
unavailable. Size-one starvation awaits actual Base removal/home reassignment,
and a negative mineral surplus at the post-population handler boundary after
turn one awaits supported-unit loss processing. Both reject before any
publication. Energy, credits, research, later riot-status decisions and
facility/project completion are outside this N/M slice. The factory's
separate training and presentation restrictions still apply.

Focused native checks cover actual Scout/Former completion and queue repetition,
valid Doctor retention, newborn support without double posting, stable Pop
wrappers across growth/starvation, exact saved continuation, and atomic late
queue/removal/deficit rejection. Seventeen affected tracked groups passed before
the final deficit guard; all four upkeep groups pass with that guard. Six
changed production translation units analyze clean. Existing original posting,
queue, population and ordinary upkeep-loop contracts supply the semantics;
these tests are synthetic live compositions, not new original-executable runs
or a completed whole-game turn test.
