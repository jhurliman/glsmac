# Capture preserves the partial-Hurry marker

RE-SMAC-CAPTURE-PARTIAL-HURRY-MARKER-001, 2026-10-18. Pinned stock SMAC.
[Clear/set capture pair](capture-partial-hurry-marker-cases.json).

**Successful ordinary survivor capture preserves a Base's existing partial
Hurry-payment marker.** Clear stays clear; set stays set through native capture,
capture notice, Base-screen recalculation and the normal outer movement return.
Changing owner, resetting minerals or discarding the production queue does
not itself grant another partial purchase.

Both controls use the accepted undefended population2 capture profile, owner2→1,
turn6, clear land, no projects and another surviving defender Base. The
preexisting marker is a declared clear/set state input, not a newly executed
pre-capture payment. Actual movement/capture results are native. Capture notice
acknowledgement and the Base window's virtual show are supplied presentation;
screen recalculation and outer return execute. Last-Base destruction, recapture,
subversion and other ownership paths are outside this pair.

## Marker versus production state

| Field | Before capture, both cases | After capture and native outer return |
| --- | --- | --- |
| Partial-Hurry marker | Clear / set | **Clear / set, respectively** |
| Stored minerals |7 |0 |
| Switching/start baseline minerals |7 |0 |
| Production baseline item | Former | Stockpile Energy |
| Current production | Former | Scout Patrol in this admitted profile |
| Additional queue length |2 |0 |

Native writer order is mineral store reset, baseline-mineral reset, queue
clear and baseline/current-item reset, followed by the capture status cleanup.
That status cleanup preserves the partial-payment bit. Later native production
selection, resource recalculations and screen binding also preserve it in both
controls. The chosen current Scout is this fixture's result, not a universal
capture production choice.

Two actual post-dialog guard queries per captured Base distinguish the
admission consequence. With marker set, positive full **and** partial choices
are rejected by the marker guard. With marker clear, both pass that particular
guard. This is not a replay of full dialog admission, quote, affordability or
a second payment. The [ordinary-local blocking input](hurry-ordinary-local-authorities.md)
is inactive in these controls.

## The later processed-owner reset is separate

The existing [Hurry reset/settlement passage](hurry-production.md) filters Bases
by **current owner**. Running that actual later passage for owner1 clears the
captured Base's marker, while preserving minerals, both mineral/item baselines,
current production and queue state from capture. An owned Base0 marker control
also clears; the remaining foreign owner2 Base's marker control stays set.

Thus the applicable reset belongs to the Base's owner **when the passage runs**.
Do not clear the marker merely on capture, interpret the baseline reset as a
marker reset, or reset every Base regardless of owner. This pair composes the
reset passage explicitly; it does not establish a new full-turn scheduling
rule beyond the existing post-faction-upkeep receipt. No refund or treasury
ownership rule is inferred.

Validation: two complete capture/outer-return controls, four native positive
choice guard queries and two later owner-reset compositions. All other observed
production values match across the clear/set pair. The work is limited to this
marker lifetime; existing Hurry arithmetic and capture consequences are reused.
