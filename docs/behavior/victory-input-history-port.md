# Saved inputs for victory pressure and dominance

`SmacSimulationState` V21 adds optional `SmacVictoryHistory`: eight signed
economic deadlines, eight paid costs, eight Base-expansion/advice history
words, and the Voice of Planet host marker. It stores no derived pressure or
dominance boolean. The original [input lifetimes](leading-human-victory-pressure-shortcut.md)
and [whole-world reset](whole-world-pressure-input-reset.md) are separate from
query-time calculation.

Fresh world construction initializes slots0..7 to zero, including unused
slots, and establishes Voice's absent marker−1 before faction initialization.
The measured positive-owner and slot0 initializer projections justify these
values; they are not inferred from map contents or zero-filled process memory.
At nonzero initialization, only the owner's expansion/advice word becomes the
current turn; economic deadline and paid cost survive.

Starting Base placement, actual Colony founding, generic canonical spawning
and successful acquisition stamp the gaining owner before publication hooks.
Ordinary nonlast removal does not. Empty-owner settlement and last-Base
disposition stamp the reinitialized owner while preserving economic history.
Capture prepares private history, exposes it during the existing bounded
query transaction, and rolls it back with the other staged inputs. Commit
publishes the gaining owner's stamp and any victim initialization together.
Validation-only capture checks therefore leave saved history unchanged.

Ordinary save/Start restores all25 signed words exactly. V11..V20 migration
retains absence; later founding, capture and initialization do not fabricate a
complete missing history record. Native signed diagnostic words, including
slot0 and unusual Voice markers, survive the port codec without reinterpretation.

## Validation

- Independent V21 wire framing, V20 absence, truncated/trailing data and
  out-of-signed32 rejection. Four original initializer projections and the
  original25-word storage pattern; all eight fresh slot reset projections.
- Real startup callbacks see history0 across all seven first-Base faction
  packages and six difficulties. Actual moved-Pod founding updates its stamp.
- Nonlast removal preserves poisoned history. Actual save/Start preserves it,
  and a later Base event preserves explicit legacy absence.
- Actual survivor capture stamps only the gaining owner. Actual last-Base
  capture/restart at turns6 and100 stamps both affected owners, preserves a
  peer and the economic attempt, and survives save/Start. The existing exact
  snapshot comparison also covers validation-only rollback.
- Cached incremental builds and affected simulation, founding, military,
  capture/restart and production regressions pass. Targeted correctness
  analysis covers ten production consumers, with later changed files rechecked.
- Focused ASan/UBSan covered the state codec and six Base/economy lifecycle
  files. Final initializer and capture-preview changes received their own
  focused instrumented checks. Other objects used the current normal build;
  leak detection was disabled. No fresh platform matrix was run.

The native reset receipt distinguishes one executed parent prefix, eight
executed child prefixes and source-backed complete loop coverage; it does not
claim continuous execution of an entire original new-game frontend. The port
tests likewise check this storage slice, not every faction initializer effect.

The complete regional strategy/research consumer is still to connect. Economic
victory commands, HQ-loss refund/cancellation, project construction and tutorial
advice actions are not implemented by this storage change. They must update
these fields at their own original boundaries when those actions are enabled.
This is stock SMAC evidence; SMAX parity remains open.
