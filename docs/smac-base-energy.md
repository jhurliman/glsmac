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

The next adapter must authenticate the Base's final allocation, current
EFFICIENCY, actual facilities/HQ, and current/baseline production. Existing
Base calculations already authenticate the supported no-project campaign and
actual Unit packages; no energy convoy package is admitted there.

The current simulation has no owned allocation rates, treasury, pending debit,
or flat-faction-labs input. Worker Pact-or-Treaty exemptions are not a complete
commerce authority. The existing technology record owns research progress,
target and cached threshold, but has no complete positive research submission
operation (choice, completion and Secrets state remain separate). These inputs
must not be silently inferred by an energy query.

`RE-SMAC-TWO-BASE-POSTING-001` establishes final per-Base research submission
(including zero) before economy credit/maintenance, followed by a once-only
faction debit settlement. It supplies noncompleting research state and rates;
it does not make raw energy or gross statistics spendable income. The new
calculation alone does not activate this posting or claim complete upkeep.
