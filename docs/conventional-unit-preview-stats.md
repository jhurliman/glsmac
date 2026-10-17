# Conventional Unit bottom-bar stats

The default bottom bar now consumes conventional Units through their checked
copied query. Those Units deliberately leave the old mutable `morale` and
`movement` wrapper fields undefined; the previous preview tried to compare
that undefined morale with a native-life label index and failed.

`query_conventional_stats(mode)` now also returns an owned `morale_name` string
from the canonical Unit definition's morale set at the checked effective index.
This is an additive query result, not another authoritative field or a saved
format change. The UI passes the Unit's explicit stored render mode. Modes 0/1
coincide for the currently admitted homeless context; this change does not infer
a home, riots or a different morale policy.

The preview's `Moves` line converts fresh remaining allowance to display units
using the queried movement quantum. The summary and object-list labels instead
use fresh full allowance, including the existing elite/damage calculation. Both
use the existing two-decimal display style. Attack and armor remain `?`; this
query does not provide those values. Conventional display never uses the static
definition's legacy `movement_per_turn` field. A repeated `unit_preview` event
refreshes the existing same-tile list item's text without replacing its widget.
Native-life labels, legacy movement formatting, the Spore Launcher prefix and
the immovable-unit omission of `Moves` retain their existing paths.

No default spawning, renderer, selection authority, movement command, gameplay
arithmetic, social state or snapshot codec changes are included.

## Regression boundary

Two native groups initialize the complete production GSE bottom bar and register
its actual Game/Map callbacks. They supply real conventional Infantry, selected
Spartan Rover and legacy Unit wrappers. Only DOM/assets, sound and timer
construction are doubled; the tests do not claim rendered pixels or a graphical
startup. The fixture emits the same `unit_preview` and `unit_select` events as
the frontend and checks the preview, existing list item and selected identity.

Cases cover spending, elite promotion, damage, overspending, retained/current
rating disagreement, both explicit render modes, switching back to a native
Unit, immovable native Units and a later rejected query context. The definition's
deliberately incorrect movement999 exposes accidental fallback to presentation
metadata. A previously returned morale label remains unchanged after promotion.
Every successful display checks complete saved world state, IDs, both RNG
streams and the frontend queue for unintended changes. Rejected context checks
raw Unit/RNG preservation without claiming a complete-world rollback.

The corrected old-script baseline fails both groups at `Undefined >= Int`.
Three copied-script controls each fail both groups: using static-definition
movement, showing full allowance as remaining, and omitting list refresh.
Controls run in separate script trees; intended source and binaries are retained.

## Validation

Source parent is `155e53a9812cad4e5a8b5a590f1a2c50c02c5587`. The full normal
suite passed 8/8 in 96.33 seconds. The full ASan/UBSan/LSan suite passed 8/8 in
175.68 seconds with leak detection and halt-on-error enabled. Both focused
groups also passed with allocation tracking in each build. LLVM 15 analysis of
the one changed production translation unit completed with zero failures in
1.85 seconds. All three copied-script controls failed both groups at their
intended assertions; no intended source or binary was mutated for those runs.

Local logs and copied controls are retained under
`/home/jhurliman/src/glsmac-conventional-unit-preview/`. Initial test-harness
corrections covered GSE index syntax and copied DOM-double handles, expected
floating-point text (`1.0`), and the fact that homeless modes 0/1 coincide. Those
failed attempts are retained separately and are not counted as production
defect controls.
