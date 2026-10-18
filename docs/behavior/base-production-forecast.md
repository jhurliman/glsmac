# Base production forecast

2026-10-18, stock SMAC. Original-code execution of fourteen small display
contexts and one facility-cost producer; no full window or pixel comparison.
Literal observations are in [the cases](base-production-forecast-cases.json).

The forecast reads the **current recalculated mineral surplus** and current
stored minerals. It does not use switching-baseline minerals, the earlier
pre-orbital surplus, or a separately retained upkeep result. Drawing itself
does not recalculate resources. Ordinary opening and worker/specialist changes
reach the resource recalculation before drawing; an isolated redraw consumes
the fields currently present. Follow [the cache contract](base-recalculation-and-cache.md).

Cost is the selected item's actual total mineral cost: unit/design cost times
the faction mineral factor, or facility/project rules cost times that factor.
Both draw paths pass the no-specific-Base context to the factor helper.
The native facility producer returned cost40 from rules cost4 and factor10.
See [mineral factors](mineral-cost-factors.md) and
[unit costs](unit-design-and-prototype-costs.md).

For an ordinary unbuilt item, let C be actual cost, M current stored minerals,
and S current mineral surplus:

- If S <= 0, display `No Minerals`, including when M >= C.
- Otherwise, if M >= C, display `TURNS:  0`.
- Otherwise, for normal positive values, display `TURNS:  ` followed by
  ceil((C-M)/S). There is no extra growth-style turn and no minimum of one
  for an already funded item. The one-turn case still says `TURNS`.

Exact diagnostic arithmetic is signed32: D=C-M, numerator=S+D-1 with32-bit
wrap, denominator=clamp(S,1,99999), signed division toward zero, then result
clamped0..999999. Only the denominator clamps S; numerator keeps original S.
Thus C200000/M0/S100000 displays3, while the ordinary mathematical ceiling is2.
C2147483647/M0/S2 wraps the numerator and displays0. These are diagnostic
integer boundaries, not claims that ordinary play reaches such values.

Existing riot status is not checked by this label. C40/M7/S4 displays
`TURNS:  9` with or without riot; M40 displays0 with or without riot. Actual
[production posting](production-posting-and-unit-creation.md) suppresses the
surplus addition during riots, but an already funded tested local facility can
still complete. Keep display and completion decisions separate.

The label uses installed localized `turns`, uppercases it, then appends a colon
and **two spaces** before the number. `No Minerals` retains installed casing.
Fourteen label executions preserve the entire Base record and consume no RNG.
Inputs deliberately keep the earlier surplus777 and switching baseline888
different from current S/M. No full-turn forecast, future cost changes, queue
forecast, artwork or complete Stockpile/already-built-item panel is claimed.

Zero total cost skips this ordinary cost-box/forecast block. Separate earlier
branches can show the already-built label for installed facilities/projects;
the fourteen controls enter the ordinary forecast branch after that decision.
