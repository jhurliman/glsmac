# Literal regional Q and initial7/8 fixtures

RE-SMAC-REGIONAL-INITIAL-LITERALS-001, 2026-10-18. Pinned stock SMAC.
The [32 literal rows](regional-initial-strategy-literals-cases.json) provide
explicit numeric inputs and native observed Q/initial7/8 outputs for the
[regional decision kernel](regional-research-summary-producer.md).
**Expectations are copied from original execution; no decision model generates
them.** No broad matrix is rerun.

Seven rows are projections of the existing complete-owner-summary corpus whose
checker already validated1,736 pressure comparisons. Their input caches come
from the stage snapshot; Q and pressure come from the native pre-store
observation, and initial7/8 comes from the immediate post-store observation.
They include the original rating2/rating1, G−F4/3, flag8, two-Base and peer
presence cases. These are initial codes before later peer/sea overrides.

Twenty-five small additional controls execute the actual **post-dominance
Q-to-initial-store fragment**. Dominance0/1 and completed-peer H are explicit
primitive boundary inputs; these cases do not execute their upstream helpers,
make new lifecycle claims or supply a replacement Q/pressure result. All Q
arithmetic, native signed comparisons/division and initial stores execute.

## Fields and complete numeric baseline

Each row includes all these integer fields, without implicit overrides:

| Field | Meaning | New-fragment baseline |
| --- | --- | ---: |
| C | Owned regional Base count |3 |
| N | Retained total Base count |4 |
| G / F / Z | Owned / covered / neutral suitable counts |10 /0 /20 |
| R | Retained maximum site rating |2 |
| Explore / Build | Signed stored priorities |0 /0 |
| H | Present positive factions from completed peer scan |1 |
| worldTiles | Tile-count scalar |3200 |
| turn | Elapsed game turn |75 |
| dominance | Resolved dominance result |0 |
| state_bit256 / regional_flag8 | Respective retained bits,0/1 |0 /0 |

Baseline native output is **Q5, initial8**. Individual rows contain full
resolved inputs, not just the differences shown in the index below.

## Compact discriminator index

| Native cases | Observed Q and initial strategy |
| --- | --- |
| First comparison equality: baseline; then Z19 |5/8;5/7 |
| Same Z19 with retained bit256 set |3/8 |
| N4→5; worldTiles3200→3201; turn75→76; dominance0→1; H1→2, separately from baseline | Each changed case6/7 versus baseline5/8 |
| Explore1; Build1 |5/7;5/8 |
| Build6; Explore−6 |5/8 in both signed-priority diagnostics |
| C3,G16,Z0 (second comparison equality); then G15 |5/8;5/7 |
| G15,Z0 with C2 /3 /4 |5/8;5/7;5/7 |
| R3→4 with Z19 |5/7→4/8 |
| C1,G4,Z1000,F0→1 (G−F4→3) |5/8→5/7 |
| G0,F5,Z0,C3 |5/7; native quotient−2, denominator2 |
| C19→20 with G1000,Z10000 |5/8→5/7 |
| C0 with low R, flag8 and negative G−F |5/8; empty-region guard bypasses pressure |
| N−1 diagnostic |5/8; no unsigned promotion into N>4 |

The C2/3/4 witnesses record native denominators1/2/2, distinguishing the odd-C
ceiling from floor division. Division metadata is null when its branch was
not executed; it does not mean quotient0. The negative G−F case records
−5/2→−2, but its initial7 is already forced by G−F<4. It is an intermediate
signed-division witness, **not** an initial-output discriminator for that rounding
alone. Negative priorities/N are diagnostic primitive inputs, not assertions
that ordinary startup produces them.

The reused complete-summary presence rows additionally expose all seven
retained Base-presence bytes. `[1,0,0,0,0,0,0]` produces native H1; adding peer2
presence produces H2. Self is included. H in the new fragment rows is an
already-resolved scalar, not a new peer-presence producer test.

The JSON declares entry scope per row. The fragment rows do not certify
full-summary admission, first-command lifecycle, reachability, dominance
arithmetic or final strategy after overrides. Existing producer/retained-input
receipts continue to own those boundaries.
