# Live Base Hurry purchases

The Base inspector connects its Hurry button to the actual local treasury and
owned production state. Full payment buys the remaining minerals; partial
payment may add zero minerals and still consumes this Base's partial-payment
allowance until the measured owner reset. Production completes during upkeep.
The command updates the current-item and mineral baselines without creating a
unit or facility immediately.

## Authority and admission

The quote/payment kernel follows the public native receipts
[hurry-production](behavior/hurry-production.md) and the frozen v2
[ordinary local authorities](behavior/hurry-ordinary-local-authorities.md).
Live admission requires the saved ordinary no-project campaign capability,
actual local ready or stamped Colony Screen ownership, the current eligible
unit/facility, the resolved mineral factor/cost, actual active retool exemption,
and the owned treasury/reservation account. Strict campaign source admission
establishes the default Hurry percentage; absent Voice follows the saved
no-project capability. Unsupported production categories remain unavailable.

The amount field currently accepts whole credits from zero through the quote.
That numeric bound and full-payment affordability are explicit port admission
rules; this does not establish original numeric-control or dialog fidelity.
Checked signed32 arithmetic refuses unverified overflowing inputs.

Native expected state includes the complete Base/turn state, treasury, reserved
spending and resolved quote inputs. Preview is read-only. Successful publication
stages production, money and frontend updates before nothrow swaps. Duplicate or
stale commands do not spend twice. A purchase at another Base invalidates an old
budget preview. Foreign/legacy views have no usable command.

## Persistence and lifetime

A versioned ProductionState extension owns the partial-payment marker. Earlier
port frames without that extension explicitly predate this command and migrate
to false; unknown, truncated or trailing extensions are rejected. Ordinary
Save/Start retains the marker and paid baseline; it does not replay payment.
Manual production switching and queue edits preserve the owned marker.

At the measured owner tail, markers clear before reservation settlement and
owner advancement. Foreign-owner Bases are untouched by this owner pass.
Successful ordinary survivor capture explicitly preserves the independent
marker while resetting minerals/baselines/queue, following the frozen
[capture marker receipt](behavior/capture-partial-hurry-marker.md). Actual
clear/set capture and real Start controls retain it. This does not establish
original last-Base destruction, recapture or subversion lifetime parity.

## Validation

In progress: 32 literal native quote/payment controls, versioned codec and
overflow refusal; actual full/partial/zero/tiny/budget-clamped transactions,
duplicate/stale/foreign commands, manual switch and real Start/reset. The actual
GSE dialog callbacks cover exact amount parsing, unavailable/full/partial/zero,
Cancel, stale refresh and research interruption. Rendered acceptance and focused
analysis/sanitizer results will be recorded before publication.
