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

Validation in progress: fourteen literal label cases plus zero-cost and wide
port-store rejection, owned/foreign/legacy native presentation and immutable
state/RNG, complete Base-screen fixtures, signed/overfilled GSE display cases,
changed Base.cpp correctness and exact-feature installed-asset rendering.
