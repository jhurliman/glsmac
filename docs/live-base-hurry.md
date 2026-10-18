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
Manual production switching and queue edits preserve the owned marker. The
legacy production scripting DTO also preserves the independent marker when
replacing its existing fields.

At the measured owner tail, markers clear before reservation settlement and
owner advancement. Foreign-owner Bases are untouched by this owner pass.
Successful ordinary survivor capture explicitly preserves the independent
marker while resetting minerals/baselines/queue, following the frozen
[capture marker receipt](behavior/capture-partial-hurry-marker.md). Actual
clear/set capture and real Start controls retain it. This does not establish
original last-Base destruction, recapture or subversion lifetime parity.

## Validation

At production checkpoint `ab9a736f`, 25 distinct affected native groups pass:
32 literal native quote/payment controls plus codec/overflow guards, actual
full/partial/zero/tiny/budget-clamped transactions, stale/duplicate/foreign
commands, manual switching and legacy DTO roundtrip, real Start and owner
reset, clear/set survivor capture, stamped Colony Screen, and the existing
production/queue/codec callers. Ten Base GSE scripts pass; the Hurry dialog
fixture also models non-idempotent Input focus registration and unavailable
close/reopen. The first real dialog run exposed a duplicate input registration;
the local visibility transition fix is included, not counted as a passing run.

Both new production sources are registered and compile through real CMake.
Targeted correctness analysis passes eight changed production translation
units. Focused ASan/UBSan passes the nine Hurry/capture/Screen groups with seven
relevant instrumented translation units; other objects reuse the cached normal
build, leak detection is off, and the external harness links without PIE.
This is not a full sanitizer or platform matrix.

Installed GOG assets, exact `ab9a736f` binary/scripts and actual SDL input plus
1024x768 backbuffer capture validate this sequence on the Linux renderer:

1. Load the existing ordinary Planet queue save (pre-Hurry format). Scout stores
   2/10 minerals, treasury29, year2119, pending Former/Scout/Scout.
2. Open Hurry: quote38. Type5: read-only preview shows stored3. Cancel, reopen,
   type5 and accept partial: stored3, treasury24, year/queue unchanged. Reopen
   reports the partial-payment marker and does not offer another purchase.
3. Actual Menu Save writes828312bytes. A fresh process Load/Start retains the
   paid marker, stored3, treasury24 and year2119 without replaying payment.
4. End the actual turn: year2120, stored5, treasury25; the reset reopens Hurry
   with quote22. Full payment spends22, stores10, treasury3, no immediate Unit.
5. End another actual turn: year2121, Scout#5 created, production advances to
   Former with stored2, pending Scout/Scout, treasury4. The support panel shows
   the two real homed Scouts. A further actual save writes828678bytes.

Both accepted renderer sessions shut down normally with no tracked memory or
OpenGL leaks; heap allocations were not checked by memorydebug. External visual
evidence includes `hurry-partial-preview-ab9a736f.png`,
`hurry-paid-ab9a736f.png`, `hurry-restored-guard-ab9a736f.png`,
`hurry-reset-ab9a736f.png`, `hurry-full-paid-ab9a736f.png` and
`hurry-completed-ab9a736f.png`. Original assets and campaign saves stay outside
Git. WSLg software rendering is not native Windows performance validation.

The broader inspector remains unfinished: governor/priorities, Psych/citizen
lifetimes, ecology, commerce, nerve stapling, wider production/workshop and
original chooser/dialog visual fidelity remain separate work. This closes the
accepted ordinary local Hurry command, not the full inspector audit or release.
