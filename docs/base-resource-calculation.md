# Live canonical nutrient and mineral preparation

`Base::PrepareSmacResourceCalculation` rebuilds raw tile plus orbital intake
from the actual center and current Pop assignments, then prepares nutrient and
mineral results. The ordinary `get_smac_resource_calculation()` wrapper returns
a detached copy, and the canonical Base screen consumes its processed N/M
intake and demand. Existing `get_intake()` remains raw intake.

The native result also accepts an explicit upkeep resource-query phase; it does
not infer that phase from the current turn or UI. This lets the upcoming upkeep
caller use the same preparation with the correct solar input.

## Owned inputs and arithmetic

The existing canonical adapters validate the registered Base, saved allocation
projection, bound owner, current/retained social inputs, frozen resource rules,
map semantics, satellites, facility source identities and actual homed units.
The saved `NoProjectsOrdinaryTerrain` scenario and the check of every registered
facility establish absent project effects. The support adapter admits actual
conventional Combat designs only, so this domain has no Supply/convoy package.
This is not a default inferred from empty optional data.

Nutrient demand uses the configured per-citizen requirement. The result contains
intake, demand, surplus, stored food before processing and a separately prepared
normalized store. Deficit with exactly zero stored food prepares -1; nonnegative
surplus with negative stored food prepares zero. Queries never install this
normalization or collect surplus. Population growth/starvation and the original
recalculation cache are not activated.

Actual local facility roles 19 (Robotic Assembly), 20 (Nanoreplicator),
21 (Quantum Converter) and 22 (Genejack) each add one half-bonus. The calculation
multiplies raw mineral intake by the summed factor once and divides toward zero
once, then subtracts actual home support. Stored production, if present, is
validated. Without Space Elevator the target does not change this stage, so
final and pre-orbital surplus are equal; no target buildability or completion
lookup is invented.

Raw intake accumulates with checked signed64 arithmetic. N/M processing admits
signed32 inputs, intermediate products and outputs; unsupported extremes reject
before any Base changes. Existing signed64 stored food is preserved. Raw energy
is available for the existing display, but energy processing, ecology, credits,
mineral shortfall actions and full balance/turn posting are not claimed.

## Focused checks

Nine admitted literal cases from `RE-SMAC-BASE-M-001` run through actual Base
intake/facility/home-support producers: 001, 002, 005-009, 018 and 031. They cover
rounding once, combined facilities, negative intake and negative surplus. The
remaining original cases require project/convoy authorities outside this live
domain. These are retained original-checked expectations, not a new original
execution or a comparison of every named/grid case.

Six nutrient-normalization boundaries check detached copies and exact unchanged
world/queued presentation state. A composed Base uses a worked forest, saved
orbital counts, two mineral facilities, a real homed Scout, stored food and an
orbital production category; ordinary/upkeep solar inputs stay distinct. The
complete real Base-screen scripts display a Genejack-adjusted mineral value
while preserving stored food across redraws. Missing authority and late overflow
reject without RNG or world mutation; existing raw/legacy intake tests remain.

The cached incremental build passed all 17 affected tracked native groups with
`--memorydebug` (`Canonical N M|Canonical orbital|Canonical resource intake`).
Targeted LLVM15 analysis of `BaseResourceCalculation.cpp` and `Base.cpp` passed
both translation units with zero findings in 3.07 seconds. No full platform or
sanitizer matrix was rerun for this arithmetic/caller change.
