# Ranking and dominance share retained score storage

2026-10-18, stock SMAC. **Yes: full ranking's consumed eight-word score table
is the same table indexed by dominance. Do not publish fresh positive scores
at the end of a full ranking update.**

One continuous native path extends the existing ordinary-parameter, single
active Gaians startup receipt. It runs global advance, full ranking, owner
upkeep, the complete owner summary and first command boundary, with the same
declared headless services as that receipt. New observations replace no score,
ranking, summary or dominance result. [Literal cases](ranking-dominance-score-order-cases.json).

| Boundary | Turn | Scores, slots0..7 | Leader / runner |
| --- | --- | --- | --- |
| Scores constructed, before ranking selection | 0 | 0,1,0,0,0,0,0,0 | Earlier references |
| Full ranking returns | 0 | 0,-1,-1,-1,-1,-1,-1,-1 | 1 / 0 |
| Owner summary enters | 1 | 0,-1,-1,-1,-1,-1,-1,-1 | 1 / 0 |
| Each of124 regional dominance call sites | 1 | 0,-1,-1,-1,-1,-1,-1,-1 | 1 / 0 |
| First command boundary | 1 | 0,-1,-1,-1,-1,-1,-1,-1 | 1 / 0 |

Exactly one score-provider entry occurs, with full-update argument1. There is
**no intervening score-only argument0 refresh** anywhere in this observed path.
Owner summary rebuilds its own Base/population/regional inputs; that does not
reconstruct this score table. The global full update precedes the turn increment.

The table's slot0 is retained and skipped by ordinary score construction.
The full rank-selection loop marks selected positive-ID slots−1, including
inactive zero-score slots in this ordinary nonnegative-score profile. Do not
generalize this profile to overflow or unusual negative scenario scoring.

Dominance directly indexes the current table through the retained leader and
runner IDs; it does not call the score provider. Early-turn admission can return
before those lookups. Therefore the124 calls prove ordering and table lifetime,
not124 score-comparison executions or a universal dominance result. The existing
[slot0 receipt](dominance-runner-slot0-score.md) separately executes admitted
score comparisons after explicit score-only updates. Those controls deliberately
refresh the table; they are not evidence of an implicit refresh in owner upkeep.

An adapter may retain separate explanatory score components, but native gameplay
consumers must see the actual retained table. Only a real subsequent score-provider
invocation can publish freshly constructed values. Existing
[ending input producers](ending-input-producers.md) and
[ordinary score construction](ordinary-ranking-score.md) retain their narrower scopes.
