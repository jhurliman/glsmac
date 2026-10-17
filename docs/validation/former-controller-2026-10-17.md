# Former/controller integration — 2026-10-17

The current integration connects saved ordinary Former work to selected-unit
continuation and owned terrain render updates. It also uses the captured native
selection distance/tie/record-order rules and retains a working Former across
upkeep while another unit is ready. Manual cycling and camera policy still
require the first-session visual comparison.

| Source and scope | Evidence |
| --- | --- |
| `df005547`, cached Linux Debug with allocation tracking | 82 affected authority/worker/manufacturing/restore groups passed; one new controller fixture failed before its intended behavior. |
| `4e99da48`, same cache, terrain-render filter | All three owned-patch, coast replay, improvement/neighbor and rollback groups pass; normal exit. |
| `50a5858f`, same cache, controller/selection filter | All four selected-controller and native-selection groups pass; normal exit. |

The controller fixture needed an owned viewport/sprite manager, correctly
separated accumulation phases and the existing write-admitted fixture scope.
The final four-group run exercises the actual queued work, stale turn stamp,
retired delayed callbacks, wrapped selection/ties and retained working Former.
No production validation guard was weakened to satisfy those tests. The coastal
test's expected image also needed the required independent RNG input. Failed
attempts were followed by affected checks only, not a platform matrix rerun.

Review found and fixed a separate frontend error-request fallthrough before
terrain dispatch (`df005547`). Worker diplomacy also rejects native owner zero
as a Pact/Treaty exemption, including frame/knowledge admission before publish.
The corresponding focused PRs received GitHub Codex review; this integration
receipt is not a review claim for unrelated unpublished feature commits.

These checks do not establish GPU/interactive appearance, disk campaign saves,
complete combat, a complete match or SMAX parity. The next acceptance target is
the [first-session checkpoint](../first-session-checkpoint.md). Native Windows
and Mac runs are reserved for that combined checkpoint and platform-sensitive
work; no full matrix ran for this batch.

Logs are under `/home/jhurliman/src/glsmac-rover-runtime/`:
`first-session-authority-tests.log`, `first-session-render-tests.log`, and
`selected-controller-{head.txt,build.log,tests.log}`. Earlier fixture failures
remain in the preceding build/test logs for diagnosis.
