# Base production cost and forecast

The owned canonical production presentation resolves current cost from the same
actual faction/world/INDUSTRY and unit/facility cost provider as production and
Hurry. The inspector shows stored minerals against that cost, remaining minerals
floored at zero for funded/overfilled stores, and the current mineral surplus.
It does not repost upkeep, mutate the queue or touch RNG. Foreign identity and
queue display do not grant the private local cost/forecast provider; unresolved
legacy records remain unavailable. Stockpile and other unsupported live
categories do not acquire invented ordinary-block authority.

The display follows the frozen [native forecast receipt](behavior/base-production-forecast.md)
and its fourteen literal cases. It uses current stored minerals and current
recalculated surplus, not switching baselines, pre-orbital surplus or retained
upkeep values. Nonpositive surplus displays `No Minerals` even if funded.
Positive surplus with funded stores displays `TURNS:  0`; otherwise the exact
signed32 wrapped numerator, denominator clamp and result clamp apply. There is
no riot override or extra growth-style turn. Zero total cost omits the ordinary
forecast block. The installed English label casing/spacing is preserved; this
is not a future-turn simulation or localization-system implementation.

The optional signed32 surplus argument on the read-only production presentation
is explicitly formatting input. It grants no gameplay authority. The inspector
passes the surplus from its already prepared current resource calculation,
avoiding another complete resource query solely to format the label. Cost and
stored minerals remain native fields. Calls without surplus do not invent one.

Original production artwork/grid layout, installed-facility special display,
queue-wide forecast and future worker/facility/cost changes remain separate.

Validation at production checkpoint `90f1e06c`: eight distinct affected native
groups pass, including the fourteen literal labels and zero-cost/wide-domain
guards, actual native Tanks cost40/current store7/supplied current surplus4
showing9 despite switching baseline888, no-surplus omission, wide store and
installed-facility withholding, foreign/legacy views, immutable state/RNG and
three complete Base-screen groups. Affected GSE scripts cover exact labels,
signed/overfilled/zero-cost/extreme port-store display. The new native test is
registered and compiles through real CMake; Base.cpp/header correctness passes.
No new platform or sanitizer matrix was required for this read-only display
arithmetic slice; the signed wrap is implemented with defined unsigned math.

An actual installed-GOG Planet save is loaded through the real menu/Start with
the exact `90f1e06c` binary/scripts, 1024x768 SDL input and GL backbuffer capture:

- Former stores2/20, remaining18, current mineral surplus+2 and `TURNS:  9`.
- Actual Doctor commit changes surplus to+1 and forecast18. Stores2, queue,
  treasury4 and year2121 remain unchanged. This distinguishes current display
  surplus from the retained upkeep pass and does not post another turn.
- Actual Worker commit restores current surplus+2 and forecast9.
- Normal shutdown exits0 with no tracked memory/OpenGL leaks; heap allocations
  were not checked by memorydebug.

External screenshots are `production-forecast-90f1e06c.png`,
`production-forecast-doctor-90f1e06c.png` and
`production-forecast-worker-90f1e06c.png`. Original assets/save remain outside
Git. This validates current panel wiring and refresh, not complete original
Base-screen pixels or native Windows performance. It does not close the full
inspector audit: governor, Psych, ecology, commerce, nerve stapling, broader
production/workshop and original artwork remain separate work.
