# Installed facility details

Clicking an installed facility row now opens a read-only detail dialog. The
ordinary SMAC view uses its explicit frozen FACILITIES identity for the name,
resolved technology prerequisite and owner-specific construction cost. Current
upkeep comes from the existing native energy calculation, including faction
free-facility exemptions. Registered description and artwork are presentation
metadata; the detail view does not claim every described ability is implemented.
No rule effect, sale, production selection or treasury posting occurs.

Unresolved source/description/artwork or unavailable private providers remain
explicitly unavailable. Foreign Bases retain public registered/source metadata
but do not expose owner-specific construction pricing or current fees. A removed
facility has no detail result. The getter requires a registered installed
definition. The dialog closes on Base refresh/hide and required question/turn
transitions; hidden Base screens clear their facility wrapper/list references.

Five focused native groups pass: actual frozen metadata versus deliberately
incorrect presentation cost/name; paid/free upkeep; pointer row, Close, Escape,
reopen, required turn and refresh callbacks; absent metadata; foreign privacy;
full nutrient/energy screen endpoints and retained production display. One
existing energy GSE script also passes. These are five distinct native groups,
with paid/free scenarios inside one group. Changed Base.cpp correctness analysis
passes. Existing unaffected analyses are reused; no platform/sanitizer matrix.

## Actual renderer acceptance

Production head ee190715, cached independent Linux renderer with WSLg/llvmpipe,
installed GOG assets and actual1024x768 backbuffer. Loaded the existing ordinary
Planet campaign through the real Load/Start path, opened the Base inspector and
navigated from planet archive to Gaia's Landing in2121 (credits4).

Clicking Headquarters opened original fac001 artwork, Construction50 minerals,
Current upkeep0 energy/turn, RequiresNone and the full wrapped registered
explanation. Close and reopen produced a byte-identical640x340 dialog (zero
changed pixels); second Close and BaseOK returned to the map. No turn was
advanced. Normal shutdown returned0, with no tracked memory/OpenGL leaks;
heap memorydebug was not enabled. Native snapshots/RNG/frontend controls provide
the stronger no-mutation check.

External screenshots under .codex-validation/playable-planet-inspector:
facility-details-ee190715.png, facility-details-reopened-ee190715.png and
facility-details-closed-ee190715.png. Logs: facility-details-native3.log,
facility-details-native4.log, facility-details-energy-gse.log,
facility-details-correctness.log and facility-details-render-ee190715.log.
The cached overlay reuses frozen8622f7cd objects only where unchanged, rebuilding
changed production/test files and affected header consumers. Binary and loaded
production scripts correspond to ee190715; the following receipt commit changes
only this documentation.

Two earlier render attempts are excluded from acceptance. The first exposed
that decorative Text does not subscribe pointer events; interactive Area rows
now own input. The second exposed that Text has measured dimensions rather than
Area height properties; those invalid assignments were removed. The focused
DOM double rejects both errors. A foreign test fixture initially used empty
slots1/2; it now uses the actual non-dense participant slot6, preserving guards.

The dialog layout and left-click detail interaction are provisional port UI,
not an original-game screenshot match or original facility-list command parity.
Selling, Datalinks/Workshop integration, Governor, full Psych/ecology and other
inspector endpoints remain separate work. Original assets/saves stay outside Git.
