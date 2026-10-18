# RE-SMAC-LIVE-ECOLOGY-001: Base ecology and retained research aggregate

This composes the existing ecology arithmetic receipts with the **actual Base
and owner-upkeep callers**. Ten native controls execute two consecutive complete
owner passes over two actually founded Gaian Bases. The ordinary low-mineral
case uses the real resource scan. Positive/threshold cases supply resolved
mineral intake, zero terraforming points and effective PLANET at the mineral
producer entry; they do not replace its result. [Cases](base-ecology-research-aggregate-cases.json).

## Which Base value is accumulated

The input is **final stored Base eco-damage**, after allowance, mineral burden,
human/global/faction scaling and final rounding. The same working field holds
raw terraforming points and intermediate values earlier in resource calculation.
Reading that field at the wrong stage gives a different quantity.

The producer inputs and exact arithmetic compose these contracts, in order:

1. Terraforming scan (companion receipt terraforming-ecology-input.md): all admitted radius
   squares, worked-square multiplier, improvements/forest and Tree Farm/Hybrid
   Forest reductions; floor the summed points divided by eight.
2. Mineral processing (companion receipt base-mineral-processing.md): ordinary gross includes
   incoming convoys and processing multipliers. Ecology uses gross before
   support/outgoing-convoy deductions and before late Space Elevator doubling.
3. Clean allowance and burden (companion receipt clean-minerals-and-ecology-burden.md): use stored
   clean-mineral history, capped mining-satellite count, actual ecological
   facilities and project ownership. Preserve negative intermediates.
4. Final scaling (companion receipt eco-damage-scaling.md): human atrocity/climate term, nonnegative
   clamp, Voice/Transcendence and perihelion doublings, difficulty, stored
   acquisition-score minus future counter, native-life setting and effective
   PLANET. The PLANET multiplier clamps PLANET to −3..+2. Final divisions and
   rounding occur before this aggregate consumes the value.

For the ordinary no-terraforming, no-history/satellite/divisor/atrocity case,
burden is `max(0, ordinaryGrossMinerals - 16)`. With Citizen difficulty, sparse
native life, PLANET0 and score-minus-future50, gross16/17/18 produce damage0/1/1.
Score49 makes gross17 produce0. This clean-mineral/rounding example is not a
replacement for the general calculation above. The stored score is not owned
technology count; the admitted ordinary Gaian opening has score0 despite owning
Centauri Ecology.

## Exact retained-aggregate lifecycle

At the start of an owner's Base/research pass, clear **that owner's** retained
ecology aggregate to zero. Traverse global Base records in ascending order,
processing only matching owners. Do not clear other factions' aggregates.
Existing two-Base posting (companion receipt two-base-upkeep-posting.md) also establishes the
record-removal/reindex continuation and an interleaved foreign-record control.

For each ordinary admitted Base:

1. Recalculate resources before production/population, then again afterward.
   The final stored eco-damage comes from the current resource calculation.
2. Run that Base's research stage, including any positive submission and
   automatic target selection it triggers.
3. Execute the ecology handler. **First**, add
   `trunc(4 * finalBaseEcoDamage / clamp(4 - currentPLANET, 1, 99))`
   to the owner's retained aggregate.
4. Draw once from the actual CRT random stream, even when damage is zero.
   Threshold/event-suppression gates follow the addition and draw. These gates
   therefore do not suppress the aggregate contribution.
5. Continue credits, maintenance and the remaining Base tail.

Later owner policy processing and the regional/capability owner summary follow
the Base pass. The summary retains the completed aggregate. On the next pass,
the same owner's aggregate is cleared again before the first Base's research.
Do not append to the previous turn's total or clear it in the summary instead.

Consequently, **Base0 research sees zero; Base1 research sees only Base0's
contribution**. A later query sees the completed total. Selection must consume
the retained value at its real call time, not recompute a sum over every Base.
Resource previews/recalculations alone do not add contributions.

The denominator clamp is distinct from the producer's PLANET clamp. Do not
cancel the factors algebraically. With score100 and gross17/18:

| Effective PLANET | Final damage by Base | Aggregate after each Base |
| --- | --- | --- |
| −3 | 2, 4 | 1, 3 |
| 0 | 1, 2 | 1, 3 |
| +1 | 1, 1 | 1, 2 |
| +2 | 0, 1 | 0, 2 |

Synthetic PLANET+5 and−100 controls discriminate denominator saturation at1
and99; they establish arithmetic, not legal policy ranges. The positive controls
explicitly set effective PLANET at producer entry because ordinary policy
refresh otherwise restores the Gaian value. They do not establish acquisition
of arbitrary social settings. Following low-mineral passes produce aggregate0
in every case; a peer canary remains unchanged throughout.

## Opening snapshot and limits

The existing admitted one-human Gaian startup supplies a compact timing control:

| Boundary | Own available attack | Foreign available attack | Region1 strategy | Ecology aggregate |
| --- | --- | --- | --- | --- |
| Cold PE storage, before gameplay initialization | 0 | 0 | 0 | 0 |
| First Base/research-pass entry, before later owner summary | 1 | 0 | 0 | 0 |
| Completed opening owner summary / first-command world | 1 | 0 | 7 | 0 |

The early capture has one cached Base, 21 explored region squares, no research
target, labs0 and Explore-only priorities. The actual opening pass makes no
positive research submission and opens no chooser. The previous controlled
positive submission from that early snapshot is an explicit intervention, not
an observed natural opening question. See Blind dispatch (companion receipt blind-research-dispatch.md)
and [regional producer](regional-research-summary-producer.md).

Cold zero-filled storage is not an initializer-reset proof: the already checked
faction initializer clears capability caches but preserves poisoned strategy
bytes. This receipt adds no independent ecology-initializer/save lifecycle
claim. The live owner-pass clear is directly checked with aggregate777 and peer888.

Positive controls suppress native-life events after the aggregate/RNG stage.
They supply upstream mineral inputs rather than create high-output terrain or
advanced facilities. Existing arithmetic receipts cover named facility/project
modifiers; this receipt does not certify their construction/capture/history
lifetimes, bloom placement, native spawning, climate transitions, AI research,
overflow behavior or a complete ecology UI. All evidence is pinned stock SMAC;
no SMAX/PRACX behavior or live presentation timing is inferred.
