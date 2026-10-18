# Leading-human dominance rule implementation

`SmacDominance` implements the read-only rule chain needed by regional strategy:
current Base population → council vote weights → potential majority, current
victory-pressure inputs → pressure, and strict admission → shortcut or score
comparison. It consumes owned-value inputs without refreshing game state.

The source contracts are [council votes](council-vote-counts.md),
[potential majority](potential-diplomatic-majority-selector.md),
[score admission](leading-human-dominance-score-path.md) and the completed
[victory-pressure shortcut](leading-human-victory-pressure-shortcut.md).

Vote construction preserves project order, faction modifier order, ignored
negative modifiers and signed truncation. Majority retains excluded votes in
the denominator, uses the querying faction's technology rather than the
candidate's, admits exact halves, and prefers the querying qualifier.
Positive exclusions read directed Pact bits; the original `-1` caller instead
reads bit0 of each candidate's Base-expansion/advice history when Cooperative
Victory is enabled.

Pressure reads future human economic deadlines even for inactive factions and
without a payment/rule/technology gate. Otherwise a non-absent Voice marker and
human ownership of its or Psi Gate's current required technology establish
pressure. The intermediate query0 majority cannot pass its positive-owner
technology gate. Pressure alone is insufficient: the admitted leader must also
be the final majority selection. A failed shortcut continues into score ratios.

The requirement IDs are caller-supplied current catalog values. The majority
requirement uses the existing resolved reference and immediate prerequisites
from the technology catalog, including its None/Disabled behavior. The pressure
branch currently admits concrete technology IDs, matching the installed-catalog
reference. No faction identity or installed technology ID is hardcoded in the
production implementation.

## Validation and limits

The compiled fixture records original receipt hashes and directly checks 23
vote cases, 26 majority cases, 22 score cases and 48 complete pressure contexts.
It uses a declared technology catalog with the reference's required/prerequisite
graph, not an original-game helper result. Earlier original 1,000/1,200-case
grids described by the source receipts are not additional port test executions.
Existing research-cost and manual-pool tests also pass. Targeted production
correctness analysis passes. Wide-arithmetic score controls and explicit
rejection of unmeasured signed32 vote overflow pass; no sanitizer/platform
matrix is needed for this pure arithmetic and array rule addition.

This patch does **not** yet connect the live regional summary. That integration
must resolve current project ownership and requirements, Base population,
retained rankings, campaign rules, all seven technology sets and the history
words from authoritative game state. In particular, do not substitute a fixed
faction birth turn for the history word or initialize legacy absent economic
history from an assumption. Saving, fresh/nonzero initialization, founding,
acquisition and advice writes remain separate implementation work. Council
ballots, winning elections and SMAX parity are not claimed here.
