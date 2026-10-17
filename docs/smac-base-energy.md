# Base energy calculation

`SmacBaseEnergy` implements the measured energy stages used after raw tile and
orbital intake: nearest completed owned Headquarters distance, inefficiency
waste, allocation, resolved specialist output, facility/project multipliers,
unequal-allocation penalties, and Stockpile conversion. Results are owned
values. Querying them neither credits a faction nor submits research.

Rates are explicit psych/labs tenths totaling at most ten. Allocatable energy
is clamped to 0..9999 before specialists. Specialist outputs are supplied in
stored order (at most sixteen), after any required replacement/allocation.
The tail admits nonnegative categories and flat labs. Evaluated signed32
arithmetic overflow rejects; this is a port domain restriction, not a claim
about unmeasured original integer overflow. Waste's nonpositive-energy and
zero-distance exits precede its otherwise-required arithmetic.

HQ callers supply only authenticated completed owned facilities; queued and
foreign HQ filtering is not performed by the distance helper. Commerce is an
explicit amount between waste evaluations; Backbone accepts separately gated
contributions. The helper cannot infer diplomatic or project state. Facility
bits use canonical FACILITIES source IDs, independently of display names.

The pinned projector copies inputs/results from the human-owned original
receipts `RE-SMAC-ENERGY-WASTE-001`, `RE-SMAC-PSYCH-ALLOC-001` and
`RE-SMAC-ENERGY-TAIL-001`. Its 27/29/32 named cases cover all supplied named
rows. HQ owner filtering and Backbone gate reduction are explicit input
projections; citizen outcomes and sampled private matrices are not asserted.
No original executable or asset is included or rerun.

Four exact registered native helper groups pass in an optimized standalone
build of the actual production/test TUs; changed-production LLVM15 analysis
reports no diagnostics. This is focused validation, not a full engine or
platform run. An initial test macro spelling error was corrected before the
successful run; a subsequent shell CRLF log-display error did not change the
passing executable result, and the analysis was rerun with LF shell text.

## Live posting boundary

The owned-state and authenticated Base adapter are documented in
[smac-live-economy.md](smac-live-economy.md). They preserve the pure calculation's
boundary: calculating energy does not itself credit a faction or choose/award
technology. Normal upkeep posting remains disconnected until those consumers
are complete.

The later RE-SMAC-RESEARCH-SUBMIT-001 adjacent controls correct the initial
projector mapping: Hologram Theatre is source11; source10 is Skunkworks and does
not receive the psych multiplier. The regenerated existing fixtures and three
literal adjacent controls pass the same four native helper groups.
